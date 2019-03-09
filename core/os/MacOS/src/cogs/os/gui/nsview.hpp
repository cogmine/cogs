//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_GUI_NSVIEW
#define COGS_HEADER_OS_GUI_NSVIEW


#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/gui/button.hpp"
#include "cogs/gui/check_box.hpp"
#include "cogs/gui/text_editor.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/collections/macos_strings.hpp"
#include "cogs/os/gfx/graphics_context.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/singleton.hpp"


namespace cogs {
namespace gui {
namespace os {


class window;
class nsview_subsystem;
class nsview_pane_base;


template <class pane_t>
class nsview_pane;

}
}
}


@interface objc_view : NSView
{
@public
	cogs::weak_rcptr< cogs::gui::os::nsview_pane< cogs::gui::bridgeable_pane> > m_cppView;
}

-(BOOL)isFlipped;
-(void)drawRect:(NSRect)r;

@end


@interface AppDelegate : NSObject <NSApplicationDelegate>
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification;
-(void)applicationWillTerminate:(NSNotification *)notification;


@end


namespace cogs {
namespace gui {
namespace os {


class nsview_subsystem : public gui::windowing::subsystem
{
public:
	typedef container_dlist<rcref<gui::window> > visible_windows_list_t;

	class control_queue_t : public priority_dispatcher
	{
	public:
		control_queue_t(const ptr<rc_obj_base>& desc)
			: object(desc)
		{ }

		~control_queue_t() { drain(); }

		bool drain() volatile
		{
			bool foundAny = false;
			while (invoke())
				foundAny = true;
			return foundAny;
		}
	};

	rcptr<control_queue_t> m_controlQueue;
	alignas (cogs::atomic::get_alignment_v<int>) volatile int m_dispatchMode;	// 0 = idle, 1 = running, 2 = refresh

private:
	visible_windows_list_t m_visibleWindows;
	volatile rcref<task<void> > m_cleanupRemoveToken;

	void cleanup() volatile
	{
		// Visible windows keep the subsystem in scope.  In case windows are left open
		// until the app closes, use a cleanup function to ensure they get closed.
		m_cleanupInstalled = false;
		m_visibleWindows.drain();
	}

	void ensure_cleanup_is_installed()
	{
		if (m_cleanupInstalled.compare_exchange(true, false))
		{
			m_cleanupRemoveToken = cleanup_queue::get_global()->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<nsview_subsystem> r2 = r;
				if (!!r2)
					r2->cleanup();
			});
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		ensure_cleanup_is_installed();

		rcptr<control_queue_t> controlQueue = m_controlQueue;
		dispatcher::dispatch_inner(*m_controlQueue, t, priority);
		update();
	}

protected:
	nsview_subsystem(const ptr<rc_obj_base>& desc)
		: gui::windowing::subsystem(desc),
		m_dispatchMode(0),
		m_controlQueue(rcnew(control_queue_t)),
		m_cleanupInstalled(false)
	{ }

public:
	~nsview_subsystem()
	{
		m_cleanupRemoveToken->cancel();

		if (!m_controlQueue->is_empty())
		{
			// In case destructed off the main thread,
			// deferring to cleanup queue ensures these get called in the main (UI) thread
			cleanup_queue::get_global()->dispatch([controlQueueRef = m_controlQueue]()
			{
				controlQueueRef->drain();
			});
		}
	}

	static rcref<nsview_subsystem> get_global()
	{
		return singleton<nsview_subsystem>::get();
	}

	void update() volatile;

	virtual bool is_ui_thread_current() const volatile	{ return [NSThread isMainThread]; }

	void remove_visible_window(visible_windows_list_t::volatile_remove_token& removeToken) volatile
	{
		m_visibleWindows->remove(removeToken);
	}

	visible_windows_list_t::volatile_remove_token add_visible_window(const rcref<window_bridge>& windowBridge) volatile
	{
		visible_windows_list_t::volatile_remove_token result = m_visibleWindows->prepend(windowBridge);
		ensure_cleanup_is_installed();
		return result;
	}

