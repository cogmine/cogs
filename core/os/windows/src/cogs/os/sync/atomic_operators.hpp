//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifdef COGS_HEADER_ENV_SYNC_ATOMIC_OPERATORS
#ifndef COGS_HEADER_OS_SYNC_ATOMIC_OPERATORS
#define COGS_HEADER_OS_SYNC_ATOMIC_OPERATORS


#include "cogs/arch/sync/atomic_operators.hpp"

namespace cogs {
namespace os {
namespace atomic {

// Allow env layer to handle all Windows atomics

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(next)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(prev)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_and)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_or)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_xor)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(add)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(subtract)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(not)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(abs)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_not)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(negative)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_count)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_forward)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_reverse)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(endian_swap)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_left)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_right)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_left)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_right)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_subtract)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(multiply)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(modulo)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_modulo)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(reciprocal)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide_whole)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide_whole)

COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(gcd)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lcm)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(greater)
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lesser)

//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	void>
//exchange(volatile T& t, const T& src, T& rtn)
//{
//	rtn = (T)InterlockedExchange8((char*)(unsigned char*)&t, (char)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	void>
//exchange(volatile T& t, const T& src, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	rtn = (T)InterlockedExchange16((short*)(unsigned char*)&t, (short)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	void>
//exchange(volatile T& t, const T& src, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	rtn = (T)InterlockedExchange((long*)(unsigned char*)&t, (long)src);
//}
//
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	void>
//exchange(T& t, const T& src, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	rtn = (T)InterlockedExchange64((__int64*)(unsigned char*)&t, (__int64)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	T
//>
//exchange(volatile T& t, const T& src)
//{
//	return (T)InterlockedExchange8((char*)(unsigned char*)&t, (char)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	T
//>
//exchange(volatile T& t, const T& src)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (T)InterlockedExchange16((short*)(unsigned char*)&t, (short)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	T
//>
//exchange(volatile T& t, const T& src)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (T)InterlockedExchange((long*)(unsigned char*)&t, (long)src);
//}
//
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//exchange(T& t, const T& src)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (T)InterlockedExchange64((__int64*)(unsigned char*)&t, (__int64)src);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
//{
//	char tmp = InterlockedCompareExchange8((char*)(unsigned char*)&t, (char)src, (char)cmp);
//	bool b = tmp == (char)cmp;
//	rtn = (T)tmp;
//	return b;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	short tmp = InterlockedCompareExchange16((short*)(unsigned char*)&t, (short)src, (short)cmp);
//	bool b = tmp == (short)cmp;
//	rtn = (T)tmp;
//	return b;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	long tmp = InterlockedCompareExchange((long*)(unsigned char*)&t, (long)src, (long)cmp);
//	bool b = tmp == (long)cmp;
//	rtn = (T)tmp;
//	return b;
//}
//
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	__int64 tmp = InterlockedCompareExchange64((__int64*)(unsigned char*)&t, (__int64)src, (__int64)cmp);
//	bool b = tmp == (__int64)cmp;
//	rtn = (T)tmp;
//	return b;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp)
//{
//	return InterlockedCompareExchange8((char*)(unsigned char*)&t, (char)src, (char)cmp) == (char)cmp;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return InterlockedCompareExchange16((short*)(unsigned char*)&t, (short)src, (short)cmp) == (short)cmp;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return InterlockedCompareExchange((long*)(unsigned char*)&t, (long)src, (long)cmp) == (long)cmp;
//}
//
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return InterlockedCompareExchange64((__int64*)(unsigned char*)&t, (__int64)src, (__int64)cmp) == (__int64)cmp;
//}
//
//
//#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
//
//
//template <typename T>
//inline std::enable_if_t<
//	!std::is_empty_v<T>
//	&& can_atomic_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	void
//>
//load(const volatile T& t, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	// Note: CMPXCHG/CMPXCHG8B/CMPXCHG16B all issue an implicit write, even if not modified.
//	volatile T* srcPtr = const_cast<volatile T*>(&t);
//	__int64 tmpCmp[2] = {};
//	InterlockedCompareExchange128((__int64*)(unsigned char*)srcPtr, tmpCmp[1], tmpCmp[0], tmpCmp);
//	bypass_strict_aliasing(tmpCmp, rtn);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	!std::is_empty_v<T>
//	&& can_atomic_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	T
//>
//load(const volatile T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	// Note: CMPXCHG/CMPXCHG8B/CMPXCHG16B all issue an implicit write, even if not modified.
//	volatile T* srcPtr = const_cast<volatile T*>(&t);
//	__int64 tmpCmp[2] = {};
//	InterlockedCompareExchange128((__int64*)(unsigned char*)srcPtr, tmpCmp[1], tmpCmp[0], tmpCmp);
//	T rtn;
//	bypass_strict_aliasing(tmpCmp, rtn);
//	return rtn;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& !std::is_empty_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	void
//>
//store(volatile T& t, const T& src)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	__int64 tmpSrc[2];
//	bypass_strict_aliasing(src, tmpSrc);
//	__int64 tmpCmp[2] = {};
//	do { } while (0 == InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp));
//}
//
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	void
//>
//exchange(volatile T& t, const T& src, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	__int64 tmpSrc[2];
//	bypass_strict_aliasing(src, tmpSrc);
//	__int64 tmpCmp[2] = {};
//	do { } while (0 == InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp));
//	bypass_strict_aliasing(tmpCmp, rtn);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	__int64 tmpSrc[2];
//	bypass_strict_aliasing(src, tmpSrc);
//	__int64 tmpCmp[2];
//	bypass_strict_aliasing(cmp, tmpCmp);
//	bool b = InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp) != 0;
//	bypass_strict_aliasing(tmpCmp, rtn);
//	return b;
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(__int64))
//	&& (sizeof(T) <= (sizeof(__int64) * 2)),
//	bool
//>
//compare_exchange(volatile T& t, const T& src, const T& cmp)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	__int64 tmpSrc[2];
//	bypass_strict_aliasing(src, tmpSrc);
//	__int64 tmpCmp[2];
//	bypass_strict_aliasing(cmp, tmpCmp);
//	return InterlockedCompareExchange128((__int64*)(unsigned char*)&t, tmpSrc[1], tmpSrc[0], tmpCmp) != 0;
//}
//
//
//#endif
//
//
//// next
//
//// We fall back to the default (compare_exchange), if:
////   1 byte, since there is no InterlockedIncrement8
////   It's a floating point, as there is no intrinsic for it.
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_arithmetic_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& ((sizeof(T) <= sizeof(char)) || std::is_floating_point_v<T>),
//	std::remove_volatile_t<T>
//>
//pre_assign_next(T& t)
//{
//	return arch::atomic::pre_assign_next(t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//pre_assign_next(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedIncrement16((short*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//pre_assign_next(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedIncrement((long*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//pre_assign_next(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedIncrement64((__int64*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_next(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
//	return (std::remove_volatile_t<T>)InterlockedAdd64((__int64*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>));
//#else
//	return (std::remove_volatile_t<T>)InterlockedAdd((__int64*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>));
//#endif
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_next(T& t)
//{
//	pre_assign_next(t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_next(T& t)
//{
//	return pre_assign_next(t) - 1;
//}
//
//
//// prev
//
//// We fall back to the default (compare_exchange), if:
////   1 byte, since there is no InterlockedDecrement8
////   It's a floating point, as there is no intrinsic for it.
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_arithmetic_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& ((sizeof(T) <= sizeof(char)) || std::is_floating_point_v<T>),
//	std::remove_volatile_t<T>
//>
//pre_assign_prev(T& t)
//{
//	return arch::atomic::pre_assign_prev(t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//pre_assign_prev(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedDecrement16((short*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//pre_assign_prev(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedDecrement((long*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//pre_assign_prev(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	return (std::remove_volatile_t<T>)InterlockedDecrement64((__int64*)(unsigned char*)&t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_prev(T& t)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
//	return (std::remove_volatile_t<T>)InterlockedAdd64((__int64*)(unsigned char*)&t, -sizeof(std::remove_pointer_t<T>));
//#else
//	return (std::remove_volatile_t<T>)InterlockedAdd((__int64*)(unsigned char*)&t, -sizeof(std::remove_pointer_t<T>));
//#endif
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_prev(T& t)
//{
//	pre_assign_prev(t);
//}
//
//template <typename T>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_scalar_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_prev(T& t)
//{
//	return pre_assign_prev(t) + 1;
//}
//
//
//// bit_and
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_and(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedAnd8((char*)(unsigned char*)&t, (char)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_and(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedAnd16((short*)(unsigned char*)&t, (short)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_and(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedAnd((long*)(unsigned char*)&t, (long)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_and(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedAnd64((__int64*)(unsigned char*)&t, (__int64)tmp);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_bit_and(T& t, const A1& a)
//{
//	post_assign_bit_and(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_bit_and(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_bit_and(t, tmp) & tmp);
//}
//
//
//// bit_or
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_or(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedOr8((char*)(unsigned char*)&t, (char)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_or(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedOr16((short*)(unsigned char*)&t, (short)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_or(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedOr((long*)(unsigned char*)&t, (long)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_or(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedOr64((__int64*)(unsigned char*)&t, (__int64)tmp);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_bit_or(T& t, const A1& a)
//{
//	post_assign_bit_or(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_bit_or(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_bit_or(t, tmp) | tmp);
//}
//
//
//// bit_xor
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_xor(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedXor8((char*)(unsigned char*)&t, (char)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_xor(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedXor16((short*)(unsigned char*)&t, (short)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_xor(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedXor((long*)(unsigned char*)&t, (long)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//post_assign_bit_xor(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedXor64((__int64*)(unsigned char*)&t, (__int64)tmp);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_bit_xor(T& t, const A1& a)
//{
//	post_assign_bit_xor(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_bit_xor(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_bit_xor(t, tmp) ^ tmp);
//}
//
//
//// add
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd8((char*)(unsigned char*)&t, (char)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd16((short*)(unsigned char*)&t, (short)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd((long*)(unsigned char*)&t, (long)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(long))
//	&& (sizeof(T) <= sizeof(__int64)),
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, (__int64)tmp);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	ptrdiff_t tmp;
//	cogs::assign(tmp, a);
//#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>));
//#else
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd((__int64*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>));
//#endif
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_add(T& t, const A1& a)
//{
//	arch::atomic::assign_add(t, a);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_add(T& t, const A1& a)
//{
//	return arch::atomic::pre_assign_add(t, a);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_add(T& t, const A1& a)
//{
//	return arch::atomic::post_assign_add(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& (is_integral_v<T> || std::is_pointer_v<T>)
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_add(T& t, const A1& a)
//{
//	post_assign_add(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_add(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_add(t, tmp) + tmp);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_add(T& t, const A1& a)
//{
//	ptrdiff_t tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_add(t, tmp) + tmp);
//}
//
//
//// subtract
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) <= sizeof(char)),
//	std::remove_volatile_t<T>
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd8((char*)(unsigned char*)&t, -(char)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(char))
//	&& (sizeof(T) <= sizeof(short)),
//	std::remove_volatile_t<T>
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd16((short*)(unsigned char*)&t, -(short)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>
//	&& (sizeof(T) > sizeof(short))
//	&& (sizeof(T) <= sizeof(long)),
//	std::remove_volatile_t<T>
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	T tmp;
//	cogs::assign(tmp, a);
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd((long*)(unsigned char*)&t, -(long)tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	COGS_ASSERT((size_t)&t % cogs::atomic::get_alignment_v<T> == 0);
//	ptrdiff_t tmp;
//	cogs::assign(tmp, a);
//#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd64((__int64*)(unsigned char*)&t, tmp * -(ptrdiff_t)sizeof(std::remove_pointer_t<T>));
//#else
//	return (std::remove_volatile_t<T>)InterlockedExchangeAdd((__int64*)(unsigned char*)&t, tmp * -(ptrdiff_t)sizeof(std::remove_pointer_t<T>));
//#endif
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	ptrdiff_t tmp;
//	cogs::assign(tmp, a);
//	return post_assign_subtract(t, tmp * sizeof(std::remove_pointer_t<T>));
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_subtract(T& t, const A1& a)
//{
//	arch::atomic::assign_subtract(t, a);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_subtract(T& t, const A1& a)
//{
//	return arch::atomic::pre_assign_subtract(t, a);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_floating_point_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//post_assign_subtract(T& t, const A1& a)
//{
//	return arch::atomic::post_assign_subtract(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& (is_integral_v<T> || std::is_pointer_v<T>)
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	void
//>
//assign_subtract(T& t, const A1& a)
//{
//	post_assign_subtract(t, a);
//}
//
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& is_integral_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_subtract(T& t, const A1& a)
//{
//	T tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_subtract(t, tmp) - tmp);
//}
//
//template <typename T, typename A1>
//inline std::enable_if_t<
//	can_atomic_v<T>
//	&& std::is_pointer_v<T>
//	&& std::is_volatile_v<T>
//	&& !std::is_const_v<T>,
//	std::remove_volatile_t<T>
//>
//pre_assign_subtract(T& t, const A1& a)
//{
//	ptrdiff_t tmp;
//	cogs::assign(tmp, a);
//	return (post_assign_subtract(t, tmp) - tmp);
//}
//
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(not)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(abs)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_not)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(negative)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_count)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_forward)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_reverse)
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(endian_swap)
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_left)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_right)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_left)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_right)
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_subtract)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(multiply)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(modulo)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_modulo)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide)
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(reciprocal)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide_whole)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide_whole)
//
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(gcd)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lcm)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(greater)
//COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lesser)


}
}
}


#endif
#endif

#include "cogs/env/sync/atomic_operators.hpp"
