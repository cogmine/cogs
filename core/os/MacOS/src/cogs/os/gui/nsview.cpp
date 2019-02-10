//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifdef COGS_COMPILE_SOURCE

#include <stdlib.h>
#include "cogs/collections/composite_string.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/gui/subsystem.hpp"
#include "cogs/os/gui/nsview.hpp"
#include "cogs/os/gui/window.hpp"
#include "cogs/os/gui/button.hpp"
#include "cogs/os/gui/check_box.hpp"
#include "cogs/os/gui/text_editor.hpp"
#include "cogs/os/gui/scroll_bar.hpp"
#include "cogs/os/gfx/nsimage.hpp"
#include "cogs/sync/quit_dispatcher.hpp"

	
namespace cogs {
//namespace os {
namespace gui {
namespace os {

/// @internal
@interface defer_task_helper : NSObject
{
@public
	// nsview_subsystem will outlive the main threads ability to handle performSelectorOnMainThread
	volatile nsview_subsystem* m_subsystem;
}

-(void)defer;

@end

@implementation defer_task_helper

-(void)defer
{
	bool ranAny = false;
	bool done = false;
	while (!done)
	{
		bool b = atomic::compare_exchange(m_subsystem->m_dispatchMode, 1, 2);
		COGS_ASSERT(b);
		for (;;)
		{
			int priority;
			rcptr<cogs::dispatcher::dispatched> dsptchd = m_subsystem->m_controlQueue->peek(priority);
			if (!!dsptchd)
			{
				if (ranAny && (priority > 0x00010000))
				{
					atomic::store(m_subsystem->m_dispatchMode, 2);
					[self performSelectorOnMainThread : @selector(defer) withObject:nil waitUntilDone : NO];
					return;
				}

				if (!m_subsystem->m_controlQueue->remove_and_invoke(dsptchd.dereference()))
					continue;

				ranAny = true;
				continue;
			}

			done = atomic::compare_exchange(m_subsystem->m_dispatchMode, 0, 1);
			break;
		}
	}

	[self release];
}

@end


alignas (atomic::get_alignment_v<timeout_t::aligned_ratio_t>) placement<timeout_t::aligned_ratio_t> timeout_t::s_ratio;
//int semaphore::s_event = 0;
placement<rcptr<volatile nsview_subsystem> >	nsview_subsystem::s_nsViewSubsystem;


void nsview_subsystem::update() volatile
{
	int oldMode = 2;
	cogs::exchange(m_dispatchMode, oldMode, oldMode);
	if (!oldMode)
	{
		defer_task_helper* helper = [[defer_task_helper alloc] init];
		helper->m_subsystem = this_rcptr.const_cast_to<volatile nsview_subsystem>().get_ptr();
		[helper performSelectorOnMainThread : @selector(defer) withObject:nil waitUntilDone : NO];
	}
}

//rcptr<console> gui::subsystem::get_default_console() volatile
//{
//	return rcptr<console>();	// TBD
//}
//
//rcptr<console> gui::subsystem::create_console() volatile
//{
//	return rcptr<console>();	// TBD
//}

void gui::subsystem::message(const composite_string& s) volatile
{
	// TBD
}

void nsview_subsystem::message(const composite_string& s) volatile
{
	// TBD
}

void nsview_subsystem::open_window(const composite_string& title, const rcref<pane>& p, const rcptr<frame>& j, const function<void()>& closeDelegate) volatile
{
	int style = NSTitledWindowMask | NSClosableWindowMask;
	//if (minimizable)
		style |= NSMiniaturizableWindowMask;
	//if (resizable)
		style |= NSResizableWindowMask;

	rcref<window_bridge> w = rcnew(window_bridge, title, style, closeDelegate);
	w->nest(p, j);
	w->install(this_rcref);
}

//void nsview_subsystem::open_full_screen(const composite_string& title, const rcref<pane>& p, const rcptr<frame>& j, const function<void()>& closeDelegate) volatile
//{
//	// tbd 
//}


rcref<button_interface> nsview_subsystem::create_button() volatile
{
	return rcnew(gui::os::button, this_rcref);
}

rcref<check_box_interface> nsview_subsystem::create_check_box() volatile
{
	return rcnew(gui::os::check_box, this_rcref);
}

rcref<text_editor_interface> nsview_subsystem::create_text_editor() volatile
{
	return rcnew(gui::os::text_editor, this_rcref);
}

rcref<scroll_bar_interface> nsview_subsystem::create_scroll_bar() volatile
{
	return rcnew(gui::os::scroll_bar, this_rcref);
}


/// @internal
class nsview_canvas_pane : public nsview_pane<bridgeable_pane>
{
private:
	typedef nsview_pane<bridgeable_pane> base_t;

public:
	nsview_canvas_pane(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: base_t(uiSubsystem)
	{ }

	~nsview_canvas_pane()
	{
		if (!!m_nsView)
			((objc_view*)m_nsView)->m_cppView.release();
	}

	virtual void installing()
	{
		objc_view* nsView = [[objc_view alloc] init];
		nsView->m_cppView = this_rcptr;
		
		[nsView setAutoresizesSubviews:NO];
		[nsView setPostsFrameChangedNotifications:YES];
		
		base_t::installing(nsView);
	}
};


rcref<bridgeable_pane> nsview_subsystem::create_native_pane() volatile
{
	rcref<nsview_canvas_pane> p = rcnew(nsview_canvas_pane, this_rcref);
	p->m_isUserDrawn = true;
	return p;
}


int main(int argc, const char * argv[])
{
	return NSApplicationMain(argc, argv);
}


namespace cogs {
	int main();
}


static void do_cleanup()
{
	cogs::thread_pool::shutdown_default();

	cogs::gui::os::nsview_subsystem::shutdown();

	cogs::default_allocator::shutdown();
}


static void quitting()
{
	NSApplication* app = [NSApplication sharedApplication];
	[app performSelectorOnMainThread: @selector(terminate:) withObject:app waitUntilDone:NO];
}


/// @internal
@implementation AppDelegate

-(void)applicationDidFinishLaunching:(NSNotification *) notification
{
	main();

	rcref<volatile nsview_subsystem> nsViewSubsystem = nsview_subsystem::get_default();

	rcptr<const single_fire_event> quitEvent = quit_dispatcher::get()->get_event();
	if (!!quitEvent)
	{
		quitEvent->dispatch(&quitting);
	}
}

-(void)applicationWillTerminate:(NSNotification *)notification
{
	// in case someone is force-quitting us by sending us this.
	quit_dispatcher::get()->force();

	atexit(do_cleanup);
}


@end


/// @internal
@implementation objc_view

-(BOOL)isFlipped	{ return TRUE; }

-(void)drawRect:(NSRect)r
{
	[super drawRect:r];
	rcptr<gui::os::nsview_pane<gui::bridgeable_pane> > cppView = m_cppView;
	if (!!cppView)
	{
		rcptr<gui::pane> owner = cppView->get_bridge();
		if (!!owner)
			owner->draw();
	}
}

@end




/*

@implementation objc_view

-(void)setFrameSize:(NSSize)sz
{
	NSSize newNSSize = sz;
	rcptr<gui::os::nsview_pane<gui::bridgeable_pane> > cppView = m_cppView;
	if ((!!cppView) && (!cppView->m_isResizing))
	{
		rcptr<gui::pane> owner = cppView->get_bridge();
		if ((!!owner) && owner->is_installed())
		{
			size newSize((longest)sz.width, (longest)sz.height);
			cppView->m_isResizing = true;
			owner->pane::reshape(newSize);
			cppView->m_isResizing = false;
		}
	}
	[super setFrameSize:newNSSize];
}

-(void)setFrameOrigin:(NSPoint)pt
{
	[super setFrameOrigin:pt];
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if ((!!cppWindow) && (!cppWindow->m_reshaping))
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if ((!!owner) && owner->is_installed())
		{
			double x = (double)pt.x;
			double y = (double)pt.y;
			canvas::point newPt(x, y);
			m_cppWindow->m_isResizing = true;
			owner->pane::reshape(newPt, owner->get_size());
			m_cppWindow->m_isResizing = false;
		}
	}
}

-(void)setFrame:(NSRect)r
{
	[super setFrameOrigin:r.origin];
	[super setFrameSize:r.size];
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if ((!!cppWindow) && (!cppWindow->m_reshaping))
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if ((!!owner) && owner->is_installed())
		{
			double w = (double)r.size.width;
			double h = (double)r.size.height;
			canvas::size newSize(w, h);

			double x = (double)r.origin.x;
			double y = (double)r.origin.y;
			canvas::point newPt(x, y);
			m_cppWindow->m_isResizing = true;
			owner->pane::reshape(newPt, newSize);
			m_cppWindow->m_isResizing = false;
		}
	}
}

@end

*/



/// @internal
@implementation objc_window


-(void)keyDown:(NSEvent *)theEvent
{
	int key = (int)[[theEvent characters] characterAtIndex:0];
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if (!!owner && owner->key_pressing(key))
			owner->character_typing(key);
	}
}


-(void)keyUp:(NSEvent *)theEvent
{
	int key = (int)[[theEvent characters] characterAtIndex:0];
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if (!!owner)
			owner->key_releasing(key);
	}
}


-(void)close
{
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!cppWindow)
		[super close];
	else
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if (!!owner)
			owner->hide();
	}
}


