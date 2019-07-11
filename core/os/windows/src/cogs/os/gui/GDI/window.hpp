//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_WINDOW
#define COGS_HEADER_OS_GUI_GDI_WINDOW


#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/gui/window.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/GDI/hwnd.hpp"


namespace cogs {
namespace gui {
namespace os {


class window : public hwnd_pane, public window_interface
{
private:
	volatile boolean m_isInModalSizingLoop;
	volatile ptr<void> m_lastTimerId;
	int m_sizingMode; // 0 = none, 1 = WM_SIZING with WM_SIZE pending, 2 = WM_WINDOWPOSCHANGING with WM_WINDOWPOSCHANGED pending
	POINT m_position; // Always in native pixels for the DPI.
	POINT m_pendingPosition;
	size m_pendingSize; // without border
	bool m_sizing = false;
	hwnd::subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
	bool m_maximizing = false;

	SIZE get_border_SIZE(double dpi)
	{
		SIZE sz;
		RECT borderRect = { 0, 0, 0, 0 };
		BOOL b = AdjustWindowRectExForDpi(&borderRect, m_style, FALSE, m_extendedStyle, (int)dpi);
		sz.cx = borderRect.right - borderRect.left;
		sz.cy = borderRect.bottom - borderRect.top;
		return sz;
	}

	SIZE get_border_SIZE()
	{
		return get_border_SIZE(get_device_context().get_dpi());
	}

	size get_border_size()
	{
		size sz;
		SIZE sz2 = get_border_SIZE();
		sz.get_width() = sz2.cx;
		sz.get_height() = sz2.cy;
		return sz;
	}

public:
	bool m_initialReshapeDone;

	window(const ptr<rc_obj_base>& desc, const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(desc, composite_string(), 0, WS_EX_NOPARENTNOTIFY | WS_EX_OVERLAPPEDWINDOW, uiSubsystem, user_drawn),
		m_initialReshapeDone(false),
		m_sizingMode(0)
	{
	}

	virtual void installing()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		string title = w->get_title().composite();

		m_style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED;
		if (!title.is_empty())
			m_style |= WS_CAPTION;

		install_HWND();
		SetWindowText(get_HWND(), title.cstr());

		hwnd_pane::installing();
	}

	virtual void hiding()
	{
		hwnd_pane::hiding();
		rcptr<volatile hwnd::subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile hwnd::subsystem>();
		uiSubsystem->remove_visible_window(m_visibleRemoveToken);
	}

	virtual void showing()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			rcptr<volatile hwnd::subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile hwnd::subsystem>();
			m_visibleRemoveToken = uiSubsystem->add_visible_window(w.dereference());
		}
		hwnd_pane::showing();
	}

	virtual void set_title(const composite_string& title)
	{
		BOOL b = SetWindowText(get_HWND(), title.composite().cstr());
	}

	virtual bool is_opaque() const
	{
		return true;
	}

