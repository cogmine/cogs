//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_ATOMIC_COMPARE_EXCHANGE
#define COGS_HEADER_SYNC_ATOMIC_COMPARE_EXCHANGE

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/env/sync/atomic_compare_exchange.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_same_v<void, bytes_to_int_t<sizeof(T)> >,
	bool>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	typedef bytes_to_uint_t<sizeof(T)> int_t;
	volatile int_t* tmpDst = reinterpret_cast<volatile int_t*>(&t);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpCmp;
	bypass_strict_aliasing(cmp, tmpCmp);
	int_t tmpRtn;
	bool b = compare_exchange(*tmpDst, tmpSrc, tmpCmp, tmpRtn);
	bypass_strict_aliasing(tmpRtn, rtn);
	return b;
}


template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_same_v<void, bytes_to_int_t<sizeof(T)> >,
	bool>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	typedef bytes_to_uint_t<sizeof(T)> int_t;
	volatile int_t* tmpDst = reinterpret_cast<volatile int_t*>(&t);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpCmp;
	bypass_strict_aliasing(cmp, tmpCmp);
	return compare_exchange(*tmpDst, tmpSrc, tmpCmp);
}


template <typename T, class functor_t>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>,
	void
>
compare_exchange_retry_loop(volatile T& t, functor_t&& fctr)
{
	T oldValue = load(t);
	for (;;)
	{
		T newValue = fctr(oldValue);
		if ((newValue == oldValue) || (compare_exchange(t, newValue, oldValue, oldValue)))
			break;
	}
}


template <typename T, class functor_t>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>,
	std::remove_cv_t<T>
>
compare_exchange_retry_loop_pre(volatile T& t, functor_t&& fctr)
{
	T oldValue = load(t);
	for (;;)
	{
		T newValue = fctr(oldValue);
		if ((newValue == oldValue) || (compare_exchange(t, newValue, oldValue, oldValue)))
			return newValue;
	}
}


template <typename T, class functor_t>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !std::is_const_v<T>,
	std::remove_cv_t<T> 
>
compare_exchange_retry_loop_post(volatile T& t, functor_t&& fctr)
{
	T oldValue = load(t);
	for (;;)
	{
		T newValue = fctr(oldValue);
		if ((newValue == oldValue) || (compare_exchange(t, newValue, oldValue, oldValue)))
			break;
	}
	return oldValue;
}

}
}


#endif
