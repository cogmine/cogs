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
#ifdef YieldProcessor
	YieldProcessor();
#elif defined(_M_X64) || defined(_M_AMD64)
	_mm_pause();
#elif defined(_M_ARM64) || defined(_M_ARM)
	__yield();
#else
	__asm__ __volatile__ ("yield");
#endif
}

}
}

#endif
