//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsToBeSplitUp, WorkInProgress
// Note: const_extudiv and extudiv need to be implemented and split out into extudiv.hpp

#ifndef COGS_EXTUMUL
#define COGS_EXTUMUL


#include "cogs/env.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {

	
/// @ingroup Math
/// @brief Computes the extended multiplication of two integers
/// @tparam uint_t Unsigned int type
template <typename uint_t>
inline uint_t extumul(const uint_t& src1, const uint_t& src2, uint_t& highPartRtn)
{
	//     F F
	//     F F
	// ---------
	//     E 1
	//   E 1
	//   E 1   
	// E 1
	//----------
	// F E 0 1
	
	uint_t l1 = get_low_part(src1);
	uint_t l2 = get_low_part(src2);
	uint_t h1 = get_high_part(src1);
	uint_t h2 = get_high_part(src2);

	uint_t ll = l1 * l2;
	uint_t lh = l1 * h2;
	uint_t hl = h1 * l2;
	uint_t hh = h1 * h2;

	//     F F
	//     F F
	// ---------
	//   E F 1 
	//   E 1
	// E 1
	//----------
	// F E 0 1

	
	uint_t llh = get_high_part(ll);
	lh += llh;				// will not overflow

	//     F F
	//     F F
	// ---------
	//   D 0 1 
	// F 1
	//----------
	// F E 0 1

	uint_t mid = lh + hl;	// might overflow
	if (mid < lh)			// if overflow
		hh += make_const_high_part<uint_t, 1>::value;

	uint_t mh = get_high_part(mid);
	uint_t ml_shifted = make_high_part(mid);

	uint_t lll = get_low_part(ll);

	uint_t low = ml_shifted | lll;
	uint_t high = hh + mh;	// will not overflow
	
	highPartRtn = high;
	return low;
}


/// @ingroup ConstMath
/// @brief Meta template to compute the extended multiplication of two constant integers
/// @tparam x Value to multiply
/// @tparam y Value to multiply
template <ulongest x, ulongest y>
class const_extumul
{
private:
	static const ulongest low_x = get_const_low_part<ulongest, x>::value;
	static const ulongest low_y = get_const_low_part<ulongest, y>::value;
	static const ulongest high_x = get_const_high_part<ulongest, x>::value;
	static const ulongest high_y = get_const_high_part<ulongest, y>::value;
	static const ulongest ll = low_x * low_y;
	static const ulongest lh = low_x * high_y;
	static const ulongest hl = high_x * low_y;
	static const ulongest hh = high_x * high_y;
	static const ulongest llh = get_const_high_part<ulongest, ll>::value;
	static const ulongest lh2 = lh + llh;				// will not overflow
	static const ulongest mid = lh2 + hl;				// might overflow
	static const ulongest carry = (mid < lh2) ? 1 : 0;
	static const ulongest hh2 = hh + make_const_high_part<ulongest, carry>::value;
	static const ulongest mh = get_const_high_part<ulongest, mid>::value;
	static const ulongest ml_shifted = make_const_high_part<ulongest, mid>::value;
	static const ulongest lll = get_const_low_part<ulongest, ll>::value;

public:
	static const ulongest high_part = hh2 + mh;			// will not overflow
	static const ulongest low_part = ml_shifted | lll;

	static const ulongest value = low_part;
};


/// @ingroup ConstMath
/// @brief Meta template to compute the extended multiplication of two constant integers
/// @tparam high_x High part of first value to multiply
/// @tparam low_x Low part of first value to multiply
/// @tparam high_y High part of second value to multiply
/// @tparam low_y Low part of second value to multiply
template <ulongest high_x, ulongest low_x, ulongest high_y, ulongest low_y>
class const_extumul2
{
private:
	static const ulongest llh = const_extumul<low_x, low_y>::high_part;
	static const ulongest lll = const_extumul<low_x, low_y>::low_part;

	static const ulongest lhh = const_extumul<low_x, high_y>::high_part;
	static const ulongest lhl = const_extumul<low_x, high_y>::low_part;

	static const ulongest hlh = const_extumul<high_x, low_y>::high_part;
	static const ulongest hll = const_extumul<high_x, low_y>::low_part;

