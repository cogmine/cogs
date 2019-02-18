//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_HEADER_OPERATORS
#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_OPERATORS
#define COGS_HEADER_ENV_SYNC_ATOMIC_OPERATORS


#include "cogs/math/bytes_to_int.hpp"
#include "cogs/os/sync/atomic_operators.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {


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

template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
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
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_add_fetch((uint_t*)(unsigned char*)&t, 1, __ATOMIC_SEQ_CST);
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
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_add_fetch((uint_t*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}


template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_next(T& t)
{
	return os::atomic::post_assign_next(t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_fetch_add((uint_t*)(unsigned char*)&t, 1, __ATOMIC_SEQ_CST);
}

template <typename T>
inline std::enable_if_t<
	std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_next(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_fetch_add((uint_t*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
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



// prev


template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
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
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_sub_fetch((uint_t*)(unsigned char*)&t, 1, __ATOMIC_SEQ_CST);
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
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_sub_fetch((uint_t*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}



template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_prev(T& t)
{
	return os::atomic::post_assign_prev(t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_fetch_sub((uint_t*)(unsigned char*)&t, 1, __ATOMIC_SEQ_CST);
}

template <typename T>
inline std::enable_if_t<
	std::is_pointer_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_prev(T& t)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	return (std::remove_volatile_t<T>)__atomic_fetch_sub((uint_t*)(unsigned char*)&t, sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
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




// bit_and


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
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_and_fetch((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_bit_and(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_and((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_and(T& t, const A1 & a)
{
	pre_assign_bit_and(t, a);
}





// bit_or


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
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_or_fetch((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_bit_or(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_or((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_or(T& t, const A1 & a)
{
	pre_assign_bit_or(t, a);
}






// bit_xor


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
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_xor_fetch((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_bit_xor(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_xor((uint_t*)(unsigned char*)&t, (uint_t)tmp, __ATOMIC_SEQ_CST);
}


template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_bit_xor(T& t, const A1 & a)
{
	pre_assign_bit_xor(t, a);
}





// add


template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
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
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_add_fetch((char*)(unsigned char*)&t, (char)tmp, __ATOMIC_SEQ_CST);
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
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_add_fetch((uint_t*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}




template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
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
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_add(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_add((char*)(unsigned char*)&t, (char)tmp, __ATOMIC_SEQ_CST);
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
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_add((uint_t*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& (std::is_integral_v<T> || std::is_pointer_v<T> || std::is_floating_point_v<T>)
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_add(T& t, const A1& a)
{
	pre_assign_add(t, a);
}



// subtract


template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
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
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
pre_assign_subtract(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_sub_fetch((char*)(unsigned char*)&t, (char)tmp, __ATOMIC_SEQ_CST);
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
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_sub_fetch((uint_t*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}




template <typename T>
inline std::enable_if_t<
	std::is_floating_point_v<T>
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
	&& std::is_integral_v<T>
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	std::remove_volatile_t<T>
>
post_assign_subtract(T& t, const A1& a)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_sub((char*)(unsigned char*)&t, (char)tmp, __ATOMIC_SEQ_CST);
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
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	ptrdiff_t tmp;
	cogs::assign(tmp, a);
	return (std::remove_volatile_t<T>)__atomic_fetch_sub((uint_t*)(unsigned char*)&t, tmp * sizeof(std::remove_pointer_t<T>), __ATOMIC_SEQ_CST);
}



template <typename T, typename A1>
inline std::enable_if_t<
	can_atomic_v<T>
	&& (std::is_integral_v<T> || std::is_pointer_v<T> || std::is_floating_point_v<T>)
	&& std::is_volatile_v<T>
	&& !std::is_const_v<T>,
	void
>
assign_subtract(T& t, const A1& a)
{
	pre_assign_subtract(t, a);
}



}



#endif
#endif

#include "cogs/operators.hpp"
