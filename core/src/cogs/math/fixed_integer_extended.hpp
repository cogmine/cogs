//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXEDINT_EXTENDED
#define COGS_HEADER_MATH_FIXEDINT_EXTENDED

#include <type_traits>
#include <array>

#include "cogs/operators.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/env.hpp"
#include "cogs/env/math/umul.hpp"
#include "cogs/io/buffer.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/is_integer_type.hpp"
#include "cogs/math/is_arithmetic_type.hpp"
#include "cogs/math/is_signed_type.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/sync/versioned_ptr.hpp"
#include "cogs/math/fixed_integer_native.hpp"

namespace cogs {


template <bool has_sign, size_t bits>
class fixed_integer_extended;


template <bool has_sign, size_t bits> struct is_arithmetic_type<fixed_integer_extended<has_sign, bits> > : public std::true_type { };

template <bool has_sign, size_t bits> struct is_integer_type<fixed_integer_extended<has_sign, bits> > : public std::true_type { };

template <bool has_sign, size_t bits> struct is_signed_type<fixed_integer_extended<has_sign, bits> > { static constexpr bool value = has_sign; };


template <bool has_sign, size_t bits>
class int_to_fixed_integer<fixed_integer_extended<has_sign, bits> >
{
public:
	typedef fixed_integer_extended<has_sign, bits> type;
};


template <bool has_sign_in, size_t n_bits_in>
class fixed_integer_extended_content
{
public:
	typedef fixed_integer_extended_content<has_sign_in, n_bits_in> this_t;

	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = n_bits_in; // should never be <= sizeof(longest)
	static constexpr size_t n_digits = (bits / (sizeof(longest) * 8)) + ((bits % (sizeof(longest) * 8) == 0) ? 0 : 1); // will be >0

	static constexpr size_t bits_used = (sizeof(ulongest) * 8) * n_digits;
#pragma warning(push)
#pragma warning(disable: 4310) // cast truncates constant value
	typedef fixed_integer_native_const<false, (const_bit_scan_reverse_v<bits_used>+1), (bits_to_int_t<(const_bit_scan_reverse_v<bits_used>+1), false>)bits_used> bits_used_t;
#pragma warning(pop)

private:
	static_assert(bits > (sizeof(longest) * 8));
	static_assert(n_digits > 0);

	template <bool, size_t>
	friend class fixed_integer_extended;

	template <bool, size_t>
	friend class fixed_integer_extended_content;

public:
	// index 0 is LSB, index (n_digits-1) is MSB
	// This is the same order used by dynamic_integer
	std::array<ulongest, n_digits> m_digits;

	fixed_integer_extended_content() { }

	template <bool has_sign2, size_t bits2>
	fixed_integer_extended_content(const fixed_integer_native<has_sign2, bits2>& src)
	{
		operator=(src);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	fixed_integer_extended_content(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		operator=(src);
	}

	template <bool has_sign2, size_t bits2>
	fixed_integer_extended_content(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		operator=(src);
	}

	fixed_integer_extended_content(const this_t& src)
	{
		operator=(src);
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const fixed_integer_native<has_sign2, bits2>& src)
	{
		m_digits[0] = (ulongest)src.get_int();
		set_sign_extension(src.is_negative(), 1);
		return *this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	this_t& operator=(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		m_digits[0] = (ulongest)value2;
		set_sign_extension(src.is_negative(), 1);
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		if constexpr (n_digits <= fixed_integer_extended<has_sign2, bits2>::n_digits)
		{
			for (size_t i = 0; i < n_digits; i++)
				m_digits[i] = src.m_digits[i];
		}
		else // if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
		{
			for (size_t i = 0; i < fixed_integer_extended<has_sign2, bits2>::n_digits; i++)
				m_digits[i] = src.m_digits[i];
			set_sign_extension(src.is_negative(), fixed_integer_extended<has_sign2, bits2>::n_digits);
		}
		return *this;
	}

	this_t& operator=(const dynamic_integer_content& src);
	//{
	//	if (src.is_negative())
	//		assign_negative(src.get_const_ptr(), src.get_length());
	//	else
	//		assign(src.get_const_ptr(), src.get_length());
	//	return *this;
	//}

	this_t& operator=(const dynamic_integer& src);
	//{
	//	return operator=(*(src.m_contents));
	//}

	this_t& operator=(const this_t& src)
	{
		m_digits = src.m_digits;
		return *this;
	}

	void assign_unsigned(const ulongest* src, size_t n)
	{
		if (n_digits <= n)
		{
			for (size_t i = 0; i < n_digits; i++)
				m_digits[i] = src[i];
		}
		else // if (n_digits > n)
		{
			for (size_t i = 0; i < n; i++)
				m_digits[i] = src[i];
			set_sign_extension(false, n);
		}
	}

	ulongest* get_digits() { return &(m_digits[0]); }
	const ulongest* get_digits() const { return &(m_digits[0]); }

	ulongest& get_digit(size_t i) { return m_digits[i]; }
	const ulongest& get_digit(size_t i) const { return m_digits[i]; }

	void set_digit(size_t i, const ulongest& src) { m_digits[i] = src; }

	void clear()
	{
		for (size_t i = 0; i < n_digits; i++)
			m_digits[i] = 0;
	}

	bool operator!() const
	{
		for (size_t i = 0; i < n_digits; i++)
		{
			if (m_digits[i] != 0)
				return false;
		}
		return true;
	}

	void assign_bit_not(const this_t& src)
	{
		for (size_t i = 0; i < n_digits; i++)
			m_digits[i] = ~src.m_digits[i];
	}

	bool is_negative() const
	{
		return (has_sign) ? ((longest)(m_digits[n_digits - 1]) < 0) : false;
	}

	bool is_exponent_of_two() const
	{
		if (is_negative())
		{
			fixed_integer_extended_content<false, bits> abs1;
			abs1.assign_abs(*this);
			return abs1.is_exponent_of_two();
		}

		for (size_t i = 0; i < n_digits; i++)
		{
			ulongest tmp = m_digits[i];
			if (tmp != 0)
			{
				if (!cogs::is_exponent_of_two(tmp))
					return false;
				++i;
				for (; i < n_digits; i++) // ensure the rest are zeros
				{
					if (m_digits[i] != 0)
						return false;
				}
				return true;
			}
		}

		return false;
	}

	bool assign_abs()
	{
		if (is_negative())
		{
			assign_negative();
			return true;

		}
		return false;
	}

	template <bool has_sign2, size_t bits2>
	void assign_abs(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		if (src.is_negative())
			assign_negative(src);
		else
			*this = src;
	}

	template <bool has_sign2, size_t bits2>
	void assign_abs(const fixed_integer_native<has_sign2, bits2>& src)
	{
		if (src.is_negative())
			assign_negative(src);
		else
			*this = src;
	}

	void assign_negative(const ulongest* src, size_t n)
	{
		const size_t lesserSize = (n_digits < n) ? n_digits : n;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] = ~(src[i]);
		if (n_digits > n)
			set_sign_extension(true, n);
		increment();
	}

	template <bool has_sign2, size_t bits2>
	void assign_negative(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		const size_t lesserSize = (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits) ? n_digits : fixed_integer_extended<has_sign2, bits2>::n_digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] = ~src.m_digits[i];
		if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
			set_sign_extension(!src.is_negative(), i);
		increment();
	}

	template <bool has_sign2, size_t bits2>
	void assign_negative(const fixed_integer_native<has_sign2, bits2>& src)
	{
		m_digits[0] = (ulongest)-(longest)(src.get_int());
		set_sign_extension(!src.is_negative(), 1);
	}

	void assign_negative()
	{
		for (size_t i = 0; i < n_digits; i++)
			m_digits[i] = ~m_digits[i];
		increment();
	}

	size_t bit_count() const
	{
		size_t result = 0;
		for (int i = 0; i < n_digits; i++)
			result += cogs::bit_count(m_digits[i]);
		return result;
	}

	void assign_bit_count()
	{
		m_digits[0] = bit_count();
		set_sign_extension(false, 1);
	}

	size_t bit_scan_forward() const
	{
		for (size_t i = 0; i < n_digits; i++)
		{
			ulongest digit = m_digits[0];
			if (!!digit)
				return (i * (sizeof(ulongest) * 8)) + cogs::bit_scan_forward(digit);
		}
		return 0;
	}

	void assign_bit_scan_forward()
	{
		m_digits[0] = bit_scan_forward();
		set_sign_extension(false, 1);
	}

	size_t bit_scan_reverse() const
	{
		size_t i = n_digits;
		do {
			--i;
			ulongest digit = m_digits[i];
			if (!!digit)
				return (i * (sizeof(ulongest) * 8)) + cogs::bit_scan_reverse(digit);
		} while (i != 0);
		return 0;
	}

	void assign_bit_scan_reverse()
	{
		m_digits[0] = bit_scan_reverse();
		set_sign_extension(false, 1);
	}

	void set_sign_extension(bool negative, size_t startIndex)
	{
		if (negative)
		{
			for (size_t i = startIndex; i < n_digits; i++)
				m_digits[i] = ~(ulongest)0;
		}
		else
		{
			for (size_t i = startIndex; i < n_digits; i++)
				m_digits[i] = 0;
		}
	}

	bool test_sign_extension(bool negative, size_t startIndex) const
	{
		ulongest tmp = negative ? ~(ulongest)0 : 0;
		for (size_t i = startIndex; i < n_digits; i++)
		{
			if (m_digits[i] != tmp)
				return false;
		}
		return true;
	}

	void set_bit(size_t i)
	{
		size_t digitIndex = i / (sizeof(ulongest) * 8);
		size_t bitIndex = i % (sizeof(ulongest) * 8);
		m_digits[digitIndex] |= (ulongest)1 << bitIndex;
	}

	void set_bit(size_t i, bool b)
	{
		size_t digitIndex = i / (sizeof(ulongest) * 8);
		size_t bitIndex = i % (sizeof(ulongest) * 8);
		if (b)
			m_digits[digitIndex] |= (ulongest)1 << bitIndex;
		else
			m_digits[digitIndex] &= ~((ulongest)1 << bitIndex);
	}

	void reset_bit(size_t i)
	{
		size_t digitIndex = i / (sizeof(ulongest) * 8);
		size_t bitIndex = i % (sizeof(ulongest) * 8);
		m_digits[digitIndex] &= ~((ulongest)1 << bitIndex);
	}

	void invert_bit(size_t i)
	{
		size_t digitIndex = i / (sizeof(ulongest) * 8);
		size_t bitIndex = i % (sizeof(ulongest) * 8);
		m_digits[digitIndex] ^= (ulongest)1 << bitIndex;
	}

	bool test_bit(size_t i) const
	{
		size_t digitIndex = i / (sizeof(ulongest) * 8);
		size_t bitIndex = i % (sizeof(ulongest) * 8);
		return m_digits[digitIndex] & (ulongest)1 << bitIndex;
	}

	template <bool has_sign2, size_t bits2>
	void copy_from(const fixed_integer_extended_content<has_sign2, bits2>& src, size_t startIndex = 0)
	{
		static constexpr bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		static constexpr size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		size_t i = startIndex;
		for (; i < lesserSize; i++)
			m_digits[i] = src.m_digits[i];
		if (thisIsLonger)
		{
			if (src.is_negative())
				for (; i < n_digits; i++)
					m_digits[i] = 0;
			else
				for (; i < n_digits; i++)
					m_digits[i] = ~(ulongest)0;
		}
	}

	void increment_from(size_t index)
	{
		size_t i = index;
		do {
			if (++(m_digits[i]) != 0)
				break;
		} while (++i < n_digits);
	}

	template <bool has_sign2, size_t bits2>
	void increment_copy(const fixed_integer_extended_content<has_sign2, bits2>& src, size_t startIndex = 0, bool overflow = true)
	{
		static constexpr bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		static constexpr size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		COGS_ASSERT(startIndex <= lesserSize);
		size_t i = startIndex;
		if (overflow)
		{
			for (;;)
			{
				if (i == lesserSize)
				{
					if (thisIsLonger)
					{
						if (!src.is_negative())
							m_digits[i++] = 1;
						for (; i < n_digits; i++)
							m_digits[i] = 0;
					}
					return;
				}

				ulongest origDigit = src.m_digits[i];
				if (origDigit != ~(ulongest)0)
				{
					m_digits[i++] = origDigit + 1;
					break;
				}

				m_digits[i++] = 0;
				//continue;
			}
		}

		copy_from(src, i);
	}

	void increment()
	{
		increment_from(0);
	}

	void decrement_from(size_t index)
	{
		size_t i = index;
		do {
			if ((m_digits[i])-- != 0)
				break;
		} while (++i < n_digits);
	}

	template <bool has_sign2, size_t bits2>
	void decrement_copy(const fixed_integer_extended_content<has_sign2, bits2>& src, size_t startIndex = 0, bool overflow = true)
	{
		static constexpr bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		static constexpr size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		COGS_ASSERT(startIndex <= lesserSize);
		size_t i = startIndex;
		if (overflow)
		{
			for (;;)
			{
				if (i == lesserSize)
				{
					if (thisIsLonger)
					{
						if (src.is_negative())
							m_digits[i++] = ~(ulongest)0 - 1; // -1 - 1
						for (; i < n_digits; i++)
							m_digits[i] = ~(ulongest)0;
					}
					return;
				}

				ulongest origDigit = src.m_digits[i];
				if (origDigit != 0)
				{
					m_digits[i++] = origDigit - 1;
					break;
				}

				m_digits[i++] = ~(ulongest)0;
				//continue;
			}
		}

		copy_from(src, i);
	}

	void decrement()
	{
		decrement_from(0);
	}

	// src must not overlap this
	void set_to_right_rotated(const this_t& src, size_t n) const
	{
		n %= (sizeof(ulongest) * 8 * n_digits);
		assign_bit_rotate_left(src, (sizeof(ulongest) * 8 * n_digits) - n);
	}

	// src must not overlap this
	void set_to_left_rotated(const this_t& src, size_t n) const
	{
		n %= (sizeof(ulongest) * 8 * n_digits);
		set_to_left_rotated_inner(src, n);
	}

	// src must not overlap this
	void set_to_left_rotated_inner(const this_t& src, size_t n) const
	{
		size_t moveWholeDigits = (n / (sizeof(ulongest) * 8));
		size_t shift = (n % (sizeof(ulongest) * 8));
		if (shift > 0)
		{
			if (moveWholeDigits > 0)
			{
				ulongest prevValue = src.m_digits[n_digits - 1];
				int i = 0;
				for (; i < moveWholeDigits; i++)
				{
					ulongest srcDigit = src.m_digits[i];
					m_digits[i + moveWholeDigits] = (srcDigit << shift) | (prevValue >> ((sizeof(ulongest) * 8) - shift));
					prevValue = srcDigit;
				}
				for (; i < (sizeof(ulongest) * 8 * n_digits); i++)
				{
					ulongest srcDigit = src.m_digits[i];
					m_digits[i - moveWholeDigits] = (srcDigit << shift) | (prevValue >> ((sizeof(ulongest) * 8) - shift));
					prevValue = srcDigit;
				}
			}
			else
			{
				ulongest prevValue = src.m_digits[n_digits - 1];
				int i = 0;
				for (; i < (sizeof(ulongest) * 8 * n_digits); i++)
				{
					ulongest srcDigit = src.m_digits[i];
					m_digits[i] = (srcDigit << shift) | (prevValue >> ((sizeof(ulongest) * 8) - shift));
					prevValue = srcDigit;
				}
			}
		}
		else if (moveWholeDigits > 0)
		{
			int i = 0;
			for (; i < moveWholeDigits; i++)
				m_digits[i + moveWholeDigits] = src.m_digits[i];
			for (; i < (sizeof(ulongest) * 8 * n_digits); i++)
				m_digits[i - moveWholeDigits] = src.m_digits[i];
		}
	}

	// src may overlap this
	void set_to_left_shifted(const this_t& src, size_t n)
	{
		if (!n)
			return;
		bool exceeded = (n >= bits);
		size_t skippedDigits = exceeded ? n_digits : (n / (sizeof(ulongest) * 8)); // Will never be > n_digits
		if (!exceeded)
		{
			size_t bitsPerDigitShift = n % (sizeof(ulongest) * 8);
			if (!bitsPerDigitShift)
			{
				// use more efficient algorithm if shift is digit-aligned
				// skippedDigits will be >0
				// Need to copy in reverse, in case the destination is also the source
				for (size_t i = n_digits - 1; i >= skippedDigits; i--)
					m_digits[i] = src.m_digits[i - skippedDigits];
			}
			else
			{ // skippedDigits will be < n_digits
				size_t lowToHighShift = (sizeof(ulongest) * 8) - bitsPerDigitShift;
				ulongest carryOver = 0;
				// Need to copy in reverse, in case the destination is also the source
				for (size_t i = n_digits - 1; i > skippedDigits; i--)
				{
					ulongest srcValue = src.m_digits[i - skippedDigits];
					m_digits[i] = (srcValue >> lowToHighShift) | carryOver;
					carryOver = srcValue << bitsPerDigitShift;
				}
				m_digits[skippedDigits] = carryOver;
			}
		}
		for (size_t i = 0; i < skippedDigits; i++)
			m_digits[i] = 0;
	}

	// src may overlap this
	void set_to_right_shifted(const this_t& src, size_t n)
	{
		if (!n)
			return;
		bool exceeded = (n >= bits);
		size_t skippedDigits = exceeded ? n_digits : (n / (sizeof(ulongest) * 8)); // Will never be > n_digits
		if (!exceeded)
		{
			size_t endIndex = n_digits - skippedDigits;
			size_t bitsPerDigitShift = n % (sizeof(ulongest) * 8);
			if (!bitsPerDigitShift)
			{
				// use more efficient algorithm if shift is digit-aligned
				// skippedDigits will be >0
				for (size_t i = 0; i < endIndex; i++)
					m_digits[i] = src.m_digits[i + skippedDigits];
			}
			else
			{ // skippedDigits will be <n_digits
				size_t i = 0;
				ulongest srcValue = src.m_digits[skippedDigits];
				size_t bitsPerDigitShiftBack = (sizeof(ulongest) * 8) - bitsPerDigitShift;
				for (;;)
				{
					m_digits[i] = srcValue >> bitsPerDigitShift;
					size_t nextIndex = i + 1;
					if (nextIndex == endIndex)
						break;
					ulongest srcValueNext = src.m_digits[skippedDigits + nextIndex];
					m_digits[i] |= srcValueNext << bitsPerDigitShiftBack;
					i = nextIndex;
					srcValue = srcValueNext;
				}
				if (src.is_negative())
					m_digits[i] |= (~(ulongest)0 << bitsPerDigitShiftBack);
			}
		}
		if (skippedDigits > 0)
			set_sign_extension(src.is_negative(), n_digits - skippedDigits);
	}


	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void assign_bit_or(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// n_digits will be the larger size.
		const size_t src1Digits = fixed_integer_extended<has_sign2, bits2>::n_digits;
		const size_t src2Digits = fixed_integer_extended<has_sign3, bits3>::n_digits;
		const size_t lesserSize = (src2Digits > src1Digits) ? src1Digits : src2Digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] = src1.m_digits[i] | src2.m_digits[i];

		if (src1Digits < src2Digits)
		{
			if (src1.is_negative())
				set_sign_extension(true, i);
			else
				for (; i < src2Digits; i++)
					m_digits[i] = src2.m_digits[i];
		}
		else if (src1Digits > src2Digits)
		{
			if (src2.is_negative())
				set_sign_extension(true, i);
			else
				for (; i < src1Digits; i++)
					m_digits[i] = src1.m_digits[i];
		}
	}

	template <bool has_sign2, size_t bits2>
	void assign_bit_or(const this_t& src1, const fixed_integer_native<has_sign2, bits2>& src2)
	{
		m_digits[0] = src1.m_digits[0] | src2.get_int();
		if (src2.is_negative())
			set_sign_extension(true, 1);
		else
			for (size_t i = 1; i < n_digits; i++)
				m_digits[i] = src1.m_digits[i];
	}

