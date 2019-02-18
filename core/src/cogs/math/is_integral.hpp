//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_INTEGRAL
#define COGS_HEADER_MATH_IS_INTEGRAL


#include <type_traits>

#include "cogs/env.hpp"


namespace cogs {


template <typename T>
class is_integral
{
public:
	static constexpr bool value = std::is_integral_v<T>;
};
template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;



// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_integral<const T>
{
public:
	static constexpr bool value = is_integral_v<T>;
};

template <typename T>
class is_integral<volatile T>
{
public:
	static constexpr bool value = is_integral_v<T>;
};

template <typename T>
class is_integral<const volatile T>
{
public:
	static constexpr bool value = is_integral_v<T>;
};


}


#endif
