//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifdef COGS_COMPILE_SOURCE

#include <shellscalingapi.h>

#include "cogs/env.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/set.hpp"
#include "cogs/function.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/os/gui/GDI/window.hpp"
#include "cogs/os/gui/GDI/button.hpp"
#include "cogs/os/gui/GDI/check_box.hpp"
#include "cogs/os/gui/GDI/text_editor.hpp"
#include "cogs/os/gui/GDI/scroll_bar.hpp"
#include "cogs/os/gui/GDI/window.hpp"
#include "cogs/sync/quit_dispatcher.hpp"


namespace cogs {

void run_cleanup();

static placement<set<composite_string, case_insensitive_comparator<composite_string> > > s_fontList;
static placement<gfx::font> s_defaultFont;
static ULONG_PTR s_gdiplusToken;

static int CALLBACK EnumFontFamExProc(const LOGFONT *lpelfe, const TEXTMETRIC *lpntme, DWORD FontType, LPARAM lParam)
{
	auto fontList = (set<composite_string, case_insensitive_comparator<composite_string> >*)lParam;
	fontList->try_insert(lpelfe->lfFaceName);
	return 1;
}

bool initialize()
{
	int cpuInfo[4];
	__cpuid(cpuInfo, 1);

#ifdef _M_X64
	if (!(cpuInfo[2] & (1 << 13)))	// if 64-bit and no support for CMPXCHG16B instruction
	{
		MessageBox(NULL, L"Unsupported 64-bit processor detected.  Please use 32-bit application instead.", L"Instruction not supported: CMPXCHG16B", MB_OK);
		return false;
	}
#else
	if (!(cpuInfo[3] & (1 << 8)))	// if 32-bit and no support for CMPXCHG8B instruction
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not supported: CMPXCHG8B", MB_OK);
		return false;
	}
	if (!(cpuInfo[2] & (1 << 23)))
	{
		MessageBox(NULL, L"Processor not supported.", L"Instruction not supported: POPCNT", MB_OK);
		return false;
	}
#endif

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;

	Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);
	BOOL b = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	DWORD err = GetLastError();

	// XP requires an explicit loading of controls.  Load all the ones we have classes for.
	INITCOMMONCONTROLSEX initCtrls;
	initCtrls.dwSize = sizeof(initCtrls);
	initCtrls.dwICC = ICC_STANDARD_CLASSES;
	b = InitCommonControlsEx(&initCtrls);

	// Load MSFTEDIT control
	LoadLibrary(TEXT("Msftedit.dll"));

	// Load fonts.  (Fonts added or removed will not be observed until relaunch)
	LOGFONT logFont = {};
	logFont.lfCharSet = DEFAULT_CHARSET;
	HDC hdc = GetDC(NULL);

	set<composite_string, case_insensitive_comparator<composite_string> >& fontList = s_fontList.get();
	new (&fontList) set<composite_string, case_insensitive_comparator<composite_string> >();
	int i = EnumFontFamiliesEx(hdc, &logFont, &EnumFontFamExProc, (LPARAM)&fontList, 0);
	ReleaseDC(NULL, hdc);

	// Load default font
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);// -sizeof(ncm.iPaddedBorderWidth);?
	b = SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, gfx::canvas::dip_dpi);

	gfx::font& defaultFont = s_defaultFont.get();
	new (&defaultFont) gfx::font();

	defaultFont.get_font_names().clear();
	defaultFont.append_font_name(ncm.lfMessageFont.lfFaceName);
	defaultFont.set_italic(ncm.lfMessageFont.lfItalic != 0);
	defaultFont.set_bold(ncm.lfMessageFont.lfWeight >= 700);
	defaultFont.set_underlined(ncm.lfMessageFont.lfUnderline != 0);
	defaultFont.set_strike_out(ncm.lfMessageFont.lfStrikeOut != 0);
	defaultFont.set_point_size(-ncm.lfMessageFont.lfHeight);

	return true;
}

void terminate()
{
	cogs::run_cleanup();

	s_defaultFont.destruct();
	s_fontList.destruct();

	cogs::default_allocator::shutdown();

	Gdiplus::GdiplusShutdown(s_gdiplusToken);
}

}


#ifdef USE_COGS_MAIN

int main();

namespace cogs {

	int main();

}

