//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_LOAD
#define COGS_HEADER_LOAD


#include <numeric>
#include <type_traits>

#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_load.hpp"

namespace cogs {


template <typename T>
inline constexpr std::enable_if_t<
	!std::is_volatile_v<std::remove_reference_t<T> >,
	T&&
>
load(T&& src)	// Value may only persist for the expression containing the call, so do not store a reference.
{
	return std::forward<T>(src);
}

template <typename T>
inline constexpr std::enable_if_t<
	std::is_volatile_v<T>
	&& std::is_class_v<T>,
	std::remove_volatile_t<T>
>
load(const T& src)
{
	std::remove_volatile_t<T> tmp(src);	// copy of a volatile object to get nonvolatile one
	return tmp;	
}

template <typename T>
inline std::enable_if_t<
	!std::is_class_v<T>,
	T
>
load(const T& src)
{
	return src;
}

template <typename T>
inline std::enable_if_t<
	!std::is_class_v<T>
	&& can_atomic_v<T>,
	T
>
load(const volatile T& src)
{
	return atomic::load(src);
}

template <typename T>
inline std::enable_if_t<
	!std::is_class_v<T>
	&& !can_atomic_v<T>,
	T
>
load(const volatile T& src) = delete;


}

#endif
