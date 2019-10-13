//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_GCD
#define COGS_HEADER_MATH_CONST_GCD

#include <type_traits>

#include "cogs/compatible.hpp"
#include "cogs/env.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the greatest common divisor of two constants
/// @tparam x Constant value
/// @tparam y Constant value
template <ulongest x, ulongest y>
class const_gcd
{
private:
	static constexpr ulongest greater = (x > y) ? x : y;
	static constexpr ulongest lesser = (x > y) ? y : x;
	static constexpr ulongest internal_gcd_remainder = (greater % lesser);
	static constexpr ulongest internal_gcd_greater = (!internal_gcd_remainder) ? 0 : lesser;
	static constexpr ulongest internal_gcd_lesser = (!internal_gcd_remainder) ? 0 : internal_gcd_remainder;

public:
	static constexpr ulongest value = (!internal_gcd_remainder) ? lesser : const_gcd<internal_gcd_greater, internal_gcd_lesser>::value;
};
template <ulongest x, ulongest y>
inline constexpr ulongest const_gcd_v = const_gcd<x, y>::value;


template <ulongest x> class const_gcd<x, 0> { public: static constexpr ulongest value = 0; };
template <ulongest y> class const_gcd<0, y> { public: static constexpr ulongest value = 0; };
template <> class const_gcd<0, 0> { public: static constexpr ulongest value = 0; };


}


#endif
