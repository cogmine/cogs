//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE
#define COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE

#include <type_traits>

#include "cogs/arch/sync/atomic.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/sync/atomic_load.hpp"

namespace cogs {
namespace atomic {


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) <= 8,
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpRtn;
	__atomic_exchange((int_t*)&t, &tmpSrc, &tmpRtn, __ATOMIC_SEQ_CST);
	bypass_strict_aliasing(tmpRtn, rtn);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) <= 8,
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpRtn;
	__atomic_exchange((int_t*)&t, &tmpSrc, &tmpRtn, __ATOMIC_SEQ_CST);
	T rtn;
	bypass_strict_aliasing(tmpRtn, rtn);
	return rtn;
}


// Fallback to cmpxchg16 until GCC gets it's atomics figured out
template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) == 16,
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T cmp = atomic::load(t);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpCmp;
	bypass_strict_aliasing(cmp, tmpCmp);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	for (;;)
	{
		int_t tmp = __sync_val_compare_and_swap((int_t*)&t, tmpCmp, tmpSrc);
		if (tmp == tmpCmp)
			break;
		tmpCmp = tmp;
	}
	bypass_strict_aliasing(tmpCmp, rtn);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) == 16,
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T cmp = atomic::load(t);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpCmp;
	bypass_strict_aliasing(cmp, tmpCmp);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	for (;;)
	{
		int_t tmp = __sync_val_compare_and_swap((int_t*)&t, tmpCmp, tmpSrc);
		if (tmp == tmpCmp)
			break;
		tmpCmp = tmp;
	}
}


}
}


#endif
