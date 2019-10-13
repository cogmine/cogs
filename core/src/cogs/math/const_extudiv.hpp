//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CONST_EXTUDIV
#define COGS_HEADER_MATH_CONST_EXTUDIV


#include "cogs/env.hpp"
#include "cogs/math/const_extumul.hpp"
#include "cogs/math/const_extuadd.hpp"
#include "cogs/mem/int_parts.hpp"


namespace cogs {


template <ulongest numerator_high_part, ulongest numerator_low_part, ulongest denominator>
class const_extudiv
{
private:
	// Narrows in on the correct answer.
	template <
		ulongest lesser_high_part,
		ulongest lesser_low_part,
		ulongest greater_high_part,
		ulongest greater_low_part,
		ulongest new_high_part = lesser_high_part,
		ulongest new_low_part = lesser_low_part
	>
	class helper
	{
	public:
		// new attempt
		typedef const_extumul2<new_high_part, new_low_part, 0, denominator> new_multiplied_t;

		static constexpr bool is_new_lesser =
			!new_multiplied_t::high_high_part
			&& !new_multiplied_t::low_high_part
			&& (
				(new_multiplied_t::high_low_part < numerator_high_part)
				|| (
					(new_multiplied_t::high_low_part == numerator_high_part)
					&& (new_multiplied_t::low_low_part < numerator_low_part)
				)
			);

		static constexpr bool is_new_greater =
			!!new_multiplied_t::high_high_part
			|| !!new_multiplied_t::low_high_part
			|| (
				(new_multiplied_t::high_low_part > numerator_high_part)
				|| (
					(new_multiplied_t::high_low_part == numerator_high_part)
					&& (new_multiplied_t::low_low_part > numerator_low_part)
					)
				);

		// If neither lesser or greater, than we've found an exact answer
		static constexpr ulongest next_lesser_high_part = is_new_lesser ? new_high_part : lesser_high_part;
		static constexpr ulongest next_lesser_low_part = is_new_lesser ? new_low_part : lesser_low_part;
		static constexpr ulongest next_greater_high_part = is_new_greater ? new_high_part : greater_high_part;
		static constexpr ulongest next_greater_low_part = is_new_greater ? new_low_part : greater_low_part;

		// Divide both by 2.
		static constexpr ulongest lesser_half_high_part = const_extudiv<next_lesser_high_part, next_lesser_low_part, 2>::high_part;
		static constexpr ulongest lesser_half_low_part = const_extudiv<next_lesser_high_part, next_lesser_low_part, 2>::low_part;
		static constexpr ulongest greater_half_high_part = const_extudiv<next_greater_high_part, next_greater_low_part, 2>::high_part;
		static constexpr ulongest greater_half_low_part = const_extudiv<next_greater_high_part, next_greater_low_part, 2>::low_part;

		// Add them.
		static constexpr ulongest added_high_part = const_extuadd2<lesser_half_high_part, lesser_half_low_part, greater_half_high_part, greater_half_low_part>::high_part;
		static constexpr ulongest added_low_part = const_extuadd2<lesser_half_high_part, lesser_half_low_part, greater_half_high_part, greater_half_low_part>::low_part;

		// Add 1, if they were both odd.
		static constexpr ulongest add_if_both_odd = (!!(next_greater_high_part % 2) && !!(next_lesser_high_part % 2)) ? 1 : 0;
		static constexpr ulongest added2_high_part = const_extuadd2<added_high_part, added_low_part, 0, add_if_both_odd>::high_part;
		static constexpr ulongest added2_low_part = const_extuadd2<added_high_part, added_low_part, 0, add_if_both_odd>::low_part;

		static constexpr bool is_new_equal = !is_new_lesser && !is_new_greater; 
		static constexpr ulongest next_new_high_part = is_new_equal ? new_high_part : added2_high_part;
		static constexpr ulongest next_new_low_part = is_new_equal ? new_low_part : added2_high_part;

		static constexpr bool is_new_solution = ((next_new_high_part == next_lesser_high_part) && (next_new_low_part == next_lesser_low_part));
		static constexpr ulongest next_lesser_high_part2 = (is_new_solution || is_new_equal) ? 0 : next_lesser_high_part;
		static constexpr ulongest next_lesser_low_part2 = (is_new_solution || is_new_equal) ? 0 : next_lesser_low_part;
		static constexpr ulongest next_greater_high_part2 = (is_new_solution || is_new_equal) ? 0 : next_greater_high_part;
		static constexpr ulongest next_greater_low_part2 = (is_new_solution || is_new_equal) ? 0 : next_greater_low_part;

		typedef helper<
			next_lesser_high_part2,
			next_lesser_low_part2,
			next_greater_high_part2,
			next_greater_low_part2,
			next_new_high_part,
			next_new_low_part
		> next_t;

		static constexpr ulongest high_part = next_t::high_part;
		static constexpr ulongest low_part = next_t::low_part;
	};

	template <ulongest high, ulongest low, ulongest new_high_part, ulongest new_low_part>
	class helper<high, low, high, low, new_high_part, new_low_part>
	{
	public:
		static constexpr ulongest high_part = high;
		static constexpr ulongest low_part = low;
	};

public:
	static constexpr ulongest high_part = helper<0, 1, numerator_high_part, numerator_low_part>::high_part;
	static constexpr ulongest low_part = helper<0, 1, numerator_high_part, numerator_low_part>::low_part;

	static constexpr ulongest value = low_part;
};


template <ulongest numerator_low_part, ulongest denominator>
class const_extudiv<0, numerator_low_part, denominator>
{
public:
	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = (numerator_low_part / denominator);
};

template <ulongest numerator_high_part, ulongest numerator_low_part>
class const_extudiv<numerator_high_part, numerator_low_part, 0>
{
public:
	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = 0;
};

template <ulongest numerator_low_part>
class const_extudiv<0, numerator_low_part, 0>
{
public:
	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = 0;
};

template <ulongest numerator_high_part, ulongest numerator_low_part>
class const_extudiv<numerator_high_part, numerator_low_part, 2>
{
private:
	static constexpr ulongest low_part1 = numerator_low_part >> 1;
	static constexpr ulongest low_part2 =
		((numerator_high_part & 1) != 0)
		? (low_part1 | ~((ulongest)-1 >> 1))
		: low_part1;

public:
	static constexpr ulongest high_part = numerator_high_part >> 1;
	static constexpr ulongest low_part = low_part2;
};

template <ulongest numerator_low_part>
class const_extudiv<0, numerator_low_part, 2>
{
public:
	static constexpr ulongest high_part = 0;
	static constexpr ulongest low_part = (numerator_low_part / 2);
};



}


#endif
