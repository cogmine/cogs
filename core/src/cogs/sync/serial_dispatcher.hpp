//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SERIAL_DISPATCHER
#define COGS_HEADER_SYNC_SERIAL_DISPATCHER


#include "cogs/function.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/wait_priority_queue.hpp"
#include "cogs/sync/semaphore.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief A dispatcher that defers tasks to another dispatcher serially.  Ensures no tasks execute in parallel.
class serial_dispatcher : public dispatcher, public object
{
private:
	class serial_dispatch_state
	{
	public:
		int m_scheduledPriority;
		unsigned int m_flags;
	};

	alignas (atomic::get_alignment_v<serial_dispatch_state>) serial_dispatch_state m_serialDispatchState;
	rcptr<task<void> > m_expireTask;
	boolean m_expireDone;

	weak_rcptr<volatile dispatcher> m_nextDispatcher;

	class serial_dispatched : public dispatched
	{
	public:
		priority_queue<int, ptr<serial_dispatched> >::remove_token m_removeToken;

		// m_removeToken is accessed in a thread-safe way.  Cast away volatility
		priority_queue<int, ptr<serial_dispatched> >::remove_token& get_remove_token() volatile { return ((serial_dispatched*)this)->m_removeToken; }
		const priority_queue<int, ptr<serial_dispatched> >::remove_token& get_remove_token() const volatile { return ((const serial_dispatched*)this)->m_removeToken; }

		serial_dispatched(const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t, const priority_queue<int, ptr<serial_dispatched> >::remove_token& rt)
			: dispatched(parentDispatcher, t),
			m_removeToken(rt)
		{ }
	};

	priority_queue<int, ptr<serial_dispatched> > m_priorityQueue;

	virtual rcref<task<bool> > cancel_inner(volatile dispatched& d) volatile
	{
		volatile serial_dispatched& d2 = *(volatile serial_dispatched*)&d;
		const priority_queue<int, ptr<serial_dispatched> >::remove_token& rt = d2.get_remove_token();
		bool b = m_priorityQueue.remove(rt);
		if (b)
			serial_dispatch();
		return signaled(b);
	}

	virtual void change_priority_inner(volatile dispatched& d, int newPriority) volatile
	{
		volatile serial_dispatched& d2 = *(volatile serial_dispatched*)&d;
		priority_queue<int, ptr<serial_dispatched> >::remove_token& rt = d2.get_remove_token();
		m_priorityQueue.change_priority(rt, newPriority);
		serial_dispatch();
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		priority_queue<int, ptr<serial_dispatched> >::preallocated i;
		auto r = m_priorityQueue.preallocate_key_with_aux<delayed_construction<serial_dispatched> >(priority, i);
		serial_dispatched* d = &(r->get());
		i.get_value() = d;
		placement_rcnew(d, *r.get_desc())(this_rcref, t, i);
		m_priorityQueue.insert_preallocated(i);
		rcref<dispatched> d2(d, i.get_desc());
		t->set_dispatched(d2);
		i.disown();
		serial_dispatch();
	}

	constexpr static int serial_dispatch_busy_flag = 0x01;      // 00001
	constexpr static int serial_dispatch_dirty_flag = 0x02;     // 00010
	constexpr static int serial_dispatch_scheduled_flag = 0x04; // 00100
	constexpr static int serial_dispatch_expired_flag = 0x08;   // 01000
	constexpr static int serial_dispatch_hand_off_flag = 0x10;  // 10000