-(void)becomeKeyWindow
{
	[super becomeKeyWindow];

	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if (!!owner)
			owner->focus(0);
	}
}


-(void)resignKeyWindow
{
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if (!!owner)
			owner->defocus();
	}
	[super resignKeyWindow];
}


- (NSSize)windowWillResize:(NSWindow *) window toSize:(NSSize)newSize
{
	NSRect r;
	r.size = newSize;
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if ((!!cppWindow) && (!cppWindow->m_isResizing))
	{
		rcptr<pane> owner = cppWindow->get_bridge();
		if ((!!owner) && owner->is_installed())
		{
			NSRect currentFrameRect = [self frame];
			NSRect currentContentRect = [NSWindow contentRectForFrameRect:currentFrameRect styleMask:[self styleMask]];

			cppWindow->m_preSizingBounds.get_position().get_x() = currentContentRect.origin.x;
			cppWindow->m_preSizingBounds.get_position().get_y() = currentContentRect.origin.y;
			cppWindow->m_preSizingBounds.get_size().get_width() = currentContentRect.size.width;
			cppWindow->m_preSizingBounds.get_size().get_height() = currentContentRect.size.height;

			r.origin.x = r.origin.y = 0;
			NSRect frame = [NSWindow contentRectForFrameRect:r styleMask:[self styleMask]];

			size newSize2(frame.size.width, frame.size.height);
			newSize2 = owner->get_cell().get_range().limit(newSize2);
			newSize2 = owner->propose_size(newSize2);
			r.size.width = newSize2.get_width().get_int<longest>();
			r.size.height = newSize2.get_height().get_int<longest>();

			r = [NSWindow frameRectForContentRect:r styleMask:[self styleMask]];
		}
	}
	return r.size;
}


