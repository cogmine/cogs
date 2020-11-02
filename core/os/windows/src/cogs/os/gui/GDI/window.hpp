//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
namespace os {


class window : public hwnd_pane, public gui::window_interface
{
private:
	volatile boolean m_isInModalSizingLoop;
	volatile ptr<void> m_lastTimerId;
	int m_sizingMode = 0; // 0 = none, 1 = WM_SIZING with WM_SIZE pending, 2 = WM_WINDOWPOSCHANGING with WM_WINDOWPOSCHANGED pending
	POINT m_position; // Always in native pixels for the DPI.
	POINT m_pendingPosition;
	gfx::size m_pendingSize; // without border
	bool m_sizing = false;
	hwnd::subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
	bool m_processingZoomRequest = false;
	bool m_mouseTracking = false;
	gfx::range m_calculatedRange;
	color m_defaultTextColor;
	color m_defaultBackgroundColor;

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
		RECT borderRect = { 0, 0, 0, 0 };
		AdjustWindowRectExForDpi(&borderRect, m_style, FALSE, m_extendedStyle, (int)dpi);
		return SIZE{ borderRect.right - borderRect.left, borderRect.bottom - borderRect.top };
	}

	SIZE get_unmaximized_border_SIZE()
	{
		return get_unmaximized_border_SIZE(get_device_context().get_dpi());
	}

public:
	explicit window(const rcref<volatile hwnd::subsystem>& uiSubsystem)
		: hwnd_pane(composite_string(), 0, WS_EX_NOPARENTNOTIFY | WS_EX_OVERLAPPEDWINDOW, uiSubsystem, hwnd_draw_mode::user)
	{ }

	virtual void installing()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		string title = w->get_title().composite();
		m_style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED;
		if (!title.is_empty())
			m_style |= WS_CAPTION;
		install_HWND();
		m_defaultTextColor = from_COLORREF(GetTextColor(get_HDC()));
		m_defaultBackgroundColor = from_COLORREF(GetBkColor(get_HDC()));
		get_device_context().fill(get_size(), m_defaultBackgroundColor, false);
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

	// Incoming contentSize is in DIPs
	// Incoming position and frameSize are in screen coordinates.
	// On Windows, we use real pixels for screen coordinates.
	virtual void set_initial_shape(
		const std::optional<gfx::point> position,
		const std::optional<gfx::size> contentSize,
		const std::optional<gfx::size> frameSize,
		bool center)
	{
		// Get current size, in case set due to CW_USEDEFAULT
		RECT r;
		GetWindowRect(get_HWND(), &r);
		POINT pt;
		pt.x = r.left;
		pt.y = r.top;

		// Figure out the size
		gfx::size contentSizeInDips;
		gfx::size adjustedContentSizeInDips = { 0, 0 };
		bool proposeNewSize = true;
		SIZE contentSIZE;
		SIZE frameBorderSIZE = get_unmaximized_border_SIZE();
		gfx::size frameBorderSizeInDips = get_device_context().make_size(frameBorderSIZE);
		if (contentSize.has_value())
			contentSizeInDips = *contentSize;
		else if (frameSize.has_value())
		{
			contentSizeInDips.get_height() = (frameSize->get_height() > frameBorderSizeInDips.get_height())
				? (frameSize->get_height() - frameBorderSizeInDips.get_height())
				: 0;
			contentSizeInDips.get_width() = (frameSize->get_width() > frameBorderSizeInDips.get_width())
				? (frameSize->get_width() - frameBorderSizeInDips.get_width())
				: 0;
		}
		else
		{
			// Use the default size
			std::optional<gfx::size> defaultSize = get_default_size();
			if (defaultSize.has_value())
			{
				adjustedContentSizeInDips = *defaultSize;
				proposeNewSize = false; // Default is safe.  No need to propose it.
			}
			else
			{
				// If no default size, propose the size set due to CW_USEDEFAULT
				contentSIZE.cx = (r.right - r.left) - frameBorderSIZE.cx;
				contentSIZE.cy = (r.bottom - r.top) - frameBorderSIZE.cy;
				contentSizeInDips = get_device_context().make_size(contentSIZE);
			}
		}
		if (proposeNewSize)
		{
			std::optional<gfx::size> opt = propose_size_best(contentSizeInDips);
			adjustedContentSizeInDips = opt.has_value() ? *opt : gfx::size(0, 0);
		}

		SIZE frameSIZE = get_device_context().make_SIZE(adjustedContentSizeInDips);
		frameSIZE += frameBorderSIZE;

		// Figure out the position
		if (position.has_value())
		{
			pt.x = (LONG)std::lround(position->get_x());
			pt.y = (LONG)std::lround(position->get_y());
		}
		else if (center)
		{
			// Calculate center position on main screen
			pt.x = 0;
			pt.y = 0;
			for (DWORD i = 0; ; i++)
			{
				DISPLAY_DEVICE dd;
				dd.cb = sizeof(DISPLAY_DEVICE);
				BOOL b = EnumDisplayDevicesW(NULL, i, &dd, 0);
				if (!b)
					break;
				if (dd.StateFlags & (DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE))
				{
					DEVMODE dm;
					dm.dmSize = sizeof(DEVMODE);
					dm.dmDriverExtra = 0;
					b = EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0);
					if (!b)
						break;
					pt.x = dm.dmPosition.x;
					pt.y = dm.dmPosition.y;
					if (dm.dmPelsWidth > (DWORD)frameSIZE.cx)
						pt.x += (dm.dmPelsWidth - (DWORD)frameSIZE.cx) / 2;
					if (dm.dmPelsHeight > (DWORD)frameSIZE.cy)
						pt.y += (dm.dmPelsHeight - (DWORD)frameSIZE.cy) / 2;
					break;
				}
			}
		}

