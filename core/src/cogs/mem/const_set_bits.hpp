//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_CONST_SET_BITS
#define COGS_HEADER_MEM_CONST_SET_BITS


#include "cogs/env.hpp"


namespace cogs {


/// @ingroup Mem
/// @brief Generates a constant the with specified number of bits set
/// @tparam int_t Type to set bits of
/// @tparam n Number of bits to set
template <typename int_t, size_t n = sizeof(int_t) * 8 >
class const_set_bits
{
public:
	static constexpr int_t value = (const_set_bits<int_t, n - 1>::value << 1) | 1;
};


template <typename int_t>
class const_set_bits<int_t, 0>
{
public:
	static constexpr int_t value = 0;
};


template <typename int_t>
class const_set_bits<int_t, 1>
{
public:
	static constexpr int_t value = 1;
};


}


#endif
