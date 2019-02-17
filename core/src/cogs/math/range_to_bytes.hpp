//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_RANGE_TO_BYTES
#define COGS_HEADER_MATH_RANGE_TO_BYTES


#include "cogs/math/bits_to_bytes.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Template helper that converts a range to the number of bytes necessary to represent it.
/// @tparam min_value Minimum value of the range
/// @tparam max_value Maximum value of the range
template <ulongest min_value, ulongest max_value>
class range_to_bytes
{
public:
	static constexpr size_t value = bits_to_bytes< range_to_bits<min_value, max_value>::value >::value;
};

template <ulongest min_value, ulongest max_value>
constexpr size_t range_to_bytes_v = range_to_bytes<min_value, max_value>::value;






template <ulongest min_value, ulongest max_value>
class range_to_int_bytes
{
public:
	// rounded up to an exponent of 2.  I.e. 1 byte, 2 bytes, 4 bytes, 8 bytes.
	static constexpr size_t value = (min_value == max_value) ?
		0 :
		bits_to_bytes_v<
			next_or_current_exponent_of_two_v<
				range_to_bits<min_value, max_value>::value>
		>;
};



template <ulongest min_value, ulongest max_value>
constexpr size_t range_to_int_bytes_v = range_to_int_bytes<min_value, max_value>::value;




}


#endif

