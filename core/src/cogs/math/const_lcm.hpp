//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CONST_LCM
#define COGS_CONST_LCM

#include <type_traits>

#include "cogs/math/const_gcd.hpp"
#include "cogs/math/extumul.hpp"


namespace cogs {



/// @ingroup ConstMath
/// @brief Meta template to compute the least common multiple of two constant integers
/// @tparam x Constant value
/// @tparam y Constant value
template <ulongest x, ulongest y>
class const_lcm
{
private:
	static const ulongest multiplied_high_part = const_extumul<x, y>::high_part;
	static const ulongest multiplied_low_part =	const_extumul<x, y>::low_part;
	static const ulongest gcd = const_gcd<x, y>::value;

public:
	static const ulongest value = const_extudiv<multiplied_high_part, multiplied_low_part, gcd>::low_part;
};

template <ulongest y> class const_lcm<0, y>	{ public: static const ulongest value = 0; };
template <ulongest x> class const_lcm<x, 0>	{ public: static const ulongest value = 0; };


}


#endif
