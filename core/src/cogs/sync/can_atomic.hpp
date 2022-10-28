//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_CAN_ATOMIC
#define COGS_HEADER_SYNC_CAN_ATOMIC

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"


namespace cogs {

template <typename T>
struct can_atomic
{
public:
	static constexpr bool value = std::is_trivial_v<T> && (sizeof(T) <= arch::atomic::largest);
};

template <> struct can_atomic<void> : std::false_type {};

template <typename T>
constexpr bool can_atomic_v = can_atomic<T>::value;


}

#endif
