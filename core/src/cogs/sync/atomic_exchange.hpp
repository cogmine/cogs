//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_ATOMIC_EXCHANGE
#define COGS_HEADER_SYNC_ATOMIC_EXCHANGE

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/env/sync/atomic_exchange.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


// atomic namespace defines exchange and compare_exchange for non-scalar types,
// whereas cogs namespace version wraps member functions.

// atomic namespace versions of exchange and compare_exchange also require all args to be of same fundamental type.

// atomic namespace versions of exchange and compare_exchange for scalar types are defined in atomic_operators.hpp

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_void_v<bytes_to_int_t<sizeof(T)> >,
	void>
exchange(volatile T& t, const T& src, T& rtn)
{
	typedef bytes_to_uint_t<sizeof(T)> int_t;
	volatile int_t* tmpDst = reinterpret_cast<volatile int_t*>(&t);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpRtn;
	exchange(*tmpDst, tmpSrc, tmpRtn);
	bypass_strict_aliasing(tmpRtn, rtn);
}

template <typename T>
inline std::enable_if_t<
	can_atomic_v<T>
	&& !is_scalar_v<T>
	&& !std::is_const_v<T>
	&& !std::is_void_v<bytes_to_int_t<sizeof(T)> >,
	T>
exchange(volatile T& t, const T& src)
{
	typedef bytes_to_uint_t<sizeof(T)> int_t;
	volatile int_t* tmpDst = reinterpret_cast<volatile int_t*>(&t);
	int_t tmpSrc;
	bypass_strict_aliasing(src, tmpSrc);
	int_t tmpRtn;
	exchange(*tmpDst, tmpSrc, tmpRtn);
	T rtn;
	bypass_strict_aliasing(tmpRtn, rtn);
	return rtn;
}


}
}


#endif
