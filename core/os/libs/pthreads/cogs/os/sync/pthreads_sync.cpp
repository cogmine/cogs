//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <xmmintrin.h>


#include "cogs/mem/default_allocator.hpp"
#include "cogs/os/sync/thread.hpp"
#include "cogs/sync/quit_dispatcher.hpp"


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


int main();
void run_cleanup();

};


int main(int argc, const char **argv)
{
	int result = 0;
	{
		result = cogs::main();
		cogs::rcptr<const cogs::single_fire_event> quitEvent = cogs::quit_dispatcher::get()->get_event();
		if (!!quitEvent)
			quitEvent->wait();
	}

	cogs::run_cleanup();
	cogs::default_allocator::shutdown();

	return result;
}

