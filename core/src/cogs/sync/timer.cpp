//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_COMPILE_SOURCE


#include "cogs/env.hpp"
#include "cogs/collections/multimap.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/semaphore.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/timer.hpp"


namespace cogs {


placement<rcptr<timer::inner_timer::globals> >	timer::inner_timer::s_globals;


class timer::inner_timer::globals
{
public:
	volatile multimap<timeout_t, rcref<inner_timer> > m_timers;

	volatile rcptr<thread>	m_timerThread;
	semaphore				m_timerThreadSemaphore;
	bool					m_terminating;
	
	globals()	
		:	m_terminating(false),
			m_timerThreadSemaphore(0, 1)
	{ }

	~globals()
	{
		rcptr<thread> timerThread = m_timerThread;
		if (!!timerThread)	// Will be false if spurious alloc in init race condition
		{
			m_terminating = true;
			m_timerThreadSemaphore.release();
			timerThread->join();
		}
	}
};


void timer::inner_timer::thread_main()
{
	rcptr<globals> acquiredTimerGlobals = init();
	if (!!acquiredTimerGlobals)
	{
		globals* myTimerGlobals = acquiredTimerGlobals.get_ptr();	// We know it's not going anywhere until this thread is joined, so don't reference it.
		acquiredTimerGlobals = 0;
		timeout_t timeout = timeout_t::infinite();
		do {
			myTimerGlobals->m_timerThreadSemaphore.acquire_any(timeout);	// timeout of next timer
			multimap<timeout_t, rcref<inner_timer> >::volatile_iterator itor;
			for (;;)
			{
				itor = myTimerGlobals->m_timers.get_first();
				if (!itor || !!(itor.get_key().get_pending()))	// Only bother with stuff that expired.
					break;
				rcref<inner_timer> t = *itor;
				myTimerGlobals->m_timers.remove(itor);
				read_token rt;
				for (;;)	// Loop until transactable write succeeds
				{
					t->m_timeoutInfo.begin_read(rt);
					if (rt->m_aborted)
						break;
					if (!!rt->m_timeout.get_pending())		// Got extended.  Put it back in the list.
						myTimerGlobals->m_timers.insert(rt->m_timeout, t);
					else									// expired
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


rcptr<timer::inner_timer::globals> timer::inner_timer::init()
{
	volatile rcptr<globals>& timerGlobals = s_globals.get();
	rcptr<globals> oldTimerGlobals = timerGlobals;
	if (!!oldTimerGlobals)
	{
		if (oldTimerGlobals.get_mark() != 0)	// mark is set, already quitting.  return 0.
			oldTimerGlobals = 0;
	}
	else	// Was uninitialized.  Create it.
	{
		rcptr<globals> newTimerGlobals = rcnew(globals);
		if (timerGlobals.compare_exchange(newTimerGlobals, oldTimerGlobals, oldTimerGlobals))
		{
			oldTimerGlobals = newTimerGlobals;		// Return the one we just created.
			newTimerGlobals->m_timerThread = thread::spawn(&thread_main);
			cleanup_queue::get_default()->dispatch(&cleanup_globals);
		}
		else if (oldTimerGlobals.get_mark() != 0)	// someone beat us to it.
			oldTimerGlobals = 0;					// If it's already marked, return 0
	}
	return oldTimerGlobals;
}

void timer::inner_timer::cleanup_globals()
{
	volatile rcptr<globals>& timerGlobals = s_globals.get();
	rcptr<globals> emptyRef;
	emptyRef.set_mark(1);
	timerGlobals = emptyRef;
}

void timer::inner_timer::defer()
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
	//else	// if shutting down, new timers will NOT fire
		// trigger();
}


}



#endif
