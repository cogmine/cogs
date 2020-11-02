//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_HWND
#define COGS_HEADER_OS_GUI_GDI_HWND

#include "cogs/os.hpp"
#include <CommCtrl.h>

#include "cogs/env.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/multimap.hpp"
#include "cogs/collections/weak_rcptr_list.hpp"
#include "cogs/function.hpp"
#include "cogs/gui/window.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gfx/device_context.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/thread.hpp"


#ifndef COGS_USE_ATL_THUNK
#define COGS_USE_ATL_THUNK 1
#endif

#if COGS_USE_ATL_THUNK
#include <atlbase.h>
#include <atlwin.h>
#endif


namespace cogs {
namespace os {


class hwnd;

class hwnd_pane;

class window;


inline string get_window_message_string(UINT msg)
{
	switch (msg)
	{
	case WM_NULL:
		return string::literal(L"WM_NULL");
	case WM_CREATE:
		return string::literal(L"WM_CREATE");
	case WM_DESTROY:
		return string::literal(L"WM_DESTROY");
	case WM_MOVE:
		return string::literal(L"WM_MOVE");
	case WM_SIZE:
		return string::literal(L"WM_SIZE");
	case WM_ACTIVATE:
		return string::literal(L"WM_ACTIVATE");
	case WM_SETFOCUS:
		return string::literal(L"WM_SETFOCUS");
	case WM_KILLFOCUS:
		return string::literal(L"WM_KILLFOCUS");
	case WM_ENABLE:
		return string::literal(L"WM_ENABLE");
	case WM_SETREDRAW:
		return string::literal(L"WM_SETREDRAW");
	case WM_SETTEXT:
		return string::literal(L"WM_SETTEXT");
	case WM_GETTEXT:
		return string::literal(L"WM_GETTEXT");
	case WM_GETTEXTLENGTH:
		return string::literal(L"WM_GETTEXTLENGTH");
	case WM_PAINT:
		return string::literal(L"WM_PAINT");
	case WM_CLOSE:
		return string::literal(L"WM_CLOSE");
	case WM_QUERYENDSESSION:
		return string::literal(L"WM_QUERYENDSESSION");
	case WM_QUERYOPEN:
		return string::literal(L"WM_QUERYOPEN");
	case WM_ENDSESSION:
		return string::literal(L"WM_ENDSESSION");
	case WM_QUIT:
		return string::literal(L"WM_QUIT");
	case WM_ERASEBKGND:
		return string::literal(L"WM_ERASEBKGND");
	case WM_SYSCOLORCHANGE:
		return string::literal(L"WM_SYSCOLORCHANGE");
	case WM_SHOWWINDOW:
		return string::literal(L"WM_SHOWWINDOW");
	case WM_WININICHANGE:
		return string::literal(L"WM_WININICHANGE/WM_SETTINGCHANGE");
	case WM_DEVMODECHANGE:
		return string::literal(L"WM_DEVMODECHANGE");
	case WM_ACTIVATEAPP:
		return string::literal(L"WM_ACTIVATEAPP");
	case WM_FONTCHANGE:
		return string::literal(L"WM_FONTCHANGE");
	case WM_TIMECHANGE:
		return string::literal(L"WM_TIMECHANGE");
	case WM_CANCELMODE:
		return string::literal(L"WM_CANCELMODE");
	case WM_SETCURSOR:
		return string::literal(L"WM_SETCURSOR");
	case WM_MOUSEACTIVATE:
		return string::literal(L"WM_MOUSEACTIVATE");
	case WM_CHILDACTIVATE:
		return string::literal(L"WM_CHILDACTIVATE");
	case WM_QUEUESYNC:
		return string::literal(L"WM_QUEUESYNC");
	case WM_GETMINMAXINFO:
		return string::literal(L"WM_GETMINMAXINFO");
	case WM_PAINTICON:
		return string::literal(L"WM_PAINTICON");
	case WM_ICONERASEBKGND:
		return string::literal(L"WM_ICONERASEBKGND");
	case WM_NEXTDLGCTL:
		return string::literal(L"WM_NEXTDLGCTL");
	case WM_SPOOLERSTATUS:
		return string::literal(L"WM_SPOOLERSTATUS");
	case WM_DRAWITEM:
		return string::literal(L"WM_DRAWITEM");
	case WM_MEASUREITEM:
		return string::literal(L"WM_MEASUREITEM");
	case WM_DELETEITEM:
		return string::literal(L"WM_DELETEITEM");
	case WM_VKEYTOITEM:
		return string::literal(L"WM_VKEYTOITEM");
	case WM_CHARTOITEM:
		return string::literal(L"WM_CHARTOITEM");
	case WM_SETFONT:
		return string::literal(L"WM_SETFONT");
	case WM_GETFONT:
		return string::literal(L"WM_GETFONT");
	case WM_SETHOTKEY:
		return string::literal(L"WM_SETHOTKEY");
	case WM_GETHOTKEY:
		return string::literal(L"WM_GETHOTKEY");
	case WM_QUERYDRAGICON:
		return string::literal(L"WM_QUERYDRAGICON");
	case WM_COMPAREITEM:
		return string::literal(L"WM_COMPAREITEM");
	case WM_GETOBJECT:
		return string::literal(L"WM_GETOBJECT");
	case WM_COMPACTING:
		return string::literal(L"WM_COMPACTING");
	case WM_COMMNOTIFY:
		return string::literal(L"WM_COMMNOTIFY");
	case WM_WINDOWPOSCHANGING:
		return string::literal(L"WM_WINDOWPOSCHANGING");
	case WM_WINDOWPOSCHANGED:
		return string::literal(L"WM_WINDOWPOSCHANGED");
	case WM_POWER:
		return string::literal(L"WM_POWER");
	case WM_COPYDATA:
		return string::literal(L"WM_COPYDATA");
	case WM_CANCELJOURNAL:
		return string::literal(L"WM_CANCELJOURNAL");
	case WM_NOTIFY:
		return string::literal(L"WM_NOTIFY");
	case WM_INPUTLANGCHANGEREQUEST:
		return string::literal(L"WM_INPUTLANGCHANGEREQUEST");
	case WM_INPUTLANGCHANGE:
		return string::literal(L"WM_INPUTLANGCHANGE");
	case WM_TCARD:
		return string::literal(L"WM_TCARD");
	case WM_HELP:
		return string::literal(L"WM_HELP");
	case WM_USERCHANGED:
		return string::literal(L"WM_USERCHANGED");
	case WM_NOTIFYFORMAT:
		return string::literal(L"WM_NOTIFYFORMAT");
	case WM_CONTEXTMENU:
		return string::literal(L"WM_CONTEXTMENU");
	case WM_STYLECHANGING:
		return string::literal(L"WM_STYLECHANGING");
	case WM_STYLECHANGED:
		return string::literal(L"WM_STYLECHANGED");
	case WM_DISPLAYCHANGE:
		return string::literal(L"WM_DISPLAYCHANGE");
	case WM_GETICON:
		return string::literal(L"WM_GETICON");
	case WM_SETICON:
		return string::literal(L"WM_SETICON");
	case WM_NCCREATE:
		return string::literal(L"WM_NCCREATE");
	case WM_NCDESTROY:
		return string::literal(L"WM_NCDESTROY");
	case WM_NCCALCSIZE:
		return string::literal(L"WM_NCCALCSIZE");
	case WM_NCHITTEST:
		return string::literal(L"WM_NCHITTEST");
	case WM_NCPAINT:
		return string::literal(L"WM_NCPAINT");
	case WM_NCACTIVATE:
		return string::literal(L"WM_NCACTIVATE");
	case WM_GETDLGCODE:
		return string::literal(L"WM_GETDLGCODE");
	case WM_SYNCPAINT:
		return string::literal(L"WM_SYNCPAINT");
	case WM_NCMOUSEMOVE:
		return string::literal(L"WM_NCMOUSEMOVE");
	case WM_NCLBUTTONDOWN:
		return string::literal(L"WM_NCLBUTTONDOWN");
	case WM_NCLBUTTONUP:
		return string::literal(L"WM_NCLBUTTONUP");
	case WM_NCLBUTTONDBLCLK:
		return string::literal(L"WM_NCLBUTTONDBLCLK");
	case WM_NCRBUTTONDOWN:
		return string::literal(L"WM_NCRBUTTONDOWN");
	case WM_NCRBUTTONUP:
		return string::literal(L"WM_NCRBUTTONUP");
	case WM_NCRBUTTONDBLCLK:
		return string::literal(L"WM_NCRBUTTONDBLCLK");
	case WM_NCMBUTTONDOWN:
		return string::literal(L"WM_NCMBUTTONDOWN");
	case WM_NCMBUTTONUP:
		return string::literal(L"WM_NCMBUTTONUP");
	case WM_NCMBUTTONDBLCLK:
		return string::literal(L"WM_NCMBUTTONDBLCLK");
	case WM_NCXBUTTONDOWN:
		return string::literal(L"WM_NCXBUTTONDOWN");
	case WM_NCXBUTTONUP:
		return string::literal(L"WM_NCXBUTTONUP");
	case WM_NCXBUTTONDBLCLK:
		return string::literal(L"WM_NCXBUTTONDBLCLK");
	case WM_INPUT_DEVICE_CHANGE:
		return string::literal(L"WM_INPUT_DEVICE_CHANGE");
	case WM_INPUT:
		return string::literal(L"WM_INPUT");
	case WM_KEYDOWN:
		return string::literal(L"WM_KEYFIRST/WM_KEYDOWN");
	case WM_KEYUP:
		return string::literal(L"WM_KEYUP");
	case WM_CHAR:
		return string::literal(L"WM_CHAR");
	case WM_DEADCHAR:
		return string::literal(L"WM_DEADCHAR");
	case WM_SYSKEYDOWN:
		return string::literal(L"WM_SYSKEYDOWN");
	case WM_SYSKEYUP:
		return string::literal(L"WM_SYSKEYUP");
	case WM_SYSCHAR:
		return string::literal(L"WM_SYSCHAR");
	case WM_SYSDEADCHAR:
		return string::literal(L"WM_SYSDEADCHAR");
	case WM_KEYLAST:
		return string::literal(L"WM_KEYLAST/WM_UNICHAR");
	case UNICODE_NOCHAR:
		return string::literal(L"UNICODE_NOCHAR");
	case WM_IME_STARTCOMPOSITION:
		return string::literal(L"WM_IME_STARTCOMPOSITION");
	case WM_IME_ENDCOMPOSITION:
		return string::literal(L"WM_IME_ENDCOMPOSITION");
	case WM_IME_KEYLAST:
		return string::literal(L"WM_IME_KEYLAST/WM_IME_COMPOSITION");
	case WM_INITDIALOG:
		return string::literal(L"WM_INITDIALOG");
	case WM_COMMAND:
		return string::literal(L"WM_COMMAND");
	case WM_SYSCOMMAND:
		return string::literal(L"WM_SYSCOMMAND");
	case WM_TIMER:
		return string::literal(L"WM_TIMER");
	case WM_HSCROLL:
		return string::literal(L"WM_HSCROLL");
	case WM_VSCROLL:
		return string::literal(L"WM_VSCROLL");
	case WM_INITMENU:
		return string::literal(L"WM_INITMENU");
	case WM_INITMENUPOPUP:
		return string::literal(L"WM_INITMENUPOPUP");
	case WM_GESTURE:
		return string::literal(L"WM_GESTURE");
	case WM_GESTURENOTIFY:
		return string::literal(L"WM_GESTURENOTIFY");
	case WM_MENUSELECT:
		return string::literal(L"WM_MENUSELECT");
	case WM_MENUCHAR:
		return string::literal(L"WM_MENUCHAR");
	case WM_ENTERIDLE:
		return string::literal(L"WM_ENTERIDLE");
	case WM_MENURBUTTONUP:
		return string::literal(L"WM_MENURBUTTONUP");
	case WM_MENUDRAG:
		return string::literal(L"WM_MENUDRAG");
	case WM_MENUGETOBJECT:
		return string::literal(L"WM_MENUGETOBJECT");
	case WM_UNINITMENUPOPUP:
		return string::literal(L"WM_UNINITMENUPOPUP");
	case WM_MENUCOMMAND:
		return string::literal(L"WM_MENUCOMMAND");
	case WM_CHANGEUISTATE:
		return string::literal(L"WM_CHANGEUISTATE");
	case WM_UPDATEUISTATE:
		return string::literal(L"WM_UPDATEUISTATE");
	case WM_QUERYUISTATE:
		return string::literal(L"WM_QUERYUISTATE");
	case WM_CTLCOLORMSGBOX:
		return string::literal(L"WM_CTLCOLORMSGBOX");
	case WM_CTLCOLOREDIT:
		return string::literal(L"WM_CTLCOLOREDIT");
	case WM_CTLCOLORLISTBOX:
		return string::literal(L"WM_CTLCOLORLISTBOX");
	case WM_CTLCOLORBTN:
		return string::literal(L"WM_CTLCOLORBTN");
	case WM_CTLCOLORDLG:
		return string::literal(L"WM_CTLCOLORDLG");
	case WM_CTLCOLORSCROLLBAR:
		return string::literal(L"WM_CTLCOLORSCROLLBAR");
	case WM_CTLCOLORSTATIC:
		return string::literal(L"WM_CTLCOLORSTATIC");
	case MN_GETHMENU:
		return string::literal(L"MN_GETHMENU");
	case WM_MOUSEMOVE:
		return string::literal(L"WM_MOUSEMOVE/WM_MOUSEFIRST");
	case WM_LBUTTONDOWN:
		return string::literal(L"WM_LBUTTONDOWN");
	case WM_LBUTTONUP:
		return string::literal(L"WM_LBUTTONUP");
	case WM_LBUTTONDBLCLK:
		return string::literal(L"WM_LBUTTONDBLCLK");
	case WM_RBUTTONDOWN:
		return string::literal(L"WM_RBUTTONDOWN");
	case WM_RBUTTONUP:
		return string::literal(L"WM_RBUTTONUP");
	case WM_RBUTTONDBLCLK:
		return string::literal(L"WM_RBUTTONDBLCLK");
	case WM_MBUTTONDOWN:
		return string::literal(L"WM_MBUTTONDOWN");
	case WM_MBUTTONUP:
		return string::literal(L"WM_MBUTTONUP");
	case WM_MBUTTONDBLCLK:
		return string::literal(L"WM_MBUTTONDBLCLK");
	case WM_MOUSEWHEEL:
		return string::literal(L"WM_MOUSEWHEEL");
	case WM_XBUTTONDOWN:
		return string::literal(L"WM_XBUTTONDOWN");
	case WM_XBUTTONUP:
		return string::literal(L"WM_XBUTTONUP");
	case WM_XBUTTONDBLCLK:
		return string::literal(L"WM_XBUTTONDBLCLK");
	case WM_MOUSEHWHEEL:
		return string::literal(L"WM_MOUSEHWHEEL/WM_MOUSELAST");
	case WM_PARENTNOTIFY:
		return string::literal(L"WM_PARENTNOTIFY");
	case WM_ENTERMENULOOP:
		return string::literal(L"WM_ENTERMENULOOP");
	case WM_EXITMENULOOP:
		return string::literal(L"WM_EXITMENULOOP");
	case WM_NEXTMENU:
		return string::literal(L"WM_NEXTMENU");
	case WM_SIZING:
		return string::literal(L"WM_SIZING");
	case WM_CAPTURECHANGED:
		return string::literal(L"WM_CAPTURECHANGED");
	case WM_MOVING:
		return string::literal(L"WM_MOVING");
	case WM_POWERBROADCAST:
		return string::literal(L"WM_POWERBROADCAST");
	case WM_DEVICECHANGE:
		return string::literal(L"WM_DEVICECHANGE");
	case WM_MDICREATE:
		return string::literal(L"WM_MDICREATE");
	case WM_MDIDESTROY:
		return string::literal(L"WM_MDIDESTROY");
	case WM_MDIACTIVATE:
		return string::literal(L"WM_MDIACTIVATE");
	case WM_MDIRESTORE:
		return string::literal(L"WM_MDIRESTORE");
	case WM_MDINEXT:
		return string::literal(L"WM_MDINEXT");
	case WM_MDIMAXIMIZE:
		return string::literal(L"WM_MDIMAXIMIZE");
	case WM_MDITILE:
		return string::literal(L"WM_MDITILE");
	case WM_MDICASCADE:
		return string::literal(L"WM_MDICASCADE");
	case WM_MDIICONARRANGE:
		return string::literal(L"WM_MDIICONARRANGE");
	case WM_MDIGETACTIVE:
		return string::literal(L"WM_MDIGETACTIVE");
	case WM_MDISETMENU:
		return string::literal(L"WM_MDISETMENU");
	case WM_ENTERSIZEMOVE:
		return string::literal(L"WM_ENTERSIZEMOVE");
	case WM_EXITSIZEMOVE:
		return string::literal(L"WM_EXITSIZEMOVE");
	case WM_DROPFILES:
		return string::literal(L"WM_DROPFILES");
	case WM_MDIREFRESHMENU:
		return string::literal(L"WM_MDIREFRESHMENU");
	case WM_POINTERDEVICECHANGE:
		return string::literal(L"WM_POINTERDEVICECHANGE");
	case WM_POINTERDEVICEINRANGE:
		return string::literal(L"WM_POINTERDEVICEINRANGE");
	case WM_POINTERDEVICEOUTOFRANGE:
		return string::literal(L"WM_POINTERDEVICEOUTOFRANGE");
	case WM_TOUCH:
		return string::literal(L"WM_TOUCH");
	case WM_NCPOINTERUPDATE:
		return string::literal(L"WM_NCPOINTERUPDATE");
	case WM_NCPOINTERDOWN:
		return string::literal(L"WM_NCPOINTERDOWN");
	case WM_NCPOINTERUP:
		return string::literal(L"WM_NCPOINTERUP");
	case WM_POINTERUPDATE:
		return string::literal(L"WM_POINTERUPDATE");
	case WM_POINTERDOWN:
		return string::literal(L"WM_POINTERDOWN");
	case WM_POINTERUP:
		return string::literal(L"WM_POINTERUP");
	case WM_POINTERENTER:
		return string::literal(L"WM_POINTERENTER");
	case WM_POINTERLEAVE:
		return string::literal(L"WM_POINTERLEAVE");
	case WM_POINTERACTIVATE:
		return string::literal(L"WM_POINTERACTIVATE");
	case WM_POINTERCAPTURECHANGED:
		return string::literal(L"WM_POINTERCAPTURECHANGED");
	case WM_TOUCHHITTESTING:
		return string::literal(L"WM_TOUCHHITTESTING");
	case WM_POINTERWHEEL:
		return string::literal(L"WM_POINTERWHEEL");
	case WM_POINTERHWHEEL:
		return string::literal(L"WM_POINTERHWHEEL");
	case DM_POINTERHITTEST:
		return string::literal(L"DM_POINTERHITTEST");
	case WM_POINTERROUTEDTO:
		return string::literal(L"WM_POINTERROUTEDTO");
	case WM_POINTERROUTEDAWAY:
		return string::literal(L"WM_POINTERROUTEDAWAY");
	case WM_POINTERROUTEDRELEASED:
		return string::literal(L"WM_POINTERROUTEDRELEASED");
	case WM_IME_SETCONTEXT:
		return string::literal(L"WM_IME_SETCONTEXT");
	case WM_IME_NOTIFY:
		return string::literal(L"WM_IME_NOTIFY");
	case WM_IME_CONTROL:
		return string::literal(L"WM_IME_CONTROL");
	case WM_IME_COMPOSITIONFULL:
		return string::literal(L"WM_IME_COMPOSITIONFULL");
	case WM_IME_SELECT:
		return string::literal(L"WM_IME_SELECT");
	case WM_IME_CHAR:
		return string::literal(L"WM_IME_CHAR");
	case WM_IME_REQUEST:
		return string::literal(L"WM_IME_REQUEST");
	case WM_IME_KEYDOWN:
		return string::literal(L"WM_IME_KEYDOWN");
	case WM_IME_KEYUP:
		return string::literal(L"WM_IME_KEYUP");
	case WM_MOUSEHOVER:
		return string::literal(L"WM_MOUSEHOVER");
	case WM_MOUSELEAVE:
		return string::literal(L"WM_MOUSELEAVE");
	case WM_NCMOUSEHOVER:
		return string::literal(L"WM_NCMOUSEHOVER");
	case WM_NCMOUSELEAVE:
		return string::literal(L"WM_NCMOUSELEAVE");
	case WM_WTSSESSION_CHANGE:
		return string::literal(L"WM_WTSSESSION_CHANGE");
	case WM_TABLET_FIRST:
		return string::literal(L"WM_TABLET_FIRST");
	case WM_TABLET_LAST:
		return string::literal(L"WM_TABLET_LAST");
	case WM_DPICHANGED:
		return string::literal(L"WM_DPICHANGED");
	case WM_DPICHANGED_BEFOREPARENT:
		return string::literal(L"WM_DPICHANGED_BEFOREPARENT");
	case WM_DPICHANGED_AFTERPARENT:
		return string::literal(L"WM_DPICHANGED_AFTERPARENT");
	case WM_GETDPISCALEDSIZE:
		return string::literal(L"WM_GETDPISCALEDSIZE");
	case WM_CUT:
		return string::literal(L"WM_CUT");
	case WM_COPY:
		return string::literal(L"WM_COPY");
	case WM_PASTE:
		return string::literal(L"WM_PASTE");
	case WM_CLEAR:
		return string::literal(L"WM_CLEAR");
	case WM_UNDO:
		return string::literal(L"WM_UNDO");
	case WM_RENDERFORMAT:
		return string::literal(L"WM_RENDERFORMAT");
	case WM_RENDERALLFORMATS:
		return string::literal(L"WM_RENDERALLFORMATS");
	case WM_DESTROYCLIPBOARD:
		return string::literal(L"WM_DESTROYCLIPBOARD");
	case WM_DRAWCLIPBOARD:
		return string::literal(L"WM_DRAWCLIPBOARD");
	case WM_PAINTCLIPBOARD:
		return string::literal(L"WM_PAINTCLIPBOARD");
	case WM_VSCROLLCLIPBOARD:
		return string::literal(L"WM_VSCROLLCLIPBOARD");
	case WM_SIZECLIPBOARD:
		return string::literal(L"WM_SIZECLIPBOARD");
	case WM_ASKCBFORMATNAME:
		return string::literal(L"WM_ASKCBFORMATNAME");
	case WM_CHANGECBCHAIN:
		return string::literal(L"WM_CHANGECBCHAIN");
	case WM_HSCROLLCLIPBOARD:
		return string::literal(L"WM_HSCROLLCLIPBOARD");
	case WM_QUERYNEWPALETTE:
		return string::literal(L"WM_QUERYNEWPALETTE");
	case WM_PALETTEISCHANGING:
		return string::literal(L"WM_PALETTEISCHANGING");
	case WM_PALETTECHANGED:
		return string::literal(L"WM_PALETTECHANGED");
	case WM_HOTKEY:
		return string::literal(L"WM_HOTKEY");
	case WM_PRINT:
		return string::literal(L"WM_PRINT");
	case WM_PRINTCLIENT:
		return string::literal(L"WM_PRINTCLIENT");
	case WM_APPCOMMAND:
		return string::literal(L"WM_APPCOMMAND");
	case WM_THEMECHANGED:
		return string::literal(L"WM_THEMECHANGED");
	case WM_CLIPBOARDUPDATE:
		return string::literal(L"WM_CLIPBOARDUPDATE");
	case WM_DWMCOMPOSITIONCHANGED:
		return string::literal(L"WM_DWMCOMPOSITIONCHANGED");
	case WM_DWMNCRENDERINGCHANGED:
		return string::literal(L"WM_DWMNCRENDERINGCHANGED");
	case WM_DWMCOLORIZATIONCOLORCHANGED:
		return string::literal(L"WM_DWMCOLORIZATIONCOLORCHANGED");
	case WM_DWMWINDOWMAXIMIZEDCHANGE:
		return string::literal(L"WM_DWMWINDOWMAXIMIZEDCHANGE");
	case WM_DWMSENDICONICTHUMBNAIL:
		return string::literal(L"WM_DWMSENDICONICTHUMBNAIL");
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		return string::literal(L"WM_DWMSENDICONICLIVEPREVIEWBITMAP");
	case WM_GETTITLEBARINFOEX:
		return string::literal(L"WM_GETTITLEBARINFOEX");
	case WM_HANDHELDFIRST:
		return string::literal(L"WM_HANDHELDFIRST");
	case WM_HANDHELDLAST:
		return string::literal(L"WM_HANDHELDLAST");
	case WM_AFXFIRST:
		return string::literal(L"WM_AFXFIRST");
	case WM_AFXLAST:
		return string::literal(L"WM_AFXLAST");
	case WM_PENWINFIRST:
		return string::literal(L"WM_PENWINFIRST");
	case WM_PENWINLAST:
		return string::literal(L"WM_PENWINLAST");
	case WM_APP:
		return string::literal(L"WM_APP");
	case WM_USER:
		return string::literal(L"WM_USER");

	default:
		int_to_fixed_integer_t<UINT> i = msg;
		return i.to_string();
	}
}


// hwnd is an RAII wrapper around an Win32 HWND object.
// hwnd also serves as a namespace for other classes involved in the hwnd GUI subsystem.
class hwnd : public object
{
public:
	class subsystem;

