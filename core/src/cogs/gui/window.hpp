//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	virtual void set_initial_shape(const point* initialPosition, const size* initialFrameSize, bool centerPosition) = 0;
	virtual void reshape_frame(const bounds& newBounds, const point& oldOrigin = point(0, 0)) = 0;
	virtual bounds get_frame_bounds() const = 0;
	virtual void set_title(const composite_string& title) = 0;
};


/// @ingroup GUI
/// @brief A GUI window
class window : public pane_bridge, public virtual pane_container
{
private:
	friend class window_task;

	class window_task : public task<void>
	{
	public:
		window* m_window;

		window_task(rc_obj_base& desc, window* w)
			: task<void>(desc),
			m_window(w)
		{ }

		virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
		{
			return m_window->get_close_event().timed_wait(timeout, spinCount);
		}

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			waitable::dispatch_inner(m_window->get_close_event(), t, priority);
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
	point m_initialScreenPosition;
	size m_initialFrameSize;
	bool m_hasInitialScreenPosition;
	bool m_hasInitialFrameSize;
	bool m_initialPositionCentered;
	composite_string m_title;

protected:
	virtual void installing()
	{
		auto nativeWindow = get_subsystem().template static_cast_to<volatile windowing::subsystem>()->create_window();
		m_nativeWindow = std::move(nativeWindow.second);
		pane_bridge::install_bridged(std::move(nativeWindow.first));
	}

	virtual void uninstalling()
	{
		pane_bridge::uninstalling();
		m_nativeWindow.release();
	}

	virtual void set_initial_shape()
	{
		m_nativeWindow->set_initial_shape(
			m_hasInitialScreenPosition ? &m_initialScreenPosition : nullptr,
			m_hasInitialFrameSize ? &m_initialFrameSize : nullptr,
			m_initialPositionCentered);
	}

public:
	window(rc_obj_base& desc,
		const gfx::canvas::point* screenPosition,
		const gfx::canvas::size* frameSize,
		bool positionCentered,
		const composite_string& title)
		: pane_bridge(desc),
		m_windowTask(desc, this),
		m_hasInitialScreenPosition(screenPosition != nullptr),
		m_hasInitialFrameSize(frameSize != nullptr),
		m_initialPositionCentered(positionCentered),
		m_title(title)
	{
		if (m_hasInitialScreenPosition)
			m_initialScreenPosition = *screenPosition;
		if (m_hasInitialFrameSize)
			m_initialFrameSize = *frameSize;
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
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis) { pane::nest_before(child, beforeThis); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis) { pane::nest_after(child, afterThis); }

	using pane_bridge::reshape;

	virtual void reshape_frame(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		if (!!m_nativeWindow)
			m_nativeWindow->reshape_frame(newBounds);
	}

	virtual bounds get_frame_bounds() const
	{
		if (!!m_nativeWindow)
			return m_nativeWindow->get_frame_bounds();
		bounds b = { { 0, 0 }, { 0, 0 } };
		return b;
	}
};


inline rcref<task<void> > windowing::subsystem::open(
	const composite_string& title,
	const rcref<pane>& p) volatile
{
	return open_window(nullptr, nullptr, false, title, p)->get_window_task();
}

inline rcref<gui::window> windowing::subsystem::open_window(
	const gfx::canvas::point* screenPosition,
	const gfx::canvas::size* frameSize,
	bool positionCentered,
	const composite_string& title,
	const rcref<pane>& p) volatile
{
	rcref<gui::window> w = rcnew(window, screenPosition, frameSize, positionCentered, title);
	w->nest(p);
	install(*w, this_rcref);
	return w;
}


}
}


#endif
