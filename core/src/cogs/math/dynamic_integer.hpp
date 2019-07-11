//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_MATH_DYNAMIC_INTEGER
#define COGS_HEADER_MATH_DYNAMIC_INTEGER

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/operators.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/fixed_integer.hpp"
#include "cogs/math/fixed_integer_extended.hpp"
#include "cogs/math/fixed_integer_extended_const.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/env/math/umul.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/is_signed.hpp"
#include "cogs/math/is_arithmetic.hpp"
#include "cogs/mem/const_bit_scan.hpp"
#include "cogs/sync/hazard.hpp"


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


namespace cogs {

class dynamic_integer;

template <typename numerator_t, typename denominator_t>
class fraction;

template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;

template <> class is_arithmetic<dynamic_integer> : public std::true_type { };

template <> class is_integral<dynamic_integer> : public std::true_type { };

template <> class is_signed<dynamic_integer> : public std::true_type { };

class dynamic_integer_content
{
public:
	typedef vector_descriptor<ulongest>	desc_t;
	typedef vector_content<ulongest>	vector_content_t;

	vector_content_t m_digits;
	bool m_isNegative;

	dynamic_integer_content()
		: m_isNegative(false)
	{ }

	dynamic_integer_content(const dynamic_integer_content& src)	// does not acquire
		: m_digits(src.m_digits),
		m_isNegative(src.m_isNegative)
	{ }

	dynamic_integer_content(const vector_content_t& digits, bool isNegative)	// does not acquire
		: m_digits(digits),
		m_isNegative(isNegative)
	{ }