	void serial_dispatch() volatile
	{
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			if ((oldState.m_flags & serial_dispatch_dirty_flag) != 0)
				break;

			serial_dispatch_state newState = oldState;
			bool own = (oldState.m_flags & serial_dispatch_busy_flag) == 0;
			if (own)
				newState.m_flags |= serial_dispatch_busy_flag;
			else
				newState.m_flags |= serial_dispatch_dirty_flag;

			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			if (!own)
				break;

			rcptr<volatile dispatcher> nextDispatcher = m_nextDispatcher;
			if (!nextDispatcher)
				nextDispatcher = thread_pool::get_default_or_immediate();
			nextDispatcher->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<volatile serial_dispatcher> r2 = r;
				if (!!r2)
					r->serial_update();
			}, const_min_int_v<int>); // best possible priority
			break;
		}
	}

	void serial_expire() volatile
	{
		if (!m_expireDone.compare_exchange(true, false))
			return;

		m_expireTask.release();
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & serial_dispatch_expired_flag) == 0);
			serial_dispatch_state newState = oldState;
			newState.m_flags &= ~serial_dispatch_scheduled_flag & ~serial_dispatch_hand_off_flag & ~serial_dispatch_dirty_flag; // remove scheduled flag, and hand-off flag if it was present
			newState.m_flags |= serial_dispatch_busy_flag | serial_dispatch_expired_flag;
			bool own = ((oldState.m_flags & serial_dispatch_busy_flag) == 0) || ((oldState.m_flags & serial_dispatch_hand_off_flag) != 0);
			if (!own)
				newState.m_flags |= serial_dispatch_dirty_flag;

			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			if (own)
				serial_update();
			break;
		}
	}

	void serial_update() volatile
	{
		serial_dispatch_state oldState;
		serial_dispatch_state newState;
		priority_queue<int, ptr<serial_dispatched> >::value_token vt;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & serial_dispatch_hand_off_flag) == 0);
			if ((oldState.m_flags & serial_dispatch_dirty_flag) != 0) // Immediately remove the retry tripwire
			{
				newState = oldState;
				newState.m_flags &= ~serial_dispatch_dirty_flag;
				if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
					continue;
				oldState.m_flags = newState.m_flags;
			}

			vt = m_priorityQueue.peek();
			int priority = 0;
			bool removeScheduled = false;
			if (!vt)
			{
				if ((oldState.m_flags & serial_dispatch_scheduled_flag) == 0) // Nothing scheduled, done if we can transition out
				{
					newState = oldState;
					newState.m_flags &= ~serial_dispatch_busy_flag & ~serial_dispatch_expired_flag;
					if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						continue;
					break;
				}

				removeScheduled = true;
			}
			else
			{
				priority = vt.get_key();
				if ((oldState.m_flags & serial_dispatch_scheduled_flag) != 0)
				{
					if (priority == oldState.m_scheduledPriority) // Something the same priority is already scheduled, done if we can transition out
					{
						newState = oldState;
						newState.m_flags &= ~serial_dispatch_busy_flag;
						if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
							continue;
						break;
					}
					removeScheduled = true;
				}
			}

			if (removeScheduled)
			{
				bool canceled = m_expireDone.compare_exchange(true, false);
				if (canceled) // This is only thread that will schedule/unschedule
				{
					m_expireTask->cancel();
					m_expireTask.release();
					for (;;) // Nothing is scheduled now.  Remove scheduled bit foracbly.
					{
						newState = oldState;
						newState.m_flags &= ~serial_dispatch_scheduled_flag;
						newState.m_flags &= ~serial_dispatch_dirty_flag; // slight efficiency
						if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
							continue;
						oldState.m_flags = newState.m_flags;
						break;
					}
					continue; // Start from the beginning, in case other flags have changed
				}
				for (;;) // Try to release update to expiring thread, if it doesn't expire before we get the chance
				{
					newState = oldState;
					newState.m_flags |= serial_dispatch_hand_off_flag;
					newState.m_flags &= ~serial_dispatch_dirty_flag; // slight efficiency
					if (atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						return;
					if (((oldState.m_flags & serial_dispatch_expired_flag) != 0)) // Already expired
						break;
					//continue;
				}
				continue; // Start from the beginning, in case other flags have changed
			}

			if ((oldState.m_flags & serial_dispatch_expired_flag) != 0)
			{
				if (priority <= oldState.m_scheduledPriority)
				{
					COGS_ASSERT(!!vt);
					if (!m_priorityQueue.remove(vt))
						continue;

					const ptr<serial_dispatched>& d = *vt;
					const rcref<task_base>& taskBase = d->get_task_base();
					if (!taskBase->signal())
						continue;

					rcptr<task<void> > chainedTask = taskBase->get_chained_task();
					if (!chainedTask)
						serial_resume();
					else
					{
						chainedTask->dispatch([r{ this_weak_rcptr }]()
						{
							rcptr<volatile serial_dispatcher> r2 = r;
							if (!!r2)
								r2->serial_resume();
						});
					}
					return;
				}

				for (;;) // Expired, but too soon.  Need to reschedule anyway.  Remove expired bit foracbly.
				{
					newState = oldState;
					newState.m_flags &= ~serial_dispatch_expired_flag;
					newState.m_flags &= ~serial_dispatch_dirty_flag; // slight efficiency
					if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						continue;
					oldState.m_flags = newState.m_flags;
					break;
				}
				continue; // Start from the beginning, in case other flags have changed
			}

			COGS_ASSERT(!!vt);
			newState.m_scheduledPriority = priority;
			newState.m_flags = oldState.m_flags & ~serial_dispatch_busy_flag;
			newState.m_flags |= serial_dispatch_scheduled_flag;
			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			((serial_dispatcher*)this)->m_expireDone = false; // don't need to set with atomicity, so cast away
			rcptr<volatile dispatcher> nextDispatcher = m_nextDispatcher;
			if (!nextDispatcher)
				nextDispatcher = thread_pool::get_default_or_immediate();
			m_expireTask = nextDispatcher->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<volatile serial_dispatcher> r2 = r;
				if (!!r2)
					r2->serial_expire();
			}, priority);
			break;
		}
	}

	void serial_resume() volatile
	{
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & serial_dispatch_expired_flag) != 0);
			COGS_ASSERT((oldState.m_flags & serial_dispatch_busy_flag) != 0);

			serial_dispatch_state newState = oldState;
			newState.m_flags &= ~serial_dispatch_dirty_flag & ~serial_dispatch_expired_flag;
			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			rcptr<volatile dispatcher> nextDispatcher = m_nextDispatcher;
			if (!nextDispatcher)
				nextDispatcher = thread_pool::get_default_or_immediate();
			nextDispatcher->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<volatile serial_dispatcher> r2 = r;
				if (!!r2)
					r2->serial_update();
			}, const_min_int_v<int>); // highest possible priority
			break;
		}
	}

	serial_dispatcher() = delete;
	serial_dispatcher(serial_dispatcher&) = delete;
	serial_dispatcher(serial_dispatcher&&) = delete;
	serial_dispatcher& operator=(serial_dispatcher&) = delete;
	serial_dispatcher& operator=(serial_dispatcher&&) = delete;

public:
	serial_dispatcher(const rcref<volatile dispatcher>& nextDispatcher)
		: m_nextDispatcher(nextDispatcher)
	{
		m_serialDispatchState.m_flags = 0;
	}

	~serial_dispatcher()
	{
		for (;;)
		{
			priority_queue<int, ptr<serial_dispatched> >::value_token vt = m_priorityQueue.get();
			if (!vt)
				break;
			(*vt)->get_task_base()->cancel();
		}
	}
};


}


#endif
