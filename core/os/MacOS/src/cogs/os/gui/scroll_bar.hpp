//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GUI_SCROLL_BAR
#define COGS_HEADER_OS_GUI_SCROLL_BAR


#include "cogs/sync/transactable.hpp"
#include "cogs/gui/scroll_bar.hpp"
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

@end


namespace cogs {
namespace gui {
namespace os {


class scroll_bar : public nsview_pane, public scroll_bar_interface
{
private:
	volatile transactable<scroll_bar_state> m_state;
	volatile double m_pos;

	delayed_construction<delegated_dependency_property<scroll_bar_state> > m_stateProperty;
	delayed_construction<delegated_dependency_property<double> > m_positionProperty;

	bool m_isHiddenWhenInactive;
	bool m_isHidden;

	dimension m_dimension;
	range m_currentRange;
	size m_currentDefaultSize;

	void set_scroll_bar_state(const scroll_bar_state& newState, double newPos)
	{
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
						nsview_pane::hiding();
					}
					break;
				}
				[objcScrollBar setEnabled:NO];
			}
			else
			{
				double maxPos = newState.m_max - newState.m_thumbSize;
				double pos = newPos;
				if (pos > maxPos)
					pos = maxPos;
			
				[objcScrollBar setDoubleValue: (pos / maxPos) ];
				[objcScrollBar setKnobProportion: (newState.m_thumbSize / newState.m_max) ];
				[objcScrollBar setEnabled:YES];
			}
			if (!!m_isHidden)
			{
				m_isHidden = false;
				nsview_pane::showing();
			}
			break;
		}
	}

public:
	scroll_bar(const ptr<rc_obj_base>& desc, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem),
		m_isHidden(false),
		m_pos(0)
	{
		auto stateGetter = [this]()
		{
			return *(m_state.begin_read());
		};

		auto stateSetter = [this](const scroll_bar_state& state)
		{
			scroll_bar_state newState = state;
			scroll_bar_state oldState = newState;
			m_state.swap_contents(oldState);
			if (newState != oldState)
			{
				double curPos = atomic::load(m_pos);
				set_scroll_bar_state(newState, curPos);
				m_stateProperty->changed();
			}
			m_stateProperty->set_complete();
		};

		placement_rcnew(&m_stateProperty.get(), this_desc, uiSubsystem, std::move(stateGetter), std::move(stateSetter));

		auto positionGetter = [this]()
		{
			return atomic::load(m_pos);
		};

		auto positionSetter = [this](double d)
		{
			double newPos = d;
			double oldPos = atomic::exchange(m_pos, newPos);
			if (newPos != oldPos)
			{
				set_scroll_bar_state(*(m_state.begin_read()), newPos);
				m_positionProperty->changed();
			}
			m_positionProperty->set_complete();
		};

		placement_rcnew(&m_positionProperty.get(), this_desc, uiSubsystem, std::move(positionGetter), std::move(positionSetter));
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

		[objcScrollBar setTarget:objcScrollBar];
		[objcScrollBar setAction: @selector(scrolled:)];

		install_NSView(objcScrollBar);
		nsview_pane::installing();

		m_stateProperty->bind_from(sb->get_state_property());
		m_positionProperty->bind(sb->get_position_property());
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
			atomic::store(m_pos, pos);
			m_positionProperty->changed();
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


- (BOOL)isFlipped { return TRUE; }

-(void)scrolled:(id)sender;
{
	cogs::rcptr<cogs::gui::os::scroll_bar> cppScrollBar = m_cppScrollBar;
	if (!!cppScrollBar)
		cppScrollBar->scrolled();
}


@end


#endif


#endif