	template <bool has_sign, size_t bits>
	dynamic_integer_content(const fixed_integer_native<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer_content(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer_content(const fixed_integer_extended<has_sign, bits>& src)
	{
		operator=(*(src.m_contents));
	}

	dynamic_integer_content& operator=(dynamic_integer_content&& src)
	{		
		m_digits.release();
		m_digits.set(src.m_digits);
		m_isNegative = src.m_isNegative;
		src.m_digits.clear_inner();
		return *this;
	}

	dynamic_integer_content& operator=(const dynamic_integer_content& src)	// does not acquire, but releases existing
	{
		m_digits.release();
		m_digits.set(src.m_digits);
		m_isNegative = src.m_isNegative;
		return *this;
	}

	dynamic_integer_content& operator=(const vector_content_t& src)	// does not acquire, but releases existing
	{
		m_digits.release();
		m_digits.set(src);
		return *this;
	}

	void assign(const dynamic_integer_content& src)	// does acquire
	{
		m_digits.acquire(src.m_digits);
		m_isNegative = src.m_isNegative;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer_content& operator=(const fixed_integer_native<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			operator=(src.abs_inner());
			m_isNegative = true;
		}
		else
		{
			m_isNegative = false;

			ulongest tmp = (ulongest)src.get_int();
			if (!tmp)
				m_digits.clear();
			else
				m_digits.assign(1, tmp);
		}
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer_content& operator=(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			fixed_integer_extended_content<false, bits> tmp;
			tmp.assign_abs(src);
			operator=(tmp);
			m_isNegative = true;
		}
		else
		{
			m_isNegative = false;

			const ulongest* srcDigits = src.get_digits();
			size_t i = fixed_integer_extended<has_sign, bits>::n_digits;
			do {
				if (srcDigits[--i] != 0)
				{
					m_digits.clear();
					m_digits.resize(i + 1);
					ulongest* digits = m_digits.m_ptr;
					for (;;)
					{
						digits[i] = srcDigits[i];
						if (!i)
							break;
						--i;
					}
					break;
				}
			} while (i != 0);
		}
		return *this;
	}

	template <typename int_t = ulongest>
	int_t get_int() const
	{
		int_t result;
		if (m_digits.get_length() == 0)
			result = 0;
		else
		{
			ulongest tmp = m_digits.get_const_ptr()[0];
			result = m_isNegative ? (int_t)-(longest)tmp : (int_t)tmp;
		}
		return result;
	}

	void swap(dynamic_integer_content& wth)
	{
		m_digits.swap(wth.m_digits);
		bool b = m_isNegative;
		m_isNegative = wth.m_isNegative;
		wth.m_isNegative = b;
	}

	void acquire()
	{
		m_digits.acquire();
	}

	void release() const
	{
		m_digits.release();
	}

	const ulongest* get_const_ptr() const { return m_digits.m_ptr; }

	size_t get_length() const { return m_digits.m_length; }

	bool operator!() const { return !get_length(); }

	bool is_negative() const { return m_isNegative; }

	void negate() { m_isNegative = !m_isNegative; }

	bool is_exponent_of_two() const
	{
		// The high digit will always be non-zero, so enough to check only that digit, then confirm the rest are zero.
		size_t i = m_digits.get_length() - 1;
		if (cogs::is_exponent_of_two(m_digits[i]))
		{
			while (i > 0)
			{
				--i;
				if (m_digits[i] != 0)
					return false;
			}
			return true;
		}
		return false;
	}

	void trim_leading_zeros()
	{
		const ulongest* digits = m_digits.get_const_ptr();
		size_t sz = m_digits.get_length();
		if (!!sz)
		{
			if (digits[--sz] == 0)
			{
				for (;;)
				{
					size_t next = sz - 1;
					if (!sz || (digits[sz - 1] != 0))
						break;
					sz = next;
				}
				m_digits.truncate_to(sz);
			}
		}
	}

	void clear()
	{
		m_isNegative = false;
		m_digits.clear();
	}

	template <bool has_sign, size_t bits>
	void to_fixed_integer(fixed_integer_native<has_sign, bits>& dst) const
	{
		if (m_digits.get_length() == 0)
		{
			dst.clear();
			return;
		}

		ulongest tmp = m_digits.get_const_ptr()[0];
		if (m_isNegative)
			dst = -(typename fixed_integer_native<has_sign, bits>::signed_int_t)tmp;
		else
			dst = (typename fixed_integer_native<has_sign, bits>::unsigned_int_t)tmp;
	}

	template <bool has_sign, size_t bits>
	void to_fixed_integer(fixed_integer_extended_content<has_sign, bits>& dst) const
	{
		ulongest* dstDigits = dst.get_digits();
		const ulongest* srcDigits = m_digits.get_const_ptr();

		size_t sz = m_digits.get_length();
		size_t lesserSize = (sz < fixed_integer_extended<has_sign, bits>::n_digits) ? sz : fixed_integer_extended<has_sign, bits>::n_digits;
		size_t i = 0;
		if (m_isNegative)
		{
			bool incrementing = true;
			for (; i < lesserSize; i++)
			{
				ulongest tmp = ~(srcDigits[i]);
				if (incrementing)
					incrementing = !++tmp;
				dstDigits[i] = tmp;
			}
			for (; i < fixed_integer_extended<has_sign, bits>::n_digits; i++)
				dstDigits[i] = ~0;
			dst.increment();
		}
		else
		{
			for (; i < lesserSize; i++)
				dstDigits[i] = srcDigits[i];
			for (; i < fixed_integer_extended<has_sign, bits>::n_digits; i++)
				dstDigits[i] = 0;
		}
	}

	int compare(const dynamic_integer_content& cmp) const
	{
		if (is_negative())
		{
			if (!cmp.is_negative())
				return -1;
		}
		else if (cmp.is_negative())
			return 1;

		int result = compare(cmp.m_digits.get_const_ptr(), cmp.m_digits.get_length());
		return m_isNegative ? -result : result;
	}

	int compare(const ulongest* cmpDigits, size_t cmpLength) const	// assumes positive
	{
		size_t n = m_digits.get_length();
		if (n > cmpLength)
			return 1;
		if (n < cmpLength)
			return -1;
		if (!n)
			return 0;
		for (;;)
		{
			--n;
			ulongest myDigit = m_digits.get_const_ptr()[n];
			ulongest cmpDigit = cmpDigits[n];
			if (myDigit > cmpDigit)
				return 1;
			if (myDigit < cmpDigit)
				return -1;
			if (!n)
				return 0;
		}
	}

	template <bool has_sign, size_t bits>
	int compare(const fixed_integer_native<has_sign, bits>& cmp) const
	{
		return compare(cmp.get_int());
	}

	template <bool has_sign, size_t bits>
	int compare(const fixed_integer_extended_content<has_sign, bits>& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.is_negative())
				return -1;
			size_t sz = m_digits.get_length();	// will be >0 because m_isNegative is true
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return -1;
			fixed_integer_extended<false, bits + 1> cmp2 = -cmp;
			const ulongest* tmpDigits = cmp2.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return -compare(tmpDigits, cmpDigits);
				cmpDigits = i;
				--i;
			}
		}
		else
		{
			if (cmp.is_negative())
				return 1;
			size_t sz = m_digits.get_length();
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return 1;
			const ulongest* tmpDigits = cmp.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return compare(tmpDigits, cmpDigits);
				if (!i)
					return !sz ? 0 : 1;
				cmpDigits = i;
				--i;
			}
		}
	}


	template <typename int_t2>
	std::enable_if_t<
		std::is_integral_v<int_t2>
		&& !std::is_signed_v<int_t2>,
		int
	>
	compare(const int_t2& cmp) const
	{
		if (m_isNegative)	// sz won't be 0 if negative
			return -1;

		size_t sz = m_digits.get_length();
		if (sz > 1)
			return 1;
		if (sz == 0)
			return !cmp ? 0 : -1;
		ulongest tmpCmp = (ulongest)cmp;
		ulongest firstDigit = m_digits.get_const_ptr()[0];
		if (firstDigit == tmpCmp)
			return 0;
		return (firstDigit > tmpCmp) ? 1 : -1;
	}


	template <typename int_t2>
	std::enable_if_t<
		std::is_integral_v<int_t2>
		&& std::is_signed_v<int_t2>,
		int
	>
	compare(const int_t2& cmp) const
	{
		if (m_isNegative)	// sz won't be 0 if negative
		{
			if (cmp >= 0)
				return -1;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return -1;								// else sz == 1
			ulongest tmpCmp = (ulongest)-(longest)cmp;	// should fit
			ulongest firstDigit = get_const_ptr()[0];
			if (firstDigit == tmpCmp)
				return 0;
			return (firstDigit > tmpCmp) ? -1 : 1;
		}
		else
		{
			if (cmp < 0)
				return 1;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return 1;
			if (sz == 0)
				return !cmp ? 0 : -1;
			ulongest tmpCmp = (ulongest)cmp;	// should fit
			ulongest firstDigit = m_digits.get_const_ptr()[0];
			if (firstDigit == tmpCmp)
				return 0;
			return (firstDigit > tmpCmp) ? 1 : -1;
		}
	}


	bool equals(const dynamic_integer_content& cmp) const
	{
		return ((m_isNegative == cmp.m_isNegative) && (m_digits.equals<ulongest, default_comparator>(cmp.m_digits.m_ptr, cmp.m_digits.m_length)));
	}

	bool equals(const ulongest* cmpDigits, size_t cmpLength) const
	{
		size_t length = m_digits.get_length();
		if (length != cmpLength)
			return false;
		if (!length)
			return true;
		for (size_t i = 0; i < length; i++)
		{
			if (m_digits.get_const_ptr()[i] != cmpDigits[i])
				return false;
		}
		return true;
	}

	template <bool has_sign, size_t bits>
	bool equals(const fixed_integer_native<has_sign, bits>& cmp) const
	{
		if (m_isNegative)	// sz won't be 0 if negative
		{
			if (!cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return false;									// else sz == 1
			return m_digits.get_const_ptr()[0] == (ulongest)-(longest)(cmp.get_int());	// should fit
		}
		else
		{
			if (cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return false;
			if (sz == 0)
				return !cmp;
			return m_digits.get_const_ptr()[0] == (ulongest)cmp.get_int();		// should fit
		}
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool equals(const int_t2& cmp) const	// int version needed by compare_exchange
	{
		int_to_fixed_integer_t<int_t2> tmp(cmp);
		return equals(cmp);
	}

	template <bool has_sign, size_t bits>
	bool equals(const fixed_integer_extended_content<has_sign, bits>& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();	// will be >0 because m_isNegative is true
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return false;
			fixed_integer_extended<false, bits + 1> cmp2 = -cmp;
			const ulongest* tmpDigits = cmp2.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return equals(tmpDigits, cmpDigits);
				cmpDigits = i;
				--i;
			}
		}
		else
		{
			if (cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return false;
			const ulongest* tmpDigits = cmp.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return equals(tmpDigits, cmpDigits);
				if (!i)
					return !sz;
				cmpDigits = i;
				--i;
			}
		}
	}

	bool is_greater_than(const dynamic_integer_content& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.m_isNegative)
				return false;
			return is_less_than(cmp.m_digits.get_const_ptr(), cmp.m_digits.get_length());
		}
		else
		{
			if (cmp.m_isNegative)
				return true;
			return is_greater_than(cmp.m_digits.get_const_ptr(), cmp.m_digits.get_length());
		}
	}

	bool is_greater_than(const ulongest* cmpDigits, size_t cmpLength) const	// Assumes both are positive
	{
		size_t length = m_digits.get_length();
		if (length > cmpLength)
			return true;
		if (length < cmpLength)
			return false;
		if (!length)
			return false;
		for (;;)
		{
			--length;
			ulongest myDigit = m_digits.get_const_ptr()[length];
			ulongest cmpDigit = cmpDigits[length];
			if (myDigit > cmpDigit)
				return true;
			if ((myDigit < cmpDigit) || !length)
				return false;
		}
	}

	template <bool has_sign, size_t bits>
	bool is_greater_than(const fixed_integer_native<has_sign, bits>& cmp) const
	{
		if (m_isNegative)	// sz won't be 0 if negative
		{
			if (!cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return false;									// else sz == 1
			ulongest tmpCmp = (ulongest)-(longest)(cmp.get_int());	// should fit
			return (m_digits.get_const_ptr()[0] < tmpCmp);
		}
		else
		{
			if (cmp.is_negative())
				return true;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return true;
			if (sz == 0)
				return false;
			ulongest tmpCmp = (ulongest)cmp.get_int();	// should fit
			return (m_digits.get_const_ptr()[0] > tmpCmp);
		}
	}

	template <bool has_sign, size_t bits>
	bool is_greater_than(const fixed_integer_extended_content<has_sign, bits>& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();	// will be >0 because m_isNegative is true
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return false;
			fixed_integer_extended<false, bits + 1> cmp2 = -cmp;
			const ulongest* tmpDigits = cmp2.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return is_less_than(tmpDigits, cmpDigits);
				cmpDigits = i;
				--i;
			}
		}
		else
		{
			if (cmp.is_negative())
				return true;
			size_t sz = m_digits.get_length();
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return true;
			if (sz == 0)
				return false;
			const ulongest* tmpDigits = cmp.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return is_greater_than(tmpDigits, cmpDigits);
				if (!i)
					return true;
				cmpDigits = i;
				--i;
			}
		}
	}

	bool is_less_than(const ulongest* cmpDigits, size_t cmpLength) const	// Assumes both are positive
	{
		size_t length = m_digits.get_length();
		if (length < cmpLength)
			return true;
		if (length > cmpLength)
			return false;
		if (!length)
			return false;
		for (;;)
		{
			--length;
			ulongest myDigit = m_digits.get_const_ptr()[length];
			ulongest cmpDigit = cmpDigits[length];
			if (myDigit < cmpDigit)
				return true;
			if ((myDigit > cmpDigit) || !length)
				return false;
		}
	}

	bool is_less_than(const dynamic_integer_content& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.m_isNegative)
				return true;
			return is_greater_than(cmp.m_digits.get_const_ptr(), cmp.m_digits.get_length());
		}
		else
		{
			if (cmp.m_isNegative)
				return false;
			return is_less_than(cmp.m_digits.get_const_ptr(), cmp.m_digits.get_length());
		}
	}

	template <bool has_sign, size_t bits>
	bool is_less_than(const fixed_integer_native<has_sign, bits>& cmp) const
	{
		if (m_isNegative)	// sz won't be 0 if negative
		{
			if (!cmp.is_negative())
				return true;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return true;									// else sz == 1
			ulongest tmpCmp = (ulongest)-(longest)(cmp.get_int());	// should fit
			return (m_digits.get_const_ptr()[0] > tmpCmp);
		}
		else
		{
			if (cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			if (sz > 1)
				return false;
			if (sz == 0)
				return !!cmp.get_int();	// we are less if cmp is non-zero
			ulongest tmpCmp = (ulongest)cmp.get_int();	// should fit
			return (m_digits.get_const_ptr()[0] < tmpCmp);
		}
	}

	template <bool has_sign, size_t bits>
	bool is_less_than(const fixed_integer_extended_content<has_sign, bits>& cmp) const
	{
		if (m_isNegative)
		{
			if (!cmp.is_negative())
				return true;
			size_t sz = m_digits.get_length();	// will be >0 because m_isNegative is true
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return true;
			fixed_integer_extended<false, bits + 1> cmp2 = -cmp;
			const ulongest* tmpDigits = cmp2.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return is_greater_than(tmpDigits, cmpDigits);
				cmpDigits = i;
				--i;
			}
		}
		else
		{
			if (cmp.is_negative())
				return false;
			size_t sz = m_digits.get_length();
			size_t cmpDigits = fixed_integer_extended<has_sign, bits>::n_digits;
			if (sz > cmpDigits)
				return false;
			if (sz == 0)
				return false;
			const ulongest* tmpDigits = cmp.get_digits();
			size_t i = cmpDigits - 1;
			for (;;)
			{
				if (tmpDigits[i] != 0)
					return is_less_than(tmpDigits, cmpDigits);
				if (!i)
					return false;
				cmpDigits = i;
				--i;
			}
		}
	}

	void increment()
	{
		size_t length = m_digits.get_length();
		if (!!length)
		{
			ulongest* digitPtr = m_digits.get_ptr();
			for (size_t i = 0; i < length; i++)
			{
				if (++(digitPtr[i]) != 0)
					return;
			}
		}
		m_digits.append(1, 1);
	}

	void decrement()
	{
		size_t length = m_digits.get_length();
		if (!length)
		{
			m_digits.append(1, 1);
			m_isNegative = true;
		}
		else if (length == 1)
		{
			if (m_digits.get_const_ptr()[0] == 1)
			{
				clear();
				return;
			}

			m_digits.get_ptr()[0]--;
		}
		else // Must have a value of >0, so won't get to the end of the list
		{
			ulongest* p = m_digits.get_ptr();
			size_t i = 0;
			ulongest oldValue;
			do {
				oldValue = (p[i++])--;
			} while (oldValue != 0);
			if ((i == length) && (oldValue == 1))
			{
				m_digits.truncate(1);
			}
		}
	}


	// Addition
	void add(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)	// Assumes both are positive and not-zero
	{																										// Does not set m_isNegative
		size_t shorterLength;
		size_t longerLength;
		const ulongest* longerDigits;
		if (src1Length > src2Length)
		{
			longerLength = src1Length;
			shorterLength = src2Length;
			longerDigits = src1Digits;
		}
		else
		{
			longerLength = src2Length;
			shorterLength = src1Length;
			longerDigits = src2Digits;
		}
		m_digits.clear();
		m_digits.resize(longerLength + 1);
		ulongest* dstDigits = m_digits.m_ptr;
		bool overflow = false;
		size_t i;
		for (i = 0; i < shorterLength; i++)
		{
			ulongest tmp = src1Digits[i];
			ulongest newValue = tmp + src2Digits[i];
			if (!overflow)
				overflow = (newValue < tmp);
			else
			{
				newValue++;
				overflow = (newValue <= tmp);
			}
			dstDigits[i] = newValue;
		}
		while (overflow && (i < longerLength))
		{
			ulongest newValue = longerDigits[i] + 1;
			dstDigits[i] = newValue;
			overflow = (0 == newValue);
			++i;
		}
		if (!overflow)
			m_digits.resize(longerLength);
		else
			dstDigits[i] = 1;
	}

	void add(const dynamic_integer_content& src1, const dynamic_integer_content& src2)	// assumes neither src1 or src2 are this
	{
		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			assign(src2);
			return;
		}

		size_t src2Length = src2.m_digits.get_length();
		if (!src2Length)
		{
			assign(src1);
			return;
		}

		const ulongest* src1Digits = src1.m_digits.get_const_ptr();
		const ulongest* src2Digits = src2.m_digits.get_const_ptr();
		m_isNegative = src1.m_isNegative;
		if (m_isNegative == src2.m_isNegative)
			add(src1Digits, src1Length, src2Digits, src2Length);
		else
			subtract(src1Digits, src1Length, src2Digits, src2Length);
	}

	template <bool has_sign, size_t bits>
	void add2(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			assign(src2);
			return;
		}

		if (src2.is_negative())
		{
			subtract(src1, src2.abs_inner());
			return;
		}

		m_isNegative = src1.is_negative();
		ulongest newDigit = (ulongest)src2.get_int();
		if (m_isNegative)
			subtract(src1.m_digits.get_const_ptr(), src1Length, &newDigit, 1);
		else
			add(src1.m_digits.get_const_ptr(), src1Length, &newDigit, 1);
	}

	template <bool has_sign, size_t bits>
	void add(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		if (!src2)
		{
			assign(src1);
			return;
		}

		add2(src1, src2);
	}

	template <bool has_sign, size_t bits>
	void add(const dynamic_integer_content& src1, const fixed_integer_extended_content<has_sign, bits>& src2)
	{
		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			operator=(src2);
			return;
		}

		if (src2.is_negative())
		{
			subtract(src1, src2.abs_inner());
			return;
		}

		ulongest* src2Digits = src2.get_digits();
		size_t src2Length = fixed_integer_extended<has_sign, bits>::n_digits;
		size_t i = src2Length - 1;
		while (src2Digits[i] == 0)
		{
			if (!i)
			{
				assign(src1);
				return;
			}
			src2Length = i--;
		}

		m_isNegative = src1.m_isNegative;
		if (m_isNegative)
			subtract(src1.m_digits.get_const_ptr(), src1Length, src2Digits, src2Length);
		else
			add(src1.m_digits.get_const_ptr(), src1Length, src2Digits, src2Length);
	}

	// Add to
	void add(const ulongest* srcDigits, size_t srcLength)	// Assumes both are positive and not-zero
	{														// Does not set m_isNegative
		vector_content_t tmp(m_digits);
		tmp.acquire();
		add(srcDigits, srcLength, tmp.get_const_ptr(), tmp.get_length());
	}

	void add()
	{
		size_t length = m_digits.get_length();
		if (!length)
			return;

		vector_content_t tmp(m_digits);
		tmp.acquire();
		const ulongest* digits = tmp.get_const_ptr();
		add(digits, length, digits, length);
	}

	void add(const dynamic_integer_content& src)
	{
		size_t srcLength = src.m_digits.get_length();
		if (!srcLength)
			return;

		size_t length = m_digits.get_length();
		if (!length)
		{
			assign(src);
			return;
		}

		if (m_isNegative == src.m_isNegative)
			add(src.m_digits.get_const_ptr(), srcLength);
		else
			subtract(src.m_digits.get_const_ptr(), srcLength);
	}

	template <bool has_sign, size_t bits>
	void add2(const fixed_integer_native<has_sign, bits>& src)
	{
		size_t length = m_digits.get_length();
		if (!length)
		{
			operator=(src);
			return;
		}

		if (src.is_negative())
		{
			subtract(src.abs_inner());
			return;
		}

		ulongest newDigit = (ulongest)src.get_int();
		if (m_isNegative)
			subtract(&newDigit, 1);
		else
			add(&newDigit, 1);
	}

	template <bool has_sign, size_t bits>
	void add(const fixed_integer_native<has_sign, bits>& src)
	{
		if (!src)
			return;

		add2(src);
	}

	template <bool has_sign, size_t bits>
	void add(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		size_t length = m_digits.get_length();
		if (!length)
		{
			operator=(src);
			return;
		}

		if (src.is_negative())
		{
			subtract(src.abs_inner());
			return;
		}

		ulongest* srcDigits = src.get_digits();
		size_t srcLength = fixed_integer_extended<has_sign, bits>::n_digits;
		size_t i = srcLength - 1;
		while (srcDigits[i] == 0)
		{
			if (!i)
				return;
			srcLength = i--;
		}

		if (m_isNegative)
			subtract(srcDigits, srcLength);
		else
			add(srcDigits, srcLength);
	}

	// Subtraction
	void subtract2(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)
	{											// (src1Length >= src2Length) && (src2Length > 0)
		ulongest* digits = m_digits.m_ptr;
		bool overflow = false;
		ulongest src1Digit;
		ulongest newValue = 0;
		size_t i;
		for (i = 0; i < src2Length; i++)
		{
			src1Digit = src1Digits[i];
			newValue = src1Digit - src2Digits[i];
			if (!overflow)
				overflow = newValue > src1Digit;
			else
			{
				newValue--;
				overflow = newValue >= src1Digit;
			}
			digits[i] = newValue;
		}
		for (; i < src1Length; i++)
		{
			newValue = src1Digits[i];
			if (overflow)
				overflow = (0 == ++newValue);
			digits[i] = newValue;
		}
		if (newValue == 0)
			m_digits.truncate(1);	// At most will need to shrink by 1
	}

	void subtract(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)	// Assumes both are same sign and both lengths are >0
	{												// Will invert m_isNegative if src2 > src1.  Will set m_isNegative to false if result is 0
		m_digits.clear();
		if (src1Length > src2Length)
		{
			m_digits.resize(src1Length);
			subtract2(src1Digits, src1Length, src2Digits, src2Length);
		}
		else if (src1Length < src2Length)
		{
			m_digits.resize(src2Length);
			subtract2(src2Digits, src2Length, src1Digits, src1Length);
			m_isNegative = !m_isNegative;
		}
		else // if (src1Length == src2Length)
		{
			size_t i = src1Length;
			for (;;)
			{
				size_t newLength = i;
				--i;
				ulongest src1Digit = src1Digits[i];
				ulongest src2Digit = src2Digits[i];
				if (src1Digit != src2Digit)
				{
					m_digits.resize(newLength);
					if (src1Digit > src2Digit)
						subtract2(src1Digits, newLength, src2Digits, newLength);
					else
					{
						subtract2(src2Digits, newLength, src1Digits, newLength);
						m_isNegative = !m_isNegative;
					}
					break;
				}
				if (!i)
				{
					clear();
					break;
				}
			}
		}
	}

	void subtract(const dynamic_integer_content& src1, const dynamic_integer_content& src2)
	{
		size_t src2Length = src2.m_digits.get_length();
		if (!src2Length)
		{
			assign(src1);
			return;
		}

		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			m_digits.acquire(src2.m_digits);
			m_isNegative = !src2.m_isNegative;
			return;
		}

		m_isNegative = src1.m_isNegative;
		if (m_isNegative == src2.m_isNegative)
			subtract(src1.m_digits.get_const_ptr(), src1Length, src2.m_digits.get_const_ptr(), src2Length);
		else
			add(src1.m_digits.get_const_ptr(), src1Length, src2.m_digits.get_const_ptr(), src2Length);
	}


	template <bool has_sign, size_t bits>
	void subtract2(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		if (src2.is_negative())
		{
			add(src1, src2.abs_inner());
			return;
		}

		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			operator=(-src2);
			return;
		}

		m_isNegative = src1.is_negative();
		ulongest newDigit = (ulongest)src2.get_int();
		if (m_isNegative)
			add(src1.m_digits.get_const_ptr(), src1Length, &newDigit, 1);
		else
			subtract(src1.m_digits.get_const_ptr(), src1Length, &newDigit, 1);
	}


	template <bool has_sign, size_t bits>
	void subtract(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		if (!src2)
		{
			assign(src1);
			return;
		}

		subtract2(src1, src2);
	}

	template <bool has_sign, size_t bits>
	void subtract(const dynamic_integer_content& src1, const fixed_integer_extended_content<has_sign, bits>& src2)
	{
		if (src2.is_negative())
		{
			add(src1, src2.abs_inner());
			return;
		}

		size_t src1Length = src1.m_digits.get_length();
		if (!src1Length)
		{
			operator=(-src2);
			return;
		}

		ulongest* src2Digits = src2.get_digits();
		size_t src2Length = fixed_integer_extended<has_sign, bits>::n_digits;
		size_t i = src2Length - 1;
		while (src2Digits[i] == 0)
		{
			if (!i)
			{
				assign(src1);
				return;
			}
			src2Length = i--;
		}

		m_isNegative = src1.m_isNegative;
		if (m_isNegative)
			add(src1.m_digits.get_const_ptr(), src1Length, src2Digits, src2Length);
		else
			subtract(src1.m_digits.get_const_ptr(), src1Length, src2Digits, src2Length);
	}

	// Subtract from
	void subtract(const ulongest* srcDigits, size_t srcLength)
	{
		vector_content_t tmp(m_digits);
		tmp.acquire();
		subtract(tmp.get_const_ptr(), tmp.get_length(), srcDigits, srcLength);
	}

	void subtract(const dynamic_integer_content& src)
	{
		size_t srcLength = src.m_digits.get_length();
		if (!srcLength)
			return;

		size_t length = m_digits.get_length();
		if (!length)
		{
			m_digits.acquire(src.m_digits);
			m_isNegative = !src.m_isNegative;
			return;
		}

		if (m_isNegative == src.m_isNegative)
			subtract(src.m_digits.get_const_ptr(), srcLength);
		else
			add(src.m_digits.get_const_ptr(), srcLength);
	}



	template <bool has_sign, size_t bits>
	void subtract2(const fixed_integer_native<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			add(src.abs_inner());
			return;
		}

		size_t length = m_digits.get_length();
		if (!length)
		{
			operator=(-src);
			return;
		}

		ulongest newDigit = (ulongest)src.get_int();
		if (m_isNegative)
			add(&newDigit, 1);
		else
			subtract(&newDigit, 1);
	}

	template <bool has_sign, size_t bits>
	void subtract(const fixed_integer_native<has_sign, bits>& src)
	{
		if (!src)
			return;

		subtract2(src);
	}

	template <bool has_sign, size_t bits>
	void subtract(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			subtract(src.abs_inner());
			return;
		}

		size_t length = m_digits.get_length();
		if (!length)
		{
			operator=(-src);
			return;
		}

		ulongest* srcDigits = src.get_digits();
		size_t srcLength = fixed_integer_extended<has_sign, bits>::n_digits;
		size_t i = srcLength - 1;
		while (srcDigits[i] == 0)
		{
			if (!i)
				return;
			srcLength = i--;
		}

		if (m_isNegative)
			add(srcDigits, srcLength);
		else
			subtract(srcDigits, srcLength);
	}

	void set_left_shifted(const ulongest* srcDigits, size_t srcLength, size_t bitShift, size_t digitShift)
	{
		size_t newLength = digitShift + srcLength;
		m_digits.resize(newLength + (!!bitShift ? 1 : 0));
		for (size_t i = 0; i < digitShift; i++)
			m_digits[i] = 0;
		if (!bitShift)
		{
			for (size_t i = 0; i < srcLength; i++)
				m_digits[digitShift + i] = srcDigits[i];
		}
		else
		{
			size_t bitShiftBack = (sizeof(ulongest) * 8) - bitShift;
			ulongest carryOver = 0;
			for (size_t i = 0; i < srcLength; i++)
			{
				ulongest srcDigitValue = srcDigits[i];
				m_digits[digitShift + i] = carryOver | (srcDigitValue >> bitShift);
				carryOver = srcDigitValue << bitShiftBack;
			}
			m_digits[newLength] = carryOver;
		}
	}


	// Multiplication
	// src1 digits will be >= src2 digits. - slightly more efficient
	void multiply2(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)
	{
		// exponent of two optimization
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
			size_t bitShift = bit_scan_forward(src2HighDigitValue);
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
				size_t bitShift = bit_scan_forward(src1HighDigitValue);
				set_left_shifted(src2Digits, src2Length, bitShift, src1HighIndex);
			}
			else
			{
				m_digits.assign(src1Length + src2Length, (ulongest)0);
				ulongest* dstDigits = m_digits.get_ptr();

				for (size_t i1 = 0; i1 < src1Length; i1++)
				{
					ulongest overflow = 0;
					for (size_t i2 = 0; i2 < src2Length; i2++)
					{
						ulongest newOverflow;
						size_t dstIndex = i1 + i2;
						ulongest* digit = &(dstDigits[dstIndex]);
						ulongest oldValue = *digit;
						*digit += env::umul_longest(src1Digits[i1], src2Digits[i2], newOverflow);
						if (*digit < oldValue)
							newOverflow++;
						oldValue = *digit;
						*digit += overflow;
						overflow = newOverflow;
						if (*digit < oldValue)
							overflow++;
					}
				}

				trim_leading_zeros();
			}
		}
	}

	void multiply(const ulongest* src1Digits, size_t src1Length, const ulongest* src2Digits, size_t src2Length)
	{
		if (src1Length < src2Length)
		{
			size_t tmp = src1Length;
			src1Length = src2Length;
			src2Length = tmp;
			const ulongest* tmp2 = src1Digits;
			src1Digits = src2Digits;
			src2Digits = tmp2;
		}

		multiply2(src1Digits, src1Length, src2Digits, src2Length);
	}

	void multiply(const dynamic_integer_content& src1, const dynamic_integer_content& src2)
	{
		size_t src2Length = src2.m_digits.get_length();
		if (src2Length)
		{
			size_t src1Length = src1.m_digits.get_length();
			if (src1Length)
			{
				m_isNegative = (src1.m_isNegative != src2.m_isNegative);
				multiply(src1.m_digits.get_const_ptr(), src1Length, src2.m_digits.get_const_ptr(), src2Length);
				return;
			}
			// fall through, 
		}
		// else if ((!src2Length) || (!src1Length))
		clear();
	}

	template <bool has_sign, size_t bits>
	void multiply2(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		if (!src1)
		{
			clear();
			return;
		}

		if (src2.m_isNegative)
		{
			ulongest newDigit = (ulongest)(src2.abs_inner().get_int());
			multiply2(src1.m_digits.get_const_ptr(), src1.m_digits.get_length(), &newDigit, 1);
			m_isNegative = !src1.m_isNegative;
			return;
		}

		m_isNegative = src1.m_isNegative;
		ulongest newDigit = (ulongest)(src2.get_int());
		multiply2(src1.m_digits.get_const_ptr(), src1.m_digits.get_length(), &newDigit, 1);
	}

	template <bool has_sign, size_t bits>
	void multiply(const dynamic_integer_content& src1, const fixed_integer_native<has_sign, bits>& src2)
	{
		if (!src2)
		{
			clear();
			return;
		}

		multiply2(src1, src2);
	}

	template <bool has_sign, size_t bits>
	void multiply(const dynamic_integer_content& src1, const fixed_integer_extended_content<has_sign, bits>& src2)
	{
		if (src2.is_negative())
		{
			multiply(src1, src2.abs_inner());
			m_isNegative = !m_isNegative;
			return;
		}

		if (!src2)
		{
			clear();
			return;
		}

		const ulongest* src2Digits = src2.get_digits();
		size_t src2Length = fixed_integer_extended<has_sign, bits>::n_digits;
		for (;;)
		{
			size_t src2LengthNext = src2Length - 1;
			if (src2Digits[src2LengthNext] != 0)
				break;
			if (!src2LengthNext)	// src2 is zero
			{
				clear();
				return;
			}
			src2Length = src2LengthNext;
		}

		m_isNegative = src1.m_isNegative;
		multiply(src1.m_digits.get_const_ptr(), src1.m_digits.get_length(), src2Digits, src2Length);
	}

	// Multiply to
	void multiply2(const ulongest* srcDigits, size_t srcLength)
	{
		vector_content_t tmp(m_digits);
		tmp.acquire();
		multiply2(srcDigits, srcLength, tmp.get_const_ptr(), tmp.get_length());
	}

	void multiply(const ulongest* srcDigits, size_t srcLength)
	{
		vector_content_t tmp(m_digits);
		tmp.acquire();
		multiply(srcDigits, srcLength, tmp.get_const_ptr(), tmp.get_length());
	}

	void multiply(const dynamic_integer_content& src)
	{
		size_t length = m_digits.get_length();
		if (length)
		{
			size_t srcLength = src.m_digits.get_length();
			if (!srcLength)
				clear();
			else
			{
				m_isNegative = (m_isNegative != src.m_isNegative);
				multiply(src.m_digits.get_const_ptr(), srcLength);
			}
		}
	}

	template <bool has_sign, size_t bits>
	void multiply2(const fixed_integer_native<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			ulongest newDigit = (ulongest)(src.abs_inner().get_int());
			multiply2(&newDigit, 1);
			m_isNegative = !m_isNegative;
			return;
		}

		ulongest newDigit = (ulongest)(src.get_int());
		multiply2(&newDigit, 1);
	}

	template <bool has_sign, size_t bits>
	void multiply(const fixed_integer_native<has_sign, bits>& src)
	{
		if (!src)
			clear();
		else if (!!m_digits.get_length())
			multiply2(src);
	}

	template <bool has_sign, size_t bits>
	void multiply2(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		if (src.is_negative())
		{
			multiply2(src.abs_inner());
			m_isNegative = !m_isNegative;
		}
		else
		{
			const ulongest* srcDigits = src.get_digits();
			size_t srcLength = fixed_integer_extended<has_sign, bits>::n_digits;
			for (;;)
			{
				size_t srcLengthNext = srcLength - 1;
				if (srcDigits[srcLengthNext] != 0)
					break;
				if (!srcLengthNext)	// src is zero
				{
					clear();
					return;
				}
				srcLength = srcLengthNext;
			}

			multiply(srcDigits, srcLength);
		}
	}

	template <bool has_sign, size_t bits>
	void multiply(const fixed_integer_extended_content<has_sign, bits>& src)
	{
		if (!!m_digits.get_length())
			multiply2(src);
	}

	void set_right_shifted(const ulongest* srcDigits, size_t srcLength, size_t shift)
	{
		// (n > 0) && (n < sizeof(ulongest)*8)
		size_t i = 0;
		ulongest srcValue = srcDigits[0];
		size_t shiftBack = (sizeof(ulongest) * 8) - shift;
		for (;;)
		{
			m_digits[i] = srcValue >> shift;
			size_t nextIndex = i + 1;
			if (nextIndex == srcLength)
				break;
			ulongest srcValueNext = srcDigits[nextIndex];
			m_digits[i] |= srcValueNext << shiftBack;
			i = nextIndex;
			srcValue = srcValueNext;
		}
		m_digits.truncate_to(srcLength);	// Might still have trailing zeroes
	}

	// Division
	void divide_whole_and_assign_modulo(const ulongest* denomDigits, size_t denomLength, dynamic_integer_content* result = 0)
	{
		// We know that neither this or denom are negative (sign is handled externally)
		// We know this > denom
		// denom > 0

		size_t denomHighIndex = denomLength - 1;
		ulongest denomHighDigitValue = denomDigits[denomHighIndex];
		size_t numerLength = m_digits.get_length();

		// exponent of two optimization
		bool isDenomPowerOfTwo = cogs::is_exponent_of_two(denomHighDigitValue);
		if (isDenomPowerOfTwo)
		{
			// Because high digit is known to be non-zero, just need to ensure all other digits are zero.
			for (size_t i = 0; i < denomHighIndex; i++)
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
			size_t bitShift = bit_scan_forward(denomHighDigitValue);
			if (!bitShift)		// No bit shifting necessary, just digit shifting
				--moduloDigits;	// Will be >0 because denom is known != 1
			size_t divideDigits = numerLength - moduloDigits;
			if (!!result)
			{
				if (!!bitShift)
					result->set_right_shifted(m_digits.m_ptr + moduloDigits - 1, divideDigits + 1, bitShift);
				else if (!!divideDigits)
					result->m_digits.assign(m_digits.m_ptr + moduloDigits, divideDigits);
				else
					result->m_digits.clear();
			}
			m_digits.truncate_to(moduloDigits);
			if (!!bitShift)
				m_digits[moduloDigits - 1] &= ~(~(ulongest)0 << bitShift);
		}
		else
		{
			// startIndex is the highest digit in destination to receive a value
			size_t startIndex = numerLength - denomLength;
			if (!!result)
				result->m_digits.assign(startIndex + 1, (ulongest)0);

			size_t highIndex = numerLength - 1;

			// Division algorithm is guess and check (divide and conquer), using the denominator's high digit (+ 1)

			// We use half-digits, to handle carries and still fit the math in native ints
			typedef bits_to_int_t<(sizeof(ulongest) * 8) / 2, false> half_unsigned_t;
			static constexpr ulongest highMask = (~(ulongest)0 ^ (half_unsigned_t)~(half_unsigned_t)0);

			for (;;)
			{
				// We special case a high divisor digit of max value, as +1 causes it to overflow into an additional digit.
				if (denomHighDigitValue >= highMask)
				{
					if (highIndex == denomHighIndex)	// highDigit of src2 is so large, there can only possibly be 1
					{
						subtract(denomDigits, denomLength);
						if (!!result)
							result->m_digits.assign(1, 1);
						break;
					}

					size_t i = (highIndex - denomHighIndex);

					do {
						i--;
						for (;;)
						{
							ulongest multiplier = m_digits[highIndex];
							if (multiplier == 0)
							{
								highIndex--;
								break;
							}
							if (!!result)
							{
								ulongest oldValue = result->m_digits[i];
								ulongest newValue = oldValue + multiplier;
								result->m_digits[i] = newValue;
								if (oldValue > newValue)
								{
									size_t incIndex = i + 1;
									while (++(result->m_digits[incIndex]) == 0)
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
									size_t decIndex = dstIndex;
									while (m_digits[++decIndex]-- == 0)
										;
								}

								if (overflow != 0)
								{
									++dstIndex;
									ulongest oldDigit = m_digits[dstIndex];
									ulongest newDigit = oldDigit - overflow;
									m_digits[dstIndex] = newDigit;
									if (newDigit > oldDigit)
									{
										size_t decIndex = dstIndex;
										while (m_digits[++decIndex]-- == 0)
											;
									}
								}
							}
						}
					} while (i != 0);

					// There is less than the rounded up value remaining, but it's still possible there is one more multiple of the denominator.
					int cmpResult = compare(denomDigits, denomLength);
					if (cmpResult != -1)
					{
						if (!!result)
							result->increment();
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
						denomHighDigitValue++;

					size_t i = startIndex;
					for (;;)
					{
						ulongest highRemainder = m_digits[highIndex];
						ulongest multiplier = 0; // Initializing this here, due to compiler error on MacOS/gcc
												 // Not actually possible for it to be uninitialized in flow below
						if (!onBoundary)
						{
							if (highRemainder >= denomHighDigitValue)
								multiplier = highRemainder / denomHighDigitValue;
							else
							{
								if (i == 0)
									break;
								i--;
								if (highRemainder == 0)
								{
									highIndex--;
									continue;
								}
								onBoundary = true;	// (highRemainder < denomHighDigitValue) && (highRemainder != 0)
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
								multiplier = (highRemainder / denomHighDigitValueHigh) << (sizeof(longest) * 4);
							else // (highRemainder < denomHighDigitValueHigh)
							{
								ulongest nextRemainderDigitValue = m_digits[highIndex - 1];
								ulongest tmpRemainder = (highRemainder << (sizeof(longest) * 4)) | (nextRemainderDigitValue >> (sizeof(longest) * 4));
								if (tmpRemainder >= denomHighDigitValue)
									multiplier = (tmpRemainder / denomHighDigitValue) << (sizeof(longest) * 4);
								else // (tmpRemainder < denomHighDigitValue)
									multiplier = tmpRemainder / denomHighDigitValueHigh;
							}
						}
						if (!!result)
						{
							ulongest oldValue = result->m_digits[i];
							ulongest newValue = oldValue + multiplier;
							result->m_digits[i] = newValue;
							if (oldValue > newValue)
							{
								size_t incIndex = i + 1;
								while (++(result->m_digits[incIndex]) == 0)
									++incIndex;
							}
						}

						ulongest overflow = 0;
						size_t dstIndex = 0; // Initializing this here, due to compiler error on MacOS/gcc
											 // Not actually possible for it to be uninitialized, as denomHighIndex cannot be less than 0
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
								size_t decIndex = dstIndex;
								while (m_digits[++decIndex]-- == 0)
									;
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
								size_t decIndex = dstIndex;
								while (m_digits[++decIndex]-- == 0)
									;
							}
						}
					}
				}
				break;
			}
		}

		if (!!result)
			result->trim_leading_zeros();
		trim_leading_zeros();
	}

	void divide_whole_and_assign_modulo(const dynamic_integer_content& denom, dynamic_integer_content& result)
	{
		size_t denomLength = denom.m_digits.get_length();
		if (!denomLength)	// divide by zero?
		{
			result.clear();
			clear();
			return;
		}

		if ((denomLength == 1) && (denom.m_digits.get_const_ptr()[0] == 1))	// div by 1 optimization
		{
			result.m_digits.acquire(m_digits);
			result.m_isNegative = (m_isNegative != denom.m_isNegative);
			clear();
			return;
		}
		int i = compare(denom);
		if (i == -1)
		{
			result.clear();
			return;
		}

		if (!i)
		{
			result.m_digits.assign(1, 1);
			result.m_isNegative = false;
			clear();
		}
		else // if (i == 1)
		{
			divide_whole_and_assign_modulo(denom.m_digits.get_const_ptr(), denom.m_digits.get_length(), &result);
			result.m_isNegative = (m_isNegative != denom.m_isNegative);
		}
	}

	void divide_whole_and_assign_modulo(const dynamic_integer_content& denom)
	{
		size_t denomLength = denom.m_digits.get_length();
		if (!denomLength	// divide by zero?
			|| ((denomLength == 1) && (denom.m_digits.get_const_ptr()[0] == 1)))	// div by 1 optimization
			clear();
		else
		{
			int i = compare(denom);
			if (i != -1)
			{
				if (!i)
					clear();
				else // if (i == 1)
					divide_whole_and_assign_modulo(denom.m_digits.get_const_ptr(), denomLength);
			}
		}
	}

	template <bool has_sign, size_t bits>
	void divide_whole_and_assign_modulo(const fixed_integer_native<has_sign, bits>& denom, dynamic_integer_content& result)
	{
		if (!denom)	// divide by zero?
		{
			result.clear();
			clear();
			return;
		}

		auto absDenom = denom.abs();
		if (absDenom == 1)	// div by 1 optimization
		{
			result.m_digits.acquire(m_digits);
			result.m_isNegative = (m_isNegative != denom.is_negative());
			clear();
			return;
		}
		int i = compare(absDenom);
		if (i == -1)
		{
			result.clear();
			return;
		}

		if (!i)
		{
			result.m_digits.assign(1, 1);
			result.m_isNegative = false;
			clear();
		}
		else // if (i == 1)
		{
			ulongest digit = absDenom;
			divide_whole_and_assign_modulo(&digit, 1, &result);
			result.m_isNegative = (m_isNegative != denom.is_negative());
		}
	}

	template <bool has_sign, size_t bits>
	void divide_whole_and_assign_modulo(const fixed_integer_native<has_sign, bits>& denom)
	{
		if (!denom)	// divide by zero?
			clear();
		else
		{
			auto absDenom = denom.abs();
			if (absDenom == 1)	// div by 1 optimization
				clear();
			else
			{
				int i = compare(absDenom);
				if (i != -1)
				{
					if (!i)
						clear();
					else // if (i == 1)
					{
						ulongest digit = absDenom.get_int();
						divide_whole_and_assign_modulo(&digit, 1);
					}
				}
			}
		}
	}

	template <bool has_sign, size_t bits>
	void divide_whole_and_assign_modulo(const fixed_integer_extended_content<has_sign, bits>& denom, dynamic_integer_content& result)
	{
		if (!denom)	// divide by zero?
		{
			result.clear();
			clear();
			return;
		}

		typename decltype(std::declval<fixed_integer_extended<has_sign, bits>&>().abs())::content_t absDenom;
		absDenom.assign_abs(denom);
		if (absDenom.equals(fixed_integer_native<0, 1>(1)))	// div by 1 optimization
		{
			result.m_digits.acquire(m_digits);
			result.m_isNegative = (m_isNegative != denom.is_negative());
			clear();
			return;
		}
		int i = compare(absDenom);
		if (i == -1)
		{
			result.clear();
			return;
		}

		if (!i)
		{
			result.m_digits.assign(1, 1);
			result.m_isNegative = false;
			clear();
		}
		else // if (i == 1)
		{
			ulongest* denomDigits = absDenom.get_digits();
			size_t i = decltype(std::declval<fixed_integer_extended<has_sign, bits>&>().abs())::n_digits - 1;
			while (denomDigits[i] == 0)	// Trim leading zeros
				i--;
			divide_whole_and_assign_modulo(denomDigits, i, &result);
			result.m_isNegative = (m_isNegative != denom.is_negative());
		}
	}

	template <bool has_sign, size_t bits>
	void divide_whole_and_assign_modulo(const fixed_integer_extended_content<has_sign, bits>& denom)
	{
		if (!denom)	// divide by zero?
			clear();
		else
		{
			typename decltype(std::declval<fixed_integer_extended<has_sign, bits>&>().abs())::content_t absDenom;
			absDenom.assign_abs(denom);
			if (absDenom.equals(fixed_integer_native<0, 1>(1)))	// div by 1 optimization
				clear();
			else
			{
				int i = compare(absDenom);
				if (i != -1)
				{
					if (!i)
						clear();
					else // if (i == 1)
					{
						ulongest* denomDigits = absDenom.get_digits();
						size_t i = decltype(std::declval<fixed_integer_extended<has_sign, bits>&>().abs())::n_digits - 1;
						while (denomDigits[i] == 0)	// Trim leading zeros
							i--;
						divide_whole_and_assign_modulo(denomDigits, i);
					}
				}
			}
		}
	}

	bool assign_reciprocal()
	{
		size_t length = m_digits.get_length();
		if (length == 0)
		{
			COGS_ASSERT(false);	// no recip of 0
			return false;
		}
		else
		{
			for (;;)
			{
				if (length == 1)
				{
					ulongest lowDigit = m_digits.get_const_ptr()[0];
					if (lowDigit == 1)	// if full value is 1 or -1, leave intact
						return false;
				}

				clear();
				return true;
			}
		}
	}

	void set_to_gcd3(int cmpValue, const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		dynamic_integer_content t1(d1);
		dynamic_integer_content t2(d2);
		t1.acquire();
		t2.acquire();
		if (cmpValue == -1)
			t1.swap(t2);
		for (;;)
		{
			t1.divide_whole_and_assign_modulo(t2.m_digits.get_const_ptr(), t2.m_digits.get_length(), this);
			if (!t1)
				break;
			t1.swap(t2);
			continue;
		}

		t1.release();
		t2.release();
	}

	void set_to_gcd2(const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		int cmpValue = d1.compare(d2.m_digits.get_const_ptr(), d2.m_digits.get_length());
		if (cmpValue == 0)
			assign(d1);
		else
			set_to_gcd3(cmpValue, d1, d2);
	}

	void set_to_gcd(const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		// TODO: Replace with more efficient GCD algorithm if extremely large values
		if (!d1)	// GCD with zero is the absolute of the non-zero value
		{
			if (!d2)
				m_digits.clear();
			else
				assign(d2);
		}
		else if (!d2)
			assign(d1);
		else
			set_to_gcd2(d1, d2);
		m_isNegative = false;
	}

	template <bool has_sign, size_t bits>
	void set_to_gcd2(const dynamic_integer_content& d1, const fixed_integer_native<has_sign, bits>& d2)
	{
		ulongest digit = d2;
		int cmpValue = d1.compare(&digit, 1);
		if (cmpValue == 0)
			assign(d1);
		else if (cmpValue == -1)	// If a positive dynamic_integer is less than a fixed_integer_native, it must be <=1 digits
			operator=(d2.gcd(d1.m_digits.get_const_ptr()[0]));
		else
		{			// Since remainder will fit within fixed_integer_native after the first round, just do one round and pass it on.
			dynamic_integer_content r(d1);
			r.acquire();
			r.divide_whole_and_assign_modulo(&digit, 1, this);
			if (!!r)
				operator=(d2.gcd(r.m_digits.get_const_ptr()[0]));
			r.release();
		}
	}

	template <bool has_sign, size_t bits>
	void set_to_gcd(const dynamic_integer_content& d1, const fixed_integer_native<has_sign, bits>& d2)
	{
		// TODO: Replace with more efficient GCD algorithm if extremely large values
		if (!d1)	// GCD with zero is the absolute of the non-zero value
		{
			if (!d2)
				m_digits.clear();
			else
				operator=(d2.abs());
		}
		else if (!d2)
			assign(d1);
		else
			set_to_gcd2(d1, d2.abs());
		m_isNegative = false;
	}

	template <bool has_sign, size_t bits>
	void set_to_gcd2(const dynamic_integer_content& d1, const fixed_integer_extended_content<has_sign, bits>& d2)
	{
		int cmpValue = d1.compare(d2.m_digits.get_const_ptr(), d2.m_digits.get_length());
		if (cmpValue == 0)
			assign(d1);
		else
		{
			dynamic_integer_content t2(d2);	// We'll need a buffer for intermediate results anyway
			set_to_gcd3(cmpValue, d1, t2);
		}
	}

	template <bool has_sign, size_t bits>
	void set_to_gcd(const dynamic_integer_content& d1, const fixed_integer_extended_content<has_sign, bits>& d2)
	{
		// TODO: Replace with more efficient GCD algorithm if extremely large values
		if (!d1)	// GCD with zero is the absolute of the non-zero value
		{
			if (!d2)
				m_digits.clear();
			else
				operator=(d2.abs());
		}
		else if (!d2)
			assign(d1);
		else
			set_to_gcd2(d1, d2.abs());
		m_isNegative = false;
	}

	void set_to_lcm2(const dynamic_integer_content& tmpGcd)
	{
		dynamic_integer_content r(*this);
		r.acquire();
		r.divide_whole_and_assign_modulo(tmpGcd.get_const_ptr(), tmpGcd.get_length(), this);
		r.release();
		tmpGcd.release();
		m_isNegative = false;
	}

	void set_to_lcm(const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		// TODO: Replace with more efficient LCM algorithm if extremely large values
		if (!d1 || !d2)
		{
			clear();
			return;
		}

		dynamic_integer_content tmpGcd;
		tmpGcd.set_to_gcd2(d1, d2);
		assign(d1);
		multiply(d2);
		set_to_lcm2(tmpGcd);
	}

	template <bool has_sign, size_t bits>
	void set_to_lcm(const dynamic_integer_content& d1, const fixed_integer_native<has_sign, bits>& d2)
	{
		// TODO: Replace with more efficient LCM algorithm if extremely large values
		if (!d1 || !d2)
		{
			clear();
			return;
		}

		auto d2abs = d2.abs();
		dynamic_integer_content tmpGcd;
		tmpGcd.set_to_gcd2(d1, d2abs);
		assign(d1);
		multiply2(d2abs);
		set_to_lcm2(tmpGcd);
	}

	template <bool has_sign, size_t bits>
	void set_to_lcm(const dynamic_integer_content& d1, const fixed_integer_extended_content<has_sign, bits>& d2)
	{
		// TODO: Replace with more efficient LCM algorithm if extremely large values
		if (!d1 || !d2)
		{
			clear();
			return;
		}

		decltype(std::declval<fixed_integer_extended_content<has_sign, bits> >().abs()) d2abs;
		d2abs.assign_abs(d2);
		dynamic_integer_content tmpGcd;
		tmpGcd.set_to_gcd2(d1, d2);
		assign(d1);
		multiply2(d2);
		set_to_lcm2(tmpGcd);
	}

	// greater
	void set_to_greater(const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		if (d1.is_greater_than(d2))
			assign(d1);
		else
			assign(d2);
	}

	template <bool has_sign, size_t bits>
	void set_to_greater(const dynamic_integer_content& d1, const fixed_integer_native<has_sign, bits>& d2)
	{
		if (d1.is_greater_than(d2))
			assign(d1);
		else
			assign(d2);
	}

	template <bool has_sign, size_t bits>
	void set_to_greater(const dynamic_integer_content& d1, const fixed_integer_extended_content<has_sign, bits>& d2)
	{
		if (d1.is_greater_than(d2))
			assign(d1);
		else
			assign(d2);
	}

	// lesser
	void set_to_lesser(const dynamic_integer_content& d1, const dynamic_integer_content& d2)
	{
		if (d1.is_less_than(d2))
			assign(d1);
		else
			assign(d2);
	}

	template <bool has_sign, size_t bits>
	void set_to_lesser(const dynamic_integer_content& d1, const fixed_integer_native<has_sign, bits>& d2)
	{
		if (d1.is_less_than(d2))
			assign(d1);
		else
			assign(d2);
	}

	template <bool has_sign, size_t bits>
	void set_to_lesser(const dynamic_integer_content& d1, const fixed_integer_extended_content<has_sign, bits>& d2)
	{
		if (d1.is_less_than(d2))
			assign(d1);
		else
			assign(d2);
	}


	// to_string
	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const
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

		// Handle negative values using a 2-way table.
		static constexpr char textDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		size_t maxLength = (m_digits.get_length() * sizeof(ulongest) * 8) + 2;	// Enough space for largest possible value, i.e. binary radix

		char tempBufferStorage[512];
		ptr<char> tempBuffer = tempBufferStorage;
		if (maxLength > 512)
			tempBuffer = default_allocator::allocate_type<char>(maxLength);

		size_t i = 0;
		bool isNeg = is_negative();
		dynamic_integer_content src(*this);
		src.acquire();
		src.m_isNegative = false;

		ulongest radixDigit = radix;
		while (!!src)
		{
			int i = src.compare(radix);
			if (i == -1)
			{
				tempBuffer[i++] = textDigits[src.get_int()];
				break;
			}
			if (!i)
			{
				tempBuffer[i++] = textDigits[0];
				tempBuffer[i++] = textDigits[1];
				break;
			}

			dynamic_integer_content remainder(src);
			remainder.acquire();
			remainder.divide_whole_and_assign_modulo(&radixDigit, 1, &src);
			tempBuffer[i++] = textDigits[remainder.get_int()];
			remainder.release();
		}

		src.release();

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

};



