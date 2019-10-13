//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_UPOW
#define COGS_HEADER_MATH_CONST_UPOW


#include "cogs/math/extumul.hpp"


namespace cogs {

/// @defgroup ConstMath Constant Math
/// @{
/// @ingroup Math
/// @brief Constant Math
/// @}

/// @ingroup ConstMath
/// @brief Meta programming template to calculate a constant value to a constant exponent.
/// @tparam exponent Constant exponent value
/// @tparam v Constant value to apply exponent to
template <ulongest exponent, ulongest v>
class const_upow
{
private:
	static constexpr ulongest half = exponent / 2;
	static constexpr ulongest rest = exponent - half;

	// Even a square takes up to twice as many bits as the original.
	// So, we ignore the high bit until the last multiplication.  Only longest*2 results are supported.

	static constexpr ulongest half_pow_high_part = const_upow<half, v>::high_part;
	static constexpr ulongest half_pow_low_part = const_upow<half, v>::low_part;

	static constexpr ulongest rest_pow_high_part = const_upow<rest, v>::high_part;
	static constexpr ulongest rest_pow_low_part = const_upow<rest, v>::low_part;

public:
	static constexpr ulongest high_high_part = const_extumul2<half_pow_high_part, half_pow_low_part, rest_pow_high_part, half_pow_low_part>::high_high_part;
	static constexpr ulongest low_high_part = const_extumul2<half_pow_high_part, half_pow_low_part, rest_pow_high_part, half_pow_low_part>::low_high_part;
	static constexpr ulongest high_low_part = const_extumul2<half_pow_high_part, half_pow_low_part, rest_pow_high_part, half_pow_low_part>::high_low_part;
	static constexpr ulongest low_low_part = const_extumul2<half_pow_high_part, half_pow_low_part, rest_pow_high_part, half_pow_low_part>::low_low_part;

	static constexpr ulongest high_part = high_low_part;
	static constexpr ulongest low_part = low_low_part;

	static constexpr ulongest value = low_part;
};


template <ulongest v>
class const_upow<1, v>
{
public:
	static constexpr ulongest high_high_part = 0;
	static constexpr ulongest low_high_part = 0;
	static constexpr ulongest high_low_part = 0;
	static constexpr ulongest low_low_part = v;

	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = v;

	static constexpr ulongest value = v;
};

template <ulongest v>
class const_upow<0, v>
{
public:
	static constexpr ulongest high_high_part = 0;
	static constexpr ulongest low_high_part = 0;
	static constexpr ulongest high_low_part = 0;
	static constexpr ulongest low_low_part = 1;

	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = 1;

	static constexpr ulongest value = 1;
};


}


#endif

