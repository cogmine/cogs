//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ASSERT
#define COGS_HEADER_ASSERT


#include <assert.h>


namespace cogs {


#ifdef COGS_DEBUG


#define COGS_ASSERT(b) assert(b)


#else

#define COGS_ASSERT(b) ((void)(std::remove_reference_t<decltype((b))>*)0)

#endif



}


#endif
