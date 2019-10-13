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
#ifdef _M_X64

	// Unlike the 32-bit compiler, the 64-bit compiler doesn't currently support a double-sized int.
	// But it does provide a compiler intrinsic to get optimal support for 128-bit multiplication results.

	return _umul128(src1, src2, &highPartRtn);
#else

	// The 32-bit compiler provides support for a double-sized int, so has already used the
	// equivalent instruction for 64-bit results.  But we still need to hand-code a 64-bit multiplication with 128-bit result.

	return extumul<ulongest>(src1, src2, highPartRtn);
#endif
}


}
}


#endif

