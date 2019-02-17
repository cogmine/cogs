//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV


#include <new>
#include <typeinfo>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>
#include <malloc/malloc.h>
#include <unistd.h>

#include "cogs/os.hpp"


#define COGS_THREAD_LOCAL(type) __thread type  

inline int printf(const char * __restrict format, ...)
{
	va_list args;
	va_start(args, format);
	NSLogv([NSString stringWithUTF8String : format], args);
	va_end(args);
	return 1;
}

namespace cogs {
namespace env {


template <size_t num_bytes>	class bytes_to_uint { public: typedef void int_t; };
template <size_t num_bytes>	class bytes_to_int { public: typedef void int_t; };
template <size_t num_bytes>	class bytes_to_float		{ public: typedef void float_t; };

template <>	class bytes_to_uint< sizeof(__uint8_t) >	{ public: typedef __uint8_t int_t; };
template <>	class bytes_to_int<  sizeof( __int8_t) >	{ public: typedef  __int8_t int_t; };

template <>	class bytes_to_uint< sizeof(__uint16_t) >	{ public: typedef __uint16_t int_t; };
template <>	class bytes_to_int<  sizeof( __int16_t) >	{ public: typedef  __int16_t int_t; };

template <>	class bytes_to_uint< sizeof(__uint32_t) >	{ public: typedef __uint32_t int_t; };
template <>	class bytes_to_int<  sizeof( __int32_t) >	{ public: typedef  __int32_t int_t; };

template <>	class bytes_to_uint< sizeof(__uint64_t) >	{ public: typedef __uint64_t int_t; };
template <>	class bytes_to_int<  sizeof( __int64_t) >	{ public: typedef  __int64_t int_t; };

#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__)
#define COGS_LONGEST_INT (16)
template <>	class bytes_to_uint<sizeof(__uint128_t)>	{ public: typedef __uint128_t int_t; };
template <>	class bytes_to_int<  sizeof( __int128_t)>	{ public: typedef  __int128_t int_t; };
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
	
	static constexpr size_t exponent_bits = 8;
	static constexpr size_t mantisaa_bits = 23;
};

template <>	class bytes_to_float<sizeof(double)>
{
public:
	typedef double	float_t;

	static constexpr size_t exponent_bits = 11;
	static constexpr size_t mantisaa_bits = 52;
};

// Assumes IEEE 754 double precision format
#define COGS_LONGEST_FLOAT	(8)
#define COGS_LONGEST_FLOAT_EXPONENT	(11)
#define COGS_LONGEST_FLOAT_MANTISSA	(52)


template <size_t _exponent_bits, size_t _mantissa_bits>
class detect_native_float_type
{
public:
	static constexpr bool is_valid = (_exponent_bits <= COGS_LONGEST_FLOAT_EXPONENT) && (_mantissa_bits <= COGS_LONGEST_FLOAT_MANTISSA);

	static constexpr size_t next_exponent_bits = (!is_valid) ? 0 : (_exponent_bits + 1);
	static constexpr size_t next_mantissa_bits = (!is_valid) ? 0 : (_mantissa_bits + 1);

	typedef typename detect_native_float_type<next_exponent_bits, next_mantissa_bits>::type type;
};

template <>
class detect_native_float_type<0, 0>
{
	static constexpr bool is_valid = false;
	typedef void type;
};

template <>
class detect_native_float_type<8, 23>
{
	static constexpr bool is_valid = true;
	typedef float type;
};

template <>
class detect_native_float_type<11, 52>
{
	static constexpr bool is_valid = true;
	typedef double type;
};


}
}



#endif

