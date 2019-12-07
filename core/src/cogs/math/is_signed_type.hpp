//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
template <typename T>
class is_signed_type
{
public:
	static constexpr bool value = std::is_signed<T>::value;
};
template <typename T>
inline constexpr bool is_signed_type_v = is_signed_type<T>::value;



// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_signed_type<const T>
{
public:
	static constexpr bool value = is_signed_type_v<T>;
};

template <typename T>
class is_signed_type<volatile T>
{
public:
	static constexpr bool value = is_signed_type_v<T>;
};

template <typename T>
class is_signed_type<const volatile T>
{
public:
	static constexpr bool value = is_signed_type_v<T>;
};



template <typename T>
class is_unsigned
{
public:
	static constexpr bool value = std::is_unsigned_v<T>;
};
template <typename T>
inline constexpr bool is_unsigned_v = is_unsigned<T>::value;



// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_unsigned<const T>
{
public:
	static constexpr bool value = is_unsigned_v<T>;
};

template <typename T>
class is_unsigned<volatile T>
{
public:
	static constexpr bool value = is_unsigned_v<T>;
};

template <typename T>
class is_unsigned<const volatile T>
{
public:
	static constexpr bool value = is_unsigned_v<T>;
};




}


#endif
