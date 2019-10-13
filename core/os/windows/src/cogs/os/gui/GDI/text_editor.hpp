//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_TEXT_EDITOR
#define COGS_HEADER_OS_GUI_GDI_TEXT_EDITOR


#include "cogs/os.hpp"
#include <commctrl.h>

#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/gui/text_editor.hpp"


namespace cogs {
namespace gui {
namespace os {


class text_editor : public hwnd_pane, public text_editor_interface
{
private:
	//color m_defaultTextColor;
	//color m_defaultBackgroundColor;
	rcptr<gfx::os::gdi::device_context::font> m_cachedFont;

public:
	text_editor(const ptr<rc_obj_base>& desc, const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(desc, composite_string::literal(MSFTEDIT_CLASS), WS_TABSTOP | ES_LEFT | ES_SAVESEL, WS_EX_TRANSPARENT, uiSubsystem,system_drawn_direct)
	{
	}

	virtual void installing()
	{
		rcptr<gui::text_editor> te = get_bridge().template static_cast_to<gui::text_editor>();

		if (te->is_multi_line())
			m_style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
		else
			m_style |= ES_AUTOHSCROLL;
		install_HWND();

		set_font(te->get_font());
		set_text(te->get_text());
		set_max_length(te->get_max_length());

		HDC hDC = GetDC(get_HWND());
		COLORREF colorRef = GetBkColor(hDC);
		//m_defaultBackgroundColor = color(GetRValue(colorRef), GetGValue(colorRef), GetBValue(colorRef));
		colorRef = GetTextColor(hDC);
		//m_defaultTextColor = color(GetRValue(colorRef), GetGValue(colorRef), GetBValue(colorRef));
		ReleaseDC(get_HWND(), hDC);

		set_text_color(color::black);

		hwnd_pane::installing();
	}

	virtual void set_text(const composite_string& text)
	{
		Edit_SetText(get_HWND(), text.composite().cstr());
	}

	virtual void set_max_length(size_t numChars)
	{
		Edit_LimitText(get_HWND(), (int)numChars);
	}

	virtual composite_string get_text() const
	{
		string s;
		for (;;)
		{
			int len = GetWindowTextLength(get_HWND()) + 2;
			s.resize(len);
			int recvLen = GetWindowText(get_HWND(), s.get_ptr(), len); // Does not NOT stall calling thread if not UI thread

			// Because we are providing space for 2 more than necessary, if we detect that this many was
			// received, it's possible there is now more data there (set by another thread).  Read again.
			if (recvLen > (len - 2))
				continue;
			break;
		}
		s.truncate_to(s.index_of(0));
		return s;
	}

	virtual void set_enabled(bool isEnabled = true)
	{
		Edit_Enable(get_HWND(), isEnabled ? TRUE : FALSE);
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = get_device_context().load_font(fnt).template static_cast_to<gfx::os::gdi::device_context::font>();
		SendMessage(get_HWND(), WM_SETFONT, (WPARAM)(m_cachedFont->get_HFONT()), MAKELPARAM(FALSE, 0));
	}

	virtual void set_text_color(const color& c)
	{
		CHARFORMAT2 cf = {};
		cf.dwMask = CFM_COLOR;
		cf.crTextColor = gfx::os::gdi::device_context::make_COLORREF(c);
		cf.cbSize = sizeof(CHARFORMAT2);
		SendMessageA(get_HWND(), EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// Strange workaround from the internet to get scroll thumb dragging to work properly with WS_EX_COMPOSITED
		if ((msg == WM_HSCROLL || msg == WM_VSCROLL)
			&& (((int)wParam & 0xFFFF) == 5))
		{
			// Change SB_THUMBTRACK to SB_THUMBPOSITION
			wParam = (WPARAM)(((int)wParam & ~0xFFFF) | 4);
		}

		// Go to default for everything
		return call_default_window_proc(msg, wParam, lParam);
	}

	virtual bool is_focusable() const { return true; }

	virtual size get_default_size() const { return size(100, 100); }

	virtual LRESULT render_native_control(HDC dc)
	{
		RECT rc;
		GetClientRect(get_HWND(), &rc);

		int savedBkMode = SetBkMode(dc, TRANSPARENT);

		rc.left -= 1;
		rc.top -= 2;

		POINTL pt;
		pt.x = 0;
		pt.y = 0;

		//Finding the size in twips
		int nLogPixelsX = ::GetDeviceCaps(dc, LOGPIXELSX);
		int nLogPixelsY = ::GetDeviceCaps(dc, LOGPIXELSY);

		rc.left = MulDiv(rc.left, 1440, nLogPixelsX);
		rc.top = MulDiv(rc.top, 1440, nLogPixelsY);
		rc.right = MulDiv(rc.right, 1440, nLogPixelsX);
		rc.bottom = MulDiv(rc.bottom, 1440, nLogPixelsY);

		FORMATRANGE fr;
		fr.hdc = dc; 
		fr.hdcTarget = dc;
		fr.rc = rc;
		fr.rcPage = rc;
		fr.chrg.cpMin = 0;
		fr.chrg.cpMax = -1;

		LRESULT result = SendMessage(get_HWND(), EM_FORMATRANGE, (WPARAM)TRUE, (LPARAM)&fr);
		SendMessage(get_HWND(), EM_FORMATRANGE, (WPARAM)FALSE, (LPARAM)NULL);

		SetBkMode(dc, savedBkMode);

		return result;
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		hwnd_pane::reshape(b, oldOrigin);
		invalidate(get_size());
	}

};


inline std::pair<rcref<bridgeable_pane>, rcref<text_editor_interface> > hwnd::subsystem::create_text_editor() volatile
{
	rcref<text_editor> te = rcnew(text_editor, this_rcref);
	return std::make_pair(te, te);
}


}
}
}


#endif
