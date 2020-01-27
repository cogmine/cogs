//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_GDI_WINDOW
#define COGS_HEADER_OS_GUI_GDI_WINDOW


#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/vector.hpp"
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
	int m_sizingMode = 0; // 0 = none, 1 = WM_SIZING with WM_SIZE pending, 2 = WM_WINDOWPOSCHANGING with WM_WINDOWPOSCHANGED pending
	POINT m_position; // Always in native pixels for the DPI.
	POINT m_pendingPosition;
	size m_pendingSize; // without border
	bool m_sizing = false;
	hwnd::subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
	bool m_processingZoomRequest = false;
	bool m_mouseTracking = false;

	bool is_zoomed() const
	{
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(get_HWND(), &windowPlacement);
		return windowPlacement.showCmd == SW_MAXIMIZE;
	}

	SIZE get_current_border_size()
	{
		SIZE sz;
		if (is_zoomed())
		{
			RECT clientRect;
			GetClientRect(get_HWND(), &clientRect);

			RECT frameRect;
			GetWindowRect(get_HWND(), &frameRect);

			sz.cx = (frameRect.right - frameRect.left) - (clientRect.right - clientRect.left);
			sz.cy = (frameRect.bottom - frameRect.top) - (clientRect.bottom - clientRect.top);
		}
		else
		{
			RECT borderRect = { 0, 0, 0, 0 };
			AdjustWindowRectExForDpi(&borderRect, m_style, FALSE, m_extendedStyle, (int)get_device_context().get_dpi());
			sz.cx = borderRect.right - borderRect.left;
			sz.cy = borderRect.bottom - borderRect.top;
		}

		return sz;
	}

	SIZE get_unmaximized_border_SIZE(double dpi)
	{
		SIZE sz;
		RECT borderRect = { 0, 0, 0, 0 };
		AdjustWindowRectExForDpi(&borderRect, m_style, FALSE, m_extendedStyle, (int)dpi);
		sz.cx = borderRect.right - borderRect.left;
		sz.cy = borderRect.bottom - borderRect.top;
		return sz;
	}

	SIZE get_unmaximized_border_SIZE()
	{
		return get_unmaximized_border_SIZE(get_device_context().get_dpi());
	}

