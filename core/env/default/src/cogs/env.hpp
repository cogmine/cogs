//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENV_DEFAULT
#define COGS_ENV_DEFAULT


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


namespace cogs {

/// @brief Namespace for compiler environment specific functionality
namespace env {


template <size_t num_bytes>	class bytes_to_uint		{ };
template <size_t num_bytes>	class bytes_to_int		{ };
template <size_t num_bytes>	class bytes_to_float	{ };

template <>	class bytes_to_uint< sizeof(unsigned char) >		{ public: typedef unsigned char int_t; };
template <>	class bytes_to_uint< sizeof(  signed char) >		{ public: typedef   signed char int_t; };

template <>	class bytes_to_uint< sizeof(unsigned short) >		{ public: typedef unsigned short int_t; };
template <>	class bytes_to_uint< sizeof(  signed short) >		{ public: typedef   signed short int_t; };

template <>	class bytes_to_uint< sizeof(unsigned long) >		{ public: typedef unsigned long int_t; };
template <>	class bytes_to_uint< sizeof(  signed long) >		{ public: typedef   signed long int_t; };

#define COGS_LONGEST_INT (4)

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

template <size_t exponent_bits_in, size_t mantissa_bits_in>
class detect_native_float_type
{
public:
	static constexpr bool is_valid = (exponent_bits_in <= COGS_LONGEST_FLOAT_EXPONENT) && (mantissa_bits_in <= COGS_LONGEST_FLOAT_MANTISSA);

	static constexpr size_t next_exponent_bits = (!is_valid) ? 0 : (exponent_bits_in + 1);
	static constexpr size_t next_mantissa_bits = (!is_valid) ? 0 : (mantissa_bits_in + 1);

	typedef typename detect_native_float_type<next_exponent_bits, next_mantissa_bits>::type type;
};

template <>
class detect_native_float_type<0, 0>
{
	static constexpr bool is_valid = false;
	typedef typename detect_native_float_type<exponent_bits, mantissa_bits>::type type;
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