	// window_class wraps a Win32 WindowClass (via RegisterClassEx).  Provides WinProc to Windows,
	// and routes all windows messages/events to our window class.
	class window_class
	{
	private:
		ATOM m_atom;
		HMODULE MsfteditDll;

	protected:
		window_class()
		{
			WNDCLASSEX wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.lpfnWndProc = (WNDPROC)DefWindowProc;
			wcex.cbWndExtra = sizeof(hwnd*);
			wcex.hInstance = GetModuleHandle(0);

			// Windows insists we provide a class name string.
			// It won't accept NULL, a blank string, or a string already used by another window class.
			// So, we'll generate a unique name from the this ptr of this window_class.

			// Put least significant byte first to trip string compares quicker.
			// Use only ASCII printable charactes 0x21 thru 0x7E, just in case.
			// 0x7E - 0x21 = 0x5D
			size_t srcAddr = (size_t)this;
			string addrStr;
			do {
				addrStr.append((wchar_t)(srcAddr % 0x5D)+0x21);
				srcAddr /= 93;
			} while (srcAddr != 0);
			wcex.lpszClassName = addrStr.cstr();

			m_atom = RegisterClassEx(&wcex);

			// XP required an explicit loading of controls.  Load all the ones we have classes for.
			INITCOMMONCONTROLSEX initCtrls;
			initCtrls.dwSize = sizeof(initCtrls);
			initCtrls.dwICC = ICC_STANDARD_CLASSES;
			InitCommonControlsEx(&initCtrls);

			// Load MSFTEDIT control
			MsfteditDll = LoadLibrary(TEXT("Msftedit.dll"));
		}