	static const ulongest hhh = const_extumul<high_x, high_y>::high_part;
	static const ulongest hhl = const_extumul<high_x, high_y>::low_part;

	static const ulongest lh1 = llh + lhl;
	static const ulongest carry_low1 = (lh1 < llh) ? 1 : 0;
	static const ulongest lh2 = lh1 + hll;
	static const ulongest carry_low2 = carry_low1 + ((lh2 < lh1) ? 1 : 0);

	static const ulongest hl1 = lhh + carry_low2;
	static const ulongest carry_high1 = (hl1 < lhh) ? 1 : 0;
	static const ulongest hl2 = hl1 + hlh;
	static const ulongest carry_high2 = carry_high1 + ((hl2 < hl1) ? 1 : 0);
	static const ulongest hl3 = hl2 + hhl;
	static const ulongest carry_high3 = carry_high2 + ((hl3 < hl2) ? 1 : 0);

	static const ulongest hh1 = hhh + carry_high3;

public:
	static const ulongest high_high_part = hh1;
	static const ulongest low_high_part = hl3;
	static const ulongest high_low_part = lh2;
	static const ulongest low_low_part = lll;

	static const ulongest high_part = high_low_part;
	static const ulongest low_part = low_low_part;

	static const ulongest value = low_part;
};




template <ulongest numerator_high_part, ulongest numerator_low_part, ulongest denominator>
class const_extudiv
{
#if 0

#error TODO

// TBD

private:
	static const ulongest rh1 = numerator_high_part / denominator;
	static const ulongest rh1m = numerator_high_part % denominator;

	static const ulongest dl = get_const_low_part<ulongest, denominator>::value;
	static const ulongest dh = get_const_high_part<ulongest, denominator>::value;

	static const ulongest adjusted_dh = dh + (!dl ? 0 : 1);
	
	template <ulongest denominator_high, ulongest denominator_low, ulongest result_high, ulongest result_low, ulongest remaining_numerator_high, ulongest remaining_numerator_low>
	class helper
	{
	public:
		static const ulongest rnhh = get_const_high_part<ulongest, remaining_numerator_high>::value;
		static const ulongest rnhl = get_const_low_part<ulongest, remaining_numerator_high>::value;
		static const ulongest rnlh = get_const_high_part<ulongest, remaining_numerator_low>::value;
		static const ulongest rnll = get_const_low_part<ulongest, remaining_numerator_low>::value;

		// To start, we know remaining_numerator_high < denom.
		// So, if !!rnhh, we know !!dh
		static const ulongest result_high = ;
		static const ulongest result_low = ;
	};

	// If no high part on the denominator, use simpler approach
	template <ulongest denominator_low, ulongest result_high, ulongest result_low, ulongest remaining_numerator_high, ulongest remaining_numerator_low>
	class helper<0, denominator_low, result_high, result_low, remaining_numerator_high, remaining_numerator_low
	{
	public:	
	};


public:
	static const ulongest high_part = helper<dh, dl, rh1, 0, rh1m>::high_part;
	static const ulongest low_part = helper<dh, dl, rh1, 0, rh1m>::low_part;

#endif
};


template <ulongest numerator_low_part, ulongest denominator>
class const_extudiv<0, numerator_low_part, denominator>
{
public:
	static const ulongest high_part = 0;
	static const ulongest low_part  = (numerator_low_part / denominator);
};

template <ulongest numerator_high_part, ulongest numerator_low_part>
class const_extudiv<numerator_high_part, numerator_low_part, 0>
{
public:
	static const ulongest high_part = 0;
	static const ulongest low_part  = 0;
};

template <ulongest numerator_low_part>
class const_extudiv<0, numerator_low_part, 0>
{
public:
	static const ulongest high_part = 0;
	static const ulongest low_part  = 0;
};


inline ulongest extudiv(ulongest numerator_high_part, ulongest numerator_low_part, ulongest denominator_low_part, ulongest denominator_high_part = 0)
{
	// TBD
	COGS_ASSERT(false);
	return 0;
}


}


#endif
