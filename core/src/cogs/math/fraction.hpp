//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good


#ifndef COGS_HEADER_MATH_FRACTION
#define COGS_HEADER_MATH_FRACTION

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/compatible.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/is_signed.hpp"
#include "cogs/math/is_integral.hpp"
#include "cogs/math/is_arithmetic.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified
#pragma warning (disable: 4307)	// multiple copy constructors specified


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
class fixed_integer_native_const;


template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;

template <typename numerator_t, typename denominator_t>
class fraction;

template <typename numerator_t, typename denominator_t>
class is_arithmetic<fraction<numerator_t, denominator_t> >
{
public:
	static constexpr bool value = true;
};

template <typename numerator_t, typename denominator_t>
class is_signed<fraction<numerator_t, denominator_t> >
{
public:
	static constexpr bool value = is_signed_v<numerator_t> || is_signed_v<denominator_t>;
};

template <typename numerator_type, typename denominator_type = numerator_type>
class fraction_content
{
private:
	static_assert(!std::is_same_v<denominator_type, fixed_integer_native_const<false, 0, 0> >);
	static_assert(!std::is_reference_v<numerator_type>);
	static_assert(!std::is_reference_v<denominator_type>);
	static_assert(!std::is_volatile_v<numerator_type>);
	static_assert(!std::is_volatile_v<denominator_type>);

public:
	typedef numerator_type numerator_t;
	typedef denominator_type denominator_t;

	typedef fraction<numerator_t, denominator_t> fraction_t;
	typedef fraction_content<numerator_t, denominator_t> this_t;

	template <typename, typename>
	friend class fraction_content;

	numerator_t		m_numerator;
	denominator_t	m_denominator;

	fraction_content()
	{ }

	template <typename numerator_t2, typename denominator_t2>
	fraction_content(const fraction_content<numerator_t2, denominator_t2>& src)
		: m_numerator(src.m_numerator),
		m_denominator(src.m_denominator)
	{ }

	template <typename numerator_t2, typename denominator_t2>
	fraction_content(fraction_content<numerator_t2, denominator_t2>&& src)
		: m_numerator(std::move(src.m_numerator)),
		m_denominator(std::move(src.m_denominator))
	{ }


	template <typename numerator_t2, typename denominator_t2>
	fraction_content(const fraction<numerator_t2, denominator_t2>& src)
		: fraction_content(fraction<numerator_t2, denominator_t2>::simplify_content_type(src))
	{ }

	template <typename numerator_t2, typename denominator_t2>
	fraction_content(fraction<numerator_t2, denominator_t2>&& src)
		: fraction_content(fraction<numerator_t2, denominator_t2>::simplify_content_type(std::move(src)))
	{ }


	template <typename numerator_t2, typename denominator_t2>
	fraction_content(numerator_t2&& n, denominator_t2&& d)
		: m_numerator(load(std::forward<numerator_t2>(n))),
		m_denominator(load(std::forward<denominator_t2>(d)))
	{ }


