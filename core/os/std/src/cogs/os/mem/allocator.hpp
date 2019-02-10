//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_OS_ALLOCATOR
#define COGS_OS_ALLOCATOR


#include "cogs/mem/os.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {

/// @brief Namespace for operating system specific functionality
namespace os {


// allocate(), try_reallocate() and usableSize
//
// It's possible a platform may allocate more than the requested size, and can provide the usable size in usableSize.
// If the usable size cannot be determined, allocate() should set usableSize to n.
//
// It's possible a platform may be able to extend the size of an allocation in place, beyond what was indicated in usableSize.
// It's possible a platform may also be able to determine the usable size of the newly extended allocation.
// If the usable size cannot be determined, try_reallocate() should set usableSize to n.
// If an allocation cannot be extended, try_reallocate() should not change usableSize.

// Given these combination of behaviors, the proper behavior of calling code that might reallocate a buffer is:
//	- Preserve the usableSize from the initial call to allocate().
//	- Only if that size needs to be exceeded, use try_reallocate(), updating the usableSize if successful
	

class allocator
{
public:
	static constexpr bool is_static = true;

	typedef ptr<void> ref_t;

	static ptr<void> allocate(size_t n, size_t align)										{ return malloc(n); }	// Let's hope default alignment is sufficient
	static void deallocate(const ptr<void>& p)												{ free(p.get_ptr()); }
	static bool try_reallocate(const ptr<void>& p, size_t n)								{ return false; }
	static size_t get_allocation_size(const ptr<void>& p, size_t align, size_t knownSize)	{ return knownSize; }
};


}
}


#endif