int main()
{
	int result = 0;
	
	if (cogs::initialize())
	{
		{
			// keeps the main subsystem in scope until quit
			//cogs::rcref<volatile cogs::gui::os::hwnd::subsystem> mainUiSubsystem = cogs::gui::os::hwnd::subsystem::get_default();
			result = cogs::main();
			{
				cogs::rcptr<const cogs::single_fire_event> quitEvent = cogs::quit_dispatcher::get()->get_event();
				if (!!quitEvent)
					quitEvent->wait();
			}
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

namespace cogs {
namespace gfx {

const font& font::get_default()
{
	return s_defaultFont.get();
}

namespace os {
namespace gdi {

const composite_string& device_context::font::resolve(const vector<composite_string>& fontNames)
{
	size_t numFontNames = fontNames.get_length();
	composite_string fontName;
	for (size_t i = 0; i < numFontNames; i++)
	{
		auto itor = s_fontList.get().find(fontNames[i]);
		if (!!itor)
			return *itor;	// Use case from font list, just in case.  ha
	}

	return gfx::font::get_default().get_font_names()[0];
}

}
}
}

namespace gui {


rcptr<volatile subsystem> subsystem::get_default()
{
	return os::hwnd::subsystem::get_default();
}

//rcptr<ui::console> subsystem::get_default_console() volatile
//{
//	return rcptr<console>();	// TBD
//}
//
//rcptr<ui::console> subsystem::create_console() volatile
//{
//	return rcptr<console>();	// TBD
//}
//
//rcref<task<void> > subsystem::message(const composite_string& s) volatile
//{
//	// TBD
//	return get_immediate_task();
//}


namespace os {


placement<rcptr<hwnd::window_class> > hwnd::window_class::s_defaultWindowClass;
placement<rcptr<volatile hwnd::subsystem> > hwnd::subsystem::s_defaultSubsystem;

rcref<task<void> > hwnd::subsystem::message(const composite_string& s) volatile
{
	MessageBox(NULL, s.composite().cstr(), L"", MB_OK);
	return get_immediate_task();
}

rcref<task<void> > hwnd::subsystem::open_full_screen(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& rshpr,
	const function<bool()>& closeDelegate) volatile
{
	// TBD
	return get_immediate_task();
}

rcref<gui::window> hwnd::subsystem::open_window(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& rshpr,
	const function<bool()>& closeDelegate) volatile
{
	rcref<gui::window> w = rcnew(gui::window, title, closeDelegate);
	w->nest(p, rshpr);
	install(*w, rcnew(bypass_constructor_permission<hwnd::subsystem>));	// Give each window it's own subsystem instance, so it's own UI thread.
	return w;
}

rcref<bridgeable_pane> hwnd::subsystem::create_native_pane() volatile
{
	class native_pane : public hwnd_pane
	{
	public:
		native_pane(const rcref<volatile hwnd::subsystem>& subSystem)
			: hwnd_pane(composite_string(), 0, 0, subSystem, user_drawn)
		{ }

		virtual void installing()
		{
			install_HWND();
			hwnd_pane::installing();
		}
	};

	rcref<hwnd_pane> p = rcnew(native_pane, this_rcref);
	return p;
}

std::pair<rcref<bridgeable_pane>, rcref<button_interface> > hwnd::subsystem::create_button() volatile
{
	rcref<button> b = rcnew(button, this_rcref);
	return std::make_pair(b, b);
}

std::pair<rcref<bridgeable_pane>, rcref<check_box_interface> > hwnd::subsystem::create_check_box() volatile
{
	rcref<check_box> cb = rcnew(check_box, this_rcref);
	return std::make_pair(cb, cb);
}

std::pair<rcref<bridgeable_pane>, rcref<text_editor_interface> > hwnd::subsystem::create_text_editor() volatile
{
	rcref<text_editor> te = rcnew(text_editor, this_rcref);
	return std::make_pair(te, te);
}

std::pair<rcref<bridgeable_pane>, rcref<scroll_bar_interface> > hwnd::subsystem::create_scroll_bar() volatile
{
	rcref<scroll_bar> sb = rcnew(scroll_bar, this_rcref);
	return std::make_pair(sb, sb);
}

std::pair<rcref<bridgeable_pane>, rcref<window_interface> > hwnd::subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window, this_rcref);
	return std::make_pair(w, w);
}


LRESULT hwnd_pane::process_message(UINT msg, WPARAM wParam, LPARAM lParam)
{
	rcptr<pane> owner = get_bridge();// m_owner;
	if (!!owner)
	{
		switch (msg)		// messages common to all hwnd views
		{
		case WM_MOUSEMOVE:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				cursor_hover(*owner, pt2);
				if (!m_mouseTracking)
				{
					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = get_HWND();
		
					if (TrackMouseEvent(&tme))
						m_mouseTracking = true;
				}
			}
			break;
		case WM_MOUSELEAVE:
			{
				m_mouseTracking = false;
				cursor_leave(*owner);
			}
			break;
		case WM_SETFOCUS:
			{
				m_focusing = true;
				owner->focus();
				m_focusing = false;
			}
			break;
		case WM_KILLFOCUS:
			{
			}
			break;
		case WM_CHAR:
			{
				character_type(*owner, (wchar_t)wParam);
			}
		case WM_KEYDOWN:
			{
				composite_string::char_t c = (string::char_t)MapVirtualKey((UINT)wParam, MAPVK_VK_TO_CHAR);
				bool doTranslate = true;
				if (!c)
					c = translate_special_key((wchar_t)wParam);
				key_press(*owner, c);
			}
			break;
		case WM_KEYUP:
			{
				string::char_t c = (string::char_t)MapVirtualKey((UINT)wParam, MAPVK_VK_TO_CHAR);
				if (!c)
					c = translate_special_key((wchar_t)wParam);
				key_release(*owner, c);
			}
			break;			
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
				point pt2 = get_device_context().make_point(pt);
				button_double_click(*owner, left_mouse_button, pt2);
			}
			break;

		case WM_LBUTTONDOWN:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_press(*owner, left_mouse_button, pt2);
			}
			break;

		case WM_LBUTTONUP:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_release(*owner, left_mouse_button, pt2);
			}
			break;

		case WM_MBUTTONDBLCLK:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_double_click(*owner, middle_mouse_button, pt2);
			}
			break;

		case WM_MBUTTONDOWN:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_press(*owner, middle_mouse_button, pt2);
			}
			break;

		case WM_MBUTTONUP:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_release(*owner, middle_mouse_button, pt2);
			}
			break;

		case WM_RBUTTONDBLCLK:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_double_click(*owner, right_mouse_button, pt2);
			}
			break;

		case WM_RBUTTONDOWN:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_press(*owner, right_mouse_button, pt2);
			}
			break;

		case WM_RBUTTONUP:
			{
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				point pt2 = get_device_context().make_point(pt);
				button_release(*owner, right_mouse_button, pt2);
			}
			break;

		case WM_COMMAND:
			{
				if (lParam)
				{
					switch (wParam)
					{
					case BN_CLICKED:	// Win32 passes button clicks to the parent window.  :/
						{
							HWND childHWND = (HWND)lParam;
							auto itor = hwnd::get_hwnd_map().find(childHWND);
							if (!!itor)
							{
								hwnd* childPtr = *itor;
								rcptr<hwnd_pane> child = childPtr->get_owner();
								rcptr<os::button> btn = child.dynamic_cast_to<os::button>();
								if (!!btn)
									btn->action();
								return 0;
							}
						}
						break;
					default:
						break;
					}
				}
			}
			break;
		case WM_VSCROLL:	// Windows sends these to the parent wind.  Pass it to child.
		case WM_HSCROLL:
		case WM_CTLCOLORSTATIC:	// Windows sends these to the parent wind.  Pass it to child.
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORBTN:
			{
				if (lParam)
				{
					HWND childHWND = (HWND)lParam;
					auto itor = hwnd::get_hwnd_map().find(childHWND);
					if (!!itor)
					{
						hwnd* childPtr = *itor;
						rcptr<hwnd_pane> child = childPtr->get_owner();
						return child->process_message(msg, wParam, 0);
					}
				}
			}
			break;
		//case WM_NCCREATE:
		//	//EnableNonClientDpiScaling(get_HWND());
		//	break;
		case WM_NCPAINT:
			break;
		case WM_ERASEBKGND:
			return TRUE;	// prevent background erase
		case WM_PAINT:
			{
				LRESULT lResult = 0;
				if (!owner->is_visible())
					return lResult;
				if (m_drawMode == system_drawn_direct)
					break;	// use default
				PAINTSTRUCT ps;
				HDC savedDC = get_HDC();	// in case owned.
				size sz = owner->get_size();
				allocate_temporary_background();
				if (m_drawMode == system_drawn_offscreen)	// Handle as a Win32 Control
				{
					COGS_ASSERT(!!m_cachedBackgroundImage);	
					HRGN updateRgn = CreateRectRgn(0, 0, 0, 0);
					GetUpdateRgn(get_HWND(), updateRgn, FALSE);
					HDC offscreenDC = m_cachedBackgroundImage->get_HDC();
					SelectClipRgn(offscreenDC, updateRgn);	// selecting into offscreen buffer
					paint_temporary_background();
					lResult = render_native_control(offscreenDC);
					get_HDC() = BeginPaint(get_HWND(), &ps);
					get_device_context().composite_pixel_image(*m_cachedBackgroundImage, sz, point(0, 0), false);
					SelectClipRgn(offscreenDC, NULL);	// unclip the offscreen buffer
					DeleteObject(updateRgn);
					EndPaint(get_HWND(), &ps);
				}
				else
				{
					// offscreen buffer is fetched before getting the redrawRgn, since it will call invalidate() on first paint.
					rcptr<gfx::os::gdi::bitmap> offScreenBuffer = get_offscreen_buffer().static_cast_to<gfx::os::gdi::bitmap>();
					COGS_ASSERT(!!offScreenBuffer);
					if (is_drawing_needed())
					{
						HDC offscreenDC = offScreenBuffer->get_HDC();
						get_HDC() = offscreenDC;
						HRGN updateRgn = m_redrawRgn;
						m_redrawRgn = NULL;
						SelectClipRgn(offscreenDC, updateRgn);	// selecting into offscreen buffer
						draw();
						SelectClipRgn(offscreenDC, NULL);	// unclip the offscreen buffer
						DeleteObject(updateRgn);
					}
					get_HDC() = BeginPaint(get_HWND(), &ps);
					paint_temporary_background();	// Won't actually paint if background is opaque
					if (!m_cachedBackgroundImage)
						get_device_context().composite_pixel_image(*offScreenBuffer, sz, point(0, 0), false);
					else
					{
						m_cachedBackgroundImage->composite_pixel_image(*offScreenBuffer, sz, point(0, 0), true);
						get_device_context().composite_pixel_image(*m_cachedBackgroundImage, sz, point(0, 0), false);
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


}
}
}

#endif
