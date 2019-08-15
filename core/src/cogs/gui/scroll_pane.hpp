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
	scroll_horizontally					= 0x01,	// 01
	scroll_vertically					= 0x02,	// 10
	scroll_horizontally_and_vertically	= 0x03	// 11
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
		rcptr<override_bounds_frame> m_scrollBarFrame;
		rcptr<scroll_bar> m_scrollBar;
		volatile transactable<scroll_bar_state>	m_scrollBarState;
		volatile double m_scrollBarPosition;
		delayed_construction<delegated_dependency_property<scroll_bar_state, io::read_only> > m_scrollBarStateProperty;
		delayed_construction<delegated_dependency_property<double> > m_scrollBarPositionProperty;

		scroll_bar_info()
			: m_scrollBarPosition(0)
		{ }
	};

	scroll_bar_info m_scrollBarInfo[2];

	rcptr<container_pane> m_contentPane;
	rcptr<native_container_pane> m_clippingPane;	// using a native pane ensures clipping of platform dependent pane children (i.e. OS buttons, etc.)
	rcptr<container_pane> m_cornerPane;

	rcptr<override_bounds_frame> m_contentFrame;
	rcptr<override_bounds_frame> m_clippingFrame;
	rcptr<override_bounds_frame> m_cornerFrame;

	range m_calculatedRange;
	size m_calculatedDefaultSize;

	bool m_hideInactiveScrollBar;
	bool m_allowScrollBarOverlay;

	bool has_scroll_bar(dimension d) const { return !!m_scrollBarInfo[(int)d].m_scrollBar; }

	void scrolled()
	{
		point oldOrigin = m_contentFrame->get_position();
		point oldScrollPosition = -oldOrigin;

		double hScrollPosition = atomic::load(m_scrollBarInfo[0].m_scrollBarPosition);
		double vScrollPosition = atomic::load(m_scrollBarInfo[1].m_scrollBarPosition);

		point newScrollPosition(hScrollPosition, vScrollPosition);
		if (oldScrollPosition != newScrollPosition)
		{
			m_contentFrame->get_position() = -newScrollPosition;
			cell::reshape(*m_contentFrame, m_contentFrame->get_fixed_bounds(), oldOrigin);
		}
	}

	bool use_scroll_bar_overlay() const
	{
		if (!m_allowScrollBarOverlay)
			return false;
		dimension d = dimension::vertical;
		if (!has_scroll_bar(d))
			d = !d;
		return m_scrollBarInfo[(int)d].m_scrollBar->can_overlay();
	}

	// Removes inactive scroll bars
	size propose_inner(const size& proposedSize) const
	{
		size sz = proposedSize;
		if (use_scroll_bar_overlay())
			return sz;

		if (m_hideInactiveScrollBar)
		{
			range contentRange = m_contentFrame->get_range();
			size contentMins = contentRange.get_min();
			for (;;)
			{
				const dimension d = dimension::horizontal;	// Doesn't matter which, logic is symmetrical
				double min = contentMins[d];
				if (has_scroll_bar(d))
				{
					if (has_scroll_bar(!d))
					{
						if (contentRange.get_min(d) > 0)
							if (contentRange.get_min(!d) > 0)
								if ((sz[d] >= min) && (sz[!d] >= contentMins[!d]))	// remove both scroll bars
								{
									if (contentRange.has_max(d) && contentRange.get_max(d) == contentMins[d])
										sz[d] = contentMins[d];
									if (contentRange.has_max(!d) && contentRange.get_max(!d) == contentMins[!d])
										sz[!d] = contentMins[!d];
									break;
								}
						min += m_scrollBarInfo[(int)!d].m_scrollBar->get_const_cell().get_default_size()[d];
					}

					if (sz[d] >= min)
					{
						if (contentRange.has_max(!d))
							sz[!d] = contentRange[!d].limit_max(sz[!d]);
					}
					else
					{
						double minWithScrollBar = m_calculatedRange.get_min(!d) + m_scrollBarInfo[(int)d].m_scrollBar->get_const_cell().get_default_size()[!d];
						if (sz[!d] < minWithScrollBar)
							sz[!d] = minWithScrollBar;
					}
				}

				if (has_scroll_bar(!d))
				{
					if (sz[!d] >= contentMins[!d])
					{
						if (contentRange.has_max(d))
							sz[d] = contentRange[d].limit_max(sz[d]);
					}
					else
					{
						double minWithScrollBar = m_calculatedRange.get_min(d) + m_scrollBarInfo[(int)!d].m_scrollBar->get_const_cell().get_default_size()[d];
						if (sz[d] < minWithScrollBar)
							sz[d] = minWithScrollBar;
					}
				}

				break;
			}
		}
		return sz;
	}

