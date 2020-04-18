//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_EXTUMUL
#define COGS_HEADER_MATH_CONST_EXTUMUL


#include "cogs/env.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the extended multiplication of two constant integers
/// @tparam x Value to multiply
/// @tparam y Value to multiply
template <ulongest x, ulongest y>
class const_extumul
{
private:
	static constexpr ulongest low_x = get_const_low_part_v<ulongest, x>;
	static constexpr ulongest low_y = get_const_low_part_v<ulongest, y>;
	static constexpr ulongest high_x = get_const_high_part_v<ulongest, x>;
	static constexpr ulongest high_y = get_const_high_part_v<ulongest, y>;
	static constexpr ulongest ll = low_x * low_y;
	static constexpr ulongest lh = low_x * high_y;
	static constexpr ulongest hl = high_x * low_y;
	static constexpr ulongest hh = high_x * high_y;
	static constexpr ulongest llh = get_const_high_part_v<ulongest, ll>;
	static constexpr ulongest lh2 = lh + llh; // will not overflow
	static constexpr ulongest mid = lh2 + hl; // might overflow
	static constexpr ulongest carry = (mid < lh2) ? 1 : 0;
	static constexpr ulongest hh2 = hh + make_const_high_part_v<ulongest, carry>;
	static constexpr ulongest mh = get_const_high_part_v<ulongest, mid>;
	static constexpr ulongest ml_shifted = make_const_high_part_v<ulongest, mid>;
	static constexpr ulongest lll = get_const_low_part_v<ulongest, ll>;

public:
	static constexpr ulongest high_part = hh2 + mh; // will not overflow
	static constexpr ulongest low_part = ml_shifted | lll;

	static constexpr ulongest value = low_part;
};


/// @ingroup ConstMath
/// @brief Meta template to compute the extended multiplication of two constant integers
/// @tparam high_x High part of first value to multiply
/// @tparam low_x Low part of first value to multiply
/// @tparam high_y High part of second value to multiply
/// @tparam low_y Low part of second value to multiply
template <ulongest high_x, ulongest low_x, ulongest high_y, ulongest low_y>
class const_extumul2
{
private:
	static constexpr ulongest llh = const_extumul<low_x, low_y>::high_part;
	static constexpr ulongest lll = const_extumul<low_x, low_y>::low_part;

	static constexpr ulongest lhh = const_extumul<low_x, high_y>::high_part;
	static constexpr ulongest lhl = const_extumul<low_x, high_y>::low_part;

	static constexpr ulongest hlh = const_extumul<high_x, low_y>::high_part;
	static constexpr ulongest hll = const_extumul<high_x, low_y>::low_part;

	static constexpr ulongest hhh = const_extumul<high_x, high_y>::high_part;
	static constexpr ulongest hhl = const_extumul<high_x, high_y>::low_part;

	static constexpr ulongest lh1 = llh + lhl;
	static constexpr ulongest carry_low1 = (lh1 < llh) ? 1 : 0;
	static constexpr ulongest lh2 = lh1 + hll;
	static constexpr ulongest carry_low2 = carry_low1 + ((lh2 < lh1) ? 1 : 0);

	static constexpr ulongest hl1 = lhh + carry_low2;
	static constexpr ulongest carry_high1 = (hl1 < lhh) ? 1 : 0;
	static constexpr ulongest hl2 = hl1 + hlh;
	static constexpr ulongest carry_high2 = carry_high1 + ((hl2 < hl1) ? 1 : 0);
	static constexpr ulongest hl3 = hl2 + hhl;
	static constexpr ulongest carry_high3 = carry_high2 + ((hl3 < hl2) ? 1 : 0);

	static constexpr ulongest hh1 = hhh + carry_high3;

public:
	static constexpr ulongest high_high_part = hh1;
	static constexpr ulongest low_high_part = hl3;
	static constexpr ulongest high_low_part = lh2;
	static constexpr ulongest low_low_part = lll;

	static constexpr ulongest high_part = high_low_part;
	static constexpr ulongest low_part = low_low_part;

	static constexpr ulongest value = low_part;
};


}


#endif
