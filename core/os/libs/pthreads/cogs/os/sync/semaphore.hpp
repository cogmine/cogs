//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_SYNC_SEMAPHORE
#define COGS_HEADER_OS_SYNC_SEMAPHORE


#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "cogs/env.hpp"
#include "cogs/os/sync/timeout.hpp"
#include "cogs/mem/rc_container_base.hpp"


namespace cogs {
namespace os {


// os::semaphore provides the basis for thread synchronization.

// cogs synchronization objects (event, mutex) use os::semaphores to block threads.


class semaphore : public object
{
private:
	sem_t	m_semaphore;

public:
	semaphore()
	{
		int result = sem_init(&m_semaphore, 0, 0);
		COGS_ASSERT(result == 0);
	}

	~semaphore()
	{
		int result = sem_destroy(&m_semaphore);
		COGS_ASSERT(result == 0);
	}

	bool acquire(const timeout_t& timeout = timeout_t::infinite() )
	{
		if (timeout.is_infinite())
		{
			int result;
			for (;;)
			{
				result = sem_wait(&m_semaphore);
				if (result == 0)
					break;
				int err = errno;
				if (err == EINTR)
					continue;
				COGS_ASSERT(false);
			}
			return true;
		}
		else
		{
			timespec ts;
			timeout.get_expiration(ts);
			int result;
			for (;;)
			{
				result = sem_timedwait(&m_semaphore, &ts);
				if (result == 0)
					break;
				int err = errno;
				if (err == EINTR)
					continue;
				if (err == ETIMEDOUT)
					return false;
				COGS_ASSERT(false);
			}
			return true;
		}
	}

	void release(size_t n)
	{
		while (n--)
		{
			int result = sem_post(&m_semaphore);
			COGS_ASSERT(result == 0);
		}
	}
};


}
}

#endif
