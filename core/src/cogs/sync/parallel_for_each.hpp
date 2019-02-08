//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_PARALLEL_FOR_EACH
#define COGS_PARALLEL_FOR_EACH


#include "cogs/function.hpp"


namespace cogs {

void dispatch_parallel(size_t n, const function<void(size_t)>& d, const function<void()>& doneDelegate = function<void()>(), int priority = 0);

}


#endif
