//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXEDINT
#define COGS_HEADER_MATH_FIXEDINT

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/load.hpp"


namespace cogs {


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
class fixed_integer_native_const;

template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;

template <bool has_sign, size_t n_bits>
class fixed_integer_native;

template <bool has_sign, size_t n_bits>
class fixed_integer_extended;

template <bool has_sign, size_t n_bits>
class fixed_integer_extended_content;

template <bool has_sign, size_t bits>
using fixed_integer = std::conditional_t<!bits,
							fixed_integer_native_const<false, 0, 0>,
							std::conditional_t<(!!bits && (bits <= (sizeof(longest) * 8))),
								fixed_integer_native<has_sign, ((!!bits && (bits <= (sizeof(longest) * 8))) ? bits : 1)>,
								fixed_integer_extended<has_sign, ((!!bits && (bits > (sizeof(longest) * 8))) ? bits : ((sizeof(longest) * 8) + 1))>
							>
						>;

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > >
class int_to_fixed_integer
{
public:
	typedef fixed_integer_native<std::is_signed_v<T>, 8 * sizeof(T)> type;
};
template <typename T>
using int_to_fixed_integer_t = typename int_to_fixed_integer<T>::type;

template <typename T>
class int_to_fixed_integer<const T>
{
public:
	typedef const int_to_fixed_integer_t<std::remove_const_t<T> > type;
};

template <typename T>
class int_to_fixed_integer<volatile T>
{
public:
	typedef volatile int_to_fixed_integer_t<std::remove_volatile_t<T> > type;
};

template <typename T>
class int_to_fixed_integer<const volatile T>
{
public:
	typedef const volatile int_to_fixed_integer_t<std::remove_cv_t<T> > type;
};

template <>
class int_to_fixed_integer<bool>
{
public:
	typedef fixed_integer<false, 1> type;
};

template <typename T>
using int_to_fixed_integer_t = typename int_to_fixed_integer<T>::type;


template <typename T> inline std::enable_if_t<std::is_integral_v<T>, int_to_fixed_integer_t<T> >
make_fixed_integer(const T& t)
{
	int_to_fixed_integer_t<T> result(t);
	return result;
}

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, int_to_fixed_integer_t<T> >
make_fixed_integer(const volatile T& t)
{
	int_to_fixed_integer_t<T> result(load(t));
	return result;
}


template <typename numerator_t, typename denominator_t>
class fraction;


template <typename T> inline constexpr std::enable_if_t<!std::is_integral_v<std::remove_reference_t<T> >, T&&>
reduce_integer_type(T&& t) { return std::forward<T>(t); }

template <typename T> inline constexpr std::enable_if_t<std::is_integral_v<T>, T>
reduce_integer_type(T& t) { return t; }

template <typename T> inline constexpr std::enable_if_t<std::is_integral_v<T>, T>
reduce_integer_type(const T& t) { return t; }

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, T>
reduce_integer_type(volatile T& t) { return load(t); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, T>
reduce_integer_type(const volatile T& t) { return load(t); }

template <bool has_sign, size_t n_bits>
auto reduce_integer_type(const fixed_integer_native<has_sign, n_bits>& t) { return t.get_int(); }

template <bool has_sign, size_t n_bits>
auto reduce_integer_type(const volatile fixed_integer_native<has_sign, n_bits>& t) { return t.get_int(); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
constexpr auto reduce_integer_type(const fixed_integer_native_const<has_sign, bits, value>& t) { return value; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
constexpr auto reduce_integer_type(const volatile fixed_integer_native_const<has_sign, bits, value>& t) { return value; }


template <int i>
class int_to_fixed_integer_const
{
public:
	typedef fixed_integer_native_const<(i < 0), range_to_bits_v<((i < 0) ? i : 0), ((i < 0) ? 0 : i)>, i> type;
};
template <int i>
using int_to_fixed_integer_const_t = typename int_to_fixed_integer_const<i>::type;


typedef int_to_fixed_integer_const_t<0> zero_t;
typedef int_to_fixed_integer_const_t<1> one_t;
typedef int_to_fixed_integer_const_t<-1> negative_one_t;


}


#endif