- (void)windowDidResize:(NSNotification *)notification;
{
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if ((!!cppWindow) && (!cppWindow->m_isResizing))
	{
		rcptr<gui::pane> owner = cppWindow->get_bridge();
		if ((!!owner) && owner->is_installed())
		{
			NSRect r = [self frame];
			NSRect r2 = [NSWindow contentRectForFrameRect:r styleMask:[self styleMask]];

			bounds newBounds;
			newBounds.get_position().get_x() = r2.origin.x;
			newBounds.get_position().get_y() = r2.origin.y;
			newBounds.get_size().get_width() = r2.size.width;
			newBounds.get_size().get_height() = r2.size.height;

			point oldOrigin(0, 0);
			if (!!cppWindow->m_initialReshapeDone)
			{
				bounds oldBounds = cppWindow->m_preSizingBounds;

				// Flip Y, since screen coords are Cartesian.
				oldBounds.get_position().get_y() += oldBounds.get_size().get_height();
				newBounds.get_position().get_y() += newBounds.get_size().get_height();
			
				oldOrigin.get_x() = oldBounds.get_position().get_x();
				oldOrigin.get_x() -= newBounds.get_position().get_x();

				oldOrigin.get_y() = newBounds.get_position().get_y();
				oldOrigin.get_y() -= oldBounds.get_position().get_y();
			}
			else
				cppWindow->m_initialReshapeDone = true;

			cppWindow->m_isResizing = true;
			owner->pane::reshape(newBounds, oldOrigin);
			cppWindow->m_isResizing = false;
		}
	}
}

