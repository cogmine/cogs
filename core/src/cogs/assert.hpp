//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_COGS_ASSERT
#define COGS_COGS_ASSERT


#include <assert.h>


namespace cogs {


#ifdef _DEBUG


#define COGS_ASSERT(b) assert(b)

//#define COGS_ASSERT(b) cogs_assert(b)
//
//inline void cogs_assert(bool b)
//{
//	if (!b)
//	{
//		b = b;
//	}
//}


#else

#define COGS_ASSERT(b)

#endif



}


#endif
