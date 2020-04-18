//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_BYPASS_STRICT_ALIASING
#define COGS_HEADER_MEM_BYPASS_STRICT_ALIASING

#include <type_traits>
#include <cstring>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"


namespace cogs {


template <typename S, typename T>
inline std::enable_if_t<
	std::is_scalar_v<T>
	&& std::is_scalar_v<S>,
	void>
bypass_strict_aliasing(const S& src, T& dst)
{
	dst = (T)src;
}

template <typename S, typename T>
inline std::enable_if_t<
	std::is_empty_v<T> || std::is_empty_v<S>,
	void>
bypass_strict_aliasing(const S& src, T& dst)
{
}

template <typename S, typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& !std::is_empty_v<S>
	&& (!std::is_scalar_v<T> || !std::is_scalar_v<S>),
	void>
bypass_strict_aliasing(const S& src, T& dst)
{
	static_assert(sizeof(T) == sizeof(S));
	std::memcpy(&dst, &src, sizeof(T));
}


}

#endif