/// @ingroup Math
/// @brief An integer with a dynamic number of bits
class dynamic_integer
{
public:
	typedef ulongest int_t;

	template <bool, size_t>
	friend class fixed_integer_native;

	template <bool, size_t>
	friend class fixed_integer_extended;

	template <bool, size_t>
	friend class fixed_integer_extended_content;

private:
	typedef dynamic_integer_content content_t;
	typedef dynamic_integer_content::desc_t				desc_t;
	typedef dynamic_integer_content::vector_content_t	vector_content_t;

	typedef transactable<dynamic_integer_content> transactable_t;
	transactable_t	m_contents;

	typedef typename transactable_t::read_token		read_token;
	typedef typename transactable_t::write_token	write_token;

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


	read_token guarded_begin_read() const volatile
	{
		read_token rt;
		guarded_begin_read(rt);
		return rt;
	}

	void guarded_begin_read(read_token& rt) const volatile
	{
		volatile hazard& h = rc_obj_base::get_hazard();
		desc_t* desc;
		hazard::pointer p;
		for (;;)
		{
			m_contents.begin_read(rt);
			desc = rt->m_digits.m_desc;
			if (!desc)
				break;
			p.bind(h, desc);
			if (!m_contents.is_current(rt) || !p.validate())
				continue;
			bool acquired = desc->acquire();
			if (p.release())
				desc->dispose();
			if (!acquired)
				continue;
			break;
		}
	}

	write_token guarded_begin_write() volatile
	{
		write_token wt;
		guarded_begin_write(wt);
		return wt;
	}

	void guarded_begin_write(write_token& wt) volatile
	{
		volatile hazard& h = rc_obj_base::get_hazard();
		desc_t* desc;
		hazard::pointer p;
		for (;;)
		{
			m_contents.begin_write(wt);
			desc = wt->m_digits.m_desc;
			if (!desc)
				break;
			p.bind(h, desc);
			if (!m_contents.is_current(wt) || !p.validate())
				continue;
			bool acquired = desc->acquire();
			if (p.release())
				desc->dispose();
			if (!acquired)
				continue;
			break;
		}
	}


	template <class functor_t>
	void guarded_write_retry_loop(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			desc_t* oldDesc = wt->m_digits.m_desc;
			fctr(*wt);
			desc_t* newDesc = wt->m_digits.m_desc;
			if (end_write(wt))
			{
				if (!!oldDesc)
					oldDesc->release();
				break;
			}
			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release();
		}
	}

	template <class functor_t>
	auto guarded_write_retry_loop_pre(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			desc_t* oldDesc = wt->m_digits.m_desc;
			fctr(*wt);
			auto result = *wt;
			desc_t* newDesc = wt->m_digits.m_desc;
			if (!!newDesc)
				newDesc->acquire();
			if (end_write(wt))
			{
				if (!!oldDesc)
					oldDesc->release();
				return result;
			}

			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release_strong(2);
		}
	}

	template <class functor_t>
	auto guarded_write_retry_loop_post(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			auto result = *wt;
			desc_t* oldDesc = wt->m_digits.m_desc;
			fctr(*wt);
			desc_t* newDesc = wt->m_digits.m_desc;
			if (end_write(wt))
				return result;
			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release();
		}
	}

	template <class functor_t, class on_fail_t>
	void guarded_write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				desc_t* oldDesc = wt->m_digits.m_desc;
				fctr(*wt);
				desc_t* newDesc = wt->m_digits.m_desc;
				if (end_write(wt))
				{
					if (!!oldDesc)
						oldDesc->release();
					break;
				}
				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release();
			}
			onFail();
		}
	}

	template <class functor_t, class on_fail_t>
	auto guarded_write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				desc_t* oldDesc = wt->m_digits.m_desc;
				fctr(*wt);
				auto result = *wt;
				desc_t* newDesc = wt->m_digits.m_desc;
				if (!!newDesc)
					newDesc->acquire();
				if (end_write(wt))
				{
					if (!!oldDesc)
						oldDesc->release();
					return result;
				}

				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release_strong(2);
			}
			onFail();
		}
	}

	template <class functor_t, class on_fail_t>
	auto guarded_write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				auto result = *wt;
				desc_t* oldDesc = wt->m_digits.m_desc;
				fctr(*wt);
				desc_t* newDesc = wt->m_digits.m_desc;
				if (end_write(wt))
					return result;
				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release();
			}
			onFail();
		}
	}

	template <class functor_t>
	void try_guarded_write_retry_loop(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			desc_t* oldDesc = wt->m_digits.m_desc;
			if (!fctr(*wt))
			{
				if (!!oldDesc)
					oldDesc->release();
				break;
			}
			desc_t* newDesc = wt->m_digits.m_desc;
			if (end_write(wt))
			{
				if (!!oldDesc)
					oldDesc->release();
				break;
			}

			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release();
		}
	}

	template <class functor_t>
	auto try_guarded_write_retry_loop_pre(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			desc_t* oldDesc = wt->m_digits.m_desc;
			if (!fctr(*wt))
				return *wt;
			auto result = *wt;
			desc_t* newDesc = wt->m_digits.m_desc;
			if (!!newDesc)
				newDesc->acquire();
			if (end_write(wt))
			{
				if (!!oldDesc)
					oldDesc->release();
				return result;
			}
			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release_strong(2);
		}
	}

	template <class functor_t>
	auto try_guarded_write_retry_loop_post(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			auto result = *wt;
			desc_t* oldDesc = wt->m_digits.m_desc;
			if (!fctr(*wt))
				return result;
			desc_t* newDesc = wt->m_digits.m_desc;
			if (end_write(wt))
				return result;
			if (!!oldDesc)
				oldDesc->release();
			if (!!newDesc)
				newDesc->release();
		}
	}

	template <class functor_t, class on_fail_t>
	void try_guarded_write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				desc_t* oldDesc = wt->m_digits.m_desc;
				if (!fctr(*wt))
				{
					if (!!oldDesc)
						oldDesc->release();
					break;
				}
				desc_t* newDesc = wt->m_digits.m_desc;
				if (end_write(wt))
				{
					if (!!oldDesc)
						oldDesc->release();
					break;
				}

				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release();
			}
			onFail();
		}
	}

	template <class functor_t, class on_fail_t>
	auto try_guarded_write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				desc_t* oldDesc = wt->m_digits.m_desc;
				if (!fctr(*wt))
					return *wt;
				auto result = *wt;
				desc_t* newDesc = wt->m_digits.m_desc;
				if (!!newDesc)
					newDesc->acquire();
				if (end_write(wt))
				{
					if (!!oldDesc)
						oldDesc->release();
					return result;
				}
				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release_strong(2);
			}
			onFail();
		}
	}

	template <class functor_t, class on_fail_t>
	auto try_guarded_write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				guarded_begin_write(wt);
				auto result = *wt;
				desc_t* oldDesc = wt->m_digits.m_desc;
				if (!fctr(*wt))
					return result;
				desc_t* newDesc = wt->m_digits.m_desc;
				if (end_write(wt))
					return result;
				if (!!oldDesc)
					oldDesc->release();
				if (!!newDesc)
					newDesc->release();
			}
			onFail();
		}
	}

	dynamic_integer(const vector_content_t& content, bool isNegative)	// does not acquire
		: m_contents(typename transactable_t::construct_embedded_t(), content, isNegative)
	{ }

	dynamic_integer(const dynamic_integer_content& content)	// does not acquire
		: m_contents(typename transactable_t::construct_embedded_t(), content)
	{ }

	dynamic_integer& operator=(const vector_content_t& content)	// does not acquire, but releases existing
	{
		*m_contents = content;
		return *this;
	}


	template <bool has_sign, size_t bits>
	dynamic_integer divide_whole(const fixed_integer_extended_content<has_sign, bits>& src) const
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, *(result.m_contents));
		return result;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer gcd(const fixed_integer_extended_content<has_sign, bits>& cmp) const
	{
		dynamic_integer result;
		result.m_contents->set_to_gcd(*m_contents, cmp);
		return result;
	}

	template <typename T>
	bool compare_exchange2(const T& src, const dynamic_integer& cmp, dynamic_integer& rtn) volatile
	{
		bool b;
		write_token wt;
		guarded_begin_write(wt);				// acquires
		if (!wt->equals(*(cmp.m_contents)))
		{
			*(rtn.m_contents) = *wt;			// hands off ownership to rtn.
			b = false;
		}
		else
		{
			dynamic_integer tmp(src);
			for (;;)
			{
				dynamic_integer_content oldContent(*wt);
				*wt = *(tmp.m_contents);				// Does not take ownership from tmp.
				if (end_write(wt))
				{
					*(rtn.m_contents) = oldContent;		// hands off ownership to rtn.
					tmp.m_contents->m_digits.clear_inner();	// tmp hands off ownership to this object.
					b = true;
					break;
				}

				oldContent.release();					// releases
				guarded_begin_write(wt);				// acquires
				if (!wt->equals(*(cmp.m_contents)))
				{
					*(rtn.m_contents) = *wt;			// hands off ownership to rtn.
					b = false;
					break;
				}
			}
		}
		return b;
	}

	template <typename T>
	bool compare_exchange2(const T& src, const dynamic_integer& cmp) volatile
	{
		bool b;
		write_token wt;
		guarded_begin_write(wt);				// acquires
		if (!wt->equals(*(cmp.m_contents)))
		{
			wt->release();						// releases
			b = false;
		}
		else
		{
			dynamic_integer tmp(src);
			for (;;)
			{
				dynamic_integer_content oldContent(*wt);
				*wt = *(tmp.m_contents);				// Does not take ownership from tmp.
				if (end_write(wt))
				{
					oldContent.release();				// releases
					tmp.m_contents->m_digits.clear_inner();	// tmp hands off ownership to this object.
					b = true;
					break;
				}

				oldContent.release();					// releases
				guarded_begin_write(wt);				// acquires
				if (!wt->equals(*(cmp.m_contents)))
				{
					wt->release();						// releases
					b = false;
					break;
				}
			}
		}
		return b;
	}