	public:
		~window_class()
		{
			UnregisterClass((LPCTSTR)(bytes_to_int_t<sizeof(LPCTSTR)>)m_atom, GetModuleHandle(0));
			//FreeLibrary(MsfteditDll);
		}

		ATOM get_ATOM() const { return m_atom; }

		static rcref<window_class> get_default() { return singleton<window_class>::get(); }
	};

	class ui_thread : public dispatcher, public object
	{
	private:
		rcptr<cogs::thread> m_thread;
		const HANDLE m_queueEvent;

		bool m_reentrancyGuard = false;
		MSG m_lastMsg;
		HWND m_lastActivatedWindow = NULL;

		priority_dispatcher m_controlQueue;

		static void thread_main(const weak_rcptr<ui_thread>& uiThread)
		{
			weak_rcptr<ui_thread> weakRef = uiThread;
			rcptr<ui_thread> strongRef = uiThread;
			volatile priority_dispatcher& controlQueue = strongRef->m_controlQueue;

			strongRef->self_release(); // Release the extra reference we acquired before spawning the thread

			HANDLE queueEvent = strongRef->m_queueEvent;
			for (;;)
			{
				strongRef.release();
				MsgWaitForMultipleObjects(1, &queueEvent, FALSE, INFINITE, QS_ALLINPUT);
				strongRef = weakRef;
				if (!strongRef)
					break;

				int priority = 0x00010000; // UI subsystem runs at 0x00010000 priority.
				strongRef->m_reentrancyGuard = true;
				for (;;) // since we have a reference to self, we won't be deleted in this block
				{
					if (!!controlQueue.try_invoke(priority))
					{
						priority = 0x00010000;
						continue;
					}
					strongRef->m_reentrancyGuard = false;

					if (!PeekMessage(&(strongRef->m_lastMsg), NULL, 0, 0, PM_REMOVE))
					{
						if (controlQueue.is_empty())
							break;

						strongRef->m_reentrancyGuard = true;
						priority = const_max_int_v<int>;
						continue;
					}

					// Handle thread messages
					switch (strongRef->m_lastMsg.message)
					{
					case WM_QUIT: // Might get this if another app wants us to quit.
						quit_dispatcher::get()->request();
						break;
					default:
						break;
					}

					HWND activeHWND = strongRef->get_last_activate_window();
					if ((activeHWND == NULL) || !IsDialogMessage(activeHWND, &(strongRef->m_lastMsg)))
					{
			// TBD if (!TranslateAccelerator(strongRef->m_lastMsg.hwnd, strongRef->accelerators, &(strongRef->m_lastMsg)))
						{
							TranslateMessage(&(strongRef->m_lastMsg));
							DispatchMessage(&(strongRef->m_lastMsg));
						}
					}
				}
			}
			CloseHandle(queueEvent);
		}

	protected:
		friend class subsystem;

