//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV

#ifdef __CYGWIN__
#ifdef __x86_64__
// 10/21/2022 - Current Mingw headers define sockaddr_in6 using u_long (8 byte)
// because a redefinition for LP64 systems is missing.  This leads to a wrong
// definition and size of sockaddr_in6 when building with winsock headers.
#undef u_long
#define u_long __ms_u_long
#endif
#endif

#include "cogs/os.hpp"

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
#include <type_traits>

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


#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__SIZEOF_INT128__)
template <> class bytes_to_int<sizeof(unsigned __int128), false> { public: typedef unsigned __int128 type; };
template <> class bytes_to_int<sizeof(__int128), true> { public: typedef __int128 type; };
#define COGS_LONGEST_INT (16)
#else
#define COGS_LONGEST_INT (8)
#endif


typedef bytes_to_int_t<COGS_LONGEST_INT> longest;
typedef bytes_to_uint_t<COGS_LONGEST_INT> ulongest;

static constexpr size_t longest_bits = sizeof(longest) * 8;


// TEMPORARY
// Because:
//	* clang supports 128-bit ints
//	* clang on Windows uses Microsoft's STL
//	* Microsoft's STL does not support 128 bits
//	... It's necessary to provide our own versions of these:

template <typename int_t> struct is_integral : public std::is_integral<int_t> { };
template <typename int_t> constexpr bool is_integral_v = is_integral<int_t>::value;

template <typename int_t> struct is_arithmetic : public std::is_arithmetic<int_t> { };
template <typename int_t> constexpr bool is_arithmetic_v = is_arithmetic<int_t>::value;

template <typename int_t> struct is_scalar : public std::is_scalar<int_t> { };
template <typename int_t> constexpr bool is_scalar_v = is_scalar<int_t>::value;

template <typename int_t> struct is_signed : public std::is_signed<int_t> { };
template <typename int_t> constexpr bool is_signed_v = is_signed<int_t>::value;

template <typename int_t> struct is_unsigned : public std::is_unsigned<int_t> { };
template <typename int_t> constexpr bool is_unsigned_v = is_unsigned<int_t>::value;

template <typename int_t> struct make_unsigned : public std::make_unsigned<int_t> { };
template <typename int_t> using make_unsigned_t = typename make_unsigned<int_t>::type;

template <typename int_t> struct make_signed : public std::make_signed<int_t> { };
template <typename int_t> using make_signed_t = typename make_signed<int_t>::type;

#if COGS_LONGEST_INT >= 16

template <> struct is_integral<unsigned __int128> { static constexpr bool value = true; };
template <> struct is_integral<__int128> { static constexpr bool value = true; };

template <> struct is_arithmetic<unsigned __int128> { static constexpr bool value = true; };
template <> struct is_arithmetic<__int128> { static constexpr bool value = true; };

template <> struct is_scalar<unsigned __int128> { static constexpr bool value = true; };
template <> struct is_scalar<__int128> { static constexpr bool value = true; };

template <> struct is_signed<unsigned __int128> { static constexpr bool value = false; };
template <> struct is_signed<__int128> { static constexpr bool value = true; };

template <> struct is_unsigned<unsigned __int128> { static constexpr bool value = true; };
template <> struct is_unsigned<__int128> { static constexpr bool value = false; };

template <> struct make_unsigned<unsigned __int128> { typedef unsigned __int128 type; };
template <> struct make_unsigned<__int128> { typedef unsigned __int128 type; };

template <> struct make_signed<__int128> { typedef __int128 type; };
template <> struct make_signed<unsigned __int128> { typedef __int128 type; };

#endif


}


#endif
