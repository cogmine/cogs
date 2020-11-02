//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/nsview.hpp"


namespace cogs {


inline rcptr<gui::windowing::subsystem> gui::windowing::subsystem::get_default()
{
	return rcnew(os::nsview_subsystem);
}


inline rcptr<gui::subsystem> gui::subsystem::get_default()
{
	return gui::windowing::subsystem::get_default();
}


inline rcref<ui::subsystem> ui::subsystem::get_default()
{
	return gui::subsystem::get_default().dereference();
}


int alt_main();


namespace os {


inline int initialize() { return EXIT_SUCCESS; }
inline void terminate() { }


template <typename F, typename T>
inline int main(F&& mainFunc, T&& uiSubsystem)
{
	return mainFunc(std::forward<T>(uiSubsystem));
}


}
}


// Allows app to use alternative name for main, in case platform (i.e. Mac) already uses it.
#define COGS_MAIN int ::cogs::alt_main()


#endif
