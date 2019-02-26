//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_COMPARE_EXCHANGE
#define COGS_HEADER_ENV_SYNC_ATOMIC_COMPARE_EXCHANGE

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
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) <= 8,
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	rtn = cmp;
	return __atomic_compare_exchange(&t, &rtn, &src, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) <= 8,
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	T rtn;
	return compare_exchange(t, src, cmp, rtn);
}


// Fallback to old intrincs until GCC gets it's atomics problem figured out
template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) == 16,
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	T tmp = __sync_val_compare_and_swap(&t, cmp, src);
	bool b = (tmp == cmp);
	rtn = tmp;
	return b;
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>
	&& sizeof(T) == 16,
	bool
>
compare_exchange(volatile T& t, const T& src, const T& cmp)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	return __sync_bool_compare_and_swap(&t, cmp, src);
}


}
}

#endif

