//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good


#include "cogs/env.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"

namespace cogs {

bool initialize()
{
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);

#ifdef _M_X64
	if (!(cpuInfo[2] & (1 << 13)))	// if 64-bit and no support for CMPXCHG16B instruction
	{
		MessageBox(NULL, L"Unsupported 64-bit processor detected.", L"Instruction not found: CMPXCHG16B", MB_OK);
		return false;
	}
#else
	if (!(cpuInfo[3] & (1 << 8)))	// if 32-bit and no support for CMPXCHG8B instruction
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not found: CMPXCHG8B", MB_OK);
		return false;
	}
	if (!(cpuInfo[2] & (1 << 23)))
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not found: POPCNT", MB_OK);
		return false;
	}
#endif

	return true;
}

void terminate()
{
	cogs::thread_pool::shutdown_default();
	cogs::default_allocator::shutdown();
}

}

#if COGS_USE_COGS_MAIN

int main();

namespace cogs {
	int main(const rcref<gui::windowing::subsystem>&);
}

int main()
{
	int result = 0;
	
	if (cogs::initialize())
	{
		{
#if COGS_DEFAULT_GUI_SUBSYSTEM == GDI
			cogs::rcref<cogs::gui::windowing::subsystem> guiSubsystem = rcnew(cogs::bypass_constructor_permission<cogs::gui::os::hwnd::subsystem>);
#else
#error COGS_DEFAULT_GUI_SUBSYSTEM must be defined.  Currently supported values are: GDI
#endif

			result = cogs::main(guiSubsystem);

			// The quit_handler depends on COGS_USE_COGS_MAIN=1, or otherwise calling the following to block termination of main().
			cogs::get_quit_event()->wait();
		}

		cogs::terminate();
	}

	return result;
}

 int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	return main();
}

#endif