		bool run_high_priority_tasks(bool runNextIdleTask = false)
		{
			bool calledAny = false;
			if (!m_reentrancyGuard)
			{
				m_reentrancyGuard = true;
				volatile priority_dispatcher& controlQueue = m_controlQueue;
				int priority = 0x00010000; // UI subsystem runs at 0x00010000 priority.
				for (;;)
				{
					if (!!controlQueue.try_invoke(priority))
					{
						calledAny = true;
						priority = 0x00010000;
						continue;
					}

					if (!runNextIdleTask)
						break;

					runNextIdleTask = false;
					priority = const_max_int_v<int>;
					continue;
				}

				m_reentrancyGuard = false;
			}

			return calledAny;
		}

		// Process tasks until a particular task is complete.
		// Effectively stalls processing of OS UI events until a particular task is complete.
		void process_tasks_until(const rcref<task<void> >& t)
		{
			if (t->is_signaled())
				return;

			HANDLE queueEvent = m_queueEvent;
			t->dispatch([queueEvent]()
			{
				SetEvent(queueEvent);
			});

			for (;;)
			{
				WaitForSingleObject(queueEvent, INFINITE);
				for (;;)
				{
					if (t->is_signaled())
						return;

					volatile priority_dispatcher& controlQueue = m_controlQueue;
					if (!controlQueue.try_invoke(const_max_int_v<int>))
						break;
				}
			}
		}

		MSG& get_msg() { return m_lastMsg; }
		HWND& get_last_activate_window() { return m_lastActivatedWindow; }

		rcptr<const cogs::thread> get_ui_thread() const { return m_thread; }

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			dispatcher::dispatch_inner(m_controlQueue, t, priority);
			SetEvent(m_queueEvent);
		}

	public:
		ui_thread()
			: m_queueEvent(CreateEvent(NULL, FALSE, TRUE, NULL))
		{
			self_acquire();
			m_thread = cogs::thread::spawn([r{ this_weak_rcptr }]()
			{
				thread_main(r);
			});
		}

		~ui_thread()
		{
			COGS_ASSERT(m_controlQueue.is_empty());
			SetEvent(m_queueEvent);
		}
	};

	class subsystem : public gui::windowing::subsystem
	{
	public:
		typedef container_dlist<rcref<gui::window> > visible_windows_list_t;

	private:
		ui_thread m_uiThread;
		rcptr<volatile visible_windows_list_t> m_visibleWindows;
		rcref<window_class> m_windowClass;
		rcptr<task<void> > m_cleanupRemoveToken;

		void cleanup() volatile
		{
			// Visible windows keep the subsystem in scope.  In case windows are left open
			// until the app closes, use a cleanup function to ensure they get closed.
			m_visibleWindows.release();
		}

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			dispatcher::dispatch_inner(m_uiThread, t, priority);
		}

	public:
		subsystem()
			: m_visibleWindows(rcnew(visible_windows_list_t)),
			m_windowClass(window_class::get_default()),
			m_cleanupRemoveToken(cleanup_queue::get()->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<subsystem> r2 = r;
				if (!!r2)
					r2->cleanup();
			}))
		{
		}

		~subsystem()
		{
			m_cleanupRemoveToken->cancel();
		}

		MSG& get_msg() const volatile { return ((subsystem*)this)->m_uiThread.get_msg(); }
		HWND& get_last_activate_window() const volatile { return ((subsystem*)this)->m_uiThread.get_last_activate_window(); }

		rcptr<const cogs::thread> get_ui_thread() const volatile { return ((const subsystem*)this)->m_uiThread.get_ui_thread(); }

		virtual bool is_ui_thread_current() const volatile { return get_ui_thread()->is_current(); }

		bool run_high_priority_tasks(bool runNextIdleTask = false) const volatile { return ((subsystem*)this)->m_uiThread.run_high_priority_tasks(runNextIdleTask); }

		void remove_visible_window(visible_windows_list_t::volatile_remove_token& removeToken) volatile
		{
			rcptr<volatile visible_windows_list_t> visibleWindows = m_visibleWindows;
			if (!!visibleWindows)
				visibleWindows->remove(removeToken);
		}

		visible_windows_list_t::volatile_remove_token add_visible_window(const rcref<gui::window>& windowBridge) volatile
		{
			rcptr<volatile visible_windows_list_t> visibleWindows = m_visibleWindows;
			if (!!visibleWindows)
				return visibleWindows->prepend(windowBridge).inserted;
			return visible_windows_list_t::volatile_remove_token();
		}

		// ui::subsystem interface
		virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::button_interface> > create_button() volatile;
		virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::check_box_interface> > create_check_box() volatile;
		virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::text_editor_interface> > create_text_editor() volatile;
		virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::scroll_bar_interface> > create_scroll_bar() volatile;
		virtual rcref<gui::bridgeable_pane> create_native_pane() volatile;
		virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::window_interface> > create_window() volatile;

		virtual rcref<task<void> > message(const composite_string& s) volatile
		{
			// TODO: Update to use button_box, allow async
			MessageBox(NULL, s.composite().cstr(), L"", MB_OK);
			return signaled();
		}

		virtual vector<gfx::bounds> get_screens() volatile
		{
			vector<gfx::bounds> result;
			for (DWORD i = 0;; i++)
			{
				DISPLAY_DEVICE dd;
				dd.cb = sizeof(DISPLAY_DEVICE);
				BOOL b = EnumDisplayDevicesW(NULL, i, &dd, 0);
				if (!b)
					break;
				if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)
				{
					DEVMODE dm;
					dm.dmSize = sizeof(DEVMODE);
					dm.dmDriverExtra = 0;
					EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0);
					result.append(1, { { (double)dm.dmPosition.x, (double)dm.dmPosition.y }, { (double)dm.dmPelsWidth, (double)dm.dmPelsHeight } });
				}
			}
			return result;
		}

		virtual rcref<gui::window> open_window(window_options&& options) volatile;
	};

private:
	HWND m_hWnd;
	WNDPROC m_defaultWndProc;
	rcptr<hwnd::window_class> m_windowClass;
	rcref<volatile subsystem> m_uiSubsystem;
	weak_rcptr<hwnd_pane> m_owner;
	rcptr<hwnd> m_parentHwndPane;
	SIZE m_minimumWindowSize;

#if COGS_USE_ATL_THUNK
	AtlThunkData_t* m_atlThunk;
	static LRESULT CALLBACK AtlThunkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
	nonvolatile_map<HWND, hwnd*>::iterator m_hWndMapRemoveToken;
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK UnownedClassWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

public:
	hwnd(const rcptr<hwnd>& parent,
		const rcptr<hwnd>& belowThis,
		const rcref<hwnd_pane>& owner,
		const composite_string& windowClassName,
		const composite_string& caption,
		DWORD& style,
		DWORD& extendedStyle,
		const rcref<volatile subsystem>& uiSubsystem,
		SIZE* minimumWindowSize)
		: m_uiSubsystem(uiSubsystem),
		m_owner(owner),
		m_parentHwndPane(parent)
	{
#if COGS_USE_ATL_THUNK
		m_atlThunk = AtlThunk_AllocateData();
#endif

		HWND parentHWND;
		extendedStyle |= WS_EX_NOPARENTNOTIFY;
		if (!!parent) // if child
		{
			parentHWND = parent->get_HWND();
			style |= WS_CHILD;
		}
		else // if window
		{
			parentHWND = 0;
#ifndef COGS_DEBUG
			// To debug drawing, disabling WS_EX_COMPOSITED will remove some unnecessary WM_PAINTs,
			// such as when context switching to debug in in VS.
			extendedStyle |= WS_EX_COMPOSITED;
#endif
			style |= WS_SYSMENU | WS_OVERLAPPED;
			if (!caption.is_empty())
				style |= WS_CAPTION;
			style |= WS_THICKFRAME;
		}
		style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		bool isUnownedClass = !!windowClassName;
		if (!isUnownedClass)
			m_windowClass = window_class::get_default();

		HWND hWnd = CreateWindowEx(extendedStyle,
			isUnownedClass ? (LPCTSTR)(windowClassName.composite().cstr()) : (LPCTSTR)((bytes_to_int_t<sizeof(LPCTSTR)>)m_windowClass->get_ATOM()),
			caption.composite().cstr(), style,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			parentHWND, 0, (HINSTANCE)(GetModuleHandle(0)), 0);
		COGS_ASSERT(hWnd != NULL);

		if (!!parent)
			SetWindowPos(hWnd, !belowThis ? HWND_TOP : belowThis->m_hWnd, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
		else if (minimumWindowSize != nullptr)
		{
			// If a window, determine it's minimum sizes.

			// First save the size chosen by CW_USEDEFAULT.  This will be a fallback default size, if content has none.
			RECT defaultRect;
			GetWindowRect(hWnd, &defaultRect);
			SIZE defaultSIZE = { defaultRect.right - defaultRect.left, defaultRect.bottom - defaultRect.top };

			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

			RECT minimumRect;
			GetClientRect(hWnd, &minimumRect);

			// Hopefully this doesn't change when DPI changes.
			// It doesn't seem like it should, or it could cause resizes when moving between windows.
			*minimumWindowSize = { minimumRect.right - minimumRect.left, minimumRect.bottom - minimumRect.top };

			// Restore to default position
			SetWindowPos(hWnd, HWND_TOP, 0, 0, defaultSIZE.cx, defaultSIZE.cy, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		m_hWnd = hWnd;

		m_defaultWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);

#if COGS_USE_ATL_THUNK
		AtlThunk_InitData(m_atlThunk, (void*)AtlThunkWndProc, (size_t)this);
		WNDPROC thunkWndProc = AtlThunk_DataToCode(m_atlThunk);
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)thunkWndProc);
#else
		if (isUnownedClass)
		{
			auto p = get_hwnd_map().insert_unique(hWnd, this);
			m_hWndMapRemoveToken = std::move(p.inserted);
			COGS_ASSERT(!!p.inserted);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)UnownedClassWndProc);
		}
		else
		{
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)WndProc);

			// GWLP_USERDATA is owned by the window class, so can only be used if using our own window class.
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(this));
		}
