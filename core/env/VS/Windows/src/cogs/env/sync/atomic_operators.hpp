//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_OPERATORS
#ifndef COGS_ENV_ATOMIC_OPERATORS
#define COGS_ENV_ATOMIC_OPERATORS


#include "cogs/math/bytes_to_int.hpp"
#include "cogs/os/sync/atomic_operators.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {

inline void read_barrier() { _ReadBarrier(); MemoryBarrier(); }
inline void write_barrier() { _WriteBarrier(); MemoryBarrier(); }
inline void full_barrier() { _ReadWriteBarrier(); MemoryBarrier(); }


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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
	do {} while (0 == _InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp));
	bypass_strict_aliasing(tmpCmp, rtn);
}

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


#else

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
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
	&& std::is_scalar_v<T>
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


COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(not)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(abs)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_not)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(negative)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_count)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_forward)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_reverse)

COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(endian_swap)

COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_left)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_right)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_left)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_right)

COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_subtract)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(multiply)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(modulo)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_modulo)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide)

COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(reciprocal)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide_whole)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide_whole)

COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(gcd)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lcm)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(greater)
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lesser)


// next

// We fall back to the default (compare_exchange), if:
//	1 byte, since there is no _InterlockedIncrement8
//	It's a floating point, as there is no intrinsic for it.

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_arithmetic_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& ((sizeof(T) <= sizeof(char)) || std::is_floating_point_v<T>),
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	return os::atomic::pre_assign_next(t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedIncrement16((short*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedIncrement((long*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedIncrement64((__int64*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
#ifdef _M_X64
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>)) + 1;
#else
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((__int64*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>)) + 1;
#endif
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_next(T& t)
{
	pre_assign_next(t);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_next(T& t)
{
	return pre_assign_next(t) - 1;
}


// prev

// We fall back to the default (compare_exchange), if:
//	1 byte, since there is no _InterlockedDecrement8
//	It's a floating point, as there is no intrinsic for it.

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_arithmetic_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& ((sizeof(T) <= sizeof(char)) || std::is_floating_point_v<T>),
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	return os::atomic::pre_assign_prev(t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedDecrement16((short*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedDecrement((long*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return (std::remove_volatile_t<T>)_InterlockedDecrement64((__int64*)(unsigned char*)&t);
}

template <typename T>
inline std::enable_if_t<
	std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
#ifdef _M_X64
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, -sizeof(std::remove_pointer_t<T>)) - 1;
#else
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((__int64*)(unsigned char*)&t, -sizeof(std::remove_pointer_t<T>)) - 1;
#endif
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_prev(T& t)
{
	pre_assign_prev(t);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_prev(T& t)
{
	return pre_assign_prev(t) + 1;
}




// bit_and


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T> >
post_assign_bit_and(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedAnd8((char*)(unsigned char*)&t, (char)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T> >
post_assign_bit_and(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedAnd16((short*)(unsigned char*)&t, (short)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T> >
post_assign_bit_and(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedAnd((long*)(unsigned char*)&t, (long)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T> >
post_assign_bit_and(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedAnd64((__int64*)(unsigned char*)&t, (__int64)tmp);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_and(T& t, const A1& a)
{
	post_assign_bit_and(t, a);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_bit_and(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (post_assign_bit_and(t, tmp) & tmp);
}


// bit_or


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T> >
post_assign_bit_or(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedOr8((char*)(unsigned char*)&t, (char)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T> >
post_assign_bit_or(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedOr16((short*)(unsigned char*)&t, (short)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T> >
post_assign_bit_or(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedOr((long*)(unsigned char*)&t, (long)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T> >
post_assign_bit_or(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedOr64((__int64*)(unsigned char*)&t, (__int64)tmp);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_or(T& t, const A1& a)
{
	post_assign_bit_or(t, a);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_bit_or(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (post_assign_bit_or(t, tmp) | tmp);
}




// bit_xor


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T> >
	post_assign_bit_xor(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedXor8((char*)(unsigned char*)&t, (char)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T> >
	post_assign_bit_xor(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedXor16((short*)(unsigned char*)&t, (short)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T> >
	post_assign_bit_xor(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedXor((long*)(unsigned char*)&t, (long)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T> >
	post_assign_bit_xor(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedXor64((__int64*)(unsigned char*)&t, (__int64)tmp);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_xor(T& t, const A1& a)
{
	post_assign_bit_xor(t, a);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_bit_xor(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (post_assign_bit_xor(t, tmp) ^ tmp);
}




// add


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd8((char*)(unsigned char*)&t, (char)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd16((short*)(unsigned char*)&t, (short)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((long*)(unsigned char*)&t, (long)tmp);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(__int64)),
	std::remove_volatile_t<T> >
post_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, (__int64)tmp);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
#ifdef _M_X64
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>));
#else
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((__int64*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>));
#endif
}




template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_add(T& t, const A1& a)
{
	os::atomic::assign_add(t, a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_add(T& t, const A1& a)
{
	return os::atomic::pre_assign_add(t, a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	return os::atomic::post_assign_add(t, a);
}




template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& (std::is_integral_v<T> || std::is_pointer_v<T>)
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_add(T& t, const A1& a)
{
	post_assign_add(t, a);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_add(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (post_assign_add(t, tmp) + tmp);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_add(T& t, const A1& a)
{
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (post_assign_add(t, tmp) + tmp);
}


// subtract

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd8((char*)(unsigned char*)&t, -(char)tmp)
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd16((short*)(unsigned char*)&t, -(short)tmp)
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((long*)(unsigned char*)&t, -(long)tmp)
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
#ifdef _M_X64
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, tmp * -(ptrdiff_t)sizeof(std::remove_pointer_t<T>));
#else
	return (std::remove_volatile_t<T>)_InterlockedExchangeAdd((__int64*)(unsigned char*)&t, tmp * -(ptrdiff_t)sizeof(std::remove_pointer_t<T>));
#endif
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
post_assign_subtract(T& t, const A1& a)
{
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return post_assign_subtract(t, tmp * sizeof(std::remove_pointer_t<T>));
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_subtract(T& t, const A1& a)
{
	os::atomic::assign_subtract(t, a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_subtract(T& t, const A1& a)
{
	return os::atomic::pre_assign_subtract(t, a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	return os::atomic::post_assign_subtract(t, a);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& (std::is_integral_v<T> || std::is_pointer_v<T>)
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_subtract(T& t, const A1& a)
{
	post_assign_subtract(t, a);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_subtract(T& t, const A1& a)
{
	T tmp;
	cogs::assign(tmp, a);
	return (post_assign_subtract(t, tmp) - tmp);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_subtract(T& t, const A1& a)
{
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (post_assign_subtract(t, tmp) - tmp);
}



}



#endif
#endif

#include "cogs/operators.hpp"