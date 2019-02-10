//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_TIME
#define COGS_TIME

#include <type_traits>

#include "cogs/collections/string.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/math/chars.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/measurement_types.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified



/// @defgroup UnitBases Unit Bases
/// @{
/// @ingroup Math
/// @brief Unit Bases, used to convert values.
/// @}

/// @defgroup Time Time
/// @{
/// @ingroup UnitBases
/// @brief Time Unit Bases
/// @}

/// @ingroup Time
/// @brief Nanoseconds unit base
class nanoseconds : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n, unsigned int radix = 10, size_t minDigits = 1)
	{
		static constexpr char_t part2[] = { (char_t)'n', (char_t)'s' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>(radix, minDigits));
		result += string_t<char_t>::contain(part2, 2);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief 100-Nanosecond increments unit base
class nano100s : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		return nanoseconds::to_string_t<char_t, unit_t>(n * 100);
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Microseconds unit base
class microseconds : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { special_characters<char_t>::MU, (char_t)'s' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 2);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Milliseconds unit base
class milliseconds : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { (char_t)'m', (char_t)'s' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 2);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Seconds unit base
class seconds : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { (char_t)'s' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Minutes unit base
class minutes : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[4] = { (char_t)'m', (char_t)'i', (char_t)'n' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Hours unit base
class hours : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { (char_t)'h' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Days unit base
class days : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { (char_t)'d' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};

/// @ingroup Time
/// @brief Weeks unit base
class weeks : public unit_type<time>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2[2] = { (char_t)'w' };
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contain(part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)	{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};


// Always specialize unit_conversion with the primary unit type as the second template parameter
template<>
class unit_conversion<nanoseconds, seconds>
{
public:
	typedef fixed_integer_native_const<false, 1, 1> numerator_const_t;
	typedef fixed_integer_native_const<false, 30, 1000000000> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};

template<>
class unit_conversion<nano100s, seconds>
{
public:
	typedef fixed_integer_native_const<false, 1, 1> numerator_const_t;
	typedef fixed_integer_native_const<false, 24, 10000000> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};


template<>
class unit_conversion<microseconds, seconds>
{
public:
	typedef fixed_integer_native_const<false, 1, 1> numerator_const_t;
	typedef fixed_integer_native_const<false, 20, 1000000> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};

template<>
class unit_conversion<milliseconds, seconds>
{
public:
	typedef fixed_integer_native_const<false, 1, 1> numerator_const_t;
	typedef fixed_integer_native_const<false, 10, 1000> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};

template<>
class unit_conversion<minutes, seconds>
{
public:
	typedef fixed_integer_native_const<false, 6, 60> numerator_const_t;
	typedef fixed_integer_native_const<false, 1, 1> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};

template<>
class unit_conversion<hours, seconds>
{
public:
	typedef fixed_integer_native_const<false, 12, 3600> numerator_const_t;
	typedef fixed_integer_native_const<false, 1, 1> denominator_const_t;
	typedef decltype(std::declval<numerator_const_t>() / std::declval<denominator_const_t>()) ratio_const_t;
};


#pragma warning(pop)

}



#endif