public:
	window(rc_obj_base& desc, const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(desc, composite_string(), 0, WS_EX_NOPARENTNOTIFY | WS_EX_OVERLAPPEDWINDOW, uiSubsystem, user_drawn)
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
		SetWindowText(get_HWND(), title.composite().cstr());
	}

	virtual bool is_opaque() const
	{
		return true;
	}

	// Incoming position are in screen coordinates.  On Windows, we use real pixels for screen coordinates.
	// Incoming size is in DIPs
	virtual void set_initial_shape(const point* initialPosition, const size* initialFrameSize, bool centerPosition)
	{
		SIZE sz;
		if (initialFrameSize)
		{
			sz.cx = (LONG)std::lround(initialFrameSize->get_width());
			sz.cy = (LONG)std::lround(initialFrameSize->get_height());
		}
		else
		{
			size defaultSize = get_default_size();
			SIZE defaultSIZE = get_device_context().make_SIZE(defaultSize);
			SIZE borderSIZE = get_unmaximized_border_SIZE();
			sz.cx = defaultSIZE.cx + borderSIZE.cx;
			sz.cy = defaultSIZE.cy + borderSIZE.cy;
		}

		POINT pt;
		if (initialPosition)
		{
			pt.x = (LONG)std::lround(initialPosition->get_x());
			pt.y = (LONG)std::lround(initialPosition->get_y());
		}
		else if (!centerPosition)
		{
			// Uses OS-selected position for new window (current position, as a result of using CW_USEDEFAULT)
			RECT r;
			GetWindowRect(get_HWND(), &r);
			pt.x = r.left;
			pt.y = r.top;
		}
		else
		{
			pt.x = 0;
			pt.y = 0;

			// Calculate center position on main screen
			DWORD i = 0;
			for (;;)
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
					b = EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0);
					if (!b)
						break;
					pt.x = dm.dmPosition.x;
					pt.y = dm.dmPosition.y;
					if (dm.dmPelsWidth > (DWORD)sz.cx)
						pt.x += (dm.dmPelsWidth - (DWORD)sz.cx) / 2;
					if (dm.dmPelsHeight > (DWORD)sz.cy)
						pt.y += (dm.dmPelsHeight - (DWORD)sz.cy) / 2;
					break;
				}
			}
		}

		m_pendingPosition = pt;
		m_position = pt;
		m_sizing = true;
		MoveWindow(get_HWND(), pt.x, pt.y, sz.cx, sz.cy, FALSE);
		m_sizing = false;

		RECT r;
		GetClientRect(get_HWND(), &r);
		SIZE contentSize = { r.right - r.left, r.bottom - r.top };
		hwnd_pane::reshape(get_device_context().make_size(contentSize));
	}

	// window::reshape() reshapes the content area, not the window.
	// Coordinates are in DIPs.  The position in newBounds will be 0,0.
	// To reposition a window and/or resize it in screen coordinates, use reshape_frame.
	virtual void reshape(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		// Get current bounds
		gfx::os::gdi::BOUNDS oldBounds = get_frame_BOUNDS();

		// Convert size to screen coordinates, then to frame size
		SIZE sz = get_device_context().make_SIZE(newBounds.get_size());
		SIZE borderSize = get_unmaximized_border_SIZE();
		sz.cx += borderSize.cx;
		sz.cy += borderSize.cy;

		m_pendingPosition = oldBounds.pt;
		m_sizing = true;
		MoveWindow(get_HWND(), oldBounds.pt.x, oldBounds.pt.y, sz.cx, sz.cy, FALSE);
		m_sizing = false;

		// We don't want to call reshape() on children, as that will happen in 
		// response to the WM_SIZE message MoveWindow will generate.  However, if the 
		// size is unchanged, WM_SIZE will not occur, and we need to propagate
		// the reshape request.
		if (sz == oldBounds.sz)
			bridgeable_pane::reshape(newBounds.get_size(), point(0, 0));
	}

	virtual void reshape_frame(const bounds& newBounds)
	{
		POINT pt = { (LONG)newBounds.get_x(), (LONG)newBounds.get_y() };
		SIZE sz = { (LONG)std::lround(newBounds.get_width()), (LONG)std::lround(newBounds.get_height()) };

		SIZE oldSIZE = get_frame_SIZE();

		m_pendingPosition = pt;
		m_sizing = true;
		MoveWindow(get_HWND(), pt.x, pt.y, sz.cx, sz.cy, FALSE);
		m_sizing = false;

		// We don't want to call reshape() on children, as that will happen in 
		// response to the WM_SIZE message this will generate.  However, if the 
		// size is unchanged, WM_SIZE will not occur, and we need to propagate
		// the reshape request.
		if (sz == oldSIZE)
			bridgeable_pane::reshape(newBounds.get_size(), point(0, 0));
	}

	RECT get_frame_RECT() const
	{
		RECT r;
		GetWindowRect(get_HWND(), &r);
		return r;
	}

	gfx::os::gdi::BOUNDS get_frame_BOUNDS() const
	{
		RECT r = get_frame_RECT();
		return { { r.left, r.top }, { r.right - r.left, r.bottom - r.top } };
	}

	virtual bounds get_frame_bounds() const
	{
		RECT r = get_frame_RECT();
		return { { (double)r.left, (double)r.top }, { (double)(r.right - r.left), (double)(r.bottom - r.top) } };
	}

	SIZE get_frame_SIZE() const
	{
		RECT r;
		GetWindowRect(get_HWND(), &r);
		return { r.right - r.left, r.bottom - r.top };
	}

	virtual void calculate_range()
	{
		bridgeable_pane::calculate_range();
		DWORD newStyle = m_style;
		if (get_range().is_fixed())
			newStyle &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME); // Disable maximize button and resizable frame
		else
			newStyle |= (WS_MAXIMIZEBOX | WS_THICKFRAME); // Enable maximize button and resizable frame
		if (m_style != newStyle)
		{
			SetWindowLong(get_HWND(), GWL_STYLE, newStyle);
			m_style = newStyle;
		}
	}

	virtual LRESULT process_message(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			switch (msg)
			{
			case WM_MOUSEWHEEL:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					ScreenToClient(get_HWND(), &pt);
					point pt2 = get_device_context().make_point(pt);
					//WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
					short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
					ui::modifier_keys_state modifiers = get_modifier_keys();
					if (wheel_move(*w, zDelta / -WHEEL_DELTA, pt2, modifiers))
						return 0;
				}
				break;
			case WM_MOUSEMOVE:
				{
					POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
					point pt2 = get_device_context().make_point(pt);
					if (m_mouseTracking)
						cursor_move(*w, pt2);
					else
					{
						cursor_enter(*w, pt2);
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
					cursor_leave(*w);
				}
				break;
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
				hwnd_pane::process_message(WM_GETDPISCALEDSIZE, wParam, lParam);

				DWORD dpi = (DWORD)wParam;
				LPSIZE sz = (LPSIZE)lParam;

				double oldDpi = get_device_context().get_dpi();

				size currentSize = get_size();
				dpi_changing(oldDpi, dpi);

				calculate_range();

				size newSize = propose_size(currentSize).find_first_valid_size(get_primary_flow_dimension());
				SIZE borderSize = get_unmaximized_border_SIZE(dpi);
				SIZE newSIZE = get_device_context().make_SIZE(newSize);
				newSIZE.cx += borderSize.cx;
				newSIZE.cy += borderSize.cy;
				*sz = newSIZE;

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
					//RECT& newClient = params->rgrc[0];
					RECT& dst = params->rgrc[1];
					RECT& src = params->rgrc[2];

					hwnd_pane::process_message(WM_NCCALCSIZE, wParam, lParam);

					src.top = 0;
					src.bottom = 0;
					src.left = 0;
					src.right = 0;

					dst.top = 0;
					dst.bottom = 0;
					dst.left = 0;
					dst.right = 0;

					return WVR_VALIDRECTS;
				}
				break;
			}
			case WM_SIZING:
			{
				// Indicate we have started sizing, so its position changes are detected.
				// Position should not be reset until WM_SIZE, to ensure top/left resizing works correctly(skew offset).
				m_sizingMode = 1;

				LPRECT resizeRect = (LPRECT)lParam;
				SIZE newSizeWithBorder;
				newSizeWithBorder.cx = resizeRect->right - resizeRect->left;
				newSizeWithBorder.cy = resizeRect->bottom - resizeRect->top;

				SIZE borderSize = get_unmaximized_border_SIZE();
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
				if (newSizeWithBorder != sz)
				{
					for (;;)
					{
						if (changedWidth)
						{
							if (!changedHeight)
							{
								m_pendingSize = propose_size(m_pendingSize, dimension::horizontal).find_first_valid_size(dimension::horizontal);
								break;
							}
						}
						else if (changedHeight)
						{
							m_pendingSize = propose_size(m_pendingSize, dimension::vertical).find_first_valid_size(dimension::vertical);
							break;
						}

						m_pendingSize = propose_size(m_pendingSize).find_first_valid_size(get_primary_flow_dimension());
						break;
					}
				}

				bool trimTop = false;
				bool trimLeft = false;
				switch (wParam)
				{
				case WMSZ_TOP:         // trim height, adjust top
				case WMSZ_TOPRIGHT:    // trim size, adjust top
					trimTop = true;
					break;
				case WMSZ_TOPLEFT:     // trim size, adjust top/left
					trimTop = true;
				case WMSZ_BOTTOMLEFT:  // trim size, adjust left
				case WMSZ_LEFT:        // trim width, adjust left
					trimLeft = true;
					break;
				case WMSZ_RIGHT:       // trim width
				case WMSZ_BOTTOM:      // trim height
				case WMSZ_BOTTOMRIGHT: // trim size
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
			case WM_NCLBUTTONDBLCLK:
			{
				if (!is_zoomed())
				{
					m_processingZoomRequest = true;
					LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
					m_processingZoomRequest = false;
					return result;
				}
				break;
			}
			case WM_SYSCOMMAND:
			{
				switch (wParam)
				{
					case SC_MAXIMIZE:
					{
						// If we maximizing the window, we need to propose the maximum size in WM_GETMINMAXINFO,
						// as the actual maximum may be a subset in 1 dimension.  i.e. If maintaining aspect ratio.
						m_processingZoomRequest = true;
						LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
						m_processingZoomRequest = false;
						return result;
					}
				}
				break;
			}
			case WM_SIZE:
			{
				SIZE requestedClientSIZE = { LOWORD(lParam), HIWORD(lParam) };
				size proposedSize;
				for (;;)
				{
					if (m_sizingMode > 0)
					{
						// m_pendingSize may be slightly different converting, proposing, and converting back.
						// If m_pendingSize reduces to the currently proposed size, use it instead of the params.
						SIZE pendingSIZE = get_device_context().make_SIZE(m_pendingSize);
						if (pendingSIZE == requestedClientSIZE)
						{
							proposedSize = m_pendingSize;
							break;
						}
					}
					size requestedClientSize = get_device_context().make_size(requestedClientSIZE);
					proposedSize = propose_size(requestedClientSize).find_first_valid_size(get_primary_flow_dimension());
					break;
				}

				point originSkew(0, 0);
				if (m_sizing || m_sizingMode > 1)
				{
					POINT originSkewPOINT = { m_position.x - m_pendingPosition.x, m_position.y - m_pendingPosition.y };
					m_position = m_pendingPosition;
					originSkew = get_device_context().make_point(originSkewPOINT);
				}
				m_sizingMode = 0;

				if ((originSkew != point(0, 0)) || (m_boundsInParentHwnd.sz != get_device_context().make_SIZE(proposedSize)))
					hwnd_pane::reshape(proposedSize, originSkew);

				// Because some versions of Windows will move the existing content when resizing from left or top,
				// invalidate the entire window each time.
				if ((originSkew.get_x() != 0) || (originSkew.get_y() != 0))
					invalidate(get_size());

				get_subsystem()->run_high_priority_tasks(false);

				return 0;
			}
			case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS* winPos = (WINDOWPOS*)lParam;
				if (m_sizingMode == 1) // If we need to save the pending size for use in WM_SIZE
				{
					m_sizingMode = 2;
					m_pendingPosition.x = winPos->x;
					m_pendingPosition.y = winPos->y;
				}
				break;
			}
			case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* winPos = (WINDOWPOS*)lParam;
				if (m_sizingMode == 1) // Not sure why we'd be in this mode, but handle it - save the pending size for use in WM_SIZE
				{
					m_sizingMode = 3;
					m_pendingPosition.x = winPos->x;
					m_pendingPosition.y = winPos->y;
				}
				else if (m_sizingMode != 2) // 0, or 3?
				{
					m_position.x = winPos->x;
					m_position.y = winPos->y;
				}
				break;
			}
			case WM_GETMINMAXINFO:
			{
				MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;

				double dpi = get_device_context().get_dpi();
				SIZE borderSize = get_unmaximized_border_SIZE(dpi);

				range r = get_range();
				SIZE minSize = get_device_context().make_SIZE(r.get_min());

				// Min size is easy
				SIZE minTrackSize = { minSize.cx + borderSize.cx, minSize.cy + borderSize.cy };
				if (minTrackSize.cx > minMaxInfo->ptMinTrackSize.x)
					minMaxInfo->ptMinTrackSize.x = minTrackSize.cx;
				if (minTrackSize.cy > minMaxInfo->ptMinTrackSize.y)
					minMaxInfo->ptMinTrackSize.y = minTrackSize.cy;

				if (r.has_max_width() || r.has_max_height())
				{
					// Due to a Windows bug with DPI awareness bug,
					// We never touch ptMaxSize, only ptMaxTrackSize.
					// If being called in response to an event that might result in maximizing,
					// we set ptMaxTrackSize to the maximized size.  Otherwise, we set ptMaxTrackSize
					// to represent separate X and Y maximimum.
					POINT maxTrackSize = minMaxInfo->ptMaxTrackSize;
					if (m_processingZoomRequest)
					{
						MONITORINFO mi;
						mi.cbSize = sizeof(mi);
						GetMonitorInfo(MonitorFromWindow(get_HWND(), MONITOR_DEFAULTTONEAREST), &mi);
						size proposedSize = get_device_context().make_size(mi.rcWork);
						proposedSize = propose_size(proposedSize).find_first_valid_size(get_primary_flow_dimension());
						SIZE proposedSIZE = get_device_context().make_SIZE(proposedSize);
						maxTrackSize.x = proposedSIZE.cx + borderSize.cx;
						maxTrackSize.y = proposedSIZE.cy + borderSize.cy;
					}
					else
					{
						if (r.has_max_width())
						{
							LONG maxX = get_device_context().make_SIZE(r.get_max_width());
							maxTrackSize.x = maxX + borderSize.cx;
						}
						if (r.has_max_height())
						{
							LONG maxY = get_device_context().make_SIZE(r.get_max_height());
							maxTrackSize.y = maxY + borderSize.cy;
						}
					}

					if (maxTrackSize.x < minMaxInfo->ptMaxTrackSize.x)
						minMaxInfo->ptMaxTrackSize.x = maxTrackSize.x;

					if (maxTrackSize.y < minMaxInfo->ptMaxTrackSize.y)
						minMaxInfo->ptMaxTrackSize.y = maxTrackSize.y;
				}

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
	const gfx::canvas::point* screenPosition,
	const gfx::canvas::size* frameSize,
	bool positionCentered,
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& f) volatile
{
	rcref<gui::window> w = rcnew(gui::window, screenPosition, frameSize, positionCentered, title);
	w->nest(p, f);
	install(*w, rcnew(hwnd::subsystem)); // Give each window it's own subsystem instance, so it's own UI thread.
	return w;
}


}
}
}


#endif
