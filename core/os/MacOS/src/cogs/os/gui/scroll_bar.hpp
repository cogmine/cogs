//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_SCROLL_BAR
#define COGS_HEADER_OS_GUI_SCROLL_BAR


#include "cogs/sync/transactable.hpp"
#include "cogs/sync/resettable_timer.hpp"
#include "cogs/gui/scroll_bar.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gui/nsview.hpp"


namespace cogs {
namespace gui {
namespace os {


class scroll_bar;


};
};
};


@interface objc_scroll_bar : NSScroller
{
@public
	cogs::weak_rcptr<cogs::gui::os::scroll_bar> m_cppScrollBar;
}

-(BOOL)isFlipped;
-(void)scrolled:(id)sender;
-(void)preferredScrollerStyleChanged:(NSNotification*)notification;
+(BOOL)isCompatibleWithOverlayScrollers;
-(void)drawRect:(NSRect)dirtyRect;
-(void)drawKnob;
-(void)defaultDrawKnob;
-(void)drawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag;
-(void)defaultDrawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag;
-(void)fade:(NSTimer*)timer;

@end


namespace cogs {
namespace gui {
namespace os {


class scroll_bar : public nsview_pane, public scroll_bar_interface
{
private:
	volatile transactable<scroll_bar_state> m_state;
	volatile double m_pos = 0;
	volatile boolean m_canAutoFade;
	volatile boolean m_shouldAutoFade;
	dimension m_dimension;
	range m_currentRange;
	size m_currentDefaultSize;
	bool m_isHiddenWhenInactive;
	bool m_isHidden = false;
	CGFloat m_knobAlpha;
	bool m_isKnobSlotVisible;

	rcref<resettable_timer> m_fadeDelayTimer;
	boolean m_fadeDelayDispatched;
	__strong NSTimer* m_fadeTimer = nullptr;
	rcptr<task<void> > m_fadeDelayTask;

	delegated_dependency_property<scroll_bar_state> m_stateProperty;
	delegated_dependency_property<double> m_positionProperty;
	delegated_dependency_property<bool> m_canAutoFadeProperty;
	delegated_dependency_property<bool> m_shouldAutoFadeProperty;

	void set_scroll_bar_state(const scroll_bar_state& newState, double newPos)
	{
		start_fade_delay();
		rcptr<gui::scroll_bar> sb = get_bridge().template static_cast_to<gui::scroll_bar>();
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		for (;;)
		{
			if (newState.m_thumbSize >= newState.m_max)
			{
				if (!!m_isHiddenWhenInactive)
				{
					if (!m_isHidden)
					{
						m_isHidden = true;
						sb->hide();
					}
					break;
				}
				[objcScrollBar setEnabled: NO];
			}
			else
			{
				double maxPos = newState.m_max - newState.m_thumbSize;
				double pos = newPos;
				if (pos > maxPos)
					pos = maxPos;

				[objcScrollBar setEnabled: YES] ;
				[objcScrollBar setDoubleValue: (pos / maxPos) ];
				[objcScrollBar setKnobProportion: (newState.m_thumbSize / newState.m_max) ];
			}
			if (!!m_isHidden)
			{
				m_isHidden = false;
				sb->show();
			}
			break;
		}
	}

	// Will execute in main thread
	void delay_expired()
	{
		m_fadeDelayDispatched = false;
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		m_fadeTimer = [NSTimer timerWithTimeInterval: 0.05 target: objcScrollBar selector: @selector(fade:) userInfo: nil repeats: YES];

		[[NSRunLoop mainRunLoop] addTimer: m_fadeTimer forMode: NSRunLoopCommonModes];
	}

	void stop_fade(CGFloat knobAlpha = 1.0)
	{
		if (!!m_fadeTimer)
		{
			[m_fadeTimer invalidate];
			m_fadeTimer = nullptr;
		}

		if (m_knobAlpha != knobAlpha)
		{
			m_knobAlpha = knobAlpha;
			objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
			[objcScrollBar setNeedsDisplay: YES];
		}
	}

