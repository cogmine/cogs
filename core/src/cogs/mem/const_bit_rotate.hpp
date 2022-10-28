//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifndef COGS_HEADER_MEM_CONST_BIT_ROTATE
#define COGS_HEADER_MEM_CONST_BIT_ROTATE


#include "cogs/env.hpp"


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute a constant bit rotate right of a constant integer value
/// @tparam x Value to rotate
/// @tparam n The number of bits to rotate
template <ulongest x, size_t n>
class const_bit_rotate_right
{
public:
	static constexpr ulongest value = ((x >> n) | (x << (sizeof(ulongest) - n)));
};

/// @ingroup ConstMath
/// @brief Meta template to compute a constant bit rotate left of a constant integer value
/// @tparam x Value to rotate
/// @tparam n The number of bits to rotate
template <ulongest x, size_t n>
class const_bit_rotate_left
{
public:
	static constexpr ulongest value = ((x << n) | (x >> (sizeof(ulongest) - n)));
};


}


#endif
