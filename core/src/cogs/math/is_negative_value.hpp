//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_NEGATIVE_VALUE
#define COGS_HEADER_MATH_IS_NEGATIVE_VALUE


#include "cogs/math/is_signed_type.hpp"

namespace cogs {


/// @ingroup Math
/// @brief Template helper to test if a value is negative.  Basically just avoids a compiler warning if an unsigned type.
/// @tparam T Integral or fixed_integer type
template <typename T>
class is_negative_value
{
private:
	class unsigned_getter
	{
	public:
		template <typename T2> static bool get(const T2&) { return false; }
	};

	class signed_getter
	{
	public:
		template <typename T2> static bool get(const T2& t)
		{
			T2 t2(0);
			return t < t2;
		}
	};

public:
	static bool get(const T& t) { return std::conditional_t<is_signed_type_v<T>, signed_getter, unsigned_getter>::get(t); }
};


}


#endif
