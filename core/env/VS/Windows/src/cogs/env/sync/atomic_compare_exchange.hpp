//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_COMPARE_EXCHANGE
#define COGS_HEADER_ENV_SYNC_ATOMIC_COMPARE_EXCHANGE

#include <type_traits>

#include "cogs/env.hpp"
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
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	char tmp = _InterlockedCompareExchange8((char*)(unsigned char*)&t, (char)src, (char)cmp);
	bool b = tmp == (char)cmp;
	rtn = (T)tmp;
	return b;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	short tmp = _InterlockedCompareExchange16((short*)(unsigned char*)&t, (short)src, (short)cmp);
	bool b = tmp == (short)cmp;
	rtn = (T)tmp;
	return b;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	long tmp = _InterlockedCompareExchange((long*)(unsigned char*)&t, (long)src, (long)cmp);
	bool b = tmp == (long)cmp;
	rtn = (T)tmp;
	return b;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmp = _InterlockedCompareExchange64((__int64*)(unsigned char*)&t, (__int64)src, (__int64)cmp);
	bool b = tmp == (__int64)cmp;
	rtn = (T)tmp;
	return b;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	return _InterlockedCompareExchange8((char*)(unsigned char*)&t, (char)src, (char)cmp) == (char)cmp;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return _InterlockedCompareExchange16((short*)(unsigned char*)&t, (short)src, (short)cmp) == (short)cmp;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return _InterlockedCompareExchange((long*)(unsigned char*)&t, (long)src, (long)cmp) == (long)cmp;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return _InterlockedCompareExchange64((__int64*)(unsigned char*)&t, (__int64)src, (__int64)cmp) == (__int64)cmp;
}



#ifdef _M_X64


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc[2];
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp[2];
	bypass_strict_aliasing(cmp, tmpCmp);
	bool b = _InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp) != 0;
	bypass_strict_aliasing(tmpCmp, rtn);
	return b;
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc[2];
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp[2];
	bypass_strict_aliasing(cmp, tmpCmp);
	return _InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp) != 0;
}

#endif

}
}

#endif
