//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, Placeholder

#ifndef COGS_HEADER_ARCH_MEM_BIT_SCAN
#define COGS_HEADER_ARCH_MEM_BIT_SCAN

#include <type_traits>

#include "cogs/load.hpp"

namespace cogs {
namespace arch {


// arch/bit_scan.hpp provides arch level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// At this time, no inline asm ops are implemented for x86.
// This file is a placeholder, should they become necessary.
// bit_scan_forward and bit_scan_reverse should be available at the env level.


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>,
	size_t
>
bit_scan_reverse(const int_t& bits);


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral_v<int_t>,
	size_t
>
bit_scan_forward(const int_t& bits);


}
}


#endif
