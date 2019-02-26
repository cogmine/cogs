//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_BUTTON
#define COGS_HEADER_OS_GUI_GDI_BUTTON


#include "cogs/os.hpp"
#include <commctrl.h>

#include "cogs/gui/button.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/os/gui/GDI/window.hpp"


namespace cogs {
namespace gui {
namespace os {


class button : public hwnd_pane, public button_interface
{
public:
	rcptr<gfx::os::gdi::device_context::font>	m_cachedFont;
	size									m_defaultSize;

	button(const ptr<rc_obj_base>& desc, const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(desc, composite_string::literal(L"BUTTON"), WS_TABSTOP | BS_CENTER | BS_PUSHBUTTON | BS_TEXT, 0, uiSubsystem, system_drawn_offscreen)
	{ }

	virtual void installing()
	{
		rcptr<gui::button> btn = get_bridge().template static_cast_to<gui::button>();

		install_HWND();
		SendMessage(get_HWND(), CCM_DPISCALE, (WPARAM)TRUE, 0);

		set_text(btn->get_text());
		set_font(btn->get_font());
		set_enabled(btn->is_enabled());
		set_default(btn->is_default());

		hwnd_pane::installing();
	}

	void action()
	{
		rcptr<gui::button> btn = get_bridge().template static_cast_to<gui::button>();
		if (!!btn)
			btn->action();
	}

	virtual void set_text(const composite_string& text)
	{
		int i = Button_SetText(get_HWND(), text.composite().cstr());
	}

	virtual void set_enabled(bool isEnabled = true)
	{
		BOOL b = Button_Enable(get_HWND(), isEnabled ? TRUE : FALSE);
	}

	virtual void set_default(bool isDefault = true)
	{
		DWORD style = this->get_style();
		if (isDefault)
		{
			style |=  BS_DEFPUSHBUTTON;
			style &= ~BS_PUSHBUTTON;
		}
		else
		{
			style |=  BS_PUSHBUTTON;
			style &= ~BS_DEFPUSHBUTTON;
		}
		set_style(style);
		Button_SetStyle(get_HWND(), style, FALSE);
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = get_device_context().load_font(fnt).template static_cast_to<gfx::os::gdi::device_context::font>();
		SendMessage(get_HWND(), WM_SETFONT, (WPARAM)(m_cachedFont->get_HFONT()), MAKELPARAM(FALSE, 0));
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
			{
				HDC hDC = (HDC)wParam;
				rcptr<pane> owner = get_bridge();
				if (!!owner)
				{
					SetBkMode(hDC, TRANSPARENT);
					allocate_temporary_background();
					paint_temporary_background();
					COGS_ASSERT(!!m_cachedBackgroundImage);
					return (LRESULT)(m_cachedBackgroundImage->get_BRUSH());
				}
			}
			break;
		case WM_COMMAND:
			{
				switch (wParam)
				{
				case BN_CLICKED:
					{
						action();
						return 0;
					}
					break;
				default:
					break;
				}
			}
			break;
		case WM_LBUTTONDBLCLK:	// Let Windows handle all types of clicks
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			return call_default_window_proc(msg, wParam, lParam);
		case WM_PAINT:			// hwnd_pane's default WM_PAINT accounts for controls that accept HDC in wParam of WM_PAINT
		default:
			break;
		}

		return call_default_window_proc(msg, wParam, lParam);
	}

	virtual void calculate_range()
	{
		SIZE sz = { 0, 0 };
		BOOL b = Button_GetIdealSize(get_HWND(), &sz);
		if (sz.cx < 16)
			sz.cx = 16;
		if (sz.cy < 16)
			sz.cy = 16;

		m_defaultSize = get_device_context().make_size(sz);
	}

	virtual void dpi_changing(double oldDpi, double newDpi)
	{
		hwnd_pane::dpi_changing(oldDpi, newDpi);
		rcptr<gui::button> btn = get_bridge().template static_cast_to<gui::button>();
		set_font(btn->get_font());
	}

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		hwnd_pane::reshape(r, oldOrigin);
		invalidate(get_size());
	}

	virtual range get_range() const { return range(m_defaultSize); }
	virtual size get_default_size() const { return m_defaultSize; }

	virtual bool is_focusable() const	{ return true; }
};


inline std::pair<rcref<bridgeable_pane>, rcref<button_interface> > hwnd::subsystem::create_button() volatile
{
	rcref<button> b = rcnew(button, this_rcref);
	return std::make_pair(b, b);
}


}
}
}


#endif
