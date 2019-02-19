//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV

#include <stdint.h>
#include <new>
#include <typeinfo>
#include <stdio.h>
#include <wchar.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>

#include "cogs/os.hpp"
#include "cogs/math/bytes_to_int.hpp"

namespace cogs {


template <> class bytes_to_int<sizeof(uint8_t), false> { public: typedef uint8_t type; };
template <> class bytes_to_int<sizeof(int8_t), true> { public: typedef int8_t type; };

template <> class bytes_to_int<sizeof(uint16_t), false> { public: typedef uint16_t type; };
template <> class bytes_to_int<sizeof(int16_t), true> { public: typedef int16_t type; };

template <> class bytes_to_int<sizeof(uint32_t), false> { public: typedef uint32_t type; };
template <> class bytes_to_int<sizeof(int32_t), true> { public: typedef int32_t type; };

template <> class bytes_to_int<sizeof(uint64_t), false> { public: typedef uint64_t type; };
template <> class bytes_to_int<sizeof(int64_t), true> { public: typedef int64_t type; };

#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__)
#define COGS_LONGEST_INT (16)
template <> class bytes_to_int<sizeof(__uint128_t), false> { public: typedef __uint128_t type; };
template <> class bytes_to_int<sizeof(__int128_t), true> { public: typedef __int128_t type; };
#else
#define COGS_LONGEST_INT (8)
#endif


typedef bytes_to_int_t<COGS_LONGEST_INT> longest;
typedef bytes_to_uint_t<COGS_LONGEST_INT> ulongest;

static constexpr size_t longest_bits = sizeof(longest) * 8;


}


#include <type_traits>

// TEMP until these are added to gcc.  These are standard.
namespace std
{

template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;
template <typename T> using make_signed_t = typename make_signed<T>::type;
template <typename T> using remove_reference_t = typename remove_reference<T>::type;
template <typename T> using remove_pointer_t = typename remove_pointer<T>::type;
template <typename T> using remove_extent_t = typename remove_extent<T>::type;
template <typename T> using remove_volatile_t = typename remove_volatile<T>::type;
template <typename T> using remove_const_t = typename remove_const<T>::type;
template <typename T> using remove_cv_t = typename remove_cv<T>::type;

template <bool B, typename T, typename F> using conditional_t = typename conditional<B,T,F>::type;
template <bool B, typename T = void> using enable_if_t = typename enable_if<B,T>::type;

template <typename T> inline constexpr bool is_scalar_v = is_scalar<T>::value;
template <typename T> inline constexpr bool is_integral_v = is_integral<T>::value;
template <typename T> inline constexpr bool is_volatile_v = is_volatile<T>::value;
template <typename T> inline constexpr bool is_const_v = is_const<T>::value;
template <typename T> inline constexpr bool is_void_v = is_void<T>::value;
template <typename T> inline constexpr bool is_empty_v = is_empty<T>::value;
template <typename T> inline constexpr bool is_signed_v = is_signed<T>::value;
template <typename T> inline constexpr bool is_unsigned_v = is_unsigned<T>::value;
template <typename T> inline constexpr bool is_reference_v = is_reference<T>::value;
template <typename T> inline constexpr bool is_pointer_v = is_pointer<T>::value;
template <typename T> inline constexpr bool is_class_v = is_class<T>::value;
template <typename T> inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
template <typename T> inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

template <class T, class... Args> inline constexpr bool is_constructible_v = is_constructible<T, Args...>::value;

template <typename T> inline constexpr bool is_trivial_v = is_trivial<T>::value;
template <typename T> inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
template <typename T> inline constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;

template <class T, class U> inline constexpr bool is_same_v = is_same<T, U>::value;
template <class T, class U> inline constexpr bool is_convertible_v = is_convertible<T, U>::value;
template <class T, class U> inline constexpr bool is_assignable_v = is_assignable<T, U>::value;

};



#endif
