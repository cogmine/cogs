//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_SYNC_THREAD
#define COGS_HEADER_ENV_SYNC_THREAD


#include "cogs/os/sync/thread.hpp"

namespace cogs {
namespace env {

using thread = os::thread;

inline unsigned int get_processor_count() { return os::get_processor_count(); }

}
}


#endif
