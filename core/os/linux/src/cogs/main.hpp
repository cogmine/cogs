//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include <cstdlib>


namespace cogs {
namespace os {


inline int initialize() { return EXIT_SUCCESS; }
inline void terminate() { }

template <typename F>
inline int main(F&& main_func)
{
	// TODO: Console UI subsystem
	//return main_func(uiSubsystem);
	int result = main_func();
	cogs::get_quit_event()->wait();
	return result;
}


}
}


#define COGS_MAIN main


#endif
