//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"


namespace cogs {

#ifndef COGS_DEFAULT_GUI_SUBSYSTEM
#define COGS_DEFAULT_GUI_SUBSYSTEM COGS_GDI
#endif

#if COGS_DEFAULT_GUI_SUBSYSTEM == COGS_GDI
inline rcptr<gui::windowing::subsystem> gui::windowing::subsystem::get_default()
{
	return singleton<os::hwnd::subsystem>::get();
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

namespace os {


inline int initialize()
{
	return EXIT_SUCCESS;
}


inline void terminate()
{
}


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


#if COGS_DEFAULT_UI_SUBSYSTEM == COGS_CONSOLE
#pragma comment(linker, "/subsystem:console")
#define COGS_MAIN int main()
#else
#pragma comment(linker, "/subsystem:windows")
#define COGS_MAIN int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif


#endif
