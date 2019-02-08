//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#include <unistd.h>

#include "cogs/os/sync/thread.hpp"


namespace cogs {

static unsigned int s_num_processors = 0;

unsigned int get_num_processors()
{
	if (!s_num_processors)
		s_num_processors = (unsigned int)sysconf(_SC_NPROCESSORS_ONLN);
	return s_num_processors;
}


// Used by spinlocks.  Spins 1, or returns false to indicate that the spin should be aborted (such as on a uni-processor system)
bool os::thread::spin_once()
{
	if (get_num_processors() == 1)
		return false;

	_mm_pause();
	return true;
}

}

