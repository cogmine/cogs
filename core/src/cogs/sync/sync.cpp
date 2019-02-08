//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good


#ifdef COGS_COMPILE_SOURCE

#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/math/measure.hpp" //
#include "cogs/math/time.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/mem/rcref_freelist.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/math/dynamic_integer.hpp"


namespace cogs {

// s_threadWaiters is used to wait on all threads to terminate before quitting.
placement<container_dlist<rcref<thread> > > thread::s_threadWaiters;


class thread_globals : public object
{
public:
	typedef rcref_freelist<os::semaphore, default_allocator, 10> freelist_t;

	freelist_t m_semaphoreFreeList;

	rcptr<cleanup_queue> m_cleanupQueue;
	rcptr<thread_pool> m_threadPoolOwner;
	weak_rcptr<thread_pool> m_threadPool;
	delayed_construction<quit_dispatcher> m_quitDispatcher;

	thread_globals()
		: m_threadPoolOwner(rcnew(thread_pool, false)),
		m_cleanupQueue(cleanup_queue::create())
	{
		placement_rcnew(this_desc, &m_quitDispatcher.get());
		m_threadPool = m_threadPoolOwner;
	}
};

static placement<rcptr<thread_globals> > s_threadGlobals;

static rcptr<thread_globals> init_threading()
{
	volatile rcptr<thread_globals>* threadGlobals = (volatile rcptr<thread_globals>*)&s_threadGlobals;
	rcptr<thread_globals> myThreadGlobals = *threadGlobals;
	if (!!myThreadGlobals)
	{
		if (myThreadGlobals.get_mark() != 0)
			myThreadGlobals = 0;
	}
	else	// Was uninitialized.  Create it.
	{
		rcptr<thread_globals> newThreadGlobals = rcnew(thread_globals);
		if (threadGlobals->compare_exchange(newThreadGlobals, myThreadGlobals, myThreadGlobals))
		{
			myThreadGlobals = newThreadGlobals;		// Return the one we just created.
			newThreadGlobals->m_threadPoolOwner->start();
		}
		else if (myThreadGlobals.get_mark() != 0)
		{
			COGS_ASSERT(false);			// this shouldn't happen
			myThreadGlobals = 0;	// If it's already marked, return 0
		}
	}
	return myThreadGlobals;
}

weak_rcptr<thread_pool>& thread_pool::get_default()
{
	rcptr<thread_globals> threadGlobals = init_threading();
	COGS_ASSERT(!!threadGlobals);	// No reason for this to be null, we don't clear it until the world ends.
	return threadGlobals->m_threadPool;
}

rcref<volatile dispatcher> dispatcher::get_default()
{
	rcptr<thread_pool> threadPool = thread_pool::get_default();
	if (!threadPool)
		return get_immediate_task().static_cast_to<volatile dispatcher>();
	return threadPool.static_cast_to<volatile dispatcher>().dereference();
}

rcref<cleanup_queue> cleanup_queue::get_default()
{
	rcptr<thread_globals> threadGlobals = init_threading();
	COGS_ASSERT(!!threadGlobals);	// No reason for this to be null, we don't clear it until the world ends.
	return threadGlobals->m_cleanupQueue.dereference();
}

rcref<quit_dispatcher> quit_dispatcher::get()
{
	rcptr<thread_globals> threadGlobals = init_threading();
	COGS_ASSERT(!!threadGlobals);	// No reason for this to be null, we don't clear it until the world ends.
	return threadGlobals->get_self_rcref(&(threadGlobals->m_quitDispatcher.get()));
}

void run_cleanup()
{
	volatile rcptr<thread_globals>* threadGlobalsPtr = &s_threadGlobals.get();
	rcptr<thread_globals> threadGlobals = *threadGlobalsPtr;
	if (!threadGlobals)
		return;

	// Phase 1: Full service cleanup - cleanup queue and threading are still online.
	//
	//	First, try to drain the default global cleanup queue (without taking it offline).
	//	This allows new cleanup requests to be chained after existing ones.
	//	(Because threads are still running, new cleanup requests could still come in late).

	threadGlobals->m_cleanupQueue->drain();

	// Phase 2: Tear down threading subsystem
	//	
	//	Cleanup phase has completed, take threading subsystem offline for cleanup.
	//	Threading system uses the same cleanup queue.
	threadGlobals->m_threadPoolOwner.release();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_LEAKED_BLOCK_DETECTION
	thread::join_all(timeout_t(measure<uint8_type, seconds>((uint8_t)10)));
#else
	thread::join_all();
#endif

	// Phase 3: Take the global cleanup queue offline and drain it again.
	//	
	//	Now that no other threads are running, once the queue is drained, we're done.
	//
	threadGlobals->m_cleanupQueue->drain();

	rcptr<thread_globals> markedEmptyRef;
	markedEmptyRef.set_mark(1);
	*threadGlobalsPtr = markedEmptyRef;
}


rcref<os::semaphore> get_os_semaphore()
{
	rcptr<thread_globals> threadGlobals = init_threading();
	COGS_ASSERT(!!threadGlobals);	// No reason for this to be null, we don't clear it until the world ends.
	return threadGlobals->m_semaphoreFreeList.get();
}


void dispatch_parallel(size_t n, const function<void(size_t)>& d, const function<void()>& doneDelegate, int priority)
{
	{
		rcptr<thread_pool> pool = thread_pool::get_default();
		if (!!pool)
		{
			pool->dispatch_parallel(n, d, doneDelegate, priority);
			return;
		}

		for (size_t i = 0; i < n; i++)
			d(i);
	}

	// Recursion might be an issue.  Since the call is at the end of the function, it might jump with no stack overhead.
	doneDelegate();
}


static placement<rcptr<task<void> > > s_defaultImmediateTask;

static void cleanup_immediate_task()
{
	volatile rcptr<task<void> >& defaultImmediateTask = s_defaultImmediateTask.get();
	defaultImmediateTask = (task<void>*)nullptr;
}

rcref<task<void> > cogs::get_immediate_task()
{
	volatile rcptr<task<void> >& defaultImmediateTask = s_defaultImmediateTask.get();
	rcptr<task<void> > myImmediateTask = defaultImmediateTask;
	if (!myImmediateTask)
	{
		rcptr<task<void> > newImmediateTask = rcnew(immediate_task<void>);
		if (defaultImmediateTask.compare_exchange(newImmediateTask, myImmediateTask, myImmediateTask))
		{
			myImmediateTask = newImmediateTask;
			cleanup_queue::get_default()->dispatch(&cleanup_immediate_task);
		}
	}
	return myImmediateTask.dereference();
}


}

#endif
