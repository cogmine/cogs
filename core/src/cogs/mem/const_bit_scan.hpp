//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifndef COGS_CONST_BIT_SCAN
#define COGS_CONST_BIT_SCAN


#include <type_traits>


namespace cogs {


/// @ingroup Mem
/// @brief Scans the constant value in reverse for the first set bit
/// @tparam x Value to scan
template <ulongest x>
class const_bit_scan_reverse
{
public:
	static constexpr ulongest value = const_bit_scan_reverse< (x >> 1) >::value + 1;
};

template <ulongest x>
constexpr bool const_bit_scan_reverse_v = const_bit_scan_reverse<x>::value;


template <>
class const_bit_scan_reverse<1>
{
public:
	static constexpr ulongest value = 0;
};

template <>
class const_bit_scan_reverse<0> { };	// invalid


/// @ingroup Mem
/// @brief Scans the constant value for the first set bit
/// @tparam x Value to scan
template <ulongest x>
class const_bit_scan_forward
{
public:
	static constexpr ulongest value = ((x & 1) == 1) ? 0 : (const_bit_scan_forward< (x >> 1) >::value + 1);
};

template <ulongest x>
constexpr bool const_bit_scan_forward_v = const_bit_scan_forward<x>::value;

template <>
class const_bit_scan_forward<1>
{
public:
	static constexpr ulongest value = 0;
};

template <>
class const_bit_scan_forward<0> { };	// invalid


}


#endif

