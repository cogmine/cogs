//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_YIELD
#define COGS_HEADER_ENV_SYNC_YIELD

#include "cogs/os.hpp"

namespace cogs {
namespace env {


inline void yield()
{
#if defined(__x86_64__) || defined(i386) || defined(__i386__) || defined(__i386)
	_mm_pause();
#else
	asm volatile ("yield");
#endif
}

}
}

#endif
