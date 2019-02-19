//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_HEADER_OS_SYNC_ATOMIC_OPERATORS
#ifndef COGS_HEADER_ARCH_SYNC_ATOMIC_OPERATORS
#define COGS_HEADER_ARCH_SYNC_ATOMIC_OPERATORS


#include <type_traits> 

#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/default_atomic_operators.hpp"


namespace cogs {
namespace arch {
namespace atomic {


COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(not)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(abs)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_not)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(negative)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_count)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_forward)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_scan_reverse)

COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(endian_swap)

COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_left)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_rotate_right)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_left)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_shift_right)

COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_and)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_or)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(bit_xor)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(add)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(subtract)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_subtract)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(next)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(prev)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(multiply)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(modulo)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_modulo)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide)

COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(reciprocal)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(divide_whole)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(inverse_divide_whole)

COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(gcd)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lcm)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(greater)
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(lesser)


}
}
}




#endif
#endif

#include "cogs/os/sync/atomic_operators.hpp"
