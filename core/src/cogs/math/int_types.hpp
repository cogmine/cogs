//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_INT_TYPES
#define COGS_HEADER_MATH_INT_TYPES


#include <cstdint>

#include "cogs/env.hpp"


namespace cogs {


/// @ingroup Math
/// @brief The largest native signed integer type available
typedef env::bytes_to_int_t<COGS_LONGEST_INT> longest;
/// @ingroup Math
/// @brief The largest native unsigned integer type available
typedef env::bytes_to_int_t<COGS_LONGEST_INT> ulongest;

/// @ingroup Math
/// @brief The number of bits in the largested native integer type available
static constexpr size_t longest_bits = sizeof(longest) * 8;



}


#endif

