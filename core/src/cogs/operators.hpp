//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OPERATORS
#define COGS_HEADER_OPERATORS


#include <array>
#include <cmath>
#include <numeric>
#include <type_traits>
#include <algorithm>
//#include <charconv>
#include <string>

#include "cogs/mem/bypass_strict_aliasing.hpp"
#include "cogs/assign.hpp"
#include "cogs/load.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/env/sync/atomic_operators.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/sync/atomic_load.hpp"
#include "cogs/sync/atomic_store.hpp"
#include "cogs/sync/atomic_exchange.hpp"
#include "cogs/sync/atomic_compare_exchange.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/is_integral.hpp"
#include "cogs/math/fixed_integer.hpp"
#include "cogs/env/mem/bit_scan.hpp"
#include "cogs/env/mem/bit_count.hpp"


namespace cogs
{


// is_array		- matches T[n], or std::array<T, n>
template <typename T, typename enable = void> struct is_array { static constexpr bool value = std::is_array_v<T>; };
template <typename T, size_t n> struct is_array<std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array<const std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array<volatile std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array<const volatile std::array<T, n> > : std::true_type {};
template <typename T> constexpr bool is_array_v = is_array<T>::value;

// is_array_type		- matches T[n], std::array<T, n>, or array_view<n, T>
template <typename T, typename enable = void> struct is_array_type { static constexpr bool value = is_array_v<T>; };
template <typename T, size_t n> struct is_array_type<std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array_type<const std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array_type<volatile std::array<T, n> > : std::true_type {};
template <typename T, size_t n> struct is_array_type<const volatile std::array<T, n> > : std::true_type {};
template <typename T> constexpr bool is_array_type_v = is_array_type<T>::value;

// is_reference_container - Indicates if a collection type will (or may) actually contain references to elements in another collection.
template <typename T, typename enable = void> struct is_reference_container { static constexpr bool value = false; };
template <typename T> constexpr bool is_reference_container_v = is_reference_container<T>::value;

// extent			- same as std::extent, but also supports std::array<T, n>, array_view<n, T>
template<class T, unsigned N = 0, typename enable = void> struct extent : std::integral_constant<size_t, 0> {};
template<class T> struct extent<T[], 0> : std::integral_constant<size_t, 0> {};
template<class T, unsigned N> struct extent<T[], N> : extent<T, N - 1> {};
template<class T, size_t I> struct extent<T[I], 0> : std::integral_constant<size_t, I> {};
template<class T, size_t I, unsigned N> struct extent<T[I], N> : std::extent<T, N - 1> {};
template <typename T, size_t n> struct extent<std::array<T, n>, 0> : std::integral_constant<size_t, n> {};
template <typename T, size_t n> struct extent<const std::array<T, n>, 0> : std::integral_constant<size_t, n> {};
template <typename T, size_t n> struct extent<volatile std::array<T, n>, 0> : std::integral_constant<size_t, n> {};
template <typename T, size_t n> struct extent<const volatile std::array<T, n>, 0> : std::integral_constant<size_t, n> {};
template <typename T, size_t n, unsigned N> struct extent<std::array<T, n>, N> : extent<T, N - 1> {};
template <typename T, size_t n, unsigned N> struct extent<const std::array<T, n>, N> : extent<T, N - 1> {};
template <typename T, size_t n, unsigned N> struct extent<volatile std::array<T, n>, N> : extent<T, N - 1> {};
template <typename T, size_t n, unsigned N> struct extent<const volatile std::array<T, n>, N> : extent<T, N - 1> {};
template <typename T, unsigned N = 0> constexpr size_t extent_v = extent<T, N>::value;

template <typename T, typename enable = void> struct remove_extent { typedef std::remove_extent_t<T> type; };
template <typename T, size_t n> struct remove_extent<std::array<T, n> > { public: typedef T type; };
template <typename T, size_t n> struct remove_extent<const std::array<T, n> > { public: typedef const T type; };
template <typename T, size_t n> struct remove_extent<volatile std::array<T, n> > { public: typedef volatile T type; };
template <typename T, size_t n> struct remove_extent<const volatile std::array<T, n> > { public: typedef const volatile T type; };
template <typename T> using remove_extent_t = typename remove_extent<T>::type;

template <typename T, typename enable = void> struct remove_all_extents { typedef T type; };
template <typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

template<class T> struct remove_all_extents<T[]> { typedef remove_all_extents_t<T> type; };
template<class T, size_t I> struct remove_all_extents<T[I]> { typedef remove_all_extents_t<T> type; };


#define COGS_DEFINE_UNARY_OPERATOR_FOR_MEMBER_OPERATOR(name, pre, post)	\
	template <typename T> inline std::enable_if_t<std::is_class_v<T>, decltype(pre(std::declval<T&>())post)> name(T& t) { return pre(t)post; }

#define COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(name)\
	template <typename T, typename... args_t>\
	inline std::enable_if_t<std::is_class_v<T>, decltype(std::declval<T&>().name(std::declval<args_t>()...))>\
	name(T& t, args_t&&... args) { return t.name(std::forward<args_t>(args)...); }\
	template <typename T, typename... args_t>\
	inline std::enable_if_t<std::is_class_v<T>, decltype(std::declval<T&>().name(std::declval<args_t>()...))>\
	name(const T& t, args_t&&... args) { return t.name(std::forward<args_t>(args)...); }\

#define COGS_DEFINE_UNARY_ASSIGN_OPERATORS(name)\
	template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, void>\
	assign_ ## name(T& t) { t = name(t); }\
	template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	pre_assign_ ## name(T& t) { t = name(t); return t; }\
	template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	post_assign_ ## name(T& t) { T tmp(t); t = name(t); return tmp; }\

#define COGS_DEFINE_UNARY_OPERATOR(name, symbol)\
	template <typename T> inline std::enable_if_t<!std::is_class_v<T>, decltype(symbol(std::declval<T&>()))> name(T& t) { return symbol(load(t)); }\
	COGS_DEFINE_UNARY_OPERATOR_FOR_MEMBER_OPERATOR(name, symbol, )\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(pre_assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(post_assign_ ## name)\
	COGS_DEFINE_UNARY_ASSIGN_OPERATORS(name)\

#define COGS_DEFINE_UNARY_OPERATOR_FROM_PRE_AND_POST(name, symbol)\
	template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, std::remove_cv_t<T> > name(T& t) { std::remove_cv_t<T> tmp(load(t)); return symbol(tmp); }\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(assign_ ## name)\
	COGS_DEFINE_UNARY_OPERATOR_FOR_MEMBER_OPERATOR(pre_assign_ ## name, symbol, )\
	COGS_DEFINE_UNARY_OPERATOR_FOR_MEMBER_OPERATOR(post_assign_ ## name, , symbol)\
	COGS_DEFINE_UNARY_ASSIGN_OPERATORS(name)\

#define COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(pre_assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(post_assign_ ## name)\
	COGS_DEFINE_UNARY_ASSIGN_OPERATORS(name)\

#define COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(name, symbol)	\
	template <typename T, typename A1> inline std::enable_if_t<std::is_class_v<T>,\
	decltype(std::declval<T&>() symbol std::declval<A1>())>\
	name(T& t, A1&& a) { return t symbol std::forward<A1>(a); }\
	template <typename T, typename A1> inline std::enable_if_t<std::is_class_v<T>,\
	decltype(std::declval<const T&>() symbol std::declval<A1>())>\
	name(const T& t, A1&& a) { return t symbol std::forward<A1>(a); }\

#define COGS_DEFINE_BINARY_ASSIGN_OPERATORS_FROM_FUNCTION(name)\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, void>\
	assign_ ## name(T& t, A1&& a) { t = name(t, std::forward<A1>(a)); }\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	pre_assign_ ## name(T& t, A1&& a) { t = name(t, std::forward<A1>(a)); return t; }\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	post_assign_ ## name(T& t, A1&& a) { std::remove_cv_t<T> tmp(t); t = name(t, std::forward<A1>(a)); return tmp; }\

#define COGS_DEFINE_BINARY_ASSIGN_OPERATORS_FROM_OPERATOR(name, symbol)\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, void>\
	assign_ ## name(T& t, A1&& a) { t symbol ## = std::forward<A1>(a); }\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	pre_assign_ ## name(T& t, A1&& a) { t symbol ## = std::forward<A1>(a); return t; }\
	template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>\
	post_assign_ ## name(T& t, A1&& a) { std::remove_cv_t<T> tmp(t); t symbol ## = std::forward<A1>(a); return tmp; }\

#define COGS_DEFINE_BINARY_OPERATOR(name, symbol)\
	COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(name, symbol)\
	COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(assign_ ## name, symbol ## =)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(pre_assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(post_assign_ ## name)\
	COGS_DEFINE_BINARY_ASSIGN_OPERATORS_FROM_OPERATOR(name, symbol )\

#define COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(pre_assign_ ## name)\
	COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(post_assign_ ## name)\
	COGS_DEFINE_BINARY_ASSIGN_OPERATORS_FROM_FUNCTION(name)\



template <class T, class M>
inline std::enable_if_t<!std::is_reference_v<T>, std::remove_reference_t<M>&&>
forward_member(M&& m) { return std::move(m); }

template<class T, class M>
inline std::enable_if_t<std::is_reference_v<T>, std::remove_reference_t<M>&>
forward_member(M&& m) { return m; }



// array_to_args
template <size_t i, typename T, typename enable = void>
struct array_to_args_helper;

template <size_t i, typename T>
struct array_to_args_helper<i, T, std::enable_if_t<!is_reference_container_v<std::remove_reference_t<T> > > >
{
	template <typename F, typename... args_t>
	static decltype(auto) expand(F&& f, T&& t, args_t&&... a)
	{ return array_to_args_helper<i - 1, T>::expand(std::forward<F>(f), std::forward<T>(t), forward_member<T>(t[i - 1]), std::forward<args_t>(a)...); }
};

template <size_t i, typename T>
struct array_to_args_helper<i, T, std::enable_if_t<is_reference_container_v<std::remove_reference_t<T> > > >
{
	template <typename F, typename... args_t>
	static decltype(auto) expand(F&& f, T&& t, args_t&&... a)
	{ return array_to_args_helper<i - 1, T>::expand(std::forward<F>(f), std::forward<T>(t), t[i - 1], std::forward<args_t>(a)...); }
};

template <typename T>
struct array_to_args_helper<0, T, void>
{
	template <typename F, typename... args_t>
	static decltype(auto) expand(F&& f, T&&, args_t&&... a)
	{ return f(std::forward<args_t>(a)...); }
};

template <typename F, typename T>
inline decltype(auto) array_to_args(F&& f, T&& t)
{ return array_to_args_helper<extent_v<T>, T>::expand(std::forward<F>(f), std::forward<T>(t)); }




// unary_array_operation_to_args
template <size_t i, typename T, typename enable = void>
struct unary_array_operation_to_args_helper;

template <size_t i, typename T>
struct unary_array_operation_to_args_helper<i, T, std::enable_if_t<!is_reference_container_v<std::remove_reference_t<T> > > >
{
	template <typename F, typename U, typename... args_t>
	static decltype(auto) expand(F&& f, U&& u, T&& t, args_t&&... a)
	{ return unary_array_operation_to_args_helper<i - 1, T>::expand(std::forward<F>(f), std::forward<U>(u), std::forward<T>(t), u(forward_member<T>(t[i - 1])), std::forward<args_t>(a)...); }
};

template <size_t i, typename T>
struct unary_array_operation_to_args_helper<i, T, std::enable_if_t<is_reference_container_v<std::remove_reference_t<T> > > >
{
	template <typename F, typename U, typename... args_t>
	static decltype(auto) expand(F&& f, U&& u, T&& t, args_t&&... a)
	{ return unary_array_operation_to_args_helper<i - 1, T>::expand(std::forward<F>(f), std::forward<U>(u), std::forward<T>(t), u(t[i - 1]), std::forward<args_t>(a)...); }
};

template <typename T>
struct unary_array_operation_to_args_helper<0, T, void>
{
	template <typename F, typename U, typename... args_t>
	static decltype(auto) expand(F&& f, U&& u, T&&, args_t&&... a)
	{ return f(std::forward<args_t>(a)...); }
};

template <typename F, typename U, typename T>
inline decltype(auto) unary_array_operation_to_args(F&& f, U&& u, T&& t)
{ return unary_array_operation_to_args_helper<extent_v<T>, T>::expand(std::forward<F>(f), std::forward<U>(u), std::forward<T>(t)); }




// binary_array_operation_to_args
template <size_t i, typename T, typename T2, typename enable = void>
struct binary_array_operation_to_args_helper;

template <size_t i, typename T, typename T2>
struct binary_array_operation_to_args_helper<i, T, T2, std::enable_if_t<!is_reference_container_v<std::remove_reference_t<T> > && !is_reference_container_v<std::remove_reference_t<T2> > > >
{
	template <typename F, typename B, typename... args_t>
	static decltype(auto) expand(F&& f, B&& b, T&& t, T2&& t2, args_t&&... a)
	{ return binary_array_operation_to_args_helper<i - 1, T, T2>::expand(std::forward<F>(f), std::forward<B>(b), std::forward<T>(t), b(forward_member<T>(t[i - 1]), forward_member<T2>(t2[i - 1])), std::forward<args_t>(a)...); }
};

template <size_t i, typename T, typename T2>
struct binary_array_operation_to_args_helper<i, T, T2, std::enable_if_t<is_reference_container_v<std::remove_reference_t<T> > && !is_reference_container_v<std::remove_reference_t<T2> >  > >
{
	template <typename F, typename B, typename... args_t>
	static decltype(auto) expand(F&& f, B&& b, T&& t, T2&& t2, args_t&&... a)
	{ return binary_array_operation_to_args_helper<i - 1, T, T2>::expand(std::forward<F>(f), std::forward<B>(b), std::forward<T>(t), b(t[i - 1], forward_member<T2>(t2[i - 1])), std::forward<args_t>(a)...); }
};

template <size_t i, typename T, typename T2>
struct binary_array_operation_to_args_helper<i, T, T2, std::enable_if_t<!is_reference_container_v<std::remove_reference_t<T> > && is_reference_container_v<std::remove_reference_t<T2> > > >
{
	template <typename F, typename B, typename... args_t>
	static decltype(auto) expand(F&& f, B&& b, T&& t, T2&& t2, args_t&&... a)
	{ return binary_array_operation_to_args_helper<i - 1, T, T2>::expand(std::forward<F>(f), std::forward<B>(b), std::forward<T>(t), b(forward_member<T>(t[i - 1]), t2[i - 1]), std::forward<args_t>(a)...); }
};

template <size_t i, typename T, typename T2>
struct binary_array_operation_to_args_helper<i, T, T2, std::enable_if_t<is_reference_container_v<std::remove_reference_t<T> > && is_reference_container_v<std::remove_reference_t<T2> >  > >
{
	template <typename F, typename B, typename... args_t>
	static decltype(auto) expand(F&& f, B&& b, T&& t, T2&& t2, args_t&&... a)
	{ return binary_array_operation_to_args_helper<i - 1, T, T2>::expand(std::forward<F>(f), std::forward<B>(b), std::forward<T>(t), b(t[i - 1], t2[i - 1]), std::forward<args_t>(a)...); }
};


template <typename T, typename T2>
struct binary_array_operation_to_args_helper<0, T, T2, void>
{
	template <typename F, typename B, typename... args_t>
	static decltype(auto) expand(F&& f, B&& b, T&&, T2&&, args_t&&... a)
	{ return f(std::forward<args_t>(a)...); }
};

template <typename F, typename B, typename T, typename T2>
inline decltype(auto) binary_array_operation_to_args(F&& f, B&& b, T&& t, T2&& t2, std::enable_if_t<(extent_v<T> == extent_v<T2>), int> = 0)
{ return binary_array_operation_to_args_helper<extent_v<T>, T, T2>::expand(std::forward<F>(f), std::forward<B>(b), std::forward<T>(t)); }





template <size_t i, typename... args_t>
struct do_for_each_arg_helper
{
};

template <size_t i>
struct do_for_each_arg_helper<i>
{
	template <typename F> static void f(F&& f2) { }
};

template <size_t i, typename T, typename... args_t>
struct do_for_each_arg_helper<i, T, args_t...>
{
	template <typename F>
	static void f(F&& f2, T&& t, args_t&&... a)
	{
		f2(i, std::forward<T>(t));
		do_for_each_arg_helper<i + 1, args_t...>::f(std::forward<F>(f2), std::forward<args_t>(a)...);
	}
};


template <typename F, typename... args_t>
void do_for_each_arg(F&& f2, args_t&&... a)
{
	do_for_each_arg_helper<0, args_t...>::f(std::forward<F>(f2), std::forward<args_t>(a)...);
}




// not

template <typename T> inline std::enable_if_t<!std::is_class_v<T>, decltype(!(std::declval<T&>()))> lnot(T& t) { return !(load(t)); }
COGS_DEFINE_UNARY_OPERATOR_FOR_MEMBER_OPERATOR(lnot, !, )
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(assign_not)
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(pre_assign_not)
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(post_assign_not)
template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, void>
assign_not(T& t) { t = lnot(t); }
template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>
pre_assign_not(T& t) { t = lnot(t); return t; }
template <typename T> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, T>
post_assign_not(T& t) { T tmp(t); t = lnot(t); return tmp; }


COGS_DEFINE_UNARY_OPERATOR(bit_not, ~)

template <typename T> inline std::enable_if_t<std::is_arithmetic_v<T> && std::is_signed_v<T>, bool>
is_negative(const T& t) { return load(t) < (std::remove_volatile_t<T>)0; }

template <typename T> inline std::enable_if_t<std::is_arithmetic_v<T> && !std::is_signed_v<T>, bool>
is_negative(const T& t) { return false; }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(is_negative)

template <typename T> inline std::enable_if_t<std::is_integral_v<T> && !std::is_signed_v<T> && !std::is_volatile_v<T>, bool>
is_exponent_of_two(const T& t) { decltype(auto) tmp(load(t)); return (tmp != 0) && ((tmp & (T)-(std::make_signed_t<T>)tmp) == tmp); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T> && !std::is_signed_v<T> && std::is_volatile_v<T>, bool>
is_exponent_of_two(const T& t) { return is_exponent_of_two(load(t)); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, bool>
is_exponent_of_two(const T& t) { return is_exponent_of_two(abs(t)); }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(is_exponent_of_two)

template <typename T> constexpr std::enable_if_t<std::is_integral_v<T>, bool>
has_fractional_part(const T& t) { return false; }

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, decltype(std::modf(std::declval<std::remove_cv_t<T> >(), nullptr))>
has_fractional_part(const T& t) { return std::modf(load(t), nullptr) != 0.0; }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(has_fractional_part)

template <typename T> inline std::enable_if_t<std::is_arithmetic_v<T> && !std::is_signed_v<T>, std::remove_volatile_t<T> >
abs(const T& t) { return load(t); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, std::make_unsigned_t<std::conditional_t<std::is_integral_v<T>, T, int> > >
abs(const T& t) { return (std::make_unsigned_t<T>)(is_negative(t) ? -t : t); }

template <typename T> inline std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, std::make_unsigned_t<std::conditional_t<std::is_integral_v<T>, T, int> > >
abs(const volatile T& t) { return abs(load(t)); }

template <typename T> inline std::enable_if_t< std::is_floating_point_v<T>, T>
abs(const T& t) { return std::fabs(t); }

template <typename T> inline std::enable_if_t< std::is_floating_point_v<T>, T>
abs(const volatile T& t) { return abs(load(t)); }


COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(abs)


COGS_DEFINE_UNARY_OPERATOR(negative, -)

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(bit_count)
	// bit_count is defined in env/mem/bit_count.hpp , as there are efficient implementations as arch/os/env intrincs

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(bit_scan_forward)
	// bit_scan_forward is defined in env/mem/bit_scan.hpp , as there are efficient implementations as arch/os/env intrincs

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(bit_scan_reverse)
	// bit_scan_reverse is defined in env/mem/bit_scan.hpp , as there are efficient implementations as arch/os/env intrincs

COGS_DEFINE_UNARY_OPERATOR_FROM_PRE_AND_POST(next, ++)
COGS_DEFINE_UNARY_OPERATOR_FROM_PRE_AND_POST(prev, --)


template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(char)),
	std::remove_volatile_t<T> >
endian_swap(const T& t)
{
	return load(t);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(char))
	&& (sizeof(T) <= sizeof(short)),
	std::remove_volatile_t<T> >
endian_swap(const T& t)
{
	decltype(auto) tmp(load(t));
	return (tmp << 8) | (tmp >> 8);
}

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(short))
	&& (sizeof(T) <= sizeof(long)),
	std::remove_volatile_t<T> >
endian_swap(const T& t)
{
	decltype(auto) tmp(load(t));
	return (tmp >> 24)
		| (tmp << 24)
		| ((tmp & ((unsigned long)0xFF << 8)) << 8)
		| ((tmp & ((unsigned long)0xFF << 16)) >> 8);
}

#if COGS_LONGEST_INT >= 8

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(long))
	&& (sizeof(T) <= sizeof(long long)),
	std::remove_volatile_t<T> >
endian_swap(const T& t)
{
	decltype(auto) tmp(load(t));
	return (tmp << 56)
		| (tmp >> 56)
		| ((tmp & ((unsigned long long)0xFF << 8)) << 40)
		| ((tmp & ((unsigned long long)0xFF << 16)) << 24)
		| ((tmp & ((unsigned long long)0xFF << 24)) << 8)
		| ((tmp & ((unsigned long long)0xFF << 32)) >> 8)
		| ((tmp & ((unsigned long long)0xFF << 40)) >> 24)
		| ((tmp & ((unsigned long long)0xFF << 48)) >> 40);
}

#if COGS_LONGEST_INT >= 16

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(long long))
	&& (sizeof(T) <= sizeof(bits_to_uint_t<128>)),
	std::remove_volatile_t<T> >
endian_swap(const T& t)
{
	decltype(auto) tmp(load(t));
	return (tmp << 120)
		| (tmp >> 120)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 8)) << 104)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 16)) << 88)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 24)) << 72)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 32)) << 56)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 40)) << 40)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 48)) << 24)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 56)) << 8)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 64)) >> 8)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 72)) >> 24)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 80)) >> 40)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 88)) >> 56)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 96)) >> 72)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 104)) >> 88)
		| ((tmp & ((bits_to_uint_t<128>)0xFF << 112)) >> 104);
}