	template <bool has_sign2, size_t bits2>
	void assign_bit_or(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		const bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		const size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] |= src.m_digits[i];
		if (thisIsLonger && src.is_negative())
			set_sign_extension(true, i);
	}


	template <bool has_sign2, size_t bits2>
	void assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src)
	{
		m_digits[0] |= src.get_int();
		if (src.is_negative())
			set_sign_extension(true, 1);
	}


	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void assign_bit_and(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// n_digits will be the smaller size.
		for (size_t i = 0; i < n_digits; i++)
			m_digits[i] = src1.m_digits[i] & src2.m_digits[i];
	}

	template <bool has_sign2, size_t bits2>
	void assign_bit_and(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		const size_t lesserSize = (fixed_integer_extended<has_sign2, bits2>::n_digits < n_digits) ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] = src.m_digits[i];
		if ((n_digits > lesserSize) && !src.is_negative())
			set_sign_extension(false, i);
	}

	template <bool has_sign2, size_t bits2>
	void assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src)
	{
		m_digits[0] &= src.get_int();
		if (!src.is_negative())
			set_sign_extension(false, 1);
	}


	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void assign_bit_xor(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// n_digits will be the larger size.
		const size_t src1Digits = fixed_integer_extended<has_sign2, bits2>::n_digits;
		const size_t src2Digits = fixed_integer_extended<has_sign3, bits3>::n_digits;
		const size_t lesserSize = (src2Digits > src1Digits) ? src1Digits : src2Digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] = src1.m_digits[i] ^ src2.m_digits[i];

		if (src1Digits < src2Digits)
		{
			if (src1.is_negative())
				for (; i < n_digits; i++)
					m_digits[i] = ~src2.m_digits[i];
			else
				for (; i < n_digits; i++)
					m_digits[i] = src2.m_digits[i];
		}
		else if (src1Digits > src2Digits)
		{
			if (src2.is_negative())
				for (; i < n_digits; i++)
					m_digits[i] = ~src1.m_digits[i];
			else
				for (; i < n_digits; i++)
					m_digits[i] = src1.m_digits[i];
		}
	}


	template <bool has_sign2, size_t bits2>
	void assign_bit_xor(const this_t& src1, const fixed_integer_extended_content<has_sign2, bits2>& src2)
	{
		m_digits[0] = src1.m_digits[0] ^ src2.get_int();
		if (src2.is_negative())
			for (size_t i = 1; i < n_digits; i++)
				m_digits[i] = ~src1.m_digits[i];
		else
			for (size_t i = 1; i < n_digits; i++)
				m_digits[i] = src1.m_digits[i];
	}


	template <bool has_sign2, size_t bits2>
	void assign_bit_xor(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		const bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		const size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		size_t i = 0;
		for (; i < lesserSize; i++)
			m_digits[i] ^= src.m_digits[i];
		if (thisIsLonger && src.is_negative())
			for (; i < n_digits; i++)
				m_digits[i] = ~m_digits[i];
	}

	template <bool has_sign2, size_t bits2>
	void assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src)
	{
		m_digits[0] ^= src.get_int();
		if (src.is_negative())
			for (size_t i = 1; i < n_digits; i++)
				m_digits[i] = ~m_digits[i];
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		// Will have only 2 digits.  Only called by add operator in operators.hpp
		COGS_ASSERT(n_digits == 2);
		ulongest oldDigit = (ulongest)src1.get_int();
		ulongest newDigit = oldDigit + (ulongest)src2.get_int();
		set_digit(0, newDigit);
		bool overflow = newDigit < oldDigit;
		ulongest highDigit;
		bool src1IsNeg = src1.is_negative();
		if (src1IsNeg == src2.is_negative())
		{
			if (src1IsNeg)
			{
				if (overflow)
					highDigit = ~(ulongest)0;
				else
					highDigit = ~(ulongest)0 - 1;
			}
			else if (overflow)
				highDigit = 1;
			else
				highDigit = 0;
		}
		else if (overflow)
			highDigit = 0;
		else
			highDigit = ~(ulongest)0;
		set_digit(1, highDigit);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add_inner(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// src1 is larger or equal size type
		COGS_ASSERT(bits2 >= bits2);
		// May have 1 more digit than src1
		COGS_ASSERT((n_digits == fixed_integer_extended<has_sign2, bits2>::n_digits) || (n_digits == fixed_integer_extended<has_sign2, bits2>::n_digits + 1));

		size_t i = 0;
		bool overflow = false;
		for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
		{
			ulongest digit1 = src1.m_digits[i];
			ulongest newDigit = digit1 + src2.m_digits[i];
			if (!overflow)
				overflow = newDigit < digit1;
			else
			{
				newDigit++;
				overflow = newDigit <= digit1;
			}

			m_digits[i] = newDigit;
		}

		if (fixed_integer_extended<has_sign2, bits2>::n_digits == fixed_integer_extended<has_sign3, bits3>::n_digits)
		{
			if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				ulongest highDigit;
				ulongest restDigits;
				bool src1IsNeg = src1.is_negative();
				if (src1IsNeg == src2.is_negative())
				{
					if (src1IsNeg)
					{
						restDigits = ~(ulongest)0;
						if (overflow)
							highDigit = ~(ulongest)0;
						else
							highDigit = ~(ulongest)0 - 1;
					}
					else
					{
						restDigits = 0;
						if (overflow)
							highDigit = 1;
						else
							highDigit = 0;
					}
				}
				else if (overflow)
				{
					highDigit = 0;
					restDigits = 0;
				}
				else
				{
					highDigit = ~(ulongest)0;
					restDigits = ~(ulongest)0;
				}

				m_digits[i++] = highDigit;
				for (; i < n_digits; i++)
					m_digits[i] = restDigits;
			}
		}
		else // if (fixed_integer_extended<has_sign2, bits2>::n_digits > fixed_integer_extended<has_sign3, bits3>::n_digits)
		{
			if (!src2.is_negative()) // if not negative, handle normal overflow
				increment_copy(src1, i, overflow);
			else // if negative, handle an implicit '+ ~(ulongest)0' (-1) overflow on all digits only if NOT overflowing,
			{    // which would add one to all the overflow.  + ~(ulongest)0 + 1 = -1 + 1 = +0 overflow (if so, ignore negative src)
				if (overflow)
					copy_from(src1, i);
				else
				{
					for (;;)
					{
						ulongest digit1 = src1.m_digits[i];
						ulongest newDigit = digit1 - 1;
						m_digits[i] = newDigit;
						if (++i == fixed_integer_extended<has_sign2, bits2>::n_digits)
						{
							break;
						}
						if (digit1 == 0)
						{
							copy_from(src1, i);
							break;
						}
					}
				}
			}
		}
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		if (fixed_integer_extended_content<has_sign2, bits2>::n_digits >= fixed_integer_extended<has_sign3, bits3>::n_digits)
			add_inner(src1, src2);
		else
			add_inner(src2, src1);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		ulongest digit1 = src1.m_digits[0];
		ulongest newDigit = digit1 + src2.get_int();
		m_digits[0] = newDigit;
		bool overflow = newDigit < digit1;
		if (!src2.is_negative()) // if not negative, handle normal overflow
			increment_copy(src1, 1, overflow);
		else // if negative, handle an implicit '+ ~(ulongest)0' (-1) overflow on all digits only if NOT overflowing,
		{    // which would add one to all the overflow.  + ~(ulongest)0 + 1 = -1 +1 = +0 overflow (if so, ignore negative src)
			if (overflow)
				copy_from(src1, 1);
			else
			{
				size_t i = 1;
				for (;;)
				{
					digit1 = m_digits[i];
					newDigit = digit1 - 1;
					m_digits[i] = newDigit;
					if (++i == fixed_integer_extended<has_sign2, bits2>::n_digits)
						break;
					if (digit1 == 0)
					{
						copy_from(src1, i);
						break;
					}
				}
			}
		}
	}

	void add(const ulongest* digits, size_t n)
	{
		const size_t lesserSize = (n_digits < n) ? n_digits : n;

		size_t i = 0;
		bool overflow = false;
		for (; i < lesserSize; i++)
		{
			ulongest digit1 = m_digits[i];
			ulongest newDigit = digit1 + digits[i];
			if (!overflow)
				overflow = newDigit < digit1;
			else
			{
				newDigit++;
				overflow = newDigit <= digit1;
			}

			m_digits[i] = newDigit;
		}

		if (n_digits > n)
		{
			if (overflow)
				increment_from(i);
		}
	}

	template <bool has_sign2, size_t bits2>
	void add(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		const size_t lesserSize = (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits) ? n_digits : fixed_integer_extended<has_sign2, bits2>::n_digits;

		size_t i = 0;
		bool overflow = false;
		for (; i < lesserSize; i++)
		{
			ulongest digit1 = m_digits[i];
			ulongest newDigit = digit1 + src.m_digits[i];
			if (!overflow)
				overflow = newDigit < digit1;
			else
			{
				newDigit++;
				overflow = newDigit <= digit1;
			}

			m_digits[i] = newDigit;
		}

		if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
		{
			if (!src.is_negative()) // if not negative, handle normal overflow
			{
				if (overflow)
					increment_from(i);
			}
			else // if negative, handle an implicit '+ ~(ulongest)0' (-1) overflow on all digits only if NOT overflowing,
			{    // which would add one to all the overflow.  + ~(ulongest)0 + 1 = -1 + 1 = +0 overflow (if so, ignore negative src)
				if (!overflow)
				{
					for (;;)
					{
						ulongest digit1 = m_digits[i];
						ulongest newDigit = digit1 - 1;
						m_digits[i] = newDigit;
						if ((digit1 == 0) || (++i == n_digits))
							break;
					}
				}
			}
		}
	}

	template <bool has_sign2, size_t bits2>
	void add(const fixed_integer_native<has_sign2, bits2>& src)
	{
		ulongest digit1 = m_digits[0];
		ulongest newDigit = digit1 + src.get_int();
		m_digits[0] = newDigit;
		bool overflow = newDigit < digit1;
		if (!src.is_negative()) // if not negative, handle normal overflow
		{
			if (overflow)
				increment_from(1);
		}
		else // if negative, handle an implicit '+ ~(ulongest)0' (-1) overflow on all digits only if NOT overflowing,
		{    // which would add one to all the overflow.  + ~(ulongest)0 + 1 = -1 +1 = +0 overflow (if so, ignore negative src)
			if (!overflow)
			{
				size_t i = 1;
				for (;;)
				{
					digit1 = m_digits[i];
					newDigit = digit1 - 1;
					m_digits[i] = newDigit;
					if ((digit1 == 0) || (++i == n_digits))
						break;
				}
			}
		}
	}

	void add(const dynamic_integer& src);
	//{
	//	if (src.is_negative())
	//		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	else
	//		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		// Will have only 2 digits.  Only called by fixed_integer_native
		COGS_ASSERT(n_digits == 2);

		ulongest digit1 = (ulongest)src1.get_int();
		ulongest newDigit = digit1 - (ulongest)src2.get_int();
		set_digit(0, newDigit);
		bool overflow = newDigit > digit1;
		ulongest highDigit;
		if (src1.is_negative())
		{
			if (src2.is_negative())
			{
				if (overflow)
					highDigit = ~(ulongest)0;
				else
					highDigit = 0;
			}
			else if (overflow)
				highDigit = ~(ulongest)0 - 1;
			else
				highDigit = ~(ulongest)0;
		}
		else if (src2.is_negative())
		{
			if (overflow)
				highDigit = 0;
			else
				highDigit = 1;
		}
		else if (overflow)
			highDigit = ~(ulongest)0;
		else
			highDigit = 0;

		set_digit(1, highDigit);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// May have 1 more digit than src3
		COGS_ASSERT((n_digits == fixed_integer_extended<has_sign3, bits3>::n_digits) || ((n_digits == fixed_integer_extended<has_sign3, bits3>::n_digits) + 1));
		static constexpr bool firstIsLonger = n_digits > fixed_integer_extended<has_sign3, bits3>::n_digits;
		ulongest digit1 = (ulongest)src1;
		ulongest newDigit = digit1 - src2.m_digits[0];
		bool overflow = newDigit > digit1;
		m_digits[0] = newDigit;
		if (src1.is_negative()) // value is implicitly -1 for src digit
		{
			size_t i = 1;
			for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
			{
				newDigit = ~(ulongest)0 - src2.m_digits[i];
				if (overflow)
					overflow = (--newDigit == ~(ulongest)0);

				m_digits[i] = newDigit;
			}

			if (firstIsLonger) // one more digit
			{
				if (src2.is_negative())
				{
					if (!overflow)
						m_digits[i] = 0; // ~(ulongest)0 - ~(ulongest)0 = 0
					else
						m_digits[i] = ~(ulongest)0; // ~(ulongest)0 - ~(ulongest)0 - 1 = -1 = ~(ulongest)0
				}
				else if (!overflow)
					m_digits[i] = ~(ulongest)0; // ~(ulongest)0 - 0 = ~(ulongest)0
				else
					m_digits[i] = ~(ulongest)0 - 1; // ~(ulongest)0 - 0 - 1 = -2 = ~(ulongest)0 - 1;
			}
		}
		else
		{
			size_t i = 1;
			if (!overflow)
			{
				m_digits[i] = (ulongest)-(longest)src2.m_digits[i];
				++i;
				// the rest are overflowing, so an additional -1
			}

			for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
				m_digits[i] = (ulongest)-(longest)src2.m_digits[i] - 1;

			if (firstIsLonger) // one more digit
			{
				if (src2.is_negative())
					m_digits[i] = 0; // 0 - ~(ulongest)0 -1 = 0
				else
					m_digits[i] = ~(ulongest)0; // 0 - 0 -1 = -1 = ~(ulongest)0
			}
		}
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		static constexpr bool firstIsLonger = fixed_integer_extended<has_sign2, bits2>::n_digits > fixed_integer_extended<has_sign3, bits3>::n_digits;
		static constexpr size_t lesserSize = firstIsLonger ? fixed_integer_extended<has_sign3, bits3>::n_digits : fixed_integer_extended<has_sign2, bits2>::n_digits;
		static constexpr size_t greaterSize = firstIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : fixed_integer_extended<has_sign3, bits3>::n_digits;
		static constexpr bool resultIsLonger = n_digits > greaterSize;
		bool overflow = false;
		size_t i;
		for (i = 0; i < lesserSize; i++)
		{
			ulongest digit1 = src1.m_digits[i];
			ulongest newDigit = digit1 - src2.m_digits[i];
			if (!overflow)
				overflow = newDigit > digit1;
			else
			{
				newDigit--;
				overflow = (newDigit >= digit1);
			}
			m_digits[i] = newDigit;
		}

		if (firstIsLonger)
		{
			if (!src2.is_negative()) // if not negative, handle normal overflow
				decrement_copy(src1, 1, overflow);
			else // if negative, handle an implicit '- ~(ulongest)0' (+1) overflow on all digits only if NOT overflowing,
			{    // which would add one to all the overflow.  - ~(ulongest)0 = +1 -1 = 0 overflow (if so, ignore negative src)
				for (;;)
				{
					if (overflow)
					{
						copy_from(src1, i);
						break;
					}

					ulongest newDigit = src1.m_digits[i] + 1; // - ~(ulongest)0 = + 1
					m_digits[i] = newDigit;
					overflow = (newDigit != 0); // The only way it did NOT overflow is if original digit was already ~(ulongest)0
					if (++i == fixed_integer_extended<has_sign2, bits2>::n_digits)
					{
						if (resultIsLonger) // one more digit
						{
							ulongest newDigit;
							if (is_negative())
							{
								if (!overflow)
									newDigit = 0; // ~(ulongest)0 - ~(ulongest)0 = 0
								else
									newDigit = ~(ulongest)0; // ~(ulongest)0 - ~(ulongest)0 - 1 = -1 = ~(ulongest)0
							}
							else if (!overflow)
								newDigit = 1; // 0 - ~(ulongest)0 = 1
							else
								newDigit = 0; // 0 - ~(ulongest)0 - 1 = 0;
							m_digits[i] = newDigit;
						}
						break;
					}
				}
			}
		}
		else if (fixed_integer_extended<has_sign2, bits2>::n_digits != fixed_integer_extended<has_sign3, bits3>::n_digits)
		{ // src2 is longer
			if (src1.is_negative())
			{
				if (overflow)
				{
					for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
					{
						ulongest newDigit = (~(ulongest)0 - 1) - src2.m_digits[i];
						m_digits[i] = newDigit;
						overflow = newDigit == ~(ulongest)0; // only one possible way to continue overflowing, if src digit is -1
						if (!overflow)
						{
							++i;
							break;
						}
					}
				}

				for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
					m_digits[i] = ~(ulongest)0 - src2.m_digits[i];

				if (resultIsLonger) // one more digit
				{
					ulongest newDigit;
					if (src2.is_negative())
					{
						if (!overflow)
							newDigit = 0; // ~(ulongest)0 - ~(ulongest)0 = 0
						else
							newDigit = ~(ulongest)0; // ~(ulongest)0 - ~(ulongest)0 - 1 = -1 = ~(ulongest)0
					}
					else if (!overflow)
						newDigit = ~(ulongest)0; // ~(ulongest)0 - 0 = ~(ulongest)0
					else
						newDigit = ~(ulongest)0 - 1; // ~(ulongest)0 - 0 - 1 = ~(ulongest)0 - 1 = -2
					m_digits[i] = newDigit;
				}
			}
			else
			{
				if (!overflow)
				{
					for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++)
					{
						ulongest newDigit = (ulongest)-(longest)src2.m_digits[i];
						m_digits[i] = newDigit;
						overflow = newDigit != 0;
						if (!overflow)
						{
							++i;
							break;
						}
					}
				}

				for (; i < fixed_integer_extended<has_sign3, bits3>::n_digits; i++) // all overflow
					m_digits[i] = (ulongest)-(longest)src2.m_digits[i] - 1;

				if (resultIsLonger) // one more digit
				{
					ulongest newDigit;
					if (src2.is_negative())
					{
						if (!overflow)
							newDigit = 1; // 0 - ~(ulongest)0 = 1
						else
							newDigit = 0; // 0 - ~(ulongest)0 - 1 = 0
					}
					else if (!overflow)
						newDigit = 0; // 0 - 0 = 0
					else
						newDigit = ~(ulongest)0; // 0 - 0 -1 = -1 = ~(ulongest)0
					m_digits[i] = newDigit;
				}
			}
		}
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src)
	{
		ulongest digit1 = m_digits[0];
		ulongest newDigit = digit1 - src.get_int();
		m_digits[0] = newDigit;
		bool overflow = newDigit > digit1;
		if (!src.is_negative()) // if not negative, handle normal overflow
			decrement_copy(src1, 1, overflow);
		else // if negative, handle an implicit '- ~(ulongest)0' (+1) overflow on all digits only if NOT overflowing,
		{    // which would add one to all the overflow.  - ~(ulongest)0 = +1 -1 = 0 overflow (if so, ignore negative src)
			size_t i = 1;
			for (;;)
			{
				if (overflow)
				{
					copy_from(src1, i);
					break;
				}
				newDigit = m_digits[i] + 1;
				m_digits[i] = newDigit;
				if (++i == n_digits)
					break;
				overflow = (newDigit != 0);
			}
		}
	}

	void subtract(const ulongest* srcDigits, size_t srcLength) // needed by divide_whole_and_assign_modulo().  src is positive.
	{
		bool thisIsLonger = n_digits > srcLength;
		size_t lesserSize = thisIsLonger ? srcLength : n_digits;
		bool overflow = false;
		size_t i = 0;
		for (; i < lesserSize; i++)
		{
			ulongest digit1 = m_digits[i];
			ulongest newDigit = digit1 - srcDigits[i];
			if (!overflow)
				overflow = newDigit > digit1;
			else
			{
				newDigit--;
				overflow = (newDigit >= digit1);
			}
			m_digits[i] = newDigit;
		}
		if (thisIsLonger)
		{
			if (overflow)
				decrement_from(i);
		}
	}

	template <bool has_sign2, size_t bits2>
	void subtract(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		static constexpr bool thisIsLonger = n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits;
		static constexpr size_t lesserSize = thisIsLonger ? fixed_integer_extended<has_sign2, bits2>::n_digits : n_digits;
		bool overflow = false;
		size_t i = 0;
		for (; i < lesserSize; i++)
		{
			ulongest digit1 = m_digits[i];
			ulongest newDigit = digit1 - src.m_digits[i];
			if (!overflow)
				overflow = newDigit > digit1;
			else
			{
				newDigit--;
				overflow = (newDigit >= digit1);
			}
			m_digits[i] = newDigit;
		}
		if (thisIsLonger)
		{
			if (!src.is_negative()) // if not negative, handle normal overflow
			{
				if (overflow)
					decrement_from(i);
			}
			else if (!overflow) // if negative, handle an implicit '+1' overflow on all digits only if NOT overflowing (which would negate the need for it)
			{
				do {
					if (0 != ++(m_digits[i]))
						break;
				} while (++i < n_digits);
			}
		}
	}

	template <bool has_sign2, size_t bits2>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src)
	{
		ulongest digit1 = m_digits[0];
		ulongest newDigit = digit1 - src.get_int();
		m_digits[0] = newDigit;
		bool overflow = newDigit > digit1;
		if (!src.is_negative())
		{
			if (overflow)
				decrement_from(1);
		}
		else if (!overflow)
		{
			size_t i = 1;
			do {
				if (0 != ++(m_digits[i])) // - ~(ulongest)0 = + 1
					break; // The only way it did NOT overflow is if original digit was already ~(ulongest)0
			} while (++i < n_digits);
		}
	}

	void subtract(const dynamic_integer& src);
	//{
	//	if (src.is_negative())
	//		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	else
	//		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//}

	void inverse_subtract(const dynamic_integer& src);
	//{
	//	if (src.is_negative())
	//		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	else
	//		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	assign_negative();
	//}

	void set_left_shifted(const ulongest* srcDigits, size_t srcLength, size_t bitShift, size_t digitShift)
	{
		size_t i = 0;
		size_t end = (digitShift >= n_digits) ? n_digits : digitShift;
		for (; i < end; i++)
			m_digits[i] = 0;
		if (end != n_digits)
		{
			end = digitShift + srcLength;
			if (end > n_digits)
				end = n_digits;
			if (!bitShift)
			{
				for (; i < end; i++)
					m_digits[i] = srcDigits[i - digitShift];
			}
			else
			{
				size_t bitShiftBack = (sizeof(ulongest) * 8) - bitShift;
				ulongest carryOver = 0;
				for (; i < end; i++)
				{
					ulongest srcDigitValue = srcDigits[digitShift + i];
					m_digits[i] = carryOver | (srcDigitValue >> bitShift);
					carryOver = srcDigitValue << bitShiftBack;
				}
				if (end < n_digits)
					m_digits[i++] = carryOver;
			}
			for (; i < n_digits; i++)
				m_digits[i++] = 0;
		}
	}

	// both will be positive
	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply2(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		size_t src1Length = fixed_integer_extended<has_sign2, bits2>::n_digits;

		// Skip leading zeros in src1.
		for (;;)
		{
			size_t src1LengthNext = src1Length - 1;
			if (src1.m_digits[src1LengthNext] != 0)
				break;
			if (!src1LengthNext) // src1 is zero
			{
				clear();
				return;
			}
			src1Length = src1LengthNext;
		}

		ulongest newDigit = (ulongest)src2.get_int();
		multiply(src1.m_digits.data(), src1Length, &newDigit, 1);
	}

	// src1 digits will be >= src2 digits. - slightly more efficient
	void multiply(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)
	{
		size_t src1HighIndex = src1Length - 1;
		size_t src2HighIndex = src2Length - 1;
		ulongest src2HighDigitValue = src2Digits[src2HighIndex];
		bool isSrc2PowerOfTwo = cogs::is_exponent_of_two(src2HighDigitValue);
		if (isSrc2PowerOfTwo)
		{
			// Because high digit is known to be non-zero, just need to ensure all other digits are zero.
			for (size_t i = 0; i < src2HighIndex; i++)
			{
				if (src2Digits[i] != 0)
				{
					isSrc2PowerOfTwo = false;
					break;
				}
			}
		}
		if (isSrc2PowerOfTwo)
		{
			size_t bitShift = cogs::bit_scan_forward(src2HighDigitValue);
			set_left_shifted(src1Digits, src1Length, bitShift, src2HighIndex);
		}
		else
		{
			ulongest src1HighDigitValue = src1Digits[src1HighIndex];
			bool isSrc1PowerOfTwo = cogs::is_exponent_of_two(src1HighDigitValue);
			if (isSrc1PowerOfTwo)
			{
				// Because high digit is known to be non-zero, just need to ensure all other digits are zero.
				for (size_t i = 0; i < src1HighIndex; i++)
				{
					if (src1Digits[i] != 0)
					{
						isSrc1PowerOfTwo = false;
						break;
					}
				}
			}
			if (isSrc1PowerOfTwo)
			{
				size_t bitShift = cogs::bit_scan_forward(src1HighDigitValue);
				set_left_shifted(src2Digits, src2Length, bitShift, src1HighIndex);
			}
			else
			{
				clear();
				size_t src2Index = 0;
				do {
					ulongest overflow = 0;
					ulongest src2Digit = src2Digits[src2Index];
					size_t dstIndex = src2Index;
					size_t src1Index = 0;
					for (;;)
					{
						ulongest newOverflow;
						ulongest src1Digit = src1Digits[src1Index];
						ulongest origDigit = m_digits[dstIndex];
						ulongest newDigit = origDigit + env::umul_longest(src1Digit, src2Digit, newOverflow);
						if (dstIndex == n_digits - 1)
						{
							m_digits[dstIndex] = newDigit + overflow;
							if (src2Index == n_digits - 1)
								return; // break out of both loops.  We're done.
							break;
						}

						if (newDigit < origDigit)
							newOverflow++;
						newDigit += overflow;
						if (newDigit < overflow)
							newOverflow++;
						overflow = newOverflow;
						m_digits[dstIndex++] = newDigit;
						if (++src1Index == src1Length)
						{
							newDigit = m_digits[dstIndex] + overflow;
							m_digits[dstIndex] = newDigit;
							if ((newDigit < overflow) && (dstIndex < src1HighIndex))
								increment_from(dstIndex + 1);
							break;
						}
					}
				} while (++src2Index < src2Length);
			}
		}
	}

	template <bool has_sign2, size_t bits2>
	void multiply(const fixed_integer_extended_content<has_sign2, bits2>& src1, const ulongest* src2Digits, size_t src2Length)
	{
		// Skip leading zeros in src1.
		size_t src1Length = n_digits;
		for (;;)
		{
			size_t src1LengthNext = src1Length - 1;
			if (src1.m_digits[src1LengthNext] != 0)
				break;
			if (!src1LengthNext) // src1 is zero
			{
				clear();
				return;
			}
			src1Length = src1LengthNext;
		}

		const ulongest* src1Digits;
		if (src1Length >= src2Length)
			src1Digits = &(src1.m_digits[0]);
		else
		{
			src1Digits = src2Digits;
			src2Digits = &(src1.m_digits[0]);
			size_t tmp = src1Length;
			src1Length = src2Length;
			src2Length = tmp;
		}

		multiply(src1Digits, src1Length, src2Digits, src2Length);
	}

	// Both will be positive
	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply2(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// Skip leading zeros in src2.
		size_t src2Length = fixed_integer_extended<has_sign3, bits3>::n_digits;
		for (;;)
		{
			size_t src2LengthNext = src2Length - 1;
			if (src2.m_digits[src2LengthNext] != 0)
				break;
			if (!src2LengthNext) // src2 is zero
			{
				clear();
				return;
			}
			src2Length = src2LengthNext;
		}

		multiply(src1, &(src2.m_digits[0]), src2Length);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		COGS_ASSERT(n_digits == 2);
		if constexpr (!has_sign2 && !has_sign3)
		{
			COGS_ASSERT(!has_sign);
			m_digits[0] = env::umul_longest(src1.get_int(), src2.get_int(), m_digits[1]);
		}
		else
		{
			ulongest tmp1;
			ulongest tmp2;
			bool oneIsNegative;
			if (src1.is_negative())
			{
				oneIsNegative = !src2.is_negative();
				if (oneIsNegative)
				{
					tmp1 = src1.get_int(); // negative value is in tmp1
					tmp2 = src2.get_int();
				}
				else // Both are negative, negate them both
				{
					tmp1 = (ulongest)-(longest)src1.get_int();
					tmp2 = (ulongest)-(longest)src2.get_int();
				}
			}
			else
			{
				oneIsNegative = src2.is_negative();
				tmp1 = src2.get_int(); // negative value is in tmp1, if any negative
				tmp2 = src1.get_int();
			}

			m_digits[0] = env::umul_longest(tmp1, tmp2, m_digits[1]);
			if (oneIsNegative)
				m_digits[1] += (tmp2 * ~(ulongest)0);
		}
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		auto x = src2.abs();
		multiply2(src1, x);
		if (src2.is_negative())
			assign_negative();
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply(const fixed_integer_extended_content<has_sign2, bits2>& src1, const fixed_integer_extended_content<has_sign3, bits3>& src2)
	{
		// More efficient to negate twice than to multiply a bunch of superflous ~(ulongest)0 digits
		if (src1.is_negative())
		{
			fixed_integer_extended_content<false, bits2> absSrc1;
			absSrc1.assign_abs(src1);
			if (src2.is_negative())
			{
				fixed_integer_extended_content<false, bits3> absSrc2;
				absSrc2.assign_abs(src2);
				multiply2(absSrc1, absSrc2);
			}
			else
				multiply2(absSrc1, src2);
		}
		else if (src2.is_negative())
		{
			fixed_integer_extended_content<false, bits3> absSrc2;
			absSrc2.assign_abs(src2);
			multiply2(src1, absSrc2);
		}
		else
			multiply2(src1, src2);
	}

	void multiply(const this_t& src1, const dynamic_integer_content& src2);
	//{
	//	if (src1.is_negative())
	//	{
	//		this_t abs1;
	//		abs1.assign_negative(src1);
	//		multiply(abs1, src2.get_const_ptr(), src2.get_length());
	//		if (!src2.is_negative())
	//			assign_negative();
	//	}
	//	else
	//	{
	//		multiply(src1, src2.get_const_ptr(), src2.get_length());
	//		if (src2.is_negative())
	//			assign_negative();
	//	}
	//}

	template <bool has_sign2, size_t bits2>
	void multiply(const fixed_integer_native<has_sign2, bits2>& src)
	{
		bool isNeg = is_negative();
		fixed_integer_extended_content<false, bits> tmp;
		tmp.assign_abs(*this);
		auto x = src.abs();
		multiply2(tmp, x);
		if (src.is_negative() != isNeg)
			assign_negative();
	}

	template <bool has_sign2, size_t bits2>
	void multiply(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		// More efficient to negate twice than to multiply a bunch of superflous ~(ulongest)0 digits
		if (is_negative())
		{
			fixed_integer_extended_content<false, bits> absSrc1;
			absSrc1.assign_abs(*this);
			if (src.is_negative())
			{
				fixed_integer_extended_content<false, bits2> absSrc2;
				absSrc2.assign_abs(src);
				multiply2(absSrc1, absSrc2);
			}
			else
				multiply2(absSrc1, src);
		}
		else
		{
			this_t tmp(*this);
			if (src.is_negative())
			{
				fixed_integer_extended_content<false, bits2> absSrc2;
				absSrc2.assign_abs(src);
				multiply2(tmp, absSrc2);
			}
			else
				multiply2(tmp, src);
		}
	}

	void multiply(const dynamic_integer_content& src);
	//{
	//	if (is_negative())
	//	{
	//		this_t abs1;
	//		abs1.assign_negative(*this);
	//		multiply(abs1, src.get_const_ptr(), src.get_length());
	//		if (!src.is_negative())
	//			assign_negative();
	//	}
	//	else
	//	{
	//		this_t tmp(*this);
	//		multiply(tmp, src.get_const_ptr(), src.get_length());
	//		if (src.is_negative())
	//			assign_negative();
	//	}
	//}

	void multiply()
	{
		// More efficient to negate twice than to multiply a bunch of superflous ~(ulongest)0 digits
		if (is_negative())
		{
			fixed_integer_extended_content<false, bits> abs1;
			abs1.assign_abs(*this);
			multiply2(abs1, abs1);
		}
		else
		{
			this_t cpy(*this);
			multiply2(cpy, cpy);
		}
	}


	void divide_whole_and_assign_modulo(const ulongest* denomDigits, size_t denomLength, ulongest* resultDigits, size_t resultLength)
	{
		// Skip leading zeros in numerator.
		size_t highIndex = n_digits - 1;
		while (m_digits[highIndex] == 0)
			--highIndex;

		divide_whole_and_assign_modulo(highIndex + 1, denomDigits, denomLength, resultDigits, resultLength);
	}


	// Only called with positive values, and only if this > denom
	// This is numerator.  This is set to remainder.
	// result will be preset to 0
	void divide_whole_and_assign_modulo(size_t thisLength, const ulongest* denomDigits, size_t denomLength, ulongest* resultDigits, size_t resultLength)
	{
		size_t denomHighIndex = denomLength - 1;
		ulongest denomHighDigitValue = denomDigits[denomHighIndex];

		// exponent of two optimization
		bool isDenomPowerOfTwo = cogs::is_exponent_of_two(denomHighDigitValue);
		if (isDenomPowerOfTwo)
		{
			// Because high digit is known to be non-zero, just need to ensure all other digits are zero.
			for (size_t i = 0; i < denomHighDigitValue; i++)
			{
				if (denomDigits[i] != 0)
				{
					isDenomPowerOfTwo = false;
					break;
				}
			}
		}
		if (isDenomPowerOfTwo)
		{
			size_t moduloDigits = denomLength;
			size_t bitShift = cogs::bit_scan_forward(denomHighDigitValue);
			if (!bitShift) // No bit shifting necessary, just digit shifting
				--moduloDigits; // Will be >0 because denom is known != 1
			size_t divideDigits = thisLength - moduloDigits;
			if (!!resultDigits)
			{
				if (!!bitShift)
				{
					//result->set_right_shifted2(m_digits.get_ptr() + moduloDigits - 1, divideDigits + 1);
					const ulongest* srcDigits = &(m_digits[moduloDigits - 1]);
					size_t srcLength = divideDigits + 1;
					size_t shiftBack = (sizeof(ulongest) * 8) - bitShift;
					size_t i = 0;
					ulongest srcValue = srcDigits[0];
					for (;;)
					{
						m_digits[i] = srcValue >> bitShift;
						size_t nextIndex = i + 1;
						if (nextIndex == srcLength)
							break;
						ulongest srcValueNext = srcDigits[nextIndex];
						m_digits[i] |= srcValueNext << shiftBack;
						i = nextIndex;
						srcValue = srcValueNext;
					}
				}
				else if (!!divideDigits)
				{
					size_t i = 0;
					do {
						resultDigits[i] = m_digits[i + moduloDigits];
					} while (++i != divideDigits);
				}
			}

			for (size_t i = moduloDigits; i < thisLength; i++)
				m_digits[i] = 0;
		}
		else
		{
			size_t highIndex = thisLength - 1;

			// Division algorithm is guess and check (divide and conquer), using the denominator's high digit (+ 1)

			// We use half-digits, to handle carries and still fit the math in native ints
			typedef bits_to_int_t<(sizeof(ulongest) * 8) / 2, false> half_unsigned_t;
			static constexpr ulongest highMask = (~(ulongest)0 ^ (half_unsigned_t)~(half_unsigned_t)0);

			// We special case a high divisor digit of max value, as +1 causes it to overflow into an additional digit.
			if (denomHighDigitValue >= highMask)
			{
				if (highIndex == denomHighIndex) // denominator is so large, there can only possibly be 1
				{
					subtract(denomDigits, denomLength);
					if (!!resultDigits)
						resultDigits[0] = 1;
					return;
				}

				size_t i = (highIndex - denomHighIndex);
				do {
					--i;
					for (;;)
					{
						ulongest multiplier = m_digits[highIndex];
						if (multiplier == 0)
						{
							highIndex--;
							break;
						}
						if (!!resultDigits && (i < resultLength))
						{
							ulongest oldValue = resultDigits[i];
							ulongest newValue = oldValue + multiplier;
							resultDigits[i] = newValue;
							if (oldValue > newValue)
							{
								size_t incIndex = i + 1;
								while ((incIndex < resultLength) && ++(resultDigits[incIndex]) == 0)
									++incIndex;
							}
						}

						// Remove our guess from the remainder, by multiplying it with the actual denominator
						ulongest overflow = 0;
						size_t dstIndex;
						for (size_t i2 = 0; i2 <= denomHighIndex; i2++)
						{
							ulongest newOverflow;
							ulongest tmpDigit = env::umul_longest(multiplier, denomDigits[i2], newOverflow);
							ulongest tmpDigit2 = tmpDigit + overflow;
							overflow = newOverflow;
							if (tmpDigit2 < tmpDigit)
								overflow++;

							dstIndex = i2 + i;
							ulongest oldDigit = m_digits[dstIndex];
							ulongest newDigit = oldDigit - tmpDigit2;
							m_digits[dstIndex] = newDigit;
							if (newDigit > oldDigit)
							{
								size_t decIndex = dstIndex + 1;
								while (m_digits[decIndex]-- == 0)
									++decIndex;
							}
						}

						if (overflow != 0)
						{
							++dstIndex;
							ulongest oldDigit = m_digits[dstIndex];
							ulongest newDigit = oldDigit - overflow;
							m_digits[dstIndex] = newDigit;
							if (newDigit > oldDigit)
							{
								size_t decIndex = dstIndex + 1;
								while (m_digits[decIndex]-- == 0)
									++decIndex;
							}
						}
					}
				} while (i != 0);

				// There is less than the rounded up value remaining, but it's still possible there is one more multiple of the denominator.
				int cmpResult = compare(highIndex, denomDigits, denomLength);
				if (cmpResult != -1)
				{
					if (!!resultDigits)
					{
						size_t incIndex = 0;
						while (++(resultDigits[incIndex]) == 0)
						{
							++incIndex;
							if (resultLength == incIndex)
								break;
						}
					}
					if (cmpResult == 1)
						subtract(denomDigits, denomLength);
					else
						clear();
				}
			}
			else
			{
				bool onBoundary = false;

				// On boundaries between digits, a half-digit is used
				ulongest denomHighDigitValueHigh = 1 + ((denomHighDigitValue & highMask) >> (sizeof(half_unsigned_t) * 8));

				if (denomHighIndex != 0)
					denomHighDigitValue++; // Was not 0.  Will not become 0

				size_t i = highIndex - denomHighIndex;
				for (;;)
				{
					ulongest highRemainder = m_digits[highIndex];
					ulongest multiplier;
					if (!onBoundary)
					{
						if (highRemainder >= denomHighDigitValue)
							multiplier = highRemainder / denomHighDigitValue;
						else
						{
							if (i == 0)
								break;
							--i;
							if (highRemainder == 0)
							{
								--highIndex;
								continue;
							}
							onBoundary = true; // (highRemainder < denomHighDigitValue) && (highRemainder != 0)
						}
					}
					else if (highRemainder == 0)
					{
						onBoundary = false;
						highIndex--;
						continue;
					}
					if (onBoundary)
					{
						if (highRemainder >= denomHighDigitValueHigh)
							multiplier = (highRemainder / denomHighDigitValueHigh) << (sizeof(ulongest) * 4);
						else // (highRemainder < denomHighDigitValueHigh)
						{
							ulongest nextRemainderDigitValue = m_digits[highIndex - 1];
							ulongest tmpRemainder = (highRemainder << (sizeof(ulongest) * 4)) | (nextRemainderDigitValue >> (sizeof(ulongest) * 4));
							if (tmpRemainder >= denomHighDigitValue)
								multiplier = (tmpRemainder / denomHighDigitValue) << (sizeof(ulongest) * 4);
							else // (tmpRemainder < denomHighDigitValue)
								multiplier = tmpRemainder / denomHighDigitValueHigh;
						}
					}
					if (!!resultDigits && (i < resultLength))
					{
						ulongest oldValue = resultDigits[i];
						ulongest newValue = oldValue + multiplier;
						resultDigits[i] = newValue;
						if (oldValue > newValue)
						{
							size_t incIndex = i + 1;
							while ((incIndex < resultLength) && ++(resultDigits[incIndex]) == 0)
								++incIndex;
						}
					}

					ulongest overflow = 0;
					size_t dstIndex;
					for (size_t i2 = 0; i2 <= denomHighIndex; i2++)
					{
						ulongest newOverflow;
						ulongest tmpDigit = env::umul_longest(multiplier, denomDigits[i2], newOverflow);
						ulongest tmpDigit2 = tmpDigit + overflow;
						overflow = newOverflow;
						if (tmpDigit2 < tmpDigit)
							overflow++;

						dstIndex = i2 + i;
						ulongest oldDigit = m_digits[dstIndex];
						ulongest newDigit = oldDigit - tmpDigit2;
						m_digits[dstIndex] = newDigit;
						if (newDigit > oldDigit)
						{
							size_t decIndex = dstIndex + 1;
							while (m_digits[decIndex]-- == 0)
								++decIndex;
						}
					}

					if (overflow != 0)
					{
						++dstIndex;
						ulongest oldDigit = m_digits[dstIndex];
						ulongest newDigit = oldDigit - overflow;
						m_digits[dstIndex] = newDigit;
						if (newDigit > oldDigit)
						{
							size_t decIndex = dstIndex + 1;
							while (m_digits[decIndex]-- == 0)
								++decIndex;
						}
					}
				}
			}
		}
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void divide_whole_and_assign_modulo(const fixed_integer_extended_content<has_sign2, bits2>& denom,
		fixed_integer_extended_content<has_sign3, bits3>* result = 0)
	{
		if (!!result)
			result->clear();

		ulongest* denomDigits;
		size_t denomLength;

		fixed_integer_extended_content<false, bits2> absDenom;

		if (denom.is_negative())
		{
			absDenom.assign_abs(denom);

			denomDigits = &absDenom.m_digits[0];
			denomLength = fixed_integer_extended_content<false, bits2>::n_digits;
			while (denomDigits[denomLength - 1] == 0)
				--denomLength; // denom was negative, so won't be zero
		}
		else
		{
			denomDigits = &denom.m_digits[0];
			denomLength = fixed_integer_extended<has_sign2, bits2>::n_digits;
			while (denomDigits[denomLength - 1] == 0)
			{
				--denomLength;
				if (denomLength == 0)
				{
					// denom is zero.  Divide by zero error!
					COGS_ASSERT(false);
					clear();
					return;
				}
			}
		}

		bool wasNegative = is_negative();
		if (wasNegative)
			assign_negative();

		int i = compare(n_digits, denomDigits, denomLength);
		if (i == 1)
		{
			divide_whole_and_assign_modulo(denomDigits, denomLength, &(result->m_digits[0]), fixed_integer_extended<has_sign3, bits3>::n_digits);
			if (wasNegative)
				assign_negative();
			if (!!result)
			{
				if (wasNegative != denom.is_negative())
					result->assign_negative();
			}
		}
		else if (i == 0)
		{
			if (!!result)
				*result = one_t();
			clear();
		}
		else if (wasNegative)
			assign_negative();
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& denom,
		fixed_integer_extended_content<has_sign3, bits3>* result = 0)
	{
		if (!!result)
			result->clear();

		if (!denom)
		{
			COGS_ASSERT(false);
			clear();
			return;
		}

		ulongest denomDigit = (ulongest)denom.abs().get_int();

		bool wasNegative = is_negative();
		if (wasNegative)
			assign_negative();

		int i = compare(n_digits, &denomDigit, 1);
		if (i == 1)
		{
			divide_whole_and_assign_modulo(&denomDigit, 1, &(result->m_digits[0]), fixed_integer_extended<has_sign3, bits3>::n_digits);
			if (wasNegative)
				assign_negative();
			if (!!result)
			{
				if (wasNegative != denom.is_negative())
					result->assign_negative();
			}
		}
		else if (i == 0)
		{
			if (!!result)
				*result = one_t();
			clear();
		}
		else if (wasNegative)
			assign_negative();
	}

	template <bool has_sign3, size_t bits3>
	void divide_whole_and_assign_modulo(const dynamic_integer_content& denom, fixed_integer_extended_content<has_sign3, bits3>* result = 0);
	//{
	//	if (!!result)
	//		result->clear();
	//
	//	ulongest* denomDigits = denom.get_const_ptr();
	//	size_t denomLength = denom.get_length();
	//
	//	bool wasNegative = is_negative();
	//	if (wasNegative)
	//		assign_negative();
	//	int i = compare(n_digits, denomDigits, denomLength);
	//	if (i == 1)
	//	{
	//		divide_whole_and_assign_modulo(denomDigits, denomLength, &(result->m_digits[0]), fixed_integer_extended<has_sign3, bits3>::n_digits);
	//		if (wasNegative)
	//			assign_negative();
	//		if (!!result)
	//		{
	//			if (wasNegative != denom.is_negative())
	//				result->assign_negative();
	//		}
	//	}
	//	else if (i == 0)
	//	{
	//		if (!!result)
	//			*result = one_t();
	//		clear();
	//	}
	//	else if (wasNegative)
	//		assign_negative();
	//}

	bool assign_reciprocal()
	{
		for (;;)
		{
			ulongest lowDigit = m_digits[0];
			if (!lowDigit)
			{
				bool restIsZero = test_sign_extension(false, 1);
				if (restIsZero)
				{
					COGS_ASSERT(false); // no recip of 0
					return false;
				}
			}
			else if (lowDigit == 1)
			{
				bool restIsZero = test_sign_extension(false, 1);
				if (restIsZero) // if full value is 1, leave intact
					return false;
			}
			else if (has_sign && (lowDigit == (ulongest)-1))
			{
				bool restIsNegOne = test_sign_extension(true, 1);
				if (restIsNegOne) // if full value is -1, leave intact
					return false;
			}

			clear();
			return true;
		}
	}

	template <bool has_sign2, size_t bits2>
	auto gcd(const fixed_integer_extended_content<has_sign2, bits2>& d2)
	{
		fixed_integer_extended_content<false, ((bits < bits2) ? bits : bits2)> result;
		result.clear();

		fixed_integer_extended_content<false, bits> t1;
		t1.assign_abs(*this);

		fixed_integer_extended_content<false, bits2> t2;
		t2.assign_abs(d2);

		size_t t1Length = fixed_integer_extended<false, bits>::n_digits;
		size_t t2Length = fixed_integer_extended<false, bits2>::n_digits;

		for (;;)
		{
			size_t i = t1Length - 1;
			if (t1.m_digits[i] != 0)
				break;
			if (i == 0)
				return result;
			t1Length = i;
		}

		for (;;)
		{
			size_t i = t2Length - 1;
			if (t2.m_digits[i] != 0)
				break;
			if (i == 0)
				return result;
			t2Length = i;
		}

		int cmpValue = t1.compare(t1Length, &(t2.m_digits[0]), t2Length);
		if (cmpValue == 0)
		{
			result = one_t();
			return result;
		}

		fixed_integer_extended_content<false, ((bits < bits2) ? bits2 : bits)> scratch;
		bool flip = (cmpValue == -1);
		for (;;)
		{
			if (flip)
			{
				t2.divide_whole_and_assign_modulo(t2Length, &(t1.m_digits[0]), t1Length, &(scratch.m_digits[0]), t2Length);
				for (;;)
				{
					size_t i = t2Length - 1;
					if (t2.m_digits[i] != 0)
						break;
					if (i == 0)
					{
						result = scratch;
						return result;
					}
					t2Length = i;
				}
				for (size_t i = 0; i < t2Length; i++) // clear result for next iteration
					scratch.m_digits[i] = 0;
			}
			else
			{
				t1.divide_whole_and_assign_modulo(t1Length, &(t2.m_digits[0]), t2Length, &(scratch.m_digits[0]), t1Length);
				for (;;)
				{
					size_t i = t1Length - 1;
					if (t1.m_digits[i] != 0)
						break;
					if (i == 0)
					{
						result = scratch;
						return result;
					}
					t1Length = i;
				}
				for (size_t i = 0; i < t1Length; i++) // clear result for next iteration
					scratch.m_digits[i] = 0;
			}
			flip = !flip;
		}
	}

	template <bool has_sign2, size_t bits2>
	auto gcd(const fixed_integer_native<has_sign2, bits2>& d2) const
	{
		fixed_integer_native<false, bits2> result(0);
		if (!!*this && !!d2)
		{
			fixed_integer_extended_content<false, bits> t1;
			t1.assign_abs(*this);
			auto t2 = d2.abs();
			ulongest digit = t2;
			int cmpValue = t1.compare(n_digits, &digit, 1);
			if (cmpValue == 0)
				result = one_t();
			else if (cmpValue == -1) // If a positive fixed_integer_extended is less than a fixed_integer_native, it must be <=1 digits
				result = t2.gcd(t1.m_digits.get_const_ptr()[0]);
			else
			{ // Since remainder will fit within fixed_integer_native after the first round, just do one round and pass it on.
				ulongest tmpResult;
				t1.divide_whole_and_assign_modulo(&digit, 1, &tmpResult, 1);
				if (!t1)
					result = tmpResult;
				else
					result = t2.gcd(t1.m_digits.get_const_ptr()[0]);
			}
		}

		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto lcm(const fixed_integer_extended_content<has_sign2, bits2>& d2)
	{
		// TODO: Replace with more efficient LCM algorithm if extremely large values

		fixed_integer_extended_content<false, bits + bits2> result;
		if (!*this || !d2)
			result.clear();
		else
		{
			auto tmpGcd = gcd(*this, d2);
			fixed_integer_extended_content<has_sign | has_sign2, bits + bits2> r;
			r.multiply(*this, d2);
			r.divide_whole_and_assign_modulo(tmpGcd.get_const_ptr(), tmpGcd.get_length(), &(result.m_digits[0]), fixed_integer_extended<false, bits + bits2>::n_digits);
		}
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto lcm(const fixed_integer_native<has_sign2, bits2>& d2)
	{
		// TODO: Replace with more efficient LCM algorithm if extremely large values

		fixed_integer_extended_content<false, bits + bits2> result;
		if (!*this || !d2)
			result.clear();
		else
		{
			auto tmpGcd = gcd(*this, d2);
			fixed_integer_extended_content<has_sign | has_sign2, bits + bits2> r;
			r.multiply(*this, d2);
			r.divide_whole_and_assign_modulo(tmpGcd.get_const_ptr(), tmpGcd.get_length(), &(result.m_digits[0]), fixed_integer_extended<false, bits + bits2>::n_digits);
		}
		return result;
	}


	template <bool has_sign2, size_t bits2>
	auto greater(const fixed_integer_extended_content<has_sign2, bits2>& d2)
	{
		fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> result;
		if (is_greater_than(d2))
			result = *this;
		else
			result = d2;
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto greater(const fixed_integer_native<has_sign2, bits2>& d2)
	{
		fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> result;
		if (is_greater_than(d2))
			result = *this;
		else
			result = d2;
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto lesser(const fixed_integer_extended_content<has_sign2, bits2>& d2)
	{
		fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> result;
		if (is_less_than(d2))
			result = *this;
		else
			result = d2;
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto lesser(const fixed_integer_native<has_sign2, bits2>& d2)
	{
		fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> result;
		if (is_less_than(d2))
			result = *this;
		else
			result = d2;
		return result;
	}

	bool equals(const ulongest* cmp, size_t n) const
	{
		for (size_t i = 0; i < n; i++)
		{
			if (m_digits[i] != cmp[i])
				return false;
		}
	}

	bool equals(const this_t& cmp) const
	{
		for (size_t i = 0; i < n_digits; i++)
		{
			if (m_digits[i] != cmp.m_digits[i])
				return false;
		}
	}

	template <bool has_sign2, size_t bits2>
	bool equals(const fixed_integer_extended_content<has_sign2, bits2>& cmp) const
	{
		if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
			return cmp.operator==(*this);

		for (size_t i = 0; i < fixed_integer_extended<has_sign2, bits2>::n_digits; i++)
		{
			if (m_digits[i] != cmp.m_digits[i])
				return false;
		}
		if (n_digits == fixed_integer_extended<has_sign2, bits2>::n_digits)
			return (cmp.is_negative() == is_negative());

		// else // (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
		return test_sign_extension(cmp.is_negative(), fixed_integer_extended<has_sign2, bits2>::n_digits);
	}


	template <bool has_sign2, size_t bits2>
	bool equals(const fixed_integer_native<has_sign2, bits2>& cmp) const
	{
		if (m_digits[0] != (ulongest)(longest)cmp.get_int())
			return false;
		return test_sign_extension(cmp.is_negative(), 1);
	}

	int compare(const this_t& cmp) const
	{
		size_t i = n_digits - 1;
		if (has_sign)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if ((longest)digit != (longest)cmpDigit)
			{
				if ((longest)digit > (longest)cmpDigit)
					return 1;
				if ((longest)digit < (longest)cmpDigit)
					return -1;
			}
			--i;
			// After the first digit, use unsigned comparison
		}
		while (i != -1)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit != cmpDigit)
			{
				if (digit > cmpDigit)
					return 1;
				if (digit < cmpDigit)
					return -1;
			}
			--i;
		}
		return 0;
	}

	// Both will be is positive
	int compare(size_t n, const ulongest* cmpDigits, size_t cmpLength) const
	{
		size_t i = n;
		if (n != cmpLength)
		{
			if (n > cmpLength)
			{
				i = cmpLength;
				if (!test_sign_extension(false, cmpLength))
					return 1;
			}
			else // if (n < cmpLength)
			{
				for (size_t i = n; i < cmpLength; i++)
				{
					if (m_digits[i] != 0)
						return -1;
				}
			}
		}

		for (;;)
		{
			--i;
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmpDigits[i];
			if (digit != cmpDigit)
			{
				if (digit > cmpDigit)
					return 1;
				if (digit < cmpDigit)
					return -1;
			}
			if (!i)
				break;
		}
		return 0; // they are equal
	}

	template <bool has_sign2, size_t bits2>
	int compare(const fixed_integer_extended_content<has_sign2, bits2>& cmp) const
	{
		size_t i = n_digits;
		if (is_negative())
		{
			if (!cmp.is_negative())
				return -1;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(true, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return -1;
					if ((longest)(m_digits[fixed_integer_extended<has_sign2, bits2>::n_digits - 1]) >= 0)
						return -1;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(true, n_digits))
						return 1;
					if ((longest)(cmp.m_digits[n_digits - 1]) >= 0)
						return 1;
				}
			}
		}
		else
		{
			if (cmp.is_negative())
				return 1;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(false, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return 1;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(false, n_digits))
						return -1;
				}
			}
		}

		for (;;)
		{
			--i;
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit != cmpDigit)
			{
				if (digit > cmpDigit)
					return 1;
				if (digit < cmpDigit)
					return -1;
			}
			if (!i)
				break;
		}
		return 0; // they are equal
	}

	template <bool has_sign2, size_t bits2>
	int compare(const fixed_integer_native<has_sign2, bits2>& cmp) const
	{
		if (is_negative())
		{
			if (!cmp.is_negative())
				return -1; // else, cmp is negative, therefore signed
			if (!test_sign_extension(true, 1))
				return -1;
			if ((longest)(m_digits[0]) >= 0) // Sign boundary condition: If lowest digit does NOT have its highest bit set, we are less
				return -1;
			if ((longest)(m_digits[0]) > (typename fixed_integer_native<has_sign2, bits2>::signed_int_t)cmp.get_int())
				return 1;
			if ((longest)(m_digits[0]) < (typename fixed_integer_native<has_sign2, bits2>::signed_int_t)cmp.get_int())
				return -1;
			return 0;
		}
		if (cmp.is_negative())
			return 1;
		if (!test_sign_extension(false, 1))
			return 1;
		if (m_digits[0] > (typename fixed_integer_native<has_sign2, bits2>::unsigned_int_t)cmp.get_int())
			return 1;
		if (m_digits[0] < (typename fixed_integer_native<has_sign2, bits2>::unsigned_int_t)cmp.get_int())
			return -1;
	}

	bool is_less_than(const this_t& cmp) const
	{
		size_t i = n_digits - 1;
		if (has_sign)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if ((longest)digit != (longest)cmpDigit)
			{
				if ((longest)digit > (longest)cmpDigit)
					return false;
				if ((longest)digit < (longest)cmpDigit)
					return true;
			}
			--i;
		}
		while (i != -1)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit > cmpDigit)
			{
				if (digit > cmpDigit)
					return false;
				if (digit < cmpDigit)
					return true;
			}
			--i;
		}
		return false;
	}

	template <bool has_sign2, size_t bits2>
	bool is_less_than(const fixed_integer_extended_content<has_sign2, bits2>& cmp) const
	{
		size_t i = n_digits;
		if (is_negative())
		{
			if (!cmp.is_negative())
				return true;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(true, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return true;
					if ((longest)(m_digits[fixed_integer_extended<has_sign2, bits2>::n_digits - 1]) >= 0)
						return true;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(true, n_digits))
						return false;
					if ((longest)(cmp.m_digits[n_digits - 1]) >= 0)
						return false;
				}
			}
		}
		else
		{
			if (cmp.is_negative())
				return false;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(false, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return false;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(false, n_digits))
						return true;
				}
			}
		}
		for (;;)
		{
			i--;
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit != cmpDigit)
				return (digit < cmpDigit);
			if (!i)
				break;
		}
		return false; // they are equal
	}

	template <bool has_sign2, size_t bits2>
	bool is_less_than(const fixed_integer_native<has_sign2, bits2>& cmp) const
	{
		if (is_negative())
		{
			if (!cmp.is_negative())
				return true;
			if (!test_sign_extension(true, 1))
				return true;
			if ((longest)(m_digits[0]) >= 0) // Sign boundary condition: If lowest digit does NOT have its highest bit set, we are less
				return true;
			return ((longest)(m_digits[0]) < cmp.get_int());
		}
		if (cmp.is_negative())
			return false;
		if (!test_sign_extension(false, 1))
			return false;
		return (m_digits[0] < (typename fixed_integer_native<has_sign2, bits2>::unsigned_int_t)(cmp.get_int()));
	}

	bool is_greater_than(const this_t& cmp) const
	{
		size_t i = n_digits - 1;
		if (has_sign)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if ((longest)digit != (longest)cmpDigit)
			{
				if ((longest)digit > (longest)cmpDigit)
					return true;
				if ((longest)digit < (longest)cmpDigit)
					return false;
			}
			--i;
		}
		while (i != -1)
		{
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit != cmpDigit)
			{
				if (digit > cmpDigit)
					return true;
				if (digit < cmpDigit)
					return false;
			}
			--i;
		}
		return false;
	}


	template <bool has_sign2, size_t bits2>
	bool is_greater_than(const fixed_integer_extended_content<has_sign2, bits2>& cmp) const
	{
		size_t i = n_digits;
		if (is_negative())
		{
			if (!cmp.is_negative())
				return false;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(true, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return false;
					if ((longest)(m_digits[fixed_integer_extended<has_sign2, bits2>::n_digits - 1]) >= 0)
						return false;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(true, n_digits))
						return true;
					if ((longest)(cmp.m_digits[n_digits - 1] >= 0))
						return true;
				}
			}
		}
		else
		{
			if (cmp.is_negative())
				return true;
			if (n_digits != fixed_integer_extended<has_sign2, bits2>::n_digits)
			{
				if (n_digits > fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					i = fixed_integer_extended<has_sign2, bits2>::n_digits;
					if (!test_sign_extension(false, fixed_integer_extended<has_sign2, bits2>::n_digits))
						return true;
				}
				else // if (n_digits < fixed_integer_extended<has_sign2, bits2>::n_digits)
				{
					if (!cmp.test_sign_extension(false, n_digits))
						return false;
				}
			}
		}
		for (;;)
		{
			--i;
			ulongest digit = m_digits[i];
			ulongest cmpDigit = cmp.m_digits[i];
			if (digit != cmpDigit)
				return (digit > cmpDigit);
			if (!i)
				break;
		}
		return false; // they are equal
	}

	template <bool has_sign2, size_t bits2>
	bool is_greater_than(const fixed_integer_native<has_sign2, bits2>& cmp) const
	{
		if (is_negative())
		{
			if (!cmp.is_negative())
				return false;
			if (!test_sign_extension(true, 1))
				return false;
			if ((longest)(m_digits[0]) >= 0) // Sign boundary condition: If lowest digit does NOT have its highest bit set, we are less
				return false;
			return ((longest)(m_digits[0]) > cmp.get_int());
		}
		if (cmp.is_negative())
			return true;
		if (!test_sign_extension(false, 1))
			return true;
		return (m_digits[0] > (typename fixed_integer_native<has_sign2, bits2>::unsigned_int_t)(cmp.get_int()));
	}

};


