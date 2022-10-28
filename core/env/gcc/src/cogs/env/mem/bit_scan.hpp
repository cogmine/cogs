//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_SCAN
#define COGS_HEADER_ENV_MEM_BIT_SCAN

#include <type_traits>

#include "cogs/load.hpp"
#include "cogs/env.hpp"


namespace cogs {


// env/bit_scan.hpp provides env level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// On gcc, use __builtin_clzl/__builtin_ctzl, __builtin_clzll/__builtin_ctzll, for bit scanning


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) <= sizeof(unsigned int)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return (sizeof(unsigned int) * 8) - __builtin_clz((unsigned int)load(bits));
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned int))
	&& (sizeof(int_t) <= sizeof(unsigned long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return (sizeof(unsigned long) * 8) - __builtin_clzl((unsigned long)load(bits));
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long))
	&& (sizeof(int_t) <= sizeof(unsigned long long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return (sizeof(unsigned long long) * 8) - __builtin_clzll((unsigned long long)load(bits));
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long long)),
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	size_t result = 0;
	make_unsigned_t<int_t> bits2 = (make_unsigned_t<int_t>)load(bits);
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
	is_integral_v<int_t>
	&& (sizeof(int_t) <= sizeof(unsigned int)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return __builtin_ctz((unsigned int)load(bits));
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned int))
	&& (sizeof(int_t) <= sizeof(unsigned long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return __builtin_ctzl((unsigned long)load(bits));
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long))
	&& (sizeof(int_t) <= sizeof(unsigned long long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	return __builtin_ctzll((unsigned long long)load(bits));
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > sizeof(unsigned long long)),
	size_t
>
bit_scan_forward(const int_t& bits)
{
	COGS_ASSERT(!!bits);
	size_t result = 0;
	make_unsigned_t<int_t> bits2 = (make_unsigned_t<int_t>)load(bits);
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