	void start_fade_delay()
	{
		if (m_isFadeSupressed || !m_shouldAutoFade || !m_canAutoFade)
			return;

		stop_fade();

		if (m_fadeDelayDispatched)
		{
			if (m_fadeDelayTimer->reschedule(make_measure<seconds>(1)))
				return; // Succeeded in postponing the existing timer.  All done

			// The timer must have already fired.
			// We are on the main thread, and delay_expired must be pending after us.
			// Cancellation should succeed.
			m_fadeDelayTask->cancel();
		}
		else
		{
			// The timer was not running.  Start it.
			m_fadeDelayDispatched = true;
			m_fadeDelayTimer->reset(make_measure<seconds>(1));
		}
		m_fadeDelayTask = m_fadeDelayTimer->dispatch([r{ this_weak_rcptr }]()
		{
			// move to main thread
			rcptr<scroll_bar> r2 = r;
			if (!r2)
				return signaled();
			return r2->get_subsystem()->dispatch([r]()
			{
				rcptr<scroll_bar> r2 = r;
				if (!!r2)
					r2->delay_expired();
			});
		});
	}

	volatile boolean m_isFadeSupressed;

	// Must only be called from main thread.
	// m_isFadeSupressed can be read from any thread, but should only be modified in the main thread.
	void suppress_fade()
	{
		if (!m_isFadeSupressed)
		{
			m_isFadeSupressed = true;
			if (m_shouldAutoFade && m_canAutoFade)
				stop_fade(1.0);
		}
	}

	// Must only be called from main thread
	void unsuppress_fade()
	{
		if (!!m_isFadeSupressed)
		{
			m_isFadeSupressed = false;
			start_fade_delay();
		}
	}

public:
	scroll_bar(rc_obj_base& desc, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem),
		m_fadeDelayTimer(rcnew(resettable_timer)),
		m_stateProperty(desc, uiSubsystem, [this]()
		{
			return *(m_state.begin_read());
		}, [this](const scroll_bar_state& state)
		{
			scroll_bar_state newState = state;
			scroll_bar_state oldState = newState;
			m_state.swap_contents(oldState);
			if (newState != oldState)
			{
				double curPos = atomic::load(m_pos);
				set_scroll_bar_state(newState, curPos);
				m_stateProperty.changed();
			}
			m_stateProperty.set_complete();
		}),
		m_positionProperty(desc, uiSubsystem, [this]()
		{
			return atomic::load(m_pos);
		}, [this](double d)
		{
			double newPos = d;
			double oldPos = atomic::exchange(m_pos, newPos);
			if (newPos != oldPos)
			{
				set_scroll_bar_state(*(m_state.begin_read()), newPos);
				m_positionProperty.changed();
			}
			m_positionProperty.set_complete();
		}),
		m_canAutoFadeProperty(desc, uiSubsystem, [this]()
		{
			return m_canAutoFade;
		}, [this](bool b)
		{
			bool oldValue = m_canAutoFade.exchange(b);
			if (b != oldValue)
			{
				if (m_shouldAutoFade)
				{
					objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
					if (objcScrollBar)
					{
						if (!b)
						{
							stop_fade(1.0);
							[objcScrollBar setScrollerStyle: NSScrollerStyleLegacy];
							[objcScrollBar setNeedsDisplay : YES] ;
						}
						else
						{
							stop_fade(0.0);
							[objcScrollBar setScrollerStyle: NSScrollerStyleOverlay];
							[objcScrollBar setNeedsDisplay: YES];
						}
					}
				}
				m_canAutoFadeProperty.changed();
			}
			m_canAutoFadeProperty.set_complete();
		}),
		m_shouldAutoFadeProperty(desc, uiSubsystem, [this]()
		{
			return m_shouldAutoFade;
		}, [this](bool b)
		{
			bool oldValue = m_shouldAutoFade.exchange(b);
			if (b != oldValue)
			{
				if (m_canAutoFade)
				{
					objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
					if (objcScrollBar)
					{
						if (!b)
						{
							stop_fade(1.0);
							[objcScrollBar setScrollerStyle: NSScrollerStyleLegacy];
							[objcScrollBar setNeedsDisplay: YES];
						}
						else
						{
							stop_fade(0.0);
							[objcScrollBar setScrollerStyle: NSScrollerStyleOverlay];
							[objcScrollBar setNeedsDisplay: YES];
						}
					}
				}
				m_shouldAutoFadeProperty.changed();
			}
			m_shouldAutoFadeProperty.set_complete();
		})
	{
	}

