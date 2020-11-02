//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	is_integral_v<T>,
	std::remove_volatile_t<T>
>
bit_rotate_right(const T& t, const A1& n)
{
	if (!n)
		return t;
	typedef make_unsigned_t<T> uint_t;
	uint_t bits2 = (uint_t)t;
	return (T)((bits2 >> (int)reduce_integer_type(n)) | (bits2 << ((sizeof(T) * 8) - (int)reduce_integer_type(n))));
}


template <typename T, typename A1>
inline std::enable_if_t<
	is_integral_v<T>,
	std::remove_volatile_t<T>
>
bit_rotate_left(const T& t, const A1& n)
{
	if (!n)
		return t;
	typedef make_unsigned_t<T> uint_t;
	uint_t bits2 = (uint_t)t;
	return (T)((bits2 << (int)reduce_integer_type(n)) | (bits2 >> ((sizeof(T) * 8) - (int)reduce_integer_type(n))));
}


}


#endif
