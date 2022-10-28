//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_HEADER_ENV_SYNC_THREAD
#define COGS_HEADER_ENV_SYNC_THREAD


#include "cogs/os/sync/thread.hpp"


namespace cogs {
namespace env {


class thread : public os::thread
{
private:
	static unsigned __stdcall thread_main(void* param)
	{
		function<void()>* taskPtr = (function<void()>*)param;
		(*taskPtr)();
		default_memory_manager::destruct_deallocate_type(taskPtr);
		return 0;
	}

	std::tuple<HANDLE, DWORD> create(const function<void()>& d)
	{
		function<void()>* taskPtr = default_memory_manager::allocate_type<function<void()> >();
		new (taskPtr) function<void()>(d);
		DWORD threadId;
		HANDLE hThread = (HANDLE)_beginthreadex(0, 0, thread_main, (void*)taskPtr, 0, (unsigned int*)&threadId);
		std::tuple<HANDLE, DWORD> result(hThread, threadId);
		return result;
	}

public:
	explicit thread(const function<void()>& d)
		: os::thread(create(d))
	{ }
};


inline unsigned int get_processor_count() { return os::get_processor_count(); }

}
}


#endif
