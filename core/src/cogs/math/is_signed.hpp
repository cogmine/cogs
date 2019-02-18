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
/// @tparam int_t Int type
template <typename T>
class is_signed
{
public:
	static constexpr bool value = std::is_signed<T>::value;
};
template <typename T>
inline constexpr bool is_signed_v = is_signed<T>::value;



// By default, map const and/or volatile to the version with no CV qualifier
template <typename T>
class is_signed<const T>
{
public:
	static constexpr bool value = is_signed_v<T>;
};

template <typename T>
class is_signed<volatile T>
{
public:
	static constexpr bool value = is_signed_v<T>;
};

template <typename T>
class is_signed<const volatile T>
{
public:
	static constexpr bool value = is_signed_v<T>;
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