#endif
#endif

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(endian_swap)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T>, decltype(load(std::declval<T&>()) << reduce_integer_type(std::declval<A1>())) >
bit_shift_left(T& t, A1&& a) { return load(t) << reduce_integer_type(std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(bit_shift_left, << )

template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T>, decltype(load(std::declval<T&>()) >> reduce_integer_type(std::declval<A1>())) >
bit_shift_right(const T& t, A1&& a) { return load(t) >> reduce_integer_type(std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(bit_shift_right, >> )


COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(bit_rotate_left)
	// bit_rotate_left is defined in env/bit_rotate.hpp , as there are efficient implementations as arch/os/env intrincs

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(bit_rotate_right)
	// bit_rotate_right is defined in env/bit_rotate.hpp , as there are efficient implementations as arch/os/env intrincs


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T>, decltype(load(std::declval<T&>()) & reduce_integer_type(std::declval<A1>())) >
bit_and(const T& t, A1&& a) { return load(t) & reduce_integer_type(std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(bit_and, & )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T>, decltype(load(std::declval<T&>()) | reduce_integer_type(std::declval<A1>())) >
bit_or(const T& t, A1&& a) { return load(t) | reduce_integer_type(std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(bit_or, | )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T>, decltype(load(std::declval<T&>()) ^ reduce_integer_type(std::declval<A1>())) >
bit_xor(const T& t, A1&& a) { return load(t) ^ reduce_integer_type(std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(bit_xor, ^ )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<const A1&>() + std::declval<const T&>())>
add(const T& t, const A1& a) { return a + t; }

template <typename T, typename A1>
inline std::enable_if_t<
	((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
		|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
		|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
		),
	decltype(std::declval<std::remove_volatile_t<T> >() + std::declval<std::remove_volatile_t<A1> >())>
add(const T& t, const A1& a)
{
	return load(t) + load(a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	(std::is_pointer_v<T> && std::is_integral_v<A1>) || (std::is_pointer_v<A1> && std::is_integral_v<T>),
	decltype(std::declval<const std::remove_volatile_t<T>&>() + std::declval<const std::remove_volatile_t<A1>&>())>
add(const T& t, const A1& a)
{
	return load(t) + load(a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) < sizeof(longest),
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, std::is_signed_v<T> || std::is_signed_v<A1> > >
add(const T& t, const A1& a)
{
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, std::is_signed_v<T> || std::is_signed_v<A1> > result(load(t));
	result += load(a);
	return result;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest),
	fixed_integer<std::is_signed_v<T> || std::is_signed_v<A1>, (sizeof(longest) * 8) + 1 > >
add(const T& t, const A1& a);
//{
//	fixed_integer<std::is_signed_v<T> || std::is_signed_v<A1>, (sizeof(longest) * 8) + 1 > result;
//	result.add(
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)),
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)));	// fixed_integer_extended, 2-arg version of add
//	return result;
//}

COGS_DEFINE_BINARY_OPERATOR(add, + )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().inverse_subtract(std::declval<T&>()))>
subtract(const T& t, const A1& a) { return a.inverse_subtract(t); }


template <typename T, typename A1>
inline std::enable_if_t<
	((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
		|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
		|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
		),
	decltype(std::declval<std::remove_volatile_t<T> >() - std::declval<std::remove_volatile_t<A1> >())>
subtract(const T& t, const A1& a)
{
	return load(t) - load(a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_pointer_v<T>
	&& (std::is_integral_v<A1> || std::is_pointer_v<A1>),
	decltype(std::declval<const std::remove_volatile_t<T>&>() - std::declval<const std::remove_volatile_t<A1>&>())>
subtract(const T& t, const A1& a)
{
	return load(t) - load(a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) < sizeof(longest)),
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, true> >
subtract(const T& t, const A1& a)
{
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, true> result(load(t));
	result -= load(a);
	return result;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest)),
	fixed_integer<true, (sizeof(longest) * 8) + 1> >
subtract(const T& t, const A1& a);
//{
//	fixed_integer<true, (sizeof(longest) * 8) + 1> result;
//	result.subtract(
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)),
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)));	// fixed_integer_extended, 2-arg version of subtract
//	return result;
//}

COGS_DEFINE_BINARY_OPERATOR(subtract, -)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>() - std::declval<T&>())>
inverse_subtract(const T& t, const A1& a) { return a - t; }

template <typename T, typename A1>
inline std::enable_if_t<
((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
	|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
	|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
	),
	decltype(std::declval<std::remove_volatile_t<A1> >() - std::declval<std::remove_volatile_t<T> >())>
inverse_subtract(const T& t, const A1& a)
{
	return load(a) - load(t);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_pointer_v<A1>
	&& (std::is_integral_v<T> || std::is_pointer_v<T>),
	decltype(std::declval<const std::remove_volatile_t<A1>&>() - std::declval<const std::remove_volatile_t<T>&>())>
inverse_subtract(const T& t, const A1& a)
{
	return load(a) - load(t);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) < sizeof(longest)),
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, true> >
inverse_subtract(const T& t, const A1& a)
{
	bits_to_int_t<(((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) * 8) + 1, true> result(load(a));
	result -= load(t);
	return result;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& (((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)) == sizeof(longest)),
	fixed_integer<true, (sizeof(longest) * 8) + 1> >
inverse_subtract(const T& t, const A1& a);
//{
//	fixed_integer<true, (sizeof(longest) * 8) + 1> result;
//	result.subtract(
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)),
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)));	// fixed_integer_extended, 2-arg version of subtract
//	return result;
//}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(inverse_subtract)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<const A1&>() * std::declval<const T&>())>
multiply(const T& t, const A1& a) { return a * t; }
	


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) + sizeof(A1)) <= sizeof(longest)),
	bytes_to_int_t<(sizeof(T) + sizeof(A1)), std::is_signed_v<T> || std::is_signed_v<A1> > >
