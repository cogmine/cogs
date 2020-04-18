//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_INTEGRAL_TYPE
#define COGS_HEADER_MATH_IS_INTEGRAL_TYPE


#include <type_traits>

#include "cogs/math/is_const_type.hpp"


namespace cogs {


template <typename T> struct is_integer_type : public std::is_integral<T> { };
template <typename T> constexpr bool is_integer_type_v = is_integer_type<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T> struct is_integer_type<const T> : public is_const_type<T> { };
template <typename T> struct is_integer_type<volatile T> : public is_const_type<T> { };
template <typename T> struct is_integer_type<const volatile T> : public is_const_type<T> { };


}


#endif
