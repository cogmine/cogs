//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_LOAD
#define COGS_HEADER_ENV_SYNC_ATOMIC_LOAD

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/assert.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& sizeof(T) <= 8,
	void
>
load(const volatile T& src, T& rtn)
{
	COGS_ASSERT(((size_t)&src % atomic::get_alignment_v<T>) == 0);
	__atomic_load(&src, &rtn, __ATOMIC_SEQ_CST);
}


// Fallback to cmpxchg16 until GCC gets it's atomics problem figured out
template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& sizeof(T) == 16,
	void
>
load(const volatile T& src, T& rtn)
{
	COGS_ASSERT(((size_t)&src % atomic::get_alignment_v<T>) == 0);
	rtn = __sync_val_compare_and_swap((volatile T*)(&src), 0, 0);
}


}
}


#endif