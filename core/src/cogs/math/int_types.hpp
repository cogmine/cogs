//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_INT_TYPES
#define COGS_HEADER_MATH_INT_TYPES


#include <cstdint>

#include "cogs/env.hpp"


namespace cogs {


///// @ingroup Math
///// @brief Signed 8-bit int
//typedef env::bytes_to_int<1>::type   sint8_t;
///// @ingroup Math
///// @brief Signed 8-bit int
//typedef env::bytes_to_int<1>::type    int8_t;
///// @ingroup Math
///// @brief Unsigned 8-bit int
//typedef env::bytes_to_uint<1>::type uint8_t;
//
///// @ingroup Math
///// @brief Signed 8-bit int
//typedef uint8_t  byte_t;
///// @ingroup Math
///// @brief Signed 8-bit int
//typedef sint8_t sbyte_t;
///// @ingroup Math
///// @brief Unsigned 8-bit int
//typedef uint8_t ubyte_t;
//
///// @ingroup Math
///// @brief Signed 16-bit int
//typedef env::bytes_to_int<2>::type   sint16_t;
///// @ingroup Math
///// @brief Signed 16-bit int
//typedef env::bytes_to_int<2>::type    int16_t;
///// @ingroup Math
///// @brief Unsigned 16-bit int
//typedef env::bytes_to_uint<2>::type uint16_t;
//
///// @ingroup Math
///// @brief Signed 32-bit int
//typedef env::bytes_to_int<4>::type   sint32_t;
///// @ingroup Math
///// @brief Signed 32-bit int
//typedef env::bytes_to_int<4>::type    int32_t;
///// @ingroup Math
///// @brief Unsigned 32-bit int
//typedef env::bytes_to_uint<4>::type uint32_t;
//
//#if COGS_LONGEST_INT >= 8
///// @ingroup Math
///// @brief Signed 64-bit int
//typedef env::bytes_to_int<8>::type   sint64_t;
///// @ingroup Math
///// @brief Signed 64-bit int
//typedef env::bytes_to_int<8>::type    int64_t;
///// @ingroup Math
///// @brief Unsigned 64-bit int
//typedef env::bytes_to_uint<8>::type uint64_t;
//#endif
//
//#if COGS_LONGEST_INT >= 16
///// @ingroup Math
///// @brief Signed 128-bit int
//typedef env::bytes_to_int<16>::type   sint128_t;
///// @ingroup Math
///// @brief Signed 128-bit int
//typedef env::bytes_to_int<16>::type    int128_t;
///// @ingroup Math
///// @brief Unsigned 128-bit int
//typedef env::bytes_to_uint<16>::type uint128_t;
//#endif

///// @ingroup Math
///// @brief The largest native signed integer type available
//typedef env::bytes_to_int<COGS_LONGEST_INT>::type   slongest;
/// @ingroup Math
/// @brief The largest native signed integer type available
typedef env::bytes_to_int<COGS_LONGEST_INT>::type    longest;
/// @ingroup Math
/// @brief The largest native unsigned integer type available
typedef env::bytes_to_uint<COGS_LONGEST_INT>::type ulongest;

/// @ingroup Math
/// @brief The number of bits in the largested native integer type available
static constexpr size_t longest_bits = sizeof(longest) * 8;



}


#endif

