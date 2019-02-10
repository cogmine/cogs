//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_WINDOWS
#define COGS_OS_WINDOWS


//#define WINVER 0x0600
#define OEMRESOURCE 1

#include <stddef.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <process.h>
#include <io.h>
#include <Gdiplus.h>
#include <richedit.h>
#include <shellscalingapi.h>
#include <cmath>
#include <memory>
#include <gdiplus.h>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "uxtheme.lib") 

#define SC_RESIZE_LEFT			0xF001
#define SC_RESIZE_RIGHT			0xF002 
#define SC_RESIZE_TOP			0xF003
#define SC_RESIZE_TOP_LEFT		0xF004 
#define SC_RESIZE_TOP_RIGHT		0xF005
#define SC_RESIZE_BOTTOM		0xF006
#define SC_RESIZE_BOTTOM_LEFT	0xF007
#define SC_RESIZE_BOTTOM_RIGHT	0xF008

#define SC_DRAGMOVE				0xF012

#include "cogs/arch.hpp"

namespace cogs {
namespace os {

class file_system
{
public:
	static constexpr size_t size_bits = 64;
};

}

namespace gui {
namespace os {
	inline void beep() { MessageBeep(-1); }
}
}

}


#endif
