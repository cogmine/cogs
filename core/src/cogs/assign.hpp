//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ASSIGN
#define COGS_HEADER_ASSIGN


#include <type_traits>


#include "cogs/load.hpp"
#include "cogs/sync/atomic_store.hpp"


namespace cogs {


// assign

template <typename T, typename S>
inline std::enable_if_t<
	std::is_assignable_v<T&, decltype(load(std::declval<S>()))>
	&& (std::is_class_v<T> || !std::is_volatile_v<T>),
	void
>
assign(T& t, S&& src)
{
	t = load(std::forward<S>(src));
}

// Provide assignment to scalar volatile, as otherwise the compiler would perform the assignment unsafely.
template <typename T, typename S>
inline std::enable_if_t<
	std::is_constructible_v<std::remove_volatile_t<T>, decltype(load(std::declval<S>()))>
	&& !std::is_class_v<T>
	&& std::is_volatile_v<T>,
	void
>
assign(T& t, S&& src)
{
	std::remove_volatile_t<T> tmp(load(std::forward<S>(src)));
	atomic::store(t, tmp);
}

// Don't need to support built-in array type, because they are not assignable?


// pre_assign
// (exchange is the equivilent of post_assign)

template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_class_v<T>,
	decltype(std::declval<T&>().assign(std::declval<args_t>()...))
>
assign(T& t, args_t&&... args)
{
	return t.assign(std::forward<args_t>(args)...);
}


template <typename T, typename S>
inline std::enable_if_t<
	!std::is_class_v<T>
	&& !std::is_volatile_v<T>,
	T&
>
pre_assign(T& t, S&& src)
{
	assign(t, std::forward<S>(src));
	return t;
}

template <typename T, typename S>
inline std::enable_if_t<
	!std::is_class_v<T>
	&& std::is_volatile_v<T>,
	std::remove_volatile_t<T>
>
pre_assign(T& t, S&& src)
{
	std::remove_volatile_t<T> tmp;
	assign(tmp, std::forward<S>(src));
	assign(t, tmp);
	return tmp;
}

// Don't need to support built-in array type, because they are not assignable?


}

#endif
