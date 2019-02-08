//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#include "cogs/macro_stringify.hpp"


#ifndef COGS_COGS_MACRO_CONCAT
#define COGS_COGS_MACRO_CONCAT


#define COGS_MACRO_CONCAT_INNER(x, y) x ## y
#define COGS_MACRO_CONCAT(x, y) COGS_MACRO_CONCAT_INNER(x, y)


#endif