// Extended representation breaks the value into two parts, a high int and a low array of ulongest
template <bool has_sign_in, size_t n_bits_in> // n_bits_in should never be <= sizeof(longest)
class fixed_integer_extended
{
public:
	typedef fixed_integer_extended<has_sign_in, n_bits_in> this_t;

	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = n_bits_in; // should never be <= sizeof(longest)
	static constexpr size_t n_digits = (bits / (sizeof(longest)*8)) + ((bits % (sizeof(longest)*8) == 0) ? 0 : 1); // will be >0

	typedef longest signed_int_t;
	typedef ulongest unsigned_int_t;
	typedef ulongest int_t;

private:
	static_assert(bits > (sizeof(longest) * 8));
	static_assert(n_digits > 0);

	template <bool, size_t>
	friend class fixed_integer_extended;

	template <bool, size_t>
	friend class fixed_integer_extended_content;

	friend class dynamic_integer;
	friend class dynamic_integer_content;

	typedef fixed_integer_extended_content<has_sign, bits> content_t;
	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token read_token;
	typedef typename transactable_t::write_token write_token;

	read_token begin_read() const volatile { return m_contents.begin_read(); }
	void begin_read(read_token& rt) const volatile { m_contents.begin_read(rt); }
	bool end_read(read_token& t) const volatile { return m_contents.end_read(t); }
	void begin_write(write_token& wt) volatile { m_contents.begin_write(wt); }
	template <typename type2>
	void begin_write(write_token& wt, type2& src) volatile { m_contents.begin_write(wt, src); }
	bool promote_read_token(read_token& rt, write_token& wt) volatile { return m_contents.promote_read_token(rt, wt); }
	template <typename type2>
	bool promote_read_token(read_token& rt, write_token& wt, type2& src) volatile { return m_contents.promote_read_token(rt, wt, src); }
	bool end_write(write_token& t) volatile { return m_contents.end_write(t); }
	template <typename type2>
	bool end_write(read_token& t, type2& src) volatile { return m_contents.end_write(t, src); }
	template <class functor_t>
	void write_retry_loop(functor_t&& fctr) volatile { m_contents.write_retry_loop(std::forward<functor_t>(fctr)); }
	template <class functor_t>
	auto write_retry_loop_pre(functor_t&& fctr) volatile { return m_contents.write_retry_loop_pre(std::forward<functor_t>(fctr)); }
	template <class functor_t>
	auto write_retry_loop_post(functor_t&& fctr) volatile { return m_contents.write_retry_loop_post(std::forward<functor_t>(fctr)); }
	template <class functor_t, class on_fail_t>
	void write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile { m_contents.write_retry_loop(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
	template <class functor_t, class on_fail_t>
	auto write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.write_retry_loop_pre(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
	template <class functor_t, class on_fail_t>
	auto write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.write_retry_loop_post(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
	template <class functor_t>
	void try_write_retry_loop(functor_t&& fctr) volatile { m_contents.try_write_retry_loop(std::forward<functor_t>(fctr)); }
	template <class functor_t>
	auto try_write_retry_loop_pre(functor_t&& fctr) volatile { return m_contents.try_write_retry_loop_pre(std::forward<functor_t>(fctr)); }
	template <class functor_t>
	auto try_write_retry_loop_post(functor_t&& fctr) volatile { return m_contents.try_write_retry_loop_post(std::forward<functor_t>(fctr)); }
	template <class functor_t, class on_fail_t>
	void try_write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile { m_contents.try_write_retry_loop(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
	template <class functor_t, class on_fail_t>
	auto try_write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.try_write_retry_loop_pre(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
	template <class functor_t, class on_fail_t>
	auto try_write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.try_write_retry_loop_post(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }

	fixed_integer_extended(const content_t& c)
		: m_contents(typename transactable_t::construct_embedded_t(), c)
	{ }

	this_t& operator=(const content_t& src)
	{
		*m_contents = src;
		return *this;
	}

	volatile this_t& operator=(const content_t& src) volatile
	{
		m_contents.assign(src);
		return *this;
	}

public:
	typedef typename content_t::bits_used_t bits_used_t;

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_contents->add(src1, src2);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_contents->subtract(src1, src2);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_extended<has_sign3, bits3>& src2)
	{
		m_contents->subtract(src1, src2);
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_contents->multiply(src1, src2);
	}


	fixed_integer_extended() { }

	fixed_integer_extended(const dynamic_integer& src); //{ operator=(src); }

	fixed_integer_extended(const volatile dynamic_integer& src); //{ operator=(src); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	fixed_integer_extended(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{ operator=(src); }


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	fixed_integer_extended(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	fixed_integer_extended(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	fixed_integer_extended(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{ operator=(src); }


	template <bool has_sign2, size_t bits2>
	fixed_integer_extended(const fixed_integer_native<has_sign2, bits2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_extended(const volatile fixed_integer_native<has_sign2, bits2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_extended(const fixed_integer_extended<has_sign2, bits2>& src)
	{ operator=(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	fixed_integer_extended(const int_t2& src)
	{ operator=(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	fixed_integer_extended(const volatile int_t2& src)
	{ operator=(src); }

	fixed_integer_extended(const this_t& src)
	{ operator=(src); }

	template <typename numerator_t, typename denominator_t>
	fixed_integer_extended(const fraction<numerator_t, denominator_t>& src)
	{ operator=(src); }

	template <typename numerator_t, typename denominator_t>
	fixed_integer_extended(const volatile fraction<numerator_t, denominator_t>& src)
	{ operator=(src); }

	this_t& operator=(const dynamic_integer& src);
	//{
	//	if (src.is_negative())
	//		m_contents->assign_negative(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	else
	//		m_contents->assign(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	//	return *this;
	//}

	volatile this_t& operator=(const dynamic_integer& src) volatile;
	//{
	//	this_t tmp(src);
	//	return operator=(tmp);
	//}

	this_t& operator=(const volatile dynamic_integer& src);
	//{
	//	dynamic_integer tmp(src);
	//	return operator=(tmp);
	//}

	volatile this_t& operator=(const volatile dynamic_integer& src) volatile;
	//{
	//	dynamic_integer tmp(src);
	//	return operator=(tmp);
	//}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	this_t& operator=(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	this_t& operator=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	volatile this_t& operator=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	volatile this_t& operator=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile
	{
		this_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	this_t& operator=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{
		for (size_t i = 0; i < (sizeof...(values2) < n_digits ? sizeof...(values2) : n_digits); i++)
			m_contents->m_digits[i] = src.get_digit(i);
		if constexpr (sizeof...(values2) < n_digits)
			m_contents->set_sign_extension(src.is_negative(), sizeof...(values2));
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	this_t& operator=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{
		for (size_t i = 0; i < (sizeof...(values2) < n_digits ? sizeof...(values2) : n_digits); i++)
			m_contents->m_digits[i] = src.get_digit(i);
		if constexpr (sizeof...(values2) < n_digits)
			m_contents->set_sign_extension(src.is_negative(), sizeof...(values2));
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	volatile this_t& operator=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile
	{
		this_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	volatile this_t& operator=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile
	{
		this_t tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const fixed_integer_native<has_sign2, bits2>& src)
	{
		*m_contents = src;
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	volatile this_t& operator=(const fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		m_contents.assign(src);
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const volatile fixed_integer_native<has_sign2, bits2>& src)
	{
		fixed_integer_native<has_sign2, bits2> tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2>
	volatile this_t& operator=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		fixed_integer_native<has_sign2, bits2> tmp(src);
		return operator=(tmp);
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		*m_contents = *(src.m_contents);
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	volatile this_t& operator=(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		m_contents.assign(*(src.m_contents));
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		m_contents.assign(*(src.begin_read()));
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	volatile this_t& operator=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		m_contents.assign(*(src.begin_read()));
		return *this;
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator=(const int_t2& src)
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		return operator=(tmp);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator=(const int_t2& src) volatile
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		return operator=(tmp);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator=(const volatile int_t2& src)
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		return operator=(tmp);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator=(const volatile int_t2& src) volatile
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		return operator=(tmp);
	}


	template <typename numerator_t, typename denominator_t>
	this_t& operator=(const fraction<numerator_t, denominator_t>& src)
	{
		return operator=(src.floor());
	}

	template <typename numerator_t, typename denominator_t>
	this_t& operator=(const volatile fraction<numerator_t, denominator_t>& src)
	{
		return operator=(src.floor());
	}

	template <typename numerator_t, typename denominator_t>
	volatile this_t& operator=(const fraction<numerator_t, denominator_t>& src) volatile
	{
		return operator=(src.floor());
	}

	template <typename numerator_t, typename denominator_t>
	volatile this_t& operator=(const volatile fraction<numerator_t, denominator_t>& src) volatile
	{
		return operator=(src.floor());
	}

	int_t get_int() const { return m_contents->m_digits[0]; }
	int_t get_int() const volatile { return begin_read()->m_digits[0]; }

	operator int_t() const { return m_contents->m_digits[0]; }
	operator int_t() const volatile { return begin_read()->m_digits[0]; }

	const this_t& simplify_type() const { return *this; }
	const volatile this_t& simplify_type() const volatile { return *this; }

	int_t* get_digits() { return &(m_contents->m_digits[0]); }
	const int_t* get_digits() const { return &(m_contents->m_digits[0]); }

	int_t& get_digit(size_t i) { return m_contents->m_digits[i]; }
	const int_t& get_digit(size_t i) const { return m_contents->m_digits[i]; }
	int_t get_digit(size_t i) const volatile { return begin_read()->m_digits[i]; }

	void set_digit(size_t i, const int_t& src) { m_contents->set_digit(i, src); }
	void set_digit(size_t i, const int_t& src) volatile { write_retry_loop([&](content_t& c) { c.set_digit(i, src); }); }

	void clear() { m_contents->clear(); }
	void clear() volatile { *this = zero_t(); }

	bool operator!() const { return !*m_contents; }
	bool operator!() const volatile { return !*begin_read(); }

	this_t operator~() const { this_t result; result.assign_bit_not(*m_contents); return result; }
	this_t operator~() const volatile { this_t result; result.assign_bit_not(*begin_read()); return result; }
	void assign_bit_not() { m_contents->assign_bit_not(*m_contents); }
	void assign_bit_not() volatile { write_retry_loop([&](content_t& c) { c.assign_bit_not(c); }); }
	this_t& pre_assign_bit_not() const { m_contents->assign_bit_not(*m_contents); return *this; }
	this_t pre_assign_bit_not() const volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_not(c); }); }
	this_t post_assign_bit_not() const { this_t result(*this); m_contents->assign_bit_not(*m_contents); return result; }
	this_t post_assign_bit_not() const volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_not(c); }); }

	bool is_negative() const { return m_contents->is_negative(); }
	bool is_negative() const volatile { return begin_read()->is_negative(); }

	bool is_exponent_of_two() const { return m_contents->is_exponent_of_two(); }
	bool is_exponent_of_two() const volatile { return begin_read()->is_exponent_of_two(); }

	constexpr bool has_fractional_part() const volatile { return false; }

	auto abs() const { fixed_integer_extended<false, bits> result; result.m_contents->assign_abs(*m_contents); return result; }
	auto abs() const volatile { fixed_integer_extended<false, bits> result; result.m_contents->assign_abs(*begin_read()); return result; }
	auto abs_inner() const { fixed_integer_extended<false, bits> result; result.m_contents->assign_negative(*m_contents); return result; }
	auto abs_inner() const volatile { fixed_integer_extended<false, bits> result; result.m_contents->assign_negative(*begin_read()); return result; }
	void assign_abs() { if (has_sign) m_contents->assign_abs(); }
	void assign_abs() volatile { if (has_sign) try_write_retry_loop([&](content_t& c) { return c.assign_abs(); }); }
	this_t& pre_assign_abs() { if (has_sign) m_contents->assign_abs(); return *this; }
	this_t pre_assign_abs() volatile { if (has_sign) return try_write_retry_loop_pre([&](content_t& c) { return c.assign_abs(); }); return *this; }
	this_t post_assign_abs() { if (has_sign) { this_t result(*this); m_contents->assign_abs(); return result; } return *this; }
	this_t post_assign_abs() volatile { return try_write_retry_loop_post([&](content_t& c) { return c.assign_abs(); }); return *this; }

	auto operator-() const { fixed_integer_extended<true, bits + 1> result; result.m_contents->assign_negative(*m_contents); return result; }
	auto operator-() const volatile { fixed_integer_extended<true, bits + 1> result; result.m_contents->assign_negative(*begin_read()); return result; }
	void assign_negative() { m_contents->assign_negative(); }
	void assign_negative() volatile { write_retry_loop([&](content_t& c) { c.assign_negative(); }); }
	this_t& pre_assign_negative() { m_contents->assign_negative(); return *this; }
	this_t pre_assign_negative() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_negative(); }); }
	this_t post_assign_negative() { this_t result(*this); m_contents->assign_negative(); return result; }
	this_t post_assign_negative() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_negative(); }); }

	size_t bit_count() const { return m_contents->bit_count(); }
	size_t bit_count() const volatile { return begin_read()->bit_count(); }
	void assign_bit_count() { m_contents->assign_bit_count(); }
	void assign_bit_count() volatile { write_retry_loop([&](content_t& c) { c.assign_bit_count(); }); }
	this_t& pre_assign_bit_count() { m_contents->assign_bit_count(); return *this; }
	this_t pre_assign_bit_count() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_count(); }); }
	this_t post_assign_bit_count() { this_t result(*this); m_contents->assign_bit_count(); return result; }
	this_t post_assign_bit_count() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_count(); }); }

	size_t bit_scan_forward() const { return m_contents->bit_scan_forward(); }
	size_t bit_scan_forward() const volatile { return begin_read()->bit_scan_forward(); }
	void assign_bit_scan_forward() { m_contents->assign_bit_scan_forward(); }
	void assign_bit_scan_forward() volatile { write_retry_loop([&](content_t& c) { c.assign_bit_scan_forward(); }); }
	this_t& pre_assign_bit_scan_forward() { m_contents->assign_bit_scan_forward(); return *this; }
	this_t pre_assign_bit_scan_forward() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_scan_forward(); }); }
	this_t post_assign_bit_scan_forward() { this_t result(*this); m_contents->assign_bit_scan_forward(); return result; }
	this_t post_assign_bit_scan_forward() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_scan_forward(); }); }

	size_t bit_scan_reverse() const { return m_contents->bit_scan_reverse(); }
	size_t bit_scan_reverse() const volatile { return begin_read()->bit_scan_reverse(); }
	void assign_bit_scan_reverse() { m_contents->assign_bit_scan_reverse(); }
	void assign_bit_scan_reverse() volatile { write_retry_loop([&](content_t& c) { c.assign_bit_scan_reverse(); }); }
	this_t& pre_assign_bit_scan_reverse() { m_contents->assign_bit_scan_reverse(); return *this; }
	this_t pre_assign_bit_scan_reverse() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_scan_reverse(); }); }
	this_t post_assign_bit_scan_reverse() { this_t result(*this); m_contents->assign_bit_scan_reverse(); return result; }
	this_t post_assign_bit_scan_reverse() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_scan_reverse(); }); }

	this_t next() const { this_t tmp(*this); tmp.m_contents->increment(); return tmp; }
	this_t next() const volatile { this_t tmp(*this); tmp.m_contents->increment(); return tmp; }
	void assign_next() { m_contents->increment(); }
	void assign_next() volatile { write_retry_loop([&](content_t& c) { c.increment(); }); }
	this_t& operator++() { assign_next(); return *this; }
	this_t operator++() volatile { return write_retry_loop_pre([&](content_t& c) { c.increment(); }); }
	this_t operator++(int) { this_t result(*this); m_contents->increment(); return result; }
	this_t operator++(int) volatile { return write_retry_loop_post([&](content_t& c) { c.increment(); }); }

	this_t prev() const { this_t tmp(*this); tmp.m_contents->decrement(); return tmp; }
	this_t prev() const volatile { this_t tmp(*this); tmp.m_contents->decrement(); return tmp; }
	void assign_prev() { m_contents->decrement(); }
	void assign_prev() volatile { write_retry_loop([&](content_t& c) { c.decrement(); }); }
	this_t& operator--() { assign_prev(); return *this; }
	this_t operator--() volatile { return write_retry_loop_pre([&](content_t& c) { c.decrement(); }); }
	this_t operator--(int) { this_t result(*this); m_contents->decrement(); return result; }
	this_t operator--(int) volatile { return write_retry_loop_post([&](content_t& c) { c.decrement(); }); }


	// bit_rotate_right
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_right_rotated(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_right_rotated(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_right_rotated(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_right_rotated(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const dynamic_integer& src) const; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const dynamic_integer& src) const volatile; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const volatile dynamic_integer& src) const; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const volatile dynamic_integer& src) const volatile; // { return bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_rotated(*tmp.m_contents, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_rotated(*tmp.m_contents, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_right_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const volatile dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const dynamic_integer& src) volatile; // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { assign_bit_rotate_right(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_bit_rotate_right(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_right_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	const this_t& pre_assign_bit_rotate_right(const dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_rotate_right(const volatile dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_rotate_right(const dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	this_t pre_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_rotated(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_rotated(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_right_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const dynamic_integer& src); // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const volatile dynamic_integer& src); // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const dynamic_integer& src) volatile; // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }

	// bit_rotate_left
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_left_rotated(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_left_rotated(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_left_rotated(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_left_rotated(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const dynamic_integer& src) const; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const dynamic_integer& src) const volatile; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const volatile dynamic_integer& src) const; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const volatile dynamic_integer& src) const volatile; // { return bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_rotated(*tmp.m_contents, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_rotated(*tmp.m_contents, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_left_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const volatile dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const dynamic_integer& src) volatile; // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { assign_bit_rotate_left(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_bit_rotate_left(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_left_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	const this_t& pre_assign_bit_rotate_left(const dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_rotate_left(const volatile dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_rotate_left(const dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	this_t pre_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_rotated(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_rotated(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_write_retry_loop([&](content_t& c) { content_t tmp(c); c.set_to_left_rotated(tmp, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const dynamic_integer& src); // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const volatile dynamic_integer& src); // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const dynamic_integer& src) volatile; // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }

	// bit_shift_right
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_right_shifted(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_right_shifted(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_right_shifted(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_right_shifted(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator>>(src % bits_used_t()); }
	this_t operator>>(const dynamic_integer& src) const; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const dynamic_integer& src) const volatile; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const volatile dynamic_integer& src) const; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const volatile dynamic_integer& src) const volatile; // { return operator>>(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& operator>>(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator>>(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->set_to_right_shifted(*m_contents, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_native<has_sign2, bits2>& src) { m_contents->set_to_right_shifted(*m_contents, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.set_to_right_shifted(c, src.get_int()); }); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_bit_shift_right(tmp); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { operator>>=(src % bits_used_t()); return *this; }
	this_t& operator>>=(const dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	this_t& operator>>=(const volatile dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	volatile this_t& operator>>=(const dynamic_integer& src) volatile; // { operator>>=(src % bits_used_t()); return *this; }
	volatile this_t& operator>>=(const volatile dynamic_integer& src) volatile; // { operator>>=(src % bits_used_t()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator>>=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator>>=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator>>=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator>>=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator>>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator>>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator>>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator>>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { operator>>=(src % bits_used_t()); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator>>=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); operator>>=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator>>=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); operator>>=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator>>=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); operator>>=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator>>=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); operator>>=(tmp);  return *this; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) { operator>>=(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { operator>>=(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_write_retry_loop([&](content_t& c) { c.set_to_right_shifted(c, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	const this_t& pre_assign_bit_shift_right(const dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_shift_right(const volatile dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_shift_right(const dynamic_integer& src) volatile; // { return pre_assign_bit_shift_right(src % bits_used_t()); }
	this_t pre_assign_bit_shift_right(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_shift_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_shifted(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_right_shifted(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_write_retry_loop([&](content_t& c) { c.set_to_right_shifted(c, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const dynamic_integer& src); // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const volatile dynamic_integer& src); // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const dynamic_integer& src) volatile; // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_shift_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }

	// bit_shift_left
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_left_shifted(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_left_shifted(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->set_to_left_shifted(*m_contents, src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->set_to_left_shifted(*begin_read(), src.get_int()); return result; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator<<(src % bits_used_t()); }
	this_t operator<<(const dynamic_integer& src) const; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const dynamic_integer& src) const volatile; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const volatile dynamic_integer& src) const; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const volatile dynamic_integer& src) const volatile; // { return operator<<(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& operator<<(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator<<(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->set_to_left_shifted(*m_contents, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_native<has_sign2, bits2>& src) { m_contents->set_to_left_shifted(*m_contents, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.set_to_left_shifted(c, src.get_int()); }); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_bit_shift_left(tmp); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { operator<<=(src % bits_used_t()); return *this; }
	this_t& operator<<=(const dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	this_t& operator<<=(const volatile dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	volatile this_t& operator<<=(const dynamic_integer& src) volatile; // { operator<<=(src % bits_used_t()); return *this; }
	volatile this_t& operator<<=(const volatile dynamic_integer& src) volatile; // { operator<<=(src % bits_used_t()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator<<=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator<<=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator<<=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); operator<<=(tmp); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator<<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator<<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator<<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator<<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { operator<<=(src % bits_used_t()); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator<<=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); operator<<=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator<<=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); operator<<=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator<<=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); operator<<=(tmp);  return *this; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator<<=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); operator<<=(tmp);  return *this; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) { operator<<=(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { operator<<=(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_write_retry_loop([&](content_t& c) { c.set_to_left_shifted(c, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	const this_t& pre_assign_bit_shift_left(const dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_shift_left(const volatile dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_shift_left(const dynamic_integer& src) volatile; // { return pre_assign_bit_shift_left(src % bits_used_t()); }
	this_t pre_assign_bit_shift_left(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_shift_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_shifted(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->set_to_left_shifted(*tmp.m_contents, src.get_int()); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_write_retry_loop([&](content_t& c) { c.set_to_left_shifted(c, src.get_int()); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const dynamic_integer& src); // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const volatile dynamic_integer& src); // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const dynamic_integer& src) volatile; // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_shift_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }


	// bit or
	template <bool has_sign2, size_t bits2> this_t operator|(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->assign_bit_or(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> this_t operator|(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->assign_bit_or(*begin_read(), src); return result; }
	template <bool has_sign2, size_t bits2> this_t operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator|(tmp); }
	template <bool has_sign2, size_t bits2> this_t operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator|(tmp); }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_or(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_or(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_or(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_or(*begin_read(), *(src.begin_read())); return result; }

	auto operator|(const this_t& src) const { if (this == &src) return *this; this_t result; result.m_contents->assign_bit_or(*m_contents, *(src.m_contents)); return result; }
	auto operator|(const this_t& src) const volatile { this_t result; result.m_contents->assign_bit_or(*begin_read(), *(src.m_contents)); return result; }
	auto operator|(const volatile this_t& src) const { this_t result; result.m_contents->assign_bit_or(*m_contents, *(src.begin_read())); return result; }
	auto operator|(const volatile this_t& src) const volatile { if (this == &src) return *this; this_t result; result.m_contents->assign_bit_or(*begin_read(), *(src.begin_read())); return result; }

	template <bool has_sign2, size_t bits2> const this_t& operator|(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator|(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator|(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator|(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_or(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_or(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_or(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_or(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_or(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_or(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator|=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator|=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator|=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator|=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator|=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator|=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator|=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator|=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator|=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator|=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator|=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_or(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_or(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_or(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_or(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_or(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_or(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_or(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_or(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_or(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_or(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_or(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_or(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_or(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_or(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_or(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_or(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_or(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_or(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_or(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_or(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_or(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_or(tmp); }


	// bit_and
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> operator&(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_and(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> operator&(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_and(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_and(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_and(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), ((bits < bits2) ? bits : bits2)> result; result.m_contents->assign_bit_and(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), ((bits < bits2) ? bits : bits2)> result; result.m_contents->assign_bit_and(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), ((bits < bits2) ? bits : bits2)> result; result.m_contents->assign_bit_and(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), ((bits < bits2) ? bits : bits2)> result; result.m_contents->assign_bit_and(*begin_read(), *(src.begin_read())); return result; }

	auto operator&(const this_t& src) const { if (this == &src) return *this; this_t result; result.m_contents->assign_bit_and(*m_contents, *(src.m_contents)); return result; }
	auto operator&(const this_t& src) const volatile { this_t result; result.m_contents->assign_bit_and(*begin_read(), *(src.m_contents)); return result; }
	auto operator&(const volatile this_t& src) const { this_t result; result.m_contents->assign_bit_and(*m_contents, *(src.begin_read())); return result; }
	auto operator&(const volatile this_t& src) const volatile { if (this == &src) return *this; this_t result; result.m_contents->assign_bit_and(*begin_read(), *(src.begin_read())); return result; }

	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_and(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_and(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_and(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_and(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_and(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_and(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator&=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator&=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator&=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator&=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator&=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator&=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator&=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator&=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator&=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator&=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator&=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_and(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_and(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_and(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_and(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_and(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_and(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_and(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_and(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_and(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_and(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_and(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_and(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_and(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_and(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_and(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_and(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_and(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_and(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_and(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_and(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_and(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_and(tmp); }

	// bit xor
	template <bool has_sign2, size_t bits2> this_t operator^(const fixed_integer_native<has_sign2, bits2>& src) const { this_t result; result.m_contents->assign_bit_xor(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> this_t operator^(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t result; result.m_contents->assign_bit_xor(*begin_read(), src); return result; }
	template <bool has_sign2, size_t bits2> this_t operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator^(tmp); }
	template <bool has_sign2, size_t bits2> this_t operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator^(tmp); }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_xor(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_xor(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_xor(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> result; result.m_contents->assign_bit_xor(*begin_read(), *(src.begin_read())); return result; }

	this_t operator^(const this_t& src) const { if (this == &src) return zero_t(); this_t result; result.m_contents->assign_bit_xor(*m_contents, *(src.m_contents)); return result; }
	this_t operator^(const this_t& src) const volatile { this_t result; result.m_contents->assign_bit_xor(*begin_read(), *(src.m_contents)); return result; }
	this_t operator^(const volatile this_t& src) const { this_t result; result.m_contents->assign_bit_xor(*m_contents, *(src.begin_read())); return result; }
	this_t operator^(const volatile this_t& src) const volatile { if (this == &src) return zero_t(); this_t result; result.m_contents->assign_bit_xor(*begin_read(), *(src.begin_read())); return result; }

	template <bool has_sign2, size_t bits2> const this_t& operator^(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator^(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator^(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator^(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_xor(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_xor(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_xor(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_xor(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_xor(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.assign_bit_xor(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator^=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator^=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator^=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator^=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator^=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator^=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator^=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator^=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator^=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator^=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator^=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->assign_bit_xor(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_xor(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_xor(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_xor(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->assign_bit_xor(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_bit_xor(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_xor(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_xor(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_bit_xor(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_bit_xor(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_bit_xor(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_xor(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_xor(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_xor(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_xor(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->assign_bit_xor(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.assign_bit_xor(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_xor(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_xor(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_xor(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_bit_xor(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_bit_xor(tmp); }

	// add
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, bits + 1> result; result.m_contents->add(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, bits + 1> result; result.m_contents->add(*begin_read(), src); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->add(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->add(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->add(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->add(*begin_read(), *(src.begin_read())); return result; }
	auto operator+(const dynamic_integer& src) const; // { return src + *this; }
	auto operator+(const dynamic_integer& src) const volatile; // { return src + *this; }
	auto operator+(const volatile dynamic_integer& src) const; // { return src + *this; }
	auto operator+(const volatile dynamic_integer& src) const volatile; // { return src + *this; }

	template <bool has_sign2, size_t bits2> const this_t& operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->add(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.add(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.add(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.add(*(src.begin_read())); }); return *this; }
	this_t& operator+=(const dynamic_integer& src); // { m_contents->add(src); return *this; }
	volatile this_t& operator+=(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { c.add(src); }); return *this; }
	this_t& operator+=(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return operator+=(tmp); }
	volatile this_t& operator+=(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return operator+=(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator+=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator+=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator+=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator+=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->add(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.add(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.add(*(src.begin_read())); }); }
	const this_t& pre_assign_add(const dynamic_integer& src); // { m_contents->add(src); return *this; }
	this_t pre_assign_add(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { c.add(src); }); }
	const this_t& pre_assign_add(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_add(tmp); }
	this_t pre_assign_add(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_add(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_add(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_add(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_add(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->add(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.add(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->add(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->add(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.add(*(src.begin_read())); }); }
	this_t post_assign_add(const dynamic_integer& src); // { this_t tmp(*this); m_contents->add(src); return tmp; }
	this_t post_assign_add(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { c.add(src); }); }
	this_t post_assign_add(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_add(tmp); }
	this_t post_assign_add(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_add(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_add(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_add(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_add(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }

	// subtract
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_extended<true, bits + 1> result; result.m_contents->subtract(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_extended<true, bits + 1> result; result.m_contents->subtract(*begin_read(), src); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-(tmp); }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-(tmp); }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<true, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->subtract(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<true, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->subtract(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<true, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->subtract(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<true, (bits > bits2 ? bits : bits2) + 1> result; result.m_contents->subtract(*begin_read(), *(src.begin_read())); return result; }
	auto operator-(const dynamic_integer& src) const; // { return src.inverse_subtract(*this); }
	auto operator-(const dynamic_integer& src) const volatile; // { return src.inverse_subtract(*this); }
	auto operator-(const volatile dynamic_integer& src) const; // { return src.inverse_subtract(*this); }
	auto operator-(const volatile dynamic_integer& src) const volatile; // { return src.inverse_subtract(*this); }

	auto operator-(const this_t& src) const { fixed_integer_extended<true, bits + 1> result; if (this == &src) result.clear(); else result.m_contents->subtract(*m_contents, *(src.m_contents)); return result; }
	auto operator-(const this_t& src) const volatile { fixed_integer_extended<true, bits + 1> result; result.m_contents->subtract(*begin_read(), *(src.m_contents)); return result; }
	auto operator-(const volatile this_t& src) const { fixed_integer_extended<true, bits + 1> result; result.m_contents->subtract(*m_contents, *(src.begin_read())); return result; }
	auto operator-(const volatile this_t& src) const volatile { fixed_integer_extended<true, bits + 1> result; if (this == &src) result.clear(); else result.m_contents->subtract(*begin_read(), *(src.begin_read())); return result; }

	template <bool has_sign2, size_t bits2> const this_t& operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->subtract(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.begin_read())); }); return *this; }
	this_t& operator-=(const dynamic_integer& src); // { m_contents->subtract(src); return *this; }
	volatile this_t& operator-=(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { c.subtract(src); }); return *this; }
	this_t& operator-=(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return operator-=(tmp); }
	volatile this_t& operator-=(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return operator-=(tmp); }

	this_t& operator-=(const this_t& src) { if (this == &src) clear(); else m_contents->subtract(*(src.m_contents)); return *this; }
	volatile this_t& operator-=(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents)); }); return *this; }
	this_t& operator-=(const volatile this_t& src) { m_contents->subtract(*(src.begin_read())); return *this; }
	volatile this_t& operator-=(const volatile this_t& src) volatile { if (this == &src) clear(); else write_retry_loop([&](content_t& c) { c.subtract(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator-=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator-=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator-=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator-=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->subtract(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read())); }); }
	const this_t& pre_assign_subtract(const dynamic_integer& src); // { m_contents->subtract(src); return *this; }
	this_t pre_assign_subtract(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { c.subtract(src); }); }
	const this_t& pre_assign_subtract(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_subtract(tmp); }
	this_t pre_assign_subtract(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_subtract(tmp); }

	const this_t& pre_assign_subtract(const this_t& src) { if (this == &src) clear(); else m_contents->subtract(*(src.m_contents)); return *this; }
	this_t pre_assign_subtract(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	const this_t& pre_assign_subtract(const volatile this_t& src) { m_contents->subtract(*(src.begin_read())); return *this; }
	this_t pre_assign_subtract(const volatile this_t& src) volatile { if (this == &src) { this_t result; clear(); result.clear(); return result; } return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.begin_read())); }); }
	this_t post_assign_subtract(const dynamic_integer& src); // { this_t tmp(*this); m_contents->subtract(src); return tmp; }
	this_t post_assign_subtract(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { c.subtract(src); }); }
	this_t post_assign_subtract(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_subtract(tmp); }
	this_t post_assign_subtract(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_subtract(tmp); }

	this_t post_assign_subtract(const this_t& src) { this_t tmp(*this); if (this == &src) clear(); else m_contents->subtract(*(src.m_contents)); return tmp; }
	this_t post_assign_subtract(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	this_t post_assign_subtract(const volatile this_t& src) { this_t tmp(*this); m_contents->subtract(*(src.begin_read())); return tmp; }
	this_t post_assign_subtract(const volatile this_t& src) volatile { if (this == &src) { return exchange(0); } return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }

	// inverse_subtract
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	auto inverse_subtract(const dynamic_integer& src) const; // { return src - *this; }
	auto inverse_subtract(const dynamic_integer& src) const volatile; // { return src - *this; }
	auto inverse_subtract(const volatile dynamic_integer& src) const; // { return src - *this; }
	auto inverse_subtract(const volatile dynamic_integer& src) const volatile; // { return src - *this; }

	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->subtract(src, *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(src, c); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.m_contents), *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.begin_read()), *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }
	void assign_inverse_subtract(const dynamic_integer& src); // { m_contents->inverse_subtract(src); return *this; }
	void assign_inverse_subtract(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { c.inverse_subtract(src); }); return *this; }
	void assign_inverse_subtract(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return operator-=(tmp); }
	void assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return operator-=(tmp); }

	void assign_inverse_subtract(const this_t& src) { if (this == &src) clear(); else m_contents->subtract(*(src.m_contents), *m_contents); }
	void assign_inverse_subtract(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	void assign_inverse_subtract(const volatile this_t& src) { m_contents->subtract(*(src.begin_read()), *m_contents); }
	void assign_inverse_subtract(const volatile this_t& src) volatile { if (this == &src) clear(); else write_retry_loop([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->subtract(src, *m_contents); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(src, c); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.m_contents), *m_contents); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.begin_read()), *m_contents); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }
	const this_t& pre_assign_inverse_subtract(const dynamic_integer& src); // { m_contents->inverse_subtract(src); return *this; }
	this_t pre_assign_inverse_subtract(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { c.inverse_subtract(src); }); }
	const this_t& pre_assign_inverse_subtract(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_inverse_subtract(tmp); }
	this_t pre_assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_inverse_subtract(tmp); }

	const this_t& pre_assign_inverse_subtract(const this_t& src) { if (this == &src) clear(); else m_contents->subtract(*(src.m_contents), *m_contents); return *this; }
	this_t pre_assign_inverse_subtract(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	const this_t& pre_assign_inverse_subtract(const volatile this_t& src) { m_contents->subtract(*(src.begin_read()), *m_contents); return *this; }
	this_t pre_assign_inverse_subtract(const volatile this_t& src) volatile { if (this == &src) { this_t result; clear(); result.clear(); return result; } return write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(src, *m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(src, c); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(*(src.m_contents), *m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->subtract(*(src.begin_read()), *m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }
	this_t post_assign_inverse_subtract(const dynamic_integer& src); // { this_t tmp(*this); m_contents->inverse_subtract(src); return tmp; }
	this_t post_assign_inverse_subtract(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { c.inverse_subtract(src); }); }
	this_t post_assign_inverse_subtract(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_inverse_subtract(tmp); }
	this_t post_assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_inverse_subtract(tmp); }

	this_t post_assign_inverse_subtract(const this_t& src) { this_t tmp(*this); if (this == &src) clear(); else m_contents->subtract(*(src.m_contents), *m_contents); return tmp; }
	this_t post_assign_inverse_subtract(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.m_contents), c); }); }
	this_t post_assign_inverse_subtract(const volatile this_t& src) { this_t tmp(*this); m_contents->subtract(*(src.begin_read()), *m_contents); return tmp; }
	this_t post_assign_inverse_subtract(const volatile this_t& src) volatile { if (this == &src) { return exchange(0); } return write_retry_loop_post([&](content_t& c) { c.subtract(*(src.begin_read()), c); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }

	// multiply
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*begin_read(), src); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*begin_read(), *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign | has_sign2, bits + bits2> result; result.m_contents->multiply(*begin_read(), *(src.begin_read())); return result; }
	auto operator*(const dynamic_integer& src) const; // { return src * *this; }
	auto operator*(const dynamic_integer& src) const volatile; // { return src * *this; }
	auto operator*(const volatile dynamic_integer& src) const; // { return src * *this; }
	auto operator*(const volatile dynamic_integer& src) const volatile; // { return src * *this; }

	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> const this_t& operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto operator*(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator*(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator*(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto operator*(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->multiply(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.multiply(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.multiply(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.multiply(*(src.begin_read())); }); return *this; }
	this_t& operator*=(const dynamic_integer& src); // { m_contents->multiply(src); return *this; }
	volatile this_t& operator*=(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { c.multiply(src); }); return *this; }
	this_t& operator*=(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return operator*=(tmp); }
	volatile this_t& operator*=(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return operator*=(tmp); }

	this_t& operator*=(const this_t& src) { if (this == &src) m_contents->multiply(); else m_contents->multiply(*(src.m_contents)); return *this; }
	volatile this_t& operator*=(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c.multiply(*(src.m_contents)); }); return *this; }
	this_t& operator*=(const volatile this_t& src) { m_contents->multiply(*(src.begin_read())); return *this; }
	volatile this_t& operator*=(const volatile this_t& src) volatile { if (this == &src) write_retry_loop([&](content_t& c) { c.multiply(); }); else write_retry_loop([&](content_t& c) { c.multiply(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <size_t bits2> this_t& operator*=(const fixed_integer_native_const<true, bits2, -1>& src) { assign_negative(); return *this; }
	template <size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<true, bits2, -1>& src) { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<true, bits2, -1>& src) volatile { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator*=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator*=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator*=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator*=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->multiply(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.multiply(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.begin_read())); }); }
	const this_t& pre_assign_multiply(const dynamic_integer& src); // { m_contents->multiply(src); return *this; }
	this_t pre_assign_multiply(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { c.multiply(src); }); }
	const this_t& pre_assign_multiply(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }
	this_t pre_assign_multiply(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<true, bits2, -1>& src) { assign_negative(); return *this; }
	template <size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>& src) { assign_negative(); return *this; }
	template <size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<true, bits2, -1>& src) volatile { return pre_assign_negative(); }
	template <size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_multiply(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_multiply(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_multiply(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_multiply(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->multiply(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.multiply(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->multiply(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->multiply(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.multiply(*(src.begin_read())); }); }
	this_t post_assign_multiply(const dynamic_integer& src); // { this_t tmp(*this); m_contents->multiply(src); return tmp; }
	this_t post_assign_multiply(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { c.multiply(src); }); }
	this_t post_assign_multiply(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_multiply(tmp); }
	this_t post_assign_multiply(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<true, bits2, -1>& src) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>& src) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<true, bits2, -1>& src) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_multiply(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_multiply(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_multiply(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_multiply(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }

	//   signed/signed  :  127 % -128 =  127 (8 bits, signed)
	//   signed/signed  :  127 %  127 =    0 (8 bits, signed)
	//   signed/signed  : -128 % -128 =    0 (8 bits, signed)
	//   signed/signed  : -128 %  127 =   -1 (8 bits, signed)
	//
	// unsigned/signed  :  255 % -128 =  127 (7 bits, unsigned)
	// unsigned/signed  :  255 %  127 =    1 (7 bits, unsigned)
	//
	//   signed/unsigned:  127 %  255 =  127 (8 bits, signed)
	//   signed/unsigned: -127 %  255 = -127 (8 bits, signed)
	//
	// unsigned/unsigned:  255 %  255 =    0 (8 bits, unsigned)

	// modulo
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(src); fixed_integer_native<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0)) > result(tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const volatile { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(src); fixed_integer_native<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0)) > result(tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%(tmp); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%(tmp); }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(tmp); return result; }
	auto operator%(const dynamic_integer& src) const; // { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	auto operator%(const dynamic_integer& src) const volatile; // { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	auto operator%(const volatile dynamic_integer& src) const; // { dynamic_integer tmp(src); return operator%(tmp); }
	auto operator%(const volatile dynamic_integer& src) const volatile; // { dynamic_integer tmp(src); return operator%(tmp); }

	auto operator%(const this_t& src) const { if (this == &src) { COGS_ASSERT(!!*this);  this_t tmp2; tmp2.clear(); return tmp2; } this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	auto operator%(const this_t& src) const volatile { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	auto operator%(const volatile this_t& src) const { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return tmp; }
	auto operator%(const volatile this_t& src) const volatile { if (this == &src) { COGS_ASSERT(!!*this);  this_t tmp2; tmp2.clear(); return tmp2; } this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return tmp; }

	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return zero_t(); }
	template <size_t bits2> auto operator%(const fixed_integer_native_const<true, bits2, -1>&) const { return zero_t(); }
	template <size_t bits2> auto operator%(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return zero_t(); }
	template <size_t bits2> auto operator%(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return zero_t(); }
	template <size_t bits2> auto operator%(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); return *this; }
	this_t& operator%=(const dynamic_integer& src); // { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	volatile this_t& operator%=(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
	this_t& operator%=(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return operator%=(tmp); }
	volatile this_t& operator%=(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return operator%=(tmp); }

	this_t& operator%=(const this_t& src) { if (this == &src) { COGS_ASSERT(!!*this); clear(); } else m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	volatile this_t& operator%=(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
	this_t& operator%=(const volatile this_t& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	volatile this_t& operator%=(const volatile this_t& src) volatile { if (this == &src) { this_t tmp(exchange(zero_t())); COGS_ASSERT(!!tmp); } else write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return *this; }
	template <size_t bits2> this_t& operator%=(const fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return *this; }
	template <size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator%=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator%=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t& operator%=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile this_t& operator%=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }
	const this_t& pre_assign_modulo(const dynamic_integer& src); // { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	this_t pre_assign_modulo(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	const this_t& pre_assign_modulo(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_modulo(tmp); }
	this_t pre_assign_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_modulo(tmp); }

	const this_t& pre_assign_modulo(const this_t& src) { if (this == &src) { COGS_ASSERT(!!*this); clear(); } else m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	this_t pre_assign_modulo(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	const this_t& pre_assign_modulo(const volatile this_t& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	this_t pre_assign_modulo(const volatile this_t& src) volatile { if (this == &src) { this_t tmp; tmp.clear(); this_t tmp2(exchange(zero_t())); COGS_ASSERT(!!tmp2); return tmp; } return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return 0; }
	template <size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return 0; }
	template <size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }
	this_t post_assign_modulo(const dynamic_integer& src); // { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	this_t post_assign_modulo(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	this_t post_assign_modulo(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_modulo(tmp); }
	this_t post_assign_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_modulo(tmp); }

	this_t post_assign_modulo(const this_t& src) { this_t tmp(*this); if (this == &src) { COGS_ASSERT(!!*this); clear(); } else m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	this_t post_assign_modulo(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	this_t post_assign_modulo(const volatile this_t& src) { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return tmp; }
	this_t post_assign_modulo(const volatile this_t& src) volatile { if (this == &src) { this_t tmp(exchange(zero_t())); COGS_ASSERT(!!tmp); return tmp; } return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }

	// inverse_modulo
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const; // { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const volatile; // { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const; // { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const volatile; // { return src % *this; }

	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
	}

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		write_retry_loop([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
	}

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		write_retry_loop([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	void assign_inverse_modulo(const dynamic_integer& src); // { *this = src % *this; }
	void assign_inverse_modulo(const dynamic_integer& src) volatile; // { write_retry_loop([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); return *this; }
	void assign_inverse_modulo(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return assign_inverse_modulo(tmp); }
	void assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return assign_inverse_modulo(tmp); }

	void assign_inverse_modulo(const this_t& src)
	{
		if (this == &src)
			clear();
		else
		{
			this_t remainder(src);
			remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
			*this = remainder;
		}
	}

	void assign_inverse_modulo(const this_t& src) volatile
	{
		this_t remainder(src);
		write_retry_loop([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	void assign_inverse_modulo(const volatile this_t& src)
	{
		this_t remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
	}

	void assign_inverse_modulo(const volatile this_t& src) volatile
	{
		if (this == &src)
			clear();
		else
		{
			this_t remainder(src);
			write_retry_loop([&](content_t& c) {
				remainder.m_contents->divide_whole_and_assign_modulo(c);
				c = remainder.m_contents;
			}, [&]() {
				remainder = src;
			});
		}
	}


	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto tmp = exchange(0); COGS_ASSERT(!!tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto tmp = exchange(0); COGS_ASSERT(!!tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	const this_t& pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_pre([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2>
	const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_pre([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	const this_t& pre_assign_inverse_modulo(const dynamic_integer& src); // { *this = src % *this; return *this; }
	this_t pre_assign_inverse_modulo(const dynamic_integer& src) volatile; // { return write_retry_loop_pre([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); }
	const this_t& pre_assign_inverse_modulo(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }
	this_t pre_assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }

	const this_t& pre_assign_inverse_modulo(const this_t& src)
	{
		if (this == &src)
			clear();
		else
		{
			this_t remainder(src);
			remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
			*this = remainder;
		}
		return *this;
	}

	this_t pre_assign_inverse_modulo(const this_t& src) volatile
	{
		this_t remainder(src);
		return write_retry_loop_pre([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	const this_t& pre_assign_inverse_modulo(const volatile this_t& src)
	{
		this_t remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return *this;
	}

	this_t pre_assign_inverse_modulo(const volatile this_t& src) volatile
	{
		if (this == &src)
		{
			this_t tmp;
			tmp.clear();
			clear();
			return tmp;
		}
		else
		{
			this_t remainder(src);
			return write_retry_loop_pre([&](content_t& c) {
				remainder.m_contents->divide_whole_and_assign_modulo(c);
				c = remainder.m_contents;
			}, [&]() {
				remainder = src;
			});
		}
	}

	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { auto tmp = exchange(0); COGS_ASSERT(!!tmp); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { auto tmp = exchange(0); COGS_ASSERT(!!tmp); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const this_t& pre_assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t pre_assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this = src % *this; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		this_t tmp(*this);
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_post([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		this_t tmp(*this);
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_post([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	this_t post_assign_inverse_modulo(const dynamic_integer& src); // { this_t tmp(*this); *this = src % *this; return tmp; }
	this_t post_assign_inverse_modulo(const dynamic_integer& src) volatile; // { return write_retry_loop_post([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); }
	this_t post_assign_inverse_modulo(const volatile dynamic_integer& src); // { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }
	this_t post_assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }


	this_t post_assign_inverse_modulo(const this_t& src)
	{
		this_t tmp(*this);
		if (this == &src)
			clear();
		else
		{
			this_t remainder(src);
			remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
			*this = remainder;
		}
		return tmp;
	}

	this_t post_assign_inverse_modulo(const this_t& src) volatile
	{
		this_t remainder(src);
		return write_retry_loop_post([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	this_t post_assign_inverse_modulo(const volatile this_t& src)
	{
		this_t tmp(*this);
		this_t remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*m_contents);
		*this = remainder;
		return tmp;
	}

	this_t post_assign_inverse_modulo(const volatile this_t& src) volatile
	{
		if (this == &src)
			return exchange(0);

		this_t remainder(src);
		return write_retry_loop_post([&](content_t& c) {
			remainder.m_contents->divide_whole_and_assign_modulo(c);
			c = remainder.m_contents;
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { this_t tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { auto result = exchange(0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { auto result = exchange(0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > this_t post_assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }

	// divide
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const dynamic_integer& src) const; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const dynamic_integer& src) const volatile; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const volatile dynamic_integer& src) const; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const volatile dynamic_integer& src) const volatile; // { return fraction<this_t, dynamic_integer>(*this, src); }

	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }

	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	this_t& operator/=(const dynamic_integer& src); // { assign_divide_whole(src); return *this; }
	this_t& operator/=(const volatile dynamic_integer& src); // { assign_divide_whole(src); return *this; }
	volatile this_t& operator/=(const dynamic_integer& src) volatile; // { assign_divide_whole(src); return *this; }
	volatile this_t& operator/=(const volatile dynamic_integer& src) volatile; // { assign_divide_whole(src); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> this_t& operator/=(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator/=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator/=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator/=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator/=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	const this_t& pre_assign_divide(const dynamic_integer& src);// { return pre_assign_divide_whole(src); }
	const this_t& pre_assign_divide(const volatile dynamic_integer& src);// { return pre_assign_divide_whole(src); }
	this_t pre_assign_divide(const dynamic_integer& src) volatile;// { return pre_assign_divide_whole(src); }
	this_t pre_assign_divide(const volatile dynamic_integer& src) volatile;// { return pre_assign_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	this_t post_assign_divide(const dynamic_integer& src);// { return post_assign_divide_whole(src); }
	this_t post_assign_divide(const volatile dynamic_integer& src);// { return post_assign_divide_whole(src); }
	this_t post_assign_divide(const dynamic_integer& src) volatile;// { return post_assign_divide_whole(src); }
	this_t post_assign_divide(const volatile dynamic_integer& src) volatile;// { return post_assign_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }

	// reciprocal
	auto reciprocal() const { return fraction<one_t, this_t>(one_t(), *this); }
	auto reciprocal() const volatile { return fraction<one_t, this_t>(one_t(), *this); }
	void assign_reciprocal() { m_contents->assign_reciprocal(); }
	void assign_reciprocal() volatile { try_write_retry_loop([&](content_t& c) { return c.assign_reciprocal(); }); }
	const this_t& pre_assign_reciprocal() { assign_reciprocal(); return *this; }
	this_t pre_assign_reciprocal() volatile { return try_write_retry_loop_pre([&](content_t& c) { return c.assign_reciprocal(); }); }
	this_t post_assign_reciprocal() { this_t result(*this); assign_reciprocal(); return result; }
	this_t post_assign_reciprocal() volatile { return try_write_retry_loop_post([&](content_t& c) { return c.assign_reciprocal(); }); }

	// inverse_divide
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const dynamic_integer& src) const;// { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const dynamic_integer& src) const volatile;// { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const volatile dynamic_integer& src) const;// { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const volatile dynamic_integer& src) const volatile;// { return fraction<dynamic_integer, this_t>(src, *this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const dynamic_integer& src);// { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const volatile dynamic_integer& src);// { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const dynamic_integer& src) volatile;// { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const volatile dynamic_integer& src) volatile;// { assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	const this_t& pre_assign_inverse_divide(const dynamic_integer& src);// { return pre_assign_inverse_divide_whole(src); }
	const this_t& pre_assign_inverse_divide(const volatile dynamic_integer& src);// { return pre_assign_inverse_divide_whole(src); }
	this_t pre_assign_inverse_divide(const dynamic_integer& src) volatile;// { return pre_assign_inverse_divide_whole(src); }
	this_t pre_assign_inverse_divide(const volatile dynamic_integer& src) volatile;// { return pre_assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	this_t post_assign_inverse_divide(const dynamic_integer& src);// { return post_assign_inverse_divide_whole(src); }
	this_t post_assign_inverse_divide(const volatile dynamic_integer& src);// { return post_assign_inverse_divide_whole(src); }
	this_t post_assign_inverse_divide(const dynamic_integer& src) volatile;// { return post_assign_inverse_divide_whole(src); }
	this_t post_assign_inverse_divide(const volatile dynamic_integer& src) volatile;// { return post_assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); return exchange(0); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }

	// floor
	const this_t& floor() const { return *this; }
	this_t floor() const volatile { return *this; }
	void assign_floor() { }
	void assign_floor() volatile { }
	const this_t& pre_assign_floor() { return *this; }
	this_t pre_assign_floor() volatile { return *this; }
	const this_t& post_assign_floor() { return *this; }
	this_t post_assign_floor() volatile { return *this; }

	// ceil
	const this_t& ceil() const { return *this; }
	this_t ceil() const volatile { return *this; }
	void assign_ceil() { }
	void assign_ceil() volatile { }
	const this_t& pre_assign_ceil() { return *this; }
	this_t pre_assign_ceil() volatile { return *this; }
	const this_t& post_assign_ceil() { return *this; }
	this_t post_assign_ceil() volatile { return *this; }

	// fractional_part
	auto fractional_part() const { return zero_t(); }
	auto fractional_part() const volatile { return zero_t(); }
	void assign_fractional_part() { clear(); }
	void assign_fractional_part() volatile { clear(); }
	this_t pre_assign_fractional_part() { clear(); return 0; }
	this_t pre_assign_fractional_part() volatile { clear(); return 0; }
	this_t post_assign_fractional_part() { this_t tmp(*this); clear(); return tmp; }
	this_t post_assign_fractional_part() volatile { return exchange(0); }

	//   signed/signed  :  127 / -1 = -127 (8 bits, signed)
	//   signed/signed  :  127 /  1 =  127 (8 bits, signed)
	//   signed/signed  : -128 / -1 =  128 (9 bits, signed) ***
	//   signed/signed  : -128 /  1 = -128 (8 bits, signed)
	//
	// unsigned/signed  :  255 / -1 = -255 (9 bits, signed) ***
	// unsigned/signed  :  255 /  1 =  255 (9 bits, signed) ***
	//
	//   signed/unsigned:  127 /  1 =  127 (8 bits, signed)
	//   signed/unsigned: -127 /  1 = -127 (8 bits, signed)
	//
	// unsigned/unsigned:  255 /  1 =  255 (8 bits, unsigned)

	// divide_whole
	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		return result;
	}

	auto divide_whole(const dynamic_integer& src) const;
	//{
	//	fixed_integer_extended<true, bits + 1> result;
	//	this_t remainder(*this);
	//	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
	//	return result;
	//}

	auto divide_whole(const dynamic_integer& src) const volatile;
	//{
	//	fixed_integer_extended<true, bits + 1> result;
	//	this_t remainder(*this);
	//	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
	//	return result;
	//}

	auto divide_whole(const volatile dynamic_integer& src) const;// { dynamic_integer tmp(src); return divide_whole(tmp); }
	auto divide_whole(const volatile dynamic_integer& src) const volatile;// { dynamic_integer tmp(src); return divide_whole(tmp); }


	auto divide_whole(const this_t& src) const
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			result = one_t();
		}
		else
		{
			this_t remainder(*this);
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		}
		return result;
	}

	auto divide_whole(const this_t& src) const volatile
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		return result;
	}

	auto divide_whole(const volatile this_t& src) const
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		return result;
	}

	auto divide_whole(const volatile this_t& src) const volatile
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			result = one_t();
		}
		else
		{
			this_t remainder(*this);
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		}
		return result;
	}

	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto divide_whole(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto divide_whole(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(src, &*m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(src, &c); }); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }
	void assign_divide_whole(const dynamic_integer& src);// { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); }
	void assign_divide_whole(const volatile dynamic_integer& src);// { dynamic_integer tmp(src); assign_divide_whole(tmp); }
	void assign_divide_whole(const dynamic_integer& src) volatile;// { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	void assign_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); assign_divide_whole(tmp); }

	void assign_divide_whole(const this_t& src) { if (this == &src) { COGS_ASSERT(!!*this); *this = one_t(); } else { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); } }
	void assign_divide_whole(const volatile this_t& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); }
	void assign_divide_whole(const this_t& src) volatile { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	void assign_divide_whole(const volatile this_t& src) volatile { if (this == &src) { this_t tmp(exchange(one_t())); COGS_ASSERT(!!tmp); } else write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }

	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { }
	template <size_t bits2> void assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(src, &*m_contents); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(src, &c); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }
	const this_t& pre_assign_divide_whole(const dynamic_integer& src);// { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return *this; }
	const this_t& pre_assign_divide_whole(const volatile dynamic_integer& src);// { dynamic_integer tmp(src); return pre_assign_divide_whole(tmp); }
	this_t pre_assign_divide_whole(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	this_t pre_assign_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_divide_whole(tmp); }

	const this_t& pre_assign_divide_whole(const this_t& src) { if (this == &src) { COGS_ASSERT(!!*this); *this = one_t(); } else { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); } return *this; }
	const this_t& pre_assign_divide_whole(const volatile this_t& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); return *this; }
	this_t pre_assign_divide_whole(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	this_t pre_assign_divide_whole(const volatile this_t& src) volatile { if (this == &src) { this_t tmp2 = one_t(); this_t tmp = exchange(tmp2); COGS_ASSERT(!!tmp); return tmp2; } return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(src, &*m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(src, &c); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }
	this_t post_assign_divide_whole(const dynamic_integer& src);// { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return tmp; }
	this_t post_assign_divide_whole(const volatile dynamic_integer& src);// { dynamic_integer tmp(src); return post_assign_divide_whole(tmp); }
	this_t post_assign_divide_whole(const dynamic_integer& src) volatile;// { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	this_t post_assign_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_divide_whole(tmp); }

	this_t post_assign_divide_whole(const this_t& src) { this_t tmp(*this); if (this == &src) { COGS_ASSERT(!!tmp); *this = one_t(); } else { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); } return tmp; }
	this_t post_assign_divide_whole(const volatile this_t& src) { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents); return tmp; }
	this_t post_assign_divide_whole(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
	this_t post_assign_divide_whole(const volatile this_t& src) volatile { if (this == &src) { this_t tmp(exchange(one_t())); COGS_ASSERT(!!tmp); return tmp; } return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c); }); }

	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> const this_t& post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }

	// inverse_divide_whole
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const;// { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const volatile;// { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const;// { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const volatile;// { return src.divide_whole(*this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2>
	void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		this_t tmp(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), &*m_contents);
	}

	template <bool has_sign2, size_t bits2>
	void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		this_t tmp(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), &*m_contents);
	}

	template <bool has_sign2, size_t bits2>
	void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		write_retry_loop([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2>
	void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		write_retry_loop([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	void assign_inverse_divide_whole(const dynamic_integer& src);// { *this = src.divide_whole(*this); }
	void assign_inverse_divide_whole(const volatile dynamic_integer& src);// { *this = src.divide_whole(*this); }
	void assign_inverse_divide_whole(const dynamic_integer& src) volatile;// { write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }
	void assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); assign_inverse_divide(tmp); }

	void assign_inverse_divide_whole(const this_t& src)
	{
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			*this = one_t();
		}
		else
		{
			this_t remainder(src);
			this_t tmp(*this);
			remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), &*m_contents);
		}
	}

	void assign_inverse_divide_whole(const volatile this_t& src)
	{
		this_t remainder(src);
		this_t tmp(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), &*m_contents);
	}

	void assign_inverse_divide_whole(const this_t& src) volatile
	{
		this_t remainder(src);
		write_retry_loop([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	void assign_inverse_divide_whole(const volatile this_t& src) volatile
	{
		if (this == &src)
		{
			this_t tmp(exchange(one_t()));
			COGS_ASSERT(!!tmp);
		}
		else
		{
			this_t remainder(src);
			write_retry_loop([&](content_t& c)
			{
				content_t tmp(c);
				remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
			}, [&]() {
				remainder = src;
			});
		}
	}

	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_pre([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_pre([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	const this_t& pre_assign_inverse_divide_whole(const dynamic_integer& src);// { assign_inverse_divide(src); return *this; }
	const this_t& pre_assign_inverse_divide_whole(const volatile dynamic_integer& src);// { assign_inverse_divide(src); return *this; }
	this_t pre_assign_inverse_divide_whole(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }
	this_t pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_inverse_divide(tmp); }

	const this_t& pre_assign_inverse_divide_whole(const this_t& src) { assign_inverse_divide(src); return *this; }
	const this_t& pre_assign_inverse_divide_whole(const volatile this_t& src) { assign_inverse_divide(src); return *this; }

	this_t pre_assign_inverse_divide_whole(const this_t& src) volatile
	{
		this_t remainder(src);
		return write_retry_loop_pre([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	this_t pre_assign_inverse_divide_whole(const volatile this_t& src) volatile
	{
		if (this == &src)
		{
			this_t one = one_t();
			this_t tmp = exchange(one);
			COGS_ASSERT(!!tmp);
			return one;
		}
		else
		{
			this_t remainder(src);
			return write_retry_loop_pre([&](content_t& c)
			{
				content_t tmp(c);
				remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
			}, [&]() {
				remainder = src;
			});
		}
	}

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		this_t remainder(src);
		return write_retry_loop_post([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<has_sign2, bits2> remainder(src);
		return write_retry_loop_post([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	this_t post_assign_inverse_divide_whole(const dynamic_integer& src);// { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	this_t post_assign_inverse_divide_whole(const volatile dynamic_integer& src);// { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	this_t post_assign_inverse_divide_whole(const dynamic_integer& src) volatile;// { return write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }
	this_t post_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_inverse_divide(tmp); }

	this_t post_assign_inverse_divide_whole(const this_t& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	this_t post_assign_inverse_divide_whole(const volatile this_t& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }

	this_t post_assign_inverse_divide_whole(const this_t& src) volatile
	{
		this_t remainder(src);
		return write_retry_loop_post([&](content_t& c)
		{
			content_t tmp(c);
			remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
		}, [&]() {
			remainder = src;
		});
	}

	this_t post_assign_inverse_divide_whole(const volatile this_t& src) volatile
	{
		if (this == &src)
		{
			this_t one = one_t();
			this_t tmp = exchange(one);
			COGS_ASSERT(!!tmp);
			return tmp;
		}
		else
		{
			this_t remainder(src);
			return write_retry_loop_post([&](content_t& c)
			{
				content_t tmp(c);
				remainder.m_contents->divide_whole_and_assign_modulo(tmp, &c);
			}, [&]() {
				remainder = src;
			});
		}
	}

	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { this_t result = exchange(0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { this_t result = exchange(0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }

	// divide_whole_and_modulo
	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divided;
		this_t remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(divided.m_contents));
		fixed_integer<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0))> remainderResult(remainder);
		return std::make_pair(divided, remainderResult);
	}

	auto divide_whole_and_modulo(const dynamic_integer& src) const;

	auto divide_whole_and_modulo(const dynamic_integer& src) const volatile;

	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const;// { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile;// { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return std::pair<this_t, zero_t>(*this, zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const { return std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }


	// inverse_divide_whole_and_inverse_modulo
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const;// { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const volatile;// { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const;// { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const volatile;// { return src.divide_whole_and_modulo(*this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }


	// divide_whole_and_assign_modulo
	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src)
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		m_contents->divide_whole_and_assign_modulo(src, &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(src, &*(result.m_contents));
		});
		return result;
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_assign_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		});
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		fixed_integer_extended<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		});
		return result;
	}

	auto divide_whole_and_assign_modulo(const dynamic_integer& src);
	auto divide_whole_and_assign_modulo(const dynamic_integer& src) volatile;

	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src);// { dynamic_integer tmp(src); return divide_whole_and_assign_modulo(tmp); }
	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return divide_whole_and_assign_modulo(tmp); }


	auto divide_whole_and_assign_modulo(const this_t& src)
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		if (this == &src)
		{
			result = one_t();
			COGS_ASSERT(!!*this);
			clear();
		}
		else
		{
			m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		}

		return result;
	}

	auto divide_whole_and_assign_modulo(const this_t& src) volatile
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		});
		return result;
	}

	auto divide_whole_and_assign_modulo(const volatile this_t& src)
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
		return result;
	}

	auto divide_whole_and_assign_modulo(const volatile this_t& src) volatile
	{
		fixed_integer_extended<has_sign, bits + (has_sign ? 1 : 0)> result;
		if (this == &src)
		{
			result = one_t();
			this_t tmp(exchange(zero_t()));
			COGS_ASSERT(!!tmp);
		}
		else
		{
			write_retry_loop([&](content_t& c)
			{
				c.divide_whole_and_assign_modulo(*(src.begin_read()), &*(result.m_contents));
			});
		}
		return result;
	}


	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) volatile { this_t tmp(exchange(zero_t())); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { this_t tmp(exchange(zero_t())); return -tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }

	// modulo_and_assign_divide_whole
	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src)
	{
		content_t remainder(*m_contents);
		remainder.divide_whole_and_assign_modulo(src, &*m_contents);
		fixed_integer_native<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0)) > result(remainder);
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.divide_whole_and_assign_modulo(src, &c);
		});
		fixed_integer_native<has_sign, (bits2 - ((!has_sign && has_sign2) ? 1 : 0)) > result = remainder;
		return result;
	}

	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return modulo_and_assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		content_t remainder(*m_contents);
		remainder.divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents);
		fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(remainder);
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c);
		});
		fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result = remainder;
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		content_t remainder(*m_contents);
		remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents);
		fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result(remainder);
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &c);
		});
		fixed_integer<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> result = remainder;
		return result;
	}

	auto modulo_and_assign_divide_whole(const dynamic_integer& src);
	//{
	//	this_t remainder(*this);
	//	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents);
	//	return remainder;
	//}

	auto modulo_and_assign_divide_whole(const dynamic_integer& src) volatile;
	//{
	//	this_t remainder;
	//	write_retry_loop([&](content_t& c)
	//	{
	//		*(remainder.m_contents) = c;
	//		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &c);
	//	});
	//	return remainder;
	//}

	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src);// { dynamic_integer tmp(src); return modulo_and_assign_divide_whole(tmp); }
	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return modulo_and_assign_divide_whole(tmp); }

	auto modulo_and_assign_divide_whole(const this_t& src)
	{
		this_t remainder;
		if (this == &src)
		{
			remainder.clear();
			*this = one_t();
		}
		else
		{
			remainder = *this;
			remainder.divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents);
		}
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const this_t& src) volatile
	{
		this_t remainder;
		write_retry_loop([&](content_t& c)
		{
			*(remainder.m_contents) = c;
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &c);
		});
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const volatile this_t& src)
	{
		this_t remainder(*this);
		remainder.divide_whole_and_assign_modulo(*(src.begin_read()), &*m_contents);
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const volatile this_t& src) volatile
	{
		this_t remainder;
		if (this == &src)
		{
			this_t one = one_t();
			this_t tmp = exchange(one);
			remainder.clear();
		}
		else
		{
			write_retry_loop([&](content_t& c)
			{
				*(remainder.m_contents) = c;
				remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), &c);
			});
		}
		return remainder;
	}


	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }

	// gcd
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->gcd(src); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->gcd(src); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> gcd(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->gcd(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> gcd(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->gcd(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->gcd(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->gcd(*(src.begin_read())); }
	auto gcd(const dynamic_integer& src) const;// { return src.gcd(*this); }
	auto gcd(const dynamic_integer& src) const volatile;// { return src.gcd(*this); }
	auto gcd(const volatile dynamic_integer& src) const;// { return src.gcd(*this); }
	auto gcd(const volatile dynamic_integer& src) const volatile;// { return src.gcd(*this); }

	fixed_integer_extended<false, bits> gcd(const this_t& src) const { if (this == &src) return abs(); return m_contents->gcd(*(src.m_contents)); }
	fixed_integer_extended<false, bits> gcd(const this_t& src) const volatile { return begin_read()->gcd(*(src.m_contents)); }
	fixed_integer_extended<false, bits> gcd(const volatile this_t& src) const { return m_contents->gcd(*(src.begin_read())); }
	fixed_integer_extended<false, bits> gcd(const volatile this_t& src) const volatile { if (this == &src) return abs(); return begin_read()->gcd(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return one_t(); }
	template <size_t bits2> auto gcd(const fixed_integer_native_const<true, bits2, -1>&) const { return one_t(); }
	template <size_t bits2> auto gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return one_t(); }
	template <size_t bits2> auto gcd(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return one_t(); }
	template <size_t bits2> auto gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }

	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { *this = gcd(src); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.gcd(src); }); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { *this = gcd(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = gcd(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); }
	void assign_gcd(const dynamic_integer& src);// { *this = src.gcd(*this); }
	void assign_gcd(const volatile dynamic_integer& src);// { *this = src.gcd(*this); }
	void assign_gcd(const dynamic_integer& src) volatile;// { *this = src.gcd(*this); }
	void assign_gcd(const volatile dynamic_integer& src) volatile;// { *this = src.gcd(*this); }

	void assign_gcd(const this_t& src) { if (this != &src) *this = gcd(*(src.m_contents)); }
	void assign_gcd(const volatile this_t& src) { *this = gcd(*(src.begin_read())); }
	void assign_gcd(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	void assign_gcd(const volatile this_t& src) volatile { if (this != &src) write_retry_loop([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.gcd(src); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); }
	const this_t& pre_assign_gcd(const dynamic_integer& src);// { assign_gcd(src); return *this; }
	const this_t& pre_assign_gcd(const volatile dynamic_integer& src);// { assign_gcd(src); return *this; }
	this_t pre_assign_gcd(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t& c) { c = src.gcd(c); }); }
	this_t pre_assign_gcd(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_gcd(tmp); }

	const this_t& pre_assign_gcd(const this_t& src) { if (this == &src) assign_abs(); else assign_gcd(src); return *this; }
	const this_t& pre_assign_gcd(const volatile this_t& src) { assign_gcd(src); return *this; }
	this_t pre_assign_gcd(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	this_t pre_assign_gcd(const volatile this_t& src) volatile { if (this == &src) return pre_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.gcd(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); }
	this_t post_assign_gcd(const dynamic_integer& src);// { this_t tmp(*this); assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const volatile dynamic_integer& src);// { this_t tmp(*this); assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const dynamic_integer& src) volatile;// { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	this_t post_assign_gcd(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_gcd(tmp); }

	this_t post_assign_gcd(const this_t& src) { this_t tmp(*this); if (this == &src) assign_abs(); else assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const volatile this_t& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.gcd(*(src.m_contents)); }); }
	this_t post_assign_gcd(const volatile this_t& src) volatile { if (this == &src) return post_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.gcd(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }

	// lcm
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->lcm(src); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->lcm(src); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->lcm(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->lcm(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->lcm(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, bits + bits2> lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->lcm(*(src.begin_read())); }
	auto lcm(const dynamic_integer& src) const;// { return src.lcm(*this); }
	auto lcm(const dynamic_integer& src) const volatile;// { return src.lcm(*this); }
	auto lcm(const volatile dynamic_integer& src) const;// { return src.lcm(*this); }
	auto lcm(const volatile dynamic_integer& src) const volatile;// { return src.lcm(*this); }

	fixed_integer_extended<false, bits + bits> lcm(const this_t& src) const { if (this == &src) return abs(); return m_contents->lcm(*(src.m_contents)); }
	fixed_integer_extended<false, bits + bits> lcm(const this_t& src) const volatile { return begin_read()->lcm(*(src.m_contents)); }
	fixed_integer_extended<false, bits + bits> lcm(const volatile this_t& src) const { return m_contents->lcm(*(src.begin_read())); }
	fixed_integer_extended<false, bits + bits> lcm(const volatile this_t& src) const volatile { if (this == &src) return abs(); return begin_read()->lcm(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return abs(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const { return abs(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return abs(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return abs(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { *this = lcm(src); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lcm(src); }); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { *this = lcm(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = lcm(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); }
	void assign_lcm(const dynamic_integer& src);// { *this = src.lcm(*this); }
	void assign_lcm(const volatile dynamic_integer& src);// { *this = src.lcm(*this); }
	void assign_lcm(const dynamic_integer& src) volatile;// { *this = src.lcm(*this); }
	void assign_lcm(const volatile dynamic_integer& src) volatile;// { *this = src.lcm(*this); }

	void assign_lcm(const this_t& src) { if (this != &src) *this = lcm(*(src.m_contents)); }
	void assign_lcm(const volatile this_t& src) { *this = lcm(*(src.begin_read())); }
	void assign_lcm(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	void assign_lcm(const volatile this_t& src) volatile { if (this != &src) write_retry_loop([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { assign_abs(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_abs(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lcm(src); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); }
	const this_t& pre_assign_lcm(const dynamic_integer& src);// { assign_lcm(src); return *this; }
	const this_t& pre_assign_lcm(const volatile dynamic_integer& src);// { assign_lcm(src); return *this; }
	this_t pre_assign_lcm(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t& c) { c = src.lcm(c); }); }
	this_t pre_assign_lcm(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_lcm(tmp); }

	const this_t& pre_assign_lcm(const this_t& src) { if (this == &src) assign_abs(); else assign_lcm(src); return *this; }
	const this_t& pre_assign_lcm(const volatile this_t& src) { assign_lcm(src); return *this; }
	this_t pre_assign_lcm(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	this_t pre_assign_lcm(const volatile this_t& src) volatile { if (this == &src) return pre_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return pre_assign_abs(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return pre_assign_abs(); }
	template <size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); return *this; }
	template <size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); return *this; }
	template <size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_abs(); }
	template <size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lcm(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); }
	this_t post_assign_lcm(const dynamic_integer& src);// { this_t tmp(*this); assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const volatile dynamic_integer& src);// { this_t tmp(*this); assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const dynamic_integer& src) volatile;// { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	this_t post_assign_lcm(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_lcm(tmp); }

	this_t post_assign_lcm(const this_t& src) { this_t tmp(*this); if (this == &src) assign_abs(); else assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const volatile this_t& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lcm(*(src.m_contents)); }); }
	this_t post_assign_lcm(const volatile this_t& src) volatile { if (this == &src) return post_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.lcm(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return post_assign_abs(); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return post_assign_abs(); }
	template <size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_abs(); }
	template <size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }

	// greater
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->greater(src); }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->greater(src); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> greater(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->greater(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> greater(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->greater(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->greater(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->greater(*(src.begin_read())); }
	auto greater(const dynamic_integer& src) const;// { return src.greater(*this); }
	auto greater(const dynamic_integer& src) const volatile;// { return src.greater(*this); }
	auto greater(const volatile dynamic_integer& src) const;// { return src.greater(*this); }
	auto greater(const volatile dynamic_integer& src) const volatile;// { return src.greater(*this); }

	fixed_integer_extended<false, bits> greater(const this_t& src) const { if (this == &src) return abs(); return m_contents->greater(*(src.m_contents)); }
	fixed_integer_extended<false, bits> greater(const this_t& src) const volatile { return begin_read()->greater(*(src.m_contents)); }
	fixed_integer_extended<false, bits> greater(const volatile this_t& src) const { return m_contents->greater(*(src.begin_read())); }
	fixed_integer_extended<false, bits> greater(const volatile this_t& src) const volatile { if (this == &src) return abs(); return begin_read()->greater(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }

	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { *this = greater(src); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.greater(src); }); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { *this = greater(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = greater(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.greater(*(src.begin_read())); }); }
	void assign_greater(const dynamic_integer& src);// { *this = src.greater(*this); }
	void assign_greater(const volatile dynamic_integer& src);// { *this = src.greater(*this); }
	void assign_greater(const dynamic_integer& src) volatile;// { *this = src.greater(*this); }
	void assign_greater(const volatile dynamic_integer& src) volatile;// { *this = src.greater(*this); }

	void assign_greater(const this_t& src) { if (this != &src) *this = greater(*(src.m_contents)); }
	void assign_greater(const volatile this_t& src) { *this = greater(*(src.begin_read())); }
	void assign_greater(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	void assign_greater(const volatile this_t& src) volatile { if (this != &src) write_retry_loop([&](content_t& c) { c = c.greater(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.greater(src); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.begin_read())); }); }
	const this_t& pre_assign_greater(const dynamic_integer& src);// { assign_greater(src); return *this; }
	const this_t& pre_assign_greater(const volatile dynamic_integer& src);// { assign_greater(src); return *this; }
	this_t pre_assign_greater(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t& c) { c = src.greater(c); }); }
	this_t pre_assign_greater(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_greater(tmp); }

	const this_t& pre_assign_greater(const this_t& src) { if (this == &src) assign_abs(); else assign_greater(src); return *this; }
	const this_t& pre_assign_greater(const volatile this_t& src) { assign_greater(src); return *this; }
	this_t pre_assign_greater(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	this_t pre_assign_greater(const volatile this_t& src) volatile { if (this == &src) return pre_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.greater(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.begin_read())); }); }
	this_t post_assign_greater(const dynamic_integer& src);// { this_t tmp(*this); assign_greater(src); return tmp; }
	this_t post_assign_greater(const volatile dynamic_integer& src);// { this_t tmp(*this); assign_greater(src); return tmp; }
	this_t post_assign_greater(const dynamic_integer& src) volatile;// { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.greater(t); }); }
	this_t post_assign_greater(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_greater(tmp); }

	this_t post_assign_greater(const this_t& src) { this_t tmp(*this); if (this == &src) assign_abs(); else assign_greater(src); return tmp; }
	this_t post_assign_greater(const volatile this_t& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	this_t post_assign_greater(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.greater(*(src.m_contents)); }); }
	this_t post_assign_greater(const volatile this_t& src) volatile { if (this == &src) return post_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.greater(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }

	// lesser
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->lesser(src); }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->lesser(src); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> lesser(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->lesser(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> lesser(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->lesser(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->lesser(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> fixed_integer_extended<false, ((bits < bits2) ? bits : bits2)> lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->lesser(*(src.begin_read())); }
	auto lesser(const dynamic_integer& src) const;// { return src.lesser(*this); }
	auto lesser(const dynamic_integer& src) const volatile;// { return src.lesser(*this); }
	auto lesser(const volatile dynamic_integer& src) const;// { return src.lesser(*this); }
	auto lesser(const volatile dynamic_integer& src) const volatile;// { return src.lesser(*this); }

	fixed_integer_extended<false, bits> lesser(const this_t& src) const { if (this == &src) return abs(); return m_contents->lesser(*(src.m_contents)); }
	fixed_integer_extended<false, bits> lesser(const this_t& src) const volatile { return begin_read()->lesser(*(src.m_contents)); }
	fixed_integer_extended<false, bits> lesser(const volatile this_t& src) const { return m_contents->lesser(*(src.begin_read())); }
	fixed_integer_extended<false, bits> lesser(const volatile this_t& src) const volatile { if (this == &src) return abs(); return begin_read()->lesser(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }

	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { *this = lesser(src); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lesser(src); }); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { *this = lesser(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = lesser(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); }
	void assign_lesser(const dynamic_integer& src);// { *this = src.lesser(*this); }
	void assign_lesser(const volatile dynamic_integer& src);// { *this = src.lesser(*this); }
	void assign_lesser(const dynamic_integer& src) volatile;// { *this = src.lesser(*this); }
	void assign_lesser(const volatile dynamic_integer& src) volatile;// { *this = src.lesser(*this); }

	void assign_lesser(const this_t& src) { if (this != &src) *this = lesser(*(src.m_contents)); }
	void assign_lesser(const volatile this_t& src) { *this = lesser(*(src.begin_read())); }
	void assign_lesser(const this_t& src) volatile { write_retry_loop([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	void assign_lesser(const volatile this_t& src) volatile { if (this != &src) write_retry_loop([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lesser(src); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); }
	const this_t& pre_assign_lesser(const dynamic_integer& src);// { assign_lesser(src); return *this; }
	const this_t& pre_assign_lesser(const volatile dynamic_integer& src);// { assign_lesser(src); return *this; }
	this_t pre_assign_lesser(const dynamic_integer& src) volatile;// { return write_retry_loop_pre([&](content_t & c) { c = src.lesser(c); }); }
	this_t pre_assign_lesser(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return pre_assign_lesser(tmp); }

	const this_t& pre_assign_lesser(const this_t& src) { if (this == &src) assign_abs(); else assign_lesser(src); return *this; }
	const this_t& pre_assign_lesser(const volatile this_t& src) { assign_lesser(src); return *this; }
	this_t pre_assign_lesser(const this_t& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	this_t pre_assign_lesser(const volatile this_t& src) volatile { if (this == &src) return pre_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lesser(src); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); }
	this_t post_assign_lesser(const dynamic_integer& src);// { this_t tmp(*this); assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const volatile dynamic_integer& src);// { this_t tmp(*this); assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const dynamic_integer& src) volatile;// { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	this_t post_assign_lesser(const volatile dynamic_integer& src) volatile;// { dynamic_integer tmp(src); return post_assign_lesser(tmp); }

	this_t post_assign_lesser(const this_t& src) { this_t tmp(*this); if (this == &src) assign_abs(); else assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const volatile this_t& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const this_t& src) volatile { return write_retry_loop_post([&](content_t& c) { c = c.lesser(*(src.m_contents)); }); }
	this_t post_assign_lesser(const volatile this_t& src) volatile { if (this == &src) return post_assign_abs(); return write_retry_loop_pre([&](content_t& c) { c = c.lesser(*(src.begin_read())); }); return *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }


	// equals
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->equals(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->equals(*(src.begin_read())); }
	bool operator==(const dynamic_integer& src) const;// { return src == *this; }
	bool operator==(const dynamic_integer& src) const volatile;// { return src == *this; }
	bool operator==(const volatile dynamic_integer& src) const;// { return src == *this; }
	bool operator==(const volatile dynamic_integer& src) const volatile;// { return src == *this; }

	bool operator==(const this_t& src) const { if (this == &src) return true; return m_contents->equals(*(src.m_contents)); }
	bool operator==(const this_t& src) const volatile { return begin_read()->equals(*(src.m_contents)); }
	bool operator==(const volatile this_t& src) const { return m_contents->equals(*(src.begin_read())); }
	bool operator==(const volatile this_t& src) const volatile { if (this == &src) return true; return begin_read()->equals(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator==(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator==(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator==(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator==(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator==(tmp); }

	// not_equals
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& src) const { return !m_contents->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return !begin_read()->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->equals(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->equals(*(src.begin_read())); }
	bool operator!=(const dynamic_integer& src) const;// { return src != *this; }
	bool operator!=(const dynamic_integer& src) const volatile;// { return src != *this; }
	bool operator!=(const volatile dynamic_integer& src) const;// { return src != *this; }
	bool operator!=(const volatile dynamic_integer& src) const volatile;// { return src != *this; }

	bool operator!=(const this_t& src) const { if (this == &src) return false; return !m_contents->equals(*(src.m_contents)); }
	bool operator!=(const this_t& src) const volatile { return !begin_read()->equals(*(src.m_contents)); }
	bool operator!=(const volatile this_t& src) const { return !m_contents->equals(*(src.begin_read())); }
	bool operator!=(const volatile this_t& src) const volatile { if (this == &src) return false; return !begin_read()->equals(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator!=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator!=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator!=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator!=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator!=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator!=(tmp); }

	// is_less_than
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->is_less_than(src); }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->is_less_than(src); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->is_less_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->is_less_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->is_less_than(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->is_less_than(*(src.begin_read())); }
	bool operator<(const dynamic_integer& src) const;// { return src > *this; }
	bool operator<(const dynamic_integer& src) const volatile;// { return src > *this; }
	bool operator<(const volatile dynamic_integer& src) const;// { return src > *this; }
	bool operator<(const volatile dynamic_integer& src) const volatile;// { return src > *this; }

	bool operator<(const this_t& src) const { if (this == &src) return false; return m_contents->is_less_than(*(src.m_contents)); }
	bool operator<(const this_t& src) const volatile { return begin_read()->is_less_than(*(src.m_contents)); }
	bool operator<(const volatile this_t& src) const { return m_contents->is_less_than(*(src.begin_read())); }
	bool operator<(const volatile this_t& src) const volatile { if (this == &src) return false; return begin_read()->is_less_than(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<(tmp); }

	// is_greater_than
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->is_greater_than(src); }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->is_greater_than(src); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->is_greater_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->is_greater_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->is_greater_than(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->is_greater_than(*(src.begin_read())); }
	bool operator>(const dynamic_integer& src) const;// { return src < *this; }
	bool operator>(const dynamic_integer& src) const volatile;// { return src < *this; }
	bool operator>(const volatile dynamic_integer& src) const;// { return src < *this; }
	bool operator>(const volatile dynamic_integer& src) const volatile;// { return src < *this; }

	bool operator>(const this_t& src) const { if (this == &src) return false; return m_contents->is_greater_than(*(src.m_contents)); }
	bool operator>(const this_t& src) const volatile { return begin_read()->is_greater_than(*(src.m_contents)); }
	bool operator>(const volatile this_t& src) const { return m_contents->is_greater_than(*(src.begin_read())); }
	bool operator>(const volatile this_t& src) const volatile { if (this == &src) return false; return begin_read()->is_greater_than(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>(tmp); }

	// is_less_than_or_equal
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& src) const { return !m_contents->is_greater_than(src); }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return !begin_read()->is_greater_than(src); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->is_greater_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->is_greater_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->is_greater_than(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->is_greater_than(*(src.begin_read())); }
	bool operator<=(const dynamic_integer& src) const;// { return src >= *this; }
	bool operator<=(const dynamic_integer& src) const volatile;// { return src >= *this; }
	bool operator<=(const volatile dynamic_integer& src) const;// { return src >= *this; }
	bool operator<=(const volatile dynamic_integer& src) const volatile;// { return src >= *this; }

	bool operator<=(const this_t& src) const { if (this == &src) return true; return !m_contents->is_greater_than(*(src.m_contents)); }
	bool operator<=(const this_t& src) const volatile { return !begin_read()->is_greater_than(*(src.m_contents)); }
	bool operator<=(const volatile this_t& src) const { return !m_contents->is_greater_than(*(src.begin_read())); }
	bool operator<=(const volatile this_t& src) const volatile { if (this == &src) return true; return !begin_read()->is_greater_than(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator<=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<=(tmp); }

	// is_greater_than_or_equal
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& src) const { return !m_contents->is_less_than(src); }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return !begin_read()->is_less_than(src); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->is_less_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->is_less_than(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return !m_contents->is_less_than(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return !begin_read()->is_less_than(*(src.begin_read())); }
	bool operator>=(const dynamic_integer& src) const;// { return src <= *this; }
	bool operator>=(const dynamic_integer& src) const volatile;// { return src <= *this; }
	bool operator>=(const volatile dynamic_integer& src) const;// { return src <= *this; }
	bool operator>=(const volatile dynamic_integer& src) const volatile;// { return src <= *this; }

	bool operator>=(const this_t& src) const { if (this == &src) return true; return !m_contents->is_less_than(*(src.m_contents)); }
	bool operator>=(const this_t& src) const volatile { return !begin_read()->is_less_than(*(src.m_contents)); }
	bool operator>=(const volatile this_t& src) const { return !m_contents->is_less_than(*(src.begin_read())); }
	bool operator>=(const volatile this_t& src) const volatile { if (this == &src) return true; return !begin_read()->is_less_than(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator>=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>=(tmp); }

	// compare
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->compare(src); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->compare(src); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->compare(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->compare(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->compare(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->compare(*(src.begin_read())); }
	int compare(const dynamic_integer& src) const;// { return -src.compare(*this); }
	int compare(const dynamic_integer& src) const volatile;// { return -src.compare(*this); }
	int compare(const volatile dynamic_integer& src) const;// { return -src.compare(*this); }
	int compare(const volatile dynamic_integer& src) const volatile;// { return -src.compare(*this); }

	int compare(const this_t& src) const { if (this == &src) return 0; return m_contents->compare(*(src.m_contents)); }
	int compare(const this_t& src) const volatile { return begin_read()->compare(*(src.m_contents)); }
	int compare(const volatile this_t& src) const { return m_contents->compare(*(src.begin_read())); }
	int compare(const volatile this_t& src) const volatile { if (this == &src) return 0; return begin_read()->compare(*(src.begin_read())); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }


	// TODO: Add templated versions of swap/exchange

	// swap
	void swap(this_t& wth) { if (this != wth) m_contents.swap(wth.m_contents); }
	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth) { wth.swap(*this); }

	// exchange
	this_t exchange(const this_t& src) { this_t rtn; m_contents.exchange(src.m_contents, rtn.m_contents); return rtn; }
	this_t exchange(const this_t& src) volatile { this_t rtn; m_contents.exchange(src.m_contents, rtn.m_contents); return rtn; }

	this_t exchange(const volatile this_t& src) { this_t tmpSrc(src); this_t rtn; m_contents.exchange(tmpSrc.m_contents, rtn.m_contents); return rtn; }
	this_t exchange(const volatile this_t& src) volatile
	{
		this_t rtn;
		if (this == &src)
			rtn = *this;
		else
		{
			this_t tmpSrc(src);
			m_contents.exchange(tmpSrc.m_contents, rtn.m_contents);
		}
		return rtn;
	}

	void exchange(const this_t& src, this_t& rtn) { COGS_ASSERT(this != &rtn); m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, this_t& rtn) { COGS_ASSERT(this != &rtn); this_t tmpSrc(src); m_contents.exchange(tmpSrc.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, this_t& rtn) volatile
	{
		if (this == &src)
			rtn = *this;
		else
		{
			this_t tmpSrc(src);
			m_contents.exchange(tmpSrc.m_contents, rtn.m_contents);
		}
	}

	void exchange(const this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, volatile this_t& rtn) volatile { COGS_ASSERT(this != &rtn); m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { COGS_ASSERT(this != &rtn); m_contents.exchange(src.m_contents, rtn.m_contents); }


	template <typename R>
	void exchange(const this_t& src, R& rtn)
	{
		cogs::assign(rtn, *this);
		if (this != &src)
			*this = src;
	}

	template <typename R>
	void exchange(const this_t& src, R& rtn) volatile
	{
		cogs::assign(rtn, *this);
		*this = src;
	}

	template <typename R>
	void exchange(const volatile this_t& src, R& rtn)
	{
		cogs::assign(rtn, *this);
		*this = src;
	}


	template <typename R>
	void exchange(const volatile this_t& src, R& rtn) volatile
	{
		if (this == &src)
			cogs::assign(rtn, *this);
		else
			cogs::assign(rtn, exchange(src));
	}


	template <typename S, typename R>
	void exchange(S&& src, R& rtn)
	{
		this_t tmp;
		cogs::assign(tmp, std::forward<S>(src));
		cogs::assign(rtn, *this);
		*this = tmp;
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpRtn;
		exchange(tmpSrc, tmpRtn);
		cogs::assign(rtn, tmpRtn);
	}

	template <typename S>
	this_t exchange(S&& src)
	{
		this_t tmp;
		cogs::assign(tmp, std::forward<S>(src));
		this_t rtn = *this;
		*this = tmp;
		return rtn;
	}

	template <typename S>
	this_t exchange(S&& src) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t rtn;
		exchange(tmpSrc, rtn);
		return rtn;
	}

	// compare_exchange
	bool compare_exchange(const this_t& src, const this_t& cmp, this_t& rtn)
	{
		this_t tmp(*this);
		bool b = (tmp == cmp);
		if (b)
			*this = src;
		rtn = tmp;
		return b;
	}

	bool compare_exchange(const this_t& src, const this_t& cmp, this_t& rtn) volatile
	{
		return m_contents.compare_exchange(*(src.m_contents), *(cmp.m_contents), *(rtn.m_contents));
	}

	bool compare_exchange(const this_t& src, const this_t& cmp)
	{
		this_t tmp(*this);
		bool b = (tmp == cmp);
		if (b)
			*this = src;
		return b;
	}

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile
	{
		return m_contents.compare_exchange(*(src.m_contents), *(cmp.m_contents));
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn)
	{
		this_t tmp(*this);
		bool b = cogs::equals(tmp, std::forward<C>(cmp));
		if (b)
			cogs::assign(*this, std::forward<S>(src));
		cogs::assign(rtn, tmp);
		return b;
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		this_t tmpRtn;
		bool b = m_contents.compare_exchange(*(tmpSrc.m_contents), *(tmpCmp.m_contents), *(tmpRtn.m_contents));
		cogs::assign(rtn, tmpRtn);
		return b;
	}

	template <typename S, typename C>
	bool compare_exchange(S&& src, C&& cmp)
	{
		bool b = cogs::equals(*this, std::forward<C>(cmp));
		if (b)
			cogs::assign(*this, std::forward<S>(src));
		return b;
	}

	template <typename S, typename C>
	bool compare_exchange(S&& src, C&& cmp) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		return m_contents.compare_exchange(*(tmpSrc.m_contents), *(tmpCmp.m_contents));
	}

	void set_bit(size_t i) { m_contents->set_bit(i); }
	void set_bit(size_t i) volatile { write_retry_loop([&](content_t& c) { c.set_bit(i); }); }
	const this_t& pre_set_bit(size_t i) { m_contents->set_bit(i); return *this; }
	this_t pre_set_bit(size_t i) volatile { return pre_write_retry_loop([&](content_t& c) { c.set_bit(i); }); }
	this_t post_set_bit(size_t i) { this_t tmp(*this); m_contents->set_bit(i); return tmp; }
	this_t post_set_bit(size_t i) volatile { return post_write_retry_loop([&](content_t& c) { c.set_bit(i); }); }

	void set_bit(size_t i, bool b) { if (b) set_bit(i); else reset_bit(i); }
	void set_bit(size_t i, bool b) volatile { if (b) set_bit(i); else reset_bit(i); }
	const this_t& pre_set_bit(size_t i, bool b) { if (b) return pre_set_bit(i); else return pre_reset_bit(i); }
	this_t pre_set_bit(size_t i, bool b) volatile { if (b) return pre_set_bit(i); else return pre_reset_bit(i); }
	this_t post_set_bit(size_t i, bool b) { if (b) return post_set_bit(i); else return post_reset_bit(i); }
	this_t post_set_bit(size_t i, bool b) volatile { if (b) return post_set_bit(i); else return post_reset_bit(i); }

	void reset_bit(size_t i) { m_contents->reset_bit(i); }
	void reset_bit(size_t i) volatile { write_retry_loop([&](content_t& c) { c.reset_bit(i); }); }
	const this_t& pre_reset_bit(size_t i) { m_contents->reset_bit(i); return *this; }
	this_t pre_reset_bit(size_t i) volatile { return pre_write_retry_loop([&](content_t& c) { c.reset_bit(i); }); }
	this_t post_reset_bit(size_t i) { this_t tmp(*this); m_contents->reset_bit(i); return tmp; }
	this_t post_reset_bit(size_t i) volatile { return post_write_retry_loop([&](content_t& c) { c.reset_bit(i); }); }

	void invert_bit(size_t i) { m_contents->invert_bit(i); }
	void invert_bit(size_t i) volatile { write_retry_loop([&](content_t& c) { c.invert_bit(i); }); }
	const this_t& pre_invert_bit(size_t i) { m_contents->invert_bit(i); return *this; }
	this_t pre_invert_bit(size_t i) volatile { return pre_write_retry_loop([&](content_t& c) { c.invert_bit(i); }); }
	this_t post_invert_bit(size_t i) { this_t tmp(*this); m_contents->invert_bit(i); return tmp; }
	this_t post_invert_bit(size_t i) volatile { return post_write_retry_loop([&](content_t& c) { c.invert_bit(i); }); }

	bool test_bit(size_t i) const { return m_contents->test_bit(i); }
	bool test_bit(size_t i) const volatile { return begin_read()->test_bit(i); }

	bool test_sign_extension(bool negative, size_t startIndex) const
	{
		return m_contents->test_sign_extension(negative, startIndex);
	}

	bool test_sign_extension(bool negative, size_t startIndex) const volatile
	{
		return begin_read()->test_sign_extension(negative, startIndex);
	}

	static this_t max_value()
	{
		this_t result;
		size_t i = 0;
		for (; i < n_digits - 1; i++)
			result.set_digit(i, ~(ulongest)0);
		if (has_sign)
			result.set_digit(i, ~(ulongest)0 >> 1);
		else
			result.set_digit(i, ~(ulongest)0);
		return result;
	}

	static this_t min_value()
	{
		this_t result;
		size_t i = 0;
		for (; i < n_digits - 1; i++)
			result.set_digit(i, 0);
		if (has_sign)
			result.set_digit(i, ~(~(ulongest)0 >> 1));
		else
			result.set_digit(i, ~(ulongest)0);
		return result;
	}


	template <typename char_t>
	string_t<char_t> to_string_t(uint8_t radix = 10, size_t minDigits = 1) const
	{
		if ((radix < 2) || !*this)
		{
			if (minDigits <= 64)
			{
				// Avoiding an allocation is more efficient, but needs a fixed max # of digits
				static constexpr char_t zeros[] = {
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
					(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0' };
				return string_t<char_t>::contain(zeros, minDigits);
			}

			return string_t<char_t>(minDigits, (char_t)'0');
		}

		if (radix > 36)
			radix = 36;

		static constexpr char textDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		static constexpr size_t maxLength = (n_digits * sizeof(int_t) * 8) + 2; // Enough space for largest possible value, i.e. binary radix

		char tempBufferStorage[512];
		ptr<char> tempBuffer = tempBufferStorage;
		if (maxLength > 512)
			tempBuffer = default_allocator::allocate_type<char>(maxLength);

		size_t i = 0;
		this_t src;
		bool isNeg = is_negative();
		if (isNeg)
			src.assign_negative(*this);
		else
			src.assign(*this);
		size_t srcLength = n_digits;
		ulongest radixDigit = radix;
		while (!!src)
		{
			size_t highIndex = srcLength - 1;
			while (src.m_contents->m_digits[highIndex] == 0)
				--highIndex;
			srcLength = highIndex + 1;

			if (src.m_contents->compare(srcLength, radixDigit, 1) == -1)
			{
				tempBuffer[i++] = textDigits[src.get_int()];
				break;
			}

			this_t remainder(src);
			remainder.m_contents->divide_whole_and_assign_modulo(srcLength, &radixDigit, 1, &(src.m_contents->m_digits[0]), srcLength);
			tempBuffer[i++] = textDigits[remainder.get_int()];
		}

		size_t strLength = i;
		if (strLength < minDigits)
			strLength = minDigits;
		if (isNeg)
			strLength++;
		string_t<char_t> str;
		str.resize(strLength);
		char_t* p = str.get_ptr();

		size_t i2 = 0;
		if (isNeg)
			p[i2++] = (char_t)'-';
		size_t i3 = i;
		while (i3 < minDigits)
		{
			p[i2++] = (char_t)'0';
			i3++;
		}

		for (; i > 0;)
			p[i2++] = (char_t)tempBuffer[--i];

		if (maxLength > 512)
			default_allocator::destruct_deallocate_type<char>(tempBuffer, maxLength);

		return str;
	}

	string to_string(uint8_t radix = 10, size_t minDigits = 1) const
	{
		return to_string_t<wchar_t>(radix, minDigits);
	}

	cstring to_cstring(uint8_t radix = 10, size_t minDigits = 1) const
	{
		return to_string_t<char>(radix, minDigits);
	}

	template <endian_t e>
	io::buffer to_buffer() const
	{
		static constexpr size_t width_bytes = n_digits * sizeof(int_t);
		io::buffer result(width_bytes);
		uint8_t* resultPtr = (uint8_t*)result.get_ptr();
		if (e == endian_t::little)
		{
			for (size_t i = 0; i < n_digits; i++)
			{
				int_t src = m_contents->m_digits[i];
				for (size_t j = 0; j < sizeof(int_t); j++)
					resultPtr[(i * sizeof(int_t)) + j] = (uint8_t)(src >> (j * 8));
			}
		}
		else // if (e == endian_t::big)
		{
			for (size_t i = 0; i < n_digits; i++)
			{
				int_t src = m_contents->m_digits[(n_digits - 1) - i];
				for (size_t j = 0; j < sizeof(int_t); j++)
					resultPtr[(i * sizeof(int_t)) + j] = (uint8_t)(src >> (((sizeof(int_t) - 1) - j) * 8));
			}
		}

		return result;
	}

	void randomize() { set_random_bits<ulongest>(get_digits(), n_digits); }
};


template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2, ulongest... values2>
struct compatible<fixed_integer_extended<has_sign1, bits1>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
	: public compatible<fixed_integer_extended_const<has_sign2, bits2, values2...>, fixed_integer_extended<has_sign1, bits1> >
{
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
struct compatible<fixed_integer_extended<has_sign1, bits1>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
	typedef fixed_integer<(has_sign1 || has_sign2),
		(bits1 > bits2) ?
		(bits1 + ((has_sign2 && !has_sign1) ? 1 : 0))
		: (bits2 + ((has_sign1 && !has_sign2) ? 1 : 0))
		> type;
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2>
struct compatible<fixed_integer_extended<has_sign1, bits1>, fixed_integer_native<has_sign2, bits2> >
{
	typedef fixed_integer<(has_sign1 || has_sign2), bits1 + ((has_sign1 && !has_sign2) ? 1 : 0)> type;
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2>
struct compatible<fixed_integer_extended<has_sign1, bits1>, fixed_integer_extended<has_sign2, bits2> >
{
	typedef fixed_integer<(has_sign1 || has_sign2), (bits1 > bits2 ? bits1 : bits2) + (((has_sign1 != has_sign2) && ((!has_sign1 && (bits1 >= bits2)) || (!has_sign2 && (bits2 >= bits1)))) ? 1 : 0)> type;
};

template <typename int_t2, bool has_sign2, size_t bits2>
struct compatible<fixed_integer_extended<has_sign2, bits2>, int_t2, std::enable_if_t<std::is_integral_v<int_t2> > >
	: public compatible<fixed_integer_extended<has_sign2, bits2>, int_to_fixed_integer_t<int_t2> >
{
};

template <typename int_t2, bool has_sign2, size_t bits2>
struct compatible<int_t2, fixed_integer_extended<has_sign2, bits2>, std::enable_if_t<std::is_integral_v<int_t2> > >
	: public compatible<int_to_fixed_integer_t<int_t2>, fixed_integer_extended<has_sign2, bits2> >
{
};


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest),
	fixed_integer<std::is_signed_v<T> || std::is_signed_v<A1>, (sizeof(longest) * 8) + 1>
>
add(const T& t, const A1& a)
{
	fixed_integer<std::is_signed_v<T> || std::is_signed_v<A1>, (sizeof(longest) * 8) + 1> result;
	result.add(int_to_fixed_integer_t<T>(load(t)), int_to_fixed_integer_t<A1>(load(a)));
	return result;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest)),
	fixed_integer<true, (sizeof(longest) * 8) + 1>
>
subtract(const T& t, const A1& a)
{
	fixed_integer<true, (sizeof(longest) * 8) + 1> result;
	result.subtract(int_to_fixed_integer_t<T>(load(t)), int_to_fixed_integer_t<A1>(load(a)));
	return result;
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest)),
	fixed_integer<true, (sizeof(longest) * 8) + 1>
>
inverse_subtract(const T& t, const A1& a)
{
	fixed_integer<true, (sizeof(longest) * 8) + 1> result;
	result.subtract(int_to_fixed_integer_t<A1>(load(a)), int_to_fixed_integer_t<T>(load(t)));
	return result;
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) + sizeof(A1)) > sizeof(longest)),
	fixed_integer<(std::is_signed_v<T> || std::is_signed_v<A1>), (sizeof(T) + sizeof(A1)) * 8>
>
multiply(const T& t, const A1& a)
{
	fixed_integer<(std::is_signed_v<T> || std::is_signed_v<A1>), (sizeof(T) + sizeof(A1)) * 8> result;
	result.multiply(int_to_fixed_integer_t<T>(load(t)), int_to_fixed_integer_t<A1>(load(a)));
	return result;
}


template <typename T, typename A1>
inline constexpr std::enable_if_t <
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& !can_accurately_divide_whole_int_v<T, A1>
	&& std::is_signed_v<A1>
	&& (sizeof(T) == sizeof(longest)),
	fixed_integer<true, (8 * sizeof(T)) + 1>
>
divide_whole(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	fixed_integer<true, (8 * sizeof(T))> t2(load(t));
	return t2.divide_whole(a2);
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& can_accurately_divide_whole_int_v<T, A1>
	&& std::is_signed_v<A1>
	&& (sizeof(T) == sizeof(longest)),
	fixed_integer<true, (8 * sizeof(longest)) + 1>
>
divide_whole(const T& t, const A1& a)
{
	fixed_integer_extended<true, (8 * sizeof(longest)) + 1> result(int_to_fixed_integer_t<T>(load(t)));
	result.assign_divide_whole(int_to_fixed_integer_t<A1>(load(a)));
	return result;
}


}


#endif
