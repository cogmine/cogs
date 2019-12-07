//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_IS_INTEGRAL_TYPE
#define COGS_HEADER_MATH_IS_INTEGRAL_TYPE


#include <type_traits>


namespace cogs {


template <typename T>
class is_integer_type
{
public:
	static constexpr bool value = std::is_integral_v<T>;
};
template <typename T>
constexpr bool is_integer_type_v = is_integer_type<T>::value;



// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_integer_type<const T>
{
public:
	static constexpr bool value = is_integer_type_v<T>;
};

template <typename T>
class is_integer_type<volatile T>
{
public:
	static constexpr bool value = is_integer_type_v<T>;
};

template <typename T>
class is_integer_type<const volatile T>
{
public:
	static constexpr bool value = is_integer_type_v<T>;
};


}


#endif
