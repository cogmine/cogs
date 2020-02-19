//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_TIMER
#define COGS_HEADER_SYNC_TIMER


#include "cogs/collections/multimap.hpp"
#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/resettable_event.hpp"
#include "cogs/sync/semaphore.hpp"
#include "cogs/sync/singleton.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @defgroup Timers Timers
/// @{
/// @ingroup Synchronization
/// @brief Timer classes
/// @}


/// @ingroup Timers
/// @brief Base class for timer classes
class timer : public waitable, public object
{
public:
	typedef timeout_t::period_t period_t;

private:
	timer() = delete;
	timer(const timer&) = delete;
	timer& operator=(const timer&) = delete;

	class inner_timer : public object
	{
	private:
		inner_timer() = delete;
		inner_timer(const inner_timer&) = delete;
		inner_timer& operator=(const inner_timer&) = delete;

		class globals
		{
		public:
			volatile multimap<timeout_t, rcref<inner_timer> > m_timers;

			volatile rcptr<thread> m_timerThread;
			semaphore m_timerThreadSemaphore{ 0, 1 };
			bool m_terminating = false;

			~globals()
			{
				rcptr<thread> timerThread = m_timerThread;
				if (!!timerThread) // Will be false if spurious alloc in init race condition
				{
					m_terminating = true;
					m_timerThreadSemaphore.release();
					timerThread->join();
				}
			}
		};

		static rcptr<globals> init()
		{
			bool isNew;
			rcptr<globals> g = singleton<globals, singleton_posthumous_behavior::return_null>::get(isNew);
			if (isNew)
				g->m_timerThread = thread::spawn(&thread_main);
			return g;
		}

		static void thread_main()
		{
			rcptr<globals> acquiredTimerGlobals = init();
			if (!!acquiredTimerGlobals)
			{
				globals* myTimerGlobals = acquiredTimerGlobals.get_ptr(); // We know it's not going anywhere until this thread is joined, so don't reference it.
				acquiredTimerGlobals = 0;
				timeout_t timeout = timeout_t::infinite();
				do {
					myTimerGlobals->m_timerThreadSemaphore.acquire_any(timeout); // timeout of next timer
					multimap<timeout_t, rcref<inner_timer> >::volatile_iterator itor;
					for (;;)
					{
						itor = myTimerGlobals->m_timers.get_first();
						if (!itor || !!(itor.get_key().get_pending())) // Only bother with stuff that expired.
							break;
						rcref<inner_timer> t = *itor;
						myTimerGlobals->m_timers.remove(itor);
						read_token rt;
						for (;;) // Loop until transactable write succeeds
						{
							t->m_timeoutInfo.begin_read(rt);
							if (rt->m_aborted)
								break;
							if (!!rt->m_timeout.get_pending()) // Got extended.  Put it back in the list.
								myTimerGlobals->m_timers.insert(rt->m_timeout, t);
							else // expired
							{
								rcptr<timer> outerTimer = t->m_outerTimer;
								if (!outerTimer)
									break;
								if (!t->m_timeoutInfo.end_write(rt, timeout_info_t(rt->m_timeout, true, false)))
									continue;
								outerTimer->triggered();
							}
							break;
						}
						//continue;
					}
					if (!itor)
						timeout = timeout_t::infinite();
					else
						timeout = itor.get_key();
				} while (!myTimerGlobals->m_terminating);
			}
		}

		class timeout_info_t
		{
		public:
			timeout_t m_timeout;
			bool m_fired;
			bool m_aborted;

			timeout_info_t()
			{ }

			timeout_info_t(const timeout_info_t& src)
				: m_timeout(src.m_timeout),
				m_fired(src.m_fired),
				m_aborted(src.m_aborted)
			{ }

			timeout_info_t(const timeout_t& t, bool fired = false, bool aborted = false)
				: m_timeout(t),
				m_fired(fired),
				m_aborted(aborted)
			{ }
		};

	public:
		typedef transactable<timeout_info_t> transactable_t;
		volatile transactable_t m_timeoutInfo;
		const weak_rcptr<timer> m_outerTimer;

