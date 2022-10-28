//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MEM_ALLOCATOR
#define COGS_HEADER_OS_MEM_ALLOCATOR


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/os.hpp"


namespace cogs {
namespace os {


class memory_manager : public memory_manager_base<memory_manager>
{
public:
	static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
	{
		if (usableSize)
			*usableSize = n;
		void* buf;
		int i = posix_memalign(&buf, align, n);
		COGS_ASSERT(i == 0);
		return buf;
	}

	static void deallocate(void* p) { free(p); }
	static bool try_reallocate(void*, size_t, size_t = cogs::largest_alignment, size_t* = nullptr) { return false; }
};


}
}


#endif
