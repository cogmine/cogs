//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ARCH_ATOMIC
#define COGS_HEADER_ARCH_ATOMIC


#include <stddef.h>


namespace cogs {
namespace arch {

/// @brief Namespace for architecture-specific atomic functionality
namespace atomic {


// The purpose of the arch-level atomic.hpp is to define the largest supported type
// available for atomic operations, and the required alignment of atomic types

// How atomic APIs are accessed depends on the arch, os, and env used.
// Compiler intrinsics are preferred if available, followed by OS APIs, followed by
// architecture (i.e. asm).


// No atomic support if architecture is 'unknown'


static constexpr size_t largest = 0;


template <size_t n>
class size_to_alignment
{
public:
	static constexpr size_t value = 0;
};


}
}
}


#endif
