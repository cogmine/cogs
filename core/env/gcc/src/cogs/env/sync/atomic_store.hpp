//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_STORE
#define COGS_HEADER_ENV_SYNC_ATOMIC_STORE

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/assert.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_load.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <typename T, typename T2 = T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_empty_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_volatile_v<T2>
	&& std::is_constructible_v<T, const T2&>
	&& sizeof(T) <= 8,
	void
>
store(volatile T& dst, const T2& src)
{
	COGS_ASSERT(((size_t)&dst % atomic::get_alignment_v<T>) == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	__atomic_store((int_t*)&dst, &tmpSrc, __ATOMIC_SEQ_CST);
}


// Fallback to cmpxchg16 until GCC gets it's atomics figured out
template <typename T, typename T2 = T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_empty_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_volatile_v<T2>
	&& std::is_constructible_v<T, const T2&>
	&& sizeof(T) == 16,
	void
>
store(volatile T& dst, const T2& src)
{
	COGS_ASSERT(((size_t)&dst % atomic::get_alignment_v<T>) == 0);
	T cmp = atomic::load(dst);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpCmp;
	bypass_strict_aliasing(cmp, tmpCmp);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	for (;;)
	{
		int_t tmp = __sync_val_compare_and_swap((int_t*)&dst, tmpCmp, tmpSrc);
		if (tmp == tmpCmp)
			break;
		tmpCmp = tmp;
	}
}


}
}


#endif
