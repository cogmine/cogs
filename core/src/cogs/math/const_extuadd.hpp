//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good


#ifndef COGS_CONST_EXTUADD
#define COGS_CONST_EXTUADD


#include "cogs/env.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {


template <ulongest x, ulongest y>
class const_extuadd
{
private:
	static constexpr bool is_overflowing = !!x && ((ulongest)-(longest)x) <= y;

	static constexpr ulongest x2 = is_overflowing ? 0 : x;
	static constexpr ulongest y2 = is_overflowing ? 0 : y;

	static constexpr ulongest x3 = is_overflowing ? ((ulongest)-(longest)x) : 0;
	static constexpr ulongest y3 = is_overflowing ? y : 0;

public:
	static constexpr ulongest high_part = is_overflowing ? 1 : 0;
	static constexpr ulongest low_part = is_overflowing ? (y3 - x3) : (x2 + y2);

	static constexpr ulongest value = low_part;
};

template <ulongest high_x, ulongest low_x, ulongest high_y, ulongest low_y>
class const_extuadd2
{
private:
	static constexpr ulongest add1_high_part = const_extuadd<low_x, low_y>::high_part;	// low
	static constexpr ulongest add1_low_part = const_extuadd<low_x, low_y>::low_part;

	static constexpr ulongest add2_high_part = const_extuadd<high_x, high_y>::high_part;	// high
	static constexpr ulongest add2_low_part = const_extuadd<high_x, high_y>::low_part;

	// add overflow from low to high
	static constexpr ulongest add3_high_part = const_extuadd<add1_high_part, add2_low_part>::high_part;
	static constexpr ulongest add3_low_part = const_extuadd<add1_high_part, add2_low_part>::low_part;

	// add overfrom from that to overflow of high.  Max 2, so won't overflow
	static constexpr ulongest add4_low_part = const_extuadd<add3_high_part, add2_high_part>::low_part;

public:
	static constexpr ulongest low_high_part = add4_low_part;
	static constexpr ulongest high_low_part = add3_low_part;
	static constexpr ulongest low_low_part = add1_low_part;

	static constexpr ulongest high_part = high_low_part;
	static constexpr ulongest low_part = low_low_part;

	static constexpr ulongest value = low_part;
};


}


#endif
