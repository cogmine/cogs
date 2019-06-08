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

-(void)keyDown:(NSEvent *)theEvent;
-(void)keyUp:(NSEvent *)theEvent;
-(void)close;
-(void)becomeKeyWindow;
-(void)resignKeyWindow;
-(NSSize)windowWillResize:(NSWindow *) window toSize:(NSSize)newSize;
-(void)windowDidResize:(NSNotification *)notification;
-(void)windowWillClose:(NSNotification *)notification;
@end



namespace cogs {
namespace gui {
namespace os {


class window : public nsview_pane, public window_interface
{
public:
	__strong objc_window* m_nsWindow;
	bounds m_preSizingBounds;
	bool m_initialReshapeDone;

	window(const ptr<rc_obj_base>& desc,  const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem),

		m_initialReshapeDone(false)
	{
	}

	//~window()
	//{
	//	((objc_window*)m_nsWindow)->m_cppWindow.release();
	//	((objc_view*)m_nsView)->m_cppView.release();
	//	//[m_nsWindow release];
	//}

	virtual void installing()
	{
		int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
		style |= NSWindowStyleMaskMiniaturizable;
		style |= NSWindowStyleMaskResizable;

		objc_view* nsView = [[objc_view alloc] init];
		nsView->m_cppView = this_rcptr;

		[nsView setAutoresizesSubviews:NO];
		[nsView setPostsFrameChangedNotifications:YES];
		
	//	NSRect screenRect = [[NSScreen mainScreen] frame];
	//	NSApplication* app = [NSApplication sharedApplication];
	//	NSMenu* mainMenu = [app mainMenu];
	//	CGFloat menuBarHeight = [mainMenu menuBarHeight];
	//	screenRect.size.height -= menuBarHeight;
	//
	//	NSRect windowBounds = NSMakeRect(0, screenRect.size.height - get_size().get_height().get_internal().get_int(), get_size().get_width().get_internal().get_int(), get_size().get_height().get_internal().get_int());
		
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
		//[str release];

		[m_nsWindow setReleasedWhenClosed:NO];
		[m_nsWindow setContentView:nsView];

		install_NSView(nsView);
		nsview_pane::installing();

//		[m_nsWindow makeKeyAndOrderFront: nil];
	}

	void character_type(wchar_t c)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
			pane_orchestrator::character_type(*w, c);
	}

	void key_release(wchar_t c)
	{
		rcptr<gui::window> w = get_bridge().template static_cast_to<gui::window>();
		if (!!w)
			pane_orchestrator::key_release(*w, c);
	}



	using nsview_pane::hide;
	using nsview_pane::focus;
	using nsview_pane::defocus;

	virtual void hiding()
	{
		if (!!m_nsWindow)
			[m_nsWindow orderOut:nil];
		nsview_pane::hiding();
	}

	virtual void showing()
	{
		nsview_pane::showing();
		if (!!m_nsWindow)
			[m_nsWindow makeKeyAndOrderFront:nil];
	}

	virtual void set_title(const composite_string& title)
	{
		__strong NSString* str = string_to_NSString(title);
		[m_nsWindow setTitle: str] ;
	}

	virtual void focus(int direction)
	{
		// not a control, don't take GUI focus from a control
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

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		NSSize newSize = make_NSSize(b.get_size());
		[m_nsWindow setContentSize: newSize];
//		[get_NSView() setFrameSize: newSize];
		[get_NSView() setNeedsDisplay:YES];
	}

	NSSize proposing_size(NSSize newSize)
	{
		NSRect r;
		r.origin.x = 0;
		r.origin.y = 0;
		r.size = newSize;

		NSRect currentFrameRect = [m_nsWindow frame];
		NSRect currentContentRect = [NSWindow contentRectForFrameRect : currentFrameRect styleMask : [m_nsWindow styleMask] ];

		m_preSizingBounds.set(point(currentContentRect.origin.x, currentContentRect.origin.y), size(currentContentRect.size.width, currentContentRect.size.height));

		NSRect frame = [NSWindow contentRectForFrameRect: r styleMask : [m_nsWindow styleMask] ];

		size newSize2(frame.size.width, frame.size.height);
		newSize2 = nsview_pane::propose_size(newSize2);
		r.size.width = newSize2.get_width();
		r.size.height = newSize2.get_height();

		r = [NSWindow frameRectForContentRect: r styleMask: [m_nsWindow styleMask] ];
		return r.size;
	}

