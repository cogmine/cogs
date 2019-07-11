//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"


namespace cogs {
namespace os {


inline int initialize()
{
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);

#ifdef _M_X64
	if (!(cpuInfo[2] & (1 << 13)))	// if 64-bit and no support for CMPXCHG16B instruction
	{
		MessageBox(NULL, L"Unsupported 64-bit processor detected.", L"Instruction not found: CMPXCHG16B", MB_OK);
		return EXIT_FAILURE;
	}
#else
	if (!(cpuInfo[3] & (1 << 8)))	// if 32-bit and no support for CMPXCHG8B instruction
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not found: CMPXCHG8B", MB_OK);
		return EXIT_FAILURE;
	}
	if (!(cpuInfo[2] & (1 << 23)))
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not found: POPCNT", MB_OK);
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

inline void terminate()
{
}


template <typename F>
inline int main(F&& main_func)
{
#if COGS_DEFAULT_GUI_SUBSYSTEM == GDI
	cogs::rcref<cogs::gui::windowing::subsystem> guiSubsystem = rcnew(cogs::bypass_constructor_permission<cogs::gui::os::hwnd::subsystem>);
#else
#error COGS_DEFAULT_GUI_SUBSYSTEM must be defined.  Currently supported values are: GDI
#endif
	int result = main_func(guiSubsystem);
	rcptr<quit_dispatcher> qd = quit_dispatcher::get();
	if (!!qd)
		qd->get_event().wait();
	return result;
}


}
}


#define COGS_MAIN main


#endif
