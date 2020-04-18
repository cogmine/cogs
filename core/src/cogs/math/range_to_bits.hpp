//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_RANGE_TO_BITS
#define COGS_HEADER_MATH_RANGE_TO_BITS


#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {

/// @ingroup ConstMath
/// @brief Template helper that converts a range to the number of bits necessary to represent it.
/// @tparam min_value Minimum value of the range
/// @tparam max_value Maximum value of the range
template <longest min_value, ulongest max_value>
class range_to_bits
{
public:
	static constexpr size_t bitsRequiredForMin = range_to_bits<0, (ulongest)~min_value >::value + 1; // if signed
	static constexpr size_t bitsRequiredForMax = range_to_bits<0, max_value>::value;

public:
	static constexpr size_t value =
		(min_value >= 0)
			? bitsRequiredForMax
			: ((bitsRequiredForMin > bitsRequiredForMax)
				? bitsRequiredForMin
				: bitsRequiredForMax);
};
template <longest min_value, ulongest max_value> inline constexpr size_t range_to_bits_v = range_to_bits<min_value, max_value>::value;


template <ulongest max_value>
class range_to_bits<0, max_value>
{
private:
	template <typename int_t, int_t x, bool unused = true>
	class helper;

	template <bool unused>
	class helper<unsigned char, 0, unused>
	{
	public:
		static constexpr size_t value = 0;
	};

	template <unsigned char x, bool unused>
	class helper<unsigned char, x, unused>
	{
	public:
		static constexpr size_t value = helper<unsigned char, (x >> 1), true>::value + 1;
	};

	template <typename int_t, int_t x, bool unused>
	class helper
	{
	public:
		typedef bytes_to_uint_t<sizeof(int_t) / 2> half_t;

		static constexpr half_t highPart = helper<half_t, (half_t)get_const_high_part_v<int_t, x>, true>::value;
		static constexpr half_t lowPart = helper<half_t, (half_t)get_const_low_part_v<int_t, x>, true>::value;

		static constexpr int_t value = (!highPart) ? lowPart : (highPart + (sizeof(half_t)*8));
	};

public:
	static constexpr size_t value = helper<ulongest, max_value, true>::value;
};


template <>
class range_to_bits<0, 0>
{
public:
	static constexpr size_t value = 0;
};


}


#endif
