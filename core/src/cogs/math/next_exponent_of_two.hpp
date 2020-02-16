//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_NEXT_EXPONENT_OF_TWO
#define COGS_HEADER_MATH_NEXT_EXPONENT_OF_TWO


#include "cogs/math/prev_exponent_of_two.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the next exponent of two of a constant integer
/// @tparam n Constant value to get the next exponent of 2 from.
template <size_t n>
class next_exponent_of_two
{
public:
	static constexpr size_t value = prev_or_current_exponent_of_two< (n << 1) >::value;
};
template <size_t n>
constexpr size_t next_exponent_of_two_v = next_exponent_of_two<n>::value;


template <>
class next_exponent_of_two<0>
{
public:
	static constexpr size_t value = 1;
};


template <size_t n>
class next_or_current_exponent_of_two
{
public:
	static constexpr size_t value = prev_or_current_exponent_of_two< (n << 1) - 1 >::value;
};
template <size_t n>
constexpr size_t next_or_current_exponent_of_two_v = next_or_current_exponent_of_two<n>::value;

template <>
class next_or_current_exponent_of_two<0>
{
public:
	static constexpr size_t value = 1;
};


}


#endif