public:
	explicit scroll_pane(const ptr<rc_obj_base>& desc, scroll_dimensions scrollDimensions = scroll_horizontally_and_vertically, bool hideInactiveScrollBar = true, bool allowScrollBarOverlay = true)
		: pane(desc),
		m_hideInactiveScrollBar(hideInactiveScrollBar),
		m_allowScrollBarOverlay(allowScrollBarOverlay)
	{
		m_contentPane = container_pane::create();

		// TODO: May need to address what happens when a native control is offscreen when drawn, and backing buffer is unavailable
		//m_contentPane->set_compositing_behavior(compositing_behavior::buffer_self_and_children);
		m_clippingPane = native_container_pane::create();
		m_cornerPane = container_pane::create();

		m_contentFrame = rcnew(override_bounds_frame, m_contentPane.dereference());
		rcref<unconstrained_frame> unconstrainedFrame = rcnew(unconstrained_frame, m_contentFrame.dereference(), alignment(0, 0));

		m_clippingFrame = rcnew(override_bounds_frame, m_clippingPane.dereference());
		m_cornerFrame = rcnew(override_bounds_frame, m_cornerPane.dereference());

		m_clippingPane->nest(m_contentPane.dereference(), unconstrainedFrame);
		pane::nest_last(m_clippingPane.dereference(), m_clippingFrame);
		pane::nest_last(m_cornerPane.dereference(), m_cornerFrame);

		bool scrollDimension[2];
		scrollDimension[0] = ((scrollDimensions & scroll_horizontally) != 0);
		scrollDimension[1] = ((scrollDimensions & scroll_vertically) != 0);

		for (int i = 0; i < 2; i++)
		{
			placement_rcnew(&m_scrollBarInfo[i].m_scrollBarStateProperty.get(), this_desc, *this, [this, i]()
				{
					return *(m_scrollBarInfo[i].m_scrollBarState.begin_read());
				});

			placement_rcnew(&m_scrollBarInfo[i].m_scrollBarPositionProperty.get(), this_desc, *this, [this, i]()
				{
					return atomic::load(m_scrollBarInfo[i].m_scrollBarPosition);
				}, [this, i](double d)
				{
					double newPos = d;
					double oldPos = atomic::exchange(m_scrollBarInfo[i].m_scrollBarPosition, newPos);
					if (oldPos != newPos)
					{
						m_scrollBarInfo[i].m_scrollBarPositionProperty->changed();
						scrolled();
					}
					m_scrollBarInfo[i].m_scrollBarPositionProperty->set_complete();
				});

			if (scrollDimension[i])
			{
				m_scrollBarInfo[i].m_scrollBar = rcnew(scroll_bar, (dimension)i, hideInactiveScrollBar);
				m_scrollBarInfo[i].m_scrollBarFrame = rcnew(override_bounds_frame, m_scrollBarInfo[i].m_scrollBar.dereference());
				m_scrollBarInfo[i].m_scrollBarFrame->get_position().get_x() = 0;
				m_scrollBarInfo[i].m_scrollBarFrame->get_position().get_y() = 0;
				pane::nest_last(m_scrollBarInfo[i].m_scrollBar.dereference(), m_scrollBarInfo[i].m_scrollBarFrame);
				m_scrollBarInfo[i].m_scrollBarStateProperty->bind_to(m_scrollBarInfo[i].m_scrollBar->get_state_property());
				m_scrollBarInfo[i].m_scrollBarPositionProperty->bind(m_scrollBarInfo[i].m_scrollBar->get_position_property());
			}
		}
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
				m_scrollBarInfo[i].m_scrollBarFrame->get_fixed_size(!d) = m_scrollBarInfo[i].m_scrollBar->get_const_cell().get_default_size()[!d];
		}

		m_calculatedRange.set_min(0, 0);
		m_calculatedRange.clear_max();

		// default to size of contents
		m_calculatedDefaultSize = m_contentPane->get_default_size();
		
		bool overlay = use_scroll_bar_overlay();
		if (!m_hideInactiveScrollBar && !overlay)
		{
			for (int i = 0; i < 2; i++)
			{
				dimension d = (dimension)i;
				if (has_scroll_bar(d))
					m_calculatedDefaultSize[!d] += m_scrollBarInfo[i].m_scrollBarFrame->get_fixed_size(!d);
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
				if (has_scroll_bar(!!d) && !overlay)
					m_calculatedRange.get_max(!d) += m_scrollBarInfo[i].m_scrollBar->get_const_cell().get_default_size()[!d];
			}

			if (!has_scroll_bar(!d))
			{
				m_calculatedRange.set_min(!d, contentRange.get_min(!d));
				if (!m_hideInactiveScrollBar && has_scroll_bar(!!d) && !overlay)
					m_calculatedRange.get_min(!d) += m_scrollBarInfo[i].m_scrollBar->get_const_cell().get_default_size()[!d];
			}
		}
	}

	virtual double propose_length(dimension d, double proposed, geometry::linear::range& rtnOtherRange) const
	{
		return cell::propose_length(d, proposed, rtnOtherRange);
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		return propose_inner(cell::propose_lengths(d, proposedSize));
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size sz = cell::propose_size(proposedSize);
		return propose_inner(sz);
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

		bool overlay = use_scroll_bar_overlay();
		auto reduce_content_bounds = [&](dimension d)
		{
			if (!overlay)
				visibleBounds.get_size()[d] -= m_scrollBarInfo[(int)!d].m_scrollBarFrame->get_fixed_size()[d];
			if (visibleBounds.get_size()[d] < contentBounds.get_size()[d])
			{
				contentBounds.get_size()[d] = visibleBounds.get_size()[d];
				if (contentBounds.get_size()[d] < contentRange.get_min()[d])
					contentBounds.get_size()[d] = contentRange.get_min()[d];
			}
		};

		bool showScrollBar[2];
		bool hasScrollBar[2] = { has_scroll_bar((dimension)0), has_scroll_bar((dimension)1) };

		if (!m_hideInactiveScrollBar)
		{
			// If not auto-hide, always include them.
			showScrollBar[0] = hasScrollBar[0];
			showScrollBar[1] = hasScrollBar[1];
			if (showScrollBar[0])
				reduce_content_bounds((dimension)1);
			if (showScrollBar[1])
				reduce_content_bounds((dimension)0);
		}
		else
		{
			showScrollBar[0] = false;
			showScrollBar[1] = false;
			bool hasBothScrollBars = hasScrollBar[0] && hasScrollBar[1];
			if (!hasBothScrollBars)
			{
				// If auto-hide and only 1 dimension...
				dimension d = hasScrollBar[0] ? (dimension)0 : (dimension)1;
				if (contentBounds.get_size()[d] > visibleBounds.get_size()[d])
				{
					showScrollBar[(int)d] = true;
					reduce_content_bounds(!d);
				}
			}
			else
			{
				int neededPrevDimension = 0;	// 1 = yes, -1 = new, 0 not checked yet
				dimension d = dimension::horizontal;	// doesn't matter which dimension is used first, it's symetrical
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
			if (hasScrollBar[i])
			{
				dimension d = (dimension)i;
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
					m_scrollBarInfo[i].m_scrollBarFrame->get_position()[!d] = b.get_size()[!d];
					m_scrollBarInfo[i].m_scrollBarFrame->get_position()[!d] -= m_scrollBarInfo[i].m_scrollBarFrame->get_fixed_size()[!d];
					m_scrollBarInfo[i].m_scrollBarState.set(scroll_bar_state(max, thumbSize));
				}
				else
				{
					contentBounds.get_position()[d] = 0;
					m_scrollBarInfo[i].m_scrollBarState.set(scroll_bar_state(0, 0));
				}
				atomic::store(m_scrollBarInfo[i].m_scrollBarPosition, pos);
				m_scrollBarInfo[i].m_scrollBarFrame->get_fixed_size()[d] = b.get_size()[d];
				m_scrollBarInfo[i].m_scrollBarStateProperty->changed();
				m_scrollBarInfo[i].m_scrollBarPositionProperty->changed();
			}
		}

		bool showBothScrollBars = showScrollBar[0] && showScrollBar[1];
		if (showBothScrollBars)
		{
			double vScrollBarWidth = m_scrollBarInfo[1].m_scrollBarFrame->get_fixed_width();
			double hScrollBarHeight = m_scrollBarInfo[0].m_scrollBarFrame->get_fixed_height();
			m_scrollBarInfo[0].m_scrollBarFrame->get_fixed_width() -= vScrollBarWidth;
			m_scrollBarInfo[1].m_scrollBarFrame->get_fixed_height() -= hScrollBarHeight;
			m_cornerFrame->get_fixed_bounds() = bounds(
				point(
					m_scrollBarInfo[1].m_scrollBarFrame->get_position().get_x(),
					m_scrollBarInfo[0].m_scrollBarFrame->get_position().get_y()),
				size(
					vScrollBarWidth,
					hScrollBarHeight));
		}

		contentBounds.get_size() = m_contentFrame->propose_size(contentBounds.get_size());
		m_contentFrame->get_fixed_bounds() = contentBounds;
		m_clippingFrame->get_fixed_bounds() = contentBounds & visibleBounds;	// clip to smaller of content and visible bounds

		for (int i = 0; i < 2; i++)
		{
			if (hasScrollBar[i])
			{
				if (m_scrollBarInfo[i].m_scrollBar->is_hidden() == showScrollBar[i])
					m_scrollBarInfo[i].m_scrollBar->show_or_hide(showScrollBar[i]);
			}
		}
		if (m_cornerPane->is_hidden() == showBothScrollBars)
			m_cornerPane->show_or_hide(showBothScrollBars);

		bool visible = (m_clippingFrame->get_fixed_bounds().get_width() != 0 && m_clippingFrame->get_fixed_bounds().get_height() != 0);
		if (m_clippingPane->is_hidden() == visible)
			m_clippingPane->show_or_hide(visible);

		pane::reshape(b, oldOrigin);
	}
};


}
}


#endif

