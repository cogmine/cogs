//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifdef COGS_COMPILE_SOURCE

#include "cogs/os/sync/thread.hpp"


namespace cogs {


static unsigned int s_num_processors = 0;

unsigned int cogs::get_num_processors()
{
	if (!s_num_processors)
	{	
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		s_num_processors = (unsigned long)si.dwNumberOfProcessors;
	}
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

#endif
