//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_SCROLL_BAR
#define COGS_HEADER_OS_GUI_GDI_SCROLL_BAR


#include "cogs/os.hpp"
#include <commctrl.h>


#include "cogs/bindable_property.hpp"
#include "cogs/gui/scroll_bar.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"
#include "cogs/os/gui/GDI/window.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace gui {
namespace os {


class scroll_bar : public hwnd_pane, public scroll_bar_interface
{
private:
	volatile transactable<scroll_bar_state>	m_state;
	volatile double						m_pos;

	delayed_construction<delegated_bindable_property<scroll_bar_state> >	m_stateProperty;
	delayed_construction<delegated_bindable_property<double> >			m_positionProperty;

	dimension	m_dimension;
	bool		m_isHiddenWhenInactive;
	bool		m_isHidden;
	range		m_currentRange;
	size		m_currentDefaultSize;

	void set_scroll_bar_state(const scroll_bar_state& newState, double newPos)
	{
		int fnBar = SB_CTL;
		for (;;)
		{
			double pos = newPos;
			if (pos + newState.m_thumbSize > newState.m_max)
			{
				pos = newState.m_max;
				pos -= newState.m_thumbSize;
			}

			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
			si.nMin = 0;
			si.nMax = (int)std::lround(newState.m_max - 1);
			si.nPage = (UINT)std::lround(newState.m_thumbSize);
			si.nPos = (int)std::lround(pos);

			if (newState.m_thumbSize >= newState.m_max)
			{
				if (!!m_isHiddenWhenInactive)
				{
					if (!m_isHidden)
					{
						m_isHidden = true;
						hiding();
					}
					break;
				}
			
				si.nMax = 0;
				si.nPage = 1;
				si.nPos = 0;
				si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_RANGE;

				EnableWindow(get_HWND(), FALSE);
			}
			else
			{
				EnableWindow(get_HWND(), TRUE);

				UINT wArrows = ESB_ENABLE_BOTH;

				if (!pos)	// No up or left arrow		// We already know pos != max
					wArrows = (m_dimension == dimension::horizontal) ? ESB_DISABLE_LEFT : ESB_DISABLE_UP;
				else
				{
					double maxPos = newState.m_max;
					maxPos -= newState.m_thumbSize;
					if (pos == maxPos)	// No down or right arrow
						wArrows = (m_dimension == dimension::horizontal) ?  ESB_DISABLE_RIGHT : ESB_DISABLE_DOWN;
				}

				SetScrollInfo(get_HWND(), fnBar, &si, TRUE);

				EnableScrollBar(get_HWND(), fnBar, wArrows);
			}

			if (!!m_isHidden)
			{
				m_isHidden = false;
				showing();
			}
			break;
		}
	}

public:
	scroll_bar(const ptr<rc_obj_base>& desc, const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(desc, string::literal(L"SCROLLBAR"), WS_TABSTOP, 0, uiSubsystem, system_drawn_offscreen),
		m_isHidden(false),
		m_pos(0)
	{
		auto stateGetter = [this]()
		{
			return *(m_state.begin_read());
		};

		auto stateSetter = [this](const scroll_bar_state& state)
		{
			scroll_bar_state newState = state;
			scroll_bar_state oldState = newState;
			m_state.swap_contents(oldState);
			if (newState != oldState)
			{
				double curPos = atomic::load(m_pos);
				set_scroll_bar_state(newState, curPos);
				m_stateProperty->changed();
			}
			m_stateProperty->set_complete();
		};

		placement_rcnew(&m_stateProperty.get(), this_desc, uiSubsystem, std::move(stateGetter), std::move(stateSetter));

		auto positionGetter = [this]()
		{
			return atomic::load(m_pos);
		};

		auto positionSetter = [this](double d)
		{
			double newPos = d;
			double oldPos = atomic::exchange(m_pos, newPos);
			if (newPos != oldPos)
			{
				set_scroll_bar_state(*(m_state.begin_read()), newPos);
				m_positionProperty->changed();
			}
			m_positionProperty->set_complete();
		};

		placement_rcnew(&m_positionProperty.get(), this_desc, uiSubsystem, std::move(positionGetter), std::move(positionSetter));
	}

	virtual void installing()
	{
		rcptr<gui::scroll_bar> sb = get_bridge().template static_cast_to<gui::scroll_bar>();

		m_dimension = sb->get_dimension();
		m_isHiddenWhenInactive = sb->is_hidden_when_inactive();
		
		if (m_dimension == dimension::horizontal)
			m_style |= SBS_HORZ | SBS_TOPALIGN;
		else
			m_style |= SBS_VERT | SBS_LEFTALIGN;

		install_HWND();
		hwnd_pane::installing();

		m_stateProperty->bind_from(sb->get_state_property());
		m_positionProperty->bind(sb->get_position_property());
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_VSCROLL:
		case WM_HSCROLL:
			{
				int fnBar = SB_CTL;
				SCROLLINFO si;
				si.cbSize = sizeof(si);
				si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;
				GetScrollInfo(get_HWND(), fnBar, &si);
				
				double pos  = si.nPos;
				double thumbSize = si.nPage;
				double max  = si.nMax + 1;
				double trackPos = si.nTrackPos;
				double maxPos = max;
				maxPos -= thumbSize;

				if (LOWORD(wParam) == SB_ENDSCROLL)
				{
					UINT wArrows = ESB_ENABLE_BOTH;
					if (!pos)	// No up or left arrow		// We already know pos != max
						wArrows = (m_dimension == dimension::horizontal) ? ESB_DISABLE_LEFT : ESB_DISABLE_UP;
					else if (pos == maxPos)	// No down or right arrow
						wArrows = (m_dimension == dimension::horizontal) ?  ESB_DISABLE_RIGHT : ESB_DISABLE_DOWN;
					EnableScrollBar(get_HWND(), fnBar, wArrows);
				}
				else
				{
					switch (LOWORD(wParam))
					{
					case SB_TOP:			// top
						pos = 0;
						break;
					case SB_BOTTOM:			// bottom
						pos = maxPos;
						break;
					case SB_LINEDOWN:		// +1
						pos++;
						if (pos > maxPos)
							pos = maxPos;
						break;
					case SB_LINEUP:			// -1
						if (pos > 0)
							pos--;
						break;
					case SB_PAGEDOWN:		// +page
						pos += thumbSize;
						if (pos > maxPos)
							pos = maxPos;
						break;
					case SB_PAGEUP:			// -page
						if (pos >= thumbSize)
							pos -= thumbSize;
						else
							pos = 0;
						break;
					case SB_THUMBPOSITION:
					case SB_THUMBTRACK:
						pos = trackPos;
						break;
					}

					double oldPos = atomic::load(m_pos);
					if (oldPos != pos)
					{
						si.fMask = SIF_POS;
						si.nPos = (int)std::lround(pos);
						SetScrollInfo(get_HWND(), fnBar, &si, TRUE);

						// sets are serialized in the UI thread.  No need to worry about synchronizing with other writes.
						atomic::store(m_pos, pos);
						m_positionProperty->changed();
					}
				}
				get_subsystem()->run_high_priority_tasks();
				return 0;
			}
			break;
		case WM_PAINT:			// Let Windows render this control
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
		default:
			break;	
		}
		return hwnd_pane::process_message(msg, wParam, lParam);
	}

	virtual void calculate_range()
	{
		m_currentRange.clear();
		int scrollBarWidth = GetSystemMetricsForDpi((m_dimension == dimension::horizontal) ? SM_CYHSCROLL : SM_CXVSCROLL, (int)get_dpi());
		double sz = get_device_context().make_size(scrollBarWidth);

		m_currentRange.get_max(!m_dimension) = sz;

		m_currentDefaultSize.set(sz, sz);
	}

	virtual range get_range() const	 { return m_currentRange; }
	virtual size get_default_size() const { return m_currentDefaultSize; }

	virtual bool is_focusable() const	{ return false; }

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		hwnd_pane::reshape(r, oldOrigin);
		invalidate(get_size());
	}
};


inline std::pair<rcref<bridgeable_pane>, rcref<scroll_bar_interface> > hwnd::subsystem::create_scroll_bar() volatile
{
	rcref<scroll_bar> sb = rcnew(scroll_bar, this_rcref);
	return std::make_pair(sb, sb);
}


}
}
}


#endif
