//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXEDINT
#define COGS_HEADER_MATH_FIXEDINT

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/int_types.hpp"
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
using fixed_integer =	typename std::conditional<!bits,
							fixed_integer_native_const<false, 0, 0>,
							typename std::conditional<(!!bits && (bits <= (sizeof(longest) * 8))),
								fixed_integer_native<has_sign, ((!!bits && (bits <= (sizeof(longest) * 8))) ? bits : 1)>,
								fixed_integer_extended<has_sign, ((!!bits && (bits > (sizeof(longest) * 8))) ? bits : ((sizeof(longest) * 8) + 1))>
							>::type
						>::type;

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
	typedef const typename int_to_fixed_integer<typename std::remove_const<T>::type>::type type;
};

template <typename T>
class int_to_fixed_integer<volatile T>
{
public:
	typedef volatile typename int_to_fixed_integer<typename std::remove_volatile<T>::type>::type type;
};

template <typename T>
class int_to_fixed_integer<const volatile T>
{
public:
	typedef const volatile typename int_to_fixed_integer<typename std::remove_cv<T>::type>::type type;
};

template <>
class int_to_fixed_integer<bool>
{
public:
	typedef fixed_integer<false, 1> type;
};


template <typename numerator_t, typename denominator_t>
class fraction;


template <typename T> inline std::enable_if_t<!std::is_integral_v<std::remove_reference_t<T> >, T&&>
reduce_integer_type(T&& t) { return std::forward<T>(t); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, std::remove_cv_t<T> >
reduce_integer_type(T& t) { return load(t); }

template <bool has_sign, size_t n_bits>
auto reduce_integer_type(const fixed_integer_native<has_sign, n_bits>& t) { return t.get_int(); }

template <bool has_sign, size_t n_bits>
auto reduce_integer_type(const volatile fixed_integer_native<has_sign, n_bits>& t) { return t.get_int(); }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
auto reduce_integer_type(const fixed_integer_native_const<has_sign, bits, value>& t) { return value; }

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
auto reduce_integer_type(const volatile fixed_integer_native_const<has_sign, bits, value>& t) { return value; }



}


#endif
