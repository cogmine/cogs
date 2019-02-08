//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENV_BIT_ROTATE
#define COGS_ENV_BIT_ROTATE


#include "cogs/math/int_types.hpp"


namespace cogs {
namespace env {


// env/bit_rotate.hpp provides env level implementation of bit_rotate_left and bit_rotate_right.
// Use of compiler intrinsics allow specialized instructions on platforms that support them.


template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	int_t
>
bit_rotate_right(const int_t& bits, size_t n)
{
	if (!n)
		return bits;
	typedef std::make_unsigned_t<int_t> uint_t;
	uint_t bits2 = (uint_t)bits;
	return (int_t)((bits2 >> n) | (bits2 << ((sizeof(int_t) * 8) - n)));
}


template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	int_t
>
bit_rotate_left(const int_t& bits, size_t n)
{
	if (!n)
		return bits;
	return ((bits << n) | (bits >> ((sizeof(int_t) * 8) - n)));
}



}
}


#endif

