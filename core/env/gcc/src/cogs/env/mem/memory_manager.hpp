//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALLOCATOR
#define COGS_HEADER_ENV_MEM_ALLOCATOR


#include "cogs/os/mem/memory_manager.hpp"


namespace cogs {
namespace env {


class memory_manager : public memory_manager_base<memory_manager>
{
public:
	static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr) { return os::memory_manager::allocate(n, align, usableSize); } // TODO: out of memory handler
	static void deallocate(void* p) { os::memory_manager::deallocate(p); }
	static bool try_reallocate(void* p, size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr) { return os::memory_manager::try_reallocate(p, n, align, usableSize); }
};


}
}


#endif
