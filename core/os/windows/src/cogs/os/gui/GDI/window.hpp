//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	enum class sizing_state
	{
		none,
		resizing,
		moving,
		zooming,
		unzooming
	};

	volatile ptr<void> m_lastTimerId;
	sizing_state m_sizingState = sizing_state::none;
	POINT m_position; // Always in native pixels for the DPI.
	POINT m_pendingPosition;
	gfx::size m_pendingSize; // without border
	hwnd::subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
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
			std::optional<gfx::size> opt = calculate_size(contentSizeInDips);
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

		m_position = pt;

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
			std::optional<gfx::size> sz = calculate_size(newContentSizeInDips, gfx::range::make_unbounded(), std::nullopt, wasMinimimWidthImposed, wasMinimimHeightImposed);
			if (sz.has_value() && *sz != newContentSizeInDips)
			{
				gfx::size newFrameSizeInDips = *sz + frameBorderSizeInDips;
				SIZE newFrameSize = get_device_context().make_SIZE(newFrameSizeInDips);
				frameSIZE = newFrameSize;
				MoveWindow(get_HWND(), pt.x, pt.y, frameSIZE.cx, frameSIZE.cy, FALSE);
			}
		}

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
		MoveWindow(get_HWND(), oldFrameBounds.pt.x, oldFrameBounds.pt.y, sz.cx, sz.cy, FALSE);
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
		m_pendingPosition = newFrameBounds.pt;
		MoveWindow(get_HWND(), newFrameBounds.pt.x, newFrameBounds.pt.y, newFrameBounds.sz.cx, newFrameBounds.sz.cy, FALSE);
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

	virtual gfx::cell::collaborative_sizes calculate_collaborative_frame_sizes(
		const gfx::size& sz,
		const gfx::range& r = gfx::range::make_unbounded(),
		const std::optional<gfx::cell::quadrant_mask>& quadrants = std::nullopt,
		const std::optional<gfx::dimension>& resizeDimension = std::nullopt) const
	{
		gfx::cell::collaborative_sizes result;
		BOUNDS frameBOUNDS = get_frame_BOUNDS();
		gfx::bounds frameBounds(frameBOUNDS.pt.x, frameBOUNDS.pt.y, frameBOUNDS.sz.cx, frameBOUNDS.sz.cy);
		gfx::range r2 = r & get_range();
		if (!r2.is_invalid())
		{
			gfx::size sz2 = r2.get_limit(sz);
			bool sizeChanged = sz != sz2;
			const gfx::size& frameSize = frameBounds.get_size();
			gfx::size newSize(0, 0);
			if (sz2.get_width() > frameSize.get_width())
				newSize.get_width() = sz2.get_width() - frameSize.get_width();
			if (sz2.get_height() > frameSize.get_height())
				newSize.get_height() = sz2.get_height() - frameSize.get_height();
			rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
			result = w.static_cast_to<gui::pane>()->calculate_collaborative_sizes(newSize, r2 - frameSize, sizeChanged ? gfx::cell::all_quadrants : quadrants, resizeDimension);
			result += frameSize;
			if (sizeChanged)
				result.update_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
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

	virtual void calculating_range()
	{
		hwnd_pane::calculating_range();

		gfx::size minimumSize = get_minimum_window_size();
		m_calculatedRange.set_min(minimumSize);
		m_calculatedRange &= hwnd_pane::get_range();
		DWORD newStyle = m_style;
		if (m_calculatedRange.is_fixed())
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
				DWORD dpi = (DWORD)wParam;
				LPSIZE sz = (LPSIZE)lParam;
				double oldDpi = get_device_context().get_dpi();

				//// There is (was?) a Windows 11 bug in which an incorrect DPI is sometimes sent in wParam.  :(
				//if (dpi == oldDpi)
				//	wprintf(L"WM_GETDPISCALEDSIZE: BAD DPI PROVIDED BY WINDOWS! %d\n", (int)dpi);

				hwnd_pane::process_message(WM_GETDPISCALEDSIZE, wParam, lParam);

				gfx::size currentSize = get_size();
				dpi_changing(oldDpi, dpi);
				calculate_range();
				std::optional<gfx::size> newSize = calculate_size(currentSize);
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

				COGS_ASSERT(m_sizingState != sizing_state::none);

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
					std::optional<gfx::size> opt = calculate_size(m_pendingSize, gfx::range::make_unbounded(), resizeDimension);
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
					[[fallthrough]];
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
			case WM_SYSCOMMAND:
			{
				WPARAM wParam2 = wParam & 0xFFF0;
				switch (wParam2)
				{
					case SC_RESTORE:
					{
						COGS_ASSERT(m_sizingState == sizing_state::none);
						m_sizingState = sizing_state::unzooming;
						LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
						COGS_ASSERT(m_sizingState == sizing_state::unzooming);
						m_sizingState = sizing_state::none;
						return result;
					}
					case SC_MAXIMIZE:
					{
						// When we maximize a window, we need to propose the maximum size in WM_GETMINMAXINFO,
						// as the actual maximum may be lesser in 1 dimension.  i.e. If maintaining aspect ratio.
						COGS_ASSERT(m_sizingState == sizing_state::none);
						m_sizingState = sizing_state::zooming;
						LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
						COGS_ASSERT(m_sizingState == sizing_state::zooming);
						m_sizingState = sizing_state::none;
						return result;
					}

					case SC_SIZE:
					{
						COGS_ASSERT(m_sizingState == sizing_state::none);
						m_sizingState = sizing_state::resizing;
						LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
						COGS_ASSERT(m_sizingState == sizing_state::resizing);
						m_sizingState = sizing_state::none;
						return result;
					}
					case SC_MOVE:
					{
						COGS_ASSERT(m_sizingState == sizing_state::none);
						m_sizingState = sizing_state::moving;
						LRESULT result = hwnd_pane::process_message(msg, wParam, lParam);
						COGS_ASSERT(m_sizingState == sizing_state::moving);
						m_sizingState = sizing_state::none;
						return result;
					}
					default:
						break;
				}
				break;
			}
			case WM_WINDOWPOSCHANGING:
			{
				WINDOWPOS* winPos = (WINDOWPOS*)lParam;
				bool isSizing = (winPos->flags & SWP_NOSIZE) == 0;
				bool isMoving = (winPos->flags & SWP_NOMOVE) == 0;

				RECT r;
				SIZE newSize;
				bool sizeChanged = false;
				if (isSizing)
				{
					newSize = SIZE{winPos->cx, winPos->cy};
					GetWindowRect(get_HWND(), &r);
					SIZE sz = { r.right - r.left, r.bottom - r.top };
					sizeChanged = sz != newSize;
				}

				if (!isMoving)
					m_pendingPosition = m_position;
				else
				{
					m_pendingPosition = POINT { winPos->x, winPos->y };
					if (!isSizing || (!sizeChanged && m_sizingState != sizing_state::zooming && m_sizingState != sizing_state::unzooming))
					{
						// if the window is only moved, there will not be a redraw.
						m_position = m_pendingPosition;
						return 0;
					}
					// Only set m_pendindPosition if it hasn't already been set (to something potentially more accurate).
				}

				if (sizeChanged)
				{
					SIZE borderSize = get_unmaximized_border_SIZE();
					SIZE requestedClientSIZE = newSize - borderSize;

					gfx::size tmpSize = get_device_context().make_size(requestedClientSIZE);;
					std::optional<gfx::size> opt = calculate_size(tmpSize, gfx::range::make_unbounded());
					tmpSize = opt.has_value() ? *opt : gfx::size(0, 0);
					SIZE sz = get_device_context().make_SIZE(tmpSize) + borderSize;
					if (sz != newSize)
					{
						winPos->cx = sz.cx;
						winPos->cy = sz.cy;
					}

					// if m_pendingSize does not decay into this value, replace it.
					SIZE pendingSIZE = get_device_context().make_SIZE(m_pendingSize);
					if (pendingSIZE.cx != winPos->cx || pendingSIZE.cy != winPos->cy)
						m_pendingSize = tmpSize;
				}
				else
				{
					// If the current size decays into this value, use it instead.
					gfx::size tmpSize = get_size();
					SIZE pendingSIZE = get_device_context().make_SIZE(tmpSize);
					if (pendingSIZE.cx != winPos->cx || pendingSIZE.cy != winPos->cy)
						m_pendingSize = tmpSize;
					else
						m_pendingSize = get_device_context().make_size(newSize);
				}

				return 0;
			}
			case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* winPos = (WINDOWPOS*)lParam;
				bool isSizing = (winPos->flags & SWP_NOSIZE) == 0;
				bool isMoving = (winPos->flags & SWP_NOMOVE) == 0;
				if (isSizing || isMoving)
				{
					SIZE requestedClientSIZE = isSizing ? (SIZE{ winPos->cx, winPos->cy } - get_unmaximized_border_SIZE()) : m_boundsInParentHwnd.sz;
					// m_pendingSize should be set.  But, in case it's not, fall back to current size.
					if (m_pendingSize.get_height() == 0 && m_pendingSize.get_width() == 0)
						m_pendingSize = get_size();
					gfx::point originSkew(0, 0);
					POINT originSkewPOINT = m_position - m_pendingPosition;
					m_position = m_pendingPosition;
					originSkew = get_device_context().make_point(originSkewPOINT);
					if (!!originSkew || m_boundsInParentHwnd.sz != requestedClientSIZE)
						hwnd_pane::reshape(m_pendingSize, originSkew);
					m_pendingSize.clear();
					get_subsystem()->run_high_priority_tasks(true);
				}
				return 0;
			}
			case WM_GETMINMAXINFO:
			{
				MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
				double dpi = get_device_context().get_dpi();
				SIZE borderSize = get_unmaximized_border_SIZE(dpi);
				const gfx::range r = get_range();
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
					if (m_sizingState != sizing_state::resizing)
					{
						MONITORINFO mi;
						mi.cbSize = sizeof(mi);
						GetMonitorInfo(MonitorFromWindow(get_HWND(), MONITOR_DEFAULTTONEAREST), &mi);
						gfx::size proposedSize = get_device_context().make_size(mi.rcWork);
						std::optional<gfx::size> opt = calculate_size(proposedSize, gfx::range::make_unbounded(), std::nullopt, true, true, true);
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
				COGS_ASSERT(m_sizingState == sizing_state::resizing || m_sizingState == sizing_state::moving);
				m_lastTimerId = (void*)SetTimer(get_HWND(), 1, USER_TIMER_MINIMUM, NULL);
				break;
			}
			case WM_EXITSIZEMOVE:
			{
				KillTimer(get_HWND(), (UINT_PTR)m_lastTimerId.get_ptr());
				break;
			}
			case WM_TIMER:
			{
				get_subsystem()->run_high_priority_tasks(true);
				break;
			}
			default:
				break;
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
