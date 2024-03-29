//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_GUI_NSVIEW
#define COGS_HEADER_OS_GUI_NSVIEW


#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
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
namespace os {


class window;
class nsview_subsystem;
class nsview_pane;


}
}


@interface objc_view : NSView
{
@public
	cogs::weak_rcptr<cogs::os::nsview_pane> m_cppView;
}
-(BOOL)isFlipped;
-(void)drawRect:(NSRect)r;

@end


namespace cogs {
namespace os {

class nsview_subsystem : public gui::windowing::subsystem
{
public:
	typedef container_dlist<rcref<gui::window> > visible_windows_list_t;

private:
	class control_queue_t : public priority_dispatcher
	{
	public:
		bool drain() volatile
		{
			bool foundAny = false;
			while (invoke())
				foundAny = true;
			return foundAny;
		}
	};

	volatile rcptr<volatile visible_windows_list_t> m_visibleWindows;
	rcptr<task<void> > m_cleanupRemoveToken;
	const rcref<volatile control_queue_t> m_controlQueue;
	int m_dispatchMode alignas(cogs::atomic::get_alignment_v<int>); // 0 = idle, 1 = running, 2 = refresh

	// Cast away volaility for access to the const rcref
	const rcref<volatile control_queue_t>& get_control_queue() const volatile
	{
		return const_cast<const nsview_subsystem*>(this)->m_controlQueue;
	}

	void run_in_main_queue() volatile
	{
		bool ranAny = false;
		for (;;)
		{
			cogs::atomic::compare_exchange(m_dispatchMode, 1, 2);
			int priority;
			rcptr<task<void> > t = get_control_queue()->peek(priority);
			if (!t)
			{
				if (cogs::atomic::compare_exchange(m_dispatchMode, 0, 1))
					break;
				//continue;
			}
			else if (ranAny && (priority > 0x00010000))
			{
				cogs::atomic::store(m_dispatchMode, 2);
				volatile nsview_subsystem* thisPtr = this;
				dispatch_async(dispatch_get_main_queue(), ^{
					thisPtr->run_in_main_queue();
				});
				break;
			}
			else
				ranAny |= get_control_queue()->remove_and_invoke(t.dereference());
			//continue;
		}
	}

	void update() volatile
	{
		int oldMode = 2;
		cogs::atomic::exchange(m_dispatchMode, oldMode, oldMode);
		if (!oldMode)
		{
			volatile nsview_subsystem* thisPtr = this;
			dispatch_async(dispatch_get_main_queue(), ^{
				thisPtr->run_in_main_queue();
			});
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		dispatcher::dispatch_inner(*get_control_queue(), t, priority);
		update();
	}

public:
	nsview_subsystem()
		: m_visibleWindows(rcnew(visible_windows_list_t)),
		m_cleanupRemoveToken(cleanup_queue::get()->dispatch([r{ this_weak_rcptr }]()
		{
			rcptr<nsview_subsystem> r2 = r;
			if (!!r2)
				r2->get_control_queue()->drain();
		})),
		m_controlQueue(rcnew(control_queue_t)),
		m_dispatchMode(0)
	{ }

	~nsview_subsystem()
	{
		m_cleanupRemoveToken->cancel();
	}

	static rcref<nsview_subsystem> get()
	{
		return singleton<nsview_subsystem>::get();
	}

	virtual bool is_ui_thread_current() const volatile { return [NSThread isMainThread]; }

	void remove_visible_window(visible_windows_list_t::volatile_remove_token& removeToken) volatile
	{
		rcptr<volatile visible_windows_list_t> visibleWindows = m_visibleWindows;
		if (!!visibleWindows)
			visibleWindows->remove(removeToken);
	}

	visible_windows_list_t::volatile_remove_token add_visible_window(const rcref<gui::window>& windowBridge) volatile
	{
		rcptr<volatile visible_windows_list_t> visibleWindows = m_visibleWindows;
		if (!!visibleWindows)
			return visibleWindows->prepend(windowBridge).inserted;
		return visible_windows_list_t::volatile_remove_token();
	}

	// ui::subsystem interface
	virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::button_interface> > create_button() volatile;
	virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::check_box_interface> > create_check_box() volatile;
	virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::text_editor_interface> > create_text_editor() volatile;
	virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::scroll_bar_interface> > create_scroll_bar() volatile;
	virtual rcref<gui::bridgeable_pane> create_native_pane() volatile;
	virtual std::pair<rcref<gui::bridgeable_pane>, rcref<gui::window_interface> > create_window() volatile;

	virtual rcref<task<void> > message(const composite_string& s) volatile
	{
		// TODO: Update to use button_box, allow async
		__strong NSString* str = string_to_NSString(s);
		__strong NSAlert* alert = [[NSAlert alloc]init];
		[alert addButtonWithTitle:@"OK"];
		[alert setInformativeText:str];
		[alert runModal];
		return signaled();
	}

	virtual vector<gfx::bounds> get_screens() volatile
	{
		vector<gfx::bounds> screens;
		CGDirectDisplayID displayIDs[50]; // 50 monitors is enough
		uint32_t numDisplays = 0;
		CGGetActiveDisplayList(std::extent_v<decltype(displayIDs)>, displayIDs, &numDisplays);
		for (size_t i = 0; i < numDisplays; i++)
		{
			// Filter out secondary (software) mirror displays.
			if (CGDisplayMirrorsDisplay(displayIDs[i]) != kCGNullDirectDisplay)
				continue;
			CGRect r = CGDisplayBounds(displayIDs[i]);
			screens.append(1, { { (double)r.origin.x, (double)r.origin.y }, { (double)r.size.width, (double)r.size.height } });
		}
		return screens;
	}

};


class nsview_pane : public object, public gui::bridgeable_pane, public gui::pane_orchestrator, public gfx::canvas, public graphics_context
{
private:
	rcref<volatile nsview_subsystem> m_uiSubsystem;
	__strong NSView* m_nsView = nullptr;
	rcptr<nsview_pane> m_parentView;
	weak_rcptr<nsview_pane> m_nextSiblingView;
	weak_rcptr<nsview_pane> m_prevSiblingView;
	weak_rcptr<nsview_pane> m_firstChildView;
	weak_rcptr<nsview_pane> m_lastChildView;
	size_t m_childCount = 0;

protected:
	bool m_isResizing = false;

