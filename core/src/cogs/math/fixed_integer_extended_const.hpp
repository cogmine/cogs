//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXED_INTEGER_EXTENDED_CONST
#define COGS_HEADER_MATH_FIXED_INTEGER_EXTENDED_CONST

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/compatible.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/env.hpp"
#include "cogs/env/math/umul.hpp"
#include "cogs/math/extumul.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/math/is_integral.hpp"
#include "cogs/math/is_arithmetic.hpp"
#include "cogs/math/is_signed.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/value_to_bits.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified
#pragma warning (disable: 4307)	// multiple copy constructors specified

template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;

template <bool has_sign, size_t bits, ulongest... values> class is_arithmetic<fixed_integer_extended_const<has_sign, bits, values...> > : public std::true_type { };

template <bool has_sign, size_t bits, ulongest... values> class is_integral<fixed_integer_extended_const<has_sign, bits, values...> > : public std::true_type { };

template <bool has_sign, size_t bits, ulongest... values> class is_signed<fixed_integer_extended_const<has_sign, bits, values...> > { public: static constexpr bool value = has_sign; };

template <bool has_sign, size_t bits, ulongest... values> class is_const_type<fixed_integer_extended_const<has_sign, bits, values...> > : public std::true_type { };


template <bool has_sign, size_t bits, ulongest... values>
class int_to_fixed_integer<fixed_integer_extended_const<has_sign, bits, values...> >
{
public:
	typedef fixed_integer_extended_const<has_sign, bits, values...> type;
};



template <bool has_sign_in, size_t bits_in, ulongest low_digit_in, ulongest... highDigits>
class fixed_integer_extended_const<has_sign_in, bits_in, low_digit_in, highDigits...>
{
public:
	typedef fixed_integer_extended_const<has_sign_in, bits_in, low_digit_in, highDigits...> this_t;

	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = bits_in;
	static constexpr size_t n_digits = sizeof...(highDigits) + 1;
	static constexpr ulongest int_value = low_digit_in;
	static constexpr bool is_const_negative = fixed_integer_extended_const<has_sign, bits, highDigits...>::is_const_negative;

	typedef fixed_integer_extended_const<is_const_negative, bits, low_digit_in, highDigits...> as_extended_t;

	static constexpr ulongest low_digit = low_digit_in;
	static constexpr bool is_const_zero = (low_digit == 0) && (fixed_integer_extended_const<has_sign, bits, highDigits...>::is_const_zero);

	typedef ulongest int_t;


private:
	template <bool, size_t, ulongest...>
	friend class fixed_integer_extended_const;

	template <typename, typename>
	friend class const_compared;

	static constexpr bool is_negative_one = (low_digit == (ulongest)-1) && fixed_integer_extended_const<has_sign, bits, highDigits...>::is_negative_one;

	static constexpr bool is_high_part_zero = fixed_integer_extended_const<has_sign, bits, highDigits...>::is_const_zero;
	static constexpr bool is_high_part_negative_one = fixed_integer_extended_const<has_sign, bits, highDigits...>::is_negative_one;
	static constexpr bool is_high_digit_zero = fixed_integer_extended_const<has_sign, bits, highDigits...>::is_high_digit_zero;
	static constexpr bool is_high_digit_negative_one = fixed_integer_extended_const<has_sign, bits, highDigits...>::is_high_digit_negative_one;

	static constexpr bool can_reduce_positive = is_high_part_zero;
	static constexpr bool low_sign_bit_set = ((longest)low_digit < 0);
	static constexpr bool can_reduce_negative = has_sign && is_high_part_negative_one && low_sign_bit_set;	// ensures high bit of low part is set


	template <bool is_zero, bool overflow, ulongest... lowPart>
	class calculate_negated;

	template <ulongest... lowPart>
	class calculate_negated<false, false, lowPart...>
	{
	public:
		static constexpr ulongest new_low_digit = ~low_digit;
		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::template calculate_negated<false, false, lowPart..., new_low_digit>::type type;
	};

	template <ulongest... lowPart>
	class calculate_negated<false, true, lowPart...>
	{
	public:
		static constexpr ulongest new_low_digit = ~low_digit + 1;
		static constexpr bool new_overflow = new_low_digit == 0;
		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::template calculate_negated<false, new_overflow, lowPart..., new_low_digit>::type type;
	};

	template <bool overflow, ulongest... lowPart>
	class calculate_negated<true, overflow, lowPart...>
	{
	public:
		typedef this_t type;
	};

	template <ulongest... lowPart>
	class calculate_negative_overflow
	{
	public:
		static constexpr bool overflow = low_digit == 0;
		static constexpr ulongest new_low_digit = low_digit - 1;
		typedef std::conditional_t<overflow,
			fixed_integer_extended_const<has_sign, bits, lowPart..., new_low_digit, highDigits...>,
			typename fixed_integer_extended_const<has_sign, bits, highDigits...>::template calculate_incremented<lowPart..., new_low_digit>::type
		> type;
	};


public:
	typedef std::conditional_t<can_reduce_positive,
		typename fixed_integer_native_const<false, (sizeof(ulongest) * 8), low_digit>::reduced_t,
		std::conditional_t<can_reduce_negative,
		typename fixed_integer_native_const<true, (sizeof(longest) * 8), low_digit>::reduced_t,
		typename fixed_integer_extended_const<is_const_negative, bits, highDigits...>::template calculate_reduced<low_digit>::type
		>
	>	reduced_t;

	typedef typename calculate_negated<is_const_zero, true>::type::reduced_t negative_t;
	typedef std::conditional_t<is_const_negative, negative_t, reduced_t> abs_t;


private:

	template <ulongest... lowPart>
	class calculate_inversed
	{
	public:
		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::template calculate_inversed<lowPart..., ~low_digit>::type type;
	};

	template <ulongest... lowPart>
	class calculate_reduced
	{
	private:
		static constexpr size_t reduced_low_digit = is_const_negative ? range_to_bits_v<(longest)low_digit, 0> : range_to_bits_v<0, (ulongest)low_digit>;

	public:
		typedef std::conditional_t<can_reduce_positive,
				fixed_integer_extended_const<false, (sizeof...(lowPart) * (sizeof(ulongest) * 8)) + reduced_low_digit, lowPart...>,
					std::conditional_t<can_reduce_negative,
						fixed_integer_extended_const<true, (sizeof...(lowPart) * (sizeof(ulongest) * 8)) + reduced_low_digit, lowPart...>,
						typename fixed_integer_extended_const<is_const_negative, bits, highDigits...>::template calculate_reduced<lowPart..., low_digit>::type
					>
			>	type;
	};