multiply(const T& t, const A1& a)
{
	bytes_to_int_t<(sizeof(T) + sizeof(A1)), std::is_signed_v<T> || std::is_signed_v<A1> > result(load(t));
	result *= load(a);
	return result;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T> && std::is_integral_v<A1>
	&& ((sizeof(T) + sizeof(A1)) > sizeof(longest)),
	fixed_integer<(std::is_signed_v<T> || std::is_signed_v<A1>), (sizeof(T) + sizeof(A1)) * 8>
>
multiply(const T& t, const A1& a);
//{
//	fixed_integer<std::is_signed_v<T> || std::is_signed_v<A1>, sizeof(T) + sizeof(A1)> result;
//	result.multiply(
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)),
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)));	// fixed_integer_extended, 2-arg version of multiply
//	return result;
//}

template <typename T, typename A1>
inline std::enable_if_t<
((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
	|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
	|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
	),
	decltype(std::declval<std::remove_volatile_t<T> >() * std::declval<std::remove_volatile_t<A1> >())>
multiply(const T& t, const A1& a)
{
	return load(t) * load(a);
}

COGS_DEFINE_BINARY_OPERATOR(multiply, * )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().inverse_modulo(std::declval<T&>()))>
modulo(const T& t, const A1& a) { return a.inverse_modulo(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
		|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
		|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
		),
	decltype(std::fmod(std::declval<std::remove_volatile_t<T>&>(), std::declval<std::remove_volatile_t<A1>&>()))>
modulo(const T& t, const A1& a)
{
	return std::fmod(load(t), load(a));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((long)-1 % (unsigned long)-1)	== 0 instead of -1
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& std::is_signed_v<T> && !std::is_signed_v<A1> && (((std::remove_volatile_t<T>)-1 % (std::remove_volatile_t<A1>)-1) == 0),
	bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<T> >
>
modulo(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return -(bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<T> >)
			((std::make_unsigned_t<std::remove_volatile_t<T> >) - t2 % a);
	return t2 % load(a);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((!std::is_signed_v<T> && !std::is_signed_v<A1>) || (std::is_signed_v<T> && !std::is_signed_v<A1> && (((std::remove_volatile_t<T>) - 1 % (std::remove_volatile_t<A1>)-1) != 0))),
	bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<T> >
>
modulo(const T& t, const A1& a)
{
	return load(t) % load(a);
}

COGS_DEFINE_BINARY_OPERATOR(modulo, % )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>() % std::declval<T&>())>
inverse_modulo(const T& t, const A1& a) { return a % t; }

template <typename T, typename A1>
inline std::enable_if_t<
	((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
		|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
		|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
		),
	decltype(std::fmod(std::declval<std::remove_volatile_t<A1> >(), std::declval<std::remove_volatile_t<T> >()))>
inverse_modulo(const T& t, const A1& a)
{
	return std::fmod(load(a), load(t));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((long)-1 % (unsigned long)-1)	== 0 instead of -1
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& std::is_signed_v<A1> && !std::is_signed_v<T> && (((std::remove_volatile_t<A1>)-1 % (std::remove_volatile_t<T>)-1) == 0),
	bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<A1> >
>
inverse_modulo(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return -(bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<A1> >)
			((std::make_unsigned_t<std::remove_volatile_t<A1> >) - a2 % t);
	return a2 % load(t);
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((!std::is_signed_v<T> && !std::is_signed_v<A1>) || (std::is_signed_v<A1> && !std::is_signed_v<T> && (((std::remove_volatile_t<A1>) - 1 % (std::remove_volatile_t<T>)-1) != 0))),
	bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<A1> >
>
inverse_modulo(const T& t, const A1& a)
{
	return load(a) % load(t);
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(inverse_modulo)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().inverse_divide(std::declval<T&>()))>
divide(const T& t, const A1& a) { return a.inverse_divide(t); }

template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<std::remove_reference_t<T> > && !std::is_class_v<std::remove_reference_t<A1> >,
	fraction<std::remove_cv_t<std::remove_reference_t<T> >, std::remove_cv_t<std::remove_reference_t<A1> > > >
divide(T&& t, A1&& a)
{ return fraction<std::remove_cv_t<std::remove_reference_t<T> >, std::remove_cv_t<std::remove_reference_t<A1> > >(
	std::forward<T>(t), std::forward<A1>(a)); }

COGS_DEFINE_BINARY_OPERATOR(divide, / )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<std::remove_reference_t<T> > && !std::is_class_v<std::remove_reference_t<A1> >,
	fraction<std::remove_cv_t<std::remove_reference_t<A1> >, std::remove_cv_t<std::remove_reference_t<T> > > >
	inverse_divide(T&& t, A1&& a)
{ return fraction<std::remove_cv_t<std::remove_reference_t<A1> >, std::remove_cv_t<std::remove_reference_t<T> > >(
	std::forward<A1>(a), std::forward<T>(t)); }

template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>() / std::declval<T&>())>
inverse_divide(const T& t, const A1& a) { return a / t; }

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(inverse_divide)

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(reciprocal)



template <typename T> inline std::enable_if_t<std::is_integral_v<T>, std::remove_volatile_t<T> >
floor(const T& t) { return load(t); }

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, decltype(std::floor(std::declval<std::remove_cv_t<T> >()))>
floor(const T& t) { return std::floor(load(t)); }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(floor)


template <typename T> inline std::enable_if_t<std::is_integral_v<T>, std::remove_volatile_t<T> >
ceil(const T& t) { return load(t); }

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, decltype(std::ceil(std::declval<std::remove_cv_t<T> >()))>
ceil(const T& t) { return std::ceil(load(t)); }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(ceil)


template <typename T> inline std::enable_if_t<std::is_integral_v<T>, std::remove_volatile_t<T> >
round(const T& t) { return load(t); }

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, decltype(std::round(std::declval<std::remove_cv_t<T> >()))>
round(const T& t) { return std::round(load(t)); }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(round)


template <typename T> constexpr std::enable_if_t<std::is_integral_v<T>, int>
fractional_part(const T& t) { return 0; }

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, decltype(std::modf(std::declval<std::remove_cv_t<T> >(), std::declval<nullptr_t>()))>
fractional_part(const T& t) { return std::modf(load(t), nullptr); }

COGS_DEFINE_UNARY_OPERATOR_FOR_FUNCTION(fractional_part)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().inverse_divide_whole(std::declval<T&>()))>
divide_whole(const T& t, const A1& a) { return a.inverse_divide_whole(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
		|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
		|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
		),
	decltype(floor(std::declval<const std::remove_volatile_t<T>&>() / std::declval<const std::remove_volatile_t<A1>&>()))>
divide_whole(const T& t, const A1& a)
{
	return std::floor(load(t) / load(a));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((long)-10 / (unsigned long)3)
// Result of (signed / unsigned) will not exceed first type
// But the compiler will promote the signed type to unsigned, and perform the operation on the wrong value.
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& std::is_signed_v<T> && !std::is_signed_v<A1> && (((std::remove_volatile_t<T>) - 10 / (std::remove_volatile_t<A1>)3) != -3),
	std::remove_volatile_t<T>
>
divide_whole(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return (std::remove_volatile_t<T>) - ((std::make_unsigned_t<std::remove_volatile_t<T> >) - t2 / a);
	return std::remove_volatile_t<T>(t2 / load(a));
}

// Result of (signed / unsigned) or (unsigned / unsigned) will not exceed first type
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((!std::is_signed_v<T> && !std::is_signed_v<A1>) || (std::is_signed_v<T> && !std::is_signed_v<A1> && (((std::remove_volatile_t<T>) - 10 / (std::remove_volatile_t<A1>)3) == -3))),
	std::remove_volatile_t<T>
>
divide_whole(const T& t, const A1& a)
{
	return std::remove_volatile_t<T>(load(t) / load(a));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((unsigned long)10 / (long)-3)
// Result should be signed, and may grow to the next larger type
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& !std::is_signed_v<T> && std::is_signed_v<A1> && (((std::remove_volatile_t<T>)10 / (std::remove_volatile_t<A1>)-3) != -3),
	bits_to_int_t<(8 * sizeof(T)) + 1, true>
>
divide_whole(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return negative(divide_whole(t, (std::make_unsigned_t<std::remove_volatile_t<A1> >)-a2));
	return load(t) / a2;
}

// if (unsigned / signed), it may grow a bit, or signed / signed, it may grow a bit
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((std::is_signed_v<T> && std::is_signed_v<A1>) || (!std::is_signed_v<T> && std::is_signed_v<A1> && (((std::remove_volatile_t<T>)10 / (std::remove_volatile_t<A1>) - 3) == -3)))
	&& (sizeof(T) < sizeof(longest)),
	bits_to_int_t<(8 * sizeof(T)) + 1, true>
>
divide_whole(const T& t, const A1& a)
{
	return (bits_to_int_t<(8 * sizeof(T)) + 1, true>)load(t) / load(a);
}

// if (unsigned / signed), or (signed / signed), it may grow a bit
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((std::is_signed_v<T> && std::is_signed_v<A1>) || (!std::is_signed_v<T> && std::is_signed_v<A1> && (((std::remove_volatile_t<T>)10 / (std::remove_volatile_t<A1>) - 3) == -3)))
	&& (sizeof(T) == sizeof(longest)),
	fixed_integer<true, (8 * sizeof(longest)) + 1>
>
divide_whole(const T& t, const A1& a);
//{
//	fixed_integer<true, (8 * sizeof(longest)) + 1> result;
//	result.divide_whole(
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)),
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)));	// fixed_integer_extended, 2-arg version of divide_whole
//	return result;
//}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(divide_whole)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().divide_whole(std::declval<T&>()))>
inverse_divide_whole(const T& t, const A1& a) { return a.divide_whole(t); }

template <typename T, typename A1>
inline std::enable_if_t<
((std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
	|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
	|| (std::is_integral_v<T> && std::is_floating_point_v<A1>)
	),
	decltype(std::declval<const std::remove_volatile_t<A1>&>() / std::declval<const std::remove_volatile_t<T>&>())>
inverse_divide_whole(const T& t, const A1& a)
{
	return std::floor(load(a) / load(t));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((long)-10 / (unsigned long)3)
// Result of (signed / unsigned) will not exceed first type
// But the compiler will promote the signed type to unsigned, and perform the operation on the wrong value.
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& std::is_signed_v<A1> && !std::is_signed_v<T> && (((std::remove_volatile_t<A1>) - 10 / (std::remove_volatile_t<T>)3) != -3),
	std::remove_volatile_t<A1>
>
inverse_divide_whole(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return (std::remove_volatile_t<A1>) - ((std::make_unsigned_t<std::remove_volatile_t<A1> >) - a2 / t);
	return std::remove_volatile_t<A1>(a2 / load(t));
}

// Result of (signed / unsigned) or (unsigned / unsigned) will not exceed first type
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((!std::is_signed_v<T> && !std::is_signed_v<A1>) || (std::is_signed_v<A1> && !std::is_signed_v<T> && (((std::remove_volatile_t<A1>) - 10 / (std::remove_volatile_t<T>)3) == -3))),
	std::remove_volatile_t<A1>
>
inverse_divide_whole(const T& t, const A1& a)
{
	return std::remove_volatile_t<A1>(load(a) / load(t));
}

// Addresses the issue in which the following operation provides an invalid result:
// ((unsigned long)10 / (long)-3)
// Result should be signed, and may grow to the next larger type
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& !std::is_signed_v<A1> && std::is_signed_v<T> && (((std::remove_volatile_t<A1>)10 / (std::remove_volatile_t<T>) - 3) != -3),
	bits_to_int_t<(8 * sizeof(A1)) + 1, true>
>
inverse_divide_whole(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return negative(divide_whole(a, (std::make_unsigned_t<std::remove_volatile_t<T> >) - t2));
	return load(a) / t2;
}

// if (unsigned / signed), it may grow a bit, or signed / signed, it may grow a bit
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((std::is_signed_v<T> && std::is_signed_v<A1>) || (!std::is_signed_v<A1> && std::is_signed_v<T> && (((std::remove_volatile_t<A1>)10 / (std::remove_volatile_t<T>) - 3) == -3)))
	&& (sizeof(A1) < sizeof(longest)),
	bits_to_int_t<(8 * sizeof(A1)) + 1, true>
	>
inverse_divide_whole(const T& t, const A1& a)
{
	return (bits_to_int_t<(8 * sizeof(A1)) + 1, true>)load(a) / load(t);
}

// if (unsigned / signed), or (signed / signed), it may grow a bit
template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& ((std::is_signed_v<T> && std::is_signed_v<A1>) || (!std::is_signed_v<A1> && std::is_signed_v<T> && (((std::remove_volatile_t<A1>)10 / (std::remove_volatile_t<T>) - 3) == -3)))
	&& (sizeof(A1) == sizeof(longest)),
	fixed_integer<true, (8 * sizeof(longest)) + 1>
>
inverse_divide_whole(const T& t, const A1& a);
//{
//	fixed_integer<true, (8 * sizeof(longest)) + 1> result;
//	result.divide_whole(
//		int_to_fixed_integer_t<std::remove_volatile_t<A1> >(load(a)),
//		int_to_fixed_integer_t<std::remove_volatile_t<T> >(load(t)));	// fixed_integer_extended, 2-arg version of divide_whole
//	return result;
//}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(inverse_divide_whole)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().inverse_divide_whole_and_modulo(std::declval<T&>()))>
divide_whole_and_modulo(const T& t, const A1& a) { return a.inverse_divide_whole_and_modulo(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>,
	std::pair<
	decltype(divide_whole(
		std::declval<const std::remove_volatile_t<T>&>(),
		std::declval<const std::remove_volatile_t<A1>&>())),
	decltype(modulo(
		std::declval<const std::remove_volatile_t<T>&>(),
		std::declval<const std::remove_volatile_t<A1>&>()))>
>
divide_whole_and_modulo(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	decltype(auto) a2(load(a));
	return std::make_pair(divide_whole(t2, a2), modulo(t2, a2));
}


template <typename T, typename A1>
inline std::enable_if_t<
	(std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
	|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
	|| (std::is_integral_v<T> && std::is_floating_point_v<A1>),
	std::pair<
	decltype(divide_whole(
		std::declval<const std::remove_volatile_t<T>&>(),
		std::declval<const std::remove_volatile_t<A1>&>())),
	decltype(modulo(
		std::declval<const std::remove_volatile_t<T>&>(),
		std::declval<const std::remove_volatile_t<A1>&>()))>
>
divide_whole_and_modulo(const T& t, const A1& a)
{
	auto divided = divide(t, a);
	decltype(divided) wholePart;
	auto fractionalPart = std::modf(divided, &wholePart);
	return std::make_pair(wholePart, fractionalPart);
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(divide_whole_and_modulo)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().divide_whole_and_modulo(std::declval<T&>()))>
inverse_divide_whole_and_modulo(const T& t, const A1& a) { return a.divide_whole_and_modulo(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>,
	std::pair<
	decltype(divide_whole(
		std::declval<const std::remove_volatile_t<A1>&>(),
		std::declval<const std::remove_volatile_t<T>&>())),
	decltype(modulo(
		std::declval<const std::remove_volatile_t<A1>&>(),
		std::declval<const std::remove_volatile_t<T>&>()))>
>
inverse_divide_whole_and_inverse_modulo(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	decltype(auto) a2(load(a));
	return std::make_pair(divide_whole(a2, t2), modulo(a2, t2));
}

template <typename T, typename A1>
inline std::enable_if_t<
	(std::is_floating_point_v<T> && std::is_floating_point_v<A1>)
	|| (std::is_floating_point_v<T> && std::is_integral_v<A1>)
	|| (std::is_integral_v<T> && std::is_floating_point_v<A1>),
	std::pair<
	decltype(divide_whole(
		std::declval<const std::remove_volatile_t<A1>&>(),
		std::declval<const std::remove_volatile_t<T>&>())),
	decltype(modulo(
		std::declval<const std::remove_volatile_t<A1>&>(),
		std::declval<const std::remove_volatile_t<T>&>()))>
>
inverse_divide_whole_and_inverse_modulo(const T& t, const A1& a)
{
	auto divided = divide(a, t);
	decltype(divided) wholePart;
	auto fractionalPart = std::modf(divided, &wholePart);
	return std::make_pair(wholePart, fractionalPart);
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(inverse_divide_whole_and_inverse_modulo)


template <typename T, typename A1>
inline std::enable_if_t <!std::is_class_v<T> && !std::is_volatile_v<T>,
	decltype(divide_whole(std::declval<T&>(), std::declval<A1&>()))>
divide_whole_and_assign_modulo(T& t, const A1& a)
{
	decltype(auto) tmp(divide_whole_and_modulo(t, a));
	t = tmp.second;
	return tmp.first;
}

template <typename T, typename A1>
inline std::enable_if_t <!std::is_class_v<T> && std::is_volatile_v<T>,
	decltype(divide_whole(std::declval<T&>(), std::declval<A1&>()))>
divide_whole_and_assign_modulo(T& t, const A1& a)
{
	// TODO: If any env were to provide this operation, move there
	decltype(divide_whole(std::declval<T&>(), std::declval<A1&>())) wholePart;
	atomic::compare_exchange_retry_loop(t, [&wholePart](const std::remove_volatile_t<T>& t2, const A1& a) {
			const std::remove_volatile_t<T> tmp(t2); 
			decltype(auto) tmp2(divide_whole_and_modulo(tmp, a));
			wholePart = tmp2.first;
			return tmp.second;
		});
	return wholePart;
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(divide_whole_and_assign_modulo)


template <typename T, typename A1>
inline std::enable_if_t <!std::is_class_v<T> && !std::is_volatile_v<T>,
	decltype(modulo(std::declval<T&>(), std::declval<A1&>()))>
modulo_and_assign_divide_whole(T& t, const A1& a)
{
	decltype(auto) tmp(divide_whole_and_modulo(t, a));
	t = tmp.second;
	return tmp.first;
}

template <typename T, typename A1>
inline std::enable_if_t <!std::is_class_v<T> && std::is_volatile_v<T>,
	decltype(modulo(std::declval<T&>(), std::declval<A1&>()))>
modulo_and_assign_divide_whole(T& t, const A1& a)
{
	// TODO: If any env were to provide this operation, move there
	decltype(modulo(std::declval<T&>(), std::declval<A1&>())) fractionalPart;
	atomic::compare_exchange_retry_loop(t, [&fractionalPart](const std::remove_volatile_t<T>& t2, const A1& a) {
			const std::remove_volatile_t<T> tmp(t2); 
			decltype(auto) tmp2(divide_whole_and_modulo(tmp, a));
			fractionalPart = tmp2.second;
			return tmp.first;
		});
	return fractionalPart;
}
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(modulo_and_assign_divide_whole)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
equals(T& t, const A1& a) { return a == t; }

template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
equals(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return false;
	return (std::make_unsigned_t<T>)t2 == load(a);
}

template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
equals(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return false;
	return load(t) == (std::make_unsigned_t<A1>)a2;
}

template <typename T, typename A1>
inline std::enable_if_t <
	!(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !std::is_class_v<T>
	&& !std::is_class_v<A1>,
	bool>
equals(const T& t, const A1& a)
{
	return load(t) == load(a);
}
COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(equals, == )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
not_equals(const T& t, const A1& a) { return a != t; }

template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && !std::is_class_v<A1>, bool>
not_equals(const T& t, const A1& a) { return !equals(t, a); }

COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(not_equals, != )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
is_less_than(const T& t, const A1& a) { return a > t; }


template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
is_less_than(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return true;
	return (std::make_unsigned_t<T>)t2 < load(a);
}

template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
is_less_than(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return false;
	return load(t) < (std::make_unsigned_t<T>)a2;
}

template <typename T, typename A1>
inline std::enable_if_t <
	!(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !std::is_class_v<T>
	&& !std::is_class_v<A1>,
	bool>
is_less_than(const T& t, const A1& a)
{
	return load(t) < load(a);
}

COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(is_less_than, < )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
is_greater_than(const T& t, const A1& a) { return a < t; }


template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
is_greater_than(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	if (is_negative(t2))
		return false;
	return (std::make_unsigned_t<T>)t2 > load(a);
}

template <typename T, typename A1>
inline std::enable_if_t <
	(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1)),
	bool>
is_greater_than(const T& t, const A1& a)
{
	decltype(auto) a2(load(a));
	if (is_negative(a2))
		return true;
	return load(t) > (std::make_unsigned_t<T>)a2;
}

template <typename T, typename A1>
inline std::enable_if_t <
	!(std::is_integral_v<T> && std::is_integral_v<A1> && (std::is_signed_v<T> && !std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !(std::is_integral_v<T> && std::is_integral_v<A1> && (!std::is_signed_v<T> && std::is_signed_v<A1>) && ((std::remove_volatile_t<T>)-1 == (std::remove_volatile_t<A1>)-1))
	&& !std::is_class_v<T>
	&& !std::is_class_v<A1>,
	bool>
is_greater_than(const T& t, const A1& a)
{
	return load(t) > load(a);
}

COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(is_greater_than, > )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
is_less_than_or_equal(const T& t, const A1& a) { return a >= t; }

template <typename T, typename A1> inline std::enable_if_t <!std::is_class_v<T> && !std::is_class_v<A1>, bool>
is_less_than_or_equal(const T& t, const A1& a)
{ return !(is_greater_than(t, a)); }

COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(is_less_than_or_equal, <= )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, bool>
is_greater_than_or_equal(const T& t, const A1& a) { return a <= t; }

template <typename T, typename A1> inline std::enable_if_t <!std::is_class_v<T> && !std::is_class_v<A1>, bool>
is_greater_than_or_equal(const T& t, const A1& a)
{ return !(is_less_than(t, a)); }

COGS_DEFINE_BINARY_OPERATOR_FOR_MEMBER_OPERATOR(is_greater_than_or_equal, >= )


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, int>
compare(const T& t, const A1& a) { return -(a.compare(t)); }

template <typename T, typename A1> inline std::enable_if_t <!std::is_class_v<T> && !std::is_class_v<A1>, int>
compare(const T& t, const A1& a)
{
	decltype(auto) t2(load(t));
	decltype(auto) a2(load(a));
	if (is_less_than(t2, a2))
		return -1;
	if (is_less_than(a2, t2))
		return 1;
	return 0;
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(compare)

// TODO: Add compare_3way / <=>, once it makes it into the standard

/// @ingroup Collections
/// @brief A default comparator
class default_comparator
{
public:
	/// @brief Tests if less than
	/// @return true if less than
	template <typename T1, typename T2 = T1>
	static bool is_less_than(T1&& t1, T2&& t2)
	{
		return cogs::is_less_than(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if less than or equal
	/// @return true if less than or equal
	template <typename T1, typename T2 = T1>
	static bool is_less_than_or_equal(T1&& t1, T2&& t2)
	{
		return cogs::is_less_than_or_equal(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if greater than
	/// @return true if greater than
	template <typename T1, typename T2 = T1>
	static bool is_greater_than(T1&& t1, T2&& t2)
	{
		return cogs::is_greater_than(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if greater than or equal
	/// @return true if greater than or equal
	template <typename T1, typename T2 = T1>
	static bool is_greater_than_or_equal(T1&& t1, T2&& t2)
	{
		return cogs::is_greater_than_or_equal(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if equal
	/// @return true if equal
	template <typename T1, typename T2 = T1>
	static bool equals(T1&& t1, T2&& t2)
	{
		return cogs::equals(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if not equal
	/// @return true if not equal
	template <typename T1, typename T2 = T1>
	static bool not_equals(T1&& t1, T2&& t2)
	{
		return cogs::not_equals(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if equal, greater than or less than.
	/// @return 0 if equal, -1 if less than, 1 if greater than.
	template <typename T1, typename T2 = T1>
	static bool compare(T1&& t1, T2&& t2)
	{
		return cogs::compare(std::forward<T1>(t1), std::forward<T2>(t2));
	}
};


template <typename comparator_t>
class reverse_comparator
{
public:
	/// @brief Tests if less than
	/// @return true if less than
	template <typename T1, typename T2 = T1>
	static bool is_less_than(T1&& t1, T2&& t2)
	{
		return !comparator_t::is_less_than(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if less than or equal
	/// @return true if less than or equal
	template <typename T1, typename T2 = T1>
	static bool is_less_than_or_equal(T1&& t1, T2&& t2)
	{
		return !comparator_t::is_less_than_or_equal(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if greater than
	/// @return true if greater than
	template <typename T1, typename T2 = T1>
	static bool is_greater_than(T1&& t1, T2&& t2)
	{
		return !comparator_t::is_greater_than(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if greater than or equal
	/// @return true if greater than or equal
	template <typename T1, typename T2 = T1>
	static bool is_greater_than_or_equal(T1&& t1, T2&& t2)
	{
		return !comparator_t::is_greater_than_or_equal(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if equal
	/// @return true if equal
	template <typename T1, typename T2 = T1>
	static bool equals(T1&& t1, T2&& t2)
	{
		return !comparator_t::equals(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if not equal
	/// @return true if not equal
	template <typename T1, typename T2 = T1>
	static bool not_equals(T1&& t1, T2&& t2)
	{
		return !comparator_t::not_equals(std::forward<T1>(t1), std::forward<T2>(t2));
	}

	/// @brief Tests if equal, greater than or less than.
	/// @return 0 if equal, -1 if less than, 1 if greater than.
	template <typename T1, typename T2 = T1>
	static bool compare(T1&& t1, T2&& t2)
	{
		return !comparator_t::compare(std::forward<T1>(t1), std::forward<T2>(t2));
	}
};



template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().greater(std::declval<T&>()))>
greater(const T& t, const A1& a) { return a.greater(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>,
	bytes_to_int_t<((sizeof(T) > sizeof(A1)) ? sizeof(T) : sizeof(A1)), std::is_signed_v<T> && std::is_signed_v<A1> >
>
greater(const T& t, const A1& a)
{
	if (cogs::is_greater_than(t, a))
		return t;
	return a;
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_floating_point_v<A1>,
	std::conditional_t<(sizeof(T) > sizeof(A1)), T, A1>
>
greater(const T& t, const A1& a)
{
	if (cogs::is_greater_than(t, a))
		return t;
	return a;
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(greater)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().lesser(std::declval<T&>()))>
lesser(const T& t, const A1& a) { return a.lesser(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>,
	bytes_to_int_t<
		((sizeof(T) >= sizeof(A1))
			? (std::is_signed_v<T>
				? sizeof(T)
				: sizeof(A1))
			: (std::is_signed_v<A1>
				? sizeof(A1)
				: sizeof(T)))
	, (std::is_signed_v<T> || std::is_signed_v<A1>)>
>
lesser(const T& t, const A1& a)
{
	if (cogs::is_less_than(t, a))
		return t;
	return a;
}


template <typename T, typename A1>
inline std::enable_if_t<
	std::is_floating_point_v<T>
	&& std::is_floating_point_v<A1>,
	std::conditional_t<(sizeof(T) < sizeof(A1)), T, A1>
>
lesser(const T& t, const A1& a)
{
	if (cogs::is_less_than(t, a))
		return t;
	return a;
}


COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(lesser)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().gcd(std::declval<T&>()))>
gcd(const T& t, const A1& a) { return a.gcd(t); }


template <typename T, typename A1>
inline std::enable_if_t<std::is_integral_v<T> && std::is_integral_v<A1>,
	bytes_to_int_t<((sizeof(T) < sizeof(A1)) ? sizeof(T) : sizeof(A1)), false> >
gcd(const T& t, const A1& a)
{
	decltype(auto) t2(abs(t));
	decltype(auto) a2(abs(a));

	if (cogs::is_less_than(t2, a2))
	{
		for (;;)
		{
			decltype(auto) r = modulo(a2, t2);
			if (!r)
				return t2;
			a2 = t2;
			t2 = r;
		}
	}
	else
	{
		for (;;)
		{
			decltype(auto) r = modulo(t2, a2);
			if (!r)
				return a2;
			t2 = a2;
			a2 = r;
		}
	}
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(gcd)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().lcm(std::declval<T&>()))>
lcm(const T& t, const A1& a) { return a.lcm(t); }

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& (sizeof(T) + sizeof(A1) <= sizeof(longest)),
	bytes_to_int_t<(sizeof(T) + sizeof(A1)), false>
>
lcm(const T& t, const A1& a)
{
	decltype(auto) t2(abs(t));
	decltype(auto) a2(abs(a));
	return divide_whole(multiply(t2, a2), gcd(t2, a2));
}

template <typename T, typename A1>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& std::is_integral_v<A1>
	&& (sizeof(T) + sizeof(A1) > sizeof(longest)),
	fixed_integer<false, (sizeof(T) + sizeof(A1)) * 8>
>
lcm(const T& t, const A1& a)
{
	decltype(auto) t2(abs(t));
	decltype(auto) a2(abs(a));
	return divide_whole(multiply(t2, a2), gcd(t2, a2));
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(lcm)


template <typename T, typename S> inline std::enable_if_t<std::is_scalar_v<T> && !std::is_volatile_v<T>, T>
exchange(T& t, S&& src)
{
	T rtn;
	assign(rtn, t);
	assign(t, std::forward<S>(src));
	return rtn;
}

template <typename T, typename S> inline std::enable_if_t<std::is_scalar_v<T> && std::is_volatile_v<T>, std::remove_volatile_t<T> >
exchange(T& t, S&& src)
{
	std::remove_volatile_t<T> tmpSrc;
	assign(tmpSrc, std::forward<S>(src));
	return atomic::exchange(t, tmpSrc);
}


template <typename T, typename S, typename R> inline std::enable_if_t<std::is_scalar_v<T> && !std::is_volatile_v<T>, void>
exchange(T& t, S&& src, R& rtn)
{
	T tmp;
	assign(tmp, std::forward<S>(src));
	assign(rtn, t);
	assign(t, std::move(tmp));
}

template <typename T, typename S, typename R> inline std::enable_if_t<std::is_scalar_v<T> && std::is_volatile_v<T>, void>
exchange(T& t, S&& src, R& rtn)
{
	std::remove_volatile_t<T> tmpSrc;
	assign(tmpSrc, std::forward<S>(src));
	std::remove_volatile_t<T> tmpRtn;
	atomic::exchange(t, tmpSrc, tmpRtn);
	assign(rtn, tmpRtn);
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(exchange)


template <typename T, typename S, typename C> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, bool>
compare_exchange(T& t, S&& src, C&& cmp)
{
	bool b = equals(t, std::forward<C>(cmp));
	assign(t, std::forward<S>(src));
	return b;
}

template <typename T, typename S, typename C> inline std::enable_if_t<!std::is_class_v<T> && std::is_volatile_v<T>, bool>
compare_exchange(T& t, S&& src, C&& cmp)
{
	std::remove_volatile_t<T> tmpSrc;
	assign(tmpSrc, std::forward<S>(src));
	std::remove_volatile_t<T> tmpCmp;
	assign(tmpCmp, std::forward<C>(cmp));
	return atomic::compare_exchange(t, tmpSrc, tmpCmp);
}


template <typename T, typename S, typename C, typename R> inline std::enable_if_t<!std::is_class_v<T> && !std::is_volatile_v<T>, bool>
compare_exchange(T& t, S&& src, C&& cmp, R& rtn)
{
	T tmp = t;
	bool b = equals(tmp, std::forward<C>(cmp));
	if (b)
		assign(t, std::forward<S>(src));
	assign(rtn, tmp);
	return b;
}

template <typename T, typename S, typename C, typename R> inline std::enable_if_t<!std::is_class_v<T> && std::is_volatile_v<T>, bool>
compare_exchange(T& t, S&& src, C&& cmp, R& rtn)
{
	std::remove_volatile_t<T> tmpSrc;
	assign(tmpSrc, std::forward<S>(src));
	std::remove_volatile_t<T> tmpCmp;
	assign(tmpCmp, std::forward<C>(cmp));
	std::remove_volatile_t<T> tmpRtn;
	bool b = atomic::compare_exchange(t, tmpSrc, tmpCmp, tmpRtn);
	assign(rtn, tmpRtn);
	return b;
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(compare_exchange)


template <typename T, typename A1> inline std::enable_if_t<!std::is_class_v<T> && std::is_class_v<A1>, decltype(std::declval<A1&>().swap(std::declval<T&>()))>
swap(T& t, const A1& a) { return a.swap(t); }

template <typename T1, typename T2>
inline std::enable_if_t<
	!std::is_class_v<T1> && !std::is_class_v<T2>
	&& !std::is_volatile_v<T1> && !std::is_volatile_v<T2>,
	void
>
swap(T1& a, T2& b)
{
	T1 tmp;
	assign(tmp, b);
	assign(b, a);
	assign(a, tmp);
}

template <typename T1, typename T2>
inline std::enable_if_t<
	!std::is_class_v<T1> && !std::is_class_v<T2>
	&& std::is_volatile_v<T1> && !std::is_volatile_v<T2>,
	void
>
swap(T1& a, T2& b)
{
	assign(b, exchange(a, b));
}

template <typename T1, typename T2>
inline std::enable_if_t<
	!std::is_class_v<T1> && !std::is_class_v<T2>
	&& !std::is_volatile_v<T1> && std::is_volatile_v<T2>,
	void
>
swap(T1& a, T2& b)
{
	assign(a, exchange(b, a));
}

COGS_DEFINE_BINARY_OPERATOR_FOR_FUNCTION(swap)


template <typename T> inline std::enable_if_t<!std::is_class_v<T>, void>
clear(T& t)
{
	assign(t, (std::remove_volatile_t<T>)0);
}

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(clear)

template <typename T> class composite_string_t;
template <typename T> class string_t;

typedef string_t<wchar_t> string;
typedef string_t<char> cstring;

typedef composite_string_t<wchar_t> composite_string;
typedef composite_string_t<char> composite_cstring;

inline cstring string_to_cstring(const composite_string& s);
inline string cstring_to_string(const composite_cstring& s);


template <typename T> inline std::enable_if_t<std::is_integral_v<T>, composite_string>
to_string(const T& t);

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, composite_string>
to_string(const T& t);

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, composite_cstring>
to_cstring(const T& t);

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, composite_cstring>
to_cstring(const T& t);



template <typename char_t, typename T>
inline std::enable_if_t<std::is_same_v<char_t, char> && (std::is_integral_v<T> || std::is_floating_point_v<T>), composite_string_t<char_t> >
to_string_t(const T& t) { return to_cstring(t); }

template <typename char_t, typename T>
inline std::enable_if_t<std::is_same_v<char_t, wchar_t> && (std::is_integral_v<T> || std::is_floating_point_v<T>), composite_string_t<char_t> >
to_string_t(const T& t) { return to_string(t); }

COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(to_string)
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(to_string_t)
COGS_DEFINE_OPERATOR_FOR_MEMBER_FUNCTION(to_cstring)


/// @brief A type that represent infinity at compile time
struct infinite_t { };


// TODO: Move there somewhere appropriate

template <typename T1, typename T2, typename... Ts>
struct is_any
{
	static constexpr bool value = std::is_same_v<T1, T2> || is_any<T1, Ts...>::value;
};

template <typename T1, typename T2, typename... Ts>
static constexpr bool is_any_v = is_any<T1, T2, Ts...>::value;

template <typename T1, typename T2>
struct is_any<T1, T2> : public std::is_same<T1, T2> { };


template <typename T1, typename... Ts>
struct are_same;

template <typename T1>
struct are_same<T1> : public std::true_type { };

template <typename T1, typename T2>
struct are_same<T1, T2> : public std::is_same<T1, T2> { };

template <typename T1, typename T2, typename... Ts>
struct are_same<T1, T2, Ts...>
{
	static constexpr bool value = std::is_same_v<T1, T2> && are_same<T2, Ts...>::value;
};

template <typename T1, typename... Ts>
static constexpr bool are_same_v = are_same<T1, Ts...>::value;


template <typename T1, typename... Ts>
struct first_type
{
	typedef T1 type;
};
template <typename T1, typename... Ts>
using first_type_t = typename first_type<T1, Ts...>::type;


template <typename T1, typename... Ts>
struct last_type
{
	typedef typename last_type<Ts...>::type type;
};
template <typename T1, typename... Ts>
using last_type_t = typename last_type<T1, Ts...>::type;

template <typename T1>
struct last_type<T1>
{
	typedef T1 type;
};


// For set types - types that contain (only) a set of other types

template <typename T, class setType>
struct prepend_type;

template <typename T, template <typename...> class setType, typename... Ts>
struct prepend_type<T, setType<Ts...> >
{
	typedef setType<T, Ts...> type;
};

template <typename T, class setType>
using prepend_type_t = typename prepend_type<T, setType>::type;


template <typename T, class setType>
struct append_type;

template <typename T, template <typename...> class setType, typename... Ts>
struct append_type<T, setType<Ts...> >
{
	typedef setType<Ts..., T> type;
};

template <typename T, class setType>
using append_type_t = typename append_type<T, setType>::type;


template <typename T, class setType>
struct remove_type;

template <typename T, template <typename...> class setType, typename... Ts>	// to match empty Ts
struct remove_type<T, setType<Ts...> >
{
	typedef setType<Ts...> type;
};

template <typename T, template <typename...> class setType, typename T2, typename... Ts>
struct remove_type<T, setType<T2, Ts...> >
{
public:
	typedef std::conditional_t<
		std::is_same_v<T, T2>,
		setType<Ts...>,
		typename prepend_type<T2,
		typename remove_type<T, setType<Ts...> >::type
		>::type> type;
};


template <typename T, class setType>
using remove_type_t = typename remove_type<T, setType>::type;


template <class setType1, class setType2>
struct are_same_set;

template <template <typename...> class setType1, template <typename...> class setType2, typename... Ts1, typename... Ts2>	// matches both empty
struct are_same_set<setType1<Ts1...>, setType2<Ts2...> >
{
	static constexpr bool value = true;
};

template <template <typename...> class setType1, template <typename...> class setType2, typename T1, typename... Ts1, typename... Ts2>	// matches empty Ts2
struct are_same_set<setType1<T1, Ts1...>, setType2<Ts2...> >
{
	static constexpr bool value = false;
};

template <template <typename...> class setType1, template <typename...> class setType2, typename... Ts1, typename T2, typename... Ts2>	// matches empty Ts1
struct are_same_set<setType1<Ts1...>, setType2<T2, Ts2...> >
{
	static constexpr bool value = false;
};

template <template <typename...> class setType1, template <typename...> class setType2, typename T1, typename... Ts1, typename T2, typename... Ts2>
struct are_same_set<setType1<T1, Ts1...>, setType2<T2, Ts2...> >
{
	static constexpr bool value = is_any<T1, T2, Ts2...>::value && are_same_set<setType1<Ts1...>, typename remove_type<T1, setType2<T2, Ts2...> >::type>::value;
};

template <class setType1, class setType2>
static constexpr bool are_same_set_v = are_same_set<setType1, setType2>::value;


template <typename... Ts>
struct pack
{
	template <typename T>
	class is_any
	{
		static constexpr bool value = cogs::is_any<T, Ts...>::value;
	};

	template <typename T>
	static constexpr bool is_any_v = is_any<T>::value;

	template <typename T>
	class are_same
	{
		static constexpr bool value = cogs::are_same<T, Ts...>::value;
	};

	template <typename T>
	static constexpr bool are_same_v = are_same<T>::value;

	typedef typename cogs::first_type<Ts...> first_type;
	typedef typename cogs::last_type<Ts...> last_type;

	template <typename T>
	class prepend_type
	{
		typedef typename cogs::prepend_type<T, pack<Ts...> > type;
	};

	template <typename T>
	using prepend_type_t = typename prepend_type<T>::type;

	template <typename T>
	class append_type
	{
		typedef typename cogs::append_type<T, pack<Ts...> > type;
	};

	template <typename T>
	using append_type_t = typename append_type<T>::type;

	template <typename T>
	class remove_type
	{
		typedef typename cogs::remove_type<T, pack<Ts...> > type;
	};

	template <typename T>
	using remove_type_t = typename remove_type<T>::type;

	template <typename... Ts2>
	class are_same_set
	{
		static constexpr bool value = cogs::are_same_set<pack<Ts...>, pack<Ts2...> >::value;
	};
};


}



#include "cogs/env/sync/atomic_operators.hpp"
#include "cogs/math/fixed_integer_native.hpp"

#endif