	// ui::subsystem interface
	virtual std::pair<rcref<bridgeable_pane>, rcref<button_interface> > create_button() volatile;
	virtual std::pair<rcref<bridgeable_pane>, rcref<check_box_interface> > create_check_box() volatile;
	virtual std::pair<rcref<bridgeable_pane>, rcref<text_editor_interface> > create_text_editor() volatile;
	virtual std::pair<rcref<bridgeable_pane>, rcref<scroll_bar_interface> > create_scroll_bar() volatile;
	virtual rcref<bridgeable_pane> create_native_pane() volatile;
	virtual std::pair<rcref<bridgeable_pane>, rcref<window_interface> > create_window() volatile;

	virtual rcref<task<void> > message(const composite_string& s) volatile;
	virtual rcref<gui::window> open_window(const composite_string& title, const rcref<pane>& p, const rcptr<frame>& f = 0, const function<void()>& closeDelegate = function<void()>()) volatile;
};


class nsview_pane : public object, public bridgeable_pane, public pane_orchestrator//, public gfx::os::graphics_context
{
public:
	weak_rcptr<pane>					m_owner;
	rcref<volatile nsview_subsystem>	m_uiSubsystem;

	rcptr<nsview_pane_base>				m_parentView;
	rcptr<window>						m_parentWindow;

	bounds								m_boundsInWindow;

	NSView*								m_nsView;

	bool								m_focusing;
	bool								m_isResizing;
	bool								m_isUserDrawn;	// i.e. window or canvas hwnd

	const rcptr<nsview_pane_base>&	get_parent_view() const			{ return m_parentView; }
	const rcptr<window>&			get_parent_window() const		{ return m_parentWindow; }
	const bounds&		get_bounds_in_window() const	{ return m_boundsInWindow; }

	      rcref<volatile nsview_subsystem>& get_subsystem()			{ return m_uiSubsystem; }
	const rcref<volatile nsview_subsystem>& get_subsystem() const	{ return m_uiSubsystem; }

	nsview_pane_base(const rcref<volatile nsview_subsystem>& uiSubsystem)
		:	m_uiSubsystem(uiSubsystem),
			m_nsView(0),
			m_focusing(false),
			m_boundsInWindow(point(0,0), size(0,0)),
			m_isResizing(false),
			m_isUserDrawn(false)
	{ }

	~nsview_pane_base()
	{
		if (!!m_nsView)
			[m_nsView release];
	}

	void installing(const rcref<pane>& owner, NSView* v);

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		m_boundsInWindow.set_size(r.get_size());
		rcptr<pane> owner = m_owner;
		if (!!owner)
		{
			if (!!m_parentView)	// if not a window
			{
				bounds boundsInParentView = r;
				rcptr<pane> p = owner->get_parent();	// find new coords in parent nsview
				for (;;)
				{
					if (!p || (p == m_parentView->m_owner))
						break;
					boundsInParentView += p->get_position();
					p = p->get_parent();
				}
				m_boundsInWindow = boundsInParentView;
				m_boundsInWindow.get_position() += m_parentView->m_boundsInWindow.get_position();

				if (!m_isResizing)
				{
					NSRect newRect = make_NSRect(boundsInParentView);
					[m_nsView setFrame: newRect];
				}
			}
		}
	}

	virtual void hiding()
	{
		[m_nsView setHidden:YES];
	}

	virtual void showing()
	{
		[m_nsView setHidden:NO];
		[m_nsView setNeedsDisplay:YES];
	}

	virtual void focusing(int direction)
	{
		if (!m_focusing)
			[[m_nsView window] makeFirstResponder: m_nsView];
	}

	virtual void invalidating(const bounds& r)
	{
		NSRect r2 = make_NSRect(r);
		[m_nsView setNeedsDisplayInRect:r2];
	}

	virtual void fill(const bounds& r, const color& c = color::black, bool blendAlpha = true)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->fill(r, c, blendAlpha);
		else
		{
			[m_nsView lockFocus];
			graphics_context::fill(r, c, blendAlpha);
			[m_nsView unlockFocus];
		}
	}