	virtual void installing()
	{
		rcptr<gui::scroll_bar> sb = get_bridge().template static_cast_to<gui::scroll_bar>();
		m_dimension = sb->get_dimension();
		m_isHiddenWhenInactive = sb->is_hidden_when_inactive();

		// use bogus frame just for the purpose of detecting dimension.
		NSRect bogusBounds;
		bogusBounds.origin.x = bogusBounds.origin.y = 0;
		if (m_dimension == dimension::horizontal)
		{
			bogusBounds.size.width = 100;
			bogusBounds.size.height = 10;
		}
		else
		{
			bogusBounds.size.width = 10;
			bogusBounds.size.height = 100;
		}

		__strong objc_scroll_bar* objcScrollBar = [[objc_scroll_bar alloc] initWithFrame:bogusBounds];
		objcScrollBar->m_cppScrollBar = this_rcptr;

		bogusBounds.origin.x = bogusBounds.origin.y = 0;

		m_isKnobSlotVisible = true;

		[objcScrollBar setTarget:objcScrollBar];
		[objcScrollBar setAction: @selector(scrolled:)];

		NSScrollerStyle preferredStyle = [NSScroller preferredScrollerStyle];
		bool canAutoFade = (preferredStyle == NSScrollerStyleOverlay);
		bool shouldAutoFade = sb->get_should_auto_fade_property()->get();
		m_canAutoFade = canAutoFade;
		m_shouldAutoFade = shouldAutoFade;
		if (shouldAutoFade && canAutoFade)
		{
			[objcScrollBar setScrollerStyle: preferredStyle];
			m_knobAlpha = 0.0;
		}
		else
		{
			[objcScrollBar setScrollerStyle: NSScrollerStyleLegacy];
			m_knobAlpha = 1.0;
		}

		m_stateProperty.bind_from(sb->get_state_property());
		m_positionProperty.bind(sb->get_position_property(), false);
		m_canAutoFadeProperty.bind_to(get_can_auto_fade_property(sb.dereference()));
		m_shouldAutoFadeProperty.bind_from(sb->get_should_auto_fade_property());

		[[NSNotificationCenter defaultCenter] addObserver:objcScrollBar selector:@selector(preferredScrollerStyleChanged:) name:NSPreferredScrollerStyleDidChangeNotification object:nil];

		install_NSView(objcScrollBar);
		nsview_pane::installing();
	}

	void scrolled()
	{
		transactable<scroll_bar_state>::read_token rt;
		m_state.begin_read(rt);
		bool setPosition = true;

		double oldPos = atomic::load(m_pos);
		double pos = oldPos;
		double max = rt->m_max;
		double thumbSize = rt->m_thumbSize;
		double maxPos = max;
		maxPos -= thumbSize;

		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		NSScrollerPart part = [objcScrollBar hitPart];
		switch (part)
		{
		case NSScrollerDecrementPage:
			if (pos <= thumbSize)
				pos = 0;
			else
				pos -= thumbSize;
			break;
		case NSScrollerIncrementPage:
			pos += thumbSize;
			if (pos > maxPos)
				pos = maxPos;
			break;
		//case NSScrollerDecrementLine:
		//	if (pos > 0)
		//		--pos;
		//	break;
		//case NSScrollerIncrementLine:
		//	if (pos < maxPos)
		//		++pos;
		//	break;
		case NSScrollerKnob:
		case NSScrollerKnobSlot:
			{
				double curValue = [objcScrollBar doubleValue];
				double scaledUp = curValue * maxPos;
				pos = (longest)scaledUp;
			}
		case NSScrollerNoPart:
		default:
			setPosition = false;
			break;
		}

		if (pos != oldPos)
		{
			if (!!setPosition)
				[objcScrollBar setDoubleValue: (pos/maxPos) ];

			// sets are serialized in the UI thread.  No need to worry about synchronizing with other writes.
			m_positionProperty.set(pos);
		}
	}