public:
	dynamic_integer()
	{ }

	dynamic_integer(const dynamic_integer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{
		m_contents->acquire();
	}

	dynamic_integer(dynamic_integer&& src)
		: m_contents(typename transactable_t::construct_embedded_t(), std::move(*(src.m_contents)))
	{
	}

	dynamic_integer(const volatile dynamic_integer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read()))
	{
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const fixed_integer_native_const<has_sign, bits, 0>& src)
	{
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const volatile fixed_integer_native_const<has_sign, bits, 0>& src)
	{
	}

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	dynamic_integer(const fixed_integer_native_const<has_sign, bits, value>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	dynamic_integer(const volatile fixed_integer_native_const<has_sign, bits, value>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits, ulongest... values>
	dynamic_integer(const fixed_integer_extended_const<has_sign, bits, values...>& src)
	{
		operator=(src);
	}


	template <bool has_sign, size_t bits, ulongest... values>
	dynamic_integer(const volatile fixed_integer_extended_const<has_sign, bits, values...>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const fixed_integer_native<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const volatile fixed_integer_native<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const fixed_integer_extended<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <bool has_sign, size_t bits>
	dynamic_integer(const volatile fixed_integer_extended<has_sign, bits>& src)
	{
		operator=(src);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer(const int_t2& src)
	{
		operator=(src);
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer(const volatile int_t2& src)
	{
		operator=(src);
	}

	~dynamic_integer()
	{
		m_contents->release();
	}

	template <typename int_t = ulongest>
	int_t get_int() const { return m_contents->get_int<int_t>(); }

	template <typename int_t = ulongest>
	int_t get_int() const volatile
	{
		dynamic_integer cpy(*this);
		return cpy.get_int<int_t>();
	}

	dynamic_integer& operator=(const dynamic_integer& src)
	{
		if (this != &src)
		{
			*m_contents = (*(src.m_contents));
			m_contents->acquire();
		}
		return *this;
	}

	dynamic_integer& operator=(dynamic_integer&& src)
	{
		*m_contents = (std::move(*(src.m_contents)));
		return *this;
	}

	volatile dynamic_integer& operator=(const dynamic_integer& src) volatile
	{
		COGS_ASSERT(this != &src);
		dynamic_integer tmp(src);
		exchange(tmp);
		return *this;
	}

	dynamic_integer& operator=(const volatile dynamic_integer& src)
	{
		read_token rt = src.guarded_begin_read();
		*m_contents = (*rt);
		rt->release();
		return *this;
	}

	volatile dynamic_integer& operator=(const volatile dynamic_integer& src) volatile
	{
		if (this != &src)
		{
			dynamic_integer tmp(src);
			exchange(tmp);
		}
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const fixed_integer_native_const<has_sign, bits, 0>& src)
	{
		clear();
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const volatile fixed_integer_native_const<has_sign, bits, 0>& src)
	{
		clear();
		return *this;
	}

	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const fixed_integer_native_const<has_sign, bits, 0>& src) volatile
	{
		clear();
		return *this;
	}


	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const volatile fixed_integer_native_const<has_sign, bits, 0>& src) volatile
	{
		clear();
		return *this;
	}

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	dynamic_integer& operator=(const fixed_integer_native_const<has_sign, bits, value>& src)
	{
		typename fixed_integer_native_const<has_sign, bits, value>::non_const_t tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	dynamic_integer& operator=(const volatile fixed_integer_native_const<has_sign, bits, value>& src)
	{
		typename fixed_integer_native_const<has_sign, bits, value>::non_const_t tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	volatile dynamic_integer& operator=(const fixed_integer_native_const<has_sign, bits, value>& src) volatile
	{
		typename fixed_integer_native_const<has_sign, bits, value>::non_const_t tmp(src);
		*this = tmp;
		return *this;
	}
	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
	volatile dynamic_integer& operator=(const volatile fixed_integer_native_const<has_sign, bits, value>& src) volatile
	{
		typename fixed_integer_native_const<has_sign, bits, value>::non_const_t tmp(src);
		*this = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits, ulongest... values>
	dynamic_integer& operator=(const fixed_integer_extended_const<has_sign, bits, values...>& src)
	{
		typename fixed_integer_extended_const<has_sign, bits, values...>::non_const_t tmp(src);
		*m_contents = tmp;
		return *this;
	}


	template <bool has_sign, size_t bits, ulongest... values>
	dynamic_integer& operator=(const volatile fixed_integer_extended_const<has_sign, bits, values...>& src)
	{
		typename fixed_integer_extended_const<has_sign, bits, values...>::non_const_t tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits, ulongest... values>
	volatile dynamic_integer& operator=(const fixed_integer_extended_const<has_sign, bits, values...>& src) volatile
	{
		typename fixed_integer_extended_const<has_sign, bits, values...>::non_const_t tmp(src);
		*this = tmp;
		return *this;
	}


	template <bool has_sign, size_t bits, ulongest... values>
	volatile dynamic_integer& operator=(const volatile fixed_integer_extended_const<has_sign, bits, values...>& src) volatile
	{
		typename fixed_integer_extended_const<has_sign, bits, values...>::non_const_t tmp(src);
		*this = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const fixed_integer_native<has_sign, bits>& src)
	{
		*m_contents = src;
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const volatile fixed_integer_native<has_sign, bits>& src)
	{
		fixed_integer_native<has_sign, bits> tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const fixed_integer_native<has_sign, bits>& src) volatile
	{
		dynamic_integer tmp(src);
		exchange(tmp);
		return *this;
	}

	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const volatile fixed_integer_native<has_sign, bits>& src) volatile
	{
		dynamic_integer tmp(src);
		exchange(tmp);
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const fixed_integer_extended<has_sign, bits>& src)
	{
		*m_contents = (*(src.m_contents));
		return *this;
	}

	template <bool has_sign, size_t bits>
	dynamic_integer& operator=(const volatile fixed_integer_extended<has_sign, bits>& src)
	{
		*m_contents = (*(src.begin_read()));
		return *this;
	}

	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const fixed_integer_extended<has_sign, bits>& src) volatile
	{
		dynamic_integer tmp(src);
		exchange(tmp);
		return *this;
	}

	template <bool has_sign, size_t bits>
	volatile dynamic_integer& operator=(const volatile fixed_integer_extended<has_sign, bits>& src) volatile
	{
		dynamic_integer tmp(src);
		exchange(tmp);
		return *this;
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer& operator=(const int_t2& src)
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer& operator=(const volatile int_t2& src)
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		*m_contents = tmp;
		return *this;
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile dynamic_integer& operator=(const int_t2& src) volatile
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		*this = tmp;
		return *this;
	}

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile dynamic_integer& operator=(const volatile int_t2& src) volatile
	{
		int_to_fixed_integer_t<int_t2> tmp(src);
		*this = tmp;
		return *this;
	}

	void clear() { m_contents->clear(); }
	void clear() volatile { m_contents.set(); }

	bool operator!() const { return !*m_contents; }
	bool operator!() const volatile { return !*begin_read(); }	// Don't need buffer, just its length

	bool is_negative() const { return m_contents->is_negative(); }
	bool is_negative() const volatile { return begin_read()->is_negative(); }

	bool is_exponent_of_two() const { return m_contents->is_exponent_of_two(); }
	bool is_exponent_of_two() const volatile
	{
		read_token rt = guarded_begin_read();	// acquires
		bool result = rt->is_exponent_of_two();
		rt->release();
		return result;
	}

	constexpr bool has_fractional_part() const volatile { return false; }

	// absolute
	dynamic_integer abs() const
	{
		dynamic_integer result(m_contents->m_digits, false);
		result.m_contents->acquire();
		return result;
	}

	dynamic_integer abs() const volatile
	{
		dynamic_integer result(guarded_begin_read()->m_digits, false);
		return result;
	}

	void assign_abs()
	{
		m_contents->m_isNegative = false;
	}

	void assign_abs() volatile
	{
		try_write_retry_loop([&](content_t& c) { if (!c.m_isNegative) return false; c.m_isNegative = false; return true; });
	}

	const dynamic_integer& pre_assign_abs()
	{
		m_contents->m_isNegative = false;
		return *this;
	}

	dynamic_integer pre_assign_abs() volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			dynamic_integer result(wt->m_digits, false);	// acquired reference transferred to result,
			wt->m_isNegative = false;
			if (end_write(wt))								// safe because absolute will not reallocate
				return result;
		}
	}

	dynamic_integer post_assign_abs()
	{
		dynamic_integer result(*this);
		m_contents->m_isNegative = false;
		return result;
	}

	dynamic_integer post_assign_abs() volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			dynamic_integer result(wt->m_digits, wt->m_isNegative);	// acquired reference transferred to result,
			wt->m_isNegative = false;								// safe because absolute will not reallocate
			if (end_write(wt))
				return result;
		}
	}


	// negative
	dynamic_integer operator-() const
	{
		dynamic_integer result(m_contents->m_digits, !m_contents->m_isNegative);
		result.m_contents->acquire();
		return result;
	}

	dynamic_integer operator-() const volatile
	{
		read_token rt = guarded_begin_read();	// acquires
		dynamic_integer result(rt->m_digits, !rt->m_isNegative);
		return result;
	}

	void assign_negative()
	{
		m_contents->negate();
	}

	void assign_negative() volatile
	{
		write_retry_loop([&](content_t& c) { c.negate(); });
	}

	const dynamic_integer& pre_assign_negative()
	{
		m_contents->negate();
		return *this;
	}

	dynamic_integer pre_assign_negative() volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			wt->negate();
			dynamic_integer result(wt->m_digits, wt->m_isNegative);	// acquired reference transferred to result,
			if (end_write(wt))										// safe because negate will not reallocate
				return result;
		}
	}

	dynamic_integer post_assign_negative()
	{
		dynamic_integer result(*this);
		m_contents->negate();
		return result;
	}

	dynamic_integer post_assign_negative() volatile
	{
		write_token wt;
		for (;;)
		{
			guarded_begin_write(wt);
			dynamic_integer result(wt->m_digits, wt->m_isNegative);	// acquired reference transferred to result,
			wt->negate();											// safe because negate will not reallocate
			if (end_write(wt))
				return result;
		}
	}

	// next
	dynamic_integer next() const { dynamic_integer tmp(*this); tmp.assign_next(); return tmp; }
	dynamic_integer next() const volatile { dynamic_integer tmp(*this); tmp.assign_next(); return tmp; }
	void assign_next() { if (m_contents->m_isNegative) m_contents->decrement(); else m_contents->increment(); }
	void assign_next() volatile { write_retry_loop([&](content_t& c) { if (c.m_isNegative) c.decrement(); else c.increment(); }); }
	const dynamic_integer& operator++() { assign_next(); return *this; }
	dynamic_integer operator++() volatile { return guarded_write_retry_loop_pre([&](content_t& c) { if (c.m_isNegative) c.decrement(); else c.increment(); }); }
	dynamic_integer operator++(int) { dynamic_integer result(*this); assign_next(); return result; }
	dynamic_integer operator++(int) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { if (c.m_isNegative) c.decrement(); else c.increment(); }); }

	// prev
	dynamic_integer prev() const { dynamic_integer tmp(*this); tmp.assign_prev(); return tmp; }
	dynamic_integer prev() const volatile { dynamic_integer tmp(*this); tmp.assign_prev(); return tmp; }
	void assign_prev() { if (m_contents->m_isNegative) m_contents->increment(); else m_contents->decrement(); }
	void assign_prev() volatile { write_retry_loop([&](content_t& c) { if (c.m_isNegative) c.increment(); else c.decrement(); }); }
	const dynamic_integer& operator--() { assign_prev(); return *this; }
	dynamic_integer operator--() volatile { return guarded_write_retry_loop_pre([&](content_t& c) { if (c.m_isNegative) c.increment(); else c.decrement(); }); }
	dynamic_integer operator--(int) { dynamic_integer result(*this); assign_prev(); return result; }
	dynamic_integer operator--(int) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { if (c.m_isNegative) c.increment(); else c.decrement(); }); }

	// add
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->add(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->add(*rt, src); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+(tmp); }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->add(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->add(*rt, *(src.m_contents)); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->add(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->add(*rt, *(src.begin_read())); rt->release(); return result; }
	auto operator+(const dynamic_integer& src) const { dynamic_integer result; result.m_contents->add(*m_contents, *(src.m_contents)); return result; }
	auto operator+(const dynamic_integer& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->add(*rt, *(src.m_contents)); rt->release(); return result; }
	auto operator+(const volatile dynamic_integer& src) const { dynamic_integer result; read_token rt = src.guarded_begin_read(); result.m_contents->add(*m_contents, *rt); rt->release(); return result; }
	auto operator+(const volatile dynamic_integer& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); read_token rt2 = src.guarded_begin_read(); result.m_contents->add(*rt, *rt2); rt->release(); rt2->release(); return result; }

	template <bool has_sign2, size_t bits2> const dynamic_integer& operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
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

	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->add(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.add(src); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.add(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->add(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.add(*(src.begin_read())); }); return *this; }
	dynamic_integer& operator+=(const dynamic_integer& src) { m_contents->add((this == &src) ? *(dynamic_integer(src).m_contents) : *(src.m_contents)); return *this; }
	volatile dynamic_integer& operator+=(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.add(*(src.m_contents)); }); return *this; }
	dynamic_integer& operator+=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator+=(tmp); }
	volatile dynamic_integer& operator+=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator+=(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator+=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator+=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator+=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator+=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator+=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator+=(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(src); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.begin_read())); }); }
	const dynamic_integer& pre_assign_add(const dynamic_integer& src) { *this += src; return *this; }
	dynamic_integer pre_assign_add(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.m_contents)); }); }
	const dynamic_integer& pre_assign_add(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_add(tmp); }
	dynamic_integer pre_assign_add(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_add(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_add(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_add(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_add(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.begin_read())); }); }
	dynamic_integer post_assign_add(const dynamic_integer& src) { dynamic_integer tmp(*this); *this += src; return tmp; }
	dynamic_integer post_assign_add(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.add(*(src.m_contents)); }); }
	dynamic_integer post_assign_add(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_add(tmp); }
	dynamic_integer post_assign_add(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_add(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_add(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_add(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_add(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_add(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_add(tmp); }

	// subtract
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->subtract(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->subtract(*rt, src); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-(tmp); }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-(tmp); }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->subtract(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->subtract(*rt, *(src.m_contents)); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->subtract(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->subtract(*rt, *(src.begin_read())); rt->release(); return result; }
	auto operator-(const dynamic_integer& src) const { dynamic_integer result; if (this == &src) { return result; } result.m_contents->subtract(*m_contents, *(src.m_contents)); return result; }
	auto operator-(const dynamic_integer& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->subtract(*rt, *(src.m_contents)); rt->release(); return result; }
	auto operator-(const volatile dynamic_integer& src) const { dynamic_integer result; read_token rt = src.guarded_begin_read(); result.m_contents->subtract(*m_contents, *rt); rt->release(); return result; }
	auto operator-(const volatile dynamic_integer& src) const volatile { dynamic_integer result; if (this == &src) { return result; } read_token rt = guarded_begin_read(); read_token rt2 = src.guarded_begin_read(); result.m_contents->subtract(*rt, *rt2); rt->release(); rt2->release(); return result; }

	template <bool has_sign2, size_t bits2> const dynamic_integer& operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
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

	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->subtract(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.subtract(src); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->subtract(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.subtract(*(src.begin_read())); }); return *this; }
	dynamic_integer& operator-=(const dynamic_integer& src) { if (this == &src) clear(); else m_contents->subtract(*(src.m_contents)); return *this; }
	volatile dynamic_integer& operator-=(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.subtract(*(src.m_contents)); }); return *this; }
	dynamic_integer& operator-=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); m_contents->subtract(*(tmp.m_contents)); return *this; }
	volatile dynamic_integer& operator-=(const volatile dynamic_integer& src) volatile { if (this == &src) { clear(); return *this; } dynamic_integer tmp(src); return operator-=(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator-=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator-=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator-=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator-=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator-=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator-=(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { *this -= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(src); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { *this -= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this -= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read())); }); }
	const dynamic_integer& pre_assign_subtract(const dynamic_integer& src) { *this -= src; return *this; }
	dynamic_integer pre_assign_subtract(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	const dynamic_integer& pre_assign_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); m_contents->subtract(*(tmp.m_contents)); return *this; }
	dynamic_integer pre_assign_subtract(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; clear(); return tmp2; } dynamic_integer tmp(src); return pre_assign_subtract(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_subtract(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this -= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this -= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this -= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.begin_read())); }); }
	dynamic_integer post_assign_subtract(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) clear(); else m_contents->subtract(*(src.m_contents)); return tmp; }
	dynamic_integer post_assign_subtract(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.subtract(*(src.m_contents)); }); }
	dynamic_integer post_assign_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); dynamic_integer tmp2(*this); m_contents->subtract(*(tmp.m_contents)); return tmp2; }
	dynamic_integer post_assign_subtract(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; return exchange(tmp2); } dynamic_integer tmp(src); return post_assign_subtract(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_subtract(tmp); }

	// inverse_subtract
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return -*this + src; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return -*this + src; }
	auto inverse_subtract(const dynamic_integer& src) const { if (this == &src) { dynamic_integer result; return result; } return -*this + src; }
	auto inverse_subtract(const dynamic_integer& src) const volatile { return -*this + src; }
	auto inverse_subtract(const volatile dynamic_integer& src) const { return -*this + src; }
	auto inverse_subtract(const volatile dynamic_integer& src) const volatile { if (this == &src) { dynamic_integer result; return result; } return -*this + src; }

	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { assign_negative(); *this += src; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.negate(); c.add(src); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { assign_negative(); *this += src; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_negative(); *this += src; }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.negate(); c.add(*(src.begin_read())); }); }
	void assign_inverse_subtract(const dynamic_integer& src) { if (this == &src) clear(); else { assign_negative(); m_contents->add(*(src.m_contents)); } }
	void assign_inverse_subtract(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	void assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); assign_negative(); m_contents->add(*(tmp.m_contents)); }
	void assign_inverse_subtract(const volatile dynamic_integer& src) volatile { if (this == &src) { clear(); } else { dynamic_integer tmp(src); return assign_inverse_subtract(tmp); } }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { assign_negative(); *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.negate(); c.add(src); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { assign_negative(); *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_negative(); *this += src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.negate(); c.add(*(src.begin_read())); }); }
	const dynamic_integer& pre_assign_inverse_subtract(const dynamic_integer& src) { assign_inverse_subtract(src); return *this; }
	dynamic_integer pre_assign_inverse_subtract(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	const dynamic_integer& pre_assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); assign_negative(); m_contents->add(*(tmp.m_contents)); return *this; }
	dynamic_integer pre_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; clear(); return tmp2; } dynamic_integer tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_negative(); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c.negate(); c.add(src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_negative(); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_negative(); *this += src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c.negate(); c.add(*(src.begin_read())); }); }
	dynamic_integer post_assign_inverse_subtract(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) clear(); else { assign_negative(); m_contents->add(*(src.m_contents)); } return tmp; }
	dynamic_integer post_assign_inverse_subtract(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c.negate(); c.add(*(src.m_contents)); }); }
	dynamic_integer post_assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); dynamic_integer tmp2(*this); assign_negative(); m_contents->add(*(tmp.m_contents)); return tmp2; }
	dynamic_integer post_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; return exchange(tmp2); } dynamic_integer tmp(src); return post_assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_subtract(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_subtract(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_subtract(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_subtract(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_subtract(tmp); }

	// multiply
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->multiply(*m_contents, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->multiply(*rt, src); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*(tmp); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*(tmp); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->multiply(*m_contents, *(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->multiply(*rt, *(src.m_contents)); rt->release(); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result; result.m_contents->multiply(*m_contents, *(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->multiply(*rt, *(src.begin_read())); rt->release(); return result; }
	auto operator*(const dynamic_integer& src) const { dynamic_integer result; result.m_contents->multiply(*m_contents, *(src.m_contents)); return result; }
	auto operator*(const dynamic_integer& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); result.m_contents->multiply(*rt, *(src.m_contents)); rt->release(); return result; }
	auto operator*(const volatile dynamic_integer& src) const { dynamic_integer result; read_token rt = src.guarded_begin_read(); result.m_contents->multiply(*m_contents, *rt); rt->release(); return result; }
	auto operator*(const volatile dynamic_integer& src) const volatile { dynamic_integer result; read_token rt = guarded_begin_read(); read_token rt2 = src.guarded_begin_read(); result.m_contents->multiply(*rt, *rt2); rt->release(); rt2->release(); return result; }

	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const fixed_integer_native_const<has_sign2, bits2, -1>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const volatile fixed_integer_native_const<has_sign2, bits2, -1>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const fixed_integer_native_const<has_sign2, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2> dynamic_integer operator*(const volatile fixed_integer_native_const<has_sign2, bits2, -1>&) const volatile { return operator-(); }
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

	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->multiply(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.multiply(src); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.multiply(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->multiply(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.multiply(*(src.begin_read())); }); return *this; }
	dynamic_integer& operator*=(const dynamic_integer& src) { m_contents->multiply((this == &src) ? *(dynamic_integer(src).m_contents) : *(src.m_contents)); return *this; }
	volatile dynamic_integer& operator*=(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { c.multiply(*(src.m_contents)); }); return *this; }
	dynamic_integer& operator*=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator*=(tmp); }
	volatile dynamic_integer& operator*=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator*=(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, -1>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator*=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator*=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator*=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator*=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator*=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator*=(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { *this *= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(src); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { *this *= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this *= src; return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.begin_read())); }); }
	const dynamic_integer& pre_assign_multiply(const dynamic_integer& src) { *this *= src; return *this; }
	dynamic_integer pre_assign_multiply(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	const dynamic_integer& pre_assign_multiply(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }
	dynamic_integer pre_assign_multiply(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, -1>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_multiply(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_multiply(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_multiply(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_multiply(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this *= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this *= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); *this *= src; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.begin_read())); }); }
	dynamic_integer post_assign_multiply(const dynamic_integer& src) { dynamic_integer tmp(*this); *this *= src; return tmp; }
	dynamic_integer post_assign_multiply(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c.multiply(*(src.m_contents)); }); }
	dynamic_integer post_assign_multiply(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_multiply(tmp); }
	dynamic_integer post_assign_multiply(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, -1>& src) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, -1>& src) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_multiply(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_multiply(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_multiply(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_multiply(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_multiply(tmp); }

	// modulo
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(src); return result;  }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(src); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%(tmp); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%(tmp); }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return result; }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return result; }
	auto operator%(const dynamic_integer& src) const { if (this == &src) { dynamic_integer tmp; return tmp; } dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return result; }
	auto operator%(const dynamic_integer& src) const volatile { dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return result; }
	auto operator%(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); dynamic_integer result(*this); result.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents)); return result; ; }
	auto operator%(const volatile dynamic_integer& src) const volatile { if (this == &src) { dynamic_integer tmp; return tmp; } dynamic_integer tmp(src); return operator%(tmp); }

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

	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const fixed_integer_native<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); return *this; }
	dynamic_integer& operator%=(const dynamic_integer& src) { if (this == &src) clear(); else m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	volatile dynamic_integer& operator%=(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
	dynamic_integer& operator%=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents)); return *this; }
	volatile dynamic_integer& operator%=(const volatile dynamic_integer& src) volatile { if (this == &src) clear(); else { dynamic_integer tmp(src); operator%=(tmp); } return *this; }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); return *this; }
	template <size_t bits2> dynamic_integer& operator%=(const fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> dynamic_integer& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> volatile dynamic_integer& operator%=(const fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return *this; }
	template <size_t bits2> volatile dynamic_integer& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return operator%=(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator%=(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator%=(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer& operator%=(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > volatile dynamic_integer& operator%=(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return operator%=(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }
	const dynamic_integer& pre_assign_modulo(const dynamic_integer& src) { operator%=(src); return *this; }
	dynamic_integer pre_assign_modulo(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	const dynamic_integer& pre_assign_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents)); return *this; }
	dynamic_integer pre_assign_modulo(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; clear(); return tmp2; } else { dynamic_integer tmp(src); return pre_assign_modulo(tmp); } }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <size_t bits2> const dynamic_integer& pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) { clear(); return *this; }
	template <size_t bits2> dynamic_integer pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <size_t bits2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_modulo(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->divide_whole_and_assign_modulo(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.begin_read())); }); }
	dynamic_integer post_assign_modulo(const dynamic_integer& src) { dynamic_integer tmp(*this); operator%=(src); return tmp; }
	dynamic_integer post_assign_modulo(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
	dynamic_integer post_assign_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp2(*this); dynamic_integer tmp(src); m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents)); return tmp2; }
	dynamic_integer post_assign_modulo(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; return exchange(tmp2); } else { dynamic_integer tmp(src); return post_assign_modulo(tmp); } }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<true, bits2, -1>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <size_t bits2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>& src) volatile { dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_modulo(tmp); }

	// inverse_modulo
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const volatile { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const volatile { return src % *this; }

	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src % c; }); }

	void assign_inverse_modulo(const dynamic_integer& src) { *this = src % *this; }

	void assign_inverse_modulo(const dynamic_integer& src) volatile 
	{
		content_t tmp = *(src.m_contents);
		tmp.acquire();
		guarded_write_retry_loop([&](content_t& c)
		{
			tmp.divide_whole_and_assign_modulo(c);
			c = std::move(tmp);
		}, [&](){
			tmp = *(src.m_contents);
			tmp.acquire();
		});
	}

	void assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); *this = src % *this; }
	void assign_inverse_modulo(const volatile dynamic_integer& src) volatile { if (this == &src) clear(); else { dynamic_integer tmp(src); assign_inverse_modulo(tmp); } }
	template <bool has_sign2, size_t bits2> auto assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> auto assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> auto assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); }
	template <bool has_sign2, size_t bits2> auto assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > void assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2>  dynamic_integer pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src % c; }); }

	const dynamic_integer& pre_assign_inverse_modulo(const dynamic_integer& src) { assign_inverse_modulo(src); return *this; }

	dynamic_integer pre_assign_inverse_modulo(const dynamic_integer& src) volatile
	{
		content_t tmp = *(src.m_contents);
		tmp.acquire();
		return guarded_write_retry_loop_pre([&](content_t& c)
		{
			tmp.divide_whole_and_assign_modulo(c);
			c = std::move(tmp);
		}, [&]() {
			tmp = *(src.m_contents);
			tmp.acquire();
		});
	}

	const dynamic_integer& pre_assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); *this = tmp % *this; return *this; }
	dynamic_integer pre_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; clear(); return tmp2; } else { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); } }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); return tmp1; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); return tmp1; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > const dynamic_integer& pre_assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer pre_assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_modulo(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_modulo(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src % c; }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_modulo(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src % c; }); }

	dynamic_integer post_assign_inverse_modulo(const dynamic_integer& src) { dynamic_integer tmp(*this); assign_inverse_modulo(src); return tmp; }

	dynamic_integer post_assign_inverse_modulo(const dynamic_integer& src) volatile
	{
		content_t tmp = *(src.m_contents);
		tmp.acquire();
		return guarded_write_retry_loop_post([&](content_t& c)
		{
			tmp.divide_whole_and_assign_modulo(c);
			c = std::move(tmp);
		}, [&]() {
			tmp = *(src.m_contents);
			tmp.acquire();
		});
	}	

	dynamic_integer post_assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp2(*this); dynamic_integer tmp(src); *this = tmp % *this; return tmp2; }
	dynamic_integer post_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2; return exchange(tmp2); } else { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); } }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) { dynamic_integer tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); return tmp2; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>& src) volatile { dynamic_integer tmp1; auto tmp2 = exchange(tmp1); COGS_ASSERT(!!tmp2); return tmp2; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_modulo(const int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_modulo(const int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_modulo(const volatile int_t2& src) { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > > dynamic_integer post_assign_inverse_modulo(const volatile int_t2& src) volatile { int_to_fixed_integer_t<int_t2> tmp(src); return post_assign_inverse_modulo(tmp); }

	// divide
	template <bool has_sign2, size_t bits2> auto operator/(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<dynamic_integer, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<dynamic_integer, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<dynamic_integer, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<dynamic_integer, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> auto operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	auto operator/(const dynamic_integer& src) const { return fraction<dynamic_integer, dynamic_integer>(*this, src); }
	auto operator/(const dynamic_integer& src) const volatile { return fraction<dynamic_integer, dynamic_integer>(*this, src); }
	auto operator/(const volatile dynamic_integer& src) const { return fraction<dynamic_integer, dynamic_integer>(*this, src); }
	auto operator/(const volatile dynamic_integer& src) const volatile { return fraction<dynamic_integer, dynamic_integer>(*this, src); }

	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<dynamic_integer, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<dynamic_integer, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<dynamic_integer, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<dynamic_integer, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<dynamic_integer, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<dynamic_integer, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<dynamic_integer, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const { return fraction<dynamic_integer, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const volatile { return fraction<dynamic_integer, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const { return fraction<dynamic_integer, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const volatile { return fraction<dynamic_integer, int_t2>(*this, i); }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const fixed_integer_native<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const fixed_integer_native<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(src); return *this; }
	dynamic_integer& operator/=(const dynamic_integer& src) { assign_divide_whole(src); return *this; }
	dynamic_integer& operator/=(const volatile dynamic_integer& src) { assign_divide_whole(src); return *this; }
	volatile dynamic_integer& operator/=(const dynamic_integer& src) volatile { assign_divide_whole(src); return *this; }
	volatile dynamic_integer& operator/=(const volatile dynamic_integer& src) volatile { assign_divide_whole(src); return *this; }

	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> dynamic_integer& operator/=(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> dynamic_integer& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> volatile dynamic_integer& operator/=(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <size_t bits2> volatile dynamic_integer& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile dynamic_integer& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile dynamic_integer& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer& operator/=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile dynamic_integer& operator/=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer& operator/=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile dynamic_integer& operator/=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(src); }
	const dynamic_integer& pre_assign_divide(const dynamic_integer& src) { return pre_assign_divide_whole(src); }
	const dynamic_integer& pre_assign_divide(const volatile dynamic_integer& src) { return pre_assign_divide_whole(src); }
	dynamic_integer pre_assign_divide(const dynamic_integer& src) volatile { return pre_assign_divide_whole(src); }
	dynamic_integer pre_assign_divide(const volatile dynamic_integer& src) volatile { return pre_assign_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> dynamic_integer pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> dynamic_integer pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(src); }
	dynamic_integer post_assign_divide(const dynamic_integer& src) { return post_assign_divide_whole(src); }
	dynamic_integer post_assign_divide(const volatile dynamic_integer& src) { return post_assign_divide_whole(src); }
	dynamic_integer post_assign_divide(const dynamic_integer& src) volatile { return post_assign_divide_whole(src); }
	dynamic_integer post_assign_divide(const volatile dynamic_integer& src) volatile { return post_assign_divide_whole(src); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }

	// reciprocal
	auto reciprocal() const { return fraction<one_t, dynamic_integer>(one_t(), *this); }
	auto reciprocal() const volatile { return fraction<one_t, dynamic_integer>(one_t(), *this); }
	void assign_reciprocal() { m_contents->assign_reciprocal(); }
	void assign_reciprocal() volatile { try_guarded_write_retry_loop([&](content_t& c) { return c.assign_reciprocal(); }); }
	const dynamic_integer& pre_assign_reciprocal() { assign_reciprocal(); return *this; }
	dynamic_integer pre_assign_reciprocal() volatile { return try_guarded_write_retry_loop_pre([&](content_t& c) { return c.assign_reciprocal(); }); }
	dynamic_integer post_assign_reciprocal() { dynamic_integer result(*this); assign_reciprocal(); return result; }
	dynamic_integer post_assign_reciprocal() volatile { return try_guarded_write_retry_loop_post([&](content_t& c) { return c.assign_reciprocal(); }); }

	// inverse_divide
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, dynamic_integer>(src, *this); }
	fraction<dynamic_integer, dynamic_integer> inverse_divide(const dynamic_integer& src) const { return fraction<dynamic_integer, dynamic_integer>(src, *this); }
	fraction<dynamic_integer, dynamic_integer> inverse_divide(const dynamic_integer& src) const volatile { return fraction<dynamic_integer, dynamic_integer>(src, *this); }
	fraction<dynamic_integer, dynamic_integer> inverse_divide(const volatile dynamic_integer& src) const { return fraction<dynamic_integer, dynamic_integer>(src, *this); }
	fraction<dynamic_integer, dynamic_integer> inverse_divide(const volatile dynamic_integer& src) const volatile { return fraction<dynamic_integer, dynamic_integer>(src, *this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, dynamic_integer>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, dynamic_integer>(src, *this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const { return fraction<int_t2, dynamic_integer>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const volatile { return fraction<int_t2, dynamic_integer>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const { return fraction<int_t2, dynamic_integer>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const volatile { return fraction<int_t2, dynamic_integer>(i, *this); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const dynamic_integer& src) { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const volatile dynamic_integer& src) { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const dynamic_integer& src) volatile { assign_inverse_divide_whole(src); }
	void assign_inverse_divide(const volatile dynamic_integer& src) volatile { assign_inverse_divide_whole(src); }

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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_inverse_divide_whole(src); }
	const dynamic_integer& pre_assign_inverse_divide(const dynamic_integer& src) { return pre_assign_inverse_divide_whole(src); }
	const dynamic_integer& pre_assign_inverse_divide(const volatile dynamic_integer& src) { return pre_assign_inverse_divide_whole(src); }
	dynamic_integer pre_assign_inverse_divide(const dynamic_integer& src) volatile { return pre_assign_inverse_divide_whole(src); }
	dynamic_integer pre_assign_inverse_divide(const volatile dynamic_integer& src) volatile { return pre_assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_inverse_divide_whole(src); }
	dynamic_integer post_assign_inverse_divide(const dynamic_integer& src) { return post_assign_inverse_divide_whole(src); }
	dynamic_integer post_assign_inverse_divide(const volatile dynamic_integer& src) { return post_assign_inverse_divide_whole(src); }
	dynamic_integer post_assign_inverse_divide(const dynamic_integer& src) volatile { return post_assign_inverse_divide_whole(src); }
	dynamic_integer post_assign_inverse_divide(const volatile dynamic_integer& src) volatile { return post_assign_inverse_divide_whole(src); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); dynamic_integer tmp; return exchange(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }

	// floor
	const dynamic_integer& floor() const { return *this; }
	dynamic_integer floor() const volatile { return *this; }
	void assign_floor() { }
	void assign_floor() volatile { }
	const dynamic_integer& pre_assign_floor() { return *this; }
	dynamic_integer pre_assign_floor() volatile { return *this; }
	const dynamic_integer& post_assign_floor() { return *this; }
	dynamic_integer post_assign_floor() volatile { return *this; }

	// ceil
	const dynamic_integer& ceil() const { return *this; }
	dynamic_integer ceil() const volatile { return *this; }
	void assign_ceil() { }
	void assign_ceil() volatile { }
	const dynamic_integer& pre_assign_ceil() { return *this; }
	dynamic_integer pre_assign_ceil() volatile { return *this; }
	const dynamic_integer& post_assign_ceil() { return *this; }
	dynamic_integer post_assign_ceil() volatile { return *this; }

	// fractional_part
	auto fractional_part() const { return zero_t(); }
	auto fractional_part() const volatile { return zero_t(); }
	void assign_fractional_part() { clear(); }
	void assign_fractional_part() volatile { clear(); }
	dynamic_integer pre_assign_fractional_part() { clear(); dynamic_integer tmp; return tmp; }
	dynamic_integer pre_assign_fractional_part() volatile { clear(); dynamic_integer tmp; return tmp; }
	dynamic_integer post_assign_fractional_part() { dynamic_integer tmp(*this); clear(); return tmp; }
	dynamic_integer post_assign_fractional_part() volatile { dynamic_integer tmp; return exchange(tmp); }

	// divide_whole
	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *(result.m_contents));
		return result;
	}

	auto divide_whole(const dynamic_integer& src) const
	{
		dynamic_integer result;
		if (this == &src)
		{
			COGS_ASSERT(!!*this);	// divide by zero
			result = one_t();
		}
		else
		{
			dynamic_integer remainder(*this);
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		}
		return result;
	}

	auto divide_whole(const dynamic_integer& src) const volatile
	{
		dynamic_integer result;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		return result;
	}

	auto divide_whole(const volatile dynamic_integer& src) const
	{
		dynamic_integer result;
		dynamic_integer tmp(src);
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *(result.m_contents));
		return result;
	}

	dynamic_integer divide_whole(const volatile dynamic_integer& src) const volatile
	{
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			one_t one;
			dynamic_integer tmp2(one);
			return tmp2;
		}

		dynamic_integer tmp(src); 
		return divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
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

	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(src, *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(src, tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *m_contents); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.begin_read()), tmp); c = std::move(tmp); }); }
	void assign_divide_whole(const dynamic_integer& src) { if (this == &src) { COGS_ASSERT(!!*this); *this = one_t(); } else { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *m_contents); } }
	void assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src); dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents); }
	void assign_divide_whole(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	void assign_divide_whole(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp2(exchange(one_t())); COGS_ASSERT(!!tmp2); } else { dynamic_integer tmp(src); assign_divide_whole(tmp); } }

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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(src, tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.begin_read()), tmp); c = std::move(tmp); }); }
	const dynamic_integer& pre_assign_divide_whole(const dynamic_integer& src) { if (this == &src) { COGS_ASSERT(!!*this); *this = one_t(); } else { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *m_contents); } return *this; }
	const dynamic_integer& pre_assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src);  dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents); return *this; }
	dynamic_integer pre_assign_divide_whole(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	dynamic_integer pre_assign_divide_whole(const volatile dynamic_integer& src) volatile { if (this == &src) { one_t constOne; dynamic_integer one = constOne; dynamic_integer tmp2(exchange(one)); COGS_ASSERT(!!tmp2); return one; } else { dynamic_integer tmp(src); return pre_assign_divide_whole(tmp); } }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> dynamic_integer pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(src, tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.begin_read()), tmp); c = std::move(tmp); }); }
	dynamic_integer post_assign_divide_whole(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) { COGS_ASSERT(!!*this); *this = one_t(); } else { dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *m_contents); } return tmp; }
	dynamic_integer post_assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp2(*this); dynamic_integer tmp(src); dynamic_integer remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents); return tmp2; }
	dynamic_integer post_assign_divide_whole(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp; c.divide_whole_and_assign_modulo(*(src.m_contents), tmp); c = std::move(tmp); }); }
	dynamic_integer post_assign_divide_whole(const volatile dynamic_integer& src) volatile { if (this == &src) { dynamic_integer tmp(exchange(one_t())); COGS_ASSERT(!!tmp); return tmp; } else { dynamic_integer tmp(src); return post_assign_divide_whole(tmp); } }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }

	// inverse_divide_whole
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const volatile { return src.divide_whole(*this); }

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
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }

	void assign_inverse_divide_whole(const dynamic_integer& src)
	{
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			*this = one_t();
		}
		else
		{
			dynamic_integer remainder(src);
			dynamic_integer tmp(*this);
			remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents);
		}
	}

	void assign_inverse_divide_whole(const volatile dynamic_integer& src)
	{
		dynamic_integer tmp(*this);
		dynamic_integer remainder(src);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents);
	}

	void assign_inverse_divide_whole(const dynamic_integer& src) volatile
	{
		dynamic_integer remainder = src;
		guarded_write_retry_loop([&](content_t& c)
		{
			content_t tmp;
			remainder.m_contents->divide_whole_and_assign_modulo(c, tmp);
			c = std::move(tmp);
		}, [&]() {
			remainder = src;
		});
	}

	void assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile
	{
		if (this == &src)
		{
			dynamic_integer tmp(exchange(one_t()));
			COGS_ASSERT(!!tmp);
		}
		else
		{
			dynamic_integer tmp(src);
			assign_inverse_divide(tmp);
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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }

	const dynamic_integer& pre_assign_inverse_divide_whole(const dynamic_integer& src) { assign_inverse_divide(src); return *this; }
	const dynamic_integer& pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) { assign_inverse_divide(src); return *this; }

	dynamic_integer pre_assign_inverse_divide_whole(const dynamic_integer& src) volatile
	{
		dynamic_integer remainder = src;
		return guarded_write_retry_loop_pre([&](content_t& c)
		{
			content_t tmp;
			remainder.m_contents->divide_whole_and_assign_modulo(c, tmp);
			c = std::move(tmp);
		}, [&]() {
			remainder = src;
		});
	}

	dynamic_integer pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile
	{
		if (this == &src)
		{
			one_t constOne;
			dynamic_integer one(constOne);
			dynamic_integer tmp(exchange(one));
			COGS_ASSERT(!!tmp);
			return one;
		}
		else
		{
			dynamic_integer tmp(src);
			return pre_assign_inverse_divide(tmp);
		}
	}

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }

	dynamic_integer post_assign_inverse_divide_whole(const dynamic_integer& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }
	dynamic_integer post_assign_inverse_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_inverse_divide(src); return tmp; }

	dynamic_integer post_assign_inverse_divide_whole(const dynamic_integer& src) volatile
	{
		dynamic_integer remainder = src;
		return guarded_write_retry_loop_post([&](content_t& c)
		{
			content_t tmp;
			remainder.m_contents->divide_whole_and_assign_modulo(c, tmp);
			c = std::move(tmp);
		}, [&]() {
			remainder = src;
		});
	}

	dynamic_integer post_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile
	{
		if (this == &src)
		{
			dynamic_integer tmp(exchange(one_t()));
			COGS_ASSERT(!!tmp);
			return tmp;
		}
		else
		{
			dynamic_integer tmp(src);
			return post_assign_inverse_divide(tmp);
		}
	}

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { dynamic_integer tmp; dynamic_integer result = exchange(tmp); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { dynamic_integer tmp; dynamic_integer result = exchange(tmp); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }

	// divide_whole_and_modulo
	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(src, *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	auto divide_whole_and_modulo(const dynamic_integer& src) const
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	auto divide_whole_and_modulo(const dynamic_integer& src) const volatile
	{
		dynamic_integer divided;
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(divided.m_contents));
		return std::make_pair(divided, remainder);
	}

	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return std::pair<dynamic_integer, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return std::pair<dynamic_integer, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return std::pair<dynamic_integer, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return std::pair<dynamic_integer, zero_t>(*this, zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const { return std::pair<dynamic_integer, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return std::pair<dynamic_integer, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return std::pair<dynamic_integer, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return std::pair<dynamic_integer, zero_t>(operator-(), zero_t()); }
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
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }

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
		dynamic_integer result;
		m_contents->divide_whole_and_assign_modulo(src, *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		dynamic_integer result;
		guarded_write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(src, *(result.m_contents));
		});
		return result;
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return divide_whole_and_assign_modulo(tmp); }

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		dynamic_integer result;
		m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		dynamic_integer result;
		guarded_write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		});
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		dynamic_integer result;
		m_contents->divide_whole_and_assign_modulo(*(src.begin_read()), *(result.m_contents));
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		dynamic_integer result;
		guarded_write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.begin_read()), *(result.m_contents));
		});
		return result;
	}

	auto divide_whole_and_assign_modulo(const dynamic_integer& src)
	{
		dynamic_integer result;
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			clear();
			result = one_t();
		}
		else
		{
			m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		}
		return result;
	}

	auto divide_whole_and_assign_modulo(const dynamic_integer& src) volatile
	{
		dynamic_integer result;
		guarded_write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.m_contents), *(result.m_contents));
		});
		return result;
	}

	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src)
	{
		dynamic_integer tmp(src);
		dynamic_integer result;
		m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *(result.m_contents));
		return result;
	}

	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src) volatile
	{
		dynamic_integer result;
		if (this == &src)
		{
			dynamic_integer zero;
			dynamic_integer tmp(exchange(zero));
			COGS_ASSERT(!!tmp);
			result = one_t();
		}
		else
		{
			dynamic_integer tmp(src);
			guarded_write_retry_loop([&](content_t& c)
			{
				c.divide_whole_and_assign_modulo(*(tmp.m_contents), *(result.m_contents));
			});
		}
		return result;
	}

	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) volatile { dynamic_integer tmp(exchange(zero_t())); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { dynamic_integer tmp(exchange(zero_t())); return -tmp; }
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
		m_contents->m_digits.clear_inner();
		remainder.divide_whole_and_assign_modulo(src, *m_contents);
		fixed_integer_native<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		guarded_write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.acquire();
			c.m_digits.clear_inner();
			remainder.divide_whole_and_assign_modulo(src, c);
		}, [&](){
			remainder.release();
		});
		fixed_integer_native<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return modulo_and_assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src)
	{
		content_t remainder(*m_contents);
		m_contents->m_digits.clear_inner();
		remainder.divide_whole_and_assign_modulo(*(src.m_contents), *m_contents);
		fixed_integer_extended<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		guarded_write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.acquire();
			c.m_digits.clear_inner();
			remainder.divide_whole_and_assign_modulo(*(src.m_contents), c);
		}, [&]() {
			remainder.release();
		});
		fixed_integer_extended<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{
		content_t remainder(*m_contents);
		m_contents->m_digits.clear_inner();
		remainder.divide_whole_and_assign_modulo(*(src.begin_read()), *m_contents);
		fixed_integer_extended<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile
	{
		content_t remainder;
		guarded_write_retry_loop([&](content_t& c)
		{
			remainder = c;
			remainder.acquire();
			c.m_digits.clear_inner();
			remainder.divide_whole_and_assign_modulo(*(src.begin_read()), c);
		}, [&]() {
			remainder.release();
		});
		fixed_integer_extended<true, bits2> result(remainder);
		remainder.release();
		return result;
	}

	auto modulo_and_assign_divide_whole(const dynamic_integer& src)
	{
		dynamic_integer remainder;
		if (this == &src)
		{
			COGS_ASSERT(!!*this);
			remainder.clear();
			*this = one_t();
		}
		else
		{
			*(remainder.m_contents) = *(m_contents);
			m_contents->m_digits.clear_inner();
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), *m_contents);
		}
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const dynamic_integer& src) volatile
	{
		dynamic_integer remainder;
		guarded_write_retry_loop([&](content_t& c)
		{
			*(remainder.m_contents) = c;
			remainder.m_contents->acquire();
			c.m_digits.clear_inner();
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), c);
		}, [&]() {
			remainder.m_contents->release();
		});
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src)
	{
		dynamic_integer tmp(src);
		dynamic_integer remainder(*this);
		remainder.m_contents->divide_whole_and_assign_modulo(*(tmp.m_contents), *m_contents);
		return remainder;
	}

	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src) volatile
	{
		dynamic_integer remainder;
		if (this == &src)
		{
			dynamic_integer tmp(exchange(one_t()));
			COGS_ASSERT(!!tmp);
			remainder.clear();
		}
		else
		{
			dynamic_integer tmp(src);
			return modulo_and_assign_divide_whole(tmp);
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
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_gcd(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.gcd(src); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_gcd(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.gcd(src); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_gcd(*m_contents, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.gcd(src); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_gcd(*m_contents.begin_read(), *(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.gcd(src); }
	auto gcd(const dynamic_integer& src) const { if (this == &src) return abs(); dynamic_integer tmp; tmp.m_contents->set_to_gcd(*m_contents, *(src.m_contents)); return tmp; }
	auto gcd(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return src.gcd(tmp); }
	auto gcd(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return gcd(tmp); }
	auto gcd(const volatile dynamic_integer& src) const volatile { if (this == &src) return abs(); dynamic_integer tmp(*this); return tmp.gcd(src); }

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

	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_gcd(*(tmp.m_contents), src); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, src); }); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_gcd(*(tmp.m_contents), *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_gcd(*(tmp.m_contents), *(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.begin_read())); }); }
	void assign_gcd(const dynamic_integer& src) { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); m_contents->set_to_gcd(*(tmp.m_contents), *(src.m_contents)); }
	void assign_gcd(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_gcd(tmp); }
	void assign_gcd(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	void assign_gcd(const volatile dynamic_integer& src) volatile { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); assign_gcd(tmp); }

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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.begin_read())); }); }
	const dynamic_integer& pre_assign_gcd(const dynamic_integer& src) { if (this == &src) assign_abs(); else assign_gcd(src); return *this; }
	const dynamic_integer& pre_assign_gcd(const volatile dynamic_integer& src) { assign_gcd(src); return *this; }
	dynamic_integer pre_assign_gcd(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	dynamic_integer pre_assign_gcd(const volatile dynamic_integer& src) volatile { if (this == &src) return pre_assign_abs(); dynamic_integer tmp(src); return pre_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> const dynamic_integer& pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> dynamic_integer pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.begin_read())); }); }
	dynamic_integer post_assign_gcd(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) assign_abs(); else assign_gcd(src); return tmp; }
	dynamic_integer post_assign_gcd(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_gcd(src); return tmp; }
	dynamic_integer post_assign_gcd(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_gcd(tmp, *(src.m_contents)); }); }
	dynamic_integer post_assign_gcd(const volatile dynamic_integer& src) volatile { if (this == &src) return post_assign_abs(); dynamic_integer tmp(src); return post_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }

	// lcm
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lcm(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lcm(src); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lcm(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lcm(src); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lcm(*m_contents, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lcm(src); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lcm(*m_contents, *(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lcm(src); }
	auto lcm(const dynamic_integer& src) const { if (this == &src) return abs(); dynamic_integer tmp; tmp.m_contents->set_to_lcm(*m_contents, *(src.m_contents)); return tmp; }
	auto lcm(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return src.lcm(tmp); }
	auto lcm(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return lcm(tmp); }
	auto lcm(const volatile dynamic_integer& src) const volatile { if (this == &src) return abs(); dynamic_integer tmp(*this); return tmp.lcm(src); }

	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return one_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return one_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return one_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return one_t(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const { return one_t(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return one_t(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return one_t(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return one_t(); }
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

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lcm(*(tmp.m_contents), src); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, src); }); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lcm(*(tmp.m_contents), *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lcm(*(tmp.m_contents), *(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.begin_read())); }); }
	void assign_lcm(const dynamic_integer& src) { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); m_contents->set_to_lcm(*(tmp.m_contents), *(src.m_contents)); }
	void assign_lcm(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_lcm(tmp); }
	void assign_lcm(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	void assign_lcm(const volatile dynamic_integer& src) volatile { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); }
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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.begin_read())); }); }
	const dynamic_integer& pre_assign_lcm(const dynamic_integer& src) { if (this == &src) assign_abs(); else assign_lcm(src); return *this; }
	const dynamic_integer& pre_assign_lcm(const volatile dynamic_integer& src) { assign_lcm(src); return *this; }
	dynamic_integer pre_assign_lcm(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	dynamic_integer pre_assign_lcm(const volatile dynamic_integer& src) volatile { if (this == &src) return pre_assign_abs(); dynamic_integer tmp(src); return pre_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); dynamic_integer tmp; return tmp; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> const dynamic_integer& pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { *this = one_t(); return *this; }
	template <size_t bits2> dynamic_integer pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <size_t bits2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.begin_read())); }); }
	dynamic_integer post_assign_lcm(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) assign_abs(); else assign_lcm(src); return tmp; }
	dynamic_integer post_assign_lcm(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_lcm(src); return tmp; }
	dynamic_integer post_assign_lcm(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lcm(tmp, *(src.m_contents)); }); }
	dynamic_integer post_assign_lcm(const volatile dynamic_integer& src) volatile { if (this == &src) return post_assign_abs(); dynamic_integer tmp(src); return post_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { dynamic_integer tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return exchange(zero_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { dynamic_integer tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <size_t bits2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return exchange(one_t()); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }

	// greater
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_greater(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.greater(src); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_greater(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.greater(src); }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_greater(*m_contents, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.greater(src); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_greater(*m_contents, *(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.greater(src); }
	auto greater(const dynamic_integer& src) const { if (this == &src) return abs(); dynamic_integer tmp; tmp.m_contents->set_to_greater(*m_contents, *(src.m_contents)); return tmp; }
	auto greater(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return src.greater(tmp); }
	auto greater(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return greater(tmp); }
	auto greater(const volatile dynamic_integer& src) const volatile { if (this == &src) return abs(); dynamic_integer tmp(*this); return tmp.greater(src); }

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

	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_greater(*(tmp.m_contents), src); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, src); }); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_greater(*(tmp.m_contents), *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_greater(*(tmp.m_contents), *(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.begin_read())); }); }
	void assign_greater(const dynamic_integer& src) { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); m_contents->set_to_greater(*(tmp.m_contents), *(src.m_contents)); }
	void assign_greater(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_greater(tmp); }
	void assign_greater(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	void assign_greater(const volatile dynamic_integer& src) volatile { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); assign_greater(tmp); }

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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.begin_read())); }); }
	const dynamic_integer& pre_assign_greater(const dynamic_integer& src) { if (this == &src) assign_abs(); else assign_greater(src); return *this; }
	const dynamic_integer& pre_assign_greater(const volatile dynamic_integer& src) { assign_greater(src); return *this; }
	dynamic_integer pre_assign_greater(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	dynamic_integer pre_assign_greater(const volatile dynamic_integer& src) volatile { if (this == &src) return pre_assign_abs(); dynamic_integer tmp(src); return pre_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.begin_read())); }); }
	dynamic_integer post_assign_greater(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) assign_abs(); else assign_greater(src); return tmp; }
	dynamic_integer post_assign_greater(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_greater(src); return tmp; }
	dynamic_integer post_assign_greater(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_greater(tmp, *(src.m_contents)); }); }
	dynamic_integer post_assign_greater(const volatile dynamic_integer& src) volatile { if (this == &src) return post_assign_abs(); dynamic_integer tmp(src); return post_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }


	// lesser
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lesser(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lesser(src); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lesser(*m_contents, src); return tmp; }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lesser(src); }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lesser(*m_contents, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lesser(src); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { dynamic_integer tmp; tmp.m_contents->set_to_lesser(*m_contents, *(src.begin_read())); return tmp; }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { dynamic_integer tmp(*this); return tmp.lesser(src); }
	auto lesser(const dynamic_integer& src) const { if (this == &src) return abs(); dynamic_integer tmp; tmp.m_contents->set_to_lesser(*m_contents, *(src.m_contents)); return tmp; }
	auto lesser(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return src.lesser(tmp); }
	auto lesser(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return lesser(tmp); }
	auto lesser(const volatile dynamic_integer& src) const volatile { if (this == &src) return abs(); dynamic_integer tmp(*this); return tmp.lesser(src); }

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

	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lesser(*(tmp.m_contents), src); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { fixed_integer_native<has_sign2, bits2> tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, src); }); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lesser(*(tmp.m_contents), *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); m_contents->set_to_lesser(*(tmp.m_contents), *(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.begin_read())); }); }
	void assign_lesser(const dynamic_integer& src) { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); m_contents->set_to_lesser(*(tmp.m_contents), *(src.m_contents)); }
	void assign_lesser(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_lesser(tmp); }
	void assign_lesser(const dynamic_integer& src) volatile { guarded_write_retry_loop([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	void assign_lesser(const volatile dynamic_integer& src) volatile { if (this == &src) { assign_abs(); return; } dynamic_integer tmp(*this); assign_lesser(tmp); }

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

	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const dynamic_integer& pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.begin_read())); }); }
	const dynamic_integer& pre_assign_lesser(const dynamic_integer& src) { if (this == &src) assign_abs(); else assign_lesser(src); return *this; }
	const dynamic_integer& pre_assign_lesser(const volatile dynamic_integer& src) { assign_lesser(src); return *this; }
	dynamic_integer pre_assign_lesser(const dynamic_integer& src) volatile { return guarded_write_retry_loop_pre([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	dynamic_integer pre_assign_lesser(const volatile dynamic_integer& src) volatile { if (this == &src) return pre_assign_abs(); dynamic_integer tmp(src); return pre_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const dynamic_integer& pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const dynamic_integer& pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const dynamic_integer& pre_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer pre_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, src); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { dynamic_integer tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	template <bool has_sign2, size_t bits2> dynamic_integer post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.begin_read())); }); }
	dynamic_integer post_assign_lesser(const dynamic_integer& src) { dynamic_integer tmp(*this); if (this == &src) assign_abs(); else assign_lesser(src); return tmp; }
	dynamic_integer post_assign_lesser(const volatile dynamic_integer& src) { dynamic_integer tmp(*this); assign_lesser(src); return tmp; }
	dynamic_integer post_assign_lesser(const dynamic_integer& src) volatile { return guarded_write_retry_loop_post([&](content_t& c) { content_t tmp(c); c.m_digits.clear_inner(); c.set_to_lesser(tmp, *(src.m_contents)); }); }
	dynamic_integer post_assign_lesser(const volatile dynamic_integer& src) volatile { if (this == &src) return post_assign_abs(); dynamic_integer tmp(src); return post_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> dynamic_integer post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> dynamic_integer post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	dynamic_integer post_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }


	// equals
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const { return m_contents->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return begin_read()->equals(src); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const { fixed_integer_native<has_sign2, bits2> tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { fixed_integer_native<has_sign2, bits2> tmp(src); return operator==(tmp); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->equals(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return m_contents->equals(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return begin_read()->equals(*(src.begin_read())); }
	bool operator==(const dynamic_integer& src) const { if (this == &src) return true; return m_contents->equals(*(src.m_contents)); }
	bool operator==(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return tmp.m_contents->equals(*(src.m_contents)); }
	bool operator==(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return m_contents->equals(*(tmp.m_contents)); }
	bool operator==(const volatile dynamic_integer& src) const volatile { if (this == &src) return true; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return tmp1.m_contents->equals(*(tmp2.m_contents)); }

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
	bool operator!=(const dynamic_integer& src) const { if (this == &src) return false; return !m_contents->equals(*(src.m_contents)); }
	bool operator!=(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return !tmp.m_contents->equals(*(src.m_contents)); }
	bool operator!=(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return !m_contents->equals(*(tmp.m_contents)); }
	bool operator!=(const volatile dynamic_integer& src) const volatile { if (this == &src) return false; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return !tmp1.m_contents->equals(*(tmp2.m_contents)); }

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
	bool operator<(const dynamic_integer& src) const { if (this == &src) return false; return m_contents->is_less_than(*(src.m_contents)); }
	bool operator<(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return tmp.m_contents->is_less_than(*(src.m_contents)); }
	bool operator<(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return m_contents->is_less_than(*(tmp.m_contents)); }
	bool operator<(const volatile dynamic_integer& src) const volatile { if (this == &src) return false; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return tmp1.m_contents->is_less_than(*(tmp2.m_contents)); }

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
	bool operator>(const dynamic_integer& src) const { if (this > &src) return false; return m_contents->is_greater_than(*(src.m_contents)); }
	bool operator>(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return tmp.m_contents->is_greater_than(*(src.m_contents)); }
	bool operator>(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return m_contents->is_greater_than(*(tmp.m_contents)); }
	bool operator>(const volatile dynamic_integer& src) const volatile { if (this > &src) return false; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return tmp1.m_contents->is_greater_than(*(tmp2.m_contents)); }

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
	bool operator<=(const dynamic_integer& src) const { if (this > &src) return true; return! m_contents->is_greater_than(*(src.m_contents)); }
	bool operator<=(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return !tmp.m_contents->is_greater_than(*(src.m_contents)); }
	bool operator<=(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return !m_contents->is_greater_than(*(tmp.m_contents)); }
	bool operator<=(const volatile dynamic_integer& src) const volatile { if (this > &src) return true; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return !tmp1.m_contents->is_greater_than(*(tmp2.m_contents)); }

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
	bool operator>=(const dynamic_integer& src) const { if (this == &src) return true; return !m_contents->is_less_than(*(src.m_contents)); }
	bool operator>=(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return !tmp.m_contents->is_less_than(*(src.m_contents)); }
	bool operator>=(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return !m_contents->is_less_than(*(tmp.m_contents)); }
	bool operator>=(const volatile dynamic_integer& src) const volatile { if (this == &src) return true; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return !tmp1.m_contents->is_less_than(*(tmp2.m_contents)); }

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
	int compare(const dynamic_integer& src) const { if (this == &src) return 0; return m_contents->compare(*(src.m_contents)); }
	int compare(const dynamic_integer& src) const volatile { dynamic_integer tmp(*this); return tmp.m_contents->compare(*(src.m_contents)); }
	int compare(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return m_contents->compare(*(tmp.m_contents)); }
	int compare(const volatile dynamic_integer& src) const volatile { if (this == &src) return 0; dynamic_integer tmp1(*this); dynamic_integer tmp2(src); return tmp1.m_contents->compare(*(tmp2.m_contents)); }

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


	// swap
	void swap(dynamic_integer& wth) { if (this != &wth) m_contents.swap(wth.m_contents); }
	void swap(dynamic_integer& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile dynamic_integer& wth) { wth.swap(*this); }

	// exchange
	void exchange(const dynamic_integer& src, dynamic_integer& rtn)
	{
		if (&src == &rtn)
			m_contents.swap(rtn.m_contents);
		else
		{
			rtn = *this;
			*this = src;
		}
	}

	void exchange(const volatile dynamic_integer& src, dynamic_integer& rtn)
	{
		rtn = *this;
		*this = src;
	}

	void exchange(const dynamic_integer& src, volatile dynamic_integer& rtn)
	{
		rtn = *this;
		*this = src;
	}

	void exchange(const volatile dynamic_integer& src, volatile dynamic_integer& rtn)
	{
		if (&src == &rtn)
		{
			dynamic_integer tmpSrc(src);
			rtn = *this;
			*this = tmpSrc;
		}
		else
		{
			rtn = *this;
			*this = src;
		}
	}

	void exchange(const dynamic_integer& src, dynamic_integer& rtn) volatile
	{
		if (&src == &rtn)
			m_contents.swap(rtn.m_contents);
		else
		{
			rtn.clear();
			dynamic_integer tmp(src);
			m_contents.exchange(tmp.m_contents, rtn.m_contents);	// transfers ownership to rtn
			tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
		}
	}

	void exchange(const volatile dynamic_integer& src, dynamic_integer& rtn) volatile
	{
		if (this == &src)
			rtn = *this;
		else
		{
			rtn.clear();
			dynamic_integer tmp(src);
			m_contents.exchange(tmp.m_contents, rtn.m_contents);	// transfers ownership to rtn
			tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
		}
	}

	void exchange(const dynamic_integer& src, volatile dynamic_integer& rtn) volatile
	{
		if (this != &rtn)
		{
			dynamic_integer tmpRtn;
			dynamic_integer tmp(src);
			m_contents.exchange(tmp.m_contents, tmpRtn.m_contents);	// transfers ownership to tmpRtn
			tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
			rtn.swap(tmpRtn);
		}
	}

	void exchange(const volatile dynamic_integer& src, volatile dynamic_integer& rtn) volatile
	{
		if (this != &rtn)
		{
			if (this == &src)
				rtn = *this;
			else
			{
				dynamic_integer tmpRtn;
				dynamic_integer tmp(src);
				m_contents.exchange(tmp.m_contents, tmpRtn.m_contents);	// transfers ownership to tmpRtn
				tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
				rtn.swap(tmpRtn);
			}
		}
	}

	dynamic_integer exchange(const dynamic_integer& src)
	{
		dynamic_integer rtn(*this);
		*this = src;
		return rtn;
	}

	dynamic_integer exchange(const dynamic_integer& src) volatile
	{
		dynamic_integer rtn;
		dynamic_integer tmp(src);
		m_contents.exchange(tmp.m_contents, rtn.m_contents);	// transfers ownership to rtn
		tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
		return rtn;
	}

	dynamic_integer exchange(const volatile dynamic_integer& src)
	{
		dynamic_integer rtn(*this);
		*this = src;
		return rtn;
	}

	dynamic_integer exchange(const volatile dynamic_integer& src) volatile
	{
		dynamic_integer rtn;
		if (this == &src)
			rtn = *this;
		else
		{
			dynamic_integer tmp(src);
			m_contents.exchange(tmp.m_contents, rtn.m_contents);	// transfers ownership to rtn
			tmp.m_contents->m_digits.clear_inner();						// transfers ownership to this
		}
		return rtn;
	}



	template <typename S, typename R>
	void exchange(S&& src, R& rtn)
	{
		dynamic_integer tmp;
		cogs::assign(tmp, std::forward<S>(src));
		cogs::assign(rtn, *this);
		*this = tmp;
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn) volatile
	{
		dynamic_integer tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		dynamic_integer tmpRtn;
		exchange(tmpSrc, tmpRtn);
		cogs::assign(rtn, tmpRtn);
	}

	template <typename S>
	dynamic_integer exchange(S&& src)
	{
		dynamic_integer tmp;
		cogs::assign(tmp, std::forward<S>(src));
		dynamic_integer rtn = *this;
		*this = tmp;
		return rtn;
	}

	template <typename S>
	dynamic_integer exchange(S&& src) volatile
	{
		dynamic_integer tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		dynamic_integer rtn;
		exchange(tmpSrc, rtn);
		return rtn;
	}


	// compare_exchange
	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp, dynamic_integer& rtn)
	{
		COGS_ASSERT(this != &rtn);
		if (*this != cmp)
		{
			rtn = *this;
			return false;
		}

		if (&rtn == &src)
		{
			m_contents.swap(rtn.m_contents);
			return true;
		}

		rtn = *this;
		*this = src;
		return true;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp, dynamic_integer& rtn)
	{
		COGS_ASSERT(this != &rtn);
		if (*this != cmp)
		{
			rtn = *this;
			return false;
		}

		rtn = *this;
		*this = src;
		return true;
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp, dynamic_integer& rtn)
	{
		dynamic_integer tmpCmp(cmp);
		return compare_exchange(src, tmpCmp, rtn);
	}

	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp, volatile dynamic_integer& rtn)
	{
		bool b = *this == cmp;
		rtn = *this;
		if (b)
			*this = src;
		return b;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp, dynamic_integer& rtn)
	{
		if (&src == &cmp)
		{
			bool b = *this == cmp;
			rtn = *this;
			return b;
		}

		dynamic_integer tmpCmp(cmp);
		return compare_exchange(src, cmp, rtn);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp, volatile dynamic_integer& rtn)
	{
		if (*this != cmp)
		{
			rtn = *this;
			return false;
		}

		if (&rtn == &src)
		{
			m_contents.swap(rtn.m_contents);
			return true;
		}
		
		rtn = *this;
		*this = src;
		return true;
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp, volatile dynamic_integer& rtn)
	{
		dynamic_integer tmpCmp(cmp);
		return compare_exchange(src, tmpCmp, rtn);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp, volatile dynamic_integer& rtn)
	{
		dynamic_integer tmpCmp(cmp);
		return compare_exchange(src, tmpCmp, rtn);
	}

	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp, dynamic_integer& rtn) volatile
	{
		if (&src == &cmp)	// if compared equal, no need to assign.
		{
			if (&rtn == &cmp)
			{
				dynamic_integer tmp(*this);
				bool b = tmp == cmp;
				rtn = tmp;
				return b;
			}

			rtn = *this;
			return rtn == cmp;
		}

		return compare_exchange2(src, cmp, rtn);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp, dynamic_integer& rtn) volatile
	{
		if (this == &src)
		{
			if (&rtn == &cmp)
			{
				dynamic_integer tmp(*this);
				bool b = tmp == cmp;
				rtn = tmp;
				return b;
			}

			rtn = *this;
			return rtn == cmp;
		}

		return compare_exchange2(src, cmp, rtn);
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp, dynamic_integer& rtn) volatile
	{
		if (this == &cmp)
		{
			exchange(src, rtn);
			return true;
		}

		dynamic_integer tmpCmp(cmp);
		return compare_exchange2(src, tmpCmp, rtn);
	}

	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp, volatile dynamic_integer& rtn) volatile
	{
		COGS_ASSERT(this != &rtn);
		dynamic_integer tmpRtn;
		bool b = compare_exchange(src, cmp, tmpRtn);
		rtn = tmpRtn;
		return b;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp, dynamic_integer& rtn) volatile
	{
		if (this == &cmp)
		{
			exchange(src, rtn);
			return true;
		}

		if ((&src == &cmp) || (this == &src))
		{
			rtn = *this;
			return rtn == cmp;
		}

		dynamic_integer tmpCmp(cmp);
		return compare_exchange2(src, tmpCmp, rtn);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp, volatile dynamic_integer& rtn) volatile
	{
		COGS_ASSERT(this != &rtn);
		if (this == &src)
		{
			dynamic_integer tmp = *this;
			rtn = tmp;
			return tmp == cmp;
		}

		dynamic_integer tmpRtn;
		bool b = compare_exchange2(src, cmp, tmpRtn);
		rtn = tmpRtn;
		return b;
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp, volatile dynamic_integer& rtn) volatile
	{
		COGS_ASSERT(this != &rtn);
		if (this == &cmp)
		{
			exchange(src, rtn);
			return true;
		}

		dynamic_integer tmpRtn;
		dynamic_integer tmpCmp(cmp);
		bool b = compare_exchange2(src, tmpCmp, tmpRtn);
		rtn = tmpRtn;
		return b;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp, volatile dynamic_integer& rtn) volatile
	{
		COGS_ASSERT(this != &rtn);
		if (this == &cmp)
		{
			exchange(src, rtn);
			return true;
		}

		if ((&src == &cmp) || (this == &src))
		{
			dynamic_integer tmp(*this);
			rtn = tmp;
			return tmp == cmp;
		}

		dynamic_integer tmpRtn;
		dynamic_integer tmpCmp(cmp);
		bool b = compare_exchange2(src, tmpCmp, tmpRtn);
		rtn = tmpRtn;
		return b;
	}


	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp)
	{
		if ((this == &src) || (&src == &cmp))
			return *this == cmp;

		if (*this == cmp)
		{
			*this = src;
			return true;
		}

		return false;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp)
	{
		if (*this == cmp)
		{
			*this = src;
			return true;
		}

		return false;
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp)
	{
		if (this == &src)
			return *this == cmp;

		if (*this == cmp)
		{
			*this = src;
			return true;
		}

		return false;
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp)
	{
		if (&src == &cmp)
			return *this == cmp;

		if (*this == cmp)
		{
			*this = src;
			return true;
		}

		return false;
	}


	bool compare_exchange(const dynamic_integer& src, const dynamic_integer& cmp) volatile
	{
		if (&src == &cmp)
			return *this == cmp;

		return compare_exchange2(src, cmp);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const dynamic_integer& cmp) volatile
	{
		if (this == &src)
			return *this == cmp;

		return compare_exchange2(src, cmp);
	}

	bool compare_exchange(const dynamic_integer& src, const volatile dynamic_integer& cmp) volatile
	{
		if (this == &cmp)
		{
			*this = src;
			return true;
		}

		dynamic_integer tmpCmp(cmp);
		return compare_exchange2(src, tmpCmp);
	}

	bool compare_exchange(const volatile dynamic_integer& src, const volatile dynamic_integer& cmp) volatile
	{
		if ((this == &src) || (&src == &cmp))
			return *this == cmp;

		if (this == &cmp)
		{
			*this = src;
			return true;
		}

		dynamic_integer tmpCmp(cmp);
		return compare_exchange2(src, tmpCmp);
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn)
	{
		dynamic_integer tmp(*this);
		bool b = cogs::equals(tmp, std::forward<C>(cmp));
		if (b)
			cogs::assign(*this, std::forward<S>(src));
		cogs::assign(rtn, tmp);
		return b;
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn) volatile
	{
		dynamic_integer tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		dynamic_integer tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		dynamic_integer tmpRtn;
		bool b = compare_exchange(tmpSrc, tmpCmp, tmpRtn);
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
		dynamic_integer tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		dynamic_integer tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		return compare_exchange(tmpSrc, tmpCmp);
	}

	
	 
	// to_string
	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const
	{
		return m_contents->to_string_t<char_t>(radix, minDigits);
	}

	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const volatile
	{
		read_token rt = guarded_begin_read();	// acquires
		dynamic_integer tmp(rt->m_digits, rt->m_isNegative);
		return tmp.to_string_t<char_t>(radix, minDigits);
	}

	string to_string(unsigned int radix = 10, size_t minDigits = 1) const
	{
		return to_string_t<wchar_t>(radix, minDigits);
	}

	string to_string(unsigned int radix = 10, size_t minDigits = 1) const volatile
	{
		return to_string_t<wchar_t>(radix, minDigits);
	}

	cstring to_cstring(unsigned int radix = 10, size_t minDigits = 1) const
	{
		return to_string_t<char>(radix, minDigits);
	}

	cstring to_cstring(unsigned int radix = 10, size_t minDigits = 1) const volatile
	{
		return to_string_t<char>(radix, minDigits);
	}
};


