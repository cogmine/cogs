//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include <cstdlib>
#include "cogs/gui/subsystem.hpp"


namespace cogs {


inline rcptr<gui::windowing::subsystem> gui::windowing::subsystem::get_default()
{
	rcptr<gui::windowing::subsystem> empty;
	return empty;
}

inline rcptr<gui::subsystem> gui::subsystem::get_default()
{
	return gui::windowing::subsystem::get_default();
}

inline rcref<ui::subsystem> ui::subsystem::get_default()
{
	return gui::subsystem::get_default().dereference();
}


namespace os {


inline int initialize() { return EXIT_SUCCESS; }
inline void terminate() { }

template <typename F, typename T>
inline int main(F&& mainFunc, T&& uiSubsystem)
{
	int result = mainFunc(std::forward<T>(uiSubsystem));
	rcptr<quit_dispatcher> qd = quit_dispatcher::get();
	if (!!qd)
		qd->get_condition().wait();
	return result;
}


}
}


#define COGS_MAIN int main()


#endif
