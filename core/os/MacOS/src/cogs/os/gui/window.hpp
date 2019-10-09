//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
namespace gui {
namespace os {


class window;


};
};
};


@interface objc_window : NSWindow <NSWindowDelegate>
{
@public
	cogs::weak_rcptr<cogs::gui::os::window> m_cppWindow;
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
namespace gui {
namespace os {


class window : public nsview_pane, public window_interface
{
public:
	__strong objc_window* m_nsWindow;
	bounds m_preSizingBounds;
	bool m_initialReshapeDone = false;
	nsview_subsystem::visible_windows_list_t::volatile_remove_token m_visibleRemoveToken;
	NSSize m_nativeSize;
	__strong NSTrackingArea* m_trackingArea;
	ui::modifier_keys_state m_lastModifierKeysState = {};

	window(const ptr<rc_obj_base>& desc, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem)
	{
	}

	virtual void installing()
	{
		int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
		style |= NSWindowStyleMaskMiniaturizable;
		style |= NSWindowStyleMaskResizable;

		__strong objc_window_view* nsView = [[objc_window_view alloc] init];
		nsView->m_cppView = this_rcptr;

		[nsView setAutoresizesSubviews:NO];
		[nsView setPostsFrameChangedNotifications:YES];

		NSRect windowBounds;		// TMP - use some temp bounds, we will resize it before showing it.
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

		rcptr<pane> owner = get_bridge();
		bool resizable = !owner->get_range().is_fixed();
		if (resizable)
			[m_nsWindow setShowsResizeIndicator : YES];
		else
			[m_nsWindow setShowsResizeIndicator : NO];
	}

	NSSize proposing_size(NSSize newNSSize)
	{
		NSRect frameRect = [m_nsWindow frame];
		NSRect contentRect = [m_nsWindow contentRectForFrameRect : frameRect];

		m_preSizingBounds.set(point(contentRect.origin.x, contentRect.origin.y), size(contentRect.size.width, contentRect.size.height));

		NSRect r;
		r.origin.x = 0;
		r.origin.y = 0;
		r.size = newNSSize;
		NSRect newFrameRect = [m_nsWindow contentRectForFrameRect:r];

		size newSize = graphics_context::make_size(newFrameRect);
		newSize = nsview_pane::propose_size(newSize);
		r.size = graphics_context::make_NSSize(newSize);
		r = [m_nsWindow frameRectForContentRect : r];
		return r.size;
	}

	virtual void set_initial_shape(const point* initialPosition, const size* initialFrameSize, bool centerPosition)
	{
		NSRect r;
		r.origin.x = 0;
		r.origin.y = 0;
		if (initialFrameSize)
			r.size = graphics_context::make_NSSize(*initialFrameSize);
		else
		{
			size defaultSize = get_default_size();
			r.size = graphics_context::make_NSSize(defaultSize);
			r.size = [m_nsWindow frameRectForContentRect: r].size;
		}

		if (initialPosition)
			r.origin = graphics_context::make_NSPoint(*initialPosition);

		[m_nsWindow setFrame:r display:FALSE];

		if (!initialPosition)	//&& centerPosition)	// MacOS doesn't seem to have OS default for initial position
			[m_nsWindow center]; 
	}

	virtual void reshape_frame(const bounds& newBounds)
	{
		NSRect r = graphics_context::make_NSRect(newBounds);

		[m_nsWindow setFrame:r display:FALSE];
	}

	virtual bounds get_frame_bounds() const
	{
		NSRect r = [m_nsWindow frame];
		return graphics_context::make_bounds(r);
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		NSSize newSize = graphics_context::make_NSSize(b.get_size());
		[m_nsWindow setContentSize : newSize];

		// We don't want to call reshape() on children, as that will happen in 
		// response to the windowDidResize message setContentSize will generate.  However, if the 
		// size is unchanged, windowDidResize will not occur, and we need to propagate

		// On Mac, it's not sufficient to check m_nativeSize, as it does not equal real pixels.
		// It's possible for the sizes to NOT machine, but still not result in a windowDidResize if pixel size is unchanged.

		objc_window_view* objcView = (objc_window_view*)get_NSView();
		NSSize newNativeSize = [objcView convertSizeToBacking: newSize];

		if (newNativeSize.width == m_nativeSize.width && newNativeSize.height == m_nativeSize.height)
			bridgeable_pane::reshape(b.get_size(), point(0, 0));

	}

	void reshaping()
	{
		NSRect r = [m_nsWindow frame];
		NSRect r2 = [NSWindow contentRectForFrameRect: r styleMask: [m_nsWindow styleMask] ];

		bounds newBounds;
		newBounds.get_position().get_x() = r2.origin.x;
		newBounds.get_position().get_y() = r2.origin.y;
		newBounds.get_size().get_width() = r2.size.width;
		newBounds.get_size().get_height() = r2.size.height;

		objc_window_view* objcView = (objc_window_view*)get_NSView();
		m_nativeSize = [objcView convertSizeToBacking: r2.size];

		point oldOrigin(0, 0);
		if (!m_initialReshapeDone)
			m_initialReshapeDone = true;
		else
		{
			bounds oldBounds = m_preSizingBounds;

			// Flip Y, since screen coords are Cartesian.
			oldBounds.get_position().get_y() += oldBounds.get_size().get_height();
			newBounds.get_position().get_y() += newBounds.get_size().get_height();

			oldOrigin.get_x() = oldBounds.get_position().get_x();
			oldOrigin.get_x() -= newBounds.get_position().get_x();

			oldOrigin.get_y() = newBounds.get_position().get_y();
			oldOrigin.get_y() -= oldBounds.get_position().get_y();
		}

		nsview_pane::reshape(newBounds, oldOrigin);
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
		pane_orchestrator::cursor_move(*w, graphics_context::make_point(pt));
	}

	void cursor_leave()
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		pane_orchestrator::cursor_leave(*w);
	}
};


inline std::pair<rcref<bridgeable_pane>, rcref<window_interface> > nsview_subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window, this_rcref);
	return std::make_pair(w, w);
}


}
}
}


#ifdef COGS_OBJECTIVE_C_CODE


/// @internal
@implementation objc_window_view

- (void)updateTrackingAreas
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppView.static_cast_to<cogs::gui::os::window>();
	if (!!cppWindow)
		cppWindow->update_tracking_areas();
}

@end



@implementation objc_window


- (void)mouseEntered:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_move([theEvent locationInWindow]);
}

-(void)mouseMoved : (NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_move([theEvent locationInWindow]);
}

-(void)mouseExited: (NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->cursor_leave();
}


-(void)keyDown:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->key_down(theEvent);
}


-(void)keyUp:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->key_up(theEvent);
}


-(void)flagsChanged:(NSEvent*)theEvent
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->flags_changed(theEvent);
}


-(void)close
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!cppWindow)
		[super close];
	else
		cppWindow->hide();
}


-(void)becomeKeyWindow
{
	[super becomeKeyWindow];

	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->focus(0);
}


-(void)resignKeyWindow
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->defocus();
	[super resignKeyWindow];
}


-(NSSize)windowWillResize:(NSWindow *) window toSize:(NSSize)newSize
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		return cppWindow->proposing_size(newSize);
	return newSize;
}


-(void)windowDidResize:(NSNotification *)notification;
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		return cppWindow->reshaping();
}

-(BOOL)windowShouldClose:(id)sender
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
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
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
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
