//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_COMPILE_SOURCE


#include "cogs/mem/placement.hpp"
#include "cogs/os/io/kqueue_pool.hpp"


namespace cogs {
namespace os {
namespace io {

placement<rcptr<kqueue_pool> >		kqueue_pool::s_defaultKqueuePool;

}
}
}

#endif
