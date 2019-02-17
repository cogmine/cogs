//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifndef COGS_HEADER_OS_SYNC_SEMAPHORE
#define COGS_HEADER_OS_SYNC_SEMAPHORE


#include "cogs/env.hpp"
#include "cogs/math/time.hpp"
#include "cogs/os.hpp"
#include "cogs/os/sync/timeout.hpp"
#include "cogs/mem/rc_container_base.hpp"


namespace cogs {
namespace os {


// os::semaphore provides the basis for thread synchronization.

// cogs synchronization objects (event, mutex) use os::semaphores to block threads.


// Because all thread blocking is done using semaphores, no priority inversion occurs on Windows.
class semaphore : public object
{
private:
	HANDLE m_hSemaphore;

public:
	semaphore()
		:	m_hSemaphore(CreateSemaphore(NULL, 0, LONG_MAX, NULL))
	{
		COGS_ASSERT(m_hSemaphore != NULL);
	}

	~semaphore()
	{
		CloseHandle(m_hSemaphore);
	}

	bool acquire(const timeout_t& timeout = timeout_t::infinite() )
	{
		DWORD ms = (timeout.is_infinite()) ? INFINITE : (DWORD)timeout.get_pending().convert_to<milliseconds>().get().get_int();
		bool result = WaitForSingleObject(m_hSemaphore, ms) == WAIT_OBJECT_0;
		return result;
	}

	void release(size_t n = 1)
	{
		ReleaseSemaphore(m_hSemaphore, (LONG)n, 0);
	}
};


}
}


#endif
