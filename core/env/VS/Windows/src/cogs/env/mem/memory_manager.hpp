//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALLOCATOR
#define COGS_HEADER_ENV_MEM_ALLOCATOR


#include <malloc.h>

#include "cogs/os/mem/memory_manager.hpp"


namespace cogs {
namespace env {


class memory_manager : public memory_manager_base<memory_manager>
{
public:
	static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
	{
		if (usableSize)
			*usableSize = n;
		return _aligned_malloc(n, align);
	} 
	static void deallocate(void* p) { _aligned_free(p); }
	static bool try_reallocate(void*, size_t, size_t = cogs::largest_alignment, size_t* = nullptr) { return false; }
};


};
};


#endif