	template <ulongest... lowPart>
	class calculate_incremented
	{
	public:
		static constexpr ulongest new_low_digit = low_digit + 1;
		static constexpr bool overflow = new_low_digit == 0;
		typedef std::conditional_t<overflow,
			fixed_integer_extended_const<has_sign, bits, lowPart..., new_low_digit, highDigits...>,
			typename fixed_integer_extended_const<has_sign, bits, highDigits...>::template calculate_incremented<lowPart..., new_low_digit>::type
			> type;
	};

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class get_added
	{
	public:
		static constexpr ulongest new_low_digit = (ulongest)low_digit + (ulongest)value2;
		static constexpr bool overflow = ((ulongest)low_digit + (ulongest)value2) < (ulongest)low_digit;
		static constexpr bool is_negative2 = value2 < 0;
		static constexpr size_t new_bits = bits + 1;

		typedef typename fixed_integer_extended_const<has_sign, new_bits, highDigits...>::template calculate_incremented<new_low_digit>::type overflow_increment_t;
		typedef fixed_integer_extended_const<has_sign, new_bits, new_low_digit, highDigits...> no_overflow_copy_t;
		typedef typename fixed_integer_extended_const<has_sign, new_bits, highDigits...>::template calculate_negative_overflow<new_low_digit>::type negative_overflow_t;

		typedef std::conditional_t<is_negative2,
					std::conditional_t<overflow,
						overflow_increment_t,
						no_overflow_copy_t
					>,
					std::conditional_t<overflow,
						no_overflow_copy_t,
						negative_overflow_t
					>
			> type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class get_added_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class get_added_extended<has_sign2, bits2, value2>
	{
	public:
		template <bool overflow, ulongest... lowPart>
		class calculate_added_extended
		{
		public:
			static constexpr ulongest new_low_digit = low_digit + value2 + (overflow ? 1 : 0);
			static constexpr bool new_overflow = overflow ? (new_low_digit <= low_digit) : (new_low_digit < low_digit);
			static constexpr bool is_negative2 = has_sign2 && ((longest)value2 < 0);
			static constexpr size_t new_bits = bits + 1;

			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
						template calculate_incremented<lowPart..., new_low_digit>::type overflow_increment_t;

			typedef fixed_integer_extended_const<has_sign, bits, lowPart..., new_low_digit, highDigits...> no_overflow_copy_t;

			typedef typename fixed_integer_extended_const<has_sign, new_bits, highDigits...>::
						template calculate_negative_overflow<lowPart..., new_low_digit>::type negative_overflow_t;

			typedef std::conditional_t<is_negative2,
						std::conditional_t<overflow,
							overflow_increment_t,
							no_overflow_copy_t
						>,
						std::conditional_t<overflow,
							no_overflow_copy_t,
							negative_overflow_t
						>
					> type;
		};

		typedef typename calculate_added_extended<false>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class get_added_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <bool overflow, ulongest... lowPart>
		class calculate_added_extended
		{
		private:
			static constexpr ulongest new_low_digit = low_digit + low_digit2 + (overflow ? 1 : 0);
			static constexpr bool new_overflow = overflow ? (newDigit <= origDigit) : (newDigit < origDigit);

		public:
			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template get_added_extended<has_sign2, bits2, highDigits2...>::
				template calculate_added_extended<new_overflow, lowPart..., new_low_digit>::type type;
		};

		static constexpr bool new_overflow = ((ulongest)low_digit + (ulongest)low_digit2) < (ulongest)low_digit;
		static constexpr ulongest new_low_digit = low_digit + low_digit2;

		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template get_added_extended<has_sign2, bits2, highDigits2...>::
			template calculate_added_extended<new_overflow, new_low_digit>::type type;
	};

	template <typename T>
	class get_added_extended2;

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class get_added_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		typedef typename get_added_extended<has_sign2, bits2, values2...>::type type;
	};

	template <ulongest value2>
	class get_multiplied
	{
	public:
		template <ulongest overflow, ulongest... lowPart>
		class calculate_multiplied
		{
		public:
			static constexpr ulongest new_low_digit = const_extumul<low_digit, value2>::low_part + overflow;
			static constexpr ulongest new_overflow = const_extumul<low_digit, value2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template get_multiplied<value2>::
				template calculate_multiplied<new_overflow, lowPart..., new_low_digit>::type type;
		};

		typedef typename calculate_multiplied<0>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class get_multiplied_extended;
	
	template <bool has_sign2, size_t bits2, ulongest value2>
	class get_multiplied_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... leading_zeros>
		class calculate_multiplied_extended2
		{
		public:
			template <ulongest overflow, ulongest... lowPart>
			class calculate_multiplied_extended
			{
			public:
				static constexpr ulongest new_low_digit = const_extumul<low_digit, value2>::low_part + overflow;
				static constexpr ulongest new_overflow = const_extumul<low_digit, value2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

				typedef typename fixed_integer_extended_const<false, bits + bits2, lowPart..., new_low_digit, new_overflow>::reduced_t type;
			};

			typedef typename calculate_multiplied_extended<0, leading_zeros...>::type tmp_t1;

			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template get_multiplied_extended<has_sign2, bits2, value2>::
				template calculate_multiplied_extended2<leading_zeros..., 0>::type tmp_t2;

			typedef typename tmp_t1::template get_added_extended2<tmp_t2>::type type;
			//typedef decltype(std::declval<tmp_t1>() + declval<tmp_t2>()) type;
		};

		typedef typename calculate_multiplied_extended2<>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class get_multiplied_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... leading_zeros>
		class calculate_multiplied_extended2
		{
		public:
			template <ulongest overflow, ulongest... lowPart>
			class calculate_multiplied_extended
			{
			public:
				static constexpr ulongest new_low_digit = const_extumul<low_digit, low_digit2>::low_part + overflow;
				static constexpr ulongest new_overflow = const_extumul<low_digit, low_digit2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

				typedef typename get_multiplied_extended<has_sign2, bits2, highDigits2...>::
					template calculate_multiplied_extended2<leading_zeros...>::
					template calculate_multiplied_extended<new_overflow, lowPart..., new_low_digit>::type type;
			};
		
			typedef typename calculate_multiplied_extended<0, leading_zeros...>::type tmp_t1;

			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template get_multiplied_extended<has_sign2, bits2, low_digit2, highDigits2...>::
				template calculate_multiplied_extended2<leading_zeros..., 0>::type tmp_t2;

			typedef typename tmp_t1::template get_added_extended2<tmp_t2>::type type;
			//typedef decltype(declval<tmp_t1>() + declval<tmp_t2>()) type;
		};

		typedef typename calculate_multiplied_extended2<>::type type;
	};

