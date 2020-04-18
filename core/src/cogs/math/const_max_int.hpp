//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_MAX_INT
#define COGS_HEADER_MATH_CONST_MAX_INT


#include "cogs/env.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Template helper to calculate the maximum constant value of an integer type
/// @tparam int_t Int type
template <typename int_t>
class const_max_int
{
private:
	static constexpr bool has_sign = ((int_t)~(int_t)0) < (int_t)0;

	template <size_t prev, size_t n>
	class shift_and_inc
	{
	public:
		static constexpr int_t newPrev = ((prev << 1) | 1);
		static constexpr int_t value = shift_and_inc<newPrev, n - 1>::value;
	};

	template <size_t prev>
	class shift_and_inc<prev, 0>
	{
	public:
		static constexpr int_t value = prev;
	};

	static constexpr int_t positive_signed_bit_count = (sizeof(int_t)*8)-1;

public:
	static constexpr int_t value = has_sign ? shift_and_inc<0, positive_signed_bit_count>::value : (int_t)~(int_t)0;
};

template <typename int_t> inline constexpr int_t const_max_int_v = const_max_int<int_t>::value;


}


#endif
