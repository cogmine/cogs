//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MAIN
#define COGS_HEADER_ENV_MAIN


#include <utility>
#include "cogs/os/main.hpp"
#include "cogs/gui/subsystem.hpp"


namespace cogs {


#ifndef COGS_DEFAULT_GUI_SUBSYSTEM
#define COGS_DEFAULT_GUI_SUBSYSTEM COGS_GDI
#endif

#if COGS_DEFAULT_GUI_SUBSYSTEM == COGS_GDI
inline rcptr<gui::windowing::subsystem> gui::windowing::subsystem::get_default()
{
	return rcnew(gui::os::hwnd::subsystem);
}

inline rcptr<gui::subsystem> gui::subsystem::get_default()
{
	return gui::windowing::subsystem::get_default();
}

inline rcref<ui::subsystem> ui::subsystem::get_default()
{
	return gui::subsystem::get_default().dereference();
}
#endif


namespace env {

inline int initialize() { return os::initialize(); }
inline void terminate() { os::terminate(); }

template <typename F, typename T> inline int main(F&& mainFunc, T&& uiSubsystem) { return os::main(std::forward<F>(mainFunc), std::forward<T>(uiSubsystem)); }


}
}


#endif