template <bool has_sign2, size_t bits2, ulongest... values2>
class compatible<dynamic_integer, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
public:
	typedef dynamic_integer type;
};

template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<dynamic_integer, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	typedef dynamic_integer type;
};

template <bool has_sign2, size_t bits2>
class compatible<dynamic_integer, fixed_integer_native<has_sign2, bits2> >
{
public:
	typedef dynamic_integer type;
};

template <bool has_sign2, size_t bits2>
class compatible<dynamic_integer, fixed_integer_extended<has_sign2, bits2> >
{
public:
	typedef dynamic_integer type;
};

template <typename int_t2>
class compatible<dynamic_integer, int_t2, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef dynamic_integer type;
};

template <typename int_t2>
class compatible<int_t2, dynamic_integer, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef dynamic_integer type;
};




// interop for fixed_integer_native





template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole(const dynamic_integer_content& src) const
{
	fixed_integer<true, bits + 1> result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)		// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)			// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt != 0)	// 00 = -256 
					{
						if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						{
							result = cogs::divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						result = cogs::negative(cogs::divide_whole(m_int, (ulongest)-srcInt)); // unsigned/unsigned divide, wont grow
						break;
					}
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::divide_whole(m_int, src.get_int());
				break;
			}
		}

		result.clear();
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const dynamic_integer_content& src)
{
	*this = divide_whole(src);
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const dynamic_integer_content& src) volatile
{
	if (src.is_negative())
	{
		if (src.get_length() == 1)		// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
		{
			longest srcInt = (longest)src.get_int();
			if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
			{
				if (srcInt < 0)			// Therefor, if src is negative (80..FF), it's within range of this. 
				{
					cogs::assign_divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
					return;
				}
			}
			else	// this range will be 0..255 == 00..FF
			{
				if (srcInt != 0)	// 00 = -256 
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						cogs::assign_divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						return;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t)
					{
						return cogs::negative(divide_whole(t, srcInt2));
					});
					return;
				}
			}
		}
	}
	else
	{
		if (src.get_length() <= 1)
		{
			cogs::assign_divide_whole(m_int, src.get_int());
			return;
		}
	}

	clear();
}


