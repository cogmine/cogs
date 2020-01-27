//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_MIN_INT
#define COGS_HEADER_MATH_CONST_MIN_INT


namespace cogs {


/// @ingroup ConstMath
/// @brief Template helper to calculate the minimum constant value of an integer type
/// @tparam int_t Int type
template <typename int_t>
class const_min_int
{
private:
	static constexpr bool has_sign = ((int_t)~(int_t)0) < (int_t)0;

public:
	static constexpr int_t value = has_sign ? ((int_t)1 << ((sizeof(int_t)*8)-1)) : (int_t)0;
};
template <typename int_t> inline constexpr int_t const_min_int_v = const_min_int<int_t>::value;


}


#endif
