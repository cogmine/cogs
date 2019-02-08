//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CONST_UROOT
#define COGS_CONST_UROOT


#include "cogs/math/const_upow.hpp"
#include "cogs/math/const_max_int.hpp"


namespace cogs {



/// @ingroup ConstMath
/// @brief Meta template to calculate a constant root of a constant value.
/// @tparam exponent Constant exponent value
/// @tparam low_part Low part of constant value to root by exponent.
/// @tparam high_part Low part of constant value to root by exponent.  Default: 0
template <ulongest exponent, ulongest low_part, ulongest high_part = 0>
class const_uroot
{
private:
	template <ulongest lower = 0, ulongest higher = const_max_int<ulongest>::value >
	class helper;

	template <ulongest answer>
	class helper<answer, answer>
	{
	public:
		static const ulongest value = answer;
	};

	template <ulongest lower, ulongest higher>
	class helper
	{
	private:
		static const ulongest high_bit = (ulongest)1 << ((sizeof(ulongest) * 8) - 1);
		static const ulongest mid = ((higher - lower) / 2) + lower;

		static const ulongest high_high_pow = const_upow<exponent, mid>::high_high_part;
		static const ulongest low_high_pow = const_upow<exponent, mid>::low_high_part;
		static const ulongest high_low_pow = const_upow<exponent, mid>::high_low_part;
		static const ulongest low_low_pow = const_upow<exponent, mid>::low_low_part;

		static const bool is_mid_less_than_or_equal_target = (!!high_high_pow || !!low_high_pow) ? false :
																(high_low_pow < high_part ? true :
																	(high_low_pow > high_part ? false :
																		(low_low_pow <= low_part)));

		static const bool is_mid_greater_than_or_equal_target = (!!high_high_pow || !!low_high_pow) ? true :
																	(high_low_pow < high_part ? false :
																		(high_low_pow > high_part ? true :
																			(low_low_pow >= low_part)));

	public:
		static const ulongest value = helper<(is_mid_less_than_or_equal_target ? mid : lower), (is_mid_greater_than_or_equal_target ? mid : (higher - 1))>::value;
	};

public:
	static const ulongest value = helper<>::value;
};



}


#endif