template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const dynamic_integer_content& src)
{
	*this = divide_whole(src);
	return *this;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const dynamic_integer_content& src) volatile
{
	if (src.is_negative())
	{
		if (src.get_length() == 1)		// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
		{
			longest srcInt = (longest)src.get_int();
			if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
			{
				if (srcInt < 0)			// Therefor, if src is negative (80..FF), it's within range of this. 
					return cogs::pre_assign_divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
			}
			else	// this range will be 0..255 == 00..FF
			{
				if (srcInt != 0)	// 00 = -256 
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						return cogs::pre_assign_divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t)
					{
						return cogs::negative(divide_whole(t, srcInt2));
					});
				}
			}
		}
	}
	else
	{
		if (src.get_length() <= 1)
			return cogs::pre_assign_divide_whole(m_int, src.get_int());
	}

	cogs::clear(m_int);
	return 0;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const dynamic_integer_content& src)
{
	this_t tmp(*this);
	assign_divide_whole(src);
	return tmp;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const dynamic_integer_content& src) volatile
{
	if (src.is_negative())
	{
		if (src.get_length() == 1)		// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
		{
			longest srcInt = (longest)src.get_int();
			if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
			{
				if (srcInt < 0)			// Therefor, if src is negative (80..FF), it's within range of this. 
					return cogs::post_assign_divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
			}
			else	// this range will be 0..255 == 00..FF
			{
				if (srcInt != 0)	// 00 = -256 
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						return cogs::post_assign_divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
					{
						return cogs::negative(divide_whole(t, srcInt2));
					});
				}
			}
		}
	}
	else
	{
		if (src.get_length() <= 1)
			return cogs::post_assign_divide_whole(m_int, src.get_int());
	}

	return cogs::exchange(m_int, 0);
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator%(const dynamic_integer_content& src) const
{
	this_t result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)	// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::modulo(m_int, srcInt);
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt != 0)	// 00 = -256 
					{
						if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						{
							result = cogs::modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						result = cogs::modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
						break;
					}
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::modulo(m_int, src.get_int());
				break;
			}
		}

		result = m_int;
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const dynamic_integer_content& src)
{
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						cogs::assign_modulo(m_int, srcInt);
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt != 0)	// 00 = -256 
					{
						if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						{
							cogs::assign_modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
						break;
					}
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				cogs::assign_modulo(m_int, src.get_int());
				break;
			}
		}

		break;
	}

	return *this;
}

template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const dynamic_integer_content& src) volatile
{
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)	// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						cogs::assign_modulo(m_int, srcInt);
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt != 0)	// 00 = -256 
					{
						if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						{
							cogs::assign_modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
						break;
					}
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				cogs::assign_modulo(m_int, src.get_int());
				break;
			}
		}

		break;
	}

	return *this;
}

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const dynamic_integer_content& src)
{
	operator%=(src);
	return *this;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const dynamic_integer_content& src) volatile
{
	if (src.is_negative())
	{
		if (src.get_length() == 1)	// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
		{
			longest srcInt = (longest)src.get_int();
			if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
			{
				if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					return cogs::pre_assign_modulo(m_int, srcInt);
			}
			else	// this range will be 0..255 == 00..FF
			{
				if (srcInt != 0)	// 00 = -256 
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						return cogs::pre_assign_modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					return pre_assign_modulo(m_int, (ulongest)-srcInt);	// src sign doesn't affect the sign of the result
				}
			}
		}
	}
	else
	{
		if (src.get_length() <= 1)
			return cogs::pre_assign_modulo(m_int, src.get_int());
	}

	return *this;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const dynamic_integer_content& src)
{
	this_t tmp(*this);
	operator%=(src);
	return tmp;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const dynamic_integer_content& src) volatile
{
	if (src.is_negative())
	{
		if (src.get_length() == 1)	// src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
		{
			longest srcInt = (longest)src.get_int();
			if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
			{
				if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					return cogs::post_assign_modulo(m_int, srcInt);
			}
			else	// this range will be 0..255 == 00..FF
			{
				if (srcInt != 0)	// 00 = -256 
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
						return cogs::post_assign_modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					return post_assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
				}
			}
		}
	}
	else
	{
		if (src.get_length() <= 1)
			return cogs::post_assign_modulo(m_int, src.get_int());
	}

	return *this;
}

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_modulo(const dynamic_integer_content& src) const
{
	typedef fixed_integer<true, bits + 1> divide_t;
	std::pair<divide_t, this_t> result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result.first = cogs::divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
						result.second = cogs::modulo(m_int, srcInt);
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						result.first = cogs::divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						result.second = cogs::modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
						break;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					result.first = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
					result.second = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result.first = cogs::divide_whole(m_int, src.get_int());
				result.second = cogs::modulo(m_int, src.get_int());
				break;
			}
		}

		result.second = m_int;
		result.first.clear();
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const dynamic_integer_content& src)
{
	fixed_integer<true, bits + 1> result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
						cogs::assign_modulo(m_int, srcInt);
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						result = cogs::divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						cogs::assign_modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
						break;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					result = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
					cogs::assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::divide_whole(m_int, src.get_int());
				cogs::assign_modulo(m_int, src.get_int());
				break;
			}
		}

		result.clear();
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const dynamic_integer_content& src) volatile
{
	fixed_integer<true, bits + 1> result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
						break;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					int_t tmp = cogs::post_assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
					result = cogs::negative(cogs::divide_whole(tmp, srcInt2)); // unsigned/unsigned divide, wont grow
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::divide_whole(cogs::post_assign_modulo(m_int, src.get_int()), src.get_int());
				break;
			}
		}

		result.clear();
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const dynamic_integer_content& src)
{
	this_t result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::modulo(m_int, srcInt);
						cogs::assign_divide_whole(m_int, srcInt);	// signed/signed divide, so may grow, i.e.: -128/-1=128
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						result.second = cogs::modulo(m_int, srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
						cogs::assign_divide_whole(m_int, srcInt);	// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						break;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					result = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
					*this = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::modulo(m_int, src.get_int());
				cogs::assign_divide_whole(m_int, src.get_int());
				break;
			}
		}

		result = m_int;
		cogs::clear(m_int);
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const dynamic_integer_content& src) volatile
{
	this_t result;
	for (;;)
	{
		if (src.is_negative())
		{
			if (src.get_length() == 1)
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign)				// this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0)		// Therefor, if src is negative (80..FF), it's within range of this. 
					{
						result = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt);
						// signed/signed divide, so may grow, i.e.: -128/-1=128
						break;
					}
				}
				else	// this range will be 0..255 == 00..FF
				{
					if (srcInt < 0)	// High bit is set, so srcInt value is accurate
					{
						result.second = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt);	// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
						// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
						break;
					}

					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
					ulongest srcInt2 = (ulongest)-srcInt;
					int_t tmp = atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
					{
						return cogs::negative(cogs::divide_whole(t, srcInt2)); // unsigned/unsigned divide, wont grow
					});
					result = cogs::modulo(tmp, srcInt2); // src sign doesn't affect the sign of the result
				}
			}
		}
		else
		{
			if (src.get_length() <= 1)
			{
				result = cogs::modulo(cogs::post_assign_divide_whole(m_int, src.get_int()), src.get_int());
				break;
			}
		}

		result = cogs::exchange(m_int, 0);
		break;
	}

	return result;
}

