//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_IS_ARITHMETIC
#define COGS_IS_ARITHMETIC


#include <type_traits>

#include "cogs/env.hpp"


namespace cogs {


template <typename T>
class is_arithmetic
{
public:
	static constexpr bool value = std::is_arithmetic;
};
template <typename T>
static constexpr bool is_arithmetic_v = is_arithmetic<T>::value;


// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_arithmetic<const T>
{
public:
	static constexpr bool value = is_arithmetic<T>::value;
};

template <typename T>
class is_arithmetic<volatile T>
{
public:
	static constexpr bool value = is_arithmetic<T>::value;
};

template <typename T>
class is_arithmetic<const volatile T>
{
public:
	static constexpr bool value = is_arithmetic<T>::value;
};




}


#endif