#endif
	}

#if !COGS_USE_ATL_THUNK
	static nonvolatile_map<HWND, hwnd*>& get_hwnd_map()
	{
		thread_local static placement<nonvolatile_map<HWND, hwnd*> > storage;
		return storage.get();
	}
#endif

	HWND get_HWND() { return m_hWnd; }
	const weak_rcptr<hwnd_pane>& get_owner() const { return m_owner; }
	const rcptr<hwnd>& get_parent_hwnd() const { return m_parentHwndPane; }

	void release()
	{
		HWND hWnd = m_hWnd;
		m_hWnd = NULL;
		DestroyWindow(hWnd);
#if COGS_USE_ATL_THUNK
		AtlThunk_FreeData(m_atlThunk);
#else
		if (!m_windowClass)
			hwnd::get_hwnd_map().remove(m_hWndMapRemoveToken);
#endif
	}

	LRESULT call_default_window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return CallWindowProc(m_defaultWndProc, m_hWnd, msg, wParam, lParam);
	}
};

enum class hwnd_draw_mode
{
	user = 0, // Drawn by pane::draw()
	system_direct = 1, // Drawn by the system directly to the display DC
	system_offscreen = 2, // Drawn by the system in response to WM_PAINT against the DC passed in WPARAM, in offscreen buffer
};


// hwnd_pane is a base for all hwnd panes.
class hwnd_pane : public object, public gui::bridgeable_pane, public gui::pane_orchestrator
{
private:
	rcref<volatile hwnd::subsystem> m_uiSubsystem;
	rcptr<hwnd> m_hwnd;
	rcref<gdi::device_context> m_deviceContext = rcnew(gdi::device_context);
	rcptr<hwnd_pane> m_parentHwndPane;
	weak_rcptr<hwnd_pane> m_prevSiblingHwnd;
	weak_rcptr<hwnd_pane> m_nextSiblingHwnd;
	weak_rcptr<hwnd_pane> m_firstChildHwnd;
	weak_rcptr<hwnd_pane> m_lastChildHwnd;
	weak_rcptr_list<gdi::bitmap> m_offscreenBuffers;
	size_t m_childCount = 0;
	composite_string m_windowClassName;
	hwnd_draw_mode m_drawMode; // i.e. window or canvas hwnd
	bool m_focusing = false;
	bool m_isVisible = false;
	ui::modifier_keys_state m_lastModifierKeysState = {};

protected:
	HRGN m_redrawRgn = NULL;
	rcptr<gdi::bitmap> m_cachedBackgroundImage;
	DWORD m_style;
	DWORD m_extendedStyle;
	composite_string m_caption;
	BOUNDS m_boundsInParentHwnd = {};
	gfx::size m_minimumWindowSize;

	gdi::device_context& get_device_context() { return *m_deviceContext; }
	const gdi::device_context& get_device_context() const { return *m_deviceContext; }

	HDC& get_HDC() { return get_device_context().get_HDC(); }
	const HDC& get_HDC() const { return get_device_context().get_HDC(); }

	rcref<volatile hwnd::subsystem>& get_subsystem() { return m_uiSubsystem; }
	const rcref<volatile hwnd::subsystem>& get_subsystem() const { return m_uiSubsystem; }

	// Converts special keys that MapVirtualKey fails to map, to special unicode values (same as NSEvent on Mac)
	wchar_t translate_special_key(const wchar_t& c)
	{
		wchar_t result = 0;
		switch (c)
		{
		case VK_UP:
			result = ui::special_keys::UpArrowKey;
			break;
		case VK_DOWN:
			result = ui::special_keys::DownArrowKey;
			break;
		case VK_LEFT:
			result = ui::special_keys::LeftArrowKey;
			break;
		case VK_RIGHT:
			result = ui::special_keys::RightArrowKey;
			break;
		case VK_F1:
			result = ui::special_keys::F1Key;
			break;
		case VK_F2:
			result = ui::special_keys::F2Key;
			break;
		case VK_F3:
			result = ui::special_keys::F3Key;
			break;
		case VK_F4:
			result = ui::special_keys::F4Key;
			break;
		case VK_F5:
			result = ui::special_keys::F5Key;
			break;
		case VK_F6:
			result = ui::special_keys::F6Key;
			break;
		case VK_F7:
			result = ui::special_keys::F7Key;
			break;
		case VK_F8:
			result = ui::special_keys::F8Key;
			break;
		case VK_F9:
			result = ui::special_keys::F9Key;
			break;
		case VK_F10:
			result = ui::special_keys::F10Key;
			break;
		case VK_F11:
			result = ui::special_keys::F11Key;
			break;
		case VK_F12:
			result = ui::special_keys::F12Key;
			break;
		case VK_F13:
			result = ui::special_keys::F13Key;
			break;
		case VK_F14:
			result = ui::special_keys::F14Key;
			break;
		case VK_F15:
			result = ui::special_keys::F15Key;
			break;
		case VK_F16:
			result = ui::special_keys::F16Key;
			break;
		case VK_F17:
			result = ui::special_keys::F17Key;
			break;
		case VK_F18:
			result = ui::special_keys::F18Key;
			break;
		case VK_F19:
			result = ui::special_keys::F19Key;
			break;
		case VK_F20:
			result = ui::special_keys::F20Key;
			break;
		case VK_F21:
			result = ui::special_keys::F21Key;
			break;
		case VK_F22:
			result = ui::special_keys::F22Key;
			break;
		case VK_F23:
			result = ui::special_keys::F23Key;
			break;
		case VK_F24:
			result = ui::special_keys::F24Key;
			break;
	//	case VK_F25:
	//		result = ui::special_keys::F25Key;
	//		break;
	//	case VK_F26:
	//		result = ui::special_keys::F26Key;
	//		break;
	//	case VK_F27:
	//		result = ui::special_keys::F27Key;
	//		break;
	//	case VK_F28:
	//		result = ui::special_keys::F28Key;
	//		break;
	//	case VK_F29:
	//		result = ui::special_keys::F29Key;
	//		break;
	//	case VK_F30:
	//		result = ui::special_keys::F30Key;
	//		break;
	//	case VK_F31:
	//		result = ui::special_keys::F31Key;
	//		break;
	//	case VK_F32:
	//		result = ui::special_keys::F32Key;
	//		break;
	//	case F33Key:
	//	case F34Key:
	//	case F35Key:
		case VK_INSERT:
			result = ui::special_keys::InsertKey;
			break;
		case VK_DELETE:
			result = ui::special_keys::DeleteKey;
			break;
		case VK_HOME:
			result = ui::special_keys::HomeKey;
			break;
	//	case BeginKey:
		case VK_END:
			result = ui::special_keys::EndKey;
			break;
		case VK_PRIOR:
			result = ui::special_keys::PageUpKey;
			break;
		case VK_NEXT:
			result = ui::special_keys::PageDownKey;
			break;
		case VK_SNAPSHOT:
			result = ui::special_keys::PrintScreenKey;
			break;
		case VK_SCROLL:
			result = ui::special_keys::ScrollLockKey;
			break;
		case VK_PAUSE:
			result = ui::special_keys::PauseKey;
			break;
	//	case SysReqKey:
		case VK_CANCEL: // ?
			result = ui::special_keys::BreakKey;
			break;
	//	case ResetKey:
	//	case StopKey:
	//	case MenuKey:
	//	case UserKey:
	//	case SystemKey:
	//	case PrintKey:
	//	case ClearLineKey:
	//	case ClearDisplayKey:
	//	case InsertLineKey:
	//	case DeleteLineKey:
	//	case InsertCharKey:
	//	case DeleteCharKey:
	//	case NextKey:
	//	case PrevKey:
	//	case SelectKey:
		case VK_EXECUTE:
			result = ui::special_keys::ExecuteKey;
			break;
	//	case UndoKey:
	//	case RedoKey:
	//	case FindKey:
		case VK_HELP:
			result = ui::special_keys::HelpKey;
			break;
	//	case ModeSwitchKey:
		default:
			break;
		}
	//	COGS_ASSERT(result); // temporary: catch codes not already mapped.
		return result;
	}

	using bridgeable_pane::invalidate;

	void draw_if_needed()
	{
		if (m_drawMode == hwnd_draw_mode::user)
		{
			rcptr<gdi::bitmap> offScreenBuffer = peek_offscreen_buffer().template static_cast_to<gdi::bitmap>();
			COGS_ASSERT(!!offScreenBuffer);
			if (is_drawing_needed())
			{
				HDC offscreenDC = offScreenBuffer->get_HDC();
				HRGN updateRgn;
				updateRgn = m_redrawRgn;
				m_redrawRgn = NULL;
				SelectClipRgn(offscreenDC, updateRgn); // selecting into offscreen buffer
				draw();
				SelectClipRgn(offscreenDC, NULL); // unclip the offscreen buffer
				return;
			}
		}
	}

	void allocate_temporary_background()
	{
		if (!!m_parentHwndPane && !get_bridge()->is_opaque())
		{
			gfx::bounds b = get_device_context().make_bounds(m_boundsInParentHwnd);

			bool needNewBuffer;
			if (!m_cachedBackgroundImage)
				needNewBuffer = true;
			else
			{
				gfx::size cacheSize = m_cachedBackgroundImage->get_size();
				needNewBuffer = ((b.get_width() > cacheSize.get_width()) || (b.get_height() > cacheSize.get_height()));
			}
			if (needNewBuffer)
			{
				gfx::size newSize = b.get_size() + gfx::size(25, 25); // add a little space to grow, to avoid creating buffers too often.
				// If this is the topmost (window) HWND, fill the buffer with the default window background theme color.
				color c = !m_parentHwndPane ? color::constant::black : color::constant::white;
				m_cachedBackgroundImage = m_deviceContext->create_bitmap(newSize, c, get_device_context().get_dpi()).template static_cast_to<gdi::bitmap>();
			}
		}
	}