		m_pendingPosition = pt;
		m_position = pt;
		m_sizing = true;

		MoveWindow(get_HWND(), pt.x, pt.y, frameSIZE.cx, frameSIZE.cy, FALSE);

		RECT rx;
		GetWindowRect(get_HWND(), &rx);

		SIZE newSize{ rx.right - rx.left, rx.bottom - rx.top };
		if (newSize != frameSIZE)
		{
			bool wasMinimimWidthImposed = newSize.cx > frameSIZE.cx;
			bool wasMinimimHeightImposed = newSize.cy > frameSIZE.cy;
			SIZE newContentSize = newSize - frameBorderSIZE;
			gfx::size newContentSizeInDips = get_device_context().make_size(newContentSize);
			std::optional<gfx::size> sz = propose_size(newContentSizeInDips).find_first_valid_size(get_primary_flow_dimension(), wasMinimimWidthImposed, wasMinimimHeightImposed);
			if (sz.has_value() && *sz != newContentSizeInDips)
			{
				gfx::size newFrameSizeInDips = *sz + frameBorderSizeInDips;
				SIZE newFrameSize = get_device_context().make_SIZE(newFrameSizeInDips);
				frameSIZE = newFrameSize;
				MoveWindow(get_HWND(), pt.x, pt.y, frameSIZE.cx, frameSIZE.cy, FALSE);
			}
		}

