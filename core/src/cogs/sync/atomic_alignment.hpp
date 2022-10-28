//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_ATOMIC_ALIGNMENT
#define COGS_HEADER_SYNC_ATOMIC_ALIGNMENT

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"

namespace cogs {
namespace atomic {


template <size_t n>
struct size_to_alignment
{
	static constexpr size_t value = arch::atomic::size_to_alignment_v<n>;
};
template <size_t n> constexpr size_t size_to_alignment_v = size_to_alignment<n>::value;


template <typename T>
struct get_alignment
{
	static constexpr size_t value = size_to_alignment_v<sizeof(T)>;
};
template <typename T> constexpr size_t get_alignment_v = get_alignment<T>::value;

}
}


#endif
