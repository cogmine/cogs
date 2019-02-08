//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ARCH_ATOMIC
#define COGS_ARCH_ATOMIC


#include <stddef.h>


namespace cogs {
namespace arch {
namespace atomic {


// The purpose of the arch-level atomic.hpp is to define the largest supported type
// available for atomic operations, and the required alignment of atomic types

// How atomic APIs are accessed depends on the arch, os, and env used.
// Compiler intrinsics are preferred if available, followed by OS APIs, followed by
// architecture (i.e. asm).
	
// On x64 and IA64, loads and stores to 1, 2, 4,  8 and 16 byte aligned
// memory addresses are guaranteed to be atomic (no partial values).
	


static const size_t largest = 16;


template <size_t n>
class size_to_alignment
{
public:
	static const size_t value =	(n ==  1) ?  1 : 
								(n ==  2) ?  2 : 
								(n ==  4) ?  4 : 
								(n ==  8) ?  8 :
								(n == 16) ? 16 : 0;
};


}
}
}


#endif