	template <typename numerator_t2, typename denominator_t2>
	this_t& operator=(fraction_content<numerator_t2, denominator_t2>& src)
	{
		cogs::assign(m_numerator, src.m_numerator);
		cogs::assign(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2, typename denominator_t2>
	this_t& operator=(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		cogs::assign(m_numerator, src.m_numerator);
		cogs::assign(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2, typename denominator_t2>
	this_t& operator=(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		cogs::assign(m_numerator, std::move(src.m_numerator));
		cogs::assign(m_denominator, std::move(src.m_denominator));
		return *this;
	}

	template <typename numerator_t2, typename = std::enable_if_t<!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2> > >
	this_t& operator=(numerator_t2&& n)
	{
		cogs::assign(m_numerator, std::forward<numerator_t2>(n));
		cogs::assign(m_denominator, one_t());
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator=(numerator_t2& n)
	{
		cogs::assign(m_numerator, n);
		cogs::assign(m_denominator, one_t());
		return *this;
	}


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign(numerator_t2&& n, denominator_t2&& d)
	{
		cogs::assign(m_numerator, std::forward<numerator_t2>(n));
		cogs::assign(m_denominator, std::forward<denominator_t2>(d));
	}

	auto factor() const
	{
		auto gcdValue = cogs::gcd(m_numerator, m_denominator);
		return cogs::divide(cogs::divide_whole(m_numerator, gcdValue), cogs::divide_whole(m_denominator, gcdValue));
	}

	auto floor() const
	{
		return cogs::divide_whole(m_numerator, m_denominator);
	}

	void assign_floor()
	{
		cogs::assign_divide_whole(m_numerator, m_denominator);
		cogs::assign(m_denominator, one_t());
	}

	auto ceil() const
	{
		return cogs::divide_whole(m_numerator, m_denominator) + (has_fractional_part() ? 1 : 0);
	}

	void assign_ceil()
	{
		cogs::assign_divide_whole(m_numerator + (has_fractional_part() ? 1 : 0), m_denominator);
		cogs::assign(m_denominator, one_t());
	}

	auto fractional_part() const
	{
		return cogs::divide(cogs::modulo(m_numerator, m_denominator), m_denominator);
	}

	void assign_fractional_part() const
	{
		cogs::assign_modulo(m_numerator, m_denominator);
	}

	void clear()
	{
		cogs::clear(m_numerator);
		cogs::assign(m_denominator, one_t());
	}

	bool is_negative() const { return cogs::is_negative(m_numerator) != cogs::is_negative(m_denominator); }

	bool is_exponent_of_two() const
	{
		auto tmp = cogs::divide_whole_and_modulo(m_numerator, m_denominator);
		if (!!tmp.second)
			return false;
		return cogs::is_exponent_of_two(tmp.m_divide);
	}

	bool has_fractional_part() const
	{
		return cogs::modulo(m_numerator, m_denominator) != 0;
	}

	auto operator-() const
	{
		return cogs::divide(cogs::negative(m_numerator), m_denominator);
	}

	void assign_negative()
	{
		if (is_signed_v<numerator_t>)
			cogs::assign_negative(m_numerator);
		else if (is_signed_v<denominator_t>)
			cogs::assign_negative(m_denominator);
	}

	auto abs() const
	{
		typename fraction<
			decltype(cogs::abs(std::declval<numerator_t>())),
			decltype(cogs::abs(std::declval<denominator_t>()))
		>::simplified_t result;
		if (is_negative(m_numerator))
		{
			if (is_negative(m_denominator))
				result = cogs::divide(cogs::abs(m_numerator), cogs::abs(m_denominator));
			else
				result = cogs::divide(cogs::abs(m_numerator), m_denominator);
		}
		else if (is_negative(m_denominator))
			result = cogs::divide(m_numerator, cogs::abs(m_denominator));
		else
			result = cogs::divide(m_numerator, m_denominator);
		return result;
	}

	void assign_abs()
	{
		cogs::assign_abs(m_numerator);
		cogs::assign_abs(m_denominator);
	}

	auto next()
	{
		return cogs::divide(cogs::add(m_numerator, m_denominator), m_denominator);
	}

	void assign_next()
	{
		cogs::assign_add(m_numerator, m_denominator);
	}

	auto prev()
	{
		return cogs::divide(cogs::subtract(m_numerator, m_denominator), m_denominator);
	}


	void assign_prev()
	{
		cogs::assign_subtract(m_numerator, m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator==(numerator_t2&& n) const
	{
		return cogs::equals(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator==(const numerator_t2& n) const
	{
		return cogs::equals(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator==(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::equals(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator==(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::equals(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator!=(numerator_t2&& n) const
	{
		return cogs::not_equals(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator!=(const numerator_t2& n) const
	{
		return cogs::not_equals(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator!=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::not_equals(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator!=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::not_equals(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	int compare(numerator_t2&& n) const
	{
		return cogs::compare(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	int compare(const numerator_t2& n) const
	{
		return cogs::compare(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	int compare(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::compare(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	int compare(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::compare(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator<(numerator_t2&& n) const
	{
		return cogs::is_less_than(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator<(const numerator_t2& n) const
	{
		return cogs::is_less_than(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator<(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::is_less_than(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator<(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::is_less_than(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator<=(numerator_t2&& n) const
	{
		return cogs::is_less_than_or_equal(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator<=(const numerator_t2& n) const
	{
		return cogs::is_less_than_or_equal(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator<=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::is_less_than_or_equal(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator<=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::is_less_than_or_equal(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator>(numerator_t2&& n) const
	{
		return cogs::is_greater_than(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator>(const numerator_t2& n) const
	{
		return cogs::is_greater_than(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator>(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::is_greater_than(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator>(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::is_greater_than(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	bool operator>=(numerator_t2&& n) const
	{
		return cogs::is_greater_than_or_equal(m_numerator, cogs::multiply(std::move(n), m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	bool operator>=(const numerator_t2& n) const
	{
		return cogs::is_greater_than_or_equal(m_numerator, cogs::multiply(n, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator>=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::is_greater_than_or_equal(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(src.m_numerator, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	bool operator>=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::is_greater_than_or_equal(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(std::move(src.m_numerator), m_denominator));
	}


	// add
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto operator+(numerator_t2&& n) const
	{
		return cogs::divide(cogs::add(m_numerator, cogs::multiply(m_denominator, std::move(n))), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto operator+(const numerator_t2& n) const
	{
		return cogs::divide(cogs::add(m_numerator, cogs::multiply(m_denominator, n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator+(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::divide(cogs::add(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator)), cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator+(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto denom(cogs::multiply(m_denominator, src.m_denominator));
		return cogs::divide(
			cogs::add(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))),
			std::move(denom));
	}

	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	this_t& operator+=(numerator_t2&& n) const
	{
		cogs::assign_add(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator+=(const numerator_t2& n) const
	{
		cogs::assign_add(m_numerator, cogs::multiply(m_denominator, n));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator+=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator, cogs::add(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign_multiply(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator+=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator, 
			cogs::add(
				std::move(tmp), 
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
		return *this;
	}


	// subtract
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto operator-(numerator_t2&& n) const
	{
		return cogs::divide(cogs::subtract(m_numerator, cogs::multiply(m_denominator, std::move(n))), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto operator-(const numerator_t2& n) const
	{
		return cogs::divide(cogs::subtract(m_numerator, cogs::multiply(m_denominator, n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator-(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::divide(
			cogs::subtract(
				cogs::multiply(m_numerator, src.m_denominator), 
				cogs::multiply(m_denominator, src.m_numerator)), 
			cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator-(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto denom(cogs::multiply(m_denominator, src.m_denominator));
		return cogs::divide(
			cogs::subtract(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))),
			std::move(denom));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	this_t& operator-=(numerator_t2&& n) const
	{
		cogs::assign_subtract(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator-=(const numerator_t2& n) const
	{
		cogs::assign_subtract(m_numerator, cogs::multiply(m_denominator, n));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator-=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator, 
			cogs::subtract(
				cogs::multiply(m_numerator, src.m_denominator), 
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign_multiply(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator-=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator, 
			cogs::subtract(
				std::move(tmp), 
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
		return *this;
	}


	// inverse_subtract
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto inverse_subtract(numerator_t2&& n) const
	{
		return cogs::divide(cogs::inverse_subtract(m_numerator, cogs::multiply(m_denominator, std::move(n))), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto inverse_subtract(const numerator_t2& n) const
	{
		return cogs::divide(cogs::inverse_subtract(m_numerator, cogs::multiply(m_denominator, n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_subtract(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide(
			cogs::inverse_subtract(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)),
			cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_subtract(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto denom(cogs::multiply(m_denominator, src.m_denominator));
		return cogs::divide(
			cogs::inverse_subtract(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))),
			std::move(denom));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_inverse_subtract(numerator_t2&& n) const
	{
		cogs::assign_inverse_subtract(m_numerator, cogs::multiply(m_denominator, std::move(n)));
	}

	template <typename numerator_t2 = numerator_t>
	void assign_inverse_subtract(const numerator_t2& n) const
	{
		cogs::assign_inverse_subtract(m_numerator, cogs::multiply(m_denominator, n));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_subtract(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::inverse_subtract(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign_multiply(m_denominator, src.m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_subtract(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator,
			cogs::inverse_subtract(
				std::move(tmp),
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
	}



	// multiply
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto operator*(numerator_t2&& n) const
	{
		return cogs::divide(cogs::multiply(m_numerator, std::move(n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto operator*(const numerator_t2& n) const
	{
		return cogs::divide(cogs::multiply(m_numerator, n), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator*(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::divide(cogs::multiply(m_numerator, src.m_numerator), cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator*(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::divide(cogs::multiply(m_numerator, std::move(src.m_numerator)), cogs::multiply(m_denominator, std::move(src.m_denominator)));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	this_t& operator*=(numerator_t2&& n) const
	{
		cogs::assign_multiply(m_numerator, std::move(n));
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator*=(const numerator_t2& n) const
	{
		cogs::assign_multiply(m_numerator, n);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator*=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign_multiply(m_numerator, src.m_numerator);
		cogs::assign_multiply(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator*=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		cogs::assign_multiply(m_numerator, std::move(src.m_numerator));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
		return *this;
	}


	// reciprocal
	auto reciprocal() const
	{
		return cogs::divide(m_denominator, m_numerator);
	}

	// assign_reciprocal
	void assign_reciprocal() const
	{
		numerator_t tmp(m_numerator);
		cogs::assign(m_numerator, m_denominator);
		cogs::assign(m_denominator, std::move(tmp));
	}


	// divide
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto operator/(numerator_t2&& n) const
	{
		return cogs::divide(m_numerator, cogs::multiply(m_denominator, std::move(n)));
	}

	template <typename numerator_t2 = numerator_t>
	auto operator/(const numerator_t2& n) const
	{
		return cogs::divide(m_numerator, cogs::multiply(m_denominator, n));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator/(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::divide(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator/(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		return cogs::divide(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(m_denominator, std::move(src.m_numerator)));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	this_t& operator/=(numerator_t2&& n) const
	{
		cogs::assign_multiply(m_denominator, std::move(n));
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator/=(const numerator_t2& n) const
	{
		cogs::assign_multiply(m_denominator, n);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator/=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign_multiply(m_numerator, src.m_denominator);
		cogs::assign_multiply(m_denominator, src.m_numerator);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator/=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		cogs::assign_multiply(m_numerator, std::move(src.m_denominator));
		cogs::assign_multiply(m_denominator, std::move(src.m_numerator));
		return *this;
	}


	// inverse_divide
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto inverse_divide(numerator_t2&& n) const
	{
		return cogs::divide(cogs::multiply(m_denominator, std::move(n)), m_numerator);
	}

	template <typename numerator_t2 = numerator_t>
	auto inverse_divide(const numerator_t2& n) const
	{
		return cogs::divide(cogs::multiply(m_denominator, n), m_numerator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_divide(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide(cogs::multiply(m_denominator, src.m_numerator), cogs::multiply(m_numerator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_divide(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		return cogs::divide(cogs::multiply(m_denominator, std::move(src.m_numerator)), cogs::multiply(m_numerator, std::move(src.m_denominator)));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_inverse_divide(numerator_t2&& n) const
	{
		numerator_t tmp(m_numerator);
		cogs::assign(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		cogs::assign(m_denominator, std::move(tmp));
	}

	template <typename numerator_t2 = numerator_t>
	void assign_inverse_divide(const numerator_t2& n) const
	{
		numerator_t tmp(m_numerator);
		cogs::assign(m_numerator, cogs::multiply(m_denominator, n));
		cogs::assign(m_denominator, std::move(tmp));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_divide(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		numerator_t tmp(m_numerator);
		cogs::assign(m_numerator, cogs::multiply(m_denominator, src.m_numerator));
		cogs::assign(m_denominator, cogs::multiply(std::move(tmp), src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_divide(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		numerator_t tmp(m_numerator);
		cogs::assign(m_numerator, cogs::multiply(m_denominator, std::move(src.m_numerator)));
		cogs::assign(m_denominator, cogs::multiply(std::move(tmp), std::move(src.m_denominator)));
	}


	// divide_whole
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto divide_whole(numerator_t2&& n) const
	{
		return cogs::divide_whole(m_numerator, cogs::multiply(m_denominator, std::move(n)));
	}

	template <typename numerator_t2 = numerator_t>
	auto divide_whole(const numerator_t2& n) const
	{
		return cogs::divide_whole(m_numerator, cogs::multiply(m_denominator, n));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide_whole(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		return cogs::divide_whole(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(m_denominator, std::move(src.m_numerator)));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_divide_whole(numerator_t2&& n) const
	{
		cogs::assign_divide_whole(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t>
	void assign_divide_whole(const numerator_t2& n) const
	{
		cogs::assign_divide_whole(m_numerator, cogs::multiply(m_denominator, n));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_divide_whole(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::divide_whole( 
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_divide_whole(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		cogs::assign(m_numerator,
			cogs::divide_whole(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign(m_denominator, one_t());
	}



	// inverse_divide_whole
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto inverse_divide_whole(numerator_t2&& n) const
	{
		return cogs::inverse_divide_whole(m_numerator, cogs::multiply(m_denominator, std::move(n)));
	}

	template <typename numerator_t2 = numerator_t>
	auto inverse_divide_whole(const numerator_t2& n) const
	{
		return cogs::inverse_divide_whole(m_numerator, cogs::multiply(m_denominator, n));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_divide_whole(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::inverse_divide_whole(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_divide_whole(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		return cogs::inverse_divide_whole(cogs::multiply(m_numerator, std::move(src.m_denominator)), cogs::multiply(m_denominator, std::move(src.m_numerator)));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_inverse_divide_whole(numerator_t2&& n) const
	{
		cogs::assign_inverse_divide_whole(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t>
	void assign_inverse_divide_whole(const numerator_t2& n) const
	{
		cogs::assign_inverse_divide_whole(m_numerator, cogs::multiply(m_denominator, n));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_divide_whole(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::inverse_divide_whole(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign(m_denominator, one_t());
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_divide_whole(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		cogs::assign(m_numerator,
			cogs::inverse_divide_whole(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign(m_denominator, one_t());
	}


	// modulo
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto operator%(numerator_t2&& n) const
	{
		return cogs::divide(cogs::modulo(m_numerator, cogs::multiply(m_denominator, std::move(n))), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto operator%(const numerator_t2& n) const
	{
		return cogs::divide(cogs::modulo(m_numerator, cogs::multiply(m_denominator, n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator%(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		return cogs::divide(
			cogs::modulo(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)),
			cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto operator%(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto denom(cogs::multiply(m_denominator, src.m_denominator));
		return cogs::divide(
			cogs::modulo(
				cogs::multiply(m_numerator, std::move(src.m_denominator)), 
				cogs::multiply(m_denominator, std::move(src.m_numerator))), 
			std::move(denom));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	this_t& operator%=(numerator_t2&& n) const
	{
		cogs::assign_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		return *this;
	}

	template <typename numerator_t2 = numerator_t>
	this_t& operator%=(const numerator_t2& n) const
	{
		cogs::assign_modulo(m_numerator, cogs::multiply(m_denominator, n));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator%=(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator, 
			cogs::modulo(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign_multiply(m_denominator, src.m_denominator);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator%=(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator,
			cogs::modulo(
				std::move(tmp),
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
		return *this;
	}



	// inverse_modulo
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto inverse_modulo(numerator_t2&& n) const
	{
		return cogs::divide(cogs::inverse_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n))), m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	auto inverse_modulo(const numerator_t2& n) const
	{
		return cogs::divide(cogs::inverse_modulo(m_numerator, cogs::multiply(m_denominator, n)), m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_modulo(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide(
			cogs::inverse_modulo(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)),
			cogs::multiply(m_denominator, src.m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto inverse_modulo(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto denom(cogs::multiply(m_denominator, src.m_denominator));
		return cogs::divide(
			cogs::inverse_modulo(
				cogs::multiply(m_numerator, std::move(src.m_denominator)),
				cogs::multiply(m_denominator, std::move(src.m_numerator))),
			std::move(denom));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_inverse_modulo(numerator_t2&& n) const
	{
		cogs::assign_inverse_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n)));
	}

	template <typename numerator_t2 = numerator_t>
	void assign_inverse_modulo(const numerator_t2& n) const
	{
		cogs::assign_inverse_modulo(m_numerator, cogs::multiply(m_denominator, n));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_modulo(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::inverse_modulo(
				cogs::multiply(m_numerator, src.m_denominator),
				cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign_multiply(m_denominator, src.m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_inverse_modulo(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator,
			cogs::inverse_modulo(
				std::move(tmp),
				cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
	}








	// divide_whole_and_modulo
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto divide_whole_and_modulo(numerator_t2&& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n))));
		return std::make_pair(divmod.first, cogs::divide(divmod.second, m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	auto divide_whole_and_modulo(const numerator_t2& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, n)));
		return std::make_pair(divmod.first, cogs::divide(divmod.second, m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole_and_modulo(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator)));
		return make_pair(divmod.first, cogs::divide(divmod.second, cogs::multiply(m_denominator, src.m_denominator)));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole_and_modulo(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, std::move(src.m_numerator))));
		return make_pair(divmod.first, cogs::divide(divmod.second, cogs::multiply(m_denominator, std::move(src.m_denominator))));
	}


	// divide_whole_and_assign_modulo
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto divide_whole_and_assign_modulo(numerator_t2&& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n))));
		cogs::assign(m_numerator, divmod.second);
		return divmod.first;
	}

	template <typename numerator_t2 = numerator_t>
	auto divide_whole_and_assign_modulo(const numerator_t2& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, n)));
		cogs::assign(m_numerator, divmod.second);
		return divmod.first;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole_and_assign_modulo(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign(m_numerator, divmod.second);
		cogs::assign_multiply(m_denominator, src.m_denominator);
		return divmod.first;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto divide_whole_and_assign_modulo(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign(m_numerator, divmod.second);
		cogs::assign_multiply(m_denominator, std::move(src.m_denominator));
		return divmod.first;
	}


	// modulo_and_assign_divide_whole
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto modulo_and_assign_divide_whole(numerator_t2&& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, std::move(n))));
		cogs::assign(m_numerator, divmod.first);
		auto result(cogs::divide(divmod.second, m_denominator));
		cogs::assign(m_denominator, one_t());
		return result;
	}

	template <typename numerator_t2 = numerator_t>
	auto modulo_and_assign_divide_whole(const numerator_t2& n) const
	{
		auto divmod(cogs::divide_whole_and_modulo(m_numerator, cogs::multiply(m_denominator, n)));
		cogs::assign(m_numerator, divmod.first);
		auto result(cogs::divide(divmod.second, m_denominator));
		cogs::assign(m_denominator, one_t());
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto modulo_and_assign_divide_whole(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, src.m_numerator)));
		cogs::assign(m_numerator, divmod.first);
		auto result(cogs::divide(divmod.second, cogs::multiply(m_denominator, src.m_denominator)));
		cogs::assign(m_denominator, one_t());
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto modulo_and_assign_divide_whole(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto divmod(cogs::divide_whole_and_modulo(cogs::multiply(m_numerator, src.m_denominator), cogs::multiply(m_denominator, std::move(src.m_numerator))));
		cogs::assign(m_numerator, divmod.first);
		auto result(cogs::divide(divmod.second, cogs::multiply(m_denominator, std::move(src.m_denominator))));
		cogs::assign(m_denominator, one_t());
		return result;
	}

	// gcd
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto gcd(numerator_t2&& n) const
	{
		return cogs::divide(cogs::gcd(m_numerator, cogs::multiply(m_denominator, std::move(n))), cogs::abs(m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	auto gcd(const numerator_t2& n) const
	{
		return cogs::divide(cogs::gcd(m_numerator, cogs::multiply(m_denominator, n)), cogs::abs(m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto gcd(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide(
			cogs::gcd(
				cogs::multiply(src.m_numerator, m_denominator),
				cogs::multiply(m_numerator, src.m_denominator)), 
			cogs::abs(
				cogs::multiply(m_denominator, src.m_denominator)));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto gcd(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		return cogs::divide(
			cogs::gcd(
				cogs::multiply(m_denominator, std::move(src.m_numerator)),
				std::move(tmp)),
			cogs::abs(
				cogs::multiply(m_denominator, std::move(src.m_denominator))));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_gcd(numerator_t2&& n) const
	{
		cogs::assign_gcd(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		cogs::assign_abs(m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	void assign_gcd(const numerator_t2& n) const
	{
		cogs::assign_gcd(m_numerator, cogs::multiply(m_denominator, n));
		cogs::assign_abs(m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_gcd(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::gcd(
				cogs::multiply(m_denominator, src.m_numerator),
				cogs::multiply(m_numerator, src.m_denominator)));
		cogs::assign(m_denominator,
			cogs::abs(
				cogs::multiply(m_denominator, src.m_denominator)));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_gcd(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator,
			cogs::gcd(
				cogs::multiply(m_denominator, std::move(src.m_numerator)),
				std::move(tmp)));
		cogs::assign(m_denominator,
			cogs::abs(
				cogs::multiply(m_denominator, std::move(src.m_denominator))));
	}

	// lcm
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto lcm(numerator_t2&& n) const
	{
		return cogs::divide(cogs::lcm(m_numerator, cogs::multiply(m_denominator, std::move(n))), cogs::abs(m_denominator));
	}

	template <typename numerator_t2 = numerator_t>
	auto lcm(const numerator_t2& n) const
	{
		return cogs::divide(cogs::lcm(m_numerator, cogs::multiply(m_denominator, n)), cogs::abs(m_denominator));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto lcm(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		return cogs::divide(
			cogs::lcm(
				cogs::multiply(src.m_numerator, m_denominator),
				cogs::multiply(m_numerator, src.m_denominator)),
			cogs::abs(
				cogs::multiply(m_denominator, src.m_denominator)));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto lcm(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		return cogs::divide(
			cogs::lcm(
				cogs::multiply(m_denominator, std::move(src.m_numerator)),
				std::move(tmp)),
			cogs::abs(
				cogs::multiply(m_denominator, std::move(src.m_denominator))));
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_lcm(numerator_t2&& n) const
	{
		cogs::assign_lcm(m_numerator, cogs::multiply(m_denominator, std::move(n)));
		cogs::assign_abs(m_denominator);
	}

	template <typename numerator_t2 = numerator_t>
	void assign_lcm(const numerator_t2& n) const
	{
		cogs::assign_lcm(m_numerator, cogs::multiply(m_denominator, n));
		cogs::assign_abs(m_denominator);
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_lcm(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		cogs::assign(m_numerator,
			cogs::lcm(
				cogs::multiply(m_denominator, src.m_numerator),
				cogs::multiply(m_numerator, src.m_denominator)));
		cogs::assign(m_denominator,
			cogs::abs(
				cogs::multiply(m_denominator, src.m_denominator)));
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_lcm(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		auto tmp(cogs::multiply(m_numerator, src.m_denominator));
		cogs::assign(m_numerator,
			cogs::lcm(
				cogs::multiply(m_denominator, std::move(src.m_numerator)),
				std::move(tmp)));
		cogs::assign(m_denominator,
			cogs::abs(
				cogs::multiply(m_denominator, std::move(src.m_denominator))));
	}

	// greater
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto greater(numerator_t2&& n) const
	{
		typedef decltype(cogs::greater(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, one_t> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this > n)
			result = *this;
		else
			result = std::move(n);
		return result;
	}

	template <typename numerator_t2 = numerator_t>
	auto greater(const numerator_t2& n) const
	{
		typedef decltype(cogs::greater(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, one_t> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this > n)
			result = *this;
		else
			result = n;
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto greater(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		typedef decltype(cogs::greater(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, denominator_t2> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this > src)
			result = *this;
		else
			result = src;
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto greater(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		typedef decltype(cogs::greater(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, denominator_t2> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this > src)
			result = *this;
		else
			result = std::move(src);
		return result;
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_greater(numerator_t2&& n) const
	{
		if (n > *this)
			*this = std::move(n);
	}

	template <typename numerator_t2 = numerator_t>
	void assign_greater(const numerator_t2& n) const
	{
		if (n > *this)
			*this = n;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_greater(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		if (src > *this)
			*this = src;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_greater(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		if (src > *this)
			*this = std::move(src);
	}


	// lesser
	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	auto lesser(numerator_t2&& n) const
	{
		typedef decltype(cogs::lesser(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, one_t> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this < n)
			result = *this;
		else
			result = std::move(n);
		return result;
	}

	template <typename numerator_t2 = numerator_t>
	auto lesser(const numerator_t2& n) const
	{
		typedef decltype(cogs::lesser(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, one_t> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this < n)
			result = *this;
		else
			result = n;
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto lesser(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		typedef decltype(cogs::lesser(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, denominator_t2> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this < src)
			result = *this;
		else
			result = src;
		return result;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	auto lesser(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		typedef decltype(cogs::lesser(std::declval<numerator_t>(), std::declval<numerator_t2>())) new_numerator_t;
		typedef compatible_t<denominator_t, denominator_t2> new_denominator_t;
		fraction<new_numerator_t, new_denominator_t> result;
		if (*this < src)
			result = *this;
		else
			result = std::move(src);
		return result;
	}


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_reference_v<numerator_t2> > >
	void assign_lesser(numerator_t2&& n) const
	{
		if (n < *this)
			*this = std::move(n);
	}

	template <typename numerator_t2 = numerator_t>
	void assign_lesser(const numerator_t2& n) const
	{
		if (n < *this)
			*this = n;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_lesser(const fraction_content<numerator_t2, denominator_t2>& src) const
	{
		if (src < *this)
			*this = src;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign_lesser(fraction_content<numerator_t2, denominator_t2>&& src) const
	{
		if (src < *this)
			*this = std::move(src);
	}




};


template <typename numerator_type, typename denominator_type>
class is_const_type<fraction_content<numerator_type, denominator_type> >
{
public:
	static constexpr bool value = is_const_type_v<numerator_type> && is_const_type_v<denominator_type>;
};


/// @ingroup Math
/// @brief A fraction type comprosed of a numerator type and a denominator type
/// @tparam numerator_t Data type to use as a numerator
/// @tparam denominator_t Data type to use as a denominator
template <typename numerator_type, typename denominator_type = numerator_type>
class fraction
{
public:
	typedef numerator_type numerator_t;
	typedef denominator_type denominator_t;

	typedef fraction<numerator_t, denominator_t> this_t;

	template <typename, typename>
	friend class fraction;

private:
	static constexpr bool has_sign = is_signed_v<numerator_t> || is_signed_v<denominator_t>;

	static_assert(!std::is_same_v<denominator_type, fixed_integer_native_const<false, 0, 0> >);
	static_assert(!std::is_reference_v<numerator_type>);
	static_assert(!std::is_reference_v<denominator_type>);
	static_assert(!std::is_volatile_v<numerator_type>);
	static_assert(!std::is_volatile_v<denominator_type>);

	template <typename numerator_t2, typename denominator_t2, typename enable = void>
	class simplification_helper
	{
	public:
		typedef fraction<numerator_t2, denominator_t2> type;
		static constexpr bool is_fraction = true;
		static constexpr bool is_simplifiable = false;
	};
	template <typename numerator_t2, typename denominator_t2>
	using simplification_helper_t = typename simplification_helper<numerator_t2, denominator_t2>::type;

	template <typename numerator_t2, bool has_sign, size_t bits>
	class simplification_helper<
		numerator_t2, 
		fixed_integer_native_const<has_sign, bits, 1> >
	{
	public:
		typedef numerator_t2 type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, typename denominator_t2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, 0>, 
		denominator_t2>
	{
	public:
		typedef fixed_integer_native_const<false, 0, 0> type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, bool has_sign2, size_t bits2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, 0>,
		fixed_integer_native_const<has_sign2, bits2, 1> >
	{
	public:
		typedef fixed_integer_native_const<false, 0, 0> type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, size_t bits2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>,
		fixed_integer_native_const<true, bits2, -1>,
		std::enable_if_t<fixed_integer_native_const<has_sign, bits, value>::is_negative>
	>
	{
	public:
		typedef decltype(cogs::negative(std::declval<fixed_integer_native_const<has_sign, bits, value> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};
	
	template <bool has_sign, size_t bits, ulongest... values, size_t bits2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>,
		fixed_integer_native_const<true, bits2, -1>,
		std::enable_if_t<fixed_integer_extended_const<has_sign, bits, values...>::is_negative>
	>
	{
	public:
		typedef decltype(cogs::negative(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	// If the fraction is with consts, and there is no remainder
	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper <
		fixed_integer_native_const<has_sign, bits, value>,
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t <
			value2 != 1
			&& std::is_same_v<
				decltype(cogs::modulo(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
		>
	>
	{
	public:
		typedef decltype(cogs::divide_whole(
			std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
			std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>, 
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t<
			value2 != 1
			&& std::is_same_v<
				decltype(cogs::modulo(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
		>
	>
	{
	public:
		typedef decltype(cogs::divide_whole(
			std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
			std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>, 
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			std::is_same_v<
				decltype(cogs::modulo(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
		>
	>
	{
	public:
		typedef decltype(cogs::divide_whole(
			std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
			std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>,
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			std::is_same_v<
				decltype(cogs::modulo(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
		>
	>
	{
	public:
		typedef decltype(cogs::divide_whole(
			std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
			std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())) type;
		static constexpr bool is_fraction = false;
		static constexpr bool is_simplifiable = true;
	};


	// If the fraction is with consts, and there is a remainder, but a gcd other than 1
	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>, 
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t<
			value2 != 1
			&& !std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& !std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(), 
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 1, 1>
			>
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::divide_whole(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), cogs::gcd(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >()))),
			decltype(cogs::divide_whole(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >(), cogs::gcd(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>, 
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t<
			value2 != 1
			&& !std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& !std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 1, 1>
			>
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::divide_whole(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), cogs::gcd(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >()))),
			decltype(cogs::divide_whole(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >(), cogs::gcd(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>, 
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			!std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& !std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 1, 1>
			>
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::divide_whole(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), cogs::gcd(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >()))),
			decltype(cogs::divide_whole(std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >(), cogs::gcd(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>,
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			!std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& !std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 1, 1>
			>
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::divide_whole(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), cogs::gcd(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >()))),
			decltype(cogs::divide_whole(std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >(), cogs::gcd(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	// If the fraction is with consts, and it cannot be otherwise simplified, but both values are negative.  Make both positive.
	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>, 
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t<
			value2 != 1
			&& !std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& std::is_same_v<
				decltype(cogs::gcd(std::declval<fixed_integer_native_const<has_sign, bits, value> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 1, 1>
			>
			&& (fixed_integer_native_const<has_sign, bits, value>::is_negative)
			&& (fixed_integer_native_const<has_sign2, bits2, value2>::is_negative)
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::negative(std::declval<fixed_integer_native_const<has_sign, bits, value> >())),
			decltype(cogs::negative(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >()))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>, 
		fixed_integer_native_const<has_sign2, bits2, value2>,
		std::enable_if_t<
			value2 != 1
			&& !std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& std::is_same_v<
				decltype(cogs::gcd(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(), std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >())),
				fixed_integer_native_const<false, 1, 1>
			>
			&& (fixed_integer_extended_const<has_sign, bits, values...>::is_negative)
			&& (fixed_integer_native_const<has_sign2, bits2, value2>::is_negative)
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::negative(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >())),
			decltype(cogs::negative(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >()))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_native_const<has_sign, bits, value>, 
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			!std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_native_const<has_sign, bits, value> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 1, 1>
			>
			&& (fixed_integer_native_const<has_sign, bits, value>::is_negative)
			&& (fixed_integer_extended_const<has_sign2, bits2, values2...>::is_negative)
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::negative(std::declval<fixed_integer_native_const<has_sign, bits, value> >())),
			decltype(cogs::negative(std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >()))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

	template <bool has_sign, size_t bits, ulongest... values, bool has_sign2, size_t bits2, ulongest... values2>
	class simplification_helper<
		fixed_integer_extended_const<has_sign, bits, values...>,
		fixed_integer_extended_const<has_sign2, bits2, values2...>,
		std::enable_if_t<
			!std::is_same_v<
				decltype(cogs::modulo(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 0, 0>
			>
			&& std::is_same_v<
				decltype(cogs::gcd(
					std::declval<fixed_integer_extended_const<has_sign, bits, values...> >(),
					std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >())),
				fixed_integer_native_const<false, 1, 1>
			>
			&& (fixed_integer_extended_const<has_sign, bits, values...>::is_negative)
			&& (fixed_integer_extended_const<has_sign2, bits2, values2...>::is_negative)
		>
	>
	{
	public:
		typedef typename fraction<
			decltype(cogs::negative(std::declval<fixed_integer_extended_const<has_sign, bits, values...> >())),
			decltype(cogs::negative(std::declval<fixed_integer_extended_const<has_sign2, bits2, values2...> >()))
		>::simplified_t inner_simplified_t;
		typedef typename inner_simplified_t::type type;
		static constexpr bool is_fraction = inner_simplified_t::is_fraction;
		static constexpr bool is_simplifiable = true;
	};

public:
	typedef simplification_helper_t<numerator_t, denominator_t> simplified_t;

	static constexpr bool is_simplified_type = !simplification_helper<numerator_t, denominator_t>::is_simplifiable;

private:
	typedef fraction_content<numerator_t, denominator_t> content_t;
	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

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

	template <typename T, typename = std::enable_if_t<!std::is_const_v<T> && !std::is_volatile_v<T> > >
	static T&& simplify_type(T&& f) { return std::forward<T>(f); }

	template <typename T>
	static T& simplify_type(T& f) { return f; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		fraction<numerator_t2, denominator_t2>&&>
	simplify_type(fraction<numerator_t2, denominator_t2>&& f) { return std::move(f); }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction<numerator_t2, denominator_t2>&>
	simplify_type(fraction<numerator_t2, denominator_t2>& f) { return f; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction<numerator_t2, denominator_t2>&>
	simplify_type(const fraction<numerator_t2, denominator_t2>& f) { return f; }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const volatile fraction<numerator_t2, denominator_t2>&>
	simplify_type(volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return *rt; }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const volatile fraction<numerator_t2, denominator_t2>&>
	simplify_type(const volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return *rt; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		numerator_t2&&>
	simplify_type(fraction<numerator_t2, denominator_t2>&& f) { return std::move(f.m_contents->m_numerator); }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_type(fraction<numerator_t2, denominator_t2>& f) { return f.m_contents->m_numerator; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_type(const fraction<numerator_t2, denominator_t2>& f) { return f.m_contents->m_numerator; }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_type(volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return rt->m_numerator; }
	
	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_type(const volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return rt->m_numerator; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_type(fraction<numerator_t2, denominator_t2>&& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_type(fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }
	
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_type(const fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_type(volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_type(const volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }



	template <typename T, typename = std::enable_if_t<!std::is_const_v<T> && !std::is_volatile_v<T> > >
	static T&& simplify_content_type(T&& f) { return std::forward<T>(f); }

	template <typename T>
	static T& simplify_content_type(T& f) { return f; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		fraction_content<numerator_t2, denominator_t2>&&>
	simplify_content_type(fraction<numerator_t2, denominator_t2>&& f) { return std::move(*(f.m_contents)); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(fraction<numerator_t2, denominator_t2>& f) { return *(f.m_contents); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(const fraction<numerator_t2, denominator_t2>& f) { return *(f.m_contents); }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = begin_read()) { return *rt; }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(const volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = begin_read()) { return *rt; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		numerator_t2&&>
	simplify_content_type(fraction<numerator_t2, denominator_t2>&& f) { return std::move(f.m_contents->m_numerator); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(fraction<numerator_t2, denominator_t2>& f) { return f.m_contents->m_numerator; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(const fraction<numerator_t2, denominator_t2>& f) { return f.m_contents->m_numerator; }

	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return rt->m_numerator; }
	
	// Assumes the return type will not survive the duration of the expression containing the function call.  Do not forward using auto.
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(const volatile fraction<numerator_t2, denominator_t2>& f, const read_token& rt = read_token()) { rt = begin_read(); return rt->m_numerator; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(fraction<numerator_t2, denominator_t2>&& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }
	
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(const fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }
	
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(const volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(fraction<numerator_t2, denominator_t2>&& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(const fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(const volatile fraction<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }



	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		fraction_content<numerator_t2, denominator_t2>&&>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>&& f) { return std::move(f); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>& f) { return f; }
	
	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<numerator_t2, denominator_t2>::is_simplifiable,
		const fraction_content<numerator_t2, denominator_t2>&>
	simplify_content_type(const fraction_content<numerator_t2, denominator_t2>& f) { return f; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		numerator_t2&&>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>&& f) { return std::move(f.m_numerator); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>& f) { return f.m_numerator; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& !is_const_type_v<numerator_t2> && is_const_type_v<denominator_t2>,
		const numerator_t2&>
	simplify_content_type(const fraction_content<numerator_t2, denominator_t2>& f) { return f.m_numerator; }



	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>&& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t::content_t>
	simplify_content_type(const fraction_content<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t::content_t(); }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>&& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(fraction_content<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<numerator_t2, denominator_t2>::is_simplifiable
		&& !simplification_helper<numerator_t2, denominator_t2>::is_fraction
		&& is_const_type_v<numerator_t2>
		&& is_const_type_v<denominator_t2>,
		typename fraction<numerator_t2, denominator_t2>::simplified_t>
	simplify_content_type(const fraction_content<numerator_t2, denominator_t2>& f) { return fraction<numerator_t2, denominator_t2>::simplified_t(); }



	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::is_simplifiable,
		numerator_t2&&>
	simplify_numerator_type(numerator_t2&& n, const denominator_t2& d) { return std::forward<numerator_t2>(n); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::is_simplifiable
		&& !simplification_helper<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::is_fraction
		&& !is_const_type_v<std::remove_cv_t<std::remove_reference_t<numerator_t2> > >
		&& is_const_type_v<std::remove_cv_t<denominator_t2> >,
		numerator_t2&&>
	simplify_numerator_type(numerator_t2&& n, const denominator_t2& d) { return std::forward<numerator_t2>(n); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::is_simplifiable
		&& is_const_type_v<std::remove_cv_t<std::remove_reference_t<numerator_t2> > >
		&& is_const_type_v<std::remove_cv_t<denominator_t2> >,
		typename fraction<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::simplified_t::numerator_type>
	simplify_numerator_type(numerator_t2&& n, const denominator_t2& d) { return typename fraction<std::remove_cv_t<std::remove_reference_t<numerator_t2> >, std::remove_cv_t<denominator_t2> >::simplified_t::numerator_type(); }



	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		!simplification_helper<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::is_simplifiable,
		denominator_t2&&>
	simplify_denominator_type(const numerator_t2& n, denominator_t2&& d) { return std::forward<denominator_t2>(d); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::is_simplifiable
		&& !simplification_helper<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::is_fraction
		&& !is_const_type_v<std::remove_cv_t<numerator_t2> >
		&& is_const_type_v<std::remove_cv_t<std::remove_reference_t<denominator_t2> > >,
		one_t>
	simplify_denominator_type(const numerator_t2& n, denominator_t2&& d) { return one_t(); }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	static std::enable_if_t<
		simplification_helper<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::is_simplifiable
		&& is_const_type_v<std::remove_cv_t<numerator_t2> >
		&& is_const_type_v<std::remove_cv_t<std::remove_reference_t<denominator_t2> > >,
		typename fraction<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::simplified_t::denominator_type>
	simplify_denominator_type(const numerator_t2& n, denominator_t2&& d) { return typename fraction<std::remove_cv_t<numerator_t2>, std::remove_cv_t<std::remove_reference_t<denominator_t2> > >::simplified_t::denominator_type(); }



	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(fraction_content<numerator_t2, denominator_t2>&& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(std::move(src)))
	{ }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(const fraction_content<numerator_t2, denominator_t2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(src))
	{ }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(fraction_content<numerator_t2, denominator_t2>&& src)
	{
		*m_contents = simplify_content_type(std::move(src));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(fraction_content<numerator_t2, denominator_t2>&& src) volatile
	{
		m_contents.set(simplify_content_type(std::move(src)));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(const fraction_content<numerator_t2, denominator_t2>& src)
	{
		*m_contents = simplify_content_type(src);
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(const fraction_content<numerator_t2, denominator_t2>& src) volatile
	{
		m_contents.set(simplify_content_type(src));
		return *this;
	}

public:
	fraction() { }

	// Need superfluous non-const copy constructors and assignment operators to avoid this_t& matching T&& instead of const this_t&

	fraction(this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.m_contents) { }
	fraction(const this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.m_contents) { }
	fraction(volatile this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.begin_read()) { }
	fraction(const volatile this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.begin_read()) { }

	fraction(this_t&& src) : m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(std::move(src))) { }

	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2> > >
	fraction(numerator_t2&& n)
		: m_contents(typename transactable_t::construct_embedded_t(), std::forward<numerator_t2>(n), one_t())
	{ }

	template <typename numerator_t2 = numerator_t>
	fraction(numerator_t2& n)
		: m_contents(typename transactable_t::construct_embedded_t(), n, one_t())
	{ }


	// Hypothetically, this could do something to better represent the original value, if the destination type has an insufficient range

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(fraction<numerator_t2, denominator_t2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(src))
	{ }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(const fraction<numerator_t2, denominator_t2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(src))
	{ }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(volatile fraction<numerator_t2, denominator_t2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(src))
	{ }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(const volatile fraction<numerator_t2, denominator_t2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(src))
	{ }	


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(fraction<numerator_t2, denominator_t2>&& src)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_content_type(std::move(src)))
	{ }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	fraction(numerator_t2&& n, denominator_t2&& d)
		: m_contents(typename transactable_t::construct_embedded_t(), simplify_numerator_type(std::forward<numerator_t2>(n), std::forward<denominator_t2>(d)),
			simplify_denominator_type(std::forward<numerator_t2>(n), std::forward<denominator_t2>(d)))
	{ }

	this_t& operator=(this_t& src) { if (this != &src) *m_contents = *src.m_contents; return *this; }
	this_t& operator=(const this_t& src) { if (this != &src) *m_contents = *src.m_contents; return *this; }
	this_t& operator=(volatile this_t& src) { COGS_ASSERT(this != &src); *m_contents = *src.begin_read(); return *this; }
	this_t& operator=(const volatile this_t& src) { COGS_ASSERT(this != &src); *m_contents = *src.begin_read(); return *this; }

	volatile this_t& operator=(this_t& src) volatile { COGS_ASSERT(this != &src); m_contents.set(*src.m_contents); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { COGS_ASSERT(this != &src); m_contents.set(*src.m_contents); return *this; }
	volatile this_t& operator=(volatile this_t& src) volatile { if (this != &src) m_contents.set(*src.begin_read()); return *this; }
	volatile this_t& operator=(const volatile this_t& src) volatile { if (this != &src) m_contents.set(*src.begin_read()); return *this; }

	this_t& operator=(this_t&& src) { *m_contents = std::move(*src.m_contents); return *this; }
	volatile this_t& operator=(this_t&& src) volatile { m_contents.set(std::move(*src.m_contents)); return *this; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(fraction<numerator_t2, denominator_t2>& src) { *m_contents = simplify_content_type(src); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(const fraction<numerator_t2, denominator_t2>& src) { *m_contents = simplify_content_type(src); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(volatile fraction<numerator_t2, denominator_t2>& src) { *m_contents = simplify_content_type(src); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(const volatile fraction<numerator_t2, denominator_t2>& src) { *m_contents = simplify_content_type(src); return *this; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(fraction<numerator_t2, denominator_t2>& src) volatile { m_contents.set(simplify_content_type(src)); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(const fraction<numerator_t2, denominator_t2>& src) volatile { m_contents.set(simplify_content_type(src)); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(volatile fraction<numerator_t2, denominator_t2>& src) volatile { m_contents.set(simplify_content_type(src)); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(const volatile fraction<numerator_t2, denominator_t2>& src) volatile { m_contents.set(simplify_content_type(src)); return *this; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	this_t& operator=(fraction<numerator_t2, denominator_t2>&& src) { *m_contents = simplify_content_type(std::move(src)); return *this; }

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	volatile this_t& operator=(fraction<numerator_t2, denominator_t2>&& src) volatile { m_contents.set(simplify_content_type(std::move(src))); return *this; }


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t< std::is_reference_v<numerator_t2> || (!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2>) > >
	this_t& operator=(numerator_t2&& n) { m_contents->assign(std::move(n), one_t()); return *this; }

	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t< std::is_reference_v<numerator_t2> || (!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2>) > >
	volatile this_t& operator=(numerator_t2&& n) volatile { m_contents.set(std::move(n), one_t()); return *this; }


	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2> > >
	this_t& operator=(numerator_t2& n) { m_contents->assign(n, one_t()); return *this; }

	template <typename numerator_t2 = numerator_t, typename = std::enable_if_t<!std::is_const_v<numerator_t2> && !std::is_volatile_v<numerator_t2> > >
	volatile this_t& operator=(numerator_t2& n) volatile { m_contents.set(n, one_t()); return *this; }


	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign(numerator_t2&& n, denominator_t2&& d)
	{
		m_contents->assign(
			simplify_numerator_type(std::forward<numerator_t2>(n), d),
			simplify_denominator_type(n, std::forward<denominator_t2>(d)));
		return *this;
	}

	template <typename numerator_t2 = numerator_t, typename denominator_t2 = denominator_t>
	void assign(numerator_t2&& n, denominator_t2&& d) volatile
	{
		m_contents.set(
			simplify_numerator_type(std::forward<numerator_t2>(n), d),
			simplify_denominator_type(n, std::forward<denominator_t2>(d)));
		return *this;
	}

	bool operator!() const			{ return !get_numerator(); }
	bool operator!() const volatile	{ return !get_numerator(); }

	bool is_negative() const			{ return m_contents->is_negative(); }
	bool is_negative() const volatile	{ return begin_read()->is_negative(); }

	bool is_exponent_of_two() const { return cogs::is_exponent_of_two(simplify_content_type(*this)); }
	bool is_exponent_of_two() const volatile { return cogs::is_exponent_of_two(simplify_content_type(*this)); }

	bool has_fractional_part() const { return m_contents->has_fractional_part(); }
	bool has_fractional_part() const volatile { return begin_read()->has_fractional_part(); }

	// absolute
	auto abs() const					{ return simplify_type(cogs::abs(simplify_content_type(*this))); }
	auto abs() const volatile			{ return simplify_type(cogs::abs(simplify_content_type(*this))); }
	void assign_abs()					{ m_contents->assign_abs(); }
	void assign_abs() volatile			{ if (has_sign) write_retry_loop([&](content_t& c) { c.assign_abs(); }); }
	const this_t& pre_assign_abs()		{ if (has_sign) m_contents->assign_abs(); return *this; }
	this_t pre_assign_abs() volatile	{ if (has_sign) return write_retry_loop_pre([&](content_t& c) { c.assign_abs(); }); return *this; }
	this_t post_assign_abs()			{ if (has_sign) { this_t result(*this); if (has_sign) m_contents->assign_abs(); return result; } return *this;}
	this_t post_assign_abs() volatile	{ if (has_sign) return write_retry_loop_post([&](content_t& c) { c.assign_abs(); }); return *this; }

	auto operator-() const					{ return simplify_type(-*m_contents); }
	auto operator-() const volatile			{ return simplify_type(-(*begin_read())); }
	void assign_negative()					{ m_contents->assign_negative(); }
	void assign_negative() volatile			{ write_retry_loop([&](content_t& c) { c.assign_negative(); }); }
	const this_t& pre_assign_negative()		{ m_contents->assign_negative(); return *this; }
	this_t pre_assign_negative() volatile	{ return write_retry_loop_pre([&](content_t& c) { c.assign_negative(); }); }
	this_t post_assign_negative()			{ this_t result(*this); m_contents->assign_negative(); return result; }
	this_t post_assign_negative() volatile	{ return write_retry_loop_post([&](content_t& c) { c.assign_negative(); }); }


	auto next() const				{ return simplify_type(cogs::next(simplify_content_type(*this))); }
	auto next() const volatile		{ return simplify_type(cogs::next(simplify_content_type(*this))); }
	void assign_next()				{ m_contents->assign_next(); }
	void assign_next() volatile		{ write_retry_loop([&](content_t& c) { c.assign_next(); }); }
	const this_t& operator++()		{ m_contents->assign_next(); return *this; }
	this_t operator++() volatile	{ return write_retry_loop_pre([&](content_t& c) { c.assign_next(); }); }
	this_t operator++(int)			{ this_t result(*this); m_contents->assign_next(); return result; }
	this_t operator++(int) volatile	{ return write_retry_loop_post([&](content_t& c) { c.assign_next(); }); }

	
	auto prev() const				{ return simplify_type(cogs::prev(simplify_content_type(*this))); }
	auto prev() const volatile		{ return simplify_type(cogs::prev(simplify_content_type(*this))); }
	void assign_prev()				{ m_contents->assign_prev(); }
	void assign_prev() volatile		{ write_retry_loop([&](content_t& c) { c.assign_prev(); }); }
	const this_t& operator--()		{ m_contents->assign_prev(); return *this; }
	this_t operator--() volatile	{ return write_retry_loop_pre([&](content_t& c) { c.assign_prev(); }); }
	this_t operator--(int)			{ this_t result(*this); m_contents->assign_prev(); return result; }
	this_t operator--(int) volatile	{ return write_retry_loop_post([&](content_t& c) { c.assign_prev(); }); }


	// add
	template <typename T>
	auto operator+(T&& t) const
	{
		return simplify_type(cogs::add(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto operator+(T&& t) const volatile
	{
		return simplify_type(cogs::add(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& operator+=(T&& t)
	{
		m_contents->assign_add(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& operator+=(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_add(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_add(T&& t)
	{
		m_contents->assign_add(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_add(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_add(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_add(T&& t)
	{
		this_t result(*this);
		m_contents->assign_add(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_add(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_add(simplify_content_type(t)); });
	}


	// subtract
	template <typename T>
	auto operator-(T&& t) const
	{
		return simplify_type(cogs::subtract(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto operator-(T&& t) const volatile
	{
		return simplify_type(cogs::subtract(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& operator-=(T&& t)
	{
		m_contents->assign_subtract(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& operator-=(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_subtract(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_subtract(T&& t)
	{
		m_contents->assign_subtract(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_subtract(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_subtract(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_subtract(T&& t)
	{
		this_t result(*this);
		m_contents->assign_subtract(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_subtract(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_subtract(simplify_content_type(t)); });
	}


	// inverse_subtract
	template <typename T>
	auto inverse_subtract(T&& t) const
	{
		return simplify_type(cogs::inverse_subtract(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto inverse_subtract(T&& t) const volatile
	{
		return simplify_type(cogs::inverse_subtract(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	void assign_inverse_subtract(T&& t)
	{
		m_contents->assign_inverse_subtract(simplify_content_type(std::forward<T>(t)));
	}

	template <typename T>
	void assign_inverse_subtract (const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_inverse_subtract(simplify_content_type(t)); });
	}

	template <typename T>
	const this_t& pre_assign_inverse_subtract(T&& t)
	{
		m_contents->assign_inverse_subtract(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_inverse_subtract(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_inverse_subtract(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_inverse_subtract(T&& t)
	{
		this_t result(*this);
		m_contents->assign_inverse_subtract(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_inverse_subtract(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_inverse_subtract(simplify_content_type(t)); });
	}



	// multiply
	template <typename T>
	auto operator*(T&& t) const
	{
		return simplify_type(cogs::multiply(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto operator*(T&& t) const volatile
	{
		return simplify_type(cogs::multiply(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& operator*=(T&& t)
	{
		m_contents->assign_multiply(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& operator*=(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_multiply(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_multiply(T&& t)
	{
		m_contents->assign_multiply(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_multiply(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_multiply(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_multiply(T&& t)
	{
		this_t result(*this);
		m_contents->assign_multiply(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_multiply(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_multiply(simplify_content_type(t)); });
	}


	// modulo
	template <typename T>
	auto operator%(T&& t) const
	{
		return simplify_type(cogs::modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto operator%(T&& t) const volatile
	{
		return simplify_type(cogs::modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& operator%=(T&& t)
	{
		m_contents->assign_modulo(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& operator%=(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_modulo(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_modulo(T&& t)
	{
		m_contents->assign_modulo(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_modulo(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_modulo(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_modulo(T&& t)
	{
		this_t result(*this);
		m_contents->assign_modulo(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_modulo(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_modulo(simplify_content_type(t)); });
	}



	// inverse_modulo
	template <typename T>
	auto inverse_modulo(T&& t) const
	{
		return simplify_type(cogs::inverse_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto inverse_modulo(T&& t) const volatile
	{
		return simplify_type(cogs::inverse_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	void assign_inverse_modulo(T&& t)
	{
		m_contents->assign_inverse_modulo(simplify_content_type(std::forward<T>(t)));
	}

	template <typename T>
	void assign_inverse_modulo(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_inverse_modulo(simplify_content_type(t)); });
	}

	template <typename T>
	const this_t& pre_assign_inverse_modulo(T&& t)
	{
		m_contents->assign_inverse_modulo(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_inverse_modulo(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_inverse_modulo(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_inverse_modulo(T&& t)
	{
		this_t result(*this);
		m_contents->assign_inverse_modulo(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_inverse_modulo(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_inverse_modulo(simplify_content_type(t)); });
	}



	// divide
	template <typename T>
	auto operator/(T&& t) const
	{
		return simplify_type(cogs::divide(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto operator/(T&& t) const volatile
	{
		return simplify_type(cogs::divide(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& operator/=(T&& t)
	{
		m_contents->assign_divide(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& operator/=(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_divide(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_divide(T&& t)
	{
		m_contents->assign_divide(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_divide(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_divide(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_divide(T&& t)
	{
		this_t result(*this);
		m_contents->assign_divide(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_divide(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_divide(simplify_content_type(t)); });
	}

	// reciprocal
	auto reciprocal() const;
	auto reciprocal() const volatile;
	void assign_reciprocal()					{ m_contents->assign_reciprocal(); }
	void assign_reciprocal() volatile			{ write_retry_loop([&](content_t& c) { c.assign_reciprocal(); }); }
	const this_t& pre_assign_reciprocal()		{ m_contents->assign_reciprocal(); return *this; }
	this_t pre_assign_reciprocal() volatile		{ return write_retry_loop_pre([&](content_t& c) { c.assign_reciprocal(); }); }
	this_t post_assign_reciprocal()				{ this_t result(*this); m_contents->assign_reciprocal(); return result; }
	this_t post_assign_reciprocal() volatile	{ return write_retry_loop_post([&](content_t& c) { c.assign_reciprocal(); }); }

	// inverse_divide
	template <typename T>
	auto inverse_divide(T&& t) const
	{
		return simplify_type(cogs::inverse_divide(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto inverse_divide(T&& t) const volatile
	{
		return simplify_type(cogs::inverse_divide(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_inverse_divide(T&& t)
	{
		m_contents->assign_inverse_divide(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_inverse_divide(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_inverse_divide(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_inverse_divide(T&& t)
	{
		m_contents->assign_inverse_divide(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_inverse_divide(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_inverse_divide(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_inverse_divide(T&& t)
	{
		this_t result(*this);
		m_contents->assign_inverse_divide(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_inverse_divide(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_inverse_divide(simplify_content_type(t)); });
	}

	// floor
	auto floor() const { return cogs::floor(simplify_content_type(*this)); }
	auto floor() const volatile { return cogs::floor(simplify_content_type(*this)); }
	void assign_floor() { m_contents->assign_floor(); }
	void assign_floor() volatile { write_retry_loop([&](content_t& c) { c.assign_floor(); }); }
	const this_t& pre_assign_floor() { m_contents->assign_floor(); return *this; }
	this_t pre_assign_floor() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_floor(); }); }
	this_t post_assign_floor() { this_t result(*this); m_contents->assign_floor(); return result; }
	this_t post_assign_floor() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_floor(); }); }

	// ceil
	auto ceil() const { return cogs::ceil(simplify_content_type(*this)); }
	auto ceil() const volatile { return cogs::ceil(simplify_content_type(*this)); }
	void assign_ceil() { m_contents->assign_ceil(); }
	void assign_ceil() volatile { write_retry_loop([&](content_t& c) { c.assign_ceil(); }); }
	const this_t& pre_assign_ceil() { m_contents->assign_ceil(); return *this; }
	this_t pre_assign_ceil() volatile { return write_retry_loop_pre([&](content_t& c) { c.assign_ceil(); }); }
	this_t post_assign_ceil() { this_t result(*this); m_contents->assign_ceil(); return result; }
	this_t post_assign_ceil() volatile { return write_retry_loop_post([&](content_t& c) { c.assign_ceil(); }); }

	// fractional_part
	auto fractional_part() const					{ return cogs::fractional_part(simplify_content_type(*this)); }
	auto fractional_part() const volatile			{ return cogs::fractional_part(simplify_content_type(*this)); }
	void assign_fractional_part()					{ m_contents->assign_fractional_part(); }
	void assign_fractional_part() volatile			{ write_retry_loop([&](content_t& c) { c.assign_fractional_part(); }); }
	const this_t& pre_assign_fractional_part()		{ m_contents->assign_fractional_part(); return *this; }
	this_t pre_assign_fractional_part() volatile	{ return write_retry_loop_pre([&](content_t& c) { c.assign_fractional_part(); }); }
	this_t post_assign_fractional_part()			{ this_t result(*this); m_contents->assign_fractional_part(); return result; }
	this_t post_assign_fractional_part() volatile	{ return write_retry_loop_post([&](content_t& c) { c.assign_fractional_part(); }); }


	// divide_whole
	template <typename T>
	auto divide_whole(T&& t) const
	{
		return simplify_type(cogs::divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto divide_whole(T&& t) const volatile
	{
		return simplify_type(cogs::divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_divide_whole(T&& t)
	{
		m_contents->assign_divide_whole(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_divide_whole(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_divide_whole(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_divide_whole(T&& t)
	{
		m_contents->assign_divide_whole(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_divide_whole(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_divide_whole(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_divide_whole(T&& t)
	{
		this_t result(*this);
		m_contents->assign_divide_whole(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_divide_whole(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_divide_whole(simplify_content_type(t)); });
	}



	// inverse_divide_whole
	template <typename T>
	auto inverse_divide_whole(T&& t) const
	{
		return simplify_type(cogs::inverse_divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto inverse_divide_whole(T&& t) const volatile
	{
		return simplify_type(cogs::inverse_divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_inverse_divide_whole(T&& t)
	{
		m_contents->assign_inverse_divide_whole(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_inverse_divide_whole(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_inverse_divide_whole(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_inverse_divide_whole(T&& t)
	{
		m_contents->assign_inverse_divide_whole(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_inverse_divide_whole(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_inverse_divide_whole(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_inverse_divide_whole(T&& t)
	{
		this_t result(*this);
		m_contents->assign_inverse_divide_whole(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_inverse_divide_whole(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_inverse_divide_whole(simplify_content_type(t)); });
	}



	// divide_whole_and_modulo
	template <typename T>
	auto divide_whole_and_modulo(T&& t) const
	{
		auto result(cogs::divide_whole_and_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
		return make_pair(simplify_type(result.first), simplify_type(result.second));
	}

	template <typename T>
	auto divide_whole_and_modulo(T&& t) const volatile
	{
		auto result(cogs::divide_whole_and_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
		return make_pair(simplify_type(result.first), simplify_type(result.second));
	}

	// divide_whole_and_assign_modulo
	template <typename T>
	auto divide_whole_and_assign_modulo(T&& t)
	{
		return simplify_type(cogs::divide_whole_and_assign_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto divide_whole_and_assign_modulo(T&& t) volatile
	{
		return simplify_type(cogs::divide_whole_and_assign_modulo(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	// modulo_and_assign_divide_whole
	template <typename T>
	auto modulo_and_assign_divide_whole(T&& t)
	{
		return simplify_type(cogs::modulo_and_assign_divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto modulo_and_assign_divide_whole(T&& t) volatile
	{
		return simplify_type(cogs::modulo_and_assign_divide_whole(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}



	// gcd
	template <typename T>
	auto gcd(T&& t) const
	{
		return simplify_type(cogs::gcd(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto gcd(T&& t) const volatile
	{
		return simplify_type(cogs::gcd(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_gcd(T&& t)
	{
		m_contents->assign_gcd(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_gcd(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_gcd(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_gcd(T&& t)
	{
		m_contents->assign_gcd(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_gcd(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_gcd(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_gcd(T&& t)
	{
		this_t result(*this);
		m_contents->assign_gcd(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_gcd(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_gcd(simplify_content_type(t)); });
	}



	// lcm
	template <typename T>
	auto lcm(T&& t) const
	{
		return simplify_type(cogs::lcm(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto lcm(T&& t) const volatile
	{
		return simplify_type(cogs::lcm(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_lcm(T&& t)
	{
		m_contents->assign_lcm(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_lcm(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_lcm(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_lcm(T&& t)
	{
		m_contents->assign_lcm(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_lcm(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_lcm(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_lcm(T&& t)
	{
		this_t result(*this);
		m_contents->assign_lcm(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_lcm(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_lcm(simplify_content_type(t)); });
	}



	// greater
	template <typename T>
	auto greater(T&& t) const
	{
		return simplify_type(cogs::greater(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto greater(T&& t) const volatile
	{
		return simplify_type(cogs::greater(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_greater(T&& t)
	{
		m_contents->assign_greater(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_greater(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_greater(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_greater(T&& t)
	{
		m_contents->assign_greater(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_greater(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_greater(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_greater(T&& t)
	{
		this_t result(*this);
		m_contents->assign_greater(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_greater(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_greater(simplify_content_type(t)); });
	}


	// lesser
	template <typename T>
	auto lesser(T&& t) const
	{
		return simplify_type(cogs::lesser(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	auto lesser(T&& t) const volatile
	{
		return simplify_type(cogs::lesser(simplify_content_type(*this), simplify_content_type(std::forward<T>(t))));
	}

	template <typename T>
	this_t& assign_lesser(T&& t)
	{
		m_contents->assign_lesser(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	volatile this_t& assign_lesser(const T& t) volatile
	{
		write_retry_loop([&](content_t& c) { c.assign_lesser(simplify_content_type(t)); });
		return *this;
	}

	template <typename T>
	const this_t& pre_assign_lesser(T&& t)
	{
		m_contents->assign_lesser(simplify_content_type(std::forward<T>(t)));
		return *this;
	}

	template <typename T>
	this_t pre_assign_lesser(const T& t) volatile
	{
		return write_retry_loop_pre([&](content_t& c) { c.assign_lesser(simplify_content_type(t)); });
	}

	template <typename T>
	this_t post_assign_lesser(T&& t)
	{
		this_t result(*this);
		m_contents->assign_lesser(simplify_content_type(std::forward<T>(t)));
		return result;
	}

	template <typename T>
	this_t post_assign_lesser(const T& t) volatile
	{
		return write_retry_loop_post([&](content_t& c) { c.assign_lesser(simplify_content_type(t)); });
	}



	// equals
	template <typename T>
	bool operator==(T&& t) const
	{
		return cogs::equals(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator==(T&& t) const volatile
	{
		return cogs::equals(simplify_content_type(*this), std::forward<T>(t));
	}

	// not_equals
	template <typename T>
	bool operator!=(T&& t) const
	{
		return cogs::not_equals(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator!=(T&& t) const volatile
	{
		return cogs::not_equals(simplify_content_type(*this), std::forward<T>(t));
	}


	// is_less_than
	template <typename T>
	bool operator<(T&& t) const
	{
		return cogs::is_less_than(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator<(T&& t) const volatile
	{
		return cogs::is_less_than(simplify_content_type(*this), std::forward<T>(t));
	}


	// is_less_than_or_equal
	template <typename T>
	bool operator<=(T&& t) const
	{
		return cogs::is_less_than_or_equal(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator<=(T&& t) const volatile
	{
		return cogs::is_less_than_or_equal(simplify_content_type(*this), std::forward<T>(t));
	}


	// is_greater_than
	template <typename T>
	bool operator>(T&& t) const
	{
		return cogs::is_greater_than(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator>(T&& t) const volatile
	{
		return cogs::is_greater_than(simplify_content_type(*this), std::forward<T>(t));
	}


	// is_greater_than_or_equal
	template <typename T>
	bool operator>=(T&& t) const
	{
		return cogs::is_greater_than_or_equal(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	bool operator>=(T&& t) const volatile
	{
		return cogs::is_greater_than_or_equal(simplify_content_type(*this), std::forward<T>(t));
	}


	// compare
	template <typename T>
	int compare(T&& t) const
	{
		return cogs::compare(simplify_content_type(*this), std::forward<T>(t));
	}

	template <typename T>
	int compare(T&& t) const volatile
	{
		return cogs::compare(simplify_content_type(*this), std::forward<T>(t));
	}


	// swap
	void swap(this_t& wth)			{ m_contents.swap(wth.m_contents); }
	void swap(this_t& wth) volatile	{ m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth)	{ wth.swap(*this); }

	// exchange
	this_t exchange(const this_t& src)
	{
		this_t rtn;
		m_contents.exchange(src.m_contents, rtn.m_contents);
		return rtn;
	}

	this_t exchange(const this_t& src) volatile
	{
		this_t rtn;
		m_contents.exchange(src.m_contents, rtn.m_contents);
		return rtn;
	}

	this_t exchange(const volatile this_t& src)
	{
		this_t tmpSrc(src);
		this_t rtn;
		m_contents.exchange(tmpSrc.m_contents, rtn.m_contents);
		return rtn;
	}

	this_t exchange(const volatile this_t& src) volatile
	{
		this_t tmpSrc(src);
		this_t rtn;
		m_contents.exchange(tmpSrc.m_contents, rtn.m_contents);
		return rtn;
	}

	void exchange(const this_t& src, this_t& rtn)
	{
		m_contents.exchange(src.m_contents, rtn.m_contents);
	}

	void exchange(const this_t& src, this_t& rtn) volatile
	{
		m_contents.exchange(src.m_contents, rtn.m_contents);
	}

	void exchange(const volatile this_t& src, this_t& rtn)
	{
		this_t tmp(src);
		m_contents.exchange(tmp.m_contents, rtn.m_contents);
	}

	void exchange(const volatile this_t& src, this_t& rtn) volatile
	{
		this_t tmp(src);
		m_contents.exchange(tmp.m_contents, rtn.m_contents);
	}

	void exchange(const this_t& src, volatile this_t& rtn)
	{
		this_t tmp;
		m_contents.exchange(src.m_contents, tmp.m_contents);
		rtn = tmp;
	}

	void exchange(const this_t& src, volatile this_t& rtn) volatile
	{
		this_t tmp;
		m_contents.exchange(src.m_contents, tmp.m_contents);
		rtn = tmp;
	}

	void exchange(const volatile this_t& src, volatile this_t& rtn)
	{
		this_t tmpSrc(src);
		this_t tmpRtn;
		m_contents.exchange(tmpSrc.m_contents, tmpRtn.m_contents);
		rtn = tmpRtn;
	}

	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile
	{
		this_t tmpSrc(src);
		this_t tmpRtn;
		m_contents.exchange(tmpSrc.m_contents, tmpRtn.m_contents);
		rtn = tmpRtn;
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn)
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, tmp.m_contents);
		cogs::assign(rtn, std::move(tmp));
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn) volatile
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, tmp.m_contents);
		cogs::assign(rtn, std::move(tmp));
	}

	template <typename S>
	void exchange(S&& src, this_t& rtn)
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, rtn.m_contents);
	}

	template <typename S>
	void exchange(S&& src, this_t& rtn) volatile
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, rtn.m_contents);
	}

	template <typename S>
	void exchange(S&& src, volatile this_t& rtn)
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, tmp.m_contents);
		cogs::assign(rtn, std::move(tmp));
	}

	template <typename S>
	void exchange(S&& src, volatile this_t& rtn) volatile
	{
		this_t tmp(std::forward<S>(src));
		m_contents.exchange(tmp.m_contents, tmp.m_contents);
		cogs::assign(rtn, std::move(tmp));
	}

	// compare_exchange
	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, const C& cmp, R& rtn)
	{
		return m_contents.compare_exchange(std::forward<S>(src), cmp, rtn);
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, const C& cmp, R& rtn) volatile
	{
		return m_contents.compare_exchange(std::forward<S>(src), cmp, rtn);
	}

	template <typename S, typename C>
	bool compare_exchange(S&& src, const C& cmp)
	{
		return m_contents.compare_exchange(std::forward<S>(src), cmp);
	}

	template <typename S, typename C>
	bool compare_exchange(S&& src, const C& cmp) volatile
	{
		return m_contents.compare_exchange(std::forward<S>(src), cmp);
	}


	void clear()			{ m_contents->clear(); }
	void clear() volatile	{ m_contents.assign(zero_t(), one_t()); }

	const numerator_t& get_numerator() const	{ return m_contents->m_numerator; }
	numerator_t get_numerator() const volatile	{ return begin_read()->m_numerator; }

	const denominator_t& get_denominator() const	{ return m_contents->m_denominator; }
	denominator_t get_denominator() const volatile	{ return begin_read()->m_denominator; }

	auto factor() const				{ return simplify_content_type(*this).factor(); }
	auto factor() const volatile	{ return simplify_content_type(*this).factor(); }
};






template <typename numerator_type1, typename denominator_type1, typename type2>
class compatible<fraction<numerator_type1, denominator_type1>, type2>
{
public:
	typedef fraction<compatible_t<numerator_type1, decltype(cogs::multiply(std::declval<type2>(), std::declval<denominator_type1>()))>, denominator_type1> type;
};

template <typename type2, typename numerator_type1, typename denominator_type1>
class compatible<type2, fraction<numerator_type1, denominator_type1> >
{
public:
	typedef compatible_t<fraction<numerator_type1, denominator_type1>, type2> type;
};

template <typename numerator_type1, typename denominator_type1, typename numerator_type2, typename denominator_type2>
class compatible<fraction<numerator_type1, denominator_type1>, fraction<numerator_type2, denominator_type2> >
{
public:
	typedef fraction<compatible_t<numerator_type1, numerator_type2>, compatible_t<denominator_type1, denominator_type2> > type;
};






template <typename numerator_type, typename denominator_type>
class is_const_type<fraction<numerator_type, denominator_type> >
{
public:
	static constexpr bool value = is_const_type_v<numerator_type> && is_const_type_v<denominator_type>;
};


template <typename numerator_t>
inline auto make_reciprocal(numerator_t&& n) { return fraction<one_t, numerator_t>(one_t(), std::forward<numerator_t>(n)); }

template <typename numerator_t, typename denominator_t>
inline auto make_reciprocal(fraction<numerator_t, denominator_t>& src) { return src.reciprocal(); }

template <typename numerator_t, typename denominator_t>
inline auto make_reciprocal(const fraction<numerator_t, denominator_t>& src) { return src.reciprocal(); }

template <typename numerator_t, typename denominator_t>
inline auto make_reciprocal(volatile fraction<numerator_t, denominator_t>& src) { return src.reciprocal(); }

template <typename numerator_t, typename denominator_t>
inline auto make_reciprocal(const volatile fraction<numerator_t, denominator_t>& src) { return src.reciprocal(); }


template <typename T>
inline std::enable_if_t<!std::is_class_v<std::remove_reference_t<T> >, decltype(make_reciprocal(std::declval<T>())) >
reciprocal(T&& t) { return make_reciprocal(std::forward<T>(t)); }


template <typename numerator_type, typename denominator_type>
auto fraction<numerator_type, denominator_type>::reciprocal() const
{ return cogs::reciprocal(simplify_content_type(*this)); }

	
template <typename numerator_type, typename denominator_type>
auto fraction<numerator_type, denominator_type>::reciprocal() const volatile
{ return cogs::reciprocal(simplify_content_type(*this)); }


#pragma warning(pop)


}



#endif
