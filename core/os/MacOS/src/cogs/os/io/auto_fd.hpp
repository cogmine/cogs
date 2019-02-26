//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_AUTOFD
#define COGS_HEADER_IO_AUTOFD


#include "cogs/env.hpp"
#include "cogs/mem/auto_handle.hpp"


namespace cogs {


inline void auto_handle_impl_close(int fd) { ::close(fd); }
typedef auto_handle<int, -1, auto_handle_impl_close> auto_fd;


}


#endif


