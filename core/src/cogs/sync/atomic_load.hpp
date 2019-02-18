//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_HEADER_OPERATORS
#ifndef COGS_HEADER_SYNC_ATOMIC_LOAD
#define COGS_HEADER_SYNC_ATOMIC_LOAD

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
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
	&& can_atomic_v<T>
	&& std::is_scalar_v<T>,
	void
>
load(const volatile T& src, T& rtn)
{
	typedef bytes_to_int_t<sizeof(T)> int_t;
	int_t tmpRtn;
	cogs::atomic::compare_exchange(*(volatile int_t*)(unsigned char*)&src, (int_t)0, (int_t)0, tmpRtn);
	bypass_strict_aliasing(tmpRtn, rtn);
	
	// Full barrier doesn't seem to be enough...
	//full_barrier();
	//rtn = src;
	//full_barrier();
}

template <typename T>
inline std::enable_if_t<
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& !std::is_scalar_v<T>
	&& !std::is_same_v<void, bytes_to_int_t<sizeof(T)> >,
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
	//&& std::is_scalar_v<T>
	&& !std::is_same_v<void, bytes_to_int_t<sizeof(T)> >,
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

#include "cogs/operators.hpp"

#endif