//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_THREAD
#define COGS_HEADER_SYNC_THREAD


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/sync/thread.hpp"
#include "cogs/sync/dispatcher.hpp"


namespace cogs {


class thread_pool;
class cleanup_queue;
class quit_dispatcher;


/// @ingroup Synchronization
/// @brief A thread class
class thread : public object
{
private:
	rcptr<os::thread>			m_osThread;
	mutable single_fire_event	m_joinSync;
	
	// Some OSes will kill other threads when the main thread exits.
	// In order to allow cleanup, such as graceful thread exit, we provide
	// a cleanup phase that occurs in the main thread before static/global
	// destruction begins.  After this cleanup has occured, any lingering
	// threads will also be joined before main() is allowed to exit.
	// In fact, all global references that may ultimately reference a
	// subsystem that employs a thread, must be cleaned up here (BEFORE static/global
	// destruct) to ensure graceful application shutdown.  (READ: NO GLOBAL rcptr<>'s
	// unless cleared to 0 at cleanup time, BEFORE global/static destruct time.)

	// s_threadWaiters is used to wait on all threads to terminate before quitting.
	class thread_waiters_t : public container_dlist<rcref<thread> > { };

	container_dlist<rcref<thread> >::volatile_remove_token	m_removeToken;
	mutable volatile boolean m_deregisteredWaiter;

	static void register_waiter(const rcref<thread>& t);
	void deregister_waiter() const;

	thread() = delete;
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;

public:
	thread(const ptr<rc_obj_base>& desc, const function<void()>& task)
		: object(desc),
		m_joinSync(desc)
	{
		m_osThread = rcnew(os::thread, task);
	}

	static unsigned int get_processor_count()
	{
		return os::thread::get_processor_count();
	}

	static rcref<thread> spawn(const function<void()>& tsk)
	{
		rcref<thread> threadRef = rcnew(thread, tsk);
		register_waiter(threadRef);
		return threadRef;
	}

	static void join_all(const timeout_t& timeout = timeout_t::infinite());	// To be called in main thread only.  Called automatically at exit.  Do not create new threads after calling.

	// returns -1 indicating a timeout
	// returns 0 indicating this is the current thread
	// returns 1 indicating that the thread was successfully joined
	int join(const timeout_t& timeout = timeout_t::infinite() ) const
	{
		int result = 0;
		if (!is_current())
		{
			if (m_joinSync.is_signaled())
				result = 1;
			else
			{
				result = m_osThread->join(timeout);
				if (result != -1)
				{
					if (m_deregisteredWaiter.compare_exchange(true, false))
					{
						deregister_waiter();	// make sure only 1 thread can call deregister_waiter
						m_joinSync.signal();
					}
					else
						m_joinSync.wait();
				}
			}
		}
		return result;
	}

	bool is_current() const		{ return m_osThread->is_current(); }

	// Used by spinlocks.  Spins 1, or returns false to indicate that the spin should be aborted (such as on a uni-processor system)
	static bool spin_once()		{ return os::thread::spin_once(); }
};


}


#endif