	rcref<volatile nsview_subsystem>& get_subsystem() { return m_uiSubsystem; }
	const rcref<volatile nsview_subsystem>& get_subsystem() const { return m_uiSubsystem; }

public:
	explicit nsview_pane(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: m_uiSubsystem(uiSubsystem)
	{ }

	NSView* get_NSView() const
	{
		return m_nsView;
	}

	void draw()
	{
		bridgeable_pane::draw();
	}

	virtual void uninstalling()
	{
		if (!!m_parentView)
		{
			m_parentView->m_childCount--;
			rcptr<nsview_pane> prevSibling = m_prevSiblingView;
			rcptr<nsview_pane> nextSibling = m_nextSiblingView;
			if (!!prevSibling)
				prevSibling->m_nextSiblingView = nextSibling;
			if (!!nextSibling)
				nextSibling->m_prevSiblingView = prevSibling;

			if (m_parentView->m_firstChildView == this)
				m_parentView->m_firstChildView = nextSibling;
			if (m_parentView->m_lastChildView == this)
				m_parentView->m_lastChildView = prevSibling;
		}
		m_nsView = nullptr;
		bridgeable_pane::uninstalling();
	}

	void install_NSView(NSView* v)
	{
		m_nsView = v;

		rcptr<gui::pane> p = get_parent();
		while (!!p)
		{
			gui::pane_bridge* bridge = p.template dynamic_cast_to<gui::pane_bridge>().get_ptr();
			if (!bridge)
			{
				p = p->get_parent();
				continue;
			}
			m_parentView = get_bridged(*bridge).template dynamic_cast_to<nsview_pane>();
			if (!m_parentView)
			{
				p = p->get_parent();
				continue;
			}

			// Find previous z-order NSView by scanning descendant tree of m_parentView until we find this object.
			// Whenever another NSView is covered, do not traverse its descendants.
			// Once we find this object, insert it after the last NSView seen.

			container_dlist<rcref<gui::pane> >::iterator itor;
			rcptr<nsview_pane> lastFoundView;
			if (m_parentView->m_childCount++ > 0) // if was 0
			{
				itor = get_bridge(*m_parentView)->get_children().get_first();
				COGS_ASSERT(!!itor); // should at least find this obj
				container_dlist<rcref<gui::pane> >::iterator nextItor;
				// start at furthest last
				for (;;)
				{
					for (;;)
					{
						if (*itor == get_bridge())
						{
							itor.release();
							break;
						}

						rcptr<nsview_pane> foundView;
						bridge = itor->dynamic_cast_to<gui::pane_bridge>().get_ptr();
						if (!!bridge)
							foundView = get_bridged(*bridge).template dynamic_cast_to<nsview_pane>();

						if (!!foundView) // Don't descend beyond another NSView, move on to the next
						{
							lastFoundView = foundView;
							break;
						}

						nextItor = (*itor)->get_children().get_first();
						if (!nextItor)
							break;

						itor = nextItor;
					}

					if (!itor)
						break;

					for (;;)
					{
						COGS_ASSERT(!!itor);
						nextItor = itor;
						++nextItor;
						if (!!nextItor)
						{
							itor = nextItor;
							break;
						}
						p = (*itor)->get_parent();
						COGS_ASSERT(p != get_bridge(*m_parentView)); // Shouldn't get here.  We should have found this obj.
						itor = p->get_sibling_iterator();
					}
				}
			}

			if (!lastFoundView) // if none found, we were the last z-order NSView in this parent NSView
			{
				if (!!m_parentView->m_lastChildView) // If any were present
				{
					m_prevSiblingView = m_parentView->m_lastChildView;
					m_prevSiblingView->m_nextSiblingView = this_rcref;
				}
				else // If none were present
				{
					m_parentView->m_firstChildView = this_rcref;
				}
				m_parentView->m_lastChildView = this_rcref;
			}
			else // We were not the last in z-order.  We fit immediately after lastFoundView
			{
				m_prevSiblingView = lastFoundView;
				m_nextSiblingView = lastFoundView->m_nextSiblingView;
				if (!!m_nextSiblingView)
					m_nextSiblingView->m_prevSiblingView = this_rcref;
				else
					m_parentView->m_lastChildView = this_rcref;
				lastFoundView->m_nextSiblingView = this_rcref;
			}
			break;
		}

		[m_nsView setAutoresizesSubviews:NO];
		[m_nsView setPostsFrameChangedNotifications:YES];

		[m_nsView setHidden:YES];

		if (!!m_parentView)
		{
			NSView* parentView = m_parentView->m_nsView;
			if (!m_prevSiblingView)
				[parentView addSubview: m_nsView];
			else
			{
				NSView* prevSiblingView = m_prevSiblingView.get_ptr()->m_nsView;
				[parentView addSubview: m_nsView positioned: NSWindowAbove relativeTo: prevSiblingView];
			}
		}

		set_externally_drawn(this_rcref);
	}

	virtual void reshape(const gfx::bounds& b, const gfx::point& oldOrigin = gfx::point(0, 0))
	{
		if (!!get_bridge())
		{
			if (!!m_parentView) // if not a window
			{
				gfx::point positionInParentView = b.get_position();
				rcptr<gui::pane> p = get_bridge()->get_parent(); // find new coords in parent nsview
				for (;;)
				{
					if (!p || (p == m_parentView->get_bridge()))
						break;
					positionInParentView += p->get_position();
					p = p->get_parent();
				}

				NSRect newRect = make_NSRect(positionInParentView, b.get_size());
				[m_nsView setFrame:newRect];
			}
		}

		bridgeable_pane::reshape(b, oldOrigin);
	}

	virtual void hiding()
	{
		[m_nsView setHidden:YES];
		bridgeable_pane::hiding();
	}

	virtual void showing()
	{
		bridgeable_pane::showing();
		[m_nsView setHidden:NO];
		[m_nsView setNeedsDisplay:YES];
	}

	virtual void focusing(int direction)
	{
		[[m_nsView window] makeFirstResponder: m_nsView];
		bridgeable_pane::focusing(direction);
	}

	virtual void invalidating(const gfx::bounds& b)
	{
		NSRect r = make_NSRect(b);
		[m_nsView setNeedsDisplayInRect:r];
	}

	virtual void fill(const gfx::bounds& b, const color& c = color::constant::black, bool blendAlpha = true)
	{
		graphics_context::fill(b, c, blendAlpha);
	}

	virtual void invert(const gfx::bounds& b)
	{
		graphics_context::invert(b);
	}

	virtual void draw_line(const gfx::point& startPt, const gfx::point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true)
	{
		graphics_context::draw_line(startPt, endPt, width, c, blendAlpha);
	}

	virtual rcref<gfx::font> load_font(const gfx::font_parameters_list& f)
	{
		return graphics_context::load_font(f);
	}

	virtual string get_default_font_name() const
	{
		return graphics_context::get_default_font_name();
	}

	using gui::bridgeable_pane::get_default_text_foreground_color;
	using gui::bridgeable_pane::get_default_text_background_color;
	using gui::bridgeable_pane::get_default_selected_text_foreground_color;
	using gui::bridgeable_pane::get_default_selected_text_background_color;
	using gui::bridgeable_pane::get_default_label_foreground_color;
	using gui::bridgeable_pane::get_default_background_color;

	virtual void draw_text(const composite_string& s, const gfx::bounds& b, const rcptr<gfx::font>& f, const color& c = color::constant::black)
	{
		// Align to nearest whole pixel to avoid text wiggling around when repositioned, due to spacing changes.
		NSRect r = make_NSRect(b);
		r = [m_nsView backingAlignedRect:r options: NSAlignAllEdgesNearest];
		gfx::bounds b2 = make_bounds(r);
		graphics_context::draw_text(s, b2, f, c);
	}

	virtual void draw_bitmap(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, bool blendAlpha = true)
	{
		graphics_context::draw_bitmap(src, srcBounds, dstBounds, blendAlpha);
	}

	virtual void draw_bitmask(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		graphics_context::draw_bitmask(msk, mskBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha);
	}

	virtual void mask_out(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool inverted = false)
	{
		graphics_context::mask_out(msk, mskBounds, dstBounds, inverted);
	}

	virtual void draw_bitmap_with_bitmask(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool blendAlpha = true, bool inverted = false)
	{
		graphics_context::draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted);
	}

