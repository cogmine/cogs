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


int main();

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

	cogs::thread_pool::shutdown_default();
	cogs::default_allocator::shutdown();

	return result;
}

