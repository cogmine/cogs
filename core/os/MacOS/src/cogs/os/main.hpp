//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/nsview.hpp"


namespace cogs {


int alt_main();


namespace os {


inline int initialize() { return EXIT_SUCCESS; }
inline void terminate() { }


template <typename F>
inline int main(F&& main_func)
{
	cogs::rcref<cogs::gui::windowing::subsystem> guiSubsystem = cogs::gui::os::nsview_subsystem::get();
	int result = main_func(guiSubsystem);
	cogs::get_quit_event()->wait();
	return result;
}


}
}


// Allows app to use alternative name for main, in case platform (i.e. Mac) already uses it.
#define COGS_MAIN ::cogs::alt_main


#endif
