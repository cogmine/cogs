//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CONST_MIN_INT
#define COGS_CONST_MIN_INT


namespace cogs {


/// @ingroup ConstMath
/// @brief Template helper to calculate the minimum constant value of an integer type
/// @tparam int_t Int type
template <typename int_t>
class const_min_int
{
private:
	static const bool has_sign = ((int_t)~(int_t)0) < (int_t)0;
	static const int_t num_positive_signed_bits = (sizeof(int_t) * 8) - 1;

public:
	static const int_t value = has_sign ? ((int_t)1 << ((sizeof(int_t)*8)-1)) : (int_t)0;
};


}


#endif
