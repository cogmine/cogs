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


bool initialize()
{
	return true;
}

void terminate()
{
	cogs::thread_pool::shutdown_default();
	cogs::default_allocator::shutdown();
}

int main();

}


#if COGS_USE_COGS_MAIN

//int main(int argc, const char* argv[])
int main()
{
	int result = 0;
	if (cogs::initialize())
	{
		result = cogs::main();
		cogs::get_quit_event()->wait();
		cogs::terminate();
	}

	return result;
}

#endif