	template <typename T>
	class get_multiplied_extended2;

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class get_multiplied_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		typedef typename get_multiplied_extended<has_sign2, bits2, values2...>::type type;
	};

	template <typename abs_t1, typename abs_t2, typename T1, typename T2>
	class divide_guess_and_check
	{
	public:
		typedef typename T2::as_extended_t::template get_added_extended2<typename T1::as_extended_t::negative_t::as_extended_t>::type difference_t;

		typedef typename difference_t::template right_shift_extended<1>::type half_difference_t;	// divide by 2

		typedef typename T1::as_extended_t::template get_added_extended2<half_difference_t>::type::as_extended_t half_way_t;

		typedef typename abs_t2::template get_multiplied_extended2<half_way_t>::type::as_extended_t test_result_t;

		static constexpr int cmp = half_difference_t::is_const_zero ? 0 : abs_t1::template compare_extended2<test_result_t>::value;

		typedef std::conditional_t<(cmp >= 0), half_way_t, T1> new_T1;
		typedef std::conditional_t<(cmp > 0), T2, half_way_t> new_T2;

		typedef typename divide_guess_and_check<abs_t1, abs_t2, new_T1, new_T2>::type type;
	};

	template <typename abs_t1, typename abs_t2, typename T1>
	class divide_guess_and_check<abs_t1, abs_t2, T1, T1>
	{
	public:
		typedef T1 type;
	};

	template <typename abs_t1, typename abs_t2>
	class get_divide_whole
	{
	public:
		static constexpr bool is_larger_or_equal1 = abs_t1::template compare_extended2<abs_t2>::value >= 0;

		typedef std::conditional_t<is_larger_or_equal1, fixed_integer_native_const<false, 1, 1>, fixed_integer_native_const<false, 1, 0> > new1;
		typedef std::conditional_t<is_larger_or_equal1, abs_t1, fixed_integer_native_const<false, 1, 0> > new2;
		typedef typename divide_guess_and_check<abs_t1, abs_t2, new1, new2>::type type;
	};

	template <typename T2>
	class get_divide_whole2
	{
	public:
		typedef typename T2::abs_t::as_extended_t abs_t2;
		static constexpr bool is_const_negative2 = T2::is_const_negative;
		static constexpr bool negate_result = is_const_negative != is_const_negative2;
		typedef typename get_divide_whole<typename abs_t::as_extended_t, abs_t2>::type tmp_t;

		std::conditional_t<negate_result, typename tmp_t::as_extended_t::negative_t, tmp_t> type;
	};

	template <typename T2>
	class get_modulo
	{
	public:
		typedef typename get_divide_whole2<T2>::type div_t;
		typedef typename div_t::template get_multiplied_extended2<T2>::type mul_t;
		typedef typename get_added_extended2<typename mul_t::negative_t>::type type;
	};

	template <typename greater_t, typename lesser_t, typename modulo_t>
	class get_gcd
	{
	public:
		typedef decltype(std::declval<lesser_t>() % std::declval<modulo_t>()) modulo_t2;
		typedef typename get_gcd<lesser_t, modulo_t, modulo_t2>::type type;
	};

	template <typename greater_t, typename lesser_t>
	class get_gcd<greater_t, lesser_t, zero_t>
	{
	public:
		typedef lesser_t type;
	};

	template <typename greater_t>
	class get_gcd<greater_t, zero_t, zero_t>
	{
	public:
		typedef greater_t type;
	};

	template <typename abs_t1, typename abs_t2>
	class get_gcd2
	{
	public:
		static constexpr bool is_first_greater = abs_t1::template compare_extended2<abs_t2>::value > 0;
		typedef std::conditional_t<is_first_greater, abs_t1, abs_t2> greater_t;
		typedef std::conditional_t<is_first_greater, abs_t2, abs_t1> lesser_t;

		// Need to special case out zero, to avoid divide by 0 errors.
		typedef std::conditional_t<lesser_t::is_const_zero, one_t, lesser_t> lesser_t3;


		typedef typename greater_t::template get_modulo<lesser_t3>::type::as_extended_t modulo_t;

		typedef typename get_gcd<greater_t, lesser_t, modulo_t>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class compare_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class compare_extended<has_sign2, bits2, value2>
	{
	public:
		static constexpr bool highMatched = is_const_negative ? is_high_part_negative_one : is_high_part_zero;
		static constexpr int this_part = ((low_digit == value2) ? 0 : (low_digit < value2 ? -1 : 1));

		// if both are neg, and high part is not -1, it's smaller
		// if both are positive, and high part is not 0, it's larger
		static constexpr int value = highMatched ? this_part : (is_const_negative ? -1 : 1);
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class compare_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		static constexpr int high_part = fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template compare_extended<has_sign2, bits2, highDigits2...>::value;

		static constexpr int this_part = ((low_digit == low_digit2) ? 0 : (low_digit < low_digit2 ? -1 : 1));

		static constexpr int value = (high_part != 0) ? high_part : this_part;
	};

	template <typename T>
	class compare_extended2
	{
	};

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		static constexpr bool is_const_negative2 = fixed_integer_extended_const<has_sign2, bits2, values2...>::is_const_negative;
		static constexpr bool is_same_sign = (is_const_negative == is_const_negative2);
		static constexpr int tmp = compare_extended<has_sign2, bits2, values2...>::value;
		static constexpr int value = !is_same_sign ? (is_const_negative ? -1 : 1) : tmp;
	};

	template <size_t n, ulongest... lowDigits>
	class left_shift_extended
	{
	};

	template <size_t n, ulongest highestNewLowDigit, ulongest... lowDigits>
	class left_shift_extended<n, highestNewLowDigit, lowDigits...>
	{
	public:
		static constexpr ulongest new_low_digit = highestNewLowDigit | (low_digit << n);
		static constexpr ulongest low_overflow = (low_digit >> ((sizeof(ulongest) * 8) - n));

		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template left_shift_extended<n, low_overflow, lowDigits..., new_low_digit>::type type;
	};

	template <size_t n>
	class left_shift_extended<n>
	{

	public:
		static constexpr bool is_whole_digit_shift = (n / (sizeof(ulongest) * 8)) > 0;
		static constexpr size_t remaining_shift_n = is_whole_digit_shift ? (n - (sizeof(ulongest) * 8)) : 0;

		typedef typename fixed_integer_extended_const<has_sign, bits, 0, low_digit, highDigits...>::
			template left_shift_extended<remaining_shift_n>::type whole_shifted_t;

		static constexpr size_t n2 = is_whole_digit_shift ? ((sizeof(ulongest) * 8) / 2) : n;
		typedef typename left_shift_extended<n2, 0>::type each_shifted_t;

		typedef std::conditional_t<is_whole_digit_shift, whole_shifted_t, each_shifted_t> type;
	};

	template <>
	class left_shift_extended<0>
	{
	public:
		typedef fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...> type;
	};

	template <size_t n, ulongest... lowDigits>
	class right_shift_extended
	{
	};

	template <size_t n, ulongest highestNewLowPart, ulongest... lowDigits>
	class right_shift_extended<n, highestNewLowPart, lowDigits...>
	{
	public:
		static constexpr ulongest new_low_digit = highestNewLowPart | (low_digit << ((sizeof(ulongest) * 8) - n));
		static constexpr ulongest low_overflow = (low_digit >> n);

		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template right_shift_extended<n, low_overflow, lowDigits..., new_low_digit>::type type;
	};

	template <size_t n>
	class right_shift_extended<n>
	{
	public:
		static constexpr bool is_whole_digit_shift = (n / (sizeof(ulongest) * 8)) > 0;
		static constexpr size_t remaining_shift_n = is_whole_digit_shift ? (n - (sizeof(ulongest) * 8)) : 0;

		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template right_shift_extended<remaining_shift_n>::type whole_shifted_t;

		static constexpr ulongest low_part = (low_digit >> n);

		typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
			template right_shift_extended<n, low_part>::type each_shifted_t;

		typedef std::conditional_t<is_whole_digit_shift, whole_shifted_t, each_shifted_t> type;
	};


	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitor_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitor_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitor_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign, bits, lowPart..., low_digit | value2, highDigits...> type;
		};

		typedef typename calculate_bitor_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitor_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitor_extended
		{
		public:
			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template bitor_extended<has_sign2, bits2, highDigits2...>::
				template calculate_bitor_extended<lowPart..., low_digit | low_digit2>::type type;
		};
		
		typedef typename calculate_bitor_extended<>::type  type;
	};


	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitand_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitand_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitand_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign2, bits2, lowPart..., low_digit & value2> type;
		};

		typedef typename calculate_bitand_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitand_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitand_extended
		{
		public:
			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template bitand_extended<has_sign2, bits2, highDigits2...>::
				template calculate_bitand_extended<lowPart..., low_digit & low_digit2>::type type;
		};

		typedef typename calculate_bitand_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitxor_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitxor_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitxor_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign, bits, lowPart..., low_digit ^ value2, highDigits...> type;
		};

		typedef typename calculate_bitxor_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitxor_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitxor_extended
		{
		public:
			typedef typename fixed_integer_extended_const<has_sign, bits, highDigits...>::
				template bitxor_extended<has_sign2, bits2, highDigits2...>::
				template calculate_bitxor_extended<lowPart..., low_digit ^ low_digit2>::type type;
		};

		typedef typename calculate_bitxor_extended<>::type  type;
	};

	template <ulongest... highDigits2>
	class bit_count_extended;

	template <ulongest low_digit2>
	class bit_count_extended<low_digit2>
	{
	public:
		static constexpr size_t value = fixed_integer_native_const<false, (sizeof(ulongest) * 8), low_digit2>::const_bit_count;
	};

	template <ulongest low_digit2, ulongest... highDigits2>
	class bit_count_extended<low_digit2, highDigits2...>
	{
	public:
		static constexpr size_t value = fixed_integer_native_const<false, (sizeof(ulongest)*8), low_digit2>::const_bit_count
			+ bit_count_extended<highDigits2...>::value;
	};


	template <ulongest... highDigits2>
	class bit_scan_forward_extended;

	template <ulongest low_digit2>
	class bit_scan_forward_extended<low_digit2>
	{
	public:
		static constexpr size_t value = fixed_integer_native_const<false, (sizeof(ulongest) * 8), low_digit2>::const_bit_scan_forward;
	};

	template <ulongest low_digit2, ulongest... highDigits2>
	class bit_scan_forward_extended<low_digit2, highDigits2...>
	{
	public:
		static constexpr size_t tmp = fixed_integer_native_const<false, (sizeof(ulongest) * 8), low_digit2>::const_bit_scan_forward;
		static constexpr size_t value = !!tmp ? tmp : ((sizeof(ulongest) * 8) + bit_scan_forward_extended<highDigits2...>::value);
	};


	template <ulongest... highDigits2>
	class bit_scan_reverse_extended;

	template <ulongest low_digit2>
	class bit_scan_reverse_extended<low_digit2>
	{
	public:
		static constexpr bool is_zero = !low_digit2;
		static constexpr size_t value = fixed_integer_native_const<false, (sizeof(ulongest) * 8), low_digit2>::const_bit_scan_reverse;
	};

	template <ulongest low_digit2, ulongest... highDigits2>
	class bit_scan_reverse_extended<low_digit2, highDigits2...>
	{
	public:
		static constexpr bool is_zero = !low_digit2 && bit_scan_reverse_extended<highDigits2...>::is_zero;

		static constexpr size_t value = bit_scan_reverse_extended<highDigits2...>::is_zero 
			? bit_scan_reverse_extended<low_digit2>::value
			: (bit_scan_reverse_extended<highDigits2...>::value + (sizeof(ulongest) * 8));
	};

