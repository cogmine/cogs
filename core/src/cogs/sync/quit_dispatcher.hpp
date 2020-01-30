//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_QUIT_DISPATCHER
#define COGS_HEADER_SYNC_QUIT_DISPATCHER


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/immediate_task.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


class thread_pool;


/// @ingroup Synchronization
/// @brief An object that facilitates the negotiation of application shutdown.
class quit_dispatcher : public dispatcher, public object
{
private:
	enum class state
	{
		running = 0,
		quit_request_pending = 1,
		quit_request_dispatched = 2,
		quit_accepted = 3
	};

	class content_t
	{
	public:
		state m_state;
		size_t m_refCount;
	};

	// Use thread_safe_transactable directly, as it provides an ABA solution.  cas_transactable does not.
	typedef thread_safe_transactable<content_t> transactable_t;
	transactable_t m_contents;
	single_fire_event m_event;
	rcref<volatile priority_dispatcher> m_priorityDispatcher;
	container_queue<function<void()> > m_abortCallbacks;

	typedef transactable_t::read_token read_token;
	typedef transactable_t::write_token write_token;

	void release_reference() volatile
	{
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			COGS_ASSERT(wt->m_refCount > 0);

			if ((wt->m_state != state::quit_request_pending) || (wt->m_refCount > 1))
			{
				wt->m_refCount--;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}

			// state::quit_request_pending with refCount == 1 falls through.
			wt->m_state = state::quit_request_dispatched;
			if (!m_contents.end_write(wt))
				continue;

			// try to dispatch a request.
			for (;;)
			{
				if (m_priorityDispatcher->invoke())
					break;

				// Nothing to invoke.  We're done if nothing interrupts transition of m_refCount to 0
				m_contents.begin_write(wt);

				// if refcount was 1, we are done.  Otherwise, something might be added.
				bool done = wt->m_refCount == 1;
				if (done)
					wt->m_state = state::quit_accepted;
				else
					wt->m_state = state::quit_request_pending;
				wt->m_refCount--;
				if (!m_contents.end_write(wt))
					continue;
				if (done)
				{
					m_abortCallbacks.clear();
					m_event.signal();
				}
				break;
			}
			break;
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_state == state::quit_accepted)
			{
				t->signal();
				break;
			}

			if (!m_contents.promote_read_token(rt, wt))
				continue;
			wt->m_refCount++;
			if (!m_contents.end_write(wt))
				continue;

			dispatcher::dispatch_inner(*m_priorityDispatcher, t, priority);

			release_reference();
		}
	}

	friend class thread_pool;

public:

	explicit quit_dispatcher(rc_obj_base& desc)
		: object(desc),
		m_event(desc),
		m_priorityDispatcher(rcnew(priority_dispatcher))
	{
		m_contents->m_refCount = 0;
		m_contents->m_state = state::running;
	}

	static rcptr<quit_dispatcher> get()
	{
		return singleton<quit_dispatcher, singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::must_call_shutdown>::get();
	}

	// Returns true if quit was requested, false if a request is already in progress or quit has completed.
	bool request() volatile
	{
		bool result = false;
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_state != state::running)
				break;

			if (!m_contents.promote_read_token(rt, wt))
				continue;

			wt->m_state = state::quit_request_pending;
			wt->m_refCount++;
			if (!m_contents.end_write(wt))
				continue;

			release_reference();
			break;
		}

		return result;
	}

	void force() volatile
	{
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_state == state::quit_accepted)
				break;

			if (!m_contents.promote_read_token(rt, wt))
				continue;

			wt->m_state = state::quit_accepted;
			if (!m_contents.end_write(wt))
				continue;

			m_priorityDispatcher->drain();
			m_abortCallbacks.clear();
			m_event.signal();
			break;
		}
	}

	// approve and deny must only be called by a quit_dispatcher-dispatched delegate.
	// One or the other must be called, or the quit will be stalled.  So, no use of weak references
	// to member delegate objects.

	// approve and deny will never be called in parallel.

	// quitAbortCallback can be handy for re-registering a handler if a subsequent handler denys the quit request.
	template <typename F>
	void approve(F&& quitAbortCallback) volatile
	{
		m_abortCallbacks.append(std::forward<F>(quitAbortCallback));
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_state == state::quit_accepted)
				break;
			COGS_ASSERT(rt->m_state == state::quit_request_dispatched);

			if (!m_contents.promote_read_token(rt, wt))
				continue;

			// Let the last thread out get the door
			wt->m_refCount++;
			wt->m_state = state::quit_request_pending;
			if (!m_contents.end_write(wt))
				continue;

			release_reference();
			break;
		}
	}

	// A handler can be re-registered prior to calling deny.
	void deny() volatile
	{
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_state == state::quit_accepted)
				break;
			COGS_ASSERT(rt->m_state == state::quit_request_dispatched);

			for (;;)
			{
				function<void()> f;
				if (m_abortCallbacks.pop(f))
				{
					f();
					if (m_contents.is_current(rt))
						continue;
				}
				break;
			}

			if (!m_contents.promote_read_token(rt, wt))
				continue;

			wt->m_state = state::running;
			if (!m_contents.end_write(wt))
				continue;
			break;
		}
	}

	// Called by outer main() glue to wait for all things blocking quit to complete.
	// Really not intended to be called from anywhere else.  So, probably a bug if you are calling it.
	const waitable& get_event() const { return m_event; }
};


// Requests that the application quit.
// The application may not quit.  It must first run all quit handlers.
// The user may yet abort a request to quit.
inline void request_quit()
{
	rcptr<quit_dispatcher> qd = quit_dispatcher::get();
	if (!!qd)
		qd->request();
}


// force_quit() is called when the application is quiting whether we like it or not.
// All quit_handler's are disposed of, and the quit is issued immediately.
inline void force_quit()
{
	rcptr<quit_dispatcher> qd = quit_dispatcher::get();
	if (!!qd)
		qd->force();
}


#include "cogs/sync/thread_pool.hpp"


}


#endif
