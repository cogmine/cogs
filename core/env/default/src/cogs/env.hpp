//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV


#include <stdio.h>
#include <new>
#include <typeinfo>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>


#include "cogs/os.hpp"
#include "cogs/math/bytes_to_int.hpp"


namespace cogs {


template <> class bytes_to_int<sizeof(unsigned char), false> { public: typedef unsigned char type; };
template <> class bytes_to_int<sizeof(signed char), true> { public: typedef signed char type; };

template <> class bytes_to_int<sizeof(unsigned short), false> { public: typedef unsigned short type; };
template <> class bytes_to_int<sizeof(signed short), true> { public: typedef signed short type; };

template <> class bytes_to_int<sizeof(unsigned long), false> { public: typedef unsigned long type; };
template <> class bytes_to_int<sizeof(signed long), true> { public: typedef signed long type; };

#define COGS_LONGEST_INT (4)

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


}


#endif
