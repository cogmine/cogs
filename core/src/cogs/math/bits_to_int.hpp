//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_BITS_TO_INT
#define COGS_HEADER_MATH_BITS_TO_INT


#include "cogs/math/bits_to_bytes.hpp"
#include "cogs/math/bytes_to_int.hpp"


namespace cogs {


/// @ingroup Math
/// @brief Template helper to provide an integer type of (at least) the specified size in bits
/// @tparam bits Number of bits
/// @tparam has_sign True if the int should be signed.  Default: true
template <size_t bits, bool has_sign = true>
class bits_to_int
{
public:
	typedef bytes_to_int_t< bits_to_int_bytes_v<bits>, has_sign> type;
};
template <size_t bits, bool has_sign = true>
using bits_to_int_t = typename bits_to_int<bits, has_sign>::type;


template <bool has_sign>
class bits_to_int<0, has_sign>
{
public:
	typedef bits_to_int_t<1, has_sign> type;
};


template <size_t bits>
class bits_to_uint : public bits_to_int<bits, false>
{
};
template <size_t bits>
using bits_to_uint_t = typename bits_to_uint<bits>::type;


}


#endif
