//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_GUI_WINDOW
#define COGS_GUI_WINDOW


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

#pragma warning(push)
#pragma warning (disable: 4250)

/// @ingroup GUI
/// @brief A GUI window
class window : public pane_bridge, public virtual pane_container
{
private:
	composite_string m_title;

	function<bool()> m_closeDelegate;
	rcref<single_fire_event> m_closeEvent;
	rcref<task<void> > m_windowTask;
	volatile boolean m_closed;
	rcptr<window_interface> m_nativeWindow;

	friend class window_task;

	class window_task : public task<void>
	{
	public:
		weak_rcptr<window> m_window;

		window_task(const rcref<window>& w)
			: m_window(w)
		{ }

		virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
		{
			rcptr<window> w = m_window;
			if (!!w)
			{
				rcref<single_fire_event> closeEvent = w->m_closeEvent;
				w.release();	// Make sure we don't hold on to window, as it will close when out of scope.
				return closeEvent->timed_wait(timeout, spinCount);
			}
			return 1;
		}

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			rcptr<window> w = m_window;
			if (!!w)
			{
				rcref<single_fire_event> closeEvent = w->m_closeEvent;
				w.release();	// Make sure we don't hold on to window, as it will close when out of scope.
				dispatcher::dispatch_inner(*closeEvent, t, priority);
			}
			else
				dispatcher::dispatch_inner(*dispatcher::get_default(), t, priority);	// already closed
		}

		virtual bool cancel() volatile
		{
			rcptr<window> w = m_window;
			if (!!w)
				return w->close();
			return false;
		}
	};


protected:
	virtual void hiding()
	{
		pane_bridge::hiding();
		if (m_closed)
			m_closeEvent->signal();
	}

	virtual void installing()
	{
		auto nativeWindow = get_subsystem().static_cast_to<volatile windowing::subsystem>()->create_window();
		m_nativeWindow = std::move(nativeWindow.second);
		pane_bridge::install_bridged(std::move(nativeWindow.first));
	}

	virtual void uninstalling()
	{
		pane_bridge::uninstalling();
		m_nativeWindow.release();
	}

public:
	window(const composite_string& title, const function<bool()>& closeDelegate = []() { return true; })
		: m_title(title),
		m_closeDelegate(closeDelegate),
		m_closeEvent(rcnew(single_fire_event)),
		m_windowTask(rcnew(window_task, this_rcref))
	{
	}

	~window()
	{
		m_closeEvent->signal();
	}

	// Gets a task that can be used to wait for a window to go out of scope, or to close (cancel) the window
	const rcref<task<void> >& get_window_task() const
	{
		return m_windowTask;
	}

	rcref<waitable> get_close_event() const
	{
		return m_closeEvent;
	}

	virtual bool close()
	{
		if (m_closed.compare_exchange(true, false))
		{
			dispatch([r{ this_rcref }]()
			{
				if (r->is_hidden())
					r->m_closeEvent->signal();
				else
					r->pane_bridge::hide();
			});
			return true;
		}
		return false;
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

	const function<bool()>& get_close_delegate() const { return m_closeDelegate; }

	const composite_string& get_title() const	{ return m_title; }

	void set_title(const composite_string& title)
	{
		m_title = title;
		if (!!m_nativeWindow)
			m_nativeWindow->set_title(title);
	}

	using pane::nest;
	using pane::nest_last;
	using pane::nest_first;
	using pane::nest_before;
	using pane::nest_after;
};

#pragma warning(pop)

}
}


#endif