public:
	static constexpr size_t const_bit_count = bit_count_extended<low_digit, highDigits...>::value;
	static constexpr size_t const_bit_scan_forward = bit_scan_forward_extended<low_digit, highDigits...>::value;
	static constexpr size_t const_bit_scan_reverse = bit_scan_reverse_extended<low_digit, highDigits...>::value;


	typedef fixed_integer<is_const_negative, reduced_t::bits> non_const_t;

	fixed_integer_extended_const() { }

	fixed_integer_extended_const(const this_t&) { }
	fixed_integer_extended_const(const volatile this_t&) { }

	this_t& operator=(const volatile this_t&) const { }
	volatile this_t& operator=(const volatile this_t&) const volatile { }

	constexpr ulongest get_int() const { return low_digit; }
	operator ulongest() const volatile { return low_digit; }

	this_t simplify_type() const volatile { return this_t(); }

	constexpr ulongest get_digit(size_t i) const
	{
		if (i == 0)
			return low_digit;
		if (i >= n_digits)
			return 0;
		return fixed_integer_extended_const<has_sign, bits, highDigits...>::get_digit(i - 1)
	}

	constexpr bool operator!() const volatile { return is_const_zero; }

	template <typename enable = void>	// delays expansion until called
	auto operator~() const volatile
	{
		typename fixed_integer_extended_const<has_sign, bits, ~values...>::reduced_t tmp;
		return tmp;
	}

	constexpr bool is_negative() const volatile { return is_const_negative; }

	static constexpr bool is_const_exponent_of_two = is_const_negative
		? negative_t::is_const_exponent_of_two
		: (!low_digit_in
			? fixed_integer_extended_const<has_sign_in, bits_in, highDigits...>::is_const_exponent_of_two
			: (((low_digit_in& (~low_digit_in + 1)) == low_digit_in) && fixed_integer_extended_const<has_sign_in, bits_in, highDigits...>::is_const_zero));

	constexpr bool is_exponent_of_two() const volatile { return is_const_exponent_of_two; }
	constexpr bool has_fractional_part() const volatile { return false; }

	// abs
	auto abs() const volatile
	{
		abs_t tmp;
		return tmp;
	}

	template <typename enable = void>	// delays expansion until called
	auto operator-() const volatile
	{
		negative_t tmp;
		return tmp;
	}

	constexpr size_t bit_count() const volatile { return const_bit_count; }
	
	constexpr size_t bit_scan_forward() const volatile { return const_bit_scan_forward; }

	constexpr size_t bit_scan_reverse() const volatile { return const_bit_scan_reverse; }

	auto next() const volatile { return *this + one_t(); }
	auto prev() const volatile { return *this - one_t(); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitor_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitor_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitor_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitor_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp | src; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp | src; }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp | src; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp | src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this | tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this | tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitand_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitand_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitand_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitand_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src & low_digit; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src & low_digit; }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & low_digit; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & low_digit; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this & tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this & tmp; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitxor_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename bitxor_extended<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitxor_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename bitxor_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp ^ src; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp ^ src; }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp ^ src; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp ^ src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this ^ tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this ^ tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_added<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_added<has_sign2, bits2, value2>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_added_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_added_extended<has_sign2, bits2, values2...>::type::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp + src; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp + src; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp + src; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp + src; }

	auto operator+(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp + src; }
	auto operator+(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp + src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this + tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this + tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this + -i;
	}


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp - src; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp - src; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp - src; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp - src; }

	auto operator-(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp - src; }
	auto operator-(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp - src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this - tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this - tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }

	auto inverse_subtract(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }
	auto inverse_subtract(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;
		typedef typename abs_t::as_extended_t::template get_multiplied<abs_t2::int_value>::type tmp_t;

		std::conditional_t<negate_result,
			typename tmp_t::as_extended_t::negative_t,
			tmp_t
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;
		typedef typename abs_t::as_extended_t::template get_multiplied<abs_t2::int_value>::type tmp_t;

		std::conditional_t<negate_result,
			typename tmp_t::as_extended_t::negative_t,
			tmp_t
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_extended_const<has_sign2, bits2, values2...>::is_const_negative;
		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type tmp_t;

		std::conditional_t<negate_result, typename tmp_t::as_extended_t::negative_t, tmp_t> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_extended_const<has_sign2, bits2, values2...>::is_const_negative;
		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type tmp_t;

		std::conditional_t<negate_result, typename tmp_t::as_extended_t::negative_t, tmp_t> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp * src; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp * src; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp * src; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp * src; }

	auto operator*(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp * src; }
	auto operator*(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp * src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this * tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this * tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_modulo<fixed_integer_native_const<has_sign2, bits2, value2> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_modulo<fixed_integer_native_const<has_sign2, bits2, value2> >::type tmp;
		return tmp;
	}


	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_modulo<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_modulo<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp % src; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp % src; }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp % src; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp % src; }

	auto operator%(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp % src; }
	auto operator%(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp % src; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this % tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this % tmp; }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }

	auto inverse_modulo(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }
	auto inverse_modulo(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{ return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(); }
	
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{ return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(); }


	template <bool has_sign2, size_t bits2>
	auto operator/(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2>
	auto operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2>
	auto operator/(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2> 
	auto operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	auto operator/(const dynamic_integer& src) const volatile
	{ return fraction<this_t, dynamic_integer>(*this, src); }

	auto operator/(const volatile dynamic_integer& src) const volatile
	{ return fraction<this_t, dynamic_integer>(*this, src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }

	// reciprocal()
	auto reciprocal() const volatile { return fraction<one_t, this_t>(); }

	// inverse_divide
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>();
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>();
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>();
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>();
	}


	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this);
	}

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this);
	}

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this);
	}

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this);
	}

	auto inverse_divide(const dynamic_integer& src) const volatile
	{
		return fraction<dynamic_integer, this_t>(src, *this);
	}

	auto inverse_divide(const volatile dynamic_integer& src) const volatile
	{
		return fraction<dynamic_integer, this_t>(src, *this);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }

	// floor
	this_t floor() const volatile { return this_t(); }

	// ceil
	this_t ceil() const volatile { return this_t(); }

	// fractional_part
	auto fractional_part() const volatile { return zero_t(); }



	// divide_whole

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return divide_whole(tmp);
	}


	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_divide_whole2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_divide_whole2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }

	auto divide_whole(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }
	auto divide_whole(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }


	// inverse_divide_whole
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i.divide_whole(*this);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i.divide_whole(*this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i.divide_whole(*this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i.divide_whole(*this);
	}

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }

	auto inverse_divide_whole(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

	auto divide_whole_and_modulo(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return gcd(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return gcd(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;
		typename get_gcd2<abs_t::as_extended_t, abs_t2>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;
		typename get_gcd2<abs_t::as_extended_t, abs_t2>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	auto gcd(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }

	template <bool has_sign2, size_t bits2>
	auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }

	template <bool has_sign2, size_t bits2>
	auto gcd(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }

	template <bool has_sign2, size_t bits2>
	auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }

	auto gcd(const dynamic_integer& t2) const volatile { return t2.gcd(*this); }

	auto gcd(const volatile dynamic_integer& t2) const volatile { return t2.gcd(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lcm(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lcm(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type mul_t;

		typedef typename get_gcd2<abs_t::as_extended_t, abs_t2>::type gcd_t;

		typename get_divide_whole<typename mul_t::as_extended_t, typename gcd_t::as_extended_t>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type mul_t;

		typedef typename get_gcd2<abs_t::as_extended_t, abs_t2>::type gcd_t;

		typename get_divide_whole<typename mul_t::as_extended_t, typename gcd_t::as_extended_t>::type tmp;
		return tmp;
	}


	template <bool has_sign2, size_t bits2>
	auto lcm(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }

	template <bool has_sign2, size_t bits2>
	auto lcm(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }

	template <bool has_sign2, size_t bits2>
	auto lcm(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }

	template <bool has_sign2, size_t bits2>
	auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }

	auto lcm(const dynamic_integer& t2) const volatile { return t2.lcm(*this); }

	auto lcm(const volatile dynamic_integer& t2) const volatile { return t2.lcm(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }


	// greater
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return greater(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return greater(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		static constexpr bool b = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value == 1;
		typedef std::conditional_t<b, this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> result_t;
		result_t result();
		return result;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		static constexpr bool b = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value == 1;
		typedef std::conditional_t<b, this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> result_t;
		result_t result();
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto greater(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }

	template <bool has_sign2, size_t bits2>
	auto greater(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }

	template <bool has_sign2, size_t bits2>
	auto greater(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }

	template <bool has_sign2, size_t bits2>
	auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }

	auto greater(const dynamic_integer& t2) const volatile { return t2.greater(*this); }

	auto greater(const volatile dynamic_integer& t2) const volatile { return t2.greater(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }


	// lesser
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lesser(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lesser(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		static constexpr bool b = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value == -1;
		typedef std::conditional_t<b, this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> result_t;
		result_t result();
		return result;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		static constexpr bool b = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value == -1;
		typedef std::conditional_t<b, this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> result_t;
		result_t result();
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto lesser(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }

	template <bool has_sign2, size_t bits2>
	auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }

	template <bool has_sign2, size_t bits2>
	auto lesser(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }

	template <bool has_sign2, size_t bits2>
	auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }

	auto lesser(const dynamic_integer& t2) const volatile { return t2.lesser(*this); }

	auto lesser(const volatile dynamic_integer& t2) const volatile { return t2.lesser(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }



	// equals
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) == 0;
	}


	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }

	bool operator==(const dynamic_integer& cmp) const volatile { return cmp == *this; }
	bool operator==(const volatile dynamic_integer& cmp) const volatile { return cmp == *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }

	// not_equals
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }

	bool operator!=(const dynamic_integer& cmp) const volatile { return cmp != *this; }
	bool operator!=(const volatile dynamic_integer& cmp) const volatile { return cmp != *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }

	// is_less_than
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }

	bool operator<(const dynamic_integer& cmp) const volatile { return cmp > *this; }
	bool operator<(const volatile dynamic_integer& cmp) const volatile { return cmp > *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }

	// is_greater_than
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }

	bool operator>(const dynamic_integer& cmp) const volatile { return cmp < *this; }
	bool operator>(const volatile dynamic_integer& cmp) const volatile { return cmp < *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }

	// is_less_than_or_equal
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }

	bool operator<=(const dynamic_integer& cmp) const volatile { return cmp >= *this; }
	bool operator<=(const volatile dynamic_integer& cmp) const volatile { return cmp >= *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }

	// is_greater_than_or_equal
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }

	bool operator>=(const dynamic_integer& cmp) const volatile { return cmp <= *this; }
	bool operator>=(const volatile dynamic_integer& cmp) const volatile { return cmp <= *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }

	// compare
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr int compare(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return compare(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return compare(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value;
	}

	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }

	int compare(const dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }
	int compare(const volatile dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr auto const_compare(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return const_compare(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr auto const_compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return const_compare(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr auto const_compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		static constexpr int value = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value;
		std::conditional_t<value == 0,
			zero_t,
			std::conditional_t<value == 1,
			one_t,
			minus_one_t
			>
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr auto const_compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		static constexpr int value = compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::value;
		std::conditional_t<value == 0,
			zero_t,
			std::conditional_t<value == 1,
			one_t,
			minus_one_t
			>
		> tmp;
		return tmp;
	}


	bool test_bit(size_t i) const volatile { return false; }


	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const volatile
	{
		non_const_t tmp(*this);
		return tmp.to_string_t(radix, minDigits);
	}

	string to_string(int radix = 10, size_t minDigits = 1) const volatile
	{
		return to_string_t<wchar_t>(radix, minDigits);
	}

	cstring to_cstring(int radix = 10, size_t minDigits = 1) const volatile
	{
		return to_string_t<char>(radix, minDigits);
	}

	template <endian_t e>
	io::buffer to_buffer() const volatile
	{
		non_const_t tmp(*this);
		return tmp.to_buffer();
	}

	// bit_shift_right
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative2;
		typename right_shift_extended<(is_const_negative2 ? ((size_t)-value2) : (size_t)value2)>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative2;
		typename right_shift_extended<(is_const_negative2 ? ((size_t)-value2) : (size_t)value2)>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return zero_t();
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return zero_t();
	}

	template <bool has_sign2, size_t bits2> auto operator>>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator>>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator>>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }


	// bit_shift_left
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative2;
		typename left_shift_extended<(is_const_negative2 ? ((size_t)-value2) : (size_t)value2)>::type tmp;
		return tmp;
	}
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative2;
		typename left_shift_extended<(is_const_negative2 ? ((size_t)-value2) : (size_t)value2)>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return zero_t();
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return zero_t();
	}

	template <bool has_sign2, size_t bits2> auto operator<<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator<<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator<<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
};