	void paint_temporary_background()
	{
		// Because HWNDs are externally rendered, their parent panes are drawn without them,
		// which essentially means HWND z-order begins after all non-HWND z-order.

		// To support transpancy, we need to capture a background image starting from the
		// nearest non-opaque parent HWND, and including all overlapping HWNDs with prior z-order.

		if (!!m_parentHwndPane && !get_bridge()->is_opaque())
		{
			gfx::bounds b = get_device_context().make_bounds(m_boundsInParentHwnd);

			// go up the tree to the next opaque (or the topmost) HWND
			rcptr<hwnd_pane> ancestorHwnd = m_parentHwndPane;
			do {
				rcptr<hwnd_pane> nextAncestorHwnd = ancestorHwnd->m_parentHwndPane;
				if (!nextAncestorHwnd)
					break;
				b.get_position() += ancestorHwnd->get_device_context().make_point(ancestorHwnd->m_boundsInParentHwnd.pt);
				ancestorHwnd = std::move(nextAncestorHwnd);
			} while (!ancestorHwnd->get_bridge()->is_opaque());

			rcptr<gdi::bitmap> offScreenBuffer = ancestorHwnd->peek_offscreen_buffer().template static_cast_to<gdi::bitmap>();
			if (!offScreenBuffer)
				return;
			COGS_ASSERT(!!offScreenBuffer);
			ancestorHwnd->draw_if_needed();

			// Render all overlapping HWNDs, in z-order, until we get to this pane
			m_cachedBackgroundImage->draw_bitmap(*offScreenBuffer, b, b.get_size(), false);

			rcptr<hwnd_pane> curHwnd = ancestorHwnd;
			for (;;)
			{
				curHwnd = curHwnd->m_firstChildHwnd;
				if (curHwnd == this)
					break;
				COGS_ASSERT(!!curHwnd); // Should have found this obj
				b.get_position() += -curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt); // Keep in same coords as curHwnd

				for (;;)
				{
					if (curHwnd->m_isVisible)
					{
						gfx::bounds b2(gfx::point(0, 0), curHwnd->get_size());
						gfx::bounds b3 = b & b2;
						if (!!b3)
						{
							offScreenBuffer = curHwnd->peek_offscreen_buffer().template static_cast_to<gdi::bitmap>();
							if (!offScreenBuffer)
								return; // Not rendering in z-order.  Skip.  Will self-heal as underlying pane draws.

							curHwnd->draw_if_needed();
							m_cachedBackgroundImage->draw_bitmap(*offScreenBuffer, b2, gfx::bounds(b2.get_position() + -b.get_position(), b2.get_size()), true);
						}
					}

					if (!!curHwnd->m_firstChildHwnd)
						break;

					for (;;)
					{
						if (!!curHwnd->m_nextSiblingHwnd)
						{
							b.get_position() += curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt); // Keep in same coords as curHwnd
							curHwnd = curHwnd->m_nextSiblingHwnd;
							if (curHwnd == this)
								return;
							b.get_position() += -curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt); // Keep in same coords as curHwnd
							break;
						}
						b.get_position() += curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt);
						curHwnd = curHwnd->m_parentHwndPane;
					}
				}
			}
		}
	}

	virtual rcref<gfx::bitmap> create_offscreen_buffer(gui::pane&, const gfx::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		rcref<gfx::bitmap> result = get_device_context().create_bitmap(sz, fillColor, get_device_context().get_dpi());
		m_offscreenBuffers.append(result.static_cast_to<gdi::bitmap>());
		return result;
	}

