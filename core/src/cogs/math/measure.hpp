//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_MATH_MEASURE
#define COGS_HEADER_MATH_MEASURE

#include <type_traits>

#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/measurement_types.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified

template <typename storage_type, class unit_type>
class measure;


template <typename unit_type, typename T>
auto make_measure(T&& t)
{
	return measure<std::remove_cv_t<std::remove_reference_t<T> >, unit_type>(std::forward<T>(t));
}


// What if storage_type is an int?  Or a param?  Do we do what we need to, to ensure volatile reads are routed?
template <typename storage_type, typename unit_type>
class measure
{
public:
	typedef storage_type storage_t;
	typedef unit_type unit_t;

	typedef measure<storage_t, unit_t> this_t;

private:
	storage_t m_contents;

	template <typename, typename>
	friend class measure;

	template <typename, typename T>
	friend auto make_measure(T&& t);

	template <typename storage_type2 = storage_t, typename = std::enable_if_t<!std::is_const_v<storage_type2> && !std::is_volatile_v<storage_type2> > >
	this_t& operator=(storage_type2&& n) { cogs::assign(m_contents, std::move(n)); }

	template <typename storage_type2 = storage_t, typename = std::enable_if_t<!std::is_const_v<storage_type2> && !std::is_volatile_v<storage_type2> > >
	this_t& operator=(storage_type2&& n) volatile { cogs::assign(m_contents, std::move(n)); }

	template <typename storage_type2 = storage_t>
	this_t& operator=(storage_type2& n) { cogs::assign(m_contents, n); }

	template <typename storage_type2 = storage_t>
	this_t& operator=(storage_type2& n) volatile { cogs::assign(m_contents, n); }

public:
	storage_t& get() { return m_contents; }
	const storage_t& get() const { return m_contents; }
	volatile storage_t& get() volatile { return m_contents; }
	const volatile storage_t& get() const volatile { return m_contents; }

	measure() { }

	explicit measure(storage_t& src) : m_contents(src) { }
	explicit measure(const storage_t& src) : m_contents(src) { }
	explicit measure(volatile storage_t& src) : m_contents(load(src)) { }
	explicit measure(const volatile storage_t& src) : m_contents(load(src)) { }

	template <typename storage_type2 = storage_t>
	explicit measure(storage_type2&& n) : m_contents(load(std::move(n))) { }

	template <typename storage_type2 = storage_t>
	explicit measure(storage_type2& n) : m_contents(load(n)) { }

	measure(this_t& src) : m_contents(src.m_contents) { }
	measure(const this_t& src) : m_contents(src.m_contents) { }
	measure(volatile this_t& src) : m_contents(load(src.m_contents)) { }
	measure(const volatile this_t& src) : m_contents(load(src.m_contents)) { }

	measure(this_t&& src) : m_contents(std::move(src.m_contents)) { }

	template <typename storage_type2 = storage_t>
	measure(measure<storage_type2, unit_t>& src)
		: m_contents(src.m_contents)
	{ }

	template <typename storage_type2 = storage_t>
	measure(const measure<storage_type2, unit_t>& src)
		: m_contents(src.m_contents)
	{ }

	template <typename storage_type2 = storage_t>
	measure(volatile measure<storage_type2, unit_t>& src)
		: m_contents(src.m_contents)
	{ }

	template <typename storage_type2 = storage_t>
	measure(const volatile measure<storage_type2, unit_t>& src)
		: m_contents(src.m_contents)
	{ }