template <bool has_sign_in, size_t bits_in, ulongest value>	// specialization for only 1 value - not valid, only used for template meta
class fixed_integer_extended_const<has_sign_in, bits_in, value>
{
public:
	typedef fixed_integer_extended_const<has_sign_in, bits_in, value> this_t;
	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = bits_in;
	static constexpr size_t n_digits = 1;
	static constexpr ulongest int_value = value;
	static constexpr bool is_const_negative = !has_sign ? false : ((longest)value < 0);

	static constexpr bool is_const_exponent_of_two = is_const_negative ? ((-(longest)int_value & (~- (longest)int_value + 1)) == -(longest)int_value) : ((int_value & (~int_value + 1)) == int_value);

	static constexpr bool const_bit_scan_forward = fixed_integer_native_const<false, (sizeof(ulongest) * 8), value>::const_bit_scan_forward;
	static constexpr bool const_bit_scan_reverse = fixed_integer_native_const<false, (sizeof(ulongest) * 8), value>::const_bit_scan_reverse;
	static constexpr bool const_bit_count = fixed_integer_native_const<false, (sizeof(ulongest) * 8), value>::const_bit_count;

	typedef fixed_integer_extended_const<is_const_negative, bits, value> as_extended_t;

	static constexpr bool is_const_zero = value == 0;

private:
	template <bool, size_t, ulongest...>
	friend class fixed_integer_extended_const;