	virtual void invert(const bounds& r)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->invert(r);
		else
		{
			[m_nsView lockFocus];
			graphics_context::invert(r);
			[m_nsView unlockFocus];
		}
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->draw_line(startPt, endPt, width, c, blendAlpha);
		else
		{
			[m_nsView lockFocus];
			graphics_context::draw_line(startPt, endPt, width, c, blendAlpha);
			[m_nsView unlockFocus];
		}
	}

	virtual void scroll(const bounds& r, const point& pt = point(0,0))
	{
		if (pt == r.get_position())
			return;

		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->scroll(r, pt);
		else
		{
			NSRect r2 = make_NSRect(r);
			point offset = r.get_position();
			offset -= pt;
			NSSize offset2;
			offset2.width = offset.get_x().to_int<longest>();
			offset2.height = offset.get_y().to_int<longest>();
			[m_nsView lockFocus];
			[m_nsView scrollRect:r2 by:offset2];
			[m_nsView unlockFocus];
		}
	}

	virtual rcref<canvas::font> load_font(const gfx::font& f)
	{
		return graphics_context::load_font(f);
	}
	
	virtual size calc_text_bounds(const composite_string& s, const rcptr<canvas::font>& f)
	{
		return graphics_context::calc_text_bounds(s, f);
	}

	virtual void draw_text(const composite_string& s, const bounds& r, const rcptr<canvas::font>& f, const color& c = color::black, bool blendAlpha = true)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->draw_text(s, r, f, c, blendAlpha);
		else
		{
			[m_nsView lockFocus];
			graphics_context::draw_text(s, r, f, c, blendAlpha);
			[m_nsView unlockFocus];
		}
	}

	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->composite_pixel_image(img, srcBounds, dstPt, blendAlpha);
		else
		{
			[m_nsView lockFocus];
			graphics_context::composite_pixel_image(img, srcBounds, dstPt, blendAlpha);
			[m_nsView unlockFocus];
		}
	}

	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->composite_scaled_pixel_image(img, srcBounds, dstBounds);
		else
		{
			[m_nsView lockFocus];
			graphics_context::composite_scaled_pixel_image(img, srcBounds, dstBounds);
			[m_nsView unlockFocus];
		}
	}


	//virtual void composite_scaled_pixel_image(const pixel_image& img, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
	//{
	//	rcptr<pane> owner = m_owner;
	//	rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
	//	if (!!offScreenBuffer)
	//		offScreenBuffer->composite_scaled_pixel_image(img, srcBounds, dstBounds, blendAlpha);
	//	else
	//	{
	//		[m_nsView lockFocus];
	//		graphics_context::composite_scaled_pixel_image(img, srcBounds, dstBounds, blendAlpha);
	//		[m_nsView unlockFocus];
	//	}
	//}

	virtual void composite_pixel_mask(const pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->composite_pixel_mask(src, srcBounds, dstPt, fore, back, blendForeAlpha, blendBackAlpha);
		else
		{
			[m_nsView lockFocus];
			graphics_context::composite_pixel_mask(src, srcBounds, dstPt, fore, back, blendForeAlpha, blendBackAlpha);
			[m_nsView unlockFocus];
		}
	}

	virtual rcref<pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi)
	{
		return graphics_context::create_pixel_image_canvas(sz, isOpaque, dpi);
	}

	virtual rcref<pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)
	{
		return graphics_context::load_pixel_image(location, dpi);
	}

	virtual rcref<pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi)
	{
		return graphics_context::load_pixel_mask(location, dpi);
	}

	virtual void save_clip()
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->save_clip();
		else
		{
			[m_nsView lockFocus];
			graphics_context::save_clip();
			[m_nsView unlockFocus];
		}
	}

	virtual void restore_clip()
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->restore_clip();
		else
		{
			[m_nsView lockFocus];
			graphics_context::restore_clip();
			[m_nsView unlockFocus];
		}
	}

	virtual void clip_out(const bounds& r)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->clip_out(r);
		else
		{
			[m_nsView lockFocus];
			graphics_context::clip_out(r);
			[m_nsView unlockFocus];
		}
	}

	virtual void clip_to(const bounds& r)
	{
		rcptr<pane> owner = m_owner;
		rcptr<pixel_image_canvas> offScreenBuffer = owner->get_offscreen_buffer();
		if (!!offScreenBuffer)
			offScreenBuffer->clip_to(r);
		else
		{
			[m_nsView lockFocus];
			graphics_context::clip_to(r);
			[m_nsView unlockFocus];
		}
	}

	virtual bool is_unclipped(const bounds& r) const
	{
		if (!r.get_height() || !r.get_width())
			return false;

		// TBD - unclipped checking when using an offscreen buffer?

		NSRect r2 = make_NSRect(r);
		return [m_nsView needsToDrawRect: r2];
	}
};


