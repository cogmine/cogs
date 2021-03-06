//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_HEADER_OS_SYNC_THREAD
#define COGS_HEADER_OS_SYNC_THREAD


#include "cogs/function.hpp"
#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/os.hpp"
#include "cogs/os/sync/timeout.hpp"


namespace cogs {
namespace os {


// os::thread is an OS level thread


class thread : public object
{
private:
	HANDLE m_hThread;
	DWORD m_threadId;

	static unsigned __stdcall thread_main(void* param)
	{
		function<void()>* taskPtr = (function<void()>*)param;
		(*taskPtr)();
		default_memory_manager::destruct_deallocate_type(taskPtr);
		return 0;
	}

	inline static unsigned int s_processorCount = 0;

public:
	explicit thread(const function<void()>& d)
	{
		function<void()>* taskPtr = default_memory_manager::allocate_type<function<void()> >();
		new (taskPtr) function<void()>(d);
		m_hThread = (HANDLE)_beginthreadex(0, 0, thread_main, (void*)taskPtr, 0, (unsigned int*)&m_threadId);
	}

	~thread() { CloseHandle(m_hThread); }

	// returns -1 indicating a timeout
	// returns 0 indicating this is the current thread
	// returns 1 indicating that the thread was successfully joined
	int join(const timeout_t& timeout = timeout_t::infinite() ) const
	{
		int result = 0;
		if (!is_current())
		{
			DWORD ms = (timeout.is_infinite()) ? INFINITE : (DWORD)timeout.get_pending().convert_to<milliseconds>().get().get_int();
			DWORD dw = WaitForSingleObject(m_hThread, ms);
			result = (dw == WAIT_OBJECT_0) ? 1 : -1;
		}
		return result;
	}

	bool is_current() const { return m_threadId == GetCurrentThreadId(); }

	// Used by spinlocks.  Spins 1, or returns false to indicate that the spin should be aborted (such as on a uni-processor system)
	static bool spin_once()
	{
		if (get_processor_count() == 1)
			return false;

#ifdef YieldProcessor
		YieldProcessor();
#elif defined(_M_X64) || defined(_M_AMD64)
		_mm_pause();
#elif defined(_M_ARM64) || defined(_M_ARM)
		__yield();
#else
		__asm__ __volatile__ ("yield");
#endif
		return true;
	}

	static unsigned int get_processor_count()
	{
		if (!s_processorCount)
		{
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			s_processorCount = (unsigned long)si.dwNumberOfProcessors;
		}
		return s_processorCount;
	}

};


}


}


#endif
