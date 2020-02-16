//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good


#ifndef COGS_HEADER_MATH_EXTUMUL
#define COGS_HEADER_MATH_EXTUMUL


#include "cogs/env.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {


/// @ingroup Math
/// @brief Computes the extended multiplication of two integers
/// @tparam uint_t Unsigned int type
template <typename uint_t>
inline uint_t extumul(const uint_t& src1, const uint_t& src2, uint_t& highPartRtn)
{
	//     F F
	//     F F
	// ---------
	//     E 1
	//   E 1
	//   E 1
	// E 1
	//----------
	// F E 0 1

	uint_t l1 = get_low_part(src1);
	uint_t l2 = get_low_part(src2);
	uint_t h1 = get_high_part(src1);
	uint_t h2 = get_high_part(src2);

	uint_t ll = l1 * l2;
	uint_t lh = l1 * h2;
	uint_t hl = h1 * l2;
	uint_t hh = h1 * h2;

	//     F F
	//     F F
	// ---------
	//   E F 1
	//   E 1
	// E 1
	//----------
	// F E 0 1

	uint_t llh = get_high_part(ll);
	lh += llh; // will not overflow

	//     F F
	//     F F
	// ---------
	//   D 0 1
	// F 1
	//----------
	// F E 0 1

	uint_t mid = lh + hl; // might overflow
	if (mid < lh) // if overflow
		hh += make_const_high_part_v<uint_t, 1>;

	uint_t mh = get_high_part(mid);
	uint_t ml_shifted = make_high_part(mid);

	uint_t lll = get_low_part(ll);

	uint_t low = ml_shifted | lll;
	uint_t high = hh + mh; // will not overflow

	highPartRtn = high;
	return low;
}


}


#endif