	template <typename, typename>
	friend class const_compared;

	static constexpr ulongest low_digit = value;
	static constexpr bool is_negative_one = !has_sign ? false : (value == (ulongest)-1);
	static constexpr bool is_high_digit_zero = is_const_zero;
	static constexpr bool is_high_digit_negative_one = is_negative_one;
	static constexpr size_t reduced_bits = is_const_negative ? range_to_bits_v<(longest)value, 0> : range_to_bits_v<0, (ulongest)value>;

	template <bool is_zero, bool overflow, ulongest... lowPart>
	class calculate_negated;

	template <ulongest... lowPart>
	class calculate_negated<false, false, lowPart...>
	{
	public:
		static constexpr ulongest new_low_digit = ~low_digit;
		typedef fixed_integer_extended_const<!is_const_negative, ((sizeof...(lowPart)+1) * (sizeof(ulongest) * 8)), lowPart..., new_low_digit> type;
	};

	template <ulongest... lowPart>
	class calculate_negated<false, true, lowPart...>
	{
	public:
		static constexpr ulongest new_low_digit = ~low_digit + 1;
		static constexpr bool new_overflow = new_low_digit == 0;

		template <bool overflow>
		class helper;

		template <>
		class helper<false>
		{
		public:
			typedef fixed_integer_extended_const<!is_const_negative, ((sizeof...(lowPart)+1) * (sizeof(ulongest) * 8)), lowPart..., new_low_digit> type;
		};

		template <>
		class helper<true>
		{
		public:
			typedef fixed_integer_extended_const<!is_const_negative, ((sizeof...(lowPart)+1) * (sizeof(ulongest) * 8)) + 1, lowPart..., new_low_digit, ~(ulongest)0> type;
		};

		typedef typename helper<new_overflow>::type type;
	};

	template <bool overflow, ulongest... lowPart>
	class calculate_negated<true, overflow, lowPart...>
	{
	public:
		typedef this_t type;
	};

	typedef bits_to_int_t<reduced_bits, is_const_negative> reduced_int_t;

public:

	typedef fixed_integer_native_const<is_const_negative, reduced_bits, (reduced_int_t)value> reduced_t;

	typedef typename calculate_negated<is_const_zero, true>::type::reduced_t negative_t;
	typedef std::conditional_t<is_const_negative, negative_t, reduced_t> abs_t;

private:
	template <ulongest... lowPart>
	class calculate_incremented
	{
	public:
		static constexpr ulongest new_low_digit = low_digit + 1;
		static constexpr bool overflow = new_low_digit == 0;
		typedef std::conditional_t<overflow && !has_sign,	// is signed, ignore overflow, value became positive
			fixed_integer_extended_const<false, bits, lowPart..., new_low_digit, (ulongest)1>,
			fixed_integer_extended_const<is_const_negative && !overflow, bits, lowPart..., new_low_digit>
		> type;
	};

	template <ulongest... lowPart>
	class calculate_negative_overflow
	{
	public:
		static constexpr bool overflow = low_digit == 0;
		static constexpr ulongest new_low_digit = low_digit - 1;

		static constexpr ulongest last_digit = (is_const_negative ? ((ulongest)-1) : 0) - (overflow ? 1 : 0);
		typedef fixed_integer_extended_const<has_sign, bits, lowPart..., new_low_digit, last_digit> type;
	};




	template <ulongest... lowPart>
	class calculate_reduced
	{
	public:
		typedef fixed_integer_extended_const<is_const_negative, bits, lowPart...> type;
	};

	template <ulongest... lowPart>
	class calculate_inversed
	{
	public:
		typedef fixed_integer_extended_const<has_sign, bits, lowPart..., ~value> type;
	};

	// Probably not needed, as there shouldn't be single-digit extended's.
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class get_added
	{
	private:
		static constexpr ulongest new_low_digit = (ulongest)low_digit + (ulongest)value2;
		static constexpr bool overflow = ((ulongest)low_digit + (ulongest)value2) < (ulongest)low_digit;
		static constexpr bool is_negative2 = value2 < 0;
		static constexpr size_t new_bits = bits + 1;
		static constexpr ulongest highDigit = (is_const_negative == is_negative2) ?
			(is_const_negative ?
			(overflow ? ~(ulongest)0 : (~(ulongest)0 - 1))
				:
				(overflow ? 1 : 0)
				)
			:
			(overflow ? 0 : ~(ulongest)0);

		static constexpr bool is_high_digit_negative = (is_const_negative == is_negative2) ? is_const_negative : !overflow;

	public:
		typedef fixed_integer_extended_const<is_high_digit_negative, new_bits, new_low_digit, highDigit> type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class get_added_extended;

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class get_added_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <bool overflow, ulongest... lowPart>
		class calculate_added_extended
		{
		public:
			typedef typename fixed_integer_extended_const<has_sign2, bits2, low_digit2, highDigits2...>::
				template get_added_extended<has_sign, bits, value>::
				template calculate_added_extended<overflow, lowPart...>::type type;
		};

		typedef typename calculate_added_extended<false>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest value2>
	class get_added_extended<has_sign2, bits2, value2>
	{
	public:
		template <bool overflow, ulongest... lowPart>
		class calculate_added_extended
		{
		public:
			static constexpr ulongest new_low_digit = low_digit + value2 + (overflow ? 1 : 0);
			static constexpr bool new_overflow = overflow ? (new_low_digit <= low_digit) : (new_low_digit < low_digit);
			static constexpr bool is_negative2 = has_sign2 && ((longest)value2 < 0);
			static constexpr ulongest high_digit = (is_const_negative == is_negative2) ?
													(is_const_negative ?
													(overflow ? ~(ulongest)0 : (~(ulongest)0 - 1))
														:
														(overflow ? 1 : 0)
														)
													:
													(overflow ? 0 : ~(ulongest)0);

			static constexpr bool is_high_digit_negative = (is_const_negative == is_negative2) ? is_const_negative : !overflow;

		public:
			typedef typename fixed_integer_extended_const<is_high_digit_negative, bits2, lowPart..., new_low_digit, high_digit>::reduced_t type;
		};

		typedef typename calculate_added_extended<false>::type type;
	};

	template <typename T>
	class get_added_extended2;

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class get_added_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		typedef typename get_added_extended<has_sign2, bits2, values2...>::type type;
	};

	template <ulongest value2>
	class get_multiplied
	{
	public:
		template <ulongest overflow, ulongest... lowPart>
		class calculate_multiplied
		{
		public:
			static constexpr ulongest new_low_digit = const_extumul<low_digit, value2>::low_part + overflow;
			static constexpr ulongest new_overflow = const_extumul<low_digit, value2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

			typedef typename fixed_integer_extended_const<false, bits, lowPart..., new_low_digit, new_overflow>::reduced_t type;
		};

		typedef typename calculate_multiplied<0>::type type;
	};


	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class get_multiplied_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class get_multiplied_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... leading_zeros>
		class calculate_multiplied_extended2
		{
		public:
			template <ulongest overflow, ulongest... lowPart>
			class calculate_multiplied_extended
			{
			public:
				static constexpr ulongest new_low_digit = const_extumul<low_digit, value2>::low_part + overflow;
				static constexpr ulongest new_overflow = const_extumul<low_digit, value2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

				typedef typename fixed_integer_extended_const<false, bits + bits2, lowPart..., new_low_digit, new_overflow>::reduced_t type;
			};

			typedef typename calculate_multiplied_extended<0, leading_zeros...>::type type;
		};