public:
	hwnd_pane(const composite_string& windowClassName, DWORD style, DWORD extendedStyle, const rcref<volatile hwnd::subsystem>& uiSubsystem, hwnd_draw_mode drawMode)
		: m_uiSubsystem(uiSubsystem),
		m_windowClassName(windowClassName),
		m_drawMode(drawMode),
		m_style(style),
		m_extendedStyle(extendedStyle)
	{ }

	~hwnd_pane()
	{
		if (m_redrawRgn != NULL)
			DeleteObject(m_redrawRgn);
		if (m_uiSubsystem->is_ui_thread_current())
			m_hwnd->release();
		else
		{
			// If destructed from a thread other than the UI thread, defer the call to DestroyWindow
			m_uiSubsystem->dispatch([r{ m_hwnd.dereference() }]()
			{
				r->release();
			});
		}
	}

	const rcptr<hwnd>& get_hwnd() const { return m_hwnd; }
	HWND get_HWND() const { return (!m_hwnd) ? NULL : m_hwnd->get_HWND(); }

	DWORD get_style() const { return m_style; }
	void set_style(DWORD newStyle) { m_style = newStyle; }

	DWORD get_extended_style() const { return m_extendedStyle; }
	void set_extended_style(DWORD newExtendedStyle) { m_extendedStyle = newExtendedStyle; }

	LRESULT call_default_window_proc(UINT msg, WPARAM wParam, LPARAM lParam) { return m_hwnd->call_default_window_proc(msg, wParam, lParam); }

	virtual void uninstalling()
	{
		m_isVisible = false;
		if (!!m_parentHwndPane)
		{
			m_parentHwndPane->m_childCount--;
			rcptr<hwnd_pane> prevSibling = m_prevSiblingHwnd;
			rcptr<hwnd_pane> nextSibling = m_nextSiblingHwnd;
			if (!!prevSibling)
				prevSibling->m_nextSiblingHwnd = nextSibling;
			if (!!nextSibling)
				nextSibling->m_prevSiblingHwnd = prevSibling;

			if (m_parentHwndPane->m_firstChildHwnd == this)
				m_parentHwndPane->m_firstChildHwnd = nextSibling;
			if (m_parentHwndPane->m_lastChildHwnd == this)
				m_parentHwndPane->m_lastChildHwnd = prevSibling;
		}

		ReleaseDC(get_HWND(), get_HDC());
		get_HDC() = NULL;
		bridgeable_pane::uninstalling();
	}

	void install_HWND()
	{
		rcptr<gui::pane> p = get_parent();
		while (!!p)
		{
			gui::pane_bridge* bridge = p.template dynamic_cast_to<gui::pane_bridge>().get_ptr();
			if (!!bridge)
				m_parentHwndPane = get_bridged(*bridge).template dynamic_cast_to<hwnd_pane>();
			if (!m_parentHwndPane)
			{
				p = p->get_parent();
				continue;
			}

			// Find previous z-order HWND by scanning descendant tree of m_parentHwndPane until we find this object.
			// Whenever another HWND is covered, do not traverse its descendants.
			// Once we find this object, insert it after the last HWND seen.

			container_dlist<rcref<gui::pane> >::iterator itor;
			rcptr<hwnd_pane> lastFoundHwnd;
			if (m_parentHwndPane->m_childCount++ > 0) // if was 0
			{
				itor = get_bridge(*m_parentHwndPane)->get_children().get_first();
				COGS_ASSERT(!!itor); // should at least find this obj
				container_dlist<rcref<gui::pane> >::iterator nextItor;
				// start at furthest last
				for (;;)
				{
					for (;;)
					{
						if (*itor == get_bridge())
						{
							itor.release();
							break;
						}

						rcptr<hwnd_pane> foundHwnd;
						bridge = itor->dynamic_cast_to<gui::pane_bridge>().get_ptr();
						if (!!bridge)
							foundHwnd = get_bridged(*bridge).template dynamic_cast_to<hwnd_pane>();

						if (!!foundHwnd) // Don't descend beyond another HWND, move on to the next
						{
							lastFoundHwnd = foundHwnd;
							break;
						}

						nextItor = (*itor)->get_children().get_first();
						if (!nextItor)
							break;

						itor = nextItor;
					}

					if (!itor)
						break;

					for (;;)
					{
						COGS_ASSERT(!!itor);
						nextItor = itor;
						++nextItor;
						if (!!nextItor)
						{
							itor = nextItor;
							break;
						}
						p = (*itor)->get_parent();
						COGS_ASSERT(p != get_bridge(*m_parentHwndPane)); // Shouldn't get here.  We should have found this obj.
						itor = p->get_sibling_iterator();
					}
				}
			}

			if (!lastFoundHwnd) // if none found, we were the last z-order HWND in this parent HWND
			{
				if (!!m_parentHwndPane->m_lastChildHwnd) // If any were present
				{
					m_prevSiblingHwnd = m_parentHwndPane->m_lastChildHwnd;
					m_prevSiblingHwnd->m_nextSiblingHwnd = this_rcref;
				}
				else // If none were present
				{
					m_parentHwndPane->m_firstChildHwnd = this_rcref;
				}
				m_parentHwndPane->m_lastChildHwnd = this_rcref;
			}
			else // We were not the last in z-order.  We fit immediately after lastFoundHwnd
			{
				m_prevSiblingHwnd = lastFoundHwnd;
				m_nextSiblingHwnd = lastFoundHwnd->m_nextSiblingHwnd;
				if (!!m_nextSiblingHwnd)
					m_nextSiblingHwnd->m_prevSiblingHwnd = this_rcref;
				else
					m_parentHwndPane->m_lastChildHwnd = this_rcref;
				lastFoundHwnd->m_nextSiblingHwnd = this_rcref;
			}
			break;
		}

		SIZE minimumWindowSIZE;
		m_hwnd = rcnew(hwnd)(
			!m_parentHwndPane ? 0 : m_parentHwndPane->get_hwnd(),
			!m_nextSiblingHwnd ? 0 : m_nextSiblingHwnd->get_hwnd(),
			this_rcref,
			m_windowClassName,
			composite_string(),
			m_style,
			m_extendedStyle,
			m_uiSubsystem,
			&minimumWindowSIZE);

		HDC hDC = GetDC(get_HWND());
		get_device_context().set_HDC(hDC);

		set_compositing_behavior(gui::pane::compositing_behavior::buffer_self_and_children);
		set_externally_drawn(*m_deviceContext.template static_cast_to<gfx::canvas>());

		if (!!m_parentHwndPane)
			get_device_context().set_dpi(m_parentHwndPane->get_device_context().get_dpi());
		else
		{
			HMONITOR monitor = MonitorFromWindow(get_HWND(), MONITOR_DEFAULTTONEAREST);
			UINT dpiX;
			UINT dpiY;
			GetDpiForMonitor(monitor, MONITOR_DPI_TYPE::MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

			// A bunch of APIs only accept one value for dpiY, so let's use height-wise DPI
			get_device_context().set_dpi(dpiY);
			m_minimumWindowSize = get_device_context().make_size(minimumWindowSIZE);
		}
	}

	gfx::size get_minimum_window_size() const
	{
		return m_minimumWindowSize;
	}

	rcref<hwnd_pane> get_top_hwnd_pane()
	{
		rcptr<hwnd_pane> p = this_rcptr;
		while (true)
		{
			rcptr<hwnd_pane> p2 = p->m_parentHwndPane;
			if (!p2)
				break;
			p = std::move(p2);
		}
		return p.dereference();
	}

	rcref<hwnd_pane> get_top_hwnd_pane(POINT& offset)
	{
		rcptr<hwnd_pane> p = this_rcptr;
		while (true)
		{
			rcptr<hwnd_pane> p2 = p->m_parentHwndPane;
			if (!p2)
				break;
			POINT& pt = p->m_boundsInParentHwnd.pt;
			offset.x += pt.x;
			offset.x += pt.y;
			p = std::move(p2);
		}
		return p.dereference();
	}

	virtual gfx::cell::propose_size_result propose_size(
		const gfx::size& sz,
		const gfx::range& r = gfx::range::make_unbounded(),
		const std::optional<gfx::dimension>& resizeDimension = std::nullopt,
		gfx::cell::sizing_mask sizingMask = gfx::cell::all_sizing_types) const
	{
		// Round up to nearest whole pixel
		return bridgeable_pane::propose_size(sz, r, resizeDimension, sizingMask).ceil();
	}

	virtual void reshape(const gfx::bounds& b, const gfx::point& oldOrigin = gfx::point(0, 0))
	{
		if (!m_parentHwndPane)
			m_boundsInParentHwnd = get_device_context().make_BOUNDS(b);
		else // if not a window
		{
			gfx::bounds boundsInParentHwnd = b;
			rcptr<gui::pane> p = get_bridge()->get_parent(); // find new coords in parent hwnd
			for (;;)
			{
				if (!p || (p == m_parentHwndPane->get_bridge()))
					break;
				boundsInParentHwnd += p->get_position();
				p = p->get_parent();
			}
			m_boundsInParentHwnd = get_device_context().make_BOUNDS(boundsInParentHwnd);
			MoveWindow(get_HWND(), m_boundsInParentHwnd.pt.x, m_boundsInParentHwnd.pt.y, m_boundsInParentHwnd.sz.cx, m_boundsInParentHwnd.sz.cy, TRUE);
		}
		bridgeable_pane::reshape(b, oldOrigin);
	}

	void hiding()
	{
		m_isVisible = false;
		ShowWindowAsync(get_HWND(), SW_HIDE);
		bridgeable_pane::hiding();
	}

	void showing()
	{
		bridgeable_pane::showing();
		ShowWindowAsync(get_HWND(), SW_SHOW);
		m_isVisible = true;
	}

	virtual void focusing(int direction)
	{
		if (!m_focusing)
			SetFocus(get_HWND());
		bridgeable_pane::focusing(direction);
	}

	void add_redraw_rect(const gfx::bounds& b)
	{
		RECT r = get_device_context().make_invalid_RECT(b);
		if (m_drawMode == hwnd_draw_mode::user)
		{
			HRGN rectRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
			if (m_redrawRgn == NULL)
				m_redrawRgn = rectRgn;
			else
			{
				CombineRgn(m_redrawRgn, m_redrawRgn, rectRgn, RGN_OR);
				DeleteObject(rectRgn);
			}
		}
		InvalidateRect(get_HWND(), &r, FALSE);
	}

	virtual void invalidating(const gfx::bounds& b)
	{
		gfx::bounds b2 = b & get_size();
		if (!!b2)
		{
			add_redraw_rect(b2);
			// Invalidate overlapping non-opaque HWNDs above this one
			// Start with descendants, without descending through any opaque
			// Visit on the way down, before
			rcptr<hwnd_pane> curHwnd = this;
			for (;;)
			{
				if (!!curHwnd->m_firstChildHwnd)
				{
					curHwnd = curHwnd->m_firstChildHwnd;
					b2.get_position() += -curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt); // Keep in coords of current curHwnd
				}
				else
				{
					for (;;)
					{
						if (!!curHwnd->m_nextSiblingHwnd)
						{
							b2.get_position() += curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt);
							curHwnd = curHwnd->m_nextSiblingHwnd;
							b2.get_position() += -curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt);
							break;
						}
						if (!curHwnd->m_parentHwndPane)
							return;
						b2.get_position() += curHwnd->get_device_context().make_point(curHwnd->m_boundsInParentHwnd.pt);
						curHwnd = curHwnd->m_parentHwndPane;
						curHwnd->add_redraw_rect(b2);
					}
				}
				curHwnd->add_redraw_rect(b2);
			}
		}
	}

	virtual LRESULT render_native_control(HDC dc)
	{
		return call_default_window_proc(WM_PAINT, (WPARAM)dc, (LPARAM)0);
	}

	ui::modifier_keys_state get_modifier_keys()
	{
		ui::modifier_keys_state result;

		BYTE m_keyStateBuffer[256] = {};
		GetKeyboardState(m_keyStateBuffer);

		result.set_key(ui::physical_modifier_key::left_shift_key, (m_keyStateBuffer[VK_LSHIFT] & 0x80) != 0);
		result.set_key(ui::physical_modifier_key::right_shift_key, (m_keyStateBuffer[VK_RSHIFT] & 0x80) != 0);

		result.set_key(ui::physical_modifier_key::left_control_key, (m_keyStateBuffer[VK_LCONTROL] & 0x80) != 0);
		result.set_key(ui::physical_modifier_key::right_control_key, (m_keyStateBuffer[VK_RCONTROL] & 0x80) != 0);

		result.set_key(ui::physical_modifier_key::left_alt_key, (m_keyStateBuffer[VK_LMENU] & 0x80) != 0);
		result.set_key(ui::physical_modifier_key::right_alt_key, (m_keyStateBuffer[VK_RMENU] & 0x80) != 0);

		result.set_key(ui::physical_modifier_key::left_command_key, (m_keyStateBuffer[VK_LWIN] & 0x80) != 0);
		result.set_key(ui::physical_modifier_key::right_command_key, (m_keyStateBuffer[VK_RWIN] & 0x80) != 0);

		result.set_key(ui::toggleable_modifier_key::caps_lock_key, (m_keyStateBuffer[VK_CAPITAL] & 0x01) != 0);
		result.set_key(ui::toggleable_modifier_key::num_lock_key, (m_keyStateBuffer[VK_NUMLOCK] & 0x01) != 0);

		if (result != m_lastModifierKeysState)
		{
			rcptr<gui::pane> p = get_bridge();
			if (!!p)
				pane_orchestrator::modifier_keys_change(*p, result);
			m_lastModifierKeysState = result;
		}

		return result;
	}

	virtual void dpi_changing(double oldDpi, double newDpi)
	{
		get_device_context().set_dpi(newDpi);
		if (!!m_cachedBackgroundImage)
			m_cachedBackgroundImage->set_dpi(newDpi);

		weak_rcptr_list<gdi::bitmap>::iterator offscreenBuffersItor = m_offscreenBuffers.get_first();
		while (!!offscreenBuffersItor)
		{
			rcptr<gdi::bitmap> pic = offscreenBuffersItor.get();
			if (!!pic)
				pic->set_dpi(newDpi);
			++offscreenBuffersItor;
		}
		offscreenBuffersItor.release();

		rcptr<gui::pane> bridgePane = get_bridge();
		invalidate(get_size());

		container_dlist<rcref<gui::pane> >::iterator itor = bridgePane->get_children().get_first();
		if (!!itor)
		{
			for (;;)
			{
				rcptr<gui::pane> p = (*itor);
				if (!p->is_externally_drawn())
				{
					pane_orchestrator::invalidate(*p, p->get_size());
					const container_dlist<rcref<gui::pane> >& children = p->get_children();
					if (!!children)
					{
						itor = children.get_first();
						continue;
					}
				}

				for (;;)
				{
					++itor;
					if (!!itor)
						break; // continue outer loop

					p = p->get_parent(); // go up
					if (p == bridgePane) // Got back to self
					{
						itor.release();
						break;
					}

					itor = p->get_sibling_iterator();
					// continue;
				}

				if (!itor)
					break;
				//continue;
			}
		}

		rcptr<hwnd_pane> currentChild = m_firstChildHwnd;
		while (!!currentChild)
		{
			currentChild->dpi_changing(oldDpi, newDpi);
			currentChild = currentChild->m_nextSiblingHwnd;
		}
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		rcptr<gui::pane> owner = get_bridge();
		if (!!owner)
		{
			switch (msg) // messages common to all hwnd views
			{
			case WM_WINDOWPOSCHANGING:
				{
					WINDOWPOS* winPos = (WINDOWPOS*)lParam;
					// Since we are keeping our own backing buffer,
					// tell Windows to leverage us to blit when the window is moved.
					winPos->flags |= SWP_NOCOPYBITS;
				}
				break;
			case WM_SETFOCUS:
				{
					m_focusing = true;
					pane_orchestrator::focus(*owner);
					m_focusing = false;
				}
				break;
			case WM_KILLFOCUS:
				{
				}
				break;
			case WM_CHAR:
				{
					rcref<hwnd_pane> p = get_top_hwnd_pane();
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					character_type(*(p->get_bridge()), (wchar_t)wParam, modifiers);
					return 0;
				}
			case WM_KEYDOWN:
				{
					wchar_t c = (wchar_t)MapVirtualKey((UINT)wParam, MAPVK_VK_TO_CHAR);
					if (!c)
						c = translate_special_key((wchar_t)wParam);
					rcref<hwnd_pane> p = get_top_hwnd_pane();
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					key_press(*(p->get_bridge()), c, modifiers);
					return 0;
				}
			case WM_KEYUP:
				{
					wchar_t c = (wchar_t)MapVirtualKey((UINT)wParam, MAPVK_VK_TO_CHAR);
					if (!c)
						c = translate_special_key((wchar_t)wParam);
					rcref<hwnd_pane> p = get_top_hwnd_pane();
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					key_release(*(p->get_bridge()), c, modifiers);
					return 0;
				}
		//	case WM_SYSKEYDOWN:
		//	case WM_SYSKEYUP:
		//		{
		//			// TBD: convert these to key_down/key_up
		//			TranslateMessage(&(m_uiSubsystem->get_msg()));
		//			return 0;
		//		}
		//		break;
			case WM_LBUTTONDBLCLK:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_double_click(*(p->get_bridge()), mouse_button::left, pt2, modifiers);
					return 0;
				}
			case WM_LBUTTONDOWN:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_press(*(p->get_bridge()), mouse_button::left, pt2, modifiers);
					return 0;
				}
			case WM_LBUTTONUP:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_release(*(p->get_bridge()), mouse_button::left, pt2, modifiers);
					return 0;
				}
			case WM_MBUTTONDBLCLK:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_double_click(*(p->get_bridge()), mouse_button::middle, pt2, modifiers);
					return 0;
				}
			case WM_MBUTTONDOWN:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_press(*(p->get_bridge()), mouse_button::middle, pt2, modifiers);
					return 0;
				}
			case WM_MBUTTONUP:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_release(*(p->get_bridge()), mouse_button::middle, pt2, modifiers);
					return 0;
				}
			case WM_RBUTTONDBLCLK:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					if (button_double_click(*(p->get_bridge()), mouse_button::right, pt2, modifiers))
						return 0;
				}
				break;
			case WM_RBUTTONDOWN:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_press(*(p->get_bridge()), mouse_button::right, pt2, modifiers);
					return 0;
				}
			case WM_RBUTTONUP:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					rcref<hwnd_pane> p = get_top_hwnd_pane(pt);
					gfx::point pt2 = get_device_context().make_point(pt);
					ui::modifier_keys_state modifiers = p->get_modifier_keys();
					button_release(*(p->get_bridge()), mouse_button::right, pt2, modifiers);
					return 0;
				}
			case WM_COMMAND:
				{
					if (lParam)
					{
						switch (wParam)
						{
						case BN_CLICKED: // Win32 passes button clicks to the parent window.  :/
							{
								HWND childHWND = (HWND)lParam;
#if COGS_USE_ATL_THUNK
								return SendMessage(childHWND, msg, wParam, 0);
#else
								auto itor = hwnd::get_hwnd_map().find(childHWND);
								if (!!itor)
								{
									hwnd* childPtr = itor->value;
									rcptr<hwnd_pane> child = childPtr->get_owner();
									return child->process_message(msg, wParam, 0);
								}
#endif
							}
							break;
						default:
							break;
						}
					}
				}
				break;
			case WM_VSCROLL: // Windows sends these to the parent HWND.  Pass it to child, where we intercept it.
			case WM_HSCROLL:
			case WM_CTLCOLORSTATIC:
			case WM_CTLCOLOREDIT:
			case WM_CTLCOLORSCROLLBAR:
			case WM_CTLCOLORLISTBOX:
			case WM_CTLCOLORBTN:
				{
					if (lParam)
					{
						HWND childHWND = (HWND)lParam;
#if COGS_USE_ATL_THUNK
						return SendMessage(childHWND, msg, wParam, 0);
#else
						auto itor = hwnd::get_hwnd_map().find(childHWND);
						if (!!itor)
						{
							hwnd* childPtr = itor->value;
							rcptr<hwnd_pane> child = childPtr->get_owner();
							return child->process_message(msg, wParam, 0);
						}
#endif
					}
				}
				break;
			case WM_NCPAINT:
				break;
			case WM_ERASEBKGND:
				return TRUE; // prevent background erase
			case WM_PAINT:
				{
					LRESULT lResult = 0;
					if (!owner->is_visible())
						return lResult;
					if (m_drawMode == hwnd_draw_mode::system_direct)
						break; // use default
					PAINTSTRUCT ps;
					HDC savedDC = get_HDC(); // in case owned.
					gfx::size sz = owner->get_size();
					allocate_temporary_background();
					if (m_drawMode == hwnd_draw_mode::system_offscreen) // Handle as a Win32 Control
					{
						COGS_ASSERT(!!m_cachedBackgroundImage);
						HRGN updateRgn = CreateRectRgn(0, 0, 0, 0);
						GetUpdateRgn(get_HWND(), updateRgn, FALSE);
						HDC offscreenDC = m_cachedBackgroundImage->get_HDC();
						SelectClipRgn(offscreenDC, updateRgn); // selecting into offscreen buffer
						paint_temporary_background();
						lResult = render_native_control(offscreenDC);
						get_HDC() = BeginPaint(get_HWND(), &ps);
						get_device_context().draw_bitmap(*m_cachedBackgroundImage, sz, sz, false);
						SelectClipRgn(offscreenDC, NULL); // unclip the offscreen buffer
						DeleteObject(updateRgn);
						EndPaint(get_HWND(), &ps);
					}
					else
					{
						// offscreen buffer is fetched before getting the redrawRgn, since it will call invalidate() on first paint.
						rcptr<gdi::bitmap> offScreenBuffer = get_offscreen_buffer().template static_cast_to<gdi::bitmap>();
						COGS_ASSERT(!!offScreenBuffer);
						if (is_drawing_needed())
						{
							HDC offscreenDC = offScreenBuffer->get_HDC();
							get_HDC() = offscreenDC;
							HRGN updateRgn = m_redrawRgn;
							m_redrawRgn = NULL;
							SelectClipRgn(offscreenDC, updateRgn); // selecting into offscreen buffer
							draw();
							SelectClipRgn(offscreenDC, NULL); // unclip the offscreen buffer
							DeleteObject(updateRgn);
						}
						get_HDC() = BeginPaint(get_HWND(), &ps);
						paint_temporary_background(); // Won't actually paint if background is opaque
						if (!m_cachedBackgroundImage)
						{
							if (!offScreenBuffer->is_opaque())
							{
								// Fill with the default window background color
								color c = get_default_background_color();// color::constant::white;
								get_device_context().fill(sz, c);
							}
							get_device_context().draw_bitmap(*offScreenBuffer, sz, sz, true);
						}
						else
						{
							m_cachedBackgroundImage->draw_bitmap(*offScreenBuffer, sz, sz, true);
							get_device_context().draw_bitmap(*m_cachedBackgroundImage, sz, sz, false);
						}
						EndPaint(get_HWND(), &ps);
					}
					get_HDC() = savedDC;
					return lResult;
				}
			default:
				break;
			}
		}
		return call_default_window_proc(msg, wParam, lParam);
	}
};


