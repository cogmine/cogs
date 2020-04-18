//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_PREV_EXPONENT_OF_TWO
#define COGS_HEADER_MATH_PREV_EXPONENT_OF_TWO


#include <stdio.h>


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the previous (or current) exponent of two
/// @tparam n Constant value to get the previous (or current) exponent of 2 from.
template <size_t n>
class prev_or_current_exponent_of_two
{
private:
	template <size_t n2, bool unused = true>
	class helper
	{
	public:
		static constexpr size_t value = prev_or_current_exponent_of_two<(n2 >> 1)>::value << 1;
	};

	template <bool unused>
	class helper<1, unused>
	{
	public:
		static constexpr size_t value = 1;
	};

	template <bool unused>
	class helper<0, unused>
	{
	public:
		static constexpr size_t value = 0;
	};

public:
	static constexpr size_t value = helper<n>::value;
};

template <size_t n>
constexpr size_t prev_or_current_exponent_of_two_v = prev_or_current_exponent_of_two<n>::value;


/// @ingroup ConstMath
/// @brief Meta template to compute the previous multiple of two
/// @tparam n Constant value to get the previous exponent of 2 from.
template <size_t n>
class prev_exponent_of_two
{
public:
	static constexpr size_t value = prev_or_current_exponent_of_two<n-1>::value;
};


template <>
class prev_exponent_of_two<0>
{
public:
	static constexpr size_t value = 0;
};


template <>
class prev_exponent_of_two<1>
{
public:
	static constexpr size_t value = 0;
};


template <size_t n>
constexpr size_t prev_exponent_of_two_v = prev_exponent_of_two<n>::value;


}


#endif
