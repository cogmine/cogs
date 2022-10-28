//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALIGNMENT
#define COGS_HEADER_ENV_MEM_ALIGNMENT


namespace cogs {

#if defined(_M_X64) || defined(_M_AMD64) || defined(_M_ARM64)
	static constexpr size_t largest_alignment = 16;// 32;
#else
	static constexpr size_t largest_alignment = 8;// 16;
#endif


}


#endif