template <bool has_sign, size_t n_bits>
fixed_integer_native<has_sign, n_bits>::fixed_integer_native(const dynamic_integer_content& src)
{
	operator=(src);
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator=(const dynamic_integer_content& src)
{
	cogs::assign(m_int, src.get_int());
	return *this;
}

template <bool has_sign, size_t n_bits>
fixed_integer_native<has_sign, n_bits>::fixed_integer_native(const dynamic_integer& src)
{ operator=(src); }

template <bool has_sign, size_t n_bits>
fixed_integer_native<has_sign, n_bits>::fixed_integer_native(const volatile dynamic_integer& src)
{ operator=(src); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator=(const dynamic_integer& src) { cogs::assign(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator=(const volatile dynamic_integer& src) { cogs::assign(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator=(const dynamic_integer& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator=(const volatile dynamic_integer& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_right(const dynamic_integer& src) const { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_right(const dynamic_integer& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_right(const volatile dynamic_integer& src) const { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_right(const volatile dynamic_integer& src) const volatile { return bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_right(const dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_right(const volatile dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_right(const dynamic_integer& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_right(const dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_right(const volatile dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_right(const dynamic_integer& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_right(const dynamic_integer& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_right(const volatile dynamic_integer& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_right(const dynamic_integer& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_left(const dynamic_integer& src) const { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_left(const dynamic_integer& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_left(const volatile dynamic_integer& src) const { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::bit_rotate_left(const volatile dynamic_integer& src) const volatile { return bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_left(const dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_left(const volatile dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_left(const dynamic_integer& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_left(const dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_left(const volatile dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_left(const dynamic_integer& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_left(const dynamic_integer& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_left(const volatile dynamic_integer& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_left(const dynamic_integer& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator>>(const dynamic_integer& src) const { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator>>(const dynamic_integer& src) const volatile { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator>>(const volatile dynamic_integer& src) const { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator>>(const volatile dynamic_integer& src) const volatile { return operator>>(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator>>=(const dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator>>=(const volatile dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator>>=(const dynamic_integer& src) volatile { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator>>=(const volatile dynamic_integer& src) volatile { operator>>=(src % bits_used_t()); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_right(const dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_right(const volatile dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_right(const dynamic_integer& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_right(const volatile dynamic_integer& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_right(const dynamic_integer& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_right(const volatile dynamic_integer& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_right(const dynamic_integer& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_right(const volatile dynamic_integer& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator<<(const dynamic_integer& src) const { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator<<(const dynamic_integer& src) const volatile { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator<<(const volatile dynamic_integer& src) const { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator<<(const volatile dynamic_integer& src) const volatile { return operator<<(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator<<=(const dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator<<=(const volatile dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator<<=(const dynamic_integer& src) volatile { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator<<=(const volatile dynamic_integer& src) volatile { operator<<=(src % bits_used_t()); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_left(const dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_left(const volatile dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_left(const dynamic_integer& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_bit_shift_left(const volatile dynamic_integer& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_left(const dynamic_integer& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_left(const volatile dynamic_integer& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_left(const dynamic_integer& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_bit_shift_left(const volatile dynamic_integer& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator+(const dynamic_integer& src) const { return src.operator+(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator+(const dynamic_integer& src) const volatile { return src.operator+(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator+(const volatile dynamic_integer& src) const { return src.operator+(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator+(const volatile dynamic_integer& src) const volatile { return src.operator+(*this); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator+=(const dynamic_integer& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator+=(const volatile dynamic_integer& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator+=(const dynamic_integer& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator+=(const volatile dynamic_integer& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_add(const dynamic_integer& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_add(const volatile dynamic_integer& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_add(const dynamic_integer& src) volatile { return cogs::pre_assign_add(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_add(const volatile dynamic_integer& src) volatile { return cogs::pre_assign_add(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_add(const dynamic_integer& src) { return cogs::post_assign_add(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_add(const volatile dynamic_integer& src) { return cogs::post_assign_add(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_add(const dynamic_integer& src) volatile { return cogs::post_assign_add(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_add(const volatile dynamic_integer& src) volatile { return cogs::post_assign_add(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator-(const dynamic_integer& src) const { return src.inverse_subtract(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator-(const dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator-(const volatile dynamic_integer& src) const { return src.inverse_subtract(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator-(const volatile dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator-=(const dynamic_integer& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator-=(const volatile dynamic_integer& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator-=(const dynamic_integer& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator-=(const volatile dynamic_integer& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_subtract(const dynamic_integer& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_subtract(const volatile dynamic_integer& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_subtract(const dynamic_integer& src) volatile { return cogs::pre_assign_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_subtract(const volatile dynamic_integer& src) volatile { return cogs::pre_assign_subtract(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_subtract(const dynamic_integer& src) { return cogs::post_assign_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_subtract(const volatile dynamic_integer& src) { return cogs::post_assign_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_subtract(const dynamic_integer& src) volatile { return cogs::post_assign_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_subtract(const volatile dynamic_integer& src) volatile { return cogs::post_assign_subtract(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_subtract(const dynamic_integer& src) const { return src - *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_subtract(const dynamic_integer& src) const volatile { return src - *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_subtract(const volatile dynamic_integer& src) const { return src - *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_subtract(const volatile dynamic_integer& src) const volatile { return src - *this; }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_subtract(const dynamic_integer& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_subtract(const volatile dynamic_integer& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_subtract(const dynamic_integer& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_subtract(const volatile dynamic_integer& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_subtract(const dynamic_integer& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_subtract(const volatile dynamic_integer& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_subtract(const dynamic_integer& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_subtract(const dynamic_integer& src) { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_subtract(const volatile dynamic_integer& src) { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_subtract(const dynamic_integer& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator*(const dynamic_integer& src) const { return src.operator*(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator*(const dynamic_integer& src) const volatile { return src.operator*(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator*(const volatile dynamic_integer& src) const { return src.operator*(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::operator*(const volatile dynamic_integer& src) const volatile { return src.operator*(*this); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator*=(const dynamic_integer& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator*=(const volatile dynamic_integer& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator*=(const dynamic_integer& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator*=(const volatile dynamic_integer& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_multiply(const dynamic_integer& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_multiply(const volatile dynamic_integer& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_multiply(const dynamic_integer& src) volatile { return cogs::pre_assign_multiply(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_multiply(const volatile dynamic_integer& src) volatile { return cogs::pre_assign_multiply(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_multiply(const dynamic_integer& src) { return cogs::post_assign_multiply(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_multiply(const volatile dynamic_integer& src) { return cogs::post_assign_multiply(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_multiply(const dynamic_integer& src) volatile { return cogs::post_assign_multiply(m_int, src.get_int()); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_multiply(const volatile dynamic_integer& src) volatile { return cogs::post_assign_multiply(m_int, src.get_int()); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator%(const dynamic_integer& src) const { return operator%(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator%(const dynamic_integer& src) const volatile { this_t tmp(*this); return tmp % src; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator%(const volatile dynamic_integer& src) const { auto rt = src.guarded_begin_read(); auto result = operator%(*rt); rt->release(); return result; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::operator%(const volatile dynamic_integer& src) const volatile { this_t tmp(*this); return tmp % src; }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const dynamic_integer& src) { operator%=(*(src.m_contents)); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const dynamic_integer& src) volatile { operator%=(*(src.m_contents)); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator%=(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const dynamic_integer& src) { return pre_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const dynamic_integer& src) volatile { return pre_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_modulo(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = pre_assign_modulo(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const dynamic_integer& src) { return post_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const dynamic_integer& src) volatile { return post_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const volatile dynamic_integer& src) { this_t tmp(*this);  auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_modulo(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = post_assign_modulo(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_modulo(const dynamic_integer& src) const { return src % *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_modulo(const dynamic_integer& src) const volatile { return src % *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_modulo(const volatile dynamic_integer& src) const { return src % *this; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_modulo(const volatile dynamic_integer& src) const volatile { return src % *this; }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_modulo(const dynamic_integer& src) { *this = src % *this; }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_modulo(const volatile dynamic_integer& src) { *this = src % *this; }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_modulo(const dynamic_integer& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src % t; }); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); assign_inverse_modulo(tmp); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_modulo(const dynamic_integer& src) { assign_inverse_modulo(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_modulo(const volatile dynamic_integer& src) { assign_inverse_modulo(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_modulo(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src % t; }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_modulo(const dynamic_integer& src) { this_t tmp(*this); assign_inverse_modulo(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_modulo(const volatile dynamic_integer& src) { this_t tmp(*this); assign_inverse_modulo(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_modulo(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src % t; }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }

template <bool has_sign, size_t n_bits>
inline fraction<fixed_integer_native<has_sign, n_bits>, dynamic_integer> fixed_integer_native<has_sign, n_bits>::operator/(const dynamic_integer& src) const { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t n_bits>
inline fraction<fixed_integer_native<has_sign, n_bits>, dynamic_integer> fixed_integer_native<has_sign, n_bits>::operator/(const dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t n_bits>
inline fraction<fixed_integer_native<has_sign, n_bits>, dynamic_integer> fixed_integer_native<has_sign, n_bits>::operator/(const volatile dynamic_integer& src) const { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t n_bits>
inline fraction<fixed_integer_native<has_sign, n_bits>, dynamic_integer> fixed_integer_native<has_sign, n_bits>::operator/(const volatile dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator/=(const dynamic_integer& src) { assign_divide_whole(*(src.m_contents)); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator/=(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator/=(const dynamic_integer& src) volatile { assign_divide_whole(*(src.m_contents)); return *this; }
template <bool has_sign, size_t n_bits>
inline volatile fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::operator/=(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); return *this; }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_divide(const dynamic_integer& src) { return *this /= src; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_divide(const volatile dynamic_integer& src) { return *this /= src; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_divide(const dynamic_integer& src) volatile { return pre_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_divide(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide(const dynamic_integer& src) { this_t tmp(*this); *this /= src; return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide(const volatile dynamic_integer& src) { this_t tmp(*this); *this /= src; return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide(const dynamic_integer& src) volatile { return post_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline fraction<dynamic_integer, fixed_integer_native<has_sign, n_bits> > fixed_integer_native<has_sign, n_bits>::inverse_divide(const dynamic_integer& src) const { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t n_bits>
inline fraction<dynamic_integer, fixed_integer_native<has_sign, n_bits> > fixed_integer_native<has_sign, n_bits>::inverse_divide(const dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t n_bits>
inline fraction<dynamic_integer, fixed_integer_native<has_sign, n_bits> > fixed_integer_native<has_sign, n_bits>::inverse_divide(const volatile dynamic_integer& src) const { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t n_bits>
inline fraction<dynamic_integer, fixed_integer_native<has_sign, n_bits> > fixed_integer_native<has_sign, n_bits>::inverse_divide(const volatile dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide(const dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide(const volatile dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide(const dynamic_integer& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); assign_inverse_divide(tmp); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide(const dynamic_integer& src) { assign_inverse_divide(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide(const volatile dynamic_integer& src) { assign_inverse_divide(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_divide(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide(const dynamic_integer& src) { this_t tmp(*this); post_assign_inverse_divide(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide(const volatile dynamic_integer& src) { this_t tmp(*this); post_assign_inverse_divide(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_divide(tmp); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole(const dynamic_integer& src) const { return divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole(const dynamic_integer& src) const volatile { this_t tmp(*this); return tmp.divide_whole(src); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return divide_whole(tmp); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole(const volatile dynamic_integer& src) const volatile { this_t tmp(*this); return tmp.divide_whole(src); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const dynamic_integer& src) { assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const dynamic_integer& src) volatile { assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_divide_whole(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release();}

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const dynamic_integer& src) { return *this /= src; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const volatile dynamic_integer& src) { return *this /= src; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const dynamic_integer& src) volatile { return pre_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_divide_whole(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const dynamic_integer& src) { this_t tmp(*this); *this /= src; return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const volatile dynamic_integer& src) { this_t tmp(*this); *this /= src; return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const dynamic_integer& src) volatile { return post_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_divide_whole(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole(const dynamic_integer& src) const { return src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole(const dynamic_integer& src) const volatile { return src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole(const volatile dynamic_integer& src) const { return src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole(const volatile dynamic_integer& src) const volatile { return src.divide_whole(*this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide_whole(const dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide_whole(const volatile dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide_whole(const dynamic_integer& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); assign_inverse_divide_whole(tmp); }


template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide_whole(const dynamic_integer& src) { assign_inverse_divide_whole(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) { assign_inverse_divide_whole(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide_whole(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_divide_whole(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide_whole(const dynamic_integer& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide_whole(const volatile dynamic_integer& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide_whole(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_divide_whole(tmp); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_modulo(const dynamic_integer& src) const { return divide_whole_and_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_modulo(const dynamic_integer& src) const volatile { this_t tmp(*this);  return tmp.divide_whole_and_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_modulo(const volatile dynamic_integer& src) const { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_modulo(*rt); rt->release(); return result; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { this_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const dynamic_integer& src) { return divide_whole_and_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const dynamic_integer& src) volatile { return divide_whole_and_assign_modulo(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_assign_modulo(*rt); rt->release(); return result; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::divide_whole_and_assign_modulo(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_assign_modulo(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const dynamic_integer& src) { return modulo_and_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const dynamic_integer& src) volatile { return modulo_and_assign_divide_whole(*(src.m_contents)); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const volatile dynamic_integer& src) { auto rt = src.guarded_begin_read(); auto result = modulo_and_assign_divide_whole(*rt); rt->release(); return result; }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::modulo_and_assign_divide_whole(const volatile dynamic_integer& src) volatile { auto rt = src.guarded_begin_read(); auto result = modulo_and_assign_divide_whole(*rt); rt->release(); return result; }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::gcd(const dynamic_integer& src) const { return src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::gcd(const dynamic_integer& src) const volatile { return src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::gcd(const volatile dynamic_integer& src) const { return src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::gcd(const volatile dynamic_integer& src) const volatile { return src.gcd(*this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_gcd(const dynamic_integer& src) { *this = src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_gcd(const volatile dynamic_integer& src) { *this = src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_gcd(const dynamic_integer& src) volatile { *this = src.gcd(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_gcd(const volatile dynamic_integer& src) volatile { *this = src.gcd(*this); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_gcd(const dynamic_integer& src) { assign_gcd(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_gcd(const volatile dynamic_integer& src) { assign_gcd(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_gcd(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.gcd(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_gcd(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_gcd(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_gcd(const dynamic_integer& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_gcd(const volatile dynamic_integer& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_gcd(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.gcd(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_gcd(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_gcd(tmp); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lcm(const dynamic_integer& src) const { return src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lcm(const dynamic_integer& src) const volatile { return src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lcm(const volatile dynamic_integer& src) const { return src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lcm(const volatile dynamic_integer& src) const volatile { return src.lcm(*this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lcm(const dynamic_integer& src) { *this = src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lcm(const volatile dynamic_integer& src) { *this = src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lcm(const dynamic_integer& src) volatile { *this = src.lcm(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lcm(const volatile dynamic_integer& src) volatile { *this = src.lcm(*this); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_lcm(const dynamic_integer& src) { assign_lcm(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_lcm(const volatile dynamic_integer& src) { assign_lcm(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_lcm(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lcm(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_lcm(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_lcm(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lcm(const dynamic_integer& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lcm(const volatile dynamic_integer& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lcm(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lcm(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lcm(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_lcm(tmp); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::greater(const dynamic_integer& src) const { return src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::greater(const dynamic_integer& src) const volatile { return src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::greater(const volatile dynamic_integer& src) const { return src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::greater(const volatile dynamic_integer& src) const volatile { return src.greater(*this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_greater(const dynamic_integer& src) { *this = src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_greater(const volatile dynamic_integer& src) { *this = src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_greater(const dynamic_integer& src) volatile { *this = src.greater(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_greater(const volatile dynamic_integer& src) volatile { *this = src.greater(*this); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_greater(const dynamic_integer& src) { assign_greater(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_greater(const volatile dynamic_integer& src) { assign_greater(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_greater(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.greater(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_greater(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_greater(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_greater(const dynamic_integer& src) { this_t tmp(*this); assign_greater(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_greater(const volatile dynamic_integer& src) { this_t tmp(*this); assign_greater(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_greater(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.greater(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_greater(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_greater(tmp); }

template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lesser(const dynamic_integer& src) const { return src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lesser(const dynamic_integer& src) const volatile { return src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lesser(const volatile dynamic_integer& src) const { return src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline auto fixed_integer_native<has_sign, n_bits>::lesser(const volatile dynamic_integer& src) const volatile { return src.lesser(*this); }

template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lesser(const dynamic_integer& src) { *this = src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lesser(const volatile dynamic_integer& src) { *this = src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lesser(const dynamic_integer& src) volatile { *this = src.lesser(*this); }
template <bool has_sign, size_t n_bits>
inline void fixed_integer_native<has_sign, n_bits>::assign_lesser(const volatile dynamic_integer& src) volatile { *this = src.lesser(*this); }

template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_lesser(const dynamic_integer& src) { assign_lesser(src); return *this; }
template <bool has_sign, size_t n_bits>
inline const fixed_integer_native<has_sign, n_bits>& fixed_integer_native<has_sign, n_bits>::pre_assign_lesser(const volatile dynamic_integer& src) { assign_lesser(src); return *this; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_lesser(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lesser(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::pre_assign_lesser(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_lesser(tmp); }

template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lesser(const dynamic_integer& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lesser(const volatile dynamic_integer& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lesser(const dynamic_integer& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lesser(t); }); }
template <bool has_sign, size_t n_bits>
inline fixed_integer_native<has_sign, n_bits> fixed_integer_native<has_sign, n_bits>::post_assign_lesser(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_lesser(tmp); }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator==(const dynamic_integer& src) const { return src == *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator==(const dynamic_integer& src) const volatile { return src == *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator==(const volatile dynamic_integer& src) const { return src == *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator==(const volatile dynamic_integer& src) const volatile { return src == *this; }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator!=(const dynamic_integer& src) const { return src != *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator!=(const dynamic_integer& src) const volatile { return src != *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator!=(const volatile dynamic_integer& src) const { return src != *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator!=(const volatile dynamic_integer& src) const volatile { return src != *this; }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<(const dynamic_integer& src) const { return src > *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<(const dynamic_integer& src) const volatile { return src > *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<(const volatile dynamic_integer& src) const { return src > *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<(const volatile dynamic_integer& src) const volatile { return src > *this; }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>(const dynamic_integer& src) const { return src < *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>(const dynamic_integer& src) const volatile { return src < *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>(const volatile dynamic_integer& src) const { return src < *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>(const volatile dynamic_integer& src) const volatile { return src < *this; }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<=(const dynamic_integer& src) const { return src >= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<=(const dynamic_integer& src) const volatile { return src >= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<=(const volatile dynamic_integer& src) const { return src >= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator<=(const volatile dynamic_integer& src) const volatile { return src >= *this; }

template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>=(const dynamic_integer& src) const { return src <= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>=(const dynamic_integer& src) const volatile { return src <= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>=(const volatile dynamic_integer& src) const { return src <= *this; }
template <bool has_sign, size_t n_bits>
inline bool fixed_integer_native<has_sign, n_bits>::operator>=(const volatile dynamic_integer& src) const volatile { return src <= *this; }

template <bool has_sign, size_t n_bits>
inline int fixed_integer_native<has_sign, n_bits>::compare(const dynamic_integer& src) const { return -src.compare(*this); }
template <bool has_sign, size_t n_bits>
inline int fixed_integer_native<has_sign, n_bits>::compare(const dynamic_integer& src) const volatile { return -src.compare(*this); }
template <bool has_sign, size_t n_bits>
inline int fixed_integer_native<has_sign, n_bits>::compare(const volatile dynamic_integer& src) const { return -src.compare(*this); }
template <bool has_sign, size_t n_bits>
inline int fixed_integer_native<has_sign, n_bits>::compare(const volatile dynamic_integer& src) const volatile { return -src.compare(*this); }


// fixed_integer_extended



template <bool has_sign, size_t n_bits>
inline fixed_integer_extended_content<has_sign, n_bits>& fixed_integer_extended_content<has_sign, n_bits>::operator=(const dynamic_integer_content& src)
{
	if (src.is_negative())
		assign_negative(src.get_const_ptr(), src.get_length());
	else
		assign(src.get_const_ptr(), src.get_length());
	return *this;
}

template <bool has_sign, size_t n_bits>
inline fixed_integer_extended_content<has_sign, n_bits>& fixed_integer_extended_content<has_sign, n_bits>::operator=(const dynamic_integer& src)
{
	return operator=(*(src.m_contents));
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_extended_content<has_sign, n_bits>::add(const dynamic_integer& src)
{
	if (src.is_negative())
		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	else
		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_extended_content<has_sign, n_bits>::subtract(const dynamic_integer& src)
{
	if (src.is_negative())
		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	else
		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_extended_content<has_sign, n_bits>::inverse_subtract(const dynamic_integer& src)
{
	if (src.is_negative())
		add(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	else
		subtract(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	assign_negative();
}


template <bool has_sign, size_t n_bits>
inline void fixed_integer_extended_content<has_sign, n_bits>::multiply(const this_t& src1, const dynamic_integer_content& src2)
{
	if (src1.is_negative())
	{
		this_t abs1;
		abs1.assign_negative(src1);
		multiply(abs1, src2.get_const_ptr(), src2.get_length());
		if (!src2.is_negative())
			assign_negative();
	}
	else
	{
		multiply(src1, src2.get_const_ptr(), src2.get_length());
		if (src2.is_negative())
			assign_negative();
	}
}

template <bool has_sign, size_t n_bits>
inline void fixed_integer_extended_content<has_sign, n_bits>::multiply(const dynamic_integer_content& src)
{
	if (is_negative())
	{
		this_t abs1;
		abs1.assign_negative(*this);
		multiply(abs1, src.get_const_ptr(), src.get_length());
		if (!src.is_negative())
			assign_negative();
	}
	else
	{
		this_t tmp(*this);
		multiply(tmp, src.get_const_ptr(), src.get_length());
		if (src.is_negative())
			assign_negative();
	}
}

template <bool has_sign, size_t n_bits>
template <bool has_sign3, size_t bits3>
inline void fixed_integer_extended_content<has_sign, n_bits>::divide_whole_and_assign_modulo(const dynamic_integer_content& denom, fixed_integer_extended_content<has_sign3, bits3>* result)
{
	if (!!result)
		result->clear();

	const ulongest* denomDigits = denom.get_const_ptr();
	size_t denomLength = denom.get_length();

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


//----

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>::fixed_integer_extended(const dynamic_integer& src) { operator=(src); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>::fixed_integer_extended(const volatile dynamic_integer& src) { operator=(src); }


template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator=(const dynamic_integer& src)
{
	if (src.is_negative())
		m_contents->assign_negative(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	else
		m_contents->assign(src.m_contents->get_const_ptr(), src.m_contents->get_length());
	return *this;
}

template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator=(const dynamic_integer& src) volatile
{
	this_t tmp(src);
	return operator=(tmp);
}

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator=(const volatile dynamic_integer& src)
{
	dynamic_integer tmp(src);
	return operator=(tmp);
}

template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator=(const volatile dynamic_integer& src) volatile
{
	dynamic_integer tmp(src);
	return operator=(tmp);
}

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_right(const dynamic_integer& src) const { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_right(const dynamic_integer& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_right(const volatile dynamic_integer& src) const { return bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_right(const volatile dynamic_integer& src) const volatile { return bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_right(const dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_right(const volatile dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_right(const dynamic_integer& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_right(const dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_right(const volatile dynamic_integer& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_right(const dynamic_integer& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_right(const dynamic_integer& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_right(const volatile dynamic_integer& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_right(const dynamic_integer& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_left(const dynamic_integer& src) const { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_left(const dynamic_integer& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_left(const volatile dynamic_integer& src) const { return bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::bit_rotate_left(const volatile dynamic_integer& src) const volatile { return bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_left(const dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_left(const volatile dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_left(const dynamic_integer& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_left(const dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_left(const volatile dynamic_integer& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_left(const dynamic_integer& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_left(const dynamic_integer& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_left(const volatile dynamic_integer& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_left(const dynamic_integer& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator>>(const dynamic_integer& src) const { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator>>(const dynamic_integer& src) const volatile { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator>>(const volatile dynamic_integer& src) const { return operator>>(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator>>(const volatile dynamic_integer& src) const volatile { return operator>>(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator>>=(const dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator>>=(const volatile dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator>>=(const dynamic_integer& src) volatile { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator>>=(const volatile dynamic_integer& src) volatile { operator>>=(src % bits_used_t()); return *this; }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_right(const dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_right(const volatile dynamic_integer& src) { operator>>=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_right(const dynamic_integer& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_right(const volatile dynamic_integer& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_right(const dynamic_integer& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_right(const volatile dynamic_integer& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_right(const dynamic_integer& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_right(const volatile dynamic_integer& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator<<(const dynamic_integer& src) const { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator<<(const dynamic_integer& src) const volatile { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator<<(const volatile dynamic_integer& src) const { return operator<<(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::operator<<(const volatile dynamic_integer& src) const volatile { return operator<<(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator<<=(const dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator<<=(const volatile dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator<<=(const dynamic_integer& src) volatile { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator<<=(const volatile dynamic_integer& src) volatile { operator<<=(src % bits_used_t()); return *this; }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_left(const dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_left(const volatile dynamic_integer& src) { operator<<=(src % bits_used_t()); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_left(const dynamic_integer& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_bit_shift_left(const volatile dynamic_integer& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_left(const dynamic_integer& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_left(const volatile dynamic_integer& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_left(const dynamic_integer& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_bit_shift_left(const volatile dynamic_integer& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator+(const dynamic_integer& src) const { return src + *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator+(const dynamic_integer& src) const volatile { return src + *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator+(const volatile dynamic_integer& src) const { return src + *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator+(const volatile dynamic_integer& src) const volatile { return src + *this; }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator+=(const dynamic_integer& src) { m_contents->add(src); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator+=(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.add(src); }); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator+=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator+=(tmp); }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator+=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator+=(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_add(const dynamic_integer& src) { m_contents->add(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_add(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.add(src); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_add(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_add(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_add(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_add(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_add(const dynamic_integer& src) { this_t tmp(*this); m_contents->add(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_add(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.add(src); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_add(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_add(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_add(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_add(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator-(const dynamic_integer& src) const { return src.inverse_subtract(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator-(const dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator-(const volatile dynamic_integer& src) const { return src.inverse_subtract(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator-(const volatile dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator-=(const dynamic_integer& src) { m_contents->subtract(src); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator-=(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.subtract(src); }); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator-=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator-=(tmp); }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator-=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator-=(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_subtract(const dynamic_integer& src) { m_contents->subtract(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_subtract(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.subtract(src); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_subtract(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_subtract(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_subtract(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_subtract(const dynamic_integer& src) { this_t tmp(*this); m_contents->subtract(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_subtract(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.subtract(src); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_subtract(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_subtract(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_subtract(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_subtract(const dynamic_integer& src) const { return src - *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_subtract(const dynamic_integer& src) const volatile { return src - *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_subtract(const volatile dynamic_integer& src) const { return src - *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_subtract(const volatile dynamic_integer& src) const volatile { return src - *this; }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_subtract(const dynamic_integer& src) { m_contents->inverse_subtract(src); return *this; }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_subtract(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.inverse_subtract(src); }); return *this; }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator-=(tmp); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_subtract(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator-=(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_subtract(const dynamic_integer& src) { m_contents->inverse_subtract(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_subtract(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.inverse_subtract(src); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_inverse_subtract(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_subtract(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_subtract(const dynamic_integer& src) { this_t tmp(*this); m_contents->inverse_subtract(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_subtract(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.inverse_subtract(src); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_subtract(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_inverse_subtract(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_subtract(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_subtract(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator*(const dynamic_integer& src) const { return src * *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator*(const dynamic_integer& src) const volatile { return src * *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator*(const volatile dynamic_integer& src) const { return src * *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator*(const volatile dynamic_integer& src) const volatile { return src * *this; }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator*=(const dynamic_integer& src) { m_contents->multiply(src); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator*=(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.multiply(src); }); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator*=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator*=(tmp); }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator*=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator*=(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_multiply(const dynamic_integer& src) { m_contents->multiply(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_multiply(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.multiply(src); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_multiply(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_multiply(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_multiply(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_multiply(const dynamic_integer& src) { this_t tmp(*this); m_contents->multiply(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_multiply(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.multiply(src); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_multiply(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_multiply(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_multiply(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_multiply(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator%(const dynamic_integer& src) const { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator%(const dynamic_integer& src) const volatile { this_t tmp(*this); tmp.m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator%(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return operator%(tmp); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::operator%(const volatile dynamic_integer& src) const volatile { dynamic_integer tmp(src); return operator%(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator%=(const dynamic_integer& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator%=(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator%=(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return operator%=(tmp); }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator%=(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return operator%=(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_modulo(const dynamic_integer& src) { m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_modulo(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_modulo(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_modulo(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_modulo(const dynamic_integer& src) { this_t tmp(*this); m_contents->divide_whole_and_assign_modulo(*(src.m_contents)); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_modulo(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c.divide_whole_and_assign_modulo(*(src.m_contents)); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_modulo(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_modulo(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_modulo(const dynamic_integer& src) const { return src % *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_modulo(const dynamic_integer& src) const volatile { return src % *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_modulo(const volatile dynamic_integer& src) const { return src % *this; }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_modulo(const volatile dynamic_integer& src) const volatile { return src % *this; }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_modulo(const dynamic_integer& src) { *this = src % *this; }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_modulo(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); return *this; }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return assign_inverse_modulo(tmp); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return assign_inverse_modulo(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_modulo(const dynamic_integer& src) { *this = src % *this; return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_modulo(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_modulo(const dynamic_integer& src) { this_t tmp(*this); *this = src % *this; return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_modulo(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { dynamic_integer tmp = src % *this; c = *(tmp.m_contents); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }

template <bool has_sign, size_t bits>
inline fraction<fixed_integer_extended<has_sign, bits>, dynamic_integer> fixed_integer_extended<has_sign, bits>::operator/(const dynamic_integer& src) const { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t bits>
inline fraction<fixed_integer_extended<has_sign, bits>, dynamic_integer> fixed_integer_extended<has_sign, bits>::operator/(const dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t bits>
inline fraction<fixed_integer_extended<has_sign, bits>, dynamic_integer> fixed_integer_extended<has_sign, bits>::operator/(const volatile dynamic_integer& src) const { return fraction<this_t, dynamic_integer>(*this, src); }
template <bool has_sign, size_t bits>
inline fraction<fixed_integer_extended<has_sign, bits>, dynamic_integer> fixed_integer_extended<has_sign, bits>::operator/(const volatile dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator/=(const dynamic_integer& src) { assign_divide_whole(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator/=(const volatile dynamic_integer& src) { assign_divide_whole(src); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator/=(const dynamic_integer& src) volatile { assign_divide_whole(src); return *this; }
template <bool has_sign, size_t bits>
inline volatile fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::operator/=(const volatile dynamic_integer& src) volatile { assign_divide_whole(src); return *this; }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_divide(const dynamic_integer& src) { return pre_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_divide(const volatile dynamic_integer& src) { return pre_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_divide(const dynamic_integer& src) volatile { return pre_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_divide(const volatile dynamic_integer& src) volatile { return pre_assign_divide_whole(src); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide(const dynamic_integer& src) { return post_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide(const volatile dynamic_integer& src) { return post_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide(const dynamic_integer& src) volatile { return post_assign_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide(const volatile dynamic_integer& src) volatile { return post_assign_divide_whole(src); }

template <bool has_sign, size_t bits>
inline fraction<dynamic_integer, fixed_integer_extended<has_sign, bits>> fixed_integer_extended<has_sign, bits>::inverse_divide(const dynamic_integer& src) const { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t bits>
inline fraction<dynamic_integer, fixed_integer_extended<has_sign, bits>> fixed_integer_extended<has_sign, bits>::inverse_divide(const dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t bits>
inline fraction<dynamic_integer, fixed_integer_extended<has_sign, bits>> fixed_integer_extended<has_sign, bits>::inverse_divide(const volatile dynamic_integer& src) const { return fraction<dynamic_integer, this_t>(src, *this); }
template <bool has_sign, size_t bits>
inline fraction<dynamic_integer, fixed_integer_extended<has_sign, bits>> fixed_integer_extended<has_sign, bits>::inverse_divide(const volatile dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide(const dynamic_integer& src) { assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide(const volatile dynamic_integer& src) { assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide(const dynamic_integer& src) volatile { assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide(const volatile dynamic_integer& src) volatile { assign_inverse_divide_whole(src); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide(const dynamic_integer& src) { return pre_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide(const volatile dynamic_integer& src) { return pre_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide(const dynamic_integer& src) volatile { return pre_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide(const volatile dynamic_integer& src) volatile { return pre_assign_inverse_divide_whole(src); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide(const dynamic_integer& src) { return post_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide(const volatile dynamic_integer& src) { return post_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide(const dynamic_integer& src) volatile { return post_assign_inverse_divide_whole(src); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide(const volatile dynamic_integer& src) volatile { return post_assign_inverse_divide_whole(src); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole(const dynamic_integer& src) const
{
	fixed_integer_extended<true, bits + 1> result;
	this_t remainder(*this);
	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
	return result;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole(const dynamic_integer& src) const volatile
{
	fixed_integer_extended<true, bits + 1> result;
	this_t remainder(*this);
	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
	return result;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return divide_whole(tmp); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole(const volatile dynamic_integer& src) const volatile { dynamic_integer tmp(src); return divide_whole(tmp); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_divide_whole(const dynamic_integer& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src); assign_divide_whole(tmp); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_divide_whole(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); assign_divide_whole(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_divide_whole(const dynamic_integer& src) { this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return pre_assign_divide_whole(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_divide_whole(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_divide_whole(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide_whole(const dynamic_integer& src) { this_t tmp(*this); this_t remainder(*this); remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return post_assign_divide_whole(tmp); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide_whole(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { content_t remainder(c); remainder.divide_whole_and_assign_modulo(*(src.m_contents), &c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_divide_whole(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole(const dynamic_integer& src) const { return src.divide_whole(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole(const dynamic_integer& src) const volatile { return src.divide_whole(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole(const volatile dynamic_integer& src) const { return src.divide_whole(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole(const volatile dynamic_integer& src) const volatile { return src.divide_whole(*this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide_whole(const dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide_whole(const volatile dynamic_integer& src) { *this = src.divide_whole(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide_whole(const dynamic_integer& src) volatile { write_retry_loop([&](content_t& c) { c = src.divide_whole(c); }); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); assign_inverse_divide(tmp); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide_whole(const dynamic_integer& src) { assign_inverse_divide(src); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) { assign_inverse_divide(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide_whole(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.divide_whole(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_inverse_divide(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide_whole(const dynamic_integer& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide_whole(const volatile dynamic_integer& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide_whole(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.divide_whole(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_inverse_divide(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_modulo(const dynamic_integer& src) const
{
	fixed_integer<true, bits + 1> divided;
	this_t remainder(*this);
	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(divided.m_contents));
	return make_pair(divided, remainder);
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_modulo(const dynamic_integer& src) const volatile
{
	fixed_integer<true, bits + 1> divided;
	this_t remainder(*this);
	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(divided.m_contents));
	return make_pair(divided, remainder);
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_modulo(const volatile dynamic_integer& src) const { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { dynamic_integer tmp(src); return divide_whole_and_modulo(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const { return src.divide_whole_and_modulo(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const volatile { return src.divide_whole_and_modulo(*this); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_assign_modulo(const dynamic_integer& src)
{
	fixed_integer_extended<true, bits + 1> result;
	m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
	return result;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_assign_modulo(const dynamic_integer& src) volatile
{
	fixed_integer_extended<true, bits + 1> result;
	write_retry_loop([&](content_t& c)
		{
			c.divide_whole_and_assign_modulo(*(src.m_contents), &*(result.m_contents));
		});
	return result;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_assign_modulo(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return divide_whole_and_assign_modulo(tmp); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::divide_whole_and_assign_modulo(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return divide_whole_and_assign_modulo(tmp); }


template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::modulo_and_assign_divide_whole(const dynamic_integer& src)
{
	this_t remainder(*this);
	remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &*m_contents);
	return remainder;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::modulo_and_assign_divide_whole(const dynamic_integer& src) volatile
{
	this_t remainder;
	write_retry_loop([&](content_t& c)
		{
			*(remainder.m_contents) = c;
			remainder.m_contents->divide_whole_and_assign_modulo(*(src.m_contents), &c);
		});
	return remainder;
}

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::modulo_and_assign_divide_whole(const volatile dynamic_integer& src) { dynamic_integer tmp(src); return modulo_and_assign_divide_whole(tmp); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::modulo_and_assign_divide_whole(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return modulo_and_assign_divide_whole(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::gcd(const dynamic_integer& src) const { return src.gcd(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::gcd(const dynamic_integer& src) const volatile { return src.gcd(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::gcd(const volatile dynamic_integer& src) const { return src.gcd(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::gcd(const volatile dynamic_integer& src) const volatile { return src.gcd(*this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_gcd(const dynamic_integer& src) { *this = src.gcd(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_gcd(const volatile dynamic_integer& src) { *this = src.gcd(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_gcd(const dynamic_integer& src) volatile { *this = src.gcd(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_gcd(const volatile dynamic_integer& src) volatile { *this = src.gcd(*this); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_gcd(const dynamic_integer& src) { assign_gcd(src); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_gcd(const volatile dynamic_integer& src) { assign_gcd(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_gcd(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.gcd(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_gcd(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_gcd(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_gcd(const dynamic_integer& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_gcd(const volatile dynamic_integer& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_gcd(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.gcd(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_gcd(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_gcd(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lcm(const dynamic_integer& src) const { return src.lcm(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lcm(const dynamic_integer& src) const volatile { return src.lcm(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lcm(const volatile dynamic_integer& src) const { return src.lcm(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lcm(const volatile dynamic_integer& src) const volatile { return src.lcm(*this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lcm(const dynamic_integer& src) { *this = src.lcm(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lcm(const volatile dynamic_integer& src) { *this = src.lcm(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lcm(const dynamic_integer& src) volatile { *this = src.lcm(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lcm(const volatile dynamic_integer& src) volatile { *this = src.lcm(*this); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_lcm(const dynamic_integer& src) { assign_lcm(src); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_lcm(const volatile dynamic_integer& src) { assign_lcm(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_lcm(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.lcm(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_lcm(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_lcm(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lcm(const dynamic_integer& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lcm(const volatile dynamic_integer& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lcm(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.lcm(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lcm(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_lcm(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::greater(const dynamic_integer& src) const { return src.greater(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::greater(const dynamic_integer& src) const volatile { return src.greater(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::greater(const volatile dynamic_integer& src) const { return src.greater(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::greater(const volatile dynamic_integer& src) const volatile { return src.greater(*this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_greater(const dynamic_integer& src) { *this = src.greater(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_greater(const volatile dynamic_integer& src) { *this = src.greater(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_greater(const dynamic_integer& src) volatile { *this = src.greater(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_greater(const volatile dynamic_integer& src) volatile { *this = src.greater(*this); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_greater(const dynamic_integer& src) { assign_greater(src); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_greater(const volatile dynamic_integer& src) { assign_greater(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_greater(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.greater(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_greater(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_greater(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_greater(const dynamic_integer& src) { this_t tmp(*this); assign_greater(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_greater(const volatile dynamic_integer& src) { this_t tmp(*this); assign_greater(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_greater(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.greater(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_greater(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_greater(tmp); }

template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lesser(const dynamic_integer& src) const { return src.lesser(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lesser(const dynamic_integer& src) const volatile { return src.lesser(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lesser(const volatile dynamic_integer& src) const { return src.lesser(*this); }
template <bool has_sign, size_t bits>
inline auto fixed_integer_extended<has_sign, bits>::lesser(const volatile dynamic_integer& src) const volatile { return src.lesser(*this); }

template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lesser(const dynamic_integer& src) { *this = src.lesser(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lesser(const volatile dynamic_integer& src) { *this = src.lesser(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lesser(const dynamic_integer& src) volatile { *this = src.lesser(*this); }
template <bool has_sign, size_t bits>
inline void fixed_integer_extended<has_sign, bits>::assign_lesser(const volatile dynamic_integer& src) volatile { *this = src.lesser(*this); }

template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_lesser(const dynamic_integer& src) { assign_lesser(src); return *this; }
template <bool has_sign, size_t bits>
inline const fixed_integer_extended<has_sign, bits>& fixed_integer_extended<has_sign, bits>::pre_assign_lesser(const volatile dynamic_integer& src) { assign_lesser(src); return *this; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_lesser(const dynamic_integer& src) volatile { return write_retry_loop_pre([&](content_t& c) { c = src.lesser(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::pre_assign_lesser(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return pre_assign_lesser(tmp); }

template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lesser(const dynamic_integer& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lesser(const volatile dynamic_integer& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lesser(const dynamic_integer& src) volatile { return write_retry_loop_post([&](content_t& c) { c = src.lesser(c); }); }
template <bool has_sign, size_t bits>
inline fixed_integer_extended<has_sign, bits> fixed_integer_extended<has_sign, bits>::post_assign_lesser(const volatile dynamic_integer& src) volatile { dynamic_integer tmp(src); return post_assign_lesser(tmp); }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator==(const dynamic_integer& src) const { return src == *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator==(const dynamic_integer& src) const volatile { return src == *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator==(const volatile dynamic_integer& src) const { return src == *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator==(const volatile dynamic_integer& src) const volatile { return src == *this; }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator!=(const dynamic_integer& src) const { return src != *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator!=(const dynamic_integer& src) const volatile { return src != *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator!=(const volatile dynamic_integer& src) const { return src != *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator!=(const volatile dynamic_integer& src) const volatile { return src != *this; }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<(const dynamic_integer& src) const { return src > *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<(const dynamic_integer& src) const volatile { return src > *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<(const volatile dynamic_integer& src) const { return src > *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<(const volatile dynamic_integer& src) const volatile { return src > *this; }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>(const dynamic_integer& src) const { return src < *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>(const dynamic_integer& src) const volatile { return src < *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>(const volatile dynamic_integer& src) const { return src < *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>(const volatile dynamic_integer& src) const volatile { return src < *this; }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<=(const dynamic_integer& src) const { return src >= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<=(const dynamic_integer& src) const volatile { return src >= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<=(const volatile dynamic_integer& src) const { return src >= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator<=(const volatile dynamic_integer& src) const volatile { return src >= *this; }

template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>=(const dynamic_integer& src) const { return src <= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>=(const dynamic_integer& src) const volatile { return src <= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>=(const volatile dynamic_integer& src) const { return src <= *this; }
template <bool has_sign, size_t bits>
inline bool fixed_integer_extended<has_sign, bits>::operator>=(const volatile dynamic_integer& src) const volatile { return src <= *this; }

template <bool has_sign, size_t bits>
inline int fixed_integer_extended<has_sign, bits>::compare(const dynamic_integer& src) const { return -src.compare(*this); }
template <bool has_sign, size_t bits>
inline int fixed_integer_extended<has_sign, bits>::compare(const dynamic_integer& src) const volatile { return -src.compare(*this); }
template <bool has_sign, size_t bits>
inline int fixed_integer_extended<has_sign, bits>::compare(const volatile dynamic_integer& src) const { return -src.compare(*this); }
template <bool has_sign, size_t bits>
inline int fixed_integer_extended<has_sign, bits>::compare(const volatile dynamic_integer& src) const volatile { return -src.compare(*this); }


// fixed_integer_native_const



template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator+(const dynamic_integer& src) const volatile { return src.operator+(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator+(const volatile dynamic_integer& src) const volatile { return src.operator+(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator-(const dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator-(const volatile dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_subtract(const dynamic_integer& src) const volatile { return src - *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_subtract(const volatile dynamic_integer& src) const volatile { return src - *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator*(const dynamic_integer& src) const volatile { return src.operator*(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator*(const volatile dynamic_integer& src) const volatile { return src.operator*(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator%(const dynamic_integer& src) const volatile { return src.inverse_modulo(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator%(const volatile dynamic_integer& src) const volatile { return src.inverse_modulo(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_modulo(const dynamic_integer& src) const volatile { return src % *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_modulo(const volatile dynamic_integer& src) const volatile { return src % *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator/(const dynamic_integer& src) const volatile
{ return fraction<this_t, dynamic_integer>(*this, src); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::operator/(const volatile dynamic_integer& src) const volatile
{ return fraction<this_t, dynamic_integer>(*this, src); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_divide(const dynamic_integer& src) const volatile
{ return fraction<dynamic_integer, this_t>(src, *this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_divide(const volatile dynamic_integer& src) const volatile
{ return fraction<dynamic_integer, this_t>(src, *this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::divide_whole(const dynamic_integer& src) const volatile { return src.inverse_divide_whole(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::divide_whole(const volatile dynamic_integer& src) const volatile { return src.inverse_divide_whole(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_divide_whole(const dynamic_integer& src) const volatile { return src.divide_whole(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::inverse_divide_whole(const volatile dynamic_integer& src) const volatile { return src.divide_whole(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::divide_whole_and_modulo(const dynamic_integer& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline auto fixed_integer_native_const<has_sign, bits, value>::divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::gcd(const dynamic_integer& t2) const volatile { return t2.gcd(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::gcd(const volatile dynamic_integer& t2) const volatile { return t2.gcd(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::lcm(const dynamic_integer& t2) const volatile { return t2.lcm(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::lcm(const volatile dynamic_integer& t2) const volatile { return t2.lcm(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::greater(const dynamic_integer& t2) const volatile { return t2.greater(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::greater(const volatile dynamic_integer& t2) const volatile { return t2.greater(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::lesser(const dynamic_integer& t2) const volatile { return t2.lesser(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <bool has_sign2, size_t bits2>
inline auto fixed_integer_native_const<has_sign, bits, value>::lesser(const volatile dynamic_integer& t2) const volatile { return t2.lesser(*this); }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator==(const dynamic_integer& cmp) const volatile { return cmp == *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator==(const volatile dynamic_integer& cmp) const volatile { return cmp == *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator!=(const dynamic_integer& cmp) const volatile { return cmp != *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator!=(const volatile dynamic_integer& cmp) const volatile { return cmp != *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator<(const dynamic_integer& cmp) const volatile { return cmp > *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator<(const volatile dynamic_integer& cmp) const volatile { return cmp > *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator>(const dynamic_integer& cmp) const volatile { return cmp < *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator>(const volatile dynamic_integer& cmp) const volatile { return cmp < *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator<=(const dynamic_integer& cmp) const volatile { return cmp >= *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator<=(const volatile dynamic_integer& cmp) const volatile { return cmp >= *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator>=(const dynamic_integer& cmp) const volatile { return cmp <= *this; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline bool fixed_integer_native_const<has_sign, bits, value>::operator>=(const volatile dynamic_integer& cmp) const volatile { return cmp <= *this; }


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline int fixed_integer_native_const<has_sign, bits, value>::compare(const dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline int fixed_integer_native_const<has_sign, bits, value>::compare(const volatile dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }



// fixed_integer_extended_const



template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator+(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp + src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator+(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp + src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator-(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp - src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator-(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp - src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_subtract(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_subtract(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_subtract(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator*(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp * src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator*(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp * src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator%(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp % src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator%(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp % src; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_modulo(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_modulo(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_modulo(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator/(const dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator/(const volatile dynamic_integer& src) const volatile { return fraction<this_t, dynamic_integer>(*this, src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_divide(const dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_divide(const volatile dynamic_integer& src) const volatile { return fraction<dynamic_integer, this_t>(src, *this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::divide_whole(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::divide_whole(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_divide_whole(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::inverse_divide_whole(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.inverse_divide_whole(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::divide_whole_and_modulo(const dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { non_const_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::gcd(const dynamic_integer& t2) const volatile { return t2.gcd(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::gcd(const volatile dynamic_integer& t2) const volatile { return t2.gcd(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::lcm(const dynamic_integer& t2) const volatile { return t2.lcm(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::lcm(const volatile dynamic_integer& t2) const volatile { return t2.lcm(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::greater(const dynamic_integer& t2) const volatile { return t2.greater(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::greater(const volatile dynamic_integer& t2) const volatile { return t2.greater(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::lesser(const dynamic_integer& t2) const volatile { return t2.lesser(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline auto fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::lesser(const volatile dynamic_integer& t2) const volatile { return t2.lesser(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator==(const dynamic_integer& cmp) const volatile { return cmp == *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator==(const volatile dynamic_integer& cmp) const volatile { return cmp == *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator!=(const dynamic_integer& cmp) const volatile { return cmp != *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator!=(const volatile dynamic_integer& cmp) const volatile { return cmp != *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator<(const dynamic_integer& cmp) const volatile { return cmp > *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator<(const volatile dynamic_integer& cmp) const volatile { return cmp > *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator>(const dynamic_integer& cmp) const volatile { return cmp < *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator>(const volatile dynamic_integer& cmp) const volatile { return cmp < *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator<=(const dynamic_integer& cmp) const volatile { return cmp >= *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator<=(const volatile dynamic_integer& cmp) const volatile { return cmp >= *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator>=(const dynamic_integer& cmp) const volatile { return cmp <= *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline bool fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::operator>=(const volatile dynamic_integer& cmp) const volatile { return cmp <= *this; }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline int fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::compare(const dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }

template <bool has_sign, size_t bits, ulongest low_digit, ulongest... highDigits>
inline int fixed_integer_extended_const<has_sign, bits, low_digit, highDigits...>::compare(const volatile dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }





template <bool has_sign, size_t bits>
class compatible<fixed_integer_extended<has_sign, bits>, dynamic_integer>
{
public:
	typedef dynamic_integer type;
};




}


#pragma warning(pop)


#endif
