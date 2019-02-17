//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_NEGATIVE_VALUE
#define COGS_HEADER_MATH_IS_NEGATIVE_VALUE


#include "cogs/math/is_signed.hpp"

namespace cogs {


/// @ingroup Math
/// @brief Template helper to test if a value is negative.  Basically just avoids a compiler warning if an unsigned type.
/// @tparam int_t Int type
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
	static bool get(const T& t)	{ return conditional<is_signed<T>::value, signed_getter, unsigned_getter>::type::get(t); }
};


}


#endif
