//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_OS_THREAD
#define COGS_OS_THREAD


#include "cogs/function.hpp"
#include "cogs/mem/allocator.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/os.hpp"
#include "cogs/os/sync/timeout.hpp"


namespace cogs {
namespace os {


// os::thread is an OS level thread


class thread : public object
{
private:
	HANDLE	m_hThread;
	DWORD	m_threadId;

	static unsigned __stdcall thread_main(void* param)
	{
		function<void()>* taskPtr = (function<void()>*)param;
		(*taskPtr)();
		default_allocator::destruct_deallocate_type(taskPtr);
		return 0;
	}

protected:
	explicit thread(const function<void()>& d)
	{
		m_hThread = (HANDLE)_beginthreadex(0, 0, thread_main, (void*)new (default_allocator::get()) function<void()>(d), 0, (unsigned int*)&m_threadId);
	}

public:
	static rcref<thread> create(const function<void()>& d)
	{
		return rcnew(bypass_constructor_permission<thread>, d);
	}

	~thread()	{ CloseHandle(m_hThread); }

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

	bool is_current() const		{ return m_threadId == GetCurrentThreadId(); }

	// Used by spinlocks.  Spins 1, or returns false to indicate that the spin should be aborted (such as on a uni-processor system)
	static bool spin_once();
};


}


unsigned int get_num_processors();


}


#endif
