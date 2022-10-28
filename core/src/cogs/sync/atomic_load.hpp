//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_ATOMIC_LOAD
#define COGS_HEADER_SYNC_ATOMIC_LOAD

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/env/sync/atomic_load.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <typename T>
inline std::enable_if_t<
	std::is_empty_v<T>,
	void
>
load(const volatile T& src, T& rtn)
{
}

template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& !can_atomic_v<T>,
	void
>
load(const volatile T& src, T& rtn) = delete;


template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& !is_scalar_v<T>
	&& !std::is_void_v<bytes_to_int_t<sizeof(T)> >,
	void
>
load(const volatile T& src, T& rtn)
{
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmp;
	cogs::atomic::load(*(volatile int_t*)(unsigned char*)&src, tmp);
	bypass_strict_aliasing(tmp, rtn);
}


template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& !std::is_void_v<bytes_to_int_t<sizeof(T)> >,
	T
>
load(const volatile T& src)
{
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmp;
	cogs::atomic::load(*(volatile int_t*)(unsigned char*)&src, tmp);
	T rtn;
	bypass_strict_aliasing(tmp, rtn);
	return rtn;
}


}
}


#endif
