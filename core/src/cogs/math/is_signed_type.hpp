//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_SIGNED_TYPE
#define COGS_HEADER_MATH_IS_SIGNED_TYPE


#include <type_traits>


namespace cogs {


/// @ingroup TypeTraits
/// @ingroup Math
/// @brief Template helpers to test if an integer type is signed
/// @tparam T integeral or fixed_integer type
template <typename T> struct is_signed_type : public is_signed<T> { };
template <typename T> inline constexpr bool is_signed_type_v = is_signed_type<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T> struct is_signed_type<const T> : public is_signed_type<T> { };
template <typename T> struct is_signed_type<volatile T> : public is_signed_type<T> { };
template <typename T> struct is_signed_type<const volatile T> : public is_signed_type<T> { };


template <typename T> struct is_unsigned_type : public is_unsigned<T> { };
template <typename T> inline constexpr bool is_unsigned_type_v = is_unsigned_type<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T> struct is_unsigned_type<const T> : public is_unsigned_type<T> { };
template <typename T> struct is_unsigned_type<volatile T> : public is_unsigned_type<T> { };
template <typename T> struct is_unsigned_type<const volatile T> : public is_unsigned_type<T> { };


}


#endif