		m_sizing = false;
		GetClientRect(get_HWND(), &r);
		contentSIZE = { r.right - r.left, r.bottom - r.top };
		gfx::size actualSize = get_device_context().make_size(contentSIZE);
		hwnd_pane::reshape(actualSize);
	}

	virtual void resize(const gfx::size& newSize)
	{
		// Get current bounds
		BOUNDS oldFrameBounds = get_frame_BOUNDS();
		// Convert size to screen coordinates, then to frame size
		SIZE sz = get_device_context().make_SIZE(newSize);
		SIZE borderSize = get_unmaximized_border_SIZE();
		sz += borderSize;
		m_pendingPosition = oldFrameBounds.pt;
		m_sizing = true;
		MoveWindow(get_HWND(), oldFrameBounds.pt.x, oldFrameBounds.pt.y, sz.cx, sz.cy, FALSE);
		m_sizing = false;
		// We don't want to call reshape() on children, as that will happen in
		// response to the WM_SIZE message MoveWindow will generate.  However, if the
		// size is unchanged, WM_SIZE will not occur, and we need to propagate
		// the reshape request.
		if (sz == oldFrameBounds.sz)
			reshaped(oldFrameBounds);
	}

	virtual void reshape_frame(const gfx::bounds& newBounds)
	{
		// Get current size
		BOUNDS oldFrameBounds = get_frame_BOUNDS();
		BOUNDS newFrameBounds = {
			.pt = POINT{ (LONG)std::lround(newBounds.get_x()), (LONG)std::lround(newBounds.get_y()) },
			.sz = SIZE{ (LONG)std::lround(newBounds.get_width()), (LONG)std::lround(newBounds.get_height()) }
		};
		m_pendingPosition = newFrameBounds.pt;;
		m_sizing = true;
		MoveWindow(get_HWND(), newFrameBounds.pt.x, newFrameBounds.pt.y, newFrameBounds.sz.cx, newFrameBounds.sz.cy, FALSE);
		m_sizing = false;
		// We don't want to call reshape() on children, as that will happen in
		// response to the WM_SIZE message this will generate.  However, if the
		// size is unchanged, WM_SIZE will not occur, and we need to propagate
		// the reshape request.
		if (newFrameBounds.sz == oldFrameBounds.sz)
			reshaped(oldFrameBounds);
	}

	void reshaped(const BOUNDS& oldFrameBounds)
	{
		BOUNDS newFrameBOUNDS = get_frame_BOUNDS();
		gfx::bounds newFrameBounds(newFrameBOUNDS.pt.x, newFrameBOUNDS.pt.y, newFrameBOUNDS.sz.cx, newFrameBOUNDS.sz.cy);
		gfx::point oldOrigin(oldFrameBounds.pt.x - newFrameBOUNDS.pt.x, oldFrameBounds.pt.y - newFrameBOUNDS.pt.y);
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		w->frame_reshaped(newFrameBounds, oldOrigin);
	}

	virtual void frame_reshaped()
	{
		RECT r;
		GetClientRect(get_HWND(), &r);
		SIZE contentSIZE = { r.right - r.left, r.bottom - r.top };
		gfx::size contentSize = get_device_context().make_size(contentSIZE);
		hwnd_pane::reshape(contentSize, gfx::point(0, 0));
	}

	virtual gfx::cell::propose_size_result propose_frame_size(
		const gfx::size& sz,
		const gfx::range& r = gfx::range::make_unbounded(),
		const std::optional<gfx::dimension>& resizeDimension = std::nullopt,
		gfx::cell::sizing_mask sizingMask = gfx::cell::all_sizing_types) const
	{
		BOUNDS frameBOUNDS = get_frame_BOUNDS();
		gfx::bounds frameBounds(frameBOUNDS.pt.x, frameBOUNDS.pt.y, frameBOUNDS.sz.cx, frameBOUNDS.sz.cy);
		gfx::size sz2;
		gfx::range r2;
		if (sz.get_width() <= frameBounds.get_width() || sz.get_height() <= frameBounds.get_height())
		{
			sz2 = gfx::size(0, 0);
			r2 = gfx::range::make_empty();
		}
		else
		{
			sz2 = sz - frameBounds.get_size();
			r2 = r - sz;
		}
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		gfx::cell::propose_size_result result = w.static_cast_to<gui::pane>()->propose_size(sz2, r2, resizeDimension, sizingMask);
		result += frameBounds.get_size();
		return result;
	}

	RECT get_frame_RECT() const
	{
		RECT r;
		GetWindowRect(get_HWND(), &r);
		return r;
	}

	BOUNDS get_frame_BOUNDS() const
	{
		RECT r = get_frame_RECT();
		return { { r.left, r.top }, { r.right - r.left, r.bottom - r.top } };
	}

	virtual gfx::bounds get_frame_bounds() const
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
		hwnd_pane::calculate_range();

		gfx::range windowRange;
		gfx::size minimumSize = get_minimum_window_size();
		m_calculatedRange.set_min(minimumSize);
		m_calculatedRange &= hwnd_pane::get_range() & windowRange;
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

	virtual gfx::range get_range() const { return m_calculatedRange; }

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
					gfx::point pt2 = get_device_context().make_point(pt);
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
					gfx::point pt2 = get_device_context().make_point(pt);
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
				gfx::size currentSize = get_size();
				dpi_changing(oldDpi, dpi);
				calculate_range();
				std::optional<gfx::size> newSize = propose_size(currentSize).find_first_valid_size(get_primary_flow_dimension());
				if (!newSize.has_value())
					sz->cx = sz->cy = 0;
				else
				{
					SIZE borderSize = get_unmaximized_border_SIZE(dpi);
					SIZE newSIZE = get_device_context().make_SIZE(*newSize);
					newSIZE += borderSize;
					*sz = newSIZE;
				}
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
				SIZE newSizeWithBorder{ resizeRect->right - resizeRect->left, resizeRect->bottom - resizeRect->top };
				SIZE borderSize = get_unmaximized_border_SIZE();
				SIZE newSizeWithoutBorder = SIZE{ newSizeWithBorder.cx - borderSize.cx, newSizeWithBorder.cy - borderSize.cy };
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
					std::optional<gfx::dimension> resizeDimension;
					if (changedWidth)
					{
						if (!changedHeight)
							resizeDimension = gfx::dimension::horizontal;
					}
					else if (changedHeight)
						resizeDimension = gfx::dimension::vertical;
					std::optional<gfx::size> opt = propose_size_best(m_pendingSize, gfx::range::make_unbounded(), resizeDimension);
					m_pendingSize = opt.has_value() ? *opt : gfx::size(0, 0);
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
						// When we maximize a window, we need to propose the maximum size in WM_GETMINMAXINFO,
						// as the actual maximum may be lesser in 1 dimension.  i.e. If maintaining aspect ratio.
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
				gfx::size proposedSize;
				for (;;)
				{
					if (m_sizingMode > 0)
					{
						// m_pendingSize may be slightly different due to converting, proposing, and converting back.
						// If m_pendingSize reduces to the currently proposed size, use it instead of the params.
						SIZE pendingSIZE = get_device_context().make_SIZE(m_pendingSize);
						if (pendingSIZE == requestedClientSIZE)
						{
							proposedSize = m_pendingSize;
							break;
						}
					}
					gfx::size requestedClientSize = get_device_context().make_size(requestedClientSIZE);
					std::optional<gfx::size> opt = propose_size_best(requestedClientSize);
					proposedSize = opt.has_value() ? *opt : gfx::size(0, 0);
					break;
				}
				gfx::point originSkew(0, 0);
				if (m_sizing || m_sizingMode > 1)
				{
					POINT originSkewPOINT = m_position - m_pendingPosition;
					m_position = m_pendingPosition;
					originSkew = get_device_context().make_point(originSkewPOINT);
				}
				m_sizingMode = 0;
				if (!!originSkew || m_boundsInParentHwnd.sz != requestedClientSIZE)
					hwnd_pane::reshape(proposedSize, originSkew);
				get_subsystem()->run_high_priority_tasks(true);
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
				gfx::range r = get_range();
				SIZE minSize = get_device_context().make_SIZE(r.get_min());
				// Min size is easy
				SIZE minTrackSize = minSize + borderSize;
				if (minTrackSize.cx > minMaxInfo->ptMinTrackSize.x)
					minMaxInfo->ptMinTrackSize.x = minTrackSize.cx;
				if (minTrackSize.cy > minMaxInfo->ptMinTrackSize.y)
					minMaxInfo->ptMinTrackSize.y = minTrackSize.cy;
				if (r.has_max_width() || r.has_max_height())
				{
					// To work around a Windows DPI awareness bug,
					// we never touch ptMaxSize, only ptMaxTrackSize.
					// If called in response to an event that might result in maximizing,
					// we set ptMaxTrackSize to the maximized size.  Otherwise, we set ptMaxTrackSize
					// to represent separate X and Y maximimum.
					POINT maxTrackSize = minMaxInfo->ptMaxTrackSize;
					if (m_processingZoomRequest)
					{
						MONITORINFO mi;
						mi.cbSize = sizeof(mi);
						GetMonitorInfo(MonitorFromWindow(get_HWND(), MONITOR_DEFAULTTONEAREST), &mi);
						gfx::size proposedSize = get_device_context().make_size(mi.rcWork);
						std::optional<gfx::size> opt = propose_size_best(proposedSize, gfx::range::make_unbounded(), std::nullopt, true, true, true);
						proposedSize = opt.has_value() ? *opt : gfx::size(0, 0);
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

	virtual bool is_opaque() const
	{
		return true;
	}

	virtual gfx::font_parameters get_default_text_font() const { return gdi::device_context::get_default_font(); }
	virtual color get_default_text_foreground_color() const { return from_COLORREF(GetSysColor(COLOR_WINDOWTEXT)); }
	virtual color get_default_text_background_color() const { return from_COLORREF(GetSysColor(COLOR_WINDOW)); }
	virtual color get_default_selected_text_foreground_color() const { return from_COLORREF(GetSysColor(COLOR_HIGHLIGHTTEXT)); }
	virtual color get_default_selected_text_background_color() const { return from_COLORREF(GetSysColor(COLOR_HIGHLIGHT)); }
	virtual color get_default_label_foreground_color() const { return from_COLORREF(GetSysColor(COLOR_WINDOWTEXT)); }
	virtual color get_default_background_color() const { return from_COLORREF(GetSysColor(COLOR_WINDOW)); }
};


inline std::pair<rcref<gui::bridgeable_pane>, rcref<gui::window_interface> > hwnd::subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window)(this_rcref);
	return std::make_pair(w, w);
}


inline rcref<gui::window> hwnd::subsystem::open_window(window_options&& options) volatile
{
	rcref<gui::window> w = rcnew(gui::window)(std::move(options));
	install(*w, rcnew(hwnd::subsystem)); // Give each window it's own subsystem instance, so it's own UI thread.
	return w;
}


}
}


#endif