-(void)windowWillClose:(NSNotification *)notification;
{
	rcptr<gui::os::window> cppWindow = m_cppWindow;
	if (!!cppWindow)
	{

	}
}

@end


/// @internal
@implementation objc_text_editor


-(BOOL)acceptsFirstResponder
{
	BOOL result = YES;
	rcptr<gui::os::text_editor> cppTextEditor = m_cppTextEditor;
	if (!!cppTextEditor)
	{
		rcptr<pane> owner = cppTextEditor->get_bridge();
		if (!!owner && owner->is_focusable())
			result = NO;
	}
	return result;
}

-(BOOL)becomeFirstResponder
{
	rcptr<gui::os::text_editor> cppTextEditor = m_cppTextEditor;
	if (!!cppTextEditor)
	{
		rcptr<pane> owner = cppTextEditor->get_bridge();
		if (!!owner)
			owner->focus(0);
	}
	return [super becomeFirstResponder];
}

@end

	/*
- (BOOL)control: (NSControl *)control textView:(NSTextView *)textView doCommandBySelector: (SEL)commandSelector
{
	BOOL result = NO;
	rcptr<gui::os::text_editor> cppTextEditor = m_cppTextEditor;
	if (!!cppTextEditor)
	{
	//	rcptr<pane> owner = cppTextEditor->get_bridge();
	//	if (!!owner && owner->is_focusable())

//		if (commandSelector == @selector(insertNewline:))
//		{
///			// if multiLine, always insert a line-break character and don’t cause the receiver to end editing
//			//[textView insertNewlineIgnoringFieldEditor:self];
//			cppTextEditor->click_ok();
//			result = YES;
//		}
//		else if (commandSelector == @selector(cancelOperation:))
//		{
//			// if multiLine, always insert a line-break character and don’t cause the receiver to end editing
//			//[textView insertNewlineIgnoringFieldEditor:self];
//			cppTextEditor->click_cancel();
//			result = YES;
//		}
//		else if (commandSelector == @selector(insertTab:))
//		{
//			// If allowTabls, always insert a tab character and don’t cause the receiver to end editing
//			//[textView insertTabIgnoringFieldEditor:self];
//			cppTextEditor->focus_next(true);
//			result = YES;
//		}
//		else if (commandSelector == @selector(insertBacktab:))
//		{
//			cppTextEditor->focus_next(false);
//			result = YES;
//		}
	}
	return result;
}
*/


/// @internal
@implementation objc_button

-(void)on_click:(id)sender
{
	rcptr<gui::os::button> cppButton = m_cppButton;
	if (!!cppButton)
		cppButton->action();
}

@end

/// @internal
@implementation objc_check_box
@end


/// @internal
@implementation objc_scroll_bar

-(BOOL)isFlipped	{ return TRUE; }

-(void)scrolled:(id)sender;
{
	rcptr<gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->scrolled();
}

@end


}
}
}

#endif