#if COGS_USE_ATL_THUNK

inline LRESULT CALLBACK hwnd::AtlThunkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	hwnd* hwndPtr = (hwnd*)hWnd;

	if (hwndPtr->m_hWnd == NULL) // Actually in the process of being deleted
		return hwndPtr->call_default_window_proc(msg, wParam, lParam);

	LRESULT result;
	hwndPtr->self_acquire();

	// Process our own queue here to ensure it has higher priority even above
	// messages sent directly to our WndProc
	hwndPtr->m_uiSubsystem->run_high_priority_tasks();

	rcptr<hwnd_pane> owner = hwndPtr->m_owner;
	if (!!owner)
		result = owner->process_message(msg, wParam, lParam);
	else
		result = hwndPtr->call_default_window_proc(msg, wParam, lParam);

	hwndPtr->self_release();
	return result;
}

#else

inline LRESULT CALLBACK hwnd::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	hwnd* hwndPtr = (hwnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (hwndPtr->m_hWnd == NULL) // Actually in the process of being deleted
		return hwndPtr->call_default_window_proc(msg, wParam, lParam);

	LRESULT result;
	hwndPtr->self_acquire();

	// Process our own queue here to ensure it has higher priority even above
	// messages sent directly to our WndProc
	hwndPtr->m_uiSubsystem->run_high_priority_tasks();

	rcptr<hwnd_pane> owner = hwndPtr->m_owner;
	if (!!owner)
		result = owner->process_message(msg, wParam, lParam);
	else
		result = hwndPtr->call_default_window_proc(msg, wParam, lParam);

	hwndPtr->self_release();
	return result;
}


inline LRESULT CALLBACK hwnd::UnownedClassWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto itor = get_hwnd_map().find(hWnd);
	if (!itor)
		return 0;

	hwnd* hwndPtr = itor->value;
	if (hwndPtr->m_hWnd == NULL) // Actually in the process of being deleted
		return hwndPtr->call_default_window_proc(msg, wParam, lParam);

	LRESULT result;
	hwndPtr->self_acquire();

	// Process our own queue here to ensure it has higher priority even above
	// messages sent directly to our WndProc
	hwndPtr->m_uiSubsystem->run_high_priority_tasks();

	rcptr<hwnd_pane> owner = hwndPtr->m_owner;
	if (!!owner)
		result = owner->process_message(msg, wParam, lParam);
	else
		result = hwndPtr->call_default_window_proc(msg, wParam, lParam);

	hwndPtr->self_release();
	return result;
}

#endif


inline rcref<gui::bridgeable_pane> hwnd::subsystem::create_native_pane() volatile
{
	class native_pane : public hwnd_pane
	{
	public:
		explicit native_pane(const rcref<volatile hwnd::subsystem>& subSystem)
			: hwnd_pane(composite_string(), 0, 0, subSystem, hwnd_draw_mode::user)
		{ }

		virtual void installing()
		{
			install_HWND();
			hwnd_pane::installing();
		}
	};

	rcref<hwnd_pane> p = rcnew(native_pane)(this_rcref);
	return p;
}


}
}


#include "cogs/os/gui/GDI/button.hpp"
#include "cogs/os/gui/GDI/check_box.hpp"
#include "cogs/os/gui/GDI/text_editor.hpp"
#include "cogs/os/gui/GDI/scroll_bar.hpp"
#include "cogs/os/gui/GDI/window.hpp"


#endif
