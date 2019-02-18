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

namespace cogs {
namespace env {

	
template <size_t num_bytes>	class bytes_to_uint { public: typedef void type; };
template <size_t num_bytes>	class bytes_to_int { public: typedef void type; };
template <size_t num_bytes>	class bytes_to_float		{ public: typedef void type; };

template <>	class bytes_to_uint< sizeof(uint8_t) >		{ public: typedef uint8_t type; };
template <>	class bytes_to_int<   sizeof( int8_t) >		{ public: typedef  int8_t type; };

template <>	class bytes_to_uint< sizeof(uint16_t) >		{ public: typedef uint16_t type; };
template <>	class bytes_to_int<   sizeof( int16_t) >		{ public: typedef  int16_t type; };

template <>	class bytes_to_uint< sizeof(uint32_t) >		{ public: typedef uint32_t type; };
template <>	class bytes_to_int<   sizeof( int32_t) >		{ public: typedef  int32_t type; };

template <>	class bytes_to_uint< sizeof(uint64_t) >		{ public: typedef uint64_t type; };
template <>	class bytes_to_int<   sizeof( int64_t) >		{ public: typedef  int64_t type; };

#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__)
#define COGS_LONGEST_INT (16)
template <>	class bytes_to_uint<sizeof(__uint128_t)>	{ public: typedef __uint128_t type; };
template <>	class bytes_to_int<  sizeof(__int128_t)>	{ public: typedef __int128_t type; };
#else
#define COGS_LONGEST_INT (8)
#endif

//typedef bytes_to_int<COGS_LONGEST_INT>::int_t   slongest;
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

#include <type_traits>

// TEMP until these are added to gcc.  These are standard.
namespace std
{

template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;
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
template <typename T> inline constexpr bool is_trivial_v = is_trivial<T>::value;
template <typename T> inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
template <typename T> inline constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;

template <class T, class U> inline constexpr bool is_same_v = is_same<T, U>::value;
template <class T, class U> inline constexpr bool is_convertible_v = is_convertible<T, U>::value;


};



#endif
