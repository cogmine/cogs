//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MATH_UMUL
#define COGS_HEADER_ENV_MATH_UMUL


#include "cogs/env.hpp"
#include "cogs/math/extumul.hpp"


namespace cogs {
namespace env {


inline ulongest umul_longest(const ulongest& src1, const ulongest& src2, ulongest& highPartRtn)
{
	return extumul<ulongest>(src1, src2, highPartRtn);
}


}
}


#endif

