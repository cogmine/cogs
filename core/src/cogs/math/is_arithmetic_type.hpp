//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_ARITHMETIC_TYPE
#define COGS_HEADER_MATH_IS_ARITHMETIC_TYPE


#include <type_traits>

#include "cogs/env.hpp"


namespace cogs {


template <typename T>
class is_arithmetic_type
{
public:
	static constexpr bool value = std::is_arithmetic_v<T>;
};
template <typename T>
static constexpr bool is_arithmetic_type_v = is_arithmetic_type<T>::value;


// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_arithmetic_type<const T>
{
public:
	static constexpr bool value = is_arithmetic_type_v<T>;
};

template <typename T>
class is_arithmetic_type<volatile T>
{
public:
	static constexpr bool value = is_arithmetic_type_v<T>;
};

template <typename T>
class is_arithmetic_type<const volatile T>
{
public:
	static constexpr bool value = is_arithmetic_type_v<T>;
};




}


#endif