		typedef typename calculate_multiplied_extended2<>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class get_multiplied_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... leading_zeros>
		class calculate_multiplied_extended2
		{
		public:
			template <ulongest overflow, ulongest... lowPart>
			class calculate_multiplied_extended
			{
			public:
				static constexpr ulongest new_low_digit = const_extumul<low_digit, low_digit2>::low_part + overflow;
				static constexpr ulongest new_overflow = const_extumul<low_digit, low_digit2>::high_part + ((new_low_digit < overflow) ? 1 : 0);

				typedef typename get_multiplied_extended<has_sign2, bits2, highDigits2...>::
					template calculate_multiplied_extended2<leading_zeros...>::
					template calculate_multiplied_extended<new_overflow, lowPart..., new_low_digit>::type type;
			};

			typedef typename calculate_multiplied_extended<0, leading_zeros...>::type type;
		};

		typedef typename calculate_multiplied_extended2<>::type type;
	};

	template <typename T>
	class get_multiplied_extended2;

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class get_multiplied_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		typedef typename get_multiplied_extended<has_sign2, bits2, values2...>::type type;
	};


	template <typename abs_t1, typename abs_t2, typename T1, typename T2>
	class divide_guess_and_check
	{
	public:
		typedef typename T2::as_extended_t::template get_added_extended2<typename T1::as_extended_t::negative_t::as_extended_t>::type difference_t;

		typedef typename difference_t::template right_shift_extended<1>::type half_difference_t;	// divide by 2

		typedef typename T1::as_extended_t::template get_added_extended2<half_difference_t>::type::as_extended_t half_way_t;

		typedef typename abs_t2::template get_multiplied_extended2<half_way_t>::type::as_extended_t test_result_t;

		static constexpr int cmp = half_difference_t::is_const_zero ? 0 : abs_t1::template compare_extended2<test_result_t>::value;

		typedef std::conditional_t<(cmp >= 0), half_way_t, T1> new_T1;
		typedef std::conditional_t<(cmp > 0), T2, half_way_t> new_T2;

		typedef typename divide_guess_and_check<abs_t1, abs_t2, new_T1, new_T2>::type type;
	};

	template <typename abs_t1, typename abs_t2, typename T1>
	class divide_guess_and_check<abs_t1, abs_t2, T1, T1>
	{
	public:
		typedef T1 type;
	};

	template <typename abs_t1, typename abs_t2>
	class get_divide_whole
	{
	public:
		static constexpr bool is_larger_or_equal1 = abs_t1::template compare_extended2<abs_t2>::value >= 0;

		typedef std::conditional_t<is_larger_or_equal1, fixed_integer_native_const<false, 1, 1>, fixed_integer_native_const<false, 1, 0> > new1;
		typedef std::conditional_t<is_larger_or_equal1, abs_t1, fixed_integer_native_const<false, 1, 0> > new2;
		typedef typename divide_guess_and_check<abs_t1, abs_t2, new1, new2>::type type;
	};

	template <typename T2>
	class get_divide_whole2
	{
	public:
		typedef typename T2::abs_t::as_extended_t abs_t2;
		static constexpr bool is_const_negative2 = T2::is_const_negative;
		static constexpr bool negate_result = is_const_negative != is_const_negative2;
		typedef typename get_divide_whole<typename abs_t::as_extended_t, abs_t2>::type tmp_t;

		typedef std::conditional_t<negate_result, typename tmp_t::as_extended_t::negative_t, tmp_t> type;
	};


	template <typename T2>
	class get_modulo
	{
	public:
		typedef typename get_divide_whole2<typename T2::as_extended_t>::type::as_extended_t div_t;
		typedef typename div_t::template get_multiplied_extended2<typename T2::as_extended_t>::type mul_t;
		typedef typename get_added_extended2<typename mul_t::as_extended_t::negative_t::as_extended_t>::type type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class compare_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class compare_extended<has_sign2, bits2, value2>
	{
	public:
		static constexpr int value = ((value == value2) ? 0 : (value < value2 ? -1 : 1));
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class compare_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		static constexpr int value = -fixed_integer_extended_const<has_sign2, bits2, low_digit2, highDigits2...>::
			template compare_extended<has_sign, bits, value>::value;
	};

	template <typename T>
	class compare_extended2
	{
	};

	template <bool has_sign2, size_t bits2, ulongest... values2>
	class compare_extended2<fixed_integer_extended_const<has_sign2, bits2, values2...> >
	{
	public:
		static constexpr bool is_const_negative2 = fixed_integer_extended_const<has_sign2, bits2, values2...>::is_const_negative;
		static constexpr bool is_same_sign = (is_const_negative == is_const_negative2);
		static constexpr int tmp = compare_extended<has_sign2, bits2, values2...>::value;
		static constexpr int value = !is_same_sign ? (is_const_negative ? -1 : 1) : tmp;
	};



	template <typename greater_t, typename lesser_t, typename modulo_t>
	class get_gcd
	{
	public:
		typedef decltype(std::declval<lesser_t>() % std::declval<modulo_t>()) modulo_t2;
		typedef typename get_gcd<lesser_t, modulo_t, modulo_t2>::type type;
	};

	template <typename greater_t, typename lesser_t>
	class get_gcd<greater_t, lesser_t, zero_t>
	{
	public:
		typedef lesser_t type;
	};

	template <typename greater_t>
	class get_gcd<greater_t, zero_t, zero_t>
	{
	public:
		typedef greater_t type;
	};

	template <typename abs_t1, typename abs_t2>
	class get_gcd2
	{
	public:
		static constexpr bool is_first_greater = abs_t1::template compare_extended2<abs_t2>::value > 0;
		typedef std::conditional_t<is_first_greater, abs_t1, abs_t2> greater_t;
		typedef std::conditional_t<is_first_greater, abs_t2, abs_t1> lesser_t;

		// Need to special case out zero, to avoid divide by 0 errors.
		typedef std::conditional_t<lesser_t::is_const_zero, one_t, lesser_t> lesser_t3;


		typedef typename greater_t::template get_modulo<lesser_t3>::type::as_extended_t modulo_t;

		typedef typename get_gcd<greater_t, lesser_t, modulo_t>::type type;
	};



	template <size_t n, ulongest... lowDigits>
	class left_shift_extended
	{
	};

	template <size_t n, ulongest highestNewLowDigit, ulongest... lowDigits>
	class left_shift_extended<n, highestNewLowDigit, lowDigits...>
	{
	public:
		static constexpr ulongest new_low_digit = highestNewLowDigit | (value << n);
		static constexpr ulongest low_overflow = (value >> ((sizeof(ulongest) * 8) - n));

		typedef fixed_integer_extended_const<has_sign, bits, lowDigits..., new_low_digit, low_overflow> type;
	};

	template <size_t n>
	class left_shift_extended<n>
	{
	public:
		static constexpr bool is_whole_digit_shift = (n / (sizeof(ulongest) * 8)) > 0;
		static constexpr size_t remaining_shift_n = is_whole_digit_shift ? (n - (sizeof(ulongest) * 8)) : 0;

		typedef typename fixed_integer_extended_const<has_sign, bits, 0, value>::
			template left_shift_extended<remaining_shift_n>::type whole_shifted_t;

		static constexpr size_t n2 = is_whole_digit_shift ? ((sizeof(ulongest) * 8) / 2) : n;
		typedef typename left_shift_extended<n2, 0>::type each_shifted_t;

		typedef std::conditional_t<is_whole_digit_shift, whole_shifted_t, each_shifted_t> type;
	};

	template <>
	class left_shift_extended<0>
	{
	public:
		typedef fixed_integer_extended_const<has_sign, bits, value> type;
	};

	template <size_t n, ulongest... lowDigits>
	class right_shift_extended
	{
	};

	template <size_t n, ulongest highestNewLowPart, ulongest... lowDigits>
	class right_shift_extended<n, highestNewLowPart, lowDigits...>
	{
	public:
		static constexpr ulongest new_low_digit = highestNewLowPart | (value << ((sizeof(ulongest) * 8) - n));
		static constexpr ulongest low_overflow = (value >> n);

		typedef fixed_integer_extended_const<has_sign, bits, lowDigits..., new_low_digit, low_overflow> type;
	};

	template <size_t n>
	class right_shift_extended<n>
	{
	public:
		static constexpr bool is_whole_digit_shift = (n / (sizeof(ulongest) * 8)) > 0;
		static constexpr size_t remaining_shift_n = is_whole_digit_shift ? (n - (sizeof(ulongest) * 8)) : 0;

		typedef fixed_integer_native_const<has_sign, 1, 0> whole_shifted_t;

		static constexpr ulongest low_part = (value >> n);

		typedef fixed_integer_extended_const<has_sign, bits, low_part> each_shifted_t;

		typedef std::conditional_t<is_whole_digit_shift, whole_shifted_t, each_shifted_t> type;
	};

	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitor_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitor_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitor_extended
		{
		public:
			typedef fixed_integer_extended_const<
				(bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2),
				(bits > bits2) ? bits : bits2,
				lowPart...,
				value | value2> type;
		};

		typedef typename calculate_bitor_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitor_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitor_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign2, bits2, lowPart..., low_digit | low_digit2, highDigits2...> type;
		};

		typedef typename calculate_bitor_extended<>::type  type;
	};


	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitand_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitand_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitand_extended
		{
		public:
			typedef fixed_integer_extended_const<
				has_sign && has_sign2,
				(bits < bits2) ? bits : bits2,
				lowPart...,
				value & value2> type;
		};

		typedef typename calculate_bitand_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitand_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitand_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign, bits, lowPart..., low_digit & low_digit2> type;
		};

		typedef typename calculate_bitand_extended<>::type  type;
	};


	template <bool has_sign2, size_t bits2, ulongest... highDigits2>
	class bitxor_extended;

	template <bool has_sign2, size_t bits2, ulongest value2>
	class bitxor_extended<has_sign2, bits2, value2>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitxor_extended
		{
		public:
			typedef fixed_integer_extended_const<
				(bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2),
				(bits > bits2) ? bits : bits2,
				lowPart...,
				value ^ value2> type;
		};

		typedef typename calculate_bitxor_extended<>::type  type;
	};

	template <bool has_sign2, size_t bits2, ulongest low_digit2, ulongest... highDigits2>
	class bitxor_extended<has_sign2, bits2, low_digit2, highDigits2...>
	{
	public:
		template <ulongest... lowPart>
		class calculate_bitxor_extended
		{
		public:
			typedef fixed_integer_extended_const<has_sign2, bits2, lowPart..., low_digit ^ low_digit2, highDigits2...> type;
		};

		typedef typename calculate_bitxor_extended<>::type  type;
	};

