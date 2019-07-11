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
	virtual void set_title(const composite_string& title) = 0;
};


/// @ingroup GUI
/// @brief A GUI window
class window : public pane_bridge, public virtual pane_container
{
private:
	composite_string m_title;

	volatile boolean m_closed;
	rcptr<window_interface> m_nativeWindow;

	friend class window_task;

	class window_task : public task<void>
	{
	public:
		window* m_window;

		window_task(const ptr<rc_obj_base>& desc, window* w)
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
			return get_immediate_task(true);
		}
	};

	window_task m_windowTask;

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

public:
	window(const ptr<rc_obj_base>& desc, const composite_string& title)
		: pane_bridge(desc),
		m_windowTask(desc, this),
		m_title(title)
	{
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
			if (!r->is_hidden() || !r->m_closed)	// Allow hide and show operations to progress until it becomes hidden, if 'closed'
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

	const composite_string& get_title() const	{ return m_title; }

	void set_title(const composite_string& title)
	{
		m_title = title;
		if (!!m_nativeWindow)
			m_nativeWindow->set_title(title);
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }
};


inline rcref<task<void> > windowing::subsystem::open(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& f) volatile
{
	return open_window(title, p, f)->get_window_task();
}

inline rcref<gui::window> windowing::subsystem::open_window(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& f) volatile
{
	rcref<gui::window> w = rcnew(window, title);
	w->nest(p, f);
	install(*w, this_rcref);
	return w;
}


}
}


#endif

