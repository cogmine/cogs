//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_WINDOW
#define COGS_HEADER_GUI_WINDOW


#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI button.
class window_interface
{
public:
	virtual void set_initial_shape(
		const std::optional<gfx::point> position,
		const std::optional<gfx::size> contentSize,
		const std::optional<gfx::size> frameSize,
		bool center) = 0;
	virtual void resize(const gfx::size& newSize) = 0;
	virtual void reshape_frame(const gfx::bounds& newBounds) = 0;
	virtual void frame_reshaped() = 0;
	virtual cell::propose_size_result propose_frame_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		cell::sizing_mask sizingMask = cell::all_sizing_types) const = 0;
	virtual bounds get_frame_bounds() const = 0;
	virtual void set_title(const composite_string& title) = 0;
};


/// @ingroup GUI
/// @brief A GUI window
class window : public pane_bridge, public virtual pane_container
{
public:
	typedef windowing::subsystem::window_options options;

private:
	friend class window_task;

	class window_task : public task<void>
	{
	public:
		window* m_window;

		explicit window_task(window* w)
			: m_window(w)
		{ }

		virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
		{
			return m_window->get_close_condition().timed_wait(timeout, spinCount);
		}

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			waitable::dispatch_inner(m_window->get_close_condition(), t, priority);
		}

		virtual rcref<task<bool> > cancel() volatile
		{
			m_window->close();
			return signaled(true);
		}
	};

	rcptr<window_interface> m_nativeWindow;
	window_task m_windowTask;
	volatile boolean m_closed;

	composite_string m_title;
	std::optional<color> m_backgroundColor;
	std::optional<gfx::point> m_position;
	std::optional<gfx::size> m_contentSize;
	std::optional<gfx::size> m_frameSize;
	bool m_center;

protected:
	virtual void installing()
	{
		auto nativeWindow = get_subsystem().template static_cast_to<volatile windowing::subsystem>()->create_window();
		pane_bridge::install_bridged(std::move(nativeWindow.first));
		m_nativeWindow = std::move(nativeWindow.second);
	}

	virtual void uninstalling()
	{
		pane_bridge::uninstalling();
		m_nativeWindow.release();
	}

	virtual void set_initial_shape()
	{
		m_nativeWindow->set_initial_shape(m_position, m_contentSize, m_frameSize, m_center);

	}

public:
	explicit window(options&& o)
		: pane_bridge({
			.children = std::move(o.content)
		}),
		m_windowTask(this),
		m_title(std::move(o.title)),
		m_backgroundColor(o.backgroundColor),
		m_center(o.center)
	{
		if (o.position.has_value())
			m_position = *o.position;
		if (o.contentSize.has_value())
			m_contentSize = *o.contentSize;
		if (o.frameSize.has_value())
			m_frameSize = *o.frameSize;
	}

	// Gets a task that can be used to wait for a window to go out of scope, or to close (cancel) the window
	rcref<task<void> > get_window_task() const
	{
		return this_rcref.member_cast_to(m_windowTask).const_cast_to<window_task>().static_cast_to<task<void> >();
	}

	void request_close()
	{
		dispatch([r{ this_rcref }]()
		{
			r->pane::request_close();
		});
	}

	void close()
	{
		dispatch([r{ this_rcref }]()
		{
			r->pane::close();
		});
	}

	virtual void hide()
	{
		dispatch([r{ this_rcref }]()
		{
			if (!r->is_hidden() || !r->m_closed) // Allow hide and show operations to progress until it becomes hidden, if 'closed'
				r->pane_bridge::hide();
		});
	}

	virtual void show()
	{
		dispatch([r{ this_rcref }]()
		{
			if (!r->is_hidden() || !r->m_closed)
				r->pane_bridge::show();
		});
	}

	const composite_string& get_title() const { return m_title; }

	void set_title(const composite_string& title)
	{
		m_title = title;
		if (!!m_nativeWindow)
			m_nativeWindow->set_title(title);
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child) { pane::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane::nest_first(child); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { pane::nest_before(beforeThis, child); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { pane::nest_after(afterThis, child); }

	using pane_bridge::reshape;

	// Bounds are in screen coordinates, not DIPS (not content coordinates, may be flipped).
	// Coordinates may be Cartesian on Mac.
	// To avoid invalid sizes, use propose_frame_size().
	virtual void reshape_frame(const bounds& newBounds)
	{
		if (!!m_nativeWindow)
			m_nativeWindow->reshape_frame(newBounds);
	}

	// Args are in screen coordinates, not DIPS (not content coordinates, ay be flipped).
	// Mainly provides a way to intercept new screen coordinates after a reshape.
	virtual void frame_reshaped(const gfx::bounds& newBounds, const gfx::point& oldOrigin = gfx::point(0, 0))
	{
		(void)newBounds;
		(void)oldOrigin;
		m_nativeWindow->frame_reshaped();
	}

	// Frame bounds are in screen coordinates, not DIPS (not content coordinates, may be flipped).
	virtual bounds get_frame_bounds() const
	{
		if (!!m_nativeWindow)
			return m_nativeWindow->get_frame_bounds();
		bounds b = { { 0, 0 }, { 0, 0 } };
		return b;
	}

	// Args are in screen coordinates, not DIPS (not content coordinates, ay be flipped).
	virtual propose_size_result propose_frame_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		if (!!m_nativeWindow)
			return m_nativeWindow->propose_frame_size(sz, r, resizeDimension, sizingMask);
		propose_size_result result;
		return result;
	}

	std::optional<size> propose_frame_size_best(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		bool nearestGreater = false,
		bool preferGreaterWidth = false,
		bool preferGreaterHeight = false) const
	{
		propose_size_result result = propose_frame_size(sz, r, resizeDimension);
		if (resizeDimension.has_value())
			return result.get_nearest(*resizeDimension, nearestGreater);
		return result.find_first_valid_size(get_primary_flow_dimension(), preferGreaterWidth, preferGreaterHeight);
	}

	virtual void drawing()
	{
		color c = m_backgroundColor.has_value() ? *m_backgroundColor : get_default_background_color();
		fill(get_size(), c);
	}
};

inline rcref<task<void> > windowing::subsystem::open(const rcref<pane>& p, const composite_string& title) volatile
{
	return open_window({ .title = title, .content = p })->get_window_task();
}

inline rcref<gui::window> windowing::subsystem::open_window(window::options&& options) volatile
{
	rcref<gui::window> w = rcnew(window)(std::move(options));
	install(*w, this_rcref);
	return w;
}


}
}


#endif
