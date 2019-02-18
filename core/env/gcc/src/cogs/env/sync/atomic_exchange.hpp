//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE
#define COGS_HEADER_ENV_SYNC_ATOMIC_EXCHANGE

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
	&& !std::is_const_v<T>,
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	__atomic_exchange((uint_t*)&t, (uint_t*)&src, (uint_t*)&rtn, __ATOMIC_SEQ_CST);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& std::is_scalar_v<T>
	&& !std::is_const_v<T>,
	T
>
exchange(volatile T& t, const T& src)
{
	COGS_ASSERT((size_t)&t % atomic::get_alignment_v<T> == 0);
	typedef bytes_to_uint_t<sizeof(T)> uint_t;
	T rtn;
	__atomic_exchange((uint_t*)(unsigned char*)&t, (uint_t*)&src, (uint_t*)&rtn, __ATOMIC_SEQ_CST);
	return rtn;
}


}
}


#endif