	virtual void calculate_range()
	{
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		m_currentRange.clear();
		double scrollBarWidth = (longest)[NSScroller scrollerWidthForControlSize:[objcScrollBar controlSize] scrollerStyle:[objcScrollBar scrollerStyle]];
		m_currentRange.get_max(!m_dimension) = scrollBarWidth;
		m_currentDefaultSize.set(scrollBarWidth, scrollBarWidth);
	}

	virtual range get_range() const { return m_currentRange; }
	virtual size get_default_size() const { return m_currentDefaultSize; }

	virtual bool is_focusable() const { return false; }

	void set_can_auto_fade(bool b)
	{
		//objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		m_canAutoFadeProperty.set(b);
	}

	//bool drawRect(const NSRect& dirtyRect)
	//{
	//	objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
	//	if ([objcScrollBar respondsToSelector : @selector(scrollerStyle)] && objcScrollBar.scrollerStyle == NSScrollerStyleOverlay) {
	//		[[NSColor clearColor]set];
	//		NSRectFill(NSInsetRect([objcScrollBar bounds], -1.0, -1.0));

	//		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
	//		CGContextRef context = [curContext CGContext];
	//		CGContextSaveGState(context);
	//		CGContextSetAlpha(context, m_knobAlpha);

	//		if (m_isKnobSlotVisible) {
	//			[objcScrollBar drawKnobSlotInRect : [objcScrollBar rectForPart : NSScrollerKnobSlot] highlight : NO] ;
	//		}
	//		[objcScrollBar drawKnob];
	//		CGContextRestoreGState(context);
	//		return true;
	//	}
	//	return false;
	//}

	void drawKnob()
	{
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		//if ([objcScrollBar respondsToSelector : @selector(scrollerStyle)] && objcScrollBar.scrollerStyle == NSScrollerStyleOverlay) {
		//	[[NSColor clearColor]set];
		//	NSRectFill(NSInsetRect([objcScrollBar bounds], -1.0, -1.0));

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		CGContextRef context = [curContext CGContext];
		CGContextSaveGState(context);
		CGContextSetAlpha(context, m_knobAlpha);

		//if (m_isKnobSlotVisible) {
		//	[objcScrollBar drawKnobSlotInRect : [objcScrollBar rectForPart : NSScrollerKnobSlot] highlight : NO] ;
		//}
		[objcScrollBar defaultDrawKnob];
		CGContextRestoreGState(context);
		//}
		//else
		//	[objcScrollBar defaultDrawKnob];
	}

	void drawKnobSlotInRect(const NSRect& slotRect, bool highlightFlag)
	{
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		//if ([objcScrollBar respondsToSelector : @selector(scrollerStyle)] && objcScrollBar.scrollerStyle == NSScrollerStyleOverlay) {
		//	[[NSColor clearColor]set];
		//	NSRectFill(NSInsetRect([objcScrollBar bounds], -1.0, -1.0));

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		CGContextRef context = [curContext CGContext];
		CGContextSaveGState(context);
		CGContextSetAlpha(context, m_knobAlpha);

		//if (m_isKnobSlotVisible) {
		//	[objcScrollBar drawKnobSlotInRect : [objcScrollBar rectForPart : NSScrollerKnobSlot] highlight : NO] ;
		//}
		[objcScrollBar defaultDrawKnobSlotInRect: slotRect highlight: highlightFlag];
		CGContextRestoreGState(context);
		//}
		//else
		//	[objcScrollBar defaultDrawKnob];
	}

