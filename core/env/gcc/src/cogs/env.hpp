//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENV_GCC
#define COGS_ENV_GCC

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

namespace cogs {
namespace env {

	
template <size_t num_bytes>	class bytes_to_uint { public: typedef void int_t; };
template <size_t num_bytes>	class bytes_to_int { public: typedef void int_t; };
template <size_t num_bytes>	class bytes_to_float		{ public: typedef void float_t; };

template <>	class bytes_to_uint< sizeof(uint8_t) >		{ public: typedef uint8_t int_t; };
template <>	class bytes_to_int<   sizeof( int8_t) >		{ public: typedef  int8_t int_t; };

template <>	class bytes_to_uint< sizeof(uint16_t) >		{ public: typedef uint16_t int_t; };
template <>	class bytes_to_int<   sizeof( int16_t) >		{ public: typedef  int16_t int_t; };

template <>	class bytes_to_uint< sizeof(uint32_t) >		{ public: typedef uint32_t int_t; };
template <>	class bytes_to_int<   sizeof( int32_t) >		{ public: typedef  int32_t int_t; };

template <>	class bytes_to_uint< sizeof(uint64_t) >		{ public: typedef uint64_t int_t; };
template <>	class bytes_to_int<   sizeof( int64_t) >		{ public: typedef  int64_t int_t; };

#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__)
#define COGS_LONGEST_INT (16)
template <>	class bytes_to_uint<sizeof(__uint128)>	{ public: typedef unsigned __int128 int_t; };
template <>	class bytes_to_int<  sizeof( __int128)>	{ public: typedef  __int128 int_t; };
#else
#define COGS_LONGEST_INT (8)
#endif

//typedef bytes_to_int<COGS_LONGEST_INT>::int_t   slongest;
typedef bytes_to_int<COGS_LONGEST_INT>::int_t    longest;
typedef bytes_to_uint<COGS_LONGEST_INT>::int_t ulongest;

template <>	class bytes_to_float<sizeof(float)>
{
public:
	typedef float	float_t;
	
	static const size_t exponent_bits = 8;
	static const size_t mantisaa_bits = 23;
};

template <>	class bytes_to_float<sizeof(double)>
{
public:
	typedef double	float_t;

	static const size_t exponent_bits = 11;
	static const size_t mantisaa_bits = 52;
};

// Assumes IEEE 754 double precision format
#define COGS_LONGEST_FLOAT	(8)
#define COGS_LONGEST_FLOAT_EXPONENT	(11)
#define COGS_LONGEST_FLOAT_MANTISSA	(52)

template <size_t exponent_bits_in, size_t mantissa_bits_in>
class detect_native_float_type
{
public:
	static const bool is_valid = (exponent_bits_in <= COGS_LONGEST_FLOAT_EXPONENT) && (mantissa_bits_in <= COGS_LONGEST_FLOAT_MANTISSA);

	static const size_t next_exponent_bits = (!is_valid) ? 0 : (exponent_bits_in + 1);
	static const size_t next_mantissa_bits = (!is_valid) ? 0 : (mantissa_bits_in + 1);

	typedef typename detect_native_float_type<next_exponent_bits, next_mantissa_bits>::type type;
};

template <>
class detect_native_float_type<0, 0>
{
	static const bool is_valid = false;
	typedef void type;
};

template <>
class detect_native_float_type<8, 23>
{
	static const bool is_valid = true;
	typedef float type;
};

template <>
class detect_native_float_type<11, 52>
{
	static const bool is_valid = true;
	typedef double type;
};


}
}



#endif
