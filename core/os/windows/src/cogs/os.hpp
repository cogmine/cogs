//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS
#define COGS_HEADER_OS


#define WINVER 0x0A00
#define NTDDI_VERSION NTDDI_WIN10
#define OEMRESOURCE 1

#include <winsock2.h>

#include <stddef.h>
#include <windows.h>
#include <windowsx.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <process.h>
#include <io.h>
#include <richedit.h>
#include <shellscalingapi.h>
#include <cmath>
#include <memory>

#pragma warning(push)
#pragma warning(disable: 4458) // declaration hides class member
#include <gdiplus.h>
#pragma warning(pop)

#undef min
#undef max

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "normaliz.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "ws2_32.lib")

#define SC_RESIZE_LEFT 0xF001
#define SC_RESIZE_RIGHT 0xF002
#define SC_RESIZE_TOP 0xF003
#define SC_RESIZE_TOP_LEFT 0xF004
#define SC_RESIZE_TOP_RIGHT 0xF005
#define SC_RESIZE_BOTTOM 0xF006
#define SC_RESIZE_BOTTOM_LEFT 0xF007
#define SC_RESIZE_BOTTOM_RIGHT 0xF008

#define SC_DRAGMOVE 0xF012

#include "cogs/arch.hpp"

namespace cogs {
namespace os {

class file_system
{
public:
	static constexpr size_t size_bits = 64;
};

inline void beep() { MessageBeep((UINT)-1); }
}

}


#endif
