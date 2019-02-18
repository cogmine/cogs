//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_ATOMIC_LOAD
#define COGS_HEADER_ENV_SYNC_ATOMIC_LOAD

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
	!std::is_empty_v<T>
	&& can_atomic_v<T>
	&& std::is_scalar_v<T>,
	void
>
load(const volatile T& src, T& rtn)
{
	COGS_ASSERT(((size_t)&src % atomic::get_alignment_v<T>) == 0);
	typedef bytes_to_int_t<sizeof(T)> int_t;
	__atomic_load((int_t*)&src, (int_t*)&rtn, __ATOMIC_SEQ_CST);
}


}
}


#endif