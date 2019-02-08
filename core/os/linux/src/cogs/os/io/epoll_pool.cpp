//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_COMPILE_SOURCE

#include "cogs/mem/placement.hpp"
#include "cogs/os/io/epoll_pool.hpp"


namespace cogs {
namespace os {
namespace io {


placement<rcptr<epoll_pool> >		epoll_pool::s_defaultEPollPool;

}
}
}

#endif
