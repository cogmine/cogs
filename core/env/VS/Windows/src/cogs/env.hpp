//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENV_VS_WINDOWS
#define COGS_ENV_VS_WINDOWS


#include <intrin.h>
#include <new>
#include <typeinfo>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>

#include "cogs/os.hpp"
#include "cogs/assert.hpp"


namespace cogs {
namespace env {

template <size_t num_bytes>	class bytes_to_uint				{ public: typedef void type; };
template <size_t num_bytes>	class bytes_to_int				{ public: typedef void type; };
template <size_t num_bytes>	class bytes_to_float			{ public: typedef void type; };

template <>	class bytes_to_uint<sizeof(unsigned char)>		{ public: typedef unsigned char type; };
template <>	class bytes_to_int<  sizeof(         char)>		{ public: typedef          char type; };

template <>	class bytes_to_uint<sizeof(unsigned short)>		{ public: typedef unsigned short type; };
template <>	class bytes_to_int<  sizeof(  signed short)>	{ public: typedef   signed short type; };

template <>	class bytes_to_uint<sizeof(unsigned long)>		{ public: typedef unsigned long type; };
template <>	class bytes_to_int<  sizeof(  signed long)>		{ public: typedef   signed long type; };

template <>	class bytes_to_uint<sizeof(unsigned __int64)>	{ public: typedef unsigned __int64 type; };
template <>	class bytes_to_int<  sizeof(  signed __int64)>	{ public: typedef   signed __int64 type; };

#define COGS_LONGEST_INT (8)

typedef bytes_to_int<COGS_LONGEST_INT>::type    longest;
typedef bytes_to_uint<COGS_LONGEST_INT>::type ulongest;

template <>	class bytes_to_float<sizeof(float)>
{
public:
	typedef float	type;
	
	static constexpr size_t exponent_bits = 8;
	static constexpr size_t mantisaa_bits = 23;
};

template <>	class bytes_to_float<sizeof(double)>
{
public:
	typedef double	type;

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