	template <typename storage_type2 = storage_t>
	measure(measure<storage_type2, unit_t>&& src)
		: m_contents(std::move(src.m_contents))
	{ }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	measure(measure<storage_type2, unit_type2>& src)
		: m_contents(cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents))
	{ }

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	measure(const measure<storage_type2, unit_type2>& src)
		: m_contents(cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents))
	{ }

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	measure(volatile measure<storage_type2, unit_type2>& src)
		: m_contents(cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents))
	{ }

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	measure(const volatile measure<storage_type2, unit_type2>& src)
		: m_contents(cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents))
	{ }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	measure(measure<storage_type2, unit_type2>&& src)
		: m_contents(cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)))
	{ }

	template <typename unit_type2>
	auto convert_to() const
	{
		return make_measure<unit_type2>(cogs::multiply(typename unit_conversion<unit_t, unit_type2>::ratio_const_t(), m_contents));
	}

	template <typename unit_type2>
	auto convert_to() const volatile
	{
		return make_measure<unit_type2>(cogs::multiply(typename unit_conversion<unit_t, unit_type2>::ratio_const_t(), m_contents));
	}


	this_t& operator=(this_t& src) { cogs::assign(m_contents, src.m_contents); return *this; }
	this_t& operator=(const this_t& src) { cogs::assign(m_contents, src.m_contents); return *this; }
	this_t& operator=(volatile this_t& src) { cogs::assign(m_contents, src.m_contents); return *this; }
	this_t& operator=(const volatile this_t& src) { cogs::assign(m_contents, src.m_contents); return *this; }

	volatile this_t& operator=(this_t& src) volatile { cogs::assign(m_contents, src.m_contents); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { cogs::assign(m_contents, src.m_contents); return *this; }
	volatile this_t& operator=(volatile this_t& src) volatile { cogs::assign(m_contents, src.m_contents); return *this; }
	volatile this_t& operator=(const volatile this_t& src) volatile { cogs::assign(m_contents, src.m_contents); return *this; }

	this_t& operator=(this_t&& src) { cogs::assign(m_contents, std::move(src.m_contents)); return *this; }
	volatile this_t& operator=(this_t&& src) volatile { cogs::assign(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t>
	this_t& operator=(measure<storage_type2, unit_t>& src)
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t& operator=(const measure<storage_type2, unit_t>& src)
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t& operator=(volatile measure<storage_type2, unit_t>& src)
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t& operator=(const volatile measure<storage_type2, unit_t>& src)
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator=(measure<storage_type2, unit_t>& src) volatile
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator=(const measure<storage_type2, unit_t>& src) volatile
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator=(volatile measure<storage_type2, unit_t>& src) volatile
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator=(const volatile measure<storage_type2, unit_t>& src) volatile
	{
		cogs::assign(m_contents, src.m_contents);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t& operator=(measure<storage_type2, unit_t>&& src)
	{
		cogs::assign(m_contents, std::move(src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator=(measure<storage_type2, unit_t>&& src) volatile
	{
		cogs::assign(m_contents, std::move(src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator=(measure<storage_type2, unit_type2>& src)
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator=(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator=(volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator=(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator=(measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator=(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator=(volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator=(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator=(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator=(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	bool operator!() const { return cogs::lnot(m_contents); }
	bool operator!() const volatile { return cogs::lnot(m_contents); }

	bool is_negative() const { return cogs::is_negative(m_contents); }
	bool is_negative() const volatile { return cogs::is_negative(m_contents); }

	bool is_exponent_of_two() const { return cogs::is_exponent_of_two(m_contents); }
	bool is_exponent_of_two() const volatile { return cogs::is_exponent_of_two(m_contents); }

	bool has_fractional_part() const { return cogs::has_fractional_part(m_contents); }
	bool has_fractional_part() const volatile { return cogs::has_fractional_part(m_contents); }

	auto abs() const { return make_measure<unit_t>(cogs::abs(m_contents)); }
	auto abs() const volatile { return make_measure<unit_t>(cogs::abs(m_contents)); }

	void assign_abs() { cogs::assign_abs(m_contents); }
	void assign_abs() volatile { cogs::assign_abs(m_contents); }
	const this_t& pre_assign_abs() { cogs::assign_abs(m_contents); return *this; }
	this_t pre_assign_abs() volatile { return cogs::pre_assign_abs(m_contents); }
	this_t post_assign_abs() { return cogs::post_assign_abs(m_contents); }
	this_t post_assign_abs() volatile { return cogs::post_assign_abs(m_contents); }

	auto operator-() const { return make_measure<unit_t>(cogs::negative(m_contents)); }
	auto operator-() const volatile { return make_measure<unit_t>(cogs::negative(m_contents)); }

	void assign_negative() { cogs::assign_negative(m_contents); }
	void assign_negative() volatile { cogs::assign_negative(m_contents); }
	const this_t& pre_assign_negative() { cogs::assign_negative(m_contents); return *this; }
	this_t pre_assign_negative() volatile { return cogs::pre_assign_negative(m_contents); }
	this_t post_assign_negative() { return cogs::post_assign_negative(m_contents); }
	this_t post_assign_negative() volatile { return cogs::post_assign_negative(m_contents); }

	auto next() const { return make_measure<unit_t>(cogs::next(m_contents)); }
	auto next() const volatile { return make_measure<unit_t>(cogs::next(m_contents)); }

	void assign_next() { cogs::assign_next(m_contents); }
	void assign_next() volatile { cogs::assign_next(m_contents); }
	this_t& operator++() { cogs::assign_next(m_contents); return *this; }
	this_t operator++() volatile { return cogs::pre_assign_next(m_contents); }
	this_t operator++(int) { return cogs::post_assign_next(m_contents); }
	this_t operator++(int) volatile { return cogs::post_assign_next(m_contents); }

	auto prev() const { return make_measure<unit_t>(cogs::prev(m_contents)); }
	auto prev() const volatile { return make_measure<unit_t>(cogs::prev(m_contents)); }

	void assign_prev() { cogs::assign_prev(m_contents); }
	void assign_prev() volatile { cogs::assign_prev(m_contents); }
	this_t& operator--() { cogs::assign_prev(m_contents); return *this; }
	this_t operator--() volatile { return cogs::pre_assign_prev(m_contents); }
	this_t operator--(int) { return cogs::post_assign_prev(m_contents); }
	this_t operator--(int) volatile { return cogs::post_assign_prev(m_contents); }


	// add
	auto operator+(const this_t& src) const { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }
	auto operator+(const volatile this_t& src) const { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }

	auto operator+(const this_t& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }
	auto operator+(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }

	auto operator+(this_t&& src) const { return make_measure<unit_t>(cogs::add(m_contents, std::move(src.m_contents))); }
	auto operator+(this_t&& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto operator+(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator+(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator+(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator+(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator+(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator+(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::add(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator+(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::add(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// +=
	this_t& operator+=(const this_t& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }
	this_t& operator+=(const volatile this_t& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }

	volatile this_t& operator+=(const this_t& src) volatile { cogs::assign_add(m_contents, src.m_contents); return *this; }
	volatile this_t& operator+=(const volatile this_t& src) volatile { cogs::assign_add(m_contents, src.m_contents); return *this; }

	this_t& operator+=(this_t&& src) { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }
	volatile this_t& operator+=(this_t&& src) volatile { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t>
	this_t& operator+=(const measure<storage_type2, unit_t>& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	this_t& operator+=(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	volatile this_t& operator+=(const measure<storage_type2, unit_t>& src) volatile { cogs::assign_add(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator+=(const volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_add(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t& operator+=(measure<storage_type2, unit_t>&& src) { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator+=(measure<storage_type2, unit_t>&& src) volatile { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator+=(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator+=(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator+=(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator+=(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator+=(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator+=(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}


	// pre_assign_add
	const this_t& pre_assign_add(const this_t& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_add(const volatile this_t& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }

	this_t pre_assign_add(const this_t& src) volatile { return cogs::pre_assign_add(m_contents, src.m_contents); }
	this_t pre_assign_add(const volatile this_t& src) volatile { return cogs::pre_assign_add(m_contents, src.m_contents); }

	const this_t& pre_assign_add(this_t&& src) { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }
	this_t pre_assign_add(this_t&& src) volatile { return cogs::pre_assign_add(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_add(const measure<storage_type2, unit_t>& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_add(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_add(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t pre_assign_add(const measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_add(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_add(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_add(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_add(measure<storage_type2, unit_t>&& src) { cogs::assign_add(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_add(measure<storage_type2, unit_t>&& src) volatile { return cogs::pre_assign_add(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_add(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_add(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_add(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_add(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_add(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_add(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::pre_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// post_assign_add
	this_t post_assign_add(const this_t& src) { return cogs::post_assign_add(m_contents, src.m_contents); }
	this_t post_assign_add(const volatile this_t& src) { return cogs::post_assign_add(m_contents, src.m_contents); }

	this_t post_assign_add(const this_t& src) volatile { return cogs::post_assign_add(m_contents, src.m_contents); }
	this_t post_assign_add(const volatile this_t& src) volatile { return cogs::post_assign_add(m_contents, src.m_contents); }

	this_t post_assign_add(this_t&& src) { return cogs::post_assign_add(m_contents, std::move(src.m_contents)); }
	this_t post_assign_add(this_t&& src) volatile { return cogs::post_assign_add(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_add(const measure<storage_type2, unit_t>& src) { return cogs::post_assign_add(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_add(const volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_add(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_add(const measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_add(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_add(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_add(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_add(measure<storage_type2, unit_t>&& src) { return cogs::post_assign_add(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_add(measure<storage_type2, unit_t>&& src) volatile { return cogs::post_assign_add(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(const measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(const volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(measure<storage_type2, unit_type2>&& src)
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_add(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::post_assign_add(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// subtract
	auto operator-(const this_t& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }
	auto operator-(const volatile this_t& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }

	auto operator-(const this_t& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }
	auto operator-(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }

	auto operator-(this_t&& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, std::move(src.m_contents))); }
	auto operator-(this_t&& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto operator-(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator-(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator-(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator-(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator-(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator-(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::subtract(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator-(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// -=
	this_t& operator-=(const this_t& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	this_t& operator-=(const volatile this_t& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	volatile this_t& operator-=(const this_t& src) volatile { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	volatile this_t& operator-=(const volatile this_t& src) volatile { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	this_t& operator-=(this_t&& src) { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }
	volatile this_t& operator-=(this_t&& src) volatile { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t>
	this_t& operator-=(const measure<storage_type2, unit_t>& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	this_t& operator-=(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	volatile this_t& operator-=(const measure<storage_type2, unit_t>& src) volatile { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator-=(const volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t& operator-=(measure<storage_type2, unit_t>&& src) { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator-=(measure<storage_type2, unit_t>&& src) volatile { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator-=(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator-=(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator-=(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator-=(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator-=(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator-=(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}


	// pre_assign_subtract
	const this_t& pre_assign_subtract(const this_t& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_subtract(const volatile this_t& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	this_t pre_assign_subtract(const this_t& src) volatile { return cogs::pre_assign_subtract(m_contents, src.m_contents); }
	this_t pre_assign_subtract(const volatile this_t& src) volatile { return cogs::pre_assign_subtract(m_contents, src.m_contents); }

	const this_t& pre_assign_subtract(this_t&& src) { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }
	this_t pre_assign_subtract(this_t&& src) volatile { return cogs::pre_assign_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_subtract(const measure<storage_type2, unit_t>& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_subtract(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_subtract(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t pre_assign_subtract(const measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_subtract(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_subtract(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_subtract(measure<storage_type2, unit_t>&& src) { cogs::assign_subtract(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_subtract(measure<storage_type2, unit_t>&& src) volatile { return cogs::pre_assign_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_subtract(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_subtract(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_subtract(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_subtract(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_subtract(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_subtract(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::pre_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// post_assign_subtract
	this_t post_assign_subtract(const this_t& src) { return cogs::post_assign_subtract(m_contents, src.m_contents); }
	this_t post_assign_subtract(const volatile this_t& src) { return cogs::post_assign_subtract(m_contents, src.m_contents); }

	this_t post_assign_subtract(const this_t& src) volatile { return cogs::post_assign_subtract(m_contents, src.m_contents); }
	this_t post_assign_subtract(const volatile this_t& src) volatile { return cogs::post_assign_subtract(m_contents, src.m_contents); }

	this_t post_assign_subtract(this_t&& src) { return cogs::post_assign_subtract(m_contents, std::move(src.m_contents)); }
	this_t post_assign_subtract(this_t&& src) volatile { return cogs::post_assign_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(const measure<storage_type2, unit_t>& src) { return cogs::post_assign_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(const volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_subtract(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(const measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_subtract(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(measure<storage_type2, unit_t>&& src) { return cogs::post_assign_subtract(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_subtract(measure<storage_type2, unit_t>&& src) volatile { return cogs::post_assign_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(const measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(const volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(measure<storage_type2, unit_type2>&& src)
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_subtract(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::post_assign_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// inverse_subtract
	auto inverse_subtract(const this_t& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }
	auto inverse_subtract(const volatile this_t& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }

	auto inverse_subtract(const this_t& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }
	auto inverse_subtract(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }

	auto inverse_subtract(this_t&& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, std::move(src.m_contents))); }
	auto inverse_subtract(this_t&& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto inverse_subtract(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_subtract(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto inverse_subtract(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_subtract(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto inverse_subtract(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_subtract(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::inverse_subtract(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_subtract(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_subtract(
			cogs::multiply(typename unit_conversion<unit_t, finer_t<unit_t, unit_type2> >::ratio_const_t(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// assign_inverse_subtract
	void assign_inverse_subtract(const this_t& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); }
	void assign_inverse_subtract(const volatile this_t& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); }

	void assign_inverse_subtract(const this_t& src) volatile { cogs::assign_inverse_subtract(m_contents, src.m_contents); }
	void assign_inverse_subtract(const volatile this_t& src) volatile { cogs::assign_inverse_subtract(m_contents, src.m_contents); }

	void assign_inverse_subtract(this_t&& src) { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); }
	void assign_inverse_subtract(this_t&& src) volatile { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(const measure<storage_type2, unit_t>& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(const measure<storage_type2, unit_t>& src) volatile { cogs::assign_inverse_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_inverse_subtract(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(measure<storage_type2, unit_t>&& src) { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_subtract(measure<storage_type2, unit_t>&& src) volatile { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_subtract(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// pre_assign_inverse_subtract
	const this_t& pre_assign_inverse_subtract(const this_t& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_inverse_subtract(const volatile this_t& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); return *this; }

	this_t pre_assign_inverse_subtract(const this_t& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, src.m_contents); }
	this_t pre_assign_inverse_subtract(const volatile this_t& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, src.m_contents); }

	const this_t& pre_assign_inverse_subtract(this_t&& src) { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); return *this; }
	this_t pre_assign_inverse_subtract(this_t&& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_subtract(const measure<storage_type2, unit_t>& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_inverse_subtract(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_subtract(const measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_subtract(measure<storage_type2, unit_t>&& src) { cogs::assign_inverse_subtract(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_subtract(measure<storage_type2, unit_t>&& src) volatile { return cogs::pre_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_subtract(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_subtract(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_subtract(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_subtract(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::pre_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// post_assign_inverse_subtract
	this_t post_assign_inverse_subtract(const this_t& src) { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }
	this_t post_assign_inverse_subtract(const volatile this_t& src) { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }

	this_t post_assign_inverse_subtract(const this_t& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }
	this_t post_assign_inverse_subtract(const volatile this_t& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }

	this_t post_assign_inverse_subtract(this_t&& src) { return cogs::post_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }
	this_t post_assign_inverse_subtract(this_t&& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(const measure<storage_type2, unit_t>& src) { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(const measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(measure<storage_type2, unit_t>&& src) { return cogs::post_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_subtract(measure<storage_type2, unit_t>&& src) volatile { return cogs::post_assign_inverse_subtract(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(const measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(measure<storage_type2, unit_type2>&& src)
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_subtract(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::post_assign_inverse_subtract(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// modulo
	auto operator%(this_t& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(const this_t& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(volatile this_t& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(const volatile this_t& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }

	auto operator%(this_t& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(const this_t& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	auto operator%(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }

	auto operator%(this_t&& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, std::move(src.m_contents))); }
	auto operator%(this_t&& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto operator%(measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator%(measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto operator%(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto operator%(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::modulo(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto operator%(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	template <typename storage_type2 = storage_t>
	auto operator%(storage_type2&& src) const
	{
		return make_measure<unit_t>(cogs::modulo(m_contents, std::move(src)));
	}

	template <typename storage_type2 = storage_t>
	auto operator%(storage_type2&& src) const volatile
	{
		return make_measure<unit_t>(cogs::modulo(m_contents, std::move(src)));
	}


	template <typename storage_type2 = storage_t>
	auto operator%(storage_type2& src) const
	{
		return make_measure<unit_t>(cogs::modulo(m_contents, src));
	}

	template <typename storage_type2 = storage_t>
	auto operator%(storage_type2& src) const volatile
	{
		return make_measure<unit_t>(cogs::modulo(m_contents, src));
	}


	// %=
	this_t& operator%=(this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	this_t& operator%=(const this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	this_t& operator%=(volatile this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	this_t& operator%=(const volatile this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	volatile this_t& operator%=(this_t& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	volatile this_t& operator%=(const this_t& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	volatile this_t& operator%=(volatile this_t& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	volatile this_t& operator%=(const volatile this_t& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	this_t& operator%=(this_t&& src) { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }
	volatile this_t& operator%=(this_t&& src) volatile { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t>
	this_t& operator%=(measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	this_t& operator%=(const measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	this_t& operator%=(volatile measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	this_t& operator%=(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(measure<storage_type2, unit_t>& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(const measure<storage_type2, unit_t>& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(const volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t& operator%=(measure<storage_type2, unit_t>&& src) { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(measure<storage_type2, unit_t>&& src) volatile { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator%=(measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator%=(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator%=(volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator%=(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator%=(measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator%=(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator%=(volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator%=(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t& operator%=(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	volatile this_t& operator%=(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}


	template <typename storage_type2 = storage_t>
	this_t& operator%=(storage_type2&& src)
	{
		cogs::assign_modulo(m_contents, std::move(src));
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(storage_type2&& src) volatile
	{
		cogs::assign_modulo(m_contents, std::move(src));
		return *this;
	}


	template <typename storage_type2 = storage_t>
	this_t& operator%=(storage_type2& src)
	{
		cogs::assign_modulo(m_contents, std::move(src));
		return *this;
	}

	template <typename storage_type2 = storage_t>
	volatile this_t& operator%=(storage_type2& src) volatile
	{
		cogs::assign_modulo(m_contents, std::move(src));
		return *this;
	}


	// pre_assign_modulo
	const this_t& pre_assign_modulo(this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_modulo(const this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_modulo(volatile this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_modulo(const volatile this_t& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	this_t pre_assign_modulo(this_t& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	this_t pre_assign_modulo(const this_t& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	this_t pre_assign_modulo(volatile this_t& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	this_t pre_assign_modulo(const volatile this_t& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }

	const this_t& pre_assign_modulo(this_t&& src) { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }
	this_t pre_assign_modulo(this_t&& src) volatile { return cogs::pre_assign_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(const measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(volatile measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_modulo(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(const measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_modulo(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(measure<storage_type2, unit_t>&& src) { cogs::assign_modulo(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(measure<storage_type2, unit_t>&& src) volatile { return cogs::pre_assign_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_modulo(measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_modulo(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_modulo(volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_modulo(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_modulo(measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_modulo(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_modulo(volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_modulo(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_modulo(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_modulo(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(storage_type2&& src)
	{
		cogs::assign_modulo(m_contents, std::move(src));
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(storage_type2&& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_modulo(storage_type2& src)
	{
		cogs::assign_modulo(m_contents, src);
		return *this;
	}

	template <typename storage_type2 = storage_t>
	this_t pre_assign_modulo(storage_type2& src) volatile
	{
		return cogs::pre_assign_modulo(m_contents, src);
	}


	// post_assign_modulo
	this_t post_assign_modulo(this_t& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(const this_t& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(volatile this_t& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(const volatile this_t& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }

	this_t post_assign_modulo(this_t& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(const this_t& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(volatile this_t& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	this_t post_assign_modulo(const volatile this_t& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }

	this_t post_assign_modulo(this_t&& src) { return cogs::post_assign_modulo(m_contents, std::move(src.m_contents)); }
	this_t post_assign_modulo(this_t&& src) volatile { return cogs::post_assign_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(measure<storage_type2, unit_t>& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(const measure<storage_type2, unit_t>& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(const volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_modulo(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(const measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_modulo(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(measure<storage_type2, unit_t>&& src) { return cogs::post_assign_modulo(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(measure<storage_type2, unit_t>&& src) volatile { return cogs::post_assign_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(const measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(const volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(measure<storage_type2, unit_type2>&& src)
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_modulo(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(storage_type2&& src)
	{
		return cogs::post_assign_modulo(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(storage_type2&& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(storage_type2& src)
	{
		return cogs::post_assign_modulo(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	this_t post_assign_modulo(storage_type2& src) volatile
	{
		return cogs::post_assign_modulo(m_contents, src);
	}


	// inverse_modulo
	auto inverse_modulo(const this_t& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }
	auto inverse_modulo(const volatile this_t& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }

	auto inverse_modulo(const this_t& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }
	auto inverse_modulo(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }

	auto inverse_modulo(this_t&& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, std::move(src.m_contents))); }
	auto inverse_modulo(this_t&& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto inverse_modulo(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_modulo(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto inverse_modulo(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_modulo(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto inverse_modulo(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto inverse_modulo(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::inverse_modulo(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto inverse_modulo(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::inverse_modulo(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// assign_inverse_modulo
	void assign_inverse_modulo(const this_t& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); }
	void assign_inverse_modulo(const volatile this_t& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); }

	void assign_inverse_modulo(const this_t& src) volatile { cogs::assign_inverse_modulo(m_contents, src.m_contents); }
	void assign_inverse_modulo(const volatile this_t& src) volatile { cogs::assign_inverse_modulo(m_contents, src.m_contents); }

	void assign_inverse_modulo(this_t&& src) { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); }
	void assign_inverse_modulo(this_t&& src) volatile { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(const measure<storage_type2, unit_t>& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(const measure<storage_type2, unit_t>& src) volatile { cogs::assign_inverse_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) volatile { cogs::assign_inverse_modulo(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(measure<storage_type2, unit_t>&& src) { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	void assign_inverse_modulo(measure<storage_type2, unit_t>&& src) volatile { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(const measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	void assign_inverse_modulo(measure<storage_type2, unit_type2>&& src) volatile
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// pre_assign_inverse_modulo
	const this_t& pre_assign_inverse_modulo(const this_t& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); return *this; }
	const this_t& pre_assign_inverse_modulo(const volatile this_t& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); return *this; }

	this_t pre_assign_inverse_modulo(const this_t& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, src.m_contents); }
	this_t pre_assign_inverse_modulo(const volatile this_t& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, src.m_contents); }

	const this_t& pre_assign_inverse_modulo(this_t&& src) { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); return *this; }
	this_t pre_assign_inverse_modulo(this_t&& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_modulo(const measure<storage_type2, unit_t>& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); return *this; }
	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) { cogs::assign_inverse_modulo(m_contents, src.m_contents); return *this; }

	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_modulo(const measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	const this_t& pre_assign_inverse_modulo(measure<storage_type2, unit_t>&& src) { cogs::assign_inverse_modulo(m_contents, std::move(src.m_contents)); return *this; }
	template <typename storage_type2 = storage_t>
	this_t pre_assign_inverse_modulo(measure<storage_type2, unit_t>&& src) volatile { return cogs::pre_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_modulo(const measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
		return *this;
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_modulo(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::pre_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	const this_t& pre_assign_inverse_modulo(measure<storage_type2, unit_type2>&& src)
	{
		cogs::assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
		return *this;
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t pre_assign_inverse_modulo(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::pre_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}


	// post_assign_inverse_modulo
	this_t post_assign_inverse_modulo(const this_t& src) { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }
	this_t post_assign_inverse_modulo(const volatile this_t& src) { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }

	this_t post_assign_inverse_modulo(const this_t& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }
	this_t post_assign_inverse_modulo(const volatile this_t& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }

	this_t post_assign_inverse_modulo(this_t&& src) { return cogs::post_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }
	this_t post_assign_inverse_modulo(this_t&& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(const measure<storage_type2, unit_t>& src) { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(const measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(const volatile measure<storage_type2, unit_t>& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(measure<storage_type2, unit_t>&& src) { return cogs::post_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }
	template <typename storage_type2 = storage_t>
	this_t post_assign_inverse_modulo(measure<storage_type2, unit_t>&& src) volatile { return cogs::post_assign_inverse_modulo(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(const measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src)
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(const measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(const volatile measure<storage_type2, unit_type2>& src) volatile
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(measure<storage_type2, unit_type2>&& src)
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	this_t post_assign_inverse_modulo(measure<storage_type2, unit_type2>&& src) volatile
	{
		return cogs::post_assign_inverse_modulo(m_contents, cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	// multiply
	template <typename storage_type2 = storage_t> auto operator*(storage_type2&& src) const { return make_measure<unit_t>(cogs::multiply(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto operator*(storage_type2&& src) const volatile { return make_measure<unit_t>(cogs::multiply(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto operator*(storage_type2& src) const { return make_measure<unit_t>(cogs::multiply(m_contents, src)); }
	template <typename storage_type2 = storage_t> auto operator*(storage_type2& src) const volatile { return make_measure<unit_t>(cogs::multiply(m_contents, src)); }

		// TBD
	template <typename storage_type2, typename unit_type2> auto operator*(measure<storage_type2, unit_type2>&& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(measure<storage_type2, unit_type2>&& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(const measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(const measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(volatile measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(const volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator*(const volatile measure<storage_type2, unit_type2>& src) const volatile = delete;

	// *=
	template <typename storage_type2 = storage_t> this_t& operator*=(storage_type2&& src) { cogs::assign_multiply(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> this_t& operator*=(storage_type2& src) { cogs::assign_multiply(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> volatile this_t& operator*=(storage_type2&& src) volatile { cogs::assign_multiply(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> volatile this_t& operator*=(storage_type2& src) volatile { cogs::assign_multiply(m_contents, std::move(src)); return *this; }

		// TBD
	template <typename storage_type2, typename unit_type2> this_t& operator*=(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator*=(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator*=(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator*=(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator*=(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator*=(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator*=(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator*=(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator*=(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator*=(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// pre_assign_multiply
	template <typename storage_type2 = storage_t> const this_t& pre_assign_multiply(storage_type2&& src) { cogs::assign_multiply(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> const this_t& pre_assign_multiply(storage_type2& src) { cogs::assign_multiply(m_contents, src); return *this; }
	template <typename storage_type2 = storage_t> this_t pre_assign_multiply(storage_type2&& src) volatile { return cogs::pre_assign_multiply(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t pre_assign_multiply(storage_type2& src) volatile { return cogs::pre_assign_multiply(m_contents, src); }

		// TBD
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_multiply(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_multiply(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_multiply(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_multiply(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_multiply(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_multiply(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_multiply(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_multiply(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_multiply(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_multiply(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// post_assign_multiply
	template <typename storage_type2 = storage_t> this_t post_assign_multiply(storage_type2&& src) { return cogs::post_assign_multiply(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_multiply(storage_type2&& src) volatile { return cogs::post_assign_multiply(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_multiply(storage_type2& src) { return cogs::post_assign_multiply(m_contents, src); }
	template <typename storage_type2 = storage_t> this_t post_assign_multiply(storage_type2& src) volatile { return cogs::post_assign_multiply(m_contents, src); }

		// TBD
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_multiply(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// divide
	template <typename storage_type2 = storage_t> auto operator/(storage_type2&& src) const { return make_measure<unit_t>(cogs::divide(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto operator/(storage_type2&& src) const volatile { return make_measure<unit_t>(cogs::divide(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto operator/(storage_type2& src) const { return make_measure<unit_t>(cogs::divide(m_contents, src)); }
	template <typename storage_type2 = storage_t> auto operator/(storage_type2& src) const volatile { return make_measure<unit_t>(cogs::divide(m_contents, src)); }

		// TBD
	template <typename storage_type2, typename unit_type2> auto operator/(measure<storage_type2, unit_type2>&& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(measure<storage_type2, unit_type2>&& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(const measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(const measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(volatile measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(const volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto operator/(const volatile measure<storage_type2, unit_type2>& src) const volatile = delete;

	// /=
	template <typename storage_type2 = storage_t> this_t& operator/=(storage_type2&& src) { cogs::assign_divide(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> this_t& operator/=(storage_type2& src) { cogs::assign_divide(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> volatile this_t& operator/=(storage_type2&& src) volatile { cogs::assign_divide(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> volatile this_t& operator/=(storage_type2& src) volatile { cogs::assign_divide(m_contents, std::move(src)); return *this; }

		// TBD
	template <typename storage_type2, typename unit_type2> this_t& operator/=(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator/=(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator/=(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator/=(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t& operator/=(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator/=(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator/=(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator/=(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator/=(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> volatile this_t& operator/=(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// pre_assign_divide
	template <typename storage_type2 = storage_t> const this_t& pre_assign_divide(storage_type2&& src) { cogs::assign_divide(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> const this_t& pre_assign_divide(storage_type2& src) { cogs::assign_divide(m_contents, src); return *this; }
	template <typename storage_type2 = storage_t> this_t pre_assign_divide(storage_type2&& src) volatile { return cogs::pre_assign_divide(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t pre_assign_divide(storage_type2& src) volatile { return cogs::pre_assign_divide(m_contents, src); }

		// TBD
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// post_assign_divide
	template <typename storage_type2 = storage_t> this_t post_assign_divide(storage_type2&& src) { return cogs::post_assign_divide(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide(storage_type2&& src) volatile { return cogs::post_assign_divide(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide(storage_type2& src) { return cogs::post_assign_divide(m_contents, src); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide(storage_type2& src) volatile { return cogs::post_assign_divide(m_contents, src); }

	// TBD
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;



	// divide_whole
	template <typename storage_type2 = storage_t> auto divide_whole(storage_type2&& src) const { return make_measure<unit_t>(cogs::divide_whole(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto divide_whole(storage_type2&& src) const volatile { return make_measure<unit_t>(cogs::divide_whole(m_contents, std::move(src))); }
	template <typename storage_type2 = storage_t> auto divide_whole(storage_type2& src) const { return make_measure<unit_t>(cogs::divide_whole(m_contents, src)); }
	template <typename storage_type2 = storage_t> auto divide_whole(storage_type2& src) const volatile { return make_measure<unit_t>(cogs::divide_whole(m_contents, src)); }

		// TBD
	template <typename storage_type2, typename unit_type2> auto divide_whole(measure<storage_type2, unit_type2>&& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(measure<storage_type2, unit_type2>&& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(const measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(const measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(volatile measure<storage_type2, unit_type2>& src) const volatile = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(const volatile measure<storage_type2, unit_type2>& src) const = delete;
	template <typename storage_type2, typename unit_type2> auto divide_whole(const volatile measure<storage_type2, unit_type2>& src) const volatile = delete;

	// assign_divide_whole
	template <typename storage_type2 = storage_t> void assign_divide_whole(storage_type2&& src) { cogs::assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> void assign_divide_whole(storage_type2& src) { cogs::assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> void assign_divide_whole(storage_type2&& src) volatile { cogs::assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> void assign_divide_whole(storage_type2& src) volatile { cogs::assign_divide_whole(m_contents, std::move(src)); }

		// TBD
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> void assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// pre_assign_divide_whole
	template <typename storage_type2 = storage_t> const this_t& pre_assign_divide_whole(storage_type2&& src) { cogs::assign_divide_whole(m_contents, std::move(src)); return *this; }
	template <typename storage_type2 = storage_t> const this_t& pre_assign_divide_whole(storage_type2& src) { cogs::assign_divide_whole(m_contents, src); return *this; }
	template <typename storage_type2 = storage_t> this_t pre_assign_divide_whole(storage_type2&& src) volatile { return cogs::pre_assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t pre_assign_divide_whole(storage_type2& src) volatile { return cogs::pre_assign_divide_whole(m_contents, src); }

		// TBD
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide_whole(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide_whole(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide_whole(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> const this_t& pre_assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide_whole(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide_whole(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide_whole(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t pre_assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;

	// post_assign_divide_whole
	template <typename storage_type2 = storage_t> this_t post_assign_divide_whole(storage_type2&& src) { return cogs::post_assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide_whole(storage_type2&& src) volatile { return cogs::post_assign_divide_whole(m_contents, std::move(src)); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide_whole(storage_type2& src) { return cogs::post_assign_divide_whole(m_contents, src); }
	template <typename storage_type2 = storage_t> this_t post_assign_divide_whole(storage_type2& src) volatile { return cogs::post_assign_divide_whole(m_contents, src); }

		// TBD
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(measure<storage_type2, unit_type2>&& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(const measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(measure<storage_type2, unit_type2>&& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(const measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(volatile measure<storage_type2, unit_type2>& src) volatile = delete;
	template <typename storage_type2, typename unit_type2> this_t post_assign_divide_whole(const volatile measure<storage_type2, unit_type2>& src) volatile = delete;


	// lesser
	auto lesser(const this_t& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }
	auto lesser(const volatile this_t& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }

	auto lesser(const this_t& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }
	auto lesser(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }

	auto lesser(this_t&& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, std::move(src.m_contents))); }
	auto lesser(this_t&& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto lesser(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto lesser(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto lesser(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto lesser(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto lesser(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto lesser(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::lesser(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto lesser(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::lesser(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// greater
	auto greater(const this_t& src) const { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }
	auto greater(const volatile this_t& src) const { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }

	auto greater(const this_t& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }
	auto greater(const volatile this_t& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }

	auto greater(this_t&& src) const { return make_measure<unit_t>(cogs::greater(m_contents, std::move(src.m_contents))); }
	auto greater(this_t&& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, std::move(src.m_contents))); }


	template <typename storage_type2 = storage_t>
	auto greater(const measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto greater(const volatile measure<storage_type2, unit_t>& src) const { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto greater(const measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto greater(const volatile measure<storage_type2, unit_t>& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }

	template <typename storage_type2 = storage_t>
	auto greater(measure<storage_type2, unit_t>&& src) const { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }
	template <typename storage_type2 = storage_t>
	auto greater(measure<storage_type2, unit_t>&& src) const volatile { return make_measure<unit_t>(cogs::greater(m_contents, src.m_contents)); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(const measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(measure<storage_type2, unit_type2>&& src) const
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	auto greater(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return make_measure<finer_t<unit_t, unit_type2> >(cogs::greater(
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t()(), m_contents),
			cogs::multiply(typename unit_conversion<unit_type2, finer_t<unit_t, unit_type2> >::ratio_const_t(), std::move(src.m_contents))));
	}


	// rate
	// reciprocal
	// inverse_divide
	// inverse_divide_whole
	// divide_whole_and_modulo
	// inverse_divide_whole_and_inverse_modulo
	// gcd
	// lcm


	// equals
	bool operator==(this_t& src) const { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(const this_t& src) const { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(volatile this_t& src) const { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(const volatile this_t& src) const { return cogs::equals(m_contents, src.m_contents); }

	bool operator==(this_t& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(const this_t& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(volatile this_t& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	bool operator==(const volatile this_t& src) const volatile { return cogs::equals(m_contents, src.m_contents); }

	bool operator==(this_t&& src) const { return cogs::equals(m_contents, std::move(src.m_contents)); }
	bool operator==(this_t&& src) const volatile { return cogs::equals(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator==(measure<storage_type2, unit_t>& src) const { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(const measure<storage_type2, unit_t>& src) const { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(volatile measure<storage_type2, unit_t>& src) const { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(const volatile measure<storage_type2, unit_t>& src) const { return cogs::equals(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator==(measure<storage_type2, unit_t>& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(const measure<storage_type2, unit_t>& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::equals(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator==(measure<storage_type2, unit_t>&& src) const { return cogs::equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator==(measure<storage_type2, unit_t>&& src) const volatile { return cogs::equals(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator==(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator==(storage_type2&& src) const
	{
		return cogs::equals(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator==(storage_type2&& src) const volatile
	{
		return cogs::equals(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator==(storage_type2& src) const
	{
		return cogs::equals(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator==(storage_type2& src) const volatile
	{
		return cogs::equals(m_contents, src);
	}


	// not_equals
	bool operator!=(this_t& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(const this_t& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(volatile this_t& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(const volatile this_t& src) const { return cogs::not_equals(m_contents, src.m_contents); }

	bool operator!=(this_t& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(const this_t& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(volatile this_t& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	bool operator!=(const volatile this_t& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }

	bool operator!=(this_t&& src) const { return cogs::not_equals(m_contents, std::move(src.m_contents)); }
	bool operator!=(this_t&& src) const volatile { return cogs::not_equals(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator!=(measure<storage_type2, unit_t>& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(const measure<storage_type2, unit_t>& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(volatile measure<storage_type2, unit_t>& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(const volatile measure<storage_type2, unit_t>& src) const { return cogs::not_equals(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator!=(measure<storage_type2, unit_t>& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(const measure<storage_type2, unit_t>& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator!=(measure<storage_type2, unit_t>&& src) const { return cogs::not_equals(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator!=(measure<storage_type2, unit_t>&& src) const volatile { return cogs::not_equals(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator!=(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::not_equals(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator!=(storage_type2&& src) const
	{
		return cogs::not_equals(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator!=(storage_type2&& src) const volatile
	{
		return cogs::not_equals(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator!=(storage_type2& src) const
	{
		return cogs::not_equals(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator!=(storage_type2& src) const volatile
	{
		return cogs::not_equals(m_contents, src);
	}


	// is_less_than
	bool operator<(this_t& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(const this_t& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(volatile this_t& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(const volatile this_t& src) const { return cogs::is_less_than(m_contents, src.m_contents); }

	bool operator<(this_t& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(const this_t& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(volatile this_t& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	bool operator<(const volatile this_t& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }

	bool operator<(this_t&& src) const { return cogs::is_less_than(m_contents, std::move(src.m_contents)); }
	bool operator<(this_t&& src) const volatile { return cogs::is_less_than(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator<(measure<storage_type2, unit_t>& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(const measure<storage_type2, unit_t>& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(volatile measure<storage_type2, unit_t>& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(const volatile measure<storage_type2, unit_t>& src) const { return cogs::is_less_than(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator<(measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(const measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator<(measure<storage_type2, unit_t>&& src) const { return cogs::is_less_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<(measure<storage_type2, unit_t>&& src) const volatile { return cogs::is_less_than(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::is_less_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator<(storage_type2&& src) const
	{
		return cogs::is_less_than(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator<(storage_type2&& src) const volatile
	{
		return cogs::is_less_than(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator<(storage_type2& src) const
	{
		return cogs::is_less_than(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator<(storage_type2& src) const volatile
	{
		return cogs::is_less_than(m_contents, src);
	}


	// is_greater_than
	bool operator>(this_t& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(const this_t& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(volatile this_t& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(const volatile this_t& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }

	bool operator>(this_t& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(const this_t& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(volatile this_t& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	bool operator>(const volatile this_t& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }

	bool operator>(this_t&& src) const { return cogs::is_greater_than(m_contents, std::move(src.m_contents)); }
	bool operator>(this_t&& src) const volatile { return cogs::is_greater_than(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator>(measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(const measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(volatile measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(const volatile measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator>(measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(const measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator>(measure<storage_type2, unit_t>&& src) const { return cogs::is_greater_than(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>(measure<storage_type2, unit_t>&& src) const volatile { return cogs::is_greater_than(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::is_greater_than(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator>(storage_type2&& src) const
	{
		return cogs::is_greater_than(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator>(storage_type2&& src) const volatile
	{
		return cogs::is_greater_than(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator>(storage_type2& src) const
	{
		return cogs::is_greater_than(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator>(storage_type2& src) const volatile
	{
		return cogs::is_greater_than(m_contents, src);
	}

	// is_less_than_or_equal
	bool operator<=(this_t& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(const this_t& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(volatile this_t& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(const volatile this_t& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }

	bool operator<=(this_t& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(const this_t& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(volatile this_t& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	bool operator<=(const volatile this_t& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }

	bool operator<=(this_t&& src) const { return cogs::is_less_than_or_equal(m_contents, std::move(src.m_contents)); }
	bool operator<=(this_t&& src) const volatile { return cogs::is_less_than_or_equal(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator<=(measure<storage_type2, unit_t>& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(const measure<storage_type2, unit_t>& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(volatile measure<storage_type2, unit_t>& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(const volatile measure<storage_type2, unit_t>& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator<=(measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(const measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator<=(measure<storage_type2, unit_t>&& src) const { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator<=(measure<storage_type2, unit_t>&& src) const volatile { return cogs::is_less_than_or_equal(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator<=(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator<=(storage_type2&& src) const
	{
		return cogs::is_less_than_or_equal(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator<=(storage_type2&& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator<=(storage_type2& src) const
	{
		return cogs::is_less_than_or_equal(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator<=(storage_type2& src) const volatile
	{
		return cogs::is_less_than_or_equal(m_contents, src);
	}


	// is_greater_than_or_equal
	bool operator>=(this_t& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(const this_t& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(volatile this_t& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(const volatile this_t& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }

	bool operator>=(this_t& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(const this_t& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(volatile this_t& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	bool operator>=(const volatile this_t& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }

	bool operator>=(this_t&& src) const { return cogs::is_greater_than_or_equal(m_contents, std::move(src.m_contents)); }
	bool operator>=(this_t&& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, std::move(src.m_contents)); }


	template <typename storage_type2 = storage_t>
	bool operator>=(measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(const measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(volatile measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(const volatile measure<storage_type2, unit_t>& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator>=(measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(const measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(const volatile measure<storage_type2, unit_t>& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }

	template <typename storage_type2 = storage_t>
	bool operator>=(measure<storage_type2, unit_t>&& src) const { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }
	template <typename storage_type2 = storage_t>
	bool operator>=(measure<storage_type2, unit_t>&& src) const volatile { return cogs::is_greater_than_or_equal(m_contents, src.m_contents); }


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(const measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(const volatile measure<storage_type2, unit_type2>& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(const measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(const volatile measure<storage_type2, unit_type2>& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), src.m_contents));
	}


	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(measure<storage_type2, unit_type2>&& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t, typename unit_type2 = unit_t>
	bool operator>=(measure<storage_type2, unit_type2>&& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents,
			cogs::multiply(typename unit_conversion<unit_type2, unit_t>::ratio_const_t(), std::move(src.m_contents)));
	}

	template <typename storage_type2 = storage_t>
	bool operator>=(storage_type2&& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents, std::move(src));
	}

	template <typename storage_type2 = storage_t>
	bool operator>=(storage_type2&& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents, std::move(src));
	}


	template <typename storage_type2 = storage_t>
	bool operator>=(storage_type2& src) const
	{
		return cogs::is_greater_than_or_equal(m_contents, src);
	}

	template <typename storage_type2 = storage_t>
	bool operator>=(storage_type2& src) const volatile
	{
		return cogs::is_greater_than_or_equal(m_contents, src);
	}


	// compare
	// swap
	// exchange
	// compare_exchange

};


}



#endif
