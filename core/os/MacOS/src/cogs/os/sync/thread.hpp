//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_SYNC_THREAD
#define COGS_HEADER_OS_SYNC_THREAD


#include <mach/semaphore.h>
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <pthread.h>
#include <unistd.h>

#include "cogs/function.hpp"
#include "cogs/env.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/time.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/os/sync/semaphore.hpp"
#include "cogs/os/sync/timeout.hpp"
#include "cogs/operators.hpp"


namespace cogs {
namespace os {


// os::thread is an OS level thread


class thread : public object
{				// Adds a timeout to join, and allows multiple joiners (normally pthreads only allows 1 joiner).
private:
	pthread_t		m_thread;
	function<void()> m_func;
	volatile size_t	m_joinState;	// 0 = unjoined.  >0 number of threads waiting on semaphore, -1 = exited, -2 = exited, and a joiner needs to release it
	volatile size_t	m_releaseCount;
	semaphore		m_exitSemaphore;
	
	static void* thread_main(void* threadArg)
	{
		thread* t = (thread*)threadArg;
		t->m_func();
			
		// Thread might be aborted (due to process termination after main thread returning) immediately after release, on some platforms.
		// It should be the very last thing.
		t->release();
		return 0;
	}

	void release()
	{
		size_t numWaiting = atomic::load(m_joinState);
		size_t newJoinState;
		do {
			atomic::store(m_releaseCount, numWaiting);
			if (!numWaiting)
				newJoinState = -2;
			else
				newJoinState = -1;
		} while (!atomic::compare_exchange(m_joinState, newJoinState, numWaiting, numWaiting));

		if (!!numWaiting)
			m_exitSemaphore.release(numWaiting);
	}

	static void move_pthread_to_realtime_scheduling_class(const pthread_t& pthread)
	{
		mach_timebase_info_data_t timebase_info;
		mach_timebase_info(&timebase_info);

		const uint64_t NANOS_PER_MSEC = 1000000ULL;
		double clock2abs = ((double)timebase_info.denom / (double)timebase_info.numer) * NANOS_PER_MSEC;

		thread_time_constraint_policy_data_t policy;
		policy.period = 0;
		policy.computation = (uint32_t)(5 * clock2abs); // 5 ms of work
		policy.constraint = (uint32_t)(10 * clock2abs);
		policy.preemptible = FALSE;

		thread_policy_set(pthread_mach_thread_np(pthread_self()),
			THREAD_TIME_CONSTRAINT_POLICY,
			(thread_policy_t)&policy,
			THREAD_TIME_CONSTRAINT_POLICY_COUNT);
	}

	explicit thread(const function<void()>& d)
		: m_func(d),
		m_releaseCount(0),
		m_joinState(0),
		m_exitSemaphore(desc)
	{
		self_acquire();	// Last thread to interact will release explicit reference.
		int i = pthread_create(&m_thread, NULL, thread_main, (void*)this);
		//move_pthread_to_realtime_scheduling_class(m_thread);
		COGS_ASSERT(i == 0);
	}

	inline static unsigned int s_processorCount = 0;

public:
	static rcref<thread> create(const function<void()>& d)	{ return rcnew(bypass_constructor_permission<thread>, d); }

	~thread()
	{
		int i = pthread_detach(m_thread);
		COGS_ASSERT(i == 0);
	}

	// returns -1 indicating a timeout
	// returns 0 indicating this is the current thread
	// returns 1 indicating that the thread was successfully joined
	int join(const timeout_t& timeout = timeout_t::infinite() ) const
	{ return const_cast<thread*>(this)->join(timeout); }

	int join(const timeout_t& timeout = timeout_t::infinite() )
	{
		static constexpr size_t terminatedState = const_max_int_v<size_t>;
		int result = 0;
		size_t joinState = atomic::load(m_joinState);
		for (;;)
		{
			if (joinState == (size_t)-2)	// A joiner needs to release a reference
			{
				if (!atomic::compare_exchange(m_joinState, terminatedState, joinState, joinState))
					continue;
				result = 1;
				self_release();
				break;
			}

			if (joinState == terminatedState)	// Terminated, nothing more to do.
			{
				result = 1;
				break;
			}

			size_t newJoinState = joinState + 1;
			if (!atomic::compare_exchange(m_joinState, newJoinState, joinState, joinState))
				continue;

			if (!!m_exitSemaphore.acquire(timeout))
				result = 1;
			else				// Try to remove outselves from the waiting count.
			{
				joinState = atomic::load(m_joinState);
				for (;;)
				{
					if (joinState == terminatedState)		// Terminated, nothing more to do.
					{
						result = 1;
						break;
					}

					COGS_ASSERT(joinState != 0);
					newJoinState = joinState - 1;
					if (!atomic::compare_exchange(m_joinState, newJoinState, joinState, joinState))
						continue;
					return -1;
				}
			}

			size_t newReleaseCount = pre_assign_prev(m_releaseCount);
			if (!newReleaseCount)
				self_release();
			break;
		}
		return result;
	}

	bool is_current() const
	{
		return (pthread_equal(m_thread, pthread_self()) != 0);
	}

	// Used by spinlocks.  Spins 1, or returns false to indicate that the spin should be aborted (such as on a uni-processor system)
	static bool spin_once()
	{
		if (get_processor_count() == 1)
			return false;

		_mm_pause();
		return true;
	}

	static unsigned int get_processor_count()
	{
		if (!s_processorCount)
			s_processorCount = (unsigned int)sysconf(_SC_NPROCESSORS_ONLN);
		return s_processorCount;
	}

};


}


unsigned int get_processor_count();


}


#endif
