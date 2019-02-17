//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_SCAN
#define COGS_HEADER_ENV_MEM_BIT_SCAN

#include <type_traits>

#include "cogs/mem/int_parts.hpp"


namespace cogs {


// env/bit_scan.hpp provides env level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// On gcc, use __builtin_clzl/__builtin_ctzl, __builtin_clzll/__builtin_ctzll, for bit scanning



// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) <= sizeof(unsigned int)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	return (sizeof(unsigned int) * 8) - __builtin_clz((unsigned int)bits);
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned int))
	&& (sizeof(int_t) <= sizeof(unsigned long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	return (sizeof(unsigned long) * 8) - __builtin_clzl((unsigned long)bits);
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned long))
	&& (sizeof(int_t) <= sizeof(unsigned long long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	return (sizeof(unsigned long long) * 8) - __builtin_clzll((unsigned long long)bits);
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned long long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	size_t result = 0;
	std::make_unsigned_t<int_t> bits2 = bits;
	unsigned long long ll;
	for (;;)
	{
		ll = (unsigned long long)bits2;
		bits2 >>= (sizeof(unsigned long long) * 8);
		if (!bits2)
			break;
		result += (sizeof(unsigned long long) * 8);
	}
	return result + bit_scan_reverse(ll);
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) <= sizeof(unsigned int)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	return __builtin_ctz((unsigned int)bits);
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned int))
	&& (sizeof(int_t) <= sizeof(unsigned long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	return __builtin_ctzl((unsigned long)bits);
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned long))
	&& (sizeof(int_t) <= sizeof(unsigned long long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	return __builtin_ctzll((unsigned long long)bits);
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value
	&& (sizeof(int_t) > sizeof(unsigned long long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	size_t result = 0;
	std::make_unsigned_t<int_t> bits2 = bits;
	unsigned long long ll;
	for (;;)
	{
		ll = (unsigned long long)bits2;
		if (!!ll)
			break;

		bits2 >>= (sizeof(unsigned long long) * 8);
		result += (sizeof(unsigned long long) * 8);
	}
	return result + bit_scan_forward(ll);
}



}


#endif