		typedef transactable_t::read_token read_token;
		typedef transactable_t::write_token write_token;

		timeout_t get_timeout() { return m_timeoutInfo.begin_read()->m_timeout; }
		period_t get_period() { return get_timeout().get_period(); }
		period_t get_expiration() { return get_timeout().get_expiration(); }
		period_t get_pending() { return get_timeout().get_pending(); }

		bool abort() // returns false if already fired
		{
			read_token rt;
			bool result = true;
			do {
				m_timeoutInfo.begin_read(rt);
				if (!!(rt->m_aborted))
					break;
				if (!!(rt->m_fired))
				{
					result = false;
					break;
				}
			} while (!m_timeoutInfo.end_write(rt, timeout_info_t(rt->m_timeout, false, true)));
			return result;
		}

		// call only from most derived class to avoid having virtual trigger() called before vtable is set up.
		// Not volatile, should be inherently serialized
		void defer()
		{
			rcptr<globals> timerGlobals = init();
			if (!!timerGlobals)
			{
				for (;;)
				{
					read_token rt;
					m_timeoutInfo.begin_read(rt);
					COGS_ASSERT(!rt->m_fired);
					timeout_t t = rt->m_timeout;
					if (!t.is_infinite())
					{
						if (t.expired())
						{
							rcptr<timer> outerTimer = m_outerTimer;
							if (!!outerTimer)
							{
								write_token wt;
								if (!m_timeoutInfo.promote_read_token(rt, wt))
									continue;
								wt->m_fired = true;
								if (!m_timeoutInfo.end_write(wt))
									continue;
								outerTimer->triggered();
							}
						}
						else
						{
							multimap<timeout_t, rcref<inner_timer> >::volatile_iterator itor = timerGlobals->m_timers.insert(t, this_rcref);
							if (itor == timerGlobals->m_timers.get_first())
								timerGlobals->m_timerThreadSemaphore.release();
						}
					}
					break;
				}
			}
			//else // if shutting down, new timers will NOT fire
				// trigger();
		}

		bool refire() // returns false if hasn't fired yet
		{
			read_token rt;
			bool result = true;
			for (;;)
			{
				m_timeoutInfo.begin_read(rt);
				if (!(rt->m_fired)) // can't refire if not fired
				{
					result = false;
					break;
				}
				COGS_ASSERT(!(rt->m_aborted)); // if it fired, it can't have been aborted.
				timeout_t newTimeout = rt->m_timeout;
				newTimeout.refire();
				if (!!m_timeoutInfo.end_write(rt, timeout_info_t(newTimeout, false, false)))
				{
					defer();
					break;
				}
				//continue;
			}
			return result;
		}

		bool refire(const timeout_t& t)
		{
			bool result = true;
			for (;;)
			{
				read_token rt;
				m_timeoutInfo.begin_read(rt);
				if (!(rt->m_fired)) // can't refire if not fired
				{
					result = false;
					break;
				}
				COGS_ASSERT(!(rt->m_aborted)); // if it fired, it can't have been aborted.
				if (!!m_timeoutInfo.end_write(rt, timeout_info_t(t, false, false)))
				{
					defer();
					break;
				}
				//continue;
			}
			return result;
		}

		bool try_extend(bool& wasAborted) // returns true if able to be extended, false if already fired.
		{
			bool result = true;
			read_token rt;
			do {
				m_timeoutInfo.begin_read(rt);
				wasAborted = rt->m_aborted;
				if (!!wasAborted)
					break;
				if (!!(rt->m_fired))
				{
					result = false;
					break;
				}
			} while (!m_timeoutInfo.end_write(rt, timeout_info_t(timeout_t(rt->m_timeout.get_period()), false, false)));
			return result;
		}

		void extend() // returns true if able to be extended, false if already fired.
		{
			read_token rt;
			do {
				m_timeoutInfo.begin_read(rt);
				if (rt->m_fired || rt->m_aborted)
					break;
			} while (!m_timeoutInfo.end_write(rt, timeout_info_t(timeout_t(rt->m_timeout.get_period()), false, false)));
		}

