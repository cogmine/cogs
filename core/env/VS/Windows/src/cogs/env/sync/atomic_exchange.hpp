//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE
#define COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE

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
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	rtn = (T)_InterlockedExchange8((char*)(unsigned char*)&t, (char)src);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	rtn = (T)_InterlockedExchange16((short*)(unsigned char*)&t, (short)src);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	rtn = (T)_InterlockedExchange((long*)(unsigned char*)&t, (long)src);
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	T
>
exchange(volatile T& t, const T& src)
{
	return (T)_InterlockedExchange8((char*)(unsigned char*)&t, (char)src);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (T)_InterlockedExchange16((short*)(unsigned char*)&t, (short)src);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (T)_InterlockedExchange((long*)(unsigned char*)&t, (long)src);
}


#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpRtn = _InterlockedExchange64((__int64*)(unsigned char*)&t, tmpSrc);
	T rtn;
	bypass_strict_aliasing(tmpRtn, rtn);
	return rtn;
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpRtn = _InterlockedExchange64((__int64*)(unsigned char*)&t, tmpSrc);
	bypass_strict_aliasing(tmpRtn, rtn);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(__int64))
	&& (sizeof(T) <= (sizeof(__int64) * 2)),
	void
>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc[2];
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp[2] = {};
	do { } while (0 == _InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp));
	bypass_strict_aliasing(tmpCmp, rtn);
}

#else

// _InterlockedExchange64 is not available on x86.  The platform (non-intrinsic?) version is linkable.
// Using our own workaround seems more efficient.  There is no 32-bit XCHG64 instruction.  Use CMPXCHG64

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp = {};
	for (;;)
	{
		__int64 oldValue = _InterlockedCompareExchange64((__int64*)(unsigned char*)&t, tmpSrc, tmpCmp);
		if (oldValue == tmpCmp)
			break;
		tmpCmp = oldValue;
	}
	T rtn;
	bypass_strict_aliasing(tmpCmp, rtn);
	return rtn;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& is_scalar_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	__int64 tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	__int64 tmpCmp = {};
	for (;;)
	{
		__int64 oldValue = _InterlockedCompareExchange64((__int64*)(unsigned char*)&t, tmpSrc, tmpCmp);
		if (oldValue == tmpCmp)
			break;
		tmpCmp = oldValue;
	}
	bypass_strict_aliasing(tmpCmp, rtn);
}

#endif


}
}

#endif
