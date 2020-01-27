//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_GUI_SCROLL_PANE
#define COGS_HEADER_GUI_SCROLL_PANE

#include "cogs/gui/pane.hpp"
#include "cogs/gui/label.hpp"
#include "cogs/gui/grid.hpp"
#include "cogs/gui/scroll_bar.hpp"
#include "cogs/gui/native_container_pane.hpp"
#include "cogs/sync/transactable.hpp"

namespace cogs {
namespace gui {

enum scroll_dimensions
{
	scroll_horizontally                = 0x01, // 01
	scroll_vertically                  = 0x02, // 10
	scroll_horizontally_and_vertically = 0x03  // 11
};

/// @ingroup GUI
/// @brief A scrollable GUI outer pane, providing a view of an inner pane.
/// Inner pane should have a minimum size, under which size the need for a scroll bar is incurred
class scroll_pane : public pane
{
private:
	class scroll_bar_info
	{
	public:
		rcref<scroll_bar> m_scrollBar;
		rcref<override_bounds_frame> m_frame;
		volatile transactable<scroll_bar_state> m_state;
		volatile double m_position = 0;
		volatile boolean m_canAutoFade;

		delegated_dependency_property<scroll_bar_state, io::read_only> m_stateProperty;
		delegated_dependency_property<double> m_positionProperty;
		delegated_dependency_property<bool, io::write_only> m_canAutoFadeProperty;

		scroll_bar_info(rc_obj_base& desc, scroll_pane& scrollPane, dimension d)
			: m_scrollBar(rcnew(scroll_bar, d, false)),
			m_frame(rcnew(override_bounds_frame, m_scrollBar)),
			m_stateProperty(desc, scrollPane, [this]()
			{
				return *(m_state.begin_read());
			}),
			m_positionProperty(desc, scrollPane, [this]()
			{
				return atomic::load(m_position);
			}, [this, &scrollPane](double d)
			{
				double newPos = d;
				double oldPos = atomic::exchange(m_position, newPos);
				if (oldPos != newPos)
				{
					m_positionProperty.changed();
					scrollPane.scrolled();
				}
				m_positionProperty.set_complete();
			}),
			m_canAutoFadeProperty(desc, scrollPane, [this, &scrollPane](bool b)
			{
				boolean oldValue = m_canAutoFade.exchange(b);
				if (oldValue != b && scrollPane.m_shouldAutoFadeScrollBar)
					scrollPane.recompose();
				m_canAutoFadeProperty.set_complete();
			})
		{
			scrollPane.pane::nest_last(m_scrollBar, m_frame);
			m_stateProperty.bind_to(m_scrollBar->get_state_property());
			m_positionProperty.bind(m_scrollBar->get_position_property(), true);
			m_canAutoFadeProperty.bind_from(m_scrollBar->get_can_auto_fade_property());
			scrollPane.m_shouldAutoFadeScrollBarProperty.bind_to(m_scrollBar->get_should_auto_fade_property());
		}
	};

	bool m_hasScrollBar[2];
	placement<scroll_bar_info> m_scrollBarInfo[2];

	rcptr<container_pane> m_contentPane;
	rcptr<native_container_pane> m_clippingPane; // using a native pane ensures clipping of platform dependent pane children (i.e. OS buttons, etc.)
	rcptr<container_pane> m_cornerPane;

	rcptr<override_bounds_frame> m_contentFrame;
	rcptr<override_bounds_frame> m_clippingFrame;
	rcptr<override_bounds_frame> m_cornerFrame;

	range m_calculatedRange;
	size m_calculatedDefaultSize;

	bool m_hideInactiveScrollBar;

	volatile boolean m_shouldAutoFadeScrollBar;
	delegated_dependency_property<bool> m_shouldAutoFadeScrollBarProperty;

	bool has_scroll_bar(dimension d) const { return m_hasScrollBar[(int)d]; }

	scroll_bar_info& get_scroll_bar_info(dimension d)
	{
		COGS_ASSERT(has_scroll_bar(d));
		return m_scrollBarInfo[(int)d].get();
	}

	const scroll_bar_info& get_scroll_bar_info(dimension d) const
	{
		COGS_ASSERT(has_scroll_bar(d));
		return m_scrollBarInfo[(int)d].get();
	}

