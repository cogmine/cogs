//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_ROTATE
#define COGS_HEADER_ENV_MEM_BIT_ROTATE


#include "cogs/env.hpp"
#include "cogs/math/fixed_integer.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_extended.hpp"


namespace cogs {


// env/bit_rotate.hpp provides env level implementation of bit_rotate_left and bit_rotate_right.
// Use of compiler intrinsics allow specialized instructions on platforms that support them.


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(unsigned char)),
	std::remove_volatile_t<T>
>
bit_rotate_right(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotr8((unsigned char)load(t), (unsigned char)reduce_integer_type(n));
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned char))
	&& (sizeof(T) <= sizeof(unsigned short)),
	std::remove_volatile_t<T>
>
bit_rotate_right(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotr16((unsigned short)load(t), (unsigned char)reduce_integer_type(n));
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned short))
	&& (sizeof(T) <= sizeof(unsigned long)),
	std::remove_volatile_t<T>
>
bit_rotate_right(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_lrotr((unsigned long)load(t), (int)reduce_integer_type(n));
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long))
	&& (sizeof(T) <= sizeof(unsigned __int64)),
	std::remove_volatile_t<T>
>
bit_rotate_right(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotr64((unsigned __int64)load(t), (int)reduce_integer_type(n));
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(unsigned char)),
	std::remove_volatile_t<T>
>
bit_rotate_left(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotl8((unsigned char)load(t), (unsigned char)reduce_integer_type(n));
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned char))
	&& (sizeof(T) <= sizeof(unsigned short)),
	std::remove_volatile_t<T>
>
bit_rotate_left(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotl16((unsigned short)load(t), (unsigned char)reduce_integer_type(n));
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned short))
	&& (sizeof(T) <= sizeof(unsigned long)),
	std::remove_volatile_t<T>
>
bit_rotate_left(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_lrotl((unsigned long)load(t), (int)reduce_integer_type(n));
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long))
	&& (sizeof(T) <= sizeof(unsigned __int64)),
	std::remove_volatile_t<T>
>
bit_rotate_left(const T& t, const A1& n)
{
	return (std::remove_volatile_t<T>)_rotl64((unsigned __int64)load(t), (int)reduce_integer_type(n));
}


}


#endif

