//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	&& is_scalar_v<T>
	&& sizeof(T) <= 8,
	void
>
load(const volatile T& src, T& rtn)
{
	COGS_ASSERT(((size_t)&src % atomic::get_alignment_v<T>) == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpRtn;
	__atomic_load((int_t*)&src, &tmpRtn, __ATOMIC_SEQ_CST);
	bypass_strict_aliasing(tmpRtn, rtn);
}


// Fallback to cmpxchg16 until GCC gets it's atomics figured out
template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& is_scalar_v<T>
	&& sizeof(T) == 16,
	void
>
load(const volatile T& src, T& rtn)
{
	COGS_ASSERT(((size_t)&src % atomic::get_alignment_v<T>) == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;

	// Will not work w/ read-only memory
	int_t tmpRtn = __sync_val_compare_and_swap((volatile int_t*)(&src), 0, 0);
	bypass_strict_aliasing(tmpRtn, rtn);
}


}
}


#endif
