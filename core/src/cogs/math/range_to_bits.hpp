//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_RANGE_TO_BITS
#define COGS_RANGE_TO_BITS


#include "cogs/math/int_types.hpp"
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
	static const size_t bitsRequiredForMin = range_to_bits<0, (ulongest)~min_value >::value + 1;	// if signed
	static const size_t bitsRequiredForMax = range_to_bits<0, max_value>::value;

public:
	static const size_t value = (min_value >= 0) ?
									bitsRequiredForMax
								:	(
										(bitsRequiredForMin > bitsRequiredForMax) ?
											bitsRequiredForMin
										:	bitsRequiredForMax
									);
};


template <ulongest max_value>
class range_to_bits<0, max_value>
{
private:
	template <typename int_t, int_t x>
	class helper;

	template <>
	class helper<unsigned char, 0>
	{
	public:
		static const size_t value = 0;
	};

	template <unsigned char x>
	class helper<unsigned char, x>
	{
	public:
		static const size_t value = helper<unsigned char, (x >> 1) >::value + 1;
	};
	
	template <typename int_t, int_t x>
	class helper
	{
	public:
		typedef typename bytes_to_uint_t<sizeof(int_t) / 2> half_t;

		static const half_t highPart = helper<half_t, (half_t)get_const_high_part<int_t, x>::value>::value;
		static const half_t lowPart = helper<half_t, (half_t)get_const_low_part<int_t, x>::value>::value;

		static const int_t value = (!highPart) ? lowPart : (highPart + (sizeof(half_t)*8));
	};

public:
	static const size_t value = helper<ulongest, max_value>::value;
};


template <>
class range_to_bits<0, 0>
{
public:
	static const size_t value = 0;
};


}


#endif