	void fade()
	{
		objc_scroll_bar* objcScrollBar = (objc_scroll_bar*)get_NSView();
		CGFloat alpha = m_knobAlpha - 0.2;
		if (alpha > 0) {
			m_knobAlpha = alpha;
			[objcScrollBar setNeedsDisplay: YES];
		}
		else
		{
			[m_fadeTimer invalidate];
			m_fadeTimer = nullptr;
		}
	}

	virtual void cursor_entering(const point& pt)
	{
		suppress_fade();
		nsview_pane::cursor_entering(pt);
	}

	virtual void cursor_leaving()
	{
		unsuppress_fade();
		nsview_pane::cursor_leaving();
	}
};


inline std::pair<rcref<bridgeable_pane>, rcref<scroll_bar_interface> > nsview_subsystem::create_scroll_bar() volatile
{
	rcref<scroll_bar> sb = rcnew(scroll_bar, this_rcref);
	return std::make_pair(sb, sb);
}


}
}
}


#ifdef COGS_OBJECTIVE_C_CODE


@implementation objc_scroll_bar


-(BOOL)isFlipped
{
	return YES;
}

-(void)scrolled:(id)sender
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->scrolled();
}

-(void)preferredScrollerStyleChanged:(NSNotification*)notification
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
	{
		NSScrollerStyle scrollerStyle = [NSScroller preferredScrollerStyle];
		cppScrollBar->set_can_auto_fade(scrollerStyle == NSScrollerStyleOverlay);
	}
}

+(BOOL)isCompatibleWithOverlayScrollers
{
	return YES;
}

-(void)drawKnob
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->drawKnob();
	else
		[super drawKnob];
}

-(void)defaultDrawKnob
{
	[super drawKnob];
}

-(void)drawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->drawKnobSlotInRect(slotRect, flag);
	else
		[super drawKnobSlotInRect:slotRect highlight:flag];
}

-(void)defaultDrawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag
{
	[super drawKnobSlotInRect:slotRect highlight:flag];
}

-(void)drawRect:(NSRect)dirtyRect
{
	[super drawRect:dirtyRect];
}

//-(void)drawRect:(NSRect)dirtyRect
//{
//	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
//	if (!cppScrollBar || !cppScrollBar->drawRect(dirtyRect))
//		[super drawRect:dirtyRect];
//}

-(void)fade:(NSTimer*)timer
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->fade();
}

//+(CGFloat)scrollerWidthForScrollerStyle:(NSScrollerStyle)scrollerStyle
//{
//	if (scrollerStyle == NSScrollerStyleOverlay)
//		return 10; // BUG: +[NSScroller scrollerWidthForControlSize:scrollerStyle:] returns 15 for NSScrollerStyleOverlay...
//	return[NSScroller scrollerWidth];
//}
//
//-(void)setState:(MyScrollViewState)state knobSlotState : (MyScrollViewState)knobSlotState
//{
//	self.hidden = (state == MyScrollViewStateNormal);
//	knobSlotVisible = (state == knobSlotState);
//	[self setNeedsDisplay: YES] ;
//}

//-(void)removeTrackingAreas
//{
//	if (trackingArea != nil)
//	{
//		[self removeTrackingArea : trackingArea] ;
//		[trackingArea release] ;
//		trackingArea = nil;
//	}
//}
//
//-(void)updateTrackingAreas
//{
//	[self removeTrackingAreas] ;
//
//	if ([self respondsToSelector : @selector(scrollerStyle)]) {
//		trackingArea = [[NSTrackingArea alloc]initWithRect:self.bounds
//			options : NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
//			owner : self
//			userInfo : nil];
//		[self addTrackingArea : trackingArea] ;
//	}
//}


@end


#endif


#endif
