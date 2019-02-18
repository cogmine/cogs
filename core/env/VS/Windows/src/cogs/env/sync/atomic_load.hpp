//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_LOAD
#define COGS_HEADER_ENV_SYNC_ATOMIC_LOAD

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"
#include "cogs/env/sync/atomic_compare_exchange.hpp"

namespace cogs {
namespace atomic {


template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& std::is_scalar_v<T>,
	void
>
load(const volatile T& src, T& rtn)
{
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpRtn;
	cogs::atomic::compare_exchange(*(volatile int_t*)(unsigned char*)&src, (int_t)0, (int_t)0, tmpRtn);
	bypass_strict_aliasing(tmpRtn, rtn);
	
	// Full barrier doesn't seem to be enough...
	//full_barrier();
	//rtn = src;
	//full_barrier();
}



#ifdef _M_X64


template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	void
>
load(const volatile T& t, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	// Note: CMPXCHG/CMPXCHG8B/CMPXCHG16B all issue an implicit write, even if not modified.
	volatile T* srcPtr = const_cast<volatile T*>(&t);
	__int64 tmpCmp[2] = {};
	_InterlockedCompareExchange128((__int64*)(unsigned char*)srcPtr, tmpCmp[1], tmpCmp[0], tmpCmp);
	bypass_strict_aliasing(tmpCmp, rtn);
}

template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	T
>
load(const volatile T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	// Note: CMPXCHG/CMPXCHG8B/CMPXCHG16B all issue an implicit write, even if not modified.
	volatile T* srcPtr = const_cast<volatile T*>(&t);
	__int64 tmpCmp[2] = {};
	_InterlockedCompareExchange128((__int64*)(unsigned char*)srcPtr, tmpCmp[1], tmpCmp[0], tmpCmp);
	T rtn;
	bypass_strict_aliasing(tmpCmp, rtn);
	return rtn;
}

#endif


}
}


#endif
