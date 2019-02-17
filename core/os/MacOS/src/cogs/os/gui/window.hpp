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
	cogs::weak_rcptr< cogs::gui::os::window> m_cppWindow;
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


class window : public nsview_pane<bridgeable_pane>
{
private:
	typedef nsview_pane<bridgeable_pane> base_t;

public:
	NSWindow*		m_nsWindow;
	int				m_style;
	composite_string			m_title;
	bounds		m_preSizingBounds;
	bool			m_initialReshapeDone;

	window(const composite_string& title, int style, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: base_t(uiSubsystem),
			m_title(title),
			m_style(style),
			m_initialReshapeDone(false)
	{
		m_isUserDrawn = true;
	}

	~window()
	{
		((objc_window*)m_nsWindow)->m_cppWindow.release();
		((objc_view*)m_nsView)->m_cppView.release();
		[m_nsWindow release];
	}

	virtual void installing()
	{
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
		objc_window* objcWindow = [[objc_window alloc] initWithContentRect:windowBounds
											  styleMask:m_style
												backing:NSBackingStoreBuffered
												  defer:NO ];
		m_nsWindow = objcWindow;
		objcWindow->m_cppWindow = this_rcptr;
		[objcWindow setDelegate:objcWindow];

		NSString* str = string_to_NSString(m_title);
		[objcWindow setTitle: str];
		[str release];

		[objcWindow setReleasedWhenClosed:NO];
		[objcWindow setContentView:nsView];

		base_t::installing(nsView);

//		[objcWindow makeKeyAndOrderFront: nil];
	}
	
	virtual void calculate_range()
	{
		base_t::calculate_range();

		rcptr<pane> owner = get_bridge();
		bool resizable = !owner->get_range().is_fixed();
		if (resizable)
			[m_nsWindow setShowsResizeIndicator:YES];
		else
			[m_nsWindow setShowsResizeIndicator:NO];
	}

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		NSSize newSize = make_NSSize(r.get_size());
		[m_nsWindow setContentSize: newSize];
//		[m_nsView setFrameSize: newSize];
		[m_nsView setNeedsDisplay:YES];
	}
	
	virtual void hiding()
	{
		if (!!m_nsWindow)
			[m_nsWindow orderOut:nil];
		base_t::hiding();
	}

	virtual void showing()
	{
		base_t::showing();
		if (!!m_nsWindow)
			[m_nsWindow makeKeyAndOrderFront:nil];
	}
};


class nsview_subsystem::window_bridge : public pane_bridge<window>
{
private:
	typedef pane_bridge<window> base_t;

public:
	nsview_subsystem::visible_windows_list_t::volatile_remove_token	m_visibleRemoveToken;
	bool									m_isVisibleRootWindow;
	composite_string									m_title;
	int										m_style;
	function<void()>						m_closeDelegate;

	window_bridge(const composite_string& title, int style, const function<void()>& closeDelegate = function<void()>())
		:	m_title(title),
			m_isVisibleRootWindow(false),
			m_style(style),
			m_closeDelegate(closeDelegate)
	{ }
	
	~window_bridge()
	{
		m_closeDelegate();
	}

	//virtual size propose_lengths(dimension d, const size& proposedSize) const
	//{
	//	return base_t::propose_lengths(d, proposedSize);
	//}

	//virtual double propose_length(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
	//{
	//	return base_t::propose_length(d, proposed, rtnOtherRange);
	//}

	//virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	//{
	//	base_t::reshape(r, oldOrigin);
	//}

	virtual void installing()
	{
		rcref<window> w = rcnew(window, m_title, m_style, get_subsystem().dereference().static_cast_to<volatile nsview_subsystem>());
		base_t::install_bridged(w);
	}

	virtual void hiding()
	{
		base_t::hiding();
		if (m_isVisibleRootWindow)
		{
			rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().static_cast_to<volatile nsview_subsystem>();
			uiSubsystem->remove_visible_window(m_visibleRemoveToken);
			m_isVisibleRootWindow = false;
		}
	}

	virtual void showing()
	{
		rcptr<volatile nsview_subsystem> uiSubsystem = get_subsystem().static_cast_to<volatile nsview_subsystem>();
		m_visibleRemoveToken = uiSubsystem->add_visible_window(this_rcref);
		m_isVisibleRootWindow = true;
		base_t::showing();
	}
};




void nsview_pane_base::installing(const rcref<pane>& owner, NSView* v)
{
	m_nsView = v;

	m_owner = owner;
	rcptr<pane> p = owner->get_parent();
	while (!!p)
	{
		rcptr<pane_bridge> parentBridge = p.dynamic_cast_to<pane_bridge>();
		if (!!parentBridge)
		{
			m_parentView = parentBridge->get_bridged().dynamic_cast_to<nsview_pane_base>();
			if (!!m_parentView)
			{
				m_parentWindow = m_parentView->m_parentWindow;
				if (!m_parentWindow)
					m_parentWindow = m_parentView.static_cast_to<window>();
				break;
			}
		}
		p = p->get_parent();
	}

	[m_nsView setAutoresizesSubviews:NO];
	[m_nsView setPostsFrameChangedNotifications:YES];

	[m_nsView setHidden:YES];

	if (!!m_parentView)
	{
		NSView* parentView = m_parentView->m_nsView;
		[parentView addSubview: m_nsView];
	}

	//owner->set_externally_drawn();
}


}
}
}


#endif
