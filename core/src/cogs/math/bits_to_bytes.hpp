//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_BITS_TO_BYTES
#define COGS_HEADER_MATH_BITS_TO_BYTES


#include "cogs/env.hpp"
#include "cogs/math/next_exponent_of_two.hpp"


namespace cogs {


template <size_t bits>
class bits_to_bytes
{
public:
	static constexpr size_t value = (bits + 7) / 8;
};
template <size_t bits>
constexpr size_t bits_to_bytes_v = bits_to_bytes<bits>::value;

template <>
class bits_to_bytes<0>
{
public:
	static constexpr size_t value = 0;
};


template <size_t bits>
class bits_to_int_bytes
{
public:
	// rounded up to an exponent of 2.  I.e. 1 byte, 2 bytes, 4 bytes, 8 bytes.
	static constexpr size_t value = bits_to_bytes_v<next_or_current_exponent_of_two_v<bits> >;
};
template <size_t bits>
constexpr size_t bits_to_int_bytes_v = bits_to_int_bytes<bits>::value;

template <>
class bits_to_int_bytes<0>
{
public:
	static constexpr size_t value = 0;
};


}


#endif
