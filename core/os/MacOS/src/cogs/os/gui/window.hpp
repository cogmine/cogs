//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_WINDOW
#define COGS_HEADER_OS_GUI_WINDOW


#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

#include "cogs/collections/container_dlist.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/window.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/nsview.hpp"


namespace cogs {
namespace os {


class window;


};
};


@interface objc_window : NSWindow <NSWindowDelegate>
{
@public
	cogs::weak_rcptr<cogs::os::window> m_cppWindow;
}

-(void)keyDown:(NSEvent*)theEvent;
-(void)keyUp:(NSEvent*)theEvent;
-(void)flagsChanged:(NSEvent*)theEvent;
-(void)close;
-(void)becomeKeyWindow;
-(void)resignKeyWindow;
-(NSSize)windowWillResize:(NSWindow *) window toSize:(NSSize)newSize;
-(void)windowDidResize:(NSNotification *)notification;
-(void)windowWillClose:(NSNotification *)notification;
@end


@interface objc_window_view : objc_view
{
}

-(void)updateTrackingAreas;
@end


namespace cogs {
namespace os {


class window : public nsview_pane, public gui::window_interface
{
public:
	__strong objc_window* m_nsWindow;
	gfx::bounds m_preSizingFrameBounds;
	bool m_initialReshapeDone = false;
	nsview_subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
	__strong NSTrackingArea* m_trackingArea;
	ui::modifier_keys_state m_lastModifierKeysState = {};
	bool m_inResize = false;
	bool m_widthChanged;
	bool m_heightChanged;
	gfx::range m_calculatedRange;

	explicit window(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(uiSubsystem)
	{ }

	virtual void installing()
	{
		int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
		style |= NSWindowStyleMaskMiniaturizable;
		style |= NSWindowStyleMaskResizable;

		__strong objc_window_view* nsView = [[objc_window_view alloc] init];
		nsView->m_cppView = this_rcptr;

		[nsView setAutoresizesSubviews:NO];
		[nsView setPostsFrameChangedNotifications:YES];

		NSRect windowBounds; // TMP - use some temp bounds, we will resize it before showing it.
		windowBounds.origin.x = 50;
		windowBounds.origin.y = 50;
		windowBounds.size.height = 50;
		windowBounds.size.width = 50;
		m_nsWindow = [[objc_window alloc] initWithContentRect:windowBounds
			styleMask: style
			backing: NSBackingStoreBuffered
			defer: NO ];
		m_nsWindow->m_cppWindow = this_rcptr;
		[m_nsWindow setDelegate: m_nsWindow];

		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		string title = w->get_title().composite();
		__strong NSString* str = string_to_NSString(title);
		[m_nsWindow setTitle: str];

		[m_nsWindow setReleasedWhenClosed:NO];
		[m_nsWindow setContentView:nsView];

		install_NSView(nsView);
		nsview_pane::installing();
	}

	virtual void uninstalling()
	{
		objc_window_view* objcView = (objc_window_view*)get_NSView();
		if (m_trackingArea != nullptr)
		{
			[objcView removeTrackingArea: m_trackingArea];
			m_trackingArea = nullptr;
		}
	}

	void key_down(NSEvent* theEvent)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			int key = (int)[[theEvent characters] characterAtIndex:0];
			ui::modifier_keys_state modifiers = get_modifier_keys(theEvent);
			if (![theEvent isARepeat])
				pane_orchestrator::key_press(*w, key, modifiers);
			pane_orchestrator::character_type(*w, key, modifiers);
		}
	}