	void scrolled()
	{
		point oldOrigin = m_contentFrame->get_position();
		point oldScrollPosition = -oldOrigin;

		double hScrollPosition = has_scroll_bar(dimension::horizontal) ? atomic::load(get_scroll_bar_info(dimension::horizontal).m_position) : 0.0;
		double vScrollPosition = has_scroll_bar(dimension::vertical) ? atomic::load(get_scroll_bar_info(dimension::vertical).m_position) : 0.0;

		point newScrollPosition(hScrollPosition, vScrollPosition);
		if (oldScrollPosition != newScrollPosition)
		{
			m_contentFrame->get_position() = -newScrollPosition;
			cell::reshape(*m_contentFrame, m_contentFrame->get_fixed_size(), oldOrigin);
		}
	}

	bool use_scroll_bar_auto_fade() const
	{
		if (!m_shouldAutoFadeScrollBar)
			return false;
		dimension d = dimension::vertical;
		if (!has_scroll_bar(d))
			d = !d;
		return get_scroll_bar_info(d).m_canAutoFade;
	}


public:
	// The behavior of scroll_pane varies depending on the input device.
	// Mode A: For traditional mouse-based input, scroll bars are always shown (though can optionally be hidden when inactive).
	// Mode B: For touch screen, or if scrolling is otherwise provided by a device, scroll bars are displayed overlaying
	// the content only when scrolling occurs, then fade.  Drag/flick scrolling is always enabled in Mode B.
	// On MacOS, there is a user setting to dynamically switch between these modes.
	explicit scroll_pane(
		rc_obj_base& desc,
		scroll_dimensions scrollDimensions = scroll_horizontally_and_vertically,
		bool hideInactiveScrollBar = true,
		bool shouldScrollBarAutoFade = true, // If false, scroll bars are always displayed in mode B.
		bool dragAndFlickScrolling = true) // If true, enables drag and flick scrolling in mode A.  It's always enabled in mode B.
		: pane(desc),
		m_hideInactiveScrollBar(hideInactiveScrollBar),
		m_shouldAutoFadeScrollBar(shouldScrollBarAutoFade),
		m_shouldAutoFadeScrollBarProperty(desc, *this, [this]()
		{
			return m_shouldAutoFadeScrollBar;
		}, [this](bool b)
		{
			bool oldValue = m_shouldAutoFadeScrollBar.exchange(b);
			if (oldValue != b)
				m_shouldAutoFadeScrollBarProperty.changed();
			m_shouldAutoFadeScrollBarProperty.set_complete();
		})
	{
		m_contentPane = rcnew(container_pane);

		// TODO: May need to address what happens when a native control is offscreen when drawn, and backing buffer is unavailable
		//m_contentPane->set_compositing_behavior(compositing_behavior::buffer_self_and_children);
		m_clippingPane = rcnew(native_container_pane);
		m_cornerPane = rcnew(container_pane);

		m_contentFrame = rcnew(override_bounds_frame, m_contentPane.dereference());
		rcref<unconstrained_frame> unconstrainedFrame = rcnew(unconstrained_frame, m_contentFrame.dereference(), alignment(0, 0));

		m_clippingFrame = rcnew(override_bounds_frame, m_clippingPane.dereference());
		m_cornerFrame = rcnew(override_bounds_frame, m_cornerPane.dereference());

		m_clippingPane->nest(m_contentPane.dereference(), unconstrainedFrame);
		pane::nest_last(m_clippingPane.dereference(), m_clippingFrame);
		pane::nest_last(m_cornerPane.dereference(), m_cornerFrame);

		m_hasScrollBar[(int)dimension::horizontal] = (scrollDimensions & scroll_horizontally) != 0;
		if (m_hasScrollBar[(int)dimension::horizontal])
			new (&get_scroll_bar_info(dimension::horizontal)) scroll_bar_info(desc, *this, dimension::horizontal);

		m_hasScrollBar[(int)dimension::vertical] = (scrollDimensions & scroll_vertically) != 0;
		if (m_hasScrollBar[(int)dimension::vertical])
			new (&get_scroll_bar_info(dimension::vertical)) scroll_bar_info(desc, *this, dimension::vertical);
	}

