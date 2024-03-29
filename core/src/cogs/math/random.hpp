//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_MATH_RANDOM
#define COGS_HEADER_MATH_RANDOM


#include "cogs/env.hpp"
#include "cogs/env/mem/bit_scan.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/mem/const_bit_scan.hpp"


namespace cogs {

/// @defgroup Random Random Number Generation
/// @{
/// @ingroup Math
/// @brief Random number generation
/// @}

// Random leverages os::random/env::random/arch::random
// Assumes the generator is able to produce some # of randomized bits
// (i.e. values are distributed across a range that is some exponent of 2)
// Assumes RAND_MAX is ((2^x)-1)

/// @ingroup Math
/// @brief The number of random bits generated by get_random.
static constexpr size_t random_bit_count = const_bit_scan_forward<((ulongest)RAND_MAX + 1)>::value;

typedef bits_to_uint_t<random_bit_count> random_uint_t;
typedef bits_to_int_t<random_bit_count> random_int_t;

/// @ingroup Random
/// @brief Gets a random value
/// @return A value containing random_bit_count set.
inline random_uint_t get_random()
{
	random_uint_t result = (random_int_t)std::rand();
	return result;
}

/// @ingroup Random
/// @brief Gets a random value with sufficient random bits to fill the specified integer type
/// @tparam uint_t Unsigned int type
/// @return A value with all bits randomly set.
template <typename uint_t>
inline uint_t get_random_int()
{
	const bool has_sign = ((uint_t)~(uint_t)0) < (uint_t)0;
	static_assert(!has_sign);
	const size_t bits = sizeof(uint_t) * 8;
	const bool evenlyDivisible = ((bits % random_bit_count) == 0);
	const size_t digits = (bits / random_bit_count) + (evenlyDivisible ? 0 : 1); // > 1
	uint_t result;
	if (bits <= random_bit_count)
		result = (uint_t)get_random();
	else
	{
		size_t i = digits;
		result = 0;
		for (;;)
		{
			result |= get_random();
			if (!--i)
				break;
			result <<= random_bit_count;
		}
	}
	return result;
}

/// @ingroup Random
/// @brief Gets a random value with sufficient random bits to fill the specified integer type
/// @tparam uint_t Unsigned int type
/// @param dst Pointer to bits to set
/// @param n Number of digits (uint_t) to set
template <typename uint_t>
inline void set_random_bits(uint_t* dst, size_t n)
{
	const bool has_sign = ((uint_t)~(uint_t)0) < (uint_t)0;
	static_assert(!has_sign);
	const size_t digit_bits = sizeof(uint_t) * 8;
	COGS_ASSERT(!!dst);
	size_t curDigit = 0;
	uint_t overflowValue = 0;
	uint_t digit;
	if (digit_bits == random_bit_count)
	{
		do {
			dst[curDigit] = (uint_t)get_random();
		} while (++curDigit < n);
	}
	else
	{
		digit = 0;
		size_t neededBits = digit_bits;
		size_t gotBits = 0;
		size_t rBits = 0;
		uint_t r;
		for (;;)
		{
			if (!rBits)
			{
				rBits = random_bit_count;
				r = (uint_t)get_random();
			}
			size_t bitsMoving = (neededBits > rBits) ? rBits : neededBits;
			digit |= (r << gotBits);
			neededBits -= bitsMoving;
			if (!!neededBits)
				gotBits += bitsMoving;
			else
			{
				dst[curDigit] = digit;
				if (++curDigit == n)
					break;
				gotBits = 0;
			}
			rBits -= bitsMoving;
			r >>= bitsMoving;
		}
	}
}

/// @ingroup Random
/// @brief Returns the number of random coin flips (bits) until heads (1) is found.
///
/// - 1/2, or 50% probability that 1 is returned
/// - 1/4, or 25% probability that 2 is returned
/// - 1/8, or 12.5% probability that 3 is returned
/// - 1/16, or 6.25% probability that 4 is returned
/// - 1/32, or 3.175% probability that 5 is returned, etc.
/// @tparam uint_t Unsigned int type
/// @param maxCount The maximum number of count flips we are interested in.  Default: max value of uint_t
/// @return The number of coin flips (bits) randomly generated until heads (1) was found.
template <typename uint_t>
inline uint_t random_coin_flips(uint_t maxCount = const_max_int_v<uint_t>)
{
	const bool has_sign = ((uint_t)~(uint_t)0) < (uint_t)0;
	static_assert(!has_sign);

	// count bits from least significant bit.
	// If random value is 0, count all bits, generate another.

	uint_t result = 0;
	for (;;)
	{
		random_uint_t r = get_random();
		if (!!r)
		{
			result += (uint_t)bit_scan_forward<random_uint_t>(r) + 1;
			if (result <= maxCount)
				break;
		}
		else
		{
			result += (uint_t)random_bit_count;
			if (result < maxCount)
				continue;
		}
		result = maxCount;
		break;
	}

	return result;
}


}


#endif
