//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_PARALLEL_FOR_EACH
#define COGS_PARALLEL_FOR_EACH


#include "cogs/function.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {

void dispatch_parallel(size_t n, const function<void(size_t)>& d, const function<void()>& doneDelegate = function<void()>(), int priority = 0)
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


}


#endif
