//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_STORE
#define COGS_HEADER_ENV_SYNC_ATOMIC_STORE

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/env/sync/atomic_exchange.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <typename T, typename T2 = T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_empty_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_volatile_v<T2>
	&& std::is_constructible_v<T, const T2&>,
	void
>
store(volatile T& dst, const T2& src)
{
	COGS_ASSERT(((size_t)&dst % atomic::get_alignment_v<T>) == 0);

	T tmp(src);
	cogs::atomic::exchange(dst, tmp);

	// Full barrier is not enough?
	//full_barrier();
	//dst = tmp;
	//full_barrier();
}


#ifdef _M_X64


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_empty_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	void
>
store(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc[2];
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp[2] = {};
	do {} while (0 == _InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp));
}

#endif


}
}


#endif