	virtual void reshape(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{	
		// We don't want to call reshape() on children, as that will happen in 
		// response to the WM_SIZE message this will generate.  However, if the 
		// size is unchanged, it's still necessary to propagate the reshape request.

		// If initial resize is empty, use existing default window size
		if (m_initialReshapeDone && !newBounds || newBounds.get_size() == get_size())
			bridgeable_pane::reshape(newBounds, point(0, 0));
		else
		{
			bounds newBounds2 = newBounds;
	
			RECT r;
			BOOL b = GetWindowRect(get_HWND(), &r);
			COGS_ASSERT(b);
	
			newBounds2 += point(r.left, r.top);

			gfx::os::gdi::BOUNDS newBounds3 = get_device_context().make_BOUNDS(newBounds2);
			m_pendingSize = get_device_context().make_size(newBounds3.sz);

			RECT r2 = { 0, 0, 0, 0 };	// If top level, account for window border
			b = AdjustWindowRectEx(&r2, m_style, FALSE, m_extendedStyle);
			COGS_ASSERT(b);

			newBounds3.sz.cx += (r2.right - r2.left);
			newBounds3.sz.cy += (r2.bottom - r2.top);
	
			b = MoveWindow(get_HWND(),
				newBounds3.pt.x,
				newBounds3.pt.y,
				newBounds3.sz.cx,
				newBounds3.sz.cy,
				FALSE);
			COGS_ASSERT(b);
	
			m_initialReshapeDone = true;
		}
	}

	virtual void calculate_range()
	{
		bridgeable_pane::calculate_range();

		HWND hwnd = get_HWND();
		if (get_range().is_fixed())
		{
			// Disable maximize button and resizable frame
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~(WS_MAXIMIZEBOX | WS_THICKFRAME));
		}
		else
		{
			// Enable maximize button and resizable frame
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_MAXIMIZEBOX | WS_THICKFRAME);
		}
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			switch (msg)
			{
			case WM_DPICHANGED:
			{
				RECT* prcNewWindow = (RECT*)lParam;
				SetWindowPos(get_HWND(),
					NULL,
					prcNewWindow->left,
					prcNewWindow->top,
					prcNewWindow->right - prcNewWindow->left,
					prcNewWindow->bottom - prcNewWindow->top,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return 0;
			}
			case WM_GETDPISCALEDSIZE:
			{
				DWORD dpi = (DWORD)wParam;
				LPSIZE sz = (LPSIZE)lParam;

				double oldDpi = get_device_context().get_dpi();

				size currentSize = get_size();
				dpi_changing(oldDpi, dpi);

				calculate_range();

				size newSize = propose_size(currentSize);

				SIZE borderSize = get_border_SIZE(dpi);

				*sz = get_device_context().make_SIZE(newSize);
				sz->cx += borderSize.cx;
				sz->cy += borderSize.cy;

				return 1;
			}
			case WM_ACTIVATE:
			{
				if (LOWORD(wParam))
				{
					get_subsystem()->get_last_activate_window() = get_HWND();
					//focus(0);
				}
				else
				{
					//defocus();
					if (get_subsystem()->get_last_activate_window() == get_HWND())
						get_subsystem()->get_last_activate_window() = NULL;
				}
				return 0;
			}
			case WM_NCCALCSIZE:
			{
				if (!!wParam)
				{
					// We do our own double-buffering on Win32, so prevent default buffering on size/move
					NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
					RECT& newClient = params->rgrc[0];
					RECT& dst = params->rgrc[1];
					RECT& src = params->rgrc[2];

					call_default_window_proc(WM_NCCALCSIZE, (WPARAM)FALSE, (LPARAM)&newClient);

					src.top = 0;
					src.bottom = 0;
					src.left = 0;
					src.right = 0;

					dst = src;

					return WVR_VALIDRECTS;
				}
			}
			case WM_SIZING:
			{
				// Indicate we have started sizing, so it position changes are detected, position
				// should not be reset until WM_SIZE, to ensure top/left resizing works correctly(skew offset).
				m_sizingMode = 1;

				LPRECT resizeRect = (LPRECT)lParam;
				SIZE newSizeWithBorder;
				newSizeWithBorder.cx = resizeRect->right - resizeRect->left;
				newSizeWithBorder.cy = resizeRect->bottom - resizeRect->top;

				SIZE borderSize = get_border_SIZE();
				SIZE newSizeWithoutBorder;
				newSizeWithoutBorder.cx = newSizeWithBorder.cx - borderSize.cx;
				newSizeWithoutBorder.cy = newSizeWithBorder.cy - borderSize.cy;

				bool changedWidth = (wParam == WMSZ_BOTTOMLEFT)
					|| (wParam == WMSZ_BOTTOMRIGHT)
					|| (wParam == WMSZ_TOPLEFT)
					|| (wParam == WMSZ_TOPRIGHT)
					|| (wParam == WMSZ_LEFT)
					|| (wParam == WMSZ_RIGHT);

				bool changedHeight = (wParam == WMSZ_BOTTOM)
					|| (wParam == WMSZ_BOTTOMLEFT)
					|| (wParam == WMSZ_BOTTOMRIGHT)
					|| (wParam == WMSZ_TOP)
					|| (wParam == WMSZ_TOPLEFT)
					|| (wParam == WMSZ_TOPRIGHT);

				m_pendingSize = get_device_context().make_size(newSizeWithoutBorder);

				RECT r;
				GetWindowRect(get_HWND(), &r);
				SIZE sz = { r.right - r.left, r.bottom - r.top };
				if (newSizeWithoutBorder != sz)
				{
					for (;;)
					{
						if (changedWidth)
						{
							if (!changedHeight)
							{
								m_pendingSize = propose_lengths(dimension::horizontal, m_pendingSize);
								break;
							}
						}
						else if (changedHeight)
						{
							m_pendingSize = propose_lengths(dimension::vertical, m_pendingSize);
							break;
						}

						m_pendingSize = propose_size(m_pendingSize);
						break;
					}
				}

				bool trimTop = false;
				bool trimLeft = false;
				switch (wParam)
				{
				case WMSZ_TOP:			// trim height, adjust top
				case WMSZ_TOPRIGHT: 	// trim size, adjust top
					trimTop = true;
					break;
				case WMSZ_TOPLEFT:		// trim size, adjust top/left
					trimTop = true;
				case WMSZ_BOTTOMLEFT:	// trim size, adjust left
				case WMSZ_LEFT:			// trim width, adjust left
					trimLeft = true;
					break;
				case WMSZ_RIGHT:		// trim width
				case WMSZ_BOTTOM:		// trim height
				case WMSZ_BOTTOMRIGHT:	// trim size
				default:
					break;
				}

				SIZE newSize = get_device_context().make_SIZE(m_pendingSize);

				if (trimLeft)
					resizeRect->left += newSizeWithoutBorder.cx - newSize.cx;
				else
					resizeRect->right -= newSizeWithoutBorder.cx - newSize.cx;

				if (trimTop)
					resizeRect->top += newSizeWithoutBorder.cy - newSize.cy;
				else
					resizeRect->bottom -= newSizeWithoutBorder.cy - newSize.cy;

				return 1;
			}
			case WM_SYSCOMMAND:
			{
				switch (wParam)
				{
					case SC_MAXIMIZE:
					{
						// If we maximizing the window, we need to propose the maximum size in WM_GETMINMAXINFO,
						// as the actual maximum may be a subset in 1 dimension.  i.e. If maintaining aspect ratio.
						m_maximizing = true;
						break;
					}
				}
				break;
			}
			case WM_SIZE:
			{
				// If getting WM_SIZE for one we just calculated, use m_pendingSize for precision.

				SIZE requestedClientSize = { LOWORD(lParam), HIWORD(lParam) };
				SIZE pendingSizeAsSIZE = get_device_context().make_SIZE(m_pendingSize);
				size requestedClientSize2;
				if (requestedClientSize == pendingSizeAsSIZE)
					requestedClientSize2 = m_pendingSize;
				else
					requestedClientSize2 = get_device_context().make_size(requestedClientSize);

				size newSize = propose_size(requestedClientSize2);
				point originSkew(0, 0);
				if (m_sizingMode > 1)
				{
					POINT originSkewPOINT = { m_position.x - m_pendingPosition.x, m_position.y - m_pendingPosition.y };
					m_position = m_pendingPosition;
					originSkew = get_device_context().make_point(originSkewPOINT);
				}
				m_sizingMode = 0;

				if ((originSkew != point(0, 0)) || (m_boundsInParentHwnd.sz != get_device_context().make_SIZE(newSize)))
					hwnd_pane::reshape(newSize, originSkew);

				// Because some versions of Windows will move the existing content when resizing from left or top,
				// invalidate the entire window each time.
				if ((originSkew.get_x() != 0) || (originSkew.get_y() != 0))
					invalidate(get_size());

				get_subsystem()->run_high_priority_tasks(false);
				return 0;
			}
			case WM_MOVE:
			{
				int i = 1;
			}
			case WM_WINDOWPOSCHANGING:
			{
				if (m_sizingMode == 1)	// If we need to save the pending size for use in WM_SIZE
				{
					m_sizingMode = 2;
					WINDOWPOS* winPos = (WINDOWPOS*)lParam;
					m_pendingPosition.x = winPos->x;
					m_pendingPosition.y = winPos->y;
				}
				break;
			}
			case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* winPos = (WINDOWPOS*)lParam;
				if (m_sizingMode == 1)	// Not sure why we'd be in this mode, but handle it - save the pending size for use in WM_SIZE
				{
					m_sizingMode = 3;
					m_pendingPosition.x = winPos->x;
					m_pendingPosition.y = winPos->y;
				}
				else if (m_sizingMode != 2)	// 0, or 3?
				{
					m_position.x = winPos->x;
					m_position.y = winPos->y;
				}
				break;
			}
			case WM_GETMINMAXINFO:
			{
				const range& r = get_range();
				SIZE borderSize = get_border_SIZE();
				MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
				SIZE minSize = get_device_context().make_SIZE(r.get_min());
				minMaxInfo->ptMinTrackSize.x = minSize.cx + borderSize.cx;
				minMaxInfo->ptMinTrackSize.y = minSize.cy + borderSize.cy;

				size maxSize = r.get_max();
				if (m_maximizing)
				{
					m_maximizing = false;
					maxSize = propose_size(maxSize);
				}

				SIZE maxSize2 = get_device_context().make_SIZE(maxSize);
				if (r.has_max_width())
					minMaxInfo->ptMaxTrackSize.x = minMaxInfo->ptMaxSize.x = maxSize2.cx + borderSize.cx;
				if (r.has_max_height())
					minMaxInfo->ptMaxTrackSize.y = minMaxInfo->ptMaxSize.y = maxSize2.cy + borderSize.cy;

				return 0;
			}
			case WM_CLOSE:
			{
				request_close(*w);
				return 0;
			}
			case WM_ENTERSIZEMOVE:
			{
				m_isInModalSizingLoop = true;
				m_lastTimerId = (void*)SetTimer(get_HWND(), 1, USER_TIMER_MINIMUM, NULL);
				break;
			}
			case WM_EXITSIZEMOVE:
			{
				m_sizingMode = 0;
				m_isInModalSizingLoop = false;
				KillTimer(get_HWND(), (UINT_PTR)m_lastTimerId.get_ptr());
				break;
			}
			case WM_TIMER:
			{
				get_subsystem()->run_high_priority_tasks(true);
				break;
			}
			}
		}
		return hwnd_pane::process_message(msg, wParam, lParam);
	}
};


inline std::pair<rcref<bridgeable_pane>, rcref<window_interface> > hwnd::subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window, this_rcref);
	return std::make_pair(w, w);
}


inline rcref<gui::window> hwnd::subsystem::open_window(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& f) volatile
{
	rcref<gui::window> w = rcnew(gui::window, title);
	w->nest(p, f);
	install(*w, rcnew(bypass_constructor_permission<hwnd::subsystem>));	// Give each window it's own subsystem instance, so it's own UI thread.
	return w;
}


}
}
}


#endif
