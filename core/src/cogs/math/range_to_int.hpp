//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_RANGE_TO_INT
#define COGS_HEADER_MATH_RANGE_TO_INT


#include "cogs/math/bytes_to_int.hpp"
#include "cogs/math/range_to_bytes.hpp"


namespace cogs {


/// @ingroup Math
/// @brief Template helper to provide an integer type sufficient to represent the range.
/// @tparam min_value Minimum value of the range
/// @tparam max_value Maximum value of the range
template <ulongest min_value, ulongest max_value>
class range_to_int
{
public:
	typedef bytes_to_int_t<range_to_int_bytes_v<min_value, max_value>, (min_value < 0)> type;
};
template <ulongest min_value, ulongest max_value> using range_to_int_t = typename range_to_int<min_value, max_value>::type;

}


#endif