	void key_up(NSEvent* theEvent)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			int key = (int)[[theEvent characters] characterAtIndex:0];
			ui::modifier_keys_state modifiers = get_modifier_keys(theEvent);
			pane_orchestrator::key_release(*w, key, modifiers);
		}
	}

	ui::modifier_keys_state get_modifier_keys(NSEvent* theEvent)
	{
		ui::modifier_keys_state result;
		NSEventModifierFlags flags = [theEvent modifierFlags];

		result.set_key(ui::physical_modifier_key::left_shift_key, (flags & (1 << 1)) != 0);
		result.set_key(ui::physical_modifier_key::right_shift_key, (flags & (1 << 2)) != 0);

		result.set_key(ui::physical_modifier_key::left_control_key, (flags & 1) != 0);
		result.set_key(ui::physical_modifier_key::right_control_key, (flags & (1 << 13)) != 0);

		result.set_key(ui::physical_modifier_key::left_alt_key, (flags & (1 << 5)) != 0);
		result.set_key(ui::physical_modifier_key::right_alt_key, (flags & (1 << 6)) != 0);

		result.set_key(ui::physical_modifier_key::left_command_key, (flags & (1 << 3)) != 0);
		result.set_key(ui::physical_modifier_key::right_command_key, (flags & (1 << 4)) != 0);

		result.set_key(ui::toggleable_modifier_key::caps_lock_key, (flags & NSEventModifierFlagCapsLock) != 0);
		result.set_key(ui::toggleable_modifier_key::num_lock_key, (flags & NSEventModifierFlagNumericPad) != 0);

		if (result != m_lastModifierKeysState)
		{
			rcptr<gui::pane> p = get_bridge();
			if (!!p)
				pane_orchestrator::modifier_keys_change(*p, result);
			m_lastModifierKeysState = result;
		}

		return result;
	}

	void flags_changed(NSEvent* theEvent)
	{
		get_modifier_keys(theEvent);
	}

	void focus(int direction = 0)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
			pane_orchestrator::focus(*w, direction);
	}

	using nsview_pane::hide;
	using nsview_pane::focus;
	using nsview_pane::defocus;

	virtual void hiding()
	{
		if (!!m_nsWindow)
			[m_nsWindow orderOut:nil];
		nsview_pane::hiding();
		rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile nsview_subsystem>();
		uiSubsystem->remove_visible_window(m_visibleRemoveToken);
	}

	virtual void showing()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
		{
			rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile nsview_subsystem>();
			m_visibleRemoveToken = uiSubsystem->add_visible_window(w.dereference());
		}
		nsview_pane::showing();
		if (!!m_nsWindow)
			[m_nsWindow makeKeyAndOrderFront:nil];
	}

	virtual void set_title(const composite_string& title)
	{
		__strong NSString* str = string_to_NSString(title);
		[m_nsWindow setTitle: str] ;
	}

	virtual bool is_opaque() const
	{
		return true;
	}

	virtual void calculate_range()
	{
		nsview_pane::calculate_range();

		NSRect minRect;
		minRect.origin.x = minRect.origin.y = 0;
		minRect.size = [m_nsWindow minSize];
		minRect = [m_nsWindow contentRectForFrameRect: minRect];
		gfx::range minRange1(gfx::size(minRect.size.width, minRect.size.height), gfx::size(0, 0), false, false);
		minRect.origin.x = minRect.origin.y = minRect.size.height = 0;
		minRect.size.width = [NSWindow minFrameWidthWithTitle: @" " styleMask: [m_nsWindow styleMask]];
		minRect.size.width += 3; // Fudge factor due to minFrameWidthWithTitle not correct detecting min width (as passed to windowWillResize)
		gfx::range minRange2(gfx::size(minRect.size.width, minRect.size.height), gfx::size(0, 0), false, false);
		minRange1 &= minRange2;
		m_calculatedRange = nsview_pane::get_range() & minRange1;
		bool resizable = !m_calculatedRange.is_empty() && !m_calculatedRange.is_fixed();
		if (resizable)
			[m_nsWindow setShowsResizeIndicator : YES];
		else
			[m_nsWindow setShowsResizeIndicator : NO];
	}

	virtual gfx::range get_range() const { return m_calculatedRange; }

	NSSize resize(NSSize newNSSize)
	{
		rcptr<gui::pane> owner = get_bridge();
		NSRect frameRect = [m_nsWindow frame];

		if (!m_widthChanged)
			m_widthChanged = frameRect.size.width != newNSSize.width;

		if (!m_heightChanged)
			m_heightChanged = frameRect.size.height != newNSSize.height;

		if (!m_widthChanged && !m_heightChanged)
			return newNSSize;

		m_preSizingFrameBounds = make_bounds(frameRect);

		NSRect r;
		r.origin.x = 0;
		r.origin.y = 0;
		r.size = newNSSize;
		NSRect newContentRect = [m_nsWindow contentRectForFrameRect: r];
		gfx::size newSize = make_size(newContentRect);

		std::optional<gfx::dimension> resizeDimension;
		if (m_widthChanged)
		{
			if (!m_heightChanged)
				resizeDimension = gfx::dimension::horizontal;
		}
		else if (m_heightChanged)
			resizeDimension = gfx::dimension::vertical;
		std::optional<gfx::size> newSizeOpt = owner->propose_size_best(newSize, gfx::range::make_unbounded(), resizeDimension);
		newSize = newSizeOpt.has_value() ? *newSizeOpt : gfx::size(0, 0);

		// Always round up the next whole pixel, to avoid the wandering window problem
		newSize.assign_ceil();

		r.size = make_NSSize(newSize);
		r = [m_nsWindow frameRectForContentRect: r];
		return r.size;
	}

	// Incoming contentSize is in DIPs
	// Incoming position and frameSize are in screen coordinates.
	virtual void set_initial_shape(
		const std::optional<gfx::point> position,
		const std::optional<gfx::size> contentSize,
		const std::optional<gfx::size> frameSize,
		bool center)
	{
		NSRect r;
		r.origin.x = 0;
		r.origin.y = 0;
		gfx::size contentSizeInDips;
		gfx::size adjustedContentSizeInDips = { 0, 0 };
		bool proposeNewSize = true;
		//NSSize frameBorderNSSize = [m_nsWindow contentRectForFrameRect: r].size;
		//gfx::size frameBorderSizeInDips = make_size(frameBorderNSSize);
		if (contentSize.has_value())
			contentSizeInDips = *contentSize;
		else if (frameSize.has_value())
		{
			NSRect frameBorderNSRect = make_NSRect(*frameSize);
			NSSize contentNSSize = [m_nsWindow contentRectForFrameRect: frameBorderNSRect].size;
			contentSizeInDips = make_size(contentNSSize);
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
			// TODO: Add fallback content size if 0x0 ?
			//gfx::size defaultSize = get_default_size();
			//r.size = make_NSSize(defaultSize);
			//r.size = [m_nsWindow frameRectForContentRect: r].size;
		}
		if (proposeNewSize)
		{
			std::optional<gfx::size> opt = propose_size_best(contentSizeInDips);
			adjustedContentSizeInDips = opt.has_value() ? *opt : gfx::size(0, 0);
		}

		// Convert back to frame size
		NSSize contentNSSize = make_NSSize(adjustedContentSizeInDips);
		r.size = [m_nsWindow frameRectForContentRect: r].size;

		//gfx::size frameSizeInDips = adjustedContentSizeInDips + frameBorderSizeInDips;
		//SIZE frameSize = get_device_context().make_SIZE(frameSizeInDips);

		if (position.has_value())
			r.origin = make_NSPoint(*position);

		[m_nsWindow setFrame:r display:FALSE];
		NSRect newNSRect = [m_nsWindow frame];
		if (newNSRect.size.width != r.size.width || newNSRect.size.height != r.size.height)
		{
			bool wasMinimimWidthImposed = newNSRect.size.width > r.size.width;
			bool wasMinimimHeightImposed = newNSRect.size.height > r.size.height;
			NSSize newContentNSSize = [m_nsWindow contentRectForFrameRect: newNSRect].size;
			gfx::size newContentSizeInDips = make_size(newContentNSSize);
			std::optional<gfx::size> sz = propose_size(newContentSizeInDips).find_first_valid_size(get_primary_flow_dimension(), wasMinimimWidthImposed, wasMinimimHeightImposed);
			if (sz.has_value() && *sz != newContentSizeInDips)
			{
				newNSRect.size = make_NSSize(*sz);
				r.size = [m_nsWindow frameRectForContentRect: newNSRect].size;
				[m_nsWindow setFrame:r display:FALSE];
			}
		}

		if (!position.has_value()) //&& centerPosition) // MacOS doesn't seem to have OS default for initial position
			[m_nsWindow center];
	}

	virtual gfx::bounds get_frame_bounds() const
	{
		NSRect r = [m_nsWindow frame];
		return make_bounds(r);
	}

	virtual void resize(const gfx::size& newSize)
	{
		NSRect oldFrameRect = [m_nsWindow frame];
		m_preSizingFrameBounds = make_bounds(oldFrameRect);

		NSSize newNSSize = make_NSSize(newSize);
		[m_nsWindow setContentSize: newNSSize];

		// We don't want to call reshape() on children, as that will happen in
		// response to the windowDidResize message setContentSize will generate.  However, if the
		// size is unchanged, windowDidResize will not occur, and we need to propagate
		// the reshape request.

		NSRect newFrameRect = [m_nsWindow frame];
		if (newFrameRect.size.width == oldFrameRect.size.width && newFrameRect.size.height == oldFrameRect.size.height)
			reshaped();
	}

	virtual void reshape_frame(const gfx::bounds& newBounds)
	{
		NSRect oldFrameRect = [m_nsWindow frame];
		m_preSizingFrameBounds = make_bounds(oldFrameRect);

		NSRect r = make_NSRect(newBounds);
		[m_nsWindow setFrame: r display: TRUE];

		// We don't want to call reshape() on children, as that will happen in
		// response to the windowDidResize message setContentSize will generate.  However, if the
		// size is unchanged, windowDidResize will not occur, and we need to propagate
		// the reshape request.

		NSRect newFrameRect = [m_nsWindow frame];
		if (newFrameRect.size.width == oldFrameRect.size.width && newFrameRect.size.height == oldFrameRect.size.height)
			reshaped();
	}

	void reshaped()
	{
		NSRect newFrameRect = [m_nsWindow frame];
		gfx::bounds newFrameBounds = make_bounds(newFrameRect);

		gfx::point oldOrigin(0, 0);
		if (!m_initialReshapeDone)
			m_initialReshapeDone = true;
		else
		{
			gfx::bounds oldFrameBounds = m_preSizingFrameBounds;

			// Flip Y, since screen coords are Cartesian.
			oldFrameBounds.get_position().get_y() += oldFrameBounds.get_size().get_height();
			newFrameBounds.get_position().get_y() += newFrameBounds.get_size().get_height();

			oldOrigin.get_x() = oldFrameBounds.get_position().get_x() - newFrameBounds.get_position().get_x();
			oldOrigin.get_y() = oldFrameBounds.get_position().get_y() - newFrameBounds.get_position().get_y();
		}

		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		w->frame_reshaped(newFrameBounds, oldOrigin);
	}

	virtual void frame_reshaped()
	{
		// Convert to context rect
		NSSize sz = [m_nsWindow contentRectForFrameRect: [m_nsWindow frame]].size;
		gfx::size sz2 = make_size(sz);
		nsview_pane::reshape(sz2, gfx::point(0, 0));
	}

	virtual gfx::cell::propose_size_result propose_frame_size(
		const gfx::size& sz,
		const gfx::range& r = gfx::range::make_unbounded(),
		const std::optional<gfx::dimension>& resizeDimension = std::nullopt,
		gfx::cell::sizing_mask sizingMask = gfx::cell::all_sizing_types) const
	{
		gfx::bounds frameBounds = get_frame_bounds();
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

	void update_tracking_areas()
	{
		objc_window_view* objcView = (objc_window_view*)get_NSView();

		if (m_trackingArea != nullptr)
		{
			[objcView removeTrackingArea: m_trackingArea];
			m_trackingArea = nullptr;
		}

		m_trackingArea = [[NSTrackingArea alloc] initWithRect:objcView.bounds
			options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
			owner: objcView
			userInfo: nil];

		[objcView addTrackingArea: m_trackingArea];
	}

	void cursor_move(const NSPoint& positionInWindow)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		objc_window_view* objcView = (objc_window_view*)get_NSView();
		NSPoint pt = [objcView convertPoint: positionInWindow fromView: nil];
		pane_orchestrator::cursor_move(*w, make_point(pt));
	}

	void cursor_leave()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		pane_orchestrator::cursor_leave(*w);
	}

	virtual gfx::font_parameters get_default_text_font() const { return os::graphics_context::get_default_font(); }
	virtual color get_default_text_foreground_color() const { return from_NSColor(NSColor.textColor); }
	virtual color get_default_text_background_color() const { return from_NSColor(NSColor.textBackgroundColor); }
	virtual color get_default_selected_text_foreground_color() const { return from_NSColor(NSColor.selectedTextColor); }
	virtual color get_default_selected_text_background_color() const { return from_NSColor(NSColor.selectedTextBackgroundColor); }
	virtual color get_default_label_foreground_color() const { return from_NSColor(NSColor.labelColor); }
	virtual color get_default_background_color() const { return from_NSColor(NSColor.windowBackgroundColor); }
};


