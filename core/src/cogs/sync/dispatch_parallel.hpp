//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_DISPATCH_PARALLEL
#define COGS_HEADER_SYNC_DISPATCH_PARALLEL


#include "cogs/function.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {

template <typename F, typename D>
inline std::enable_if_t<
	(std::is_invocable_v<F, size_t> || std::is_invocable_v<F>)
	&& std::is_invocable_v<D>,
	void>
dispatch_parallel(size_t n, F&& f, D&& doneFunc, int priority = 0)
{
	{
		rcptr<thread_pool> pool = thread_pool::get_default();
		if (!!pool)
		{
			pool->dispatch_parallel(n, std::forward<F>(f), std::forward<D>(doneFunc), priority);
			return;
		}

		for (size_t i = 0; i < n; i++)
		{
			if constexpr (std::is_invocable_v<F, size_t>)
				f(i);
			else
				f();
		}
	}

	// Recursion might be an issue.  Since the call is at the end of the function, it might jump with no stack overhead.
	doneFunc();
}


template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F, size_t>
	|| std::is_invocable_v<F>,
	void>
dispatch_parallel(size_t n, F&& f, int priority = 0)
{
	dispatch_parallel(n, std::forward<F>(f), []() {}, priority);
}


}


#endif
