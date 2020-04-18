//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_COUNT
#define COGS_HEADER_ENV_MEM_BIT_COUNT


#include <smmintrin.h>
#include "cogs/math/bytes_to_int.hpp"


namespace cogs {


template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>
	&& !std::is_volatile_v<int_t>
	&& (sizeof(int_t) <= sizeof(unsigned int)),
	size_t
>
bit_count(const int_t& bits)
{
	return __builtin_popcount((unsigned int)bits);
}


template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>
	&& !std::is_volatile_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned int))
	&& (sizeof(int_t) <= sizeof(unsigned long)),
	size_t
>
bit_count(const int_t& bits)
{
	return __builtin_popcountl((unsigned long)bits);
}


template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>
	&& !std::is_volatile_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long))
	&& (sizeof(int_t) <= sizeof(unsigned long long)),
	size_t
>
bit_count(const int_t& bits)
{
	return __builtin_popcountll((unsigned long long)bits);
}


template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>
	&& !std::is_volatile_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long long)),
	size_t
>
bit_count(const int_t& bits)
{
	size_t result = 0;
	std::make_unsigned_t<int_t> bits2 = bits;
	while (!!bits2)
	{
		unsigned long long ll = (unsigned long long)bits2;
		result += bit_count(ll);
		bits2 >>= (sizeof(unsigned long long) * 8);
	}
	return result;
}


}


#endif
