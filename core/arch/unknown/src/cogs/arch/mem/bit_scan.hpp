//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_ARCH_BIT_SCAN
#define COGS_HEADER_ARCH_BIT_SCAN

#include <type_traits>

#include "cogs/load.hpp"

namespace cogs {

/// @brief Namespace for architecture-specific functionality
namespace arch {


// arch/bit_scan.hpp provides arch level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// If arch is 'unknown' we use the brute force approach.

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>,
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	size_t result = 0;
	COGS_ASSERT(!bits);
	std::make_unsigned_t<int_t> bits2 = (std::make_unsigned_t<int_t>)load(bits);
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
inline std::enable_if_t<
	std::is_integral_v<int_t>,
	size_t
>
bit_scan_forward(const int_t& bits)
{
	size_t result = 0;
	COGS_ASSERT(!bits);
	std::make_unsigned_t<int_t> bits2 = (std::make_unsigned_t<int_t>)load(bits);
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