		// Tries to extend timer to new timeout.  If sooner than original timeout, timer is aborted.
		bool try_reschedule(const timeout_t& t, bool& wasAborted) // returns false if already fired
		{
			bool result = true;
			read_token rt;
			do {
				m_timeoutInfo.begin_read(rt);
				wasAborted = !!(rt->m_aborted);
				if (wasAborted)
					break;
				if (!!(rt->m_fired))
				{
					result = false;
					break;
				}
				wasAborted = (t < rt->m_timeout);
			}
			while (!m_timeoutInfo.end_write(rt, timeout_info_t(t, false, wasAborted)));
			return result;
		}

		inner_timer(const timeout_t& t, const rcref<timer>& tmr)
			: m_timeoutInfo(transactable_t::construct_embedded_t(), t),
			m_outerTimer(tmr)
		{ }
	};

	volatile rcptr<inner_timer> m_innerTimer;

	virtual void triggered() = 0;

protected:
	resettable_event m_event;

	explicit timer(const timeout_t& t)
	{
		if (!t.is_infinite())
			m_innerTimer = rcnew(inner_timer)(t, this_rcref);
	}

	void defer() { m_innerTimer->defer(); }

	// Caller error to invoke on a timer that has never been started.
	// returns false if called when in a pending/active/started state.
	// Uses next iteration of previously used period.
	// For example, if the previous period was 10 seconds, and it last expired 2 seconds ago, the timer will expire in 8 minutes.
	bool refire() { m_event.reset(); return m_innerTimer->refire(); }

	// Caller error to invoke on a timer that has never been started.
	// returns false if called when in a pending/active/started state.
	// Passing an infinite timeout puts the timer into an aborted state.  (Unfired, but will not expire/fire).
	bool refire(const timeout_t& t) { m_event.reset(); return m_innerTimer->refire(t); }

	// returns false if the timer is found to be in an expired/signaled state.
	bool reschedule(const timeout_t& t)
	{
		bool result = true;
		if (t.is_infinite())
			abort();
		else
		{
			rcptr<inner_timer> newTimer;
			rcptr<inner_timer> tmr = m_innerTimer;
			for (;;)
			{
				if (!!tmr)
				{
					bool wasAborted = false;
					result = tmr->try_reschedule(t, wasAborted); // returns false if already fired.  wasAborted is true if new timeout is earlier, or was already aborted.
					if (!wasAborted)
						break;
				}
				// !tmr || !!wasAborted
				if (!newTimer)
					newTimer = rcnew(inner_timer)(t, this_rcref);
				if (!!m_innerTimer.compare_exchange(newTimer, tmr, tmr))
					newTimer->defer();
				// The only call that changes m_innerTimer is reschedule, so failure to swap it out means another reschedule succeeded.
				// This reschedule attempt doesn't matter anymore. Go ahead and claim success.
				break;
			}
		}
		return result;
	}

	// The return values of reschedule() and refire() allow consise tracking of whether the timer has
	// transitioned into a signaled state.
	//
	// reset() does not provide that level of granularity.
	// reset() is useful to start or restart a timer that is known to be aborted or signaled, but not pending,
	// when there is no potentially trhead contention with other calls to reset(), refire(), or reschedule().
	void reset(const timeout_t& t)
	{
		while (!reschedule(t) && !refire(t))
		{
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		return dispatcher::dispatch_inner(m_event, t, priority);
	}

public:
	~timer()
	{
		rcptr<inner_timer> t = m_innerTimer;
		if (!!t)
			t->abort();
	}

	timeout_t get_timeout() { rcptr<inner_timer> t = m_innerTimer; return (!t) ? timeout_t::infinite() : t->get_timeout(); }
	period_t get_period() { return get_timeout().get_period(); }
	period_t get_expiration() { return get_timeout().get_expiration(); }
	period_t get_pending() { return get_timeout().get_pending(); }

	bool try_extend(bool& wasAborted) { return m_innerTimer->try_extend(wasAborted); }
	void extend() { m_innerTimer->extend(); }

	bool abort() // returns false if it has already gone off
	{
		rcptr<inner_timer> t = m_innerTimer;
		return (!!t) ? (t->abort()) : true;
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
};



}


#endif