	virtual rcref<gfx::bitmap> create_bitmap(const gfx::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return graphics_context::create_bitmap(sz, fillColor);
	}

	virtual rcref<gfx::bitmap> load_bitmap(const composite_string& location)
	{
		return graphics_context::load_bitmap(location);
	}

	virtual rcref<gfx::bitmask> create_bitmask(const gfx::size& sz, std::optional<bool> value = std::nullopt)
	{
		return graphics_context::create_bitmask(sz, value);
	}

	virtual rcref<gfx::bitmask> load_bitmask(const composite_string& location)
	{
		return graphics_context::load_bitmask(location);
	}

	virtual void save_clip()
	{
		graphics_context::save_clip();
	}

	virtual void restore_clip()
	{
		graphics_context::restore_clip();
	}

	virtual void clip_out(const gfx::bounds& b)
	{
		graphics_context::clip_out(b, get_size());
	}

	virtual void clip_to(const gfx::bounds& b)
	{
		graphics_context::clip_to(b);
	}

	virtual bool is_unclipped(const gfx::bounds& b) const
	{
		if (!b.get_height() || !b.get_width())
			return false;

		NSRect r = make_NSRect(b);
		return [m_nsView needsToDrawRect: r];
	}

	using pane_orchestrator::request_close;
	using pane_orchestrator::close;
	using pane_orchestrator::focus;
	using bridgeable_pane::get_bridge;
};


inline rcref<gui::bridgeable_pane> nsview_subsystem::create_native_pane() volatile
{
	class native_pane : public nsview_pane
	{
	public:
		explicit native_pane(const rcref<volatile nsview_subsystem>& subSystem)
			: nsview_pane(subSystem)
		{ }

		~native_pane()
		{
			objc_view* objcView = (objc_view*)get_NSView();
			if (!!objcView)
				objcView->m_cppView.release();
		}

		virtual void installing()
		{
			__strong objc_view* objcView = [[objc_view alloc] init];
			objcView->m_cppView = this_rcptr;
			[objcView setAutoresizesSubviews:NO];
			install_NSView(objcView);
			nsview_pane::installing();
		}
	};

	rcref<nsview_pane> p = rcnew(native_pane)(this_rcref);
	return p;
}


}

}


#ifdef COGS_OBJECTIVE_C_CODE


/// @internal
@implementation objc_view


-(BOOL)isFlipped
{
	return YES;
}

-(void)drawRect:(NSRect)r
{
	[super drawRect: r];
	cogs::rcptr<cogs::os::nsview_pane> cppView = m_cppView;
	if (!!cppView)
		cppView->draw();
}

@end


#endif


#include "cogs/os/gui/window.hpp"
#include "cogs/os/gui/button.hpp"
#include "cogs/os/gui/check_box.hpp"
#include "cogs/os/gui/scroll_bar.hpp"
#include "cogs/os/gui/text_editor.hpp"


#endif
