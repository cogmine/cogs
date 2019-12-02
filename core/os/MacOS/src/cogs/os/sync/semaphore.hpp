//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_SYNC_SEMAPHORE
#define COGS_HEADER_OS_SYNC_SEMAPHORE


#include <mach/semaphore.h>
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <pthread.h>

#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/math/time.hpp"
#include "cogs/os/sync/timeout.hpp"
#include "cogs/mem/rc_container.hpp"


namespace cogs {
namespace os {


// os::semaphore provides the basis for thread synchronization.

// cogs synchronization objects (event, mutex) use os::semaphores to block threads.


class semaphore : public object
{
private:
	semaphore_t m_semaphore;

public:
	semaphore(rc_obj_base& desc)
		: object(desc)
	{
		kern_return_t result = semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, 0);
		COGS_ASSERT(result == KERN_SUCCESS);
	}

	~semaphore()
	{
		kern_return_t result = semaphore_destroy(mach_task_self(), m_semaphore);
		COGS_ASSERT(result == KERN_SUCCESS);
	}

	bool acquire(const timeout_t& timeout = timeout_t::infinite() )
	{
		kern_return_t result;
		if (timeout.is_infinite())
			result = semaphore_wait(m_semaphore);
		else
		{
			mach_timespec_t ts;
			timeout.get_pending(ts);
			result = semaphore_timedwait(m_semaphore, ts);
		}
		return result == KERN_SUCCESS;
	}

	void release(size_t n)
	{
		while (n--)
		{
			kern_return_t result = semaphore_signal(m_semaphore);
			COGS_ASSERT(result == KERN_SUCCESS);
		}
	}

};


}
}

#endif