inline std::pair<rcref<gui::bridgeable_pane>, rcref<gui::window_interface> > nsview_subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window)(this_rcref);
	return std::make_pair(w, w);
}


}
}


#ifdef COGS_OBJECTIVE_C_CODE


/// @internal
@implementation objc_window_view

- (void)updateTrackingAreas
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppView.static_cast_to<cogs::os::window>();
	if (!!cppWindow)
		cppWindow->update_tracking_areas();
}

@end


@implementation objc_window


- (void)mouseEntered:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_move([theEvent locationInWindow]);
}

-(void)mouseMoved : (NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_move([theEvent locationInWindow]);
}

-(void)mouseExited: (NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_leave();
}


-(void)keyDown:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->key_down(theEvent);
}


-(void)keyUp:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->key_up(theEvent);
}


-(void)flagsChanged:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->flags_changed(theEvent);
}


-(void)close
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!cppWindow)
		[super close];
	else
		cppWindow->hide();
}


-(void)becomeKeyWindow
{
	[super becomeKeyWindow];

	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->focus(0);
}


-(void)resignKeyWindow
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->defocus();
	[super resignKeyWindow];
}

-(void)windowWillStartLiveResize:(NSNotification*)notification
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		cppWindow->m_inResize = true;
		cppWindow->m_widthChanged = false;
		cppWindow->m_heightChanged = false;
	}
}

-(void)windowWillEndLiveResize:(NSNotification*)notification
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->m_inResize = false;
}

-(NSSize)windowWillResize:(NSWindow *) window toSize:(NSSize)newSize
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		return cppWindow->resize(newSize);
	return newSize;
}


-(void)windowDidResize:(NSNotification *)notification;
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		return cppWindow->reshaped();
}

-(BOOL)windowShouldClose:(id)sender
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		cogs::rcptr<cogs::gui::window> w = cppWindow->get_bridge().template static_cast_to<cogs::gui::window>();
		if (!!w && !cppWindow->request_close(*w))
			return NO;
	}
	return YES;
}

-(void)windowWillClose:(NSNotification *)notification;
{
	cogs::rcptr<cogs::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		cogs::rcptr<cogs::gui::window> w = cppWindow->get_bridge().template static_cast_to<cogs::gui::window>();
		if (!!w)
			cppWindow->close(*w);
	}
}


@end


#endif


#endif
