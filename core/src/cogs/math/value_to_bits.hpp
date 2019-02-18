//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_VALUE_TO_BITS
#define COGS_HEADER_MATH_VALUE_TO_BITS


#include "cogs/math/range_to_bits.hpp"


namespace cogs {

/// @ingroup ConstMath
/// @brief Template helper that converts a value to the number of bits necessary to represent it.
/// @tparam v The value
template <longest v>
class value_to_bits
{
public:
	static constexpr size_t value = (v >= 0) ? range_to_bits_v<0, v> : range_to_bits_v<v, 0>;
};
template <ulongest v>
inline constexpr size_t value_to_bits_v = value_to_bits<v>::value;


}


#endif

