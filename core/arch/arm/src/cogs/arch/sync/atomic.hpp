//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ARCH_SYNC_ATOMIC
#define COGS_HEADER_ARCH_SYNC_ATOMIC


#include <stddef.h>


namespace cogs {
namespace arch {
namespace atomic {


// The purpose of the arch-level atomic.hpp is to define the largest supported type
// available for atomic operations, and the required alignment of atomic types

// How atomic APIs are accessed depends on the arch, os, and env used.
// Compiler intrinsics are preferred if available, followed by OS APIs, followed by
// architecture (i.e. asm).

// On ARM, loads and stores to 1, 2, 4, and 8 byte aligned
// memory addresses are guaranteed to be atomic (no partial values).


static constexpr size_t largest = 8;


template <size_t n>
struct size_to_alignment
{
	static constexpr size_t value =
		(n == 1) ? 1 :
		(n == 2) ? 2 :
		(n <= 4) ? 4 :
		largest;
};
template <size_t n> constexpr size_t size_to_alignment_v = size_to_alignment<n>::value;


}
}
}


#endif
