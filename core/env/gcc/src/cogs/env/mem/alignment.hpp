//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALIGNMENT
#define COGS_HEADER_ENV_MEM_ALIGNMENT


namespace cogs {


#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__SIZEOF_INT128__)
	static constexpr size_t largest_alignment = 16; // 32;
#else
	static constexpr size_t largest_alignment = 8; // 16;
#endif


}


#endif
