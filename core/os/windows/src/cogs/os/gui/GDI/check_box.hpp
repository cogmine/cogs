//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_GDI_CHECK_BOX
#define COGS_GDI_CHECK_BOX


#include "cogs/os.hpp"
#include <commctrl.h>

#include "cogs/gui/check_box.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/os/gui/GDI/window.hpp"


namespace cogs {
namespace gui {
namespace os {

#pragma warning(push)
#pragma warning (disable: 4250)

class check_box : public hwnd_pane, public check_box_interface
{
public:
	rcptr<gfx::os::gdi::device_context::font> m_cachedFont;
	size m_defaultSize;
	bool m_isChecked;

	check_box(const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(composite_string::literal(L"BUTTON"), WS_TABSTOP | BS_AUTOCHECKBOX, 0, uiSubsystem, system_drawn_direct)
	{ }

	virtual void installing()
	{
		rcptr<gui::check_box> cb = get_bridge().static_cast_to<gui::check_box>();

		install_HWND();
		SendMessage(get_HWND(), CCM_DPISCALE, (WPARAM)TRUE, 0);

		set_text(cb->get_text());
		set_enabled(cb->is_enabled());
		set_checked(cb->is_checked());
		set_font(cb->get_font());

		hwnd_pane::installing();
	}

	void action()
	{
		rcptr<gui::check_box> b = get_bridge().static_cast_to<gui::check_box>();
		if (!!b)
			b->action();
	}

	virtual void set_text(const composite_string& text)
	{
		m_caption = text;
		int i = Button_SetText(get_HWND(), m_caption.composite().cstr());
	}

	virtual void set_enabled(bool isEnabled = true)
	{
		BOOL b = Button_Enable(get_HWND(), isEnabled ? TRUE : FALSE);
	}

	virtual void set_checked(bool isChecked = true)
	{
		m_isChecked = isChecked;
		Button_SetCheck(get_HWND(), isChecked ? BST_CHECKED : BST_UNCHECKED);
	}

	virtual bool is_checked() const
	{
		return m_isChecked;
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = get_device_context().load_font(fnt).static_cast_to<gfx::os::gdi::device_context::font>();
		SendMessage(get_HWND(), WM_SETFONT, (WPARAM)(m_cachedFont->get_HFONT()), MAKELPARAM(FALSE, 0));
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case BM_SETCHECK:
			{
				bool newState = (wParam == BST_CHECKED);
				if (m_isChecked != newState)
				{
					m_isChecked = newState;
					action();
				}
			}
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
			{
				HDC hDC = (HDC)wParam;
				rcptr<gui::check_box> owner = get_bridge().static_cast_to<gui::check_box>();
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
		HBITMAP hBitMap = LoadBitmap(0, MAKEINTRESOURCE(OBM_CHECKBOXES));
		BITMAP bmp;
		m_defaultSize.set(0, 0);
		if (GetObject(hBitMap, sizeof(BITMAP), &bmp) != 0)
			m_defaultSize.set((double)bmp.bmWidth / 4, (double)bmp.bmHeight / 3);

		DeleteObject(hBitMap);
		if (!!m_caption)
		{
			size textBounds = m_cachedFont->calc_text_bounds(m_caption);
			m_defaultSize.get_width() += textBounds.get_width();
			if (m_defaultSize.get_height() < textBounds.get_height())
				m_defaultSize.get_height() = textBounds.get_height();
		}
	}

	virtual void dpi_changing(double oldDpi, double newDpi)
	{
		hwnd_pane::dpi_changing(oldDpi, newDpi);
		rcptr<gui::check_box> cb = get_bridge().static_cast_to<gui::check_box>();
		set_font(cb->get_font());
	}

	virtual range get_range() const { return range(m_defaultSize); }
	virtual size get_default_size() const { return m_defaultSize; }

	virtual bool is_focusable() const { return true; }

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		hwnd_pane::reshape(r, oldOrigin);
		invalidate(get_size());
	}
};


#pragma warning(pop)


}
}
}


#endif
