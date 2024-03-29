//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_BYTES_TO_INT
#define COGS_HEADER_MATH_BYTES_TO_INT


#include <type_traits>
#include "cogs/math/next_exponent_of_two.hpp"


namespace cogs {


/// @ingroup Math
/// @brief Template helper to provide an integer type of (at least) the specified size in bytes
/// @tparam bytes Number of bytes
/// @tparam has_sign True if the int should be signed.  Default: true
template <size_t bytes, bool has_sign = true, typename enable = void>
class bytes_to_int
{
public:
	typedef void type;
};
template <size_t bytes, bool has_sign = true>
using bytes_to_int_t = typename bytes_to_int<bytes, has_sign>::type;


template <size_t bytes, bool has_sign>
class bytes_to_int<bytes, has_sign, typename std::enable_if_t<next_or_current_exponent_of_two_v<bytes> != bytes> >
{
public:
	typedef bytes_to_int_t<next_or_current_exponent_of_two_v<bytes> > type;
};


template <size_t bits>
class bytes_to_uint : public bytes_to_int<bits, false>
{
};
template <size_t bits>
using bytes_to_uint_t = typename bytes_to_uint<bits>::type;


}


#include "cogs/env.hpp"

#endif
