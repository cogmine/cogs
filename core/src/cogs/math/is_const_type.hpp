//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_CONST_TYPE
#define COGS_HEADER_MATH_IS_CONST_TYPE

#include <type_traits>


namespace cogs {


/// @ingroup TypeTraits
/// @ingroup Math
/// @brief Template helpers to test if an integer type is const.  i.e. fixed_integer_native_const or fixed_integer_extended_const
/// @tparam int_t Int type
template <typename T>
class is_const_type
{
public:
	static constexpr bool value = false;
};
template <typename T>
static constexpr bool is_const_type_v = is_const_type<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_const_type<const T>
{
public:
	static constexpr bool value = is_const_type<T>::value;
};

template <typename T>
class is_const_type<volatile T>
{
public:
	static constexpr bool value = is_const_type<T>::value;
};

template <typename T>
class is_const_type<const volatile T>
{
public:
	static constexpr bool value = is_const_type<T>::value;
};


}


#endif
