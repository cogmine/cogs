//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_ARCH_BIT_SCAN
#define COGS_ARCH_BIT_SCAN

#include <type_traits>

namespace cogs {

/// @brief Namespace for architecture-specific functionality
namespace arch {


// arch/bit_scan.hpp provides arch level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// If arch is 'unknown' we use the brute force approach.
	

// bits must not be zero
template <typename int_t>
inline typename std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	size_t result = 0;
	COGS_ASSERT(!bits);
	typename std::make_unsigned<int_t>::type bits2 = bits;
	for (;;)
	{
		bits2 >>= 1;
		if (!bits2)
			break;
		result++;
	}
	return result;
}


// bits must not be zero
template <typename int_t>
inline typename std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	size_t
>
bit_scan_forward(const int_t& bits)
{
	size_t result = 0;
	COGS_ASSERT(!bits);
	typename std::make_unsigned<int_t>::type bits2 = bits;
	while ((bits2 & 1) != 1)
	{
		result++;
		bits2 >>= 1;
	}
	return result;
}



}
}


#endif