public:

	typedef fixed_integer_native<is_const_negative, reduced_bits> non_const_t;

	constexpr ulongest get_digit(size_t i) const volatile { return (i == 0) : low_digit : 0; }

	constexpr bool is_exponent_of_two() const volatile { return is_const_exponent_of_two; }

	constexpr size_t bit_scan_forward() const volatile { return const_bit_scan_forward; }

	constexpr size_t bit_scan_reverse() const volatile { return const_bit_scan_reverse; }

	constexpr size_t bit_count() const volatile { return const_bit_count; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_divide_whole2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_divide_whole2<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;
		typedef typename abs_t::as_extended_t::template get_multiplied<abs_t2::int_value>::type tmp_t;

		std::conditional_t<negate_result,
			typename tmp_t::as_extended_t::negative_t,
			tmp_t
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;

		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;
		typedef typename abs_t::as_extended_t::template get_multiplied<abs_t2::int_value>::type tmp_t;

		std::conditional_t<negate_result,
			typename tmp_t::as_extended_t::negative_t,
			tmp_t
		> tmp;
		return tmp;
	}



	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_modulo<fixed_integer_native_const<has_sign2, bits2, value2> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename get_modulo<fixed_integer_native_const<has_sign2, bits2, value2> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_modulo<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename get_modulo<fixed_integer_extended_const<has_sign2, bits2, values2...> >::type tmp;
		return tmp;
	}




	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return gcd(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return gcd(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;
		typename get_gcd2<abs_t::as_extended_t, abs_t2>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;
		typename get_gcd2<abs_t::as_extended_t, abs_t2>::type tmp;
		return tmp;
	}


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lcm(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp;
		return lcm(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type mul_t;

		typedef typename get_gcd2<abs_t::as_extended_t, abs_t2>::type gcd_t;

		typename get_divide_whole<typename mul_t::as_extended_t, typename gcd_t::as_extended_t>::type tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typedef typename fixed_integer_extended_const<has_sign2, bits2, values2...>::abs_t::as_extended_t abs_t2;

		typedef typename abs_t::template get_multiplied_extended2<abs_t2>::type mul_t;

		typedef typename get_gcd2<abs_t::as_extended_t, abs_t2>::type gcd_t;

		typename get_divide_whole<typename mul_t::as_extended_t, typename gcd_t::as_extended_t>::type tmp;
		return tmp;
	}

};


template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class const_compared<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	static constexpr int value = const_compared<fixed_integer_extended_const<has_sign1, bits1, values1...>,
		fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t>::value;
};

template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2, ulongest... values2>
class const_compared<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
private:
	static constexpr bool is_negative1 = fixed_integer_extended_const<has_sign1, bits1, values1...>::is_const_negative;
	static constexpr bool is_negative2 = fixed_integer_extended_const<has_sign2, bits2, values2...>::is_const_negative;
	static constexpr bool is_same_sign = (is_negative1 == is_negative2);
	static constexpr int tmp = fixed_integer_extended_const<has_sign1, bits1, values1...>::
		template compare_extended<has_sign2, bits2, values2...>::value;

public:
	static constexpr int value = !is_same_sign ? (is_negative1 ? -1 : 1) : tmp;
};



template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2, ulongest... values2>
class compatible<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2),
		(bits1 > bits2) ?
		(bits1 + ((has_sign2 && !has_sign1) ? 1 : 0))
		: (bits2 + ((has_sign1 && !has_sign2) ? 1 : 0))
		> type;
};

template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2),
		(bits1 + ((has_sign2 && !has_sign1) ? 1 : 0))
	> type;
};

template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_native<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), bits1 + ((has_sign1 && !has_sign2) ? 1 : 0)> type;
};

template <bool has_sign1, size_t bits1, ulongest... values1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_extended_const<has_sign1, bits1, values1...>, fixed_integer_extended<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), (bits1 > bits2 ? bits1 : bits2) + (((has_sign1 != has_sign2) && ((!has_sign1 && (bits1 >= bits2)) || (!has_sign2 && (bits2 >= bits1)))) ? 1 : 0)> type;
};

template <typename int_t2, bool has_sign2, size_t bits2, ulongest... values2>
class compatible<fixed_integer_extended_const<has_sign2, bits2, values2...>, int_t2, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<fixed_integer_extended_const<has_sign2, bits2, values2...>, int_to_fixed_integer_t<int_t2> >::type type;
};

template <typename int_t2, bool has_sign2, size_t bits2, ulongest... values2>
class compatible<int_t2, fixed_integer_extended_const<has_sign2, bits2, values2...>, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<int_to_fixed_integer_t<int_t2>, fixed_integer_extended_const<has_sign2, bits2, values2...> >::type type;
};

template <bool has_sign, size_t bits, ulongest... values>
class compatible<fixed_integer_extended_const<has_sign, bits, values...>, dynamic_integer>
{
public:
	typedef dynamic_integer type;
};





#pragma warning(pop)


}


#endif