//template <class pane_t>
//class nsview_pane : public nsview_pane_base, public pane_t
//{
//public:
//	nsview_pane(const rcref<volatile nsview_subsystem>& uiSubsystem)
//		:	nsview_pane_base(uiSubsystem)
//	{ }
//	
//	virtual void installing() = 0;
//
//	void installing(NSView* nsView)
//	{
//		rcptr<pane> owner = pane_t::get_bridge();
//		nsview_pane_base::installing(owner.dereference(), nsView);
//		pane_t::installing();
//	}
//
//	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
//	{
//		nsview_pane_base::reshape(r, oldOrigin);
//		pane_t::reshape(r, oldOrigin);
//	}
//
//	virtual void hiding()
//	{
//		nsview_pane_base::hiding();
//		pane_t::hiding();
//	}
//
//	virtual void showing()
//	{
//		nsview_pane_base::showing();
//		pane_t::showing();
//	}
//
//	virtual void focusing(int direction)
//	{
//		nsview_pane_base::focusing(direction);
//		pane_t::focusing(direction);
//	}
//
//	virtual void invalidating(const bounds& r)
//	{
//		nsview_pane_base::invalidating(r);
//		pane_t::invalidating(r); 
//	}
//
//	virtual void fill(const bounds& r, const color& c = color::black, bool blendAlpha = true)
//	{ nsview_pane_base::fill(r, c, blendAlpha); }
//
//	virtual void invert(const bounds& r)
//	{ nsview_pane_base::invert(r); }
//
//	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
//	{ nsview_pane_base::draw_line(startPt, endPt, width, c, blendAlpha); }
//
//	virtual void scroll(const bounds& r, const point& pt = point(0,0))
//	{ nsview_pane_base::scroll(r, pt); }
//
//	virtual rcref<canvas::font> load_font(const gfx::font& f)
//	{ return nsview_pane_base::load_font(f); }
//
//	virtual size calc_text_bounds(const composite_string& s, const rcptr<canvas::font>& f)
//	{ return nsview_pane_base::calc_text_bounds(s, f); }
//
//	virtual void draw_text(const composite_string& s, const bounds& r, const rcptr<canvas::font>& f, const color& c = color::black, bool blendAlpha = true)
//	{ nsview_pane_base::draw_text(s, r, f, c, blendAlpha); }
//
//	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true)
//	{ nsview_pane_base::composite_pixel_image(img, srcBounds, dstPt, blendAlpha); }
//
//	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds)
//	{ nsview_pane_base::composite_scaled_pixel_image(img, srcBounds, dstBounds); }
//
//	//virtual void composite_scaled_pixel_image(const pixel_image& img, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
//	//{ nsview_pane_base::composite_scaled_pixel_image(img, srcBounds, dstBounds, blendAlpha); }
//
//	virtual void composite_pixel_mask(const pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
//	{ nsview_pane_base::composite_pixel_mask(src, srcBounds, dstPt, fore, back, blendForeAlpha, blendBackAlpha); }
//
//	virtual rcref<pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi)
//	{ return nsview_pane_base::create_pixel_image_canvas(sz, isOpaque, dpi); }
//
//	virtual rcref<pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)			{ return nsview_pane_base::load_pixel_image(location, dpi); }
//	virtual rcref<pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi)			{ return nsview_pane_base::load_pixel_mask(location, dpi); }
//
//	virtual void save_clip()										{ nsview_pane_base::save_clip(); }
//	virtual void restore_clip()										{ nsview_pane_base::restore_clip(); }
//	virtual void clip_out(const bounds& r)							{ nsview_pane_base::clip_out(r); }
//	virtual void clip_to(const bounds& r)							{ nsview_pane_base::clip_to(r); }
//	virtual bool is_unclipped(const bounds& r) const				{ return nsview_pane_base::is_unclipped(r); }
//};
//

}
}
}


#endif
