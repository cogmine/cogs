//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_ARITHMETIC_TYPE
#define COGS_HEADER_MATH_IS_ARITHMETIC_TYPE


#include <type_traits>

#include "cogs/env.hpp"


namespace cogs {


template <typename T> struct is_arithmetic_type : public std::is_arithmetic<T> { };
template <typename T> static constexpr bool is_arithmetic_type_v = is_arithmetic_type<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T> struct is_arithmetic_type<const T> : public is_arithmetic_type<T> { };
template <typename T> struct is_arithmetic_type<volatile T> : public is_arithmetic_type<T> { };
template <typename T> struct is_arithmetic_type<const volatile T> : public is_arithmetic_type<T> { };


}


#endif
