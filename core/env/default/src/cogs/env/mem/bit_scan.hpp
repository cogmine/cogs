//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENV_BIT_SCAN
#define COGS_ENV_BIT_SCAN


#include "cogs/arch/mem/const_bit_scan.hpp"


namespace cogs {
namespace env {


// env/bit_scan.hpp provides env level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// No atomic support if env is 'default'.  Uses arch level.


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	size_t
>
bit_scan_reverse(const int_t& bits)
{
	return arch::bit_scan_reverse(bits);
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	std::is_integral<int_t>::value
	&& !std::is_volatile<int_t>::value,
	size_t
>
bit_scan_forward(const int_t& bits)
{
	return arch::bit_scan_forward(bits);
}






}
}


#endif