	virtual void reshaping(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		if (!m_isResizing)
		{
			NSRect r = [self frame];
			NSRect r2 = [NSWindow contentRectForFrameRect: r styleMask: [self styleMask] ];

			bounds newBounds;
			newBounds.get_position().get_x() = r2.origin.x;
			newBounds.get_position().get_y() = r2.origin.y;
			newBounds.get_size().get_width() = r2.size.width;
			newBounds.get_size().get_height() = r2.size.height;

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

			m_isResizing = true;
			nsview_pane::reshape(newBounds, oldOrigin);
			m_isResizing = false;
		}
	}
};


//class nsview_subsystem::window_bridge : public pane_bridge<window>
//{
//private:
//	typedef pane_bridge<window> base_t;
//
//public:
//	nsview_subsystem::visible_windows_list_t::volatile_remove_token	m_visibleRemoveToken;
//	bool									m_isVisibleRootWindow;
//	composite_string						m_title;
//	int										m_style;
//	function<void()>						m_closeDelegate;
//
//	window_bridge(const composite_string& title, int style, const function<void()>& closeDelegate = function<void()>())
//		:	m_title(title),
//			m_isVisibleRootWindow(false),
//			m_style(style),
//			m_closeDelegate(closeDelegate)
//	{ }
//	
//	~window_bridge()
//	{
//		m_closeDelegate();
//	}
//
//	//virtual size propose_lengths(dimension d, const size& proposedSize) const
//	//{
//	//	return base_t::propose_lengths(d, proposedSize);
//	//}
//
//	//virtual double propose_length(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
//	//{
//	//	return base_t::propose_length(d, proposed, rtnOtherRange);
//	//}
//
//	//virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//	//{
//	//	base_t::reshape(b, oldOrigin);
//	//}
//
//	virtual void installing()
//	{
//		rcref<window> w = rcnew(window, m_title, m_style, get_subsystem().dereference().template static_cast_to<volatile nsview_subsystem>());
//		base_t::install_bridged(w);
//	}
//
//	virtual void hiding()
//	{
//		base_t::hiding();
//		if (m_isVisibleRootWindow)
//		{
//			rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile nsview_subsystem>();
//			uiSubsystem->remove_visible_window(m_visibleRemoveToken);
//			m_isVisibleRootWindow = false;
//		}
//	}
//
//	virtual void showing()
//	{
//		rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().template static_cast_to<volatile nsview_subsystem>();
//		m_visibleRemoveToken = uiSubsystem->add_visible_window(this_rcref);
//		m_isVisibleRootWindow = true;
//		base_t::showing();
//	}
//};




//void nsview_pane_base::installing(const rcref<pane>& owner, NSView* v)
//{
//	m_nsView = v;
//
//	m_owner = owner;
//	rcptr<pane> p = owner->get_parent();
//	while (!!p)
//	{
//		rcptr<pane_bridge> parentBridge = p.template dynamic_cast_to<pane_bridge>();
//		if (!!parentBridge)
//		{
//			m_parentView = parentBridge->get_bridged().template dynamic_cast_to<nsview_pane_base>();
//			if (!!m_parentView)
//			{
//				m_parentWindow = m_parentView->m_parentWindow;
//				if (!m_parentWindow)
//					m_parentWindow = m_parentView.template static_cast_to<window>();
//				break;
//			}
//		}
//		p = p->get_parent();
//	}
//
//	[m_nsView setAutoresizesSubviews:NO];
//	[m_nsView setPostsFrameChangedNotifications:YES];
//
//	[m_nsView setHidden:YES];
//
//	if (!!m_parentView)
//	{
//		NSView* parentView = m_parentView->m_nsView;
//		[parentView addSubview: m_nsView];
//	}
//
//	//owner->set_externally_drawn();
//}


inline std::pair<rcref<bridgeable_pane>, rcref<window_interface> > nsview_subsystem::create_window() volatile
{
	rcref<window> w = rcnew(window, this_rcref);
	return std::make_pair(w, w);
}


//inline rcref<gui::window> nsview_subsystem::open_window(
//	const composite_string& title,
//	const rcref<pane>& p,
//	const rcptr<frame>& f,
//	const function<bool()>& closeDelegate) volatile
//{
//	rcref<gui::window> w = rcnew(gui::window, title, closeDelegate);
//	w->nest(p, f);
//	install(*w, this_rcref);
//	return w;
//}
//

}
}
}




/// @internal
@implementation objc_window


-(void)keyDown:(NSEvent *)theEvent
{
	int key = (int)[[theEvent characters] characterAtIndex:0];
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->character_type(key);
}


-(void)keyUp:(NSEvent *)theEvent
{
	int key = (int)[[theEvent characters] characterAtIndex:0];
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		cppWindow->key_release(key);
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
		return cppWindow->reshaping(newSize);
}


-(void)windowDidResize:(NSNotification *)notification;
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
		return cppWindow->reshaping();
}

-(void)windowWillClose:(NSNotification *)notification;
{
	cogs::rcptr<cogs::gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{

	}
}

@end


#endif
