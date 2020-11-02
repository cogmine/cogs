//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_ARCH_BIT_SCAN
#define COGS_HEADER_ARCH_BIT_SCAN

#include <type_traits>

#include "cogs/load.hpp"

namespace cogs {

/// @brief Namespace for architecture-specific functionality
namespace arch {


// arch/bit_scan.hpp provides arch level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// If arch is 'unknown' we use the brute force approach.

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) <= 4),
	size_t>
bit_scan_reverse(const int_t& bits)
{
	uint64_t v = (make_unsigned_t<int_t>)load(bits);
	COGS_ASSERT(!!v);
	// Use de Bruijn sequences
	static const int table[32] =
	{
		 0,  9,  1, 10, 13, 21,  2, 29,
		11, 14, 16, 18, 22, 25,  3, 30,
		 8, 12, 20, 28, 15, 17, 24,  7,
		19, 27, 23,  6, 26,  5,  4, 31
	};
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return table[(v * 0x07C4ACDDU) >> 27];
}

// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > 4)
	&& (sizeof(int_t) <= 8),
	size_t>
bit_scan_reverse(const int_t& bits)
{
	uint64_t v = (make_unsigned_t<int_t>)load(bits);
	COGS_ASSERT(!!v);
	// Use de Bruijn sequences
	static const int table[64] = {
		 0, 47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63
	};
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return table[(v * 0x03f79d71b4cb0a89) >> 58];
}

// bits must not be zero
template <typename T>
inline std::enable_if_t<
	is_integral_v<T>
	&& (sizeof(T) > 8),
	size_t>
bit_scan_reverse(const T& bits)
{
	make_unsigned_t<T> v = (make_unsigned_t<T>)load(bits);
	COGS_ASSERT(!!v);
	size_t result = 0;
	uint64_t ll;
	for (;;)
	{
		ll = (uint64_t)v;
		v >>= (sizeof(uint64_t) * 8);
		if (!v)
			break;
		result += (sizeof(uint64_t) * 8);
	}
	return result + bit_scan_reverse(ll);
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) <= 4),
	size_t>
bit_scan_forward(const int_t& bits)
{
	uint32_t v = (make_unsigned_t<int_t>)load(bits);
	COGS_ASSERT(!!v);
	// Use de Bruijn sequences
	static const int table[32] = {
		 0,  1, 16,  2, 29, 17,  3, 22,
		30, 20, 18, 11, 13,  4,  7, 23,
		31, 15, 28, 21, 19, 10, 12,  6,
		14, 27,  9,  5, 26,  8, 25, 24,
	};
	return table[((v & -v) * 0x6EB14F9) >> 27];
}


// bits must not be zero
template <typename int_t>
inline std::enable_if_t<
	is_integral_v<int_t>
	&& (sizeof(int_t) > 4)
	&& (sizeof(int_t) <= 8),
	size_t>
bit_scan_forward(const int_t& bits)
{
	uint64_t v = (make_unsigned_t<int_t>)load(bits);
	COGS_ASSERT(!!v);
	// Use de Bruijn sequences
	static const int table[64] = {
		 0,  1, 48,  2, 57, 49, 28,  3,
		61, 58, 50, 42, 38, 29, 17,  4,
		62, 55, 59, 36, 53, 51, 43, 22,
		45, 39, 33, 30, 24, 18, 12,  5,
		63, 47, 56, 27, 60, 41, 37, 16,
		54, 35, 52, 21, 44, 32, 23, 11,
		46, 26, 40, 15, 34, 20, 31, 10,
		25, 14, 19,  9, 13,  8,  7,  6
	};
	return table[((v & -v) * 0x03f79d71b4cb0a89) >> 58];
}


// bits must not be zero
template <typename T>
inline std::enable_if_t<
	is_integral_v<T>
	&& (sizeof(T) > 8),
	size_t>
bit_scan_forward(const T& bits)
{
	make_unsigned_t<T> v = (make_unsigned_t<T>)load(bits);
	COGS_ASSERT(!!v);
	size_t result = 0;
	uint64_t ll;
	for (;;)
	{
		ll = (uint64_t)v;
		if (!!ll)
			break;
		v >>= (sizeof(uint64_t) * 8);
		result += (sizeof(uint64_t) * 8);
	}
	return result + bit_scan_forward(ll);
}


}
}


#endif