	~scroll_pane()
	{
		if (m_hasScrollBar[(int)dimension::horizontal])
			m_scrollBarInfo[(int)dimension::horizontal].destruct();
		if (m_hasScrollBar[(int)dimension::vertical])
			m_scrollBarInfo[(int)dimension::vertical].destruct();
	}

	virtual range get_range() const { return m_calculatedRange; }

	virtual size get_default_size() const { return m_calculatedDefaultSize; }

	using pane_container::nest;

	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		m_contentPane->nest_last(child, f);
	}

	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		m_contentPane->nest_first(child, f);
	}

	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0)
	{
		m_contentPane->nest_before(child, beforeThis, f);
	}

	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0)
	{
		m_contentPane->nest_after(child, afterThis, f);
	}

	// Nests a pane to be rendered at the intersection of 2 visible scroll bars.
	// If no corner pane is specified, the scroll_pane's background color is used.
	void nest_corner(const rcref<pane>&child, const rcptr<frame>& f = 0)
	{
		nest_corner_last(child, f);
	}

	void nest_corner_last(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		m_cornerPane->nest_last(child, f);
	}

	void nest_corner_first(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		m_cornerPane->nest_first(child, f);
	}

	void nest_corner_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0)
	{
		m_cornerPane->nest_before(child, beforeThis, f);
	}

	void nest_corner_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0)
	{
		m_cornerPane->nest_after(child, afterThis, f);
	}

	virtual void calculate_range()
	{
		pane::calculate_range();

		// recalculate scroll bar widths, in case dimensions for the scrollbars were just recalculated as well
		for (int i = 0; i < 2; i++)
		{
			dimension d = (dimension)i;
			if (has_scroll_bar(d))
            {
                auto& sbinfo = get_scroll_bar_info(d);
				sbinfo.m_frame->get_fixed_size(!d) = sbinfo.m_scrollBar->get_const_cell().get_default_size()[!d];
            }
        }

		m_calculatedRange.set_min(0, 0);
		m_calculatedRange.clear_max();

		// default to size of contents
		m_calculatedDefaultSize = m_contentPane->get_default_size();

		bool autoFade = use_scroll_bar_auto_fade();
		if (!m_hideInactiveScrollBar && !autoFade)
		{
			for (int i = 0; i < 2; i++)
			{
				dimension d = (dimension)i;
				if (has_scroll_bar(d))
					m_calculatedDefaultSize[!d] += get_scroll_bar_info(d).m_frame->get_fixed_size(!d);
			}
		}

		// Limit max size to max size of contents
		range contentRange = m_contentPane->get_range();

		for (int i = 0; i < 2; i++)
		{
			dimension d = (dimension)i;
			if (contentRange.has_max(!d))
			{
				m_calculatedRange.set_max(!d, contentRange.get_max(!d));
				if (has_scroll_bar(!!d) && !autoFade)
					m_calculatedRange.get_max(!d) += get_scroll_bar_info(d).m_scrollBar->get_const_cell().get_default_size()[!d];
			}

			if (!has_scroll_bar(!d))
			{
				m_calculatedRange.set_min(!d, contentRange.get_min(!d));
				if (!m_hideInactiveScrollBar && has_scroll_bar(!!d) && !autoFade)
					m_calculatedRange.get_min(!d) += get_scroll_bar_info(d).m_scrollBar->get_const_cell().get_default_size()[!d];
			}
		}
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		range r2 = get_range() & r;
		if (r2.is_empty())
			result.set_empty();
		else
		{
			size newSize = r2.limit(sz);
			range contentRange = m_contentFrame->get_range();
			constexpr dimension d = dimension::horizontal;

			// If we are not hiding inactive scroll bars or are showing scroll bars over content (auto fade), use as is
			if (!use_scroll_bar_auto_fade() && m_hideInactiveScrollBar)
			{
				if (has_scroll_bar(d))
				{
					if (contentRange.has_max(!d) && newSize[!d] > contentRange.get_max(!d) && newSize[d] >= contentRange.get_min(d))
						newSize[!d] = contentRange.get_max(!d);
				}
				else
				{
					double min = contentRange.get_min(d);
					if (newSize[!d] < contentRange.get_min(!d))
						min += get_scroll_bar_info(!d).m_frame->get_fixed_size(d);
					if (newSize[d] < min)
						newSize[d] = min;
				}

				if (has_scroll_bar(!d))
				{
					if (contentRange.has_max(d) && newSize[d] > contentRange.get_max(d) && newSize[!d] >= contentRange.get_min(!d))
						newSize[d] = contentRange.get_max(d);
				}
				else
				{
					double min = contentRange.get_min(!d);
					if (newSize[d] < contentRange.get_min(d))
						min += get_scroll_bar_info(d).m_frame->get_fixed_size(!d);
					if (newSize[!d] < min)
						newSize[!d] = min;
				}
			}
			result.set(newSize);
			result.make_relative(sz);
		}
		return result;
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		range contentRange = m_contentFrame->get_range();
		size contentDefaultSize = m_contentFrame->get_default_size();
		point contentOldOrigin = m_contentFrame->get_position();
		bounds contentBounds(contentOldOrigin, contentDefaultSize);
		bounds visibleBounds = b;

		// limit contentBounds to visibleBounds, but may still have greater min size
		contentBounds.get_size() = contentRange.limit(visibleBounds.get_size());

		bool autoFade = use_scroll_bar_auto_fade();
		auto reduce_content_bounds = [&](dimension d)
		{
			if (!autoFade)
				visibleBounds.get_size()[d] -= get_scroll_bar_info(!d).m_frame->get_fixed_size()[d];
			if (visibleBounds.get_size()[d] < contentBounds.get_size()[d])
			{
				contentBounds.get_size()[d] = visibleBounds.get_size()[d];
				if (contentBounds.get_size()[d] < contentRange.get_min()[d])
					contentBounds.get_size()[d] = contentRange.get_min()[d];
			}
		};

		bool showScrollBar[2];
		if (!m_hideInactiveScrollBar)
		{
			// If not auto-hide, always include them.
			showScrollBar[(int)dimension::horizontal] = has_scroll_bar(dimension::horizontal);
			showScrollBar[(int)dimension::vertical] = has_scroll_bar(dimension::vertical);
			if (showScrollBar[(int)dimension::horizontal])
				reduce_content_bounds(dimension::vertical);
			if (showScrollBar[(int)dimension::vertical])
				reduce_content_bounds(dimension::horizontal);
		}
		else
		{
			showScrollBar[(int)dimension::horizontal] = false;
			showScrollBar[(int)dimension::vertical] = false;
			bool hasBothScrollBars = has_scroll_bar(dimension::horizontal) && has_scroll_bar(dimension::vertical);
			if (!hasBothScrollBars)
			{
				// If auto-hide and only 1 dimension...
				dimension d = has_scroll_bar(dimension::horizontal) ? (dimension)0 : (dimension)1;
				if (contentBounds.get_size()[d] > visibleBounds.get_size()[d])
				{
					showScrollBar[(int)d] = true;
					reduce_content_bounds(!d);
				}
			}
			else
			{
				int neededPrevDimension = 0; // 1 = yes, -1 = new, 0 not checked yet
				dimension d = dimension::horizontal; // doesn't matter which dimension is used first, it's symetrical
				for (;;)
				{
					showScrollBar[(int)d] = (contentBounds.get_size()[d] > visibleBounds.get_size()[d]);
					if (contentBounds.get_size()[d] > visibleBounds.get_size()[d])
					{
						showScrollBar[(int)d] = true;
						reduce_content_bounds(!d);
						if (neededPrevDimension == 1)
							break;
						neededPrevDimension = 1;
					}
					else if (neededPrevDimension != 0)
						break;
					else
						neededPrevDimension = -1;
					d = !d;
					//continue;
				}
			}
		}

		for (int i = 0; i < 2; i++)
		{
			dimension d = (dimension)i;
			if (has_scroll_bar(d))
			{
				contentBounds.get_position()[d] += oldOrigin[d];
				double pos = 0;
				if (showScrollBar[i])
				{
					double thumbSize = visibleBounds.get_size()[d];
					double max = contentBounds.get_size()[d];
					pos = -(contentBounds.get_position()[d]);
					double maxPos = max;
					maxPos -= thumbSize;
					if (pos > maxPos)
					{
						pos = maxPos;
						contentBounds.get_position()[d] = -pos;
					}
					else if (pos < 0)
					{
						pos = 0;
						contentBounds.get_position()[d] = 0;
					}
					get_scroll_bar_info(d).m_frame->get_position()[!d] = b.get_size()[!d];
					get_scroll_bar_info(d).m_frame->get_position()[!d] -= get_scroll_bar_info(d).m_frame->get_fixed_size()[!d];
					get_scroll_bar_info(d).m_state.set(scroll_bar_state(max, thumbSize));
				}
				else
				{
					contentBounds.get_position()[d] = 0;
					get_scroll_bar_info(d).m_state.set(scroll_bar_state(0, 0));
				}
				atomic::store(get_scroll_bar_info(d).m_position, pos);
				get_scroll_bar_info(d).m_frame->get_fixed_size()[d] = b.get_size()[d];
				get_scroll_bar_info(d).m_stateProperty.changed();
				get_scroll_bar_info(d).m_positionProperty.changed();
			}
		}

		bool showBothScrollBars = showScrollBar[(int)dimension::horizontal] && showScrollBar[(int)dimension::vertical];
		if (showBothScrollBars)
		{
			double vScrollBarWidth = get_scroll_bar_info(dimension::vertical).m_frame->get_fixed_width();
			double hScrollBarHeight = get_scroll_bar_info(dimension::horizontal).m_frame->get_fixed_height();
			get_scroll_bar_info(dimension::horizontal).m_frame->get_fixed_width() -= vScrollBarWidth;
			get_scroll_bar_info(dimension::vertical).m_frame->get_fixed_height() -= hScrollBarHeight;
			m_cornerFrame->get_bounds() = bounds(
				point(
					get_scroll_bar_info(dimension::vertical).m_frame->get_position().get_x(),
					get_scroll_bar_info(dimension::horizontal).m_frame->get_position().get_y()),
				size(
					vScrollBarWidth,
					hScrollBarHeight));
		}

		contentBounds.get_size() = m_contentFrame->propose_size(contentBounds.get_size()).find_first_valid_size(m_contentFrame->get_primary_flow_dimension());;
		m_contentFrame->get_bounds() = contentBounds;
		m_clippingFrame->get_bounds() = contentBounds & visibleBounds; // clip to smaller of content and visible bounds

		for (int i = 0; i < 2; i++)
		{
			dimension d = (dimension)i;
			if (has_scroll_bar(d))
			{
				if (get_scroll_bar_info(d).m_scrollBar->is_hidden() == showScrollBar[i])
					get_scroll_bar_info(d).m_scrollBar->show_or_hide(showScrollBar[i]);
			}
		}
		if (m_cornerPane->is_hidden() == showBothScrollBars)
			m_cornerPane->show_or_hide(showBothScrollBars);

		bool visible = (m_clippingFrame->get_bounds().get_width() != 0 && m_clippingFrame->get_bounds().get_height() != 0);
		if (m_clippingPane->is_hidden() == visible)
			m_clippingPane->show_or_hide(visible);

		pane::reshape(b, oldOrigin);
	}

	virtual bool wheel_moving(double distance, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		// Give child pane first chance to intercept it
		if (!pane::wheel_moving(distance, pt, modifiers))
		{
			dimension scrollDimension = dimension::horizontal;
			if (has_scroll_bar(dimension::vertical)) // If shift is held, scroll horizontally
			{
				if (!has_scroll_bar(dimension::horizontal) || !modifiers.get_key(ui::modifier_key::shift_key))
					scrollDimension = dimension::vertical;
			}
			else if (!has_scroll_bar(dimension::horizontal))
				return false;
			get_scroll_bar_info(scrollDimension).m_scrollBar->scroll(distance);
		}
		return true;
	}

	virtual rcref<dependency_property<bool> > get_should_auto_fade_scroll_bar_property() { return get_self_rcref(&m_shouldAutoFadeScrollBarProperty); }
};


}
}


#endif
