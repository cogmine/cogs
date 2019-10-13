//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_GUI_FRAME
#define COGS_HEADER_GUI_FRAME


#include "cogs/geometry/cell.hpp"
#include "cogs/gfx/canvas.hpp"


namespace cogs {
namespace gui {


typedef gfx::canvas::size size;
typedef gfx::canvas::point point;
typedef gfx::canvas::bounds bounds;
typedef gfx::canvas::range range;
typedef gfx::canvas::margin margin;
typedef gfx::canvas::proportion proportion;
typedef gfx::canvas::direction direction;
typedef gfx::canvas::dimension dimension;
typedef gfx::canvas::flow flow;
typedef gfx::canvas::script_flow script_flow;
typedef gfx::canvas::cell cell;
typedef gfx::canvas::alignment alignment;
typedef gfx::canvas::proportional_sizing_group proportional_sizing_group;
typedef gfx::canvas::fair_sizing_group fair_sizing_group;
typedef gfx::canvas::equal_sizing_group equal_sizing_group;

typedef gfx::canvas::font font;
typedef gfx::canvas::bitmap bitmap;
typedef gfx::canvas::bitmask bitmask;


// frame and cell facilitate sizing/resizing behavior of 2D rectangular elements.

// The base frame passes everything through to the equivalent methods in contained cell.
class frame : public cell
{
private:
	rcptr<cell> m_cell;

	// Preserved position from last reshape, in parent coordinates (in which 0,0 is parent's origin).
	point m_childPosition;

protected:
	void aligned_reshape(const alignment& a, const size& calculatedSize, const bounds& originalBounds, const point& oldOrigin = point(0, 0))
	{
		size remaining = originalBounds.get_size() - calculatedSize;
		size newChildPosition = remaining * a;
		point childOldPosition = get_child_position(); // not set first time called, but oldOrigin is undefined then anyway.
		bounds b(originalBounds.get_position() + newChildPosition, calculatedSize);
		frame::reshape(b, oldOrigin + (childOldPosition - b.get_position()));
	}

public:
	const rcptr<cell>& get_cell() const { return m_cell; }

	const rcptr<cell>& get_innermost_cell() const
	{
		rcptr<const frame> f = this;
		rcptr<cell> r = m_cell;
		while (!!r && r->is_frame())
		{
			f = r.template static_cast_to<frame>();
			r = f->get_cell();
		}
		return f->get_cell();
	}

	frame(const rcref<cell>& r)
		: m_cell(r),
		m_childPosition(0, 0)
	{ }

	virtual bool is_frame() const { return true; }

	virtual point get_child_position() const
	{
		return m_childPosition;
	}

	point get_innermost_child_position() const
	{
		rcptr<const frame> f = this;
		rcptr<cell> r = m_cell;
		while (!!r && r->is_frame())
		{
			f = r.template static_cast_to<frame>();
			r = f->get_cell();
		}
		return f->get_child_position();
	}

	virtual size get_default_size() const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->get_default_size();
		return size(0, 0);
	}

	virtual range get_range() const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->get_range();
		return range();
	}

	virtual dimension get_primary_flow_dimension() const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->get_primary_flow_dimension();
		return dimension::horizontal;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->propose_length(d, proposed, rtnOtherRange);
		return proposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->propose_lengths(d, proposedSize);
		return proposedSize;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			return r->propose_size(proposedSize);
		return proposedSize;
	}

protected:
	virtual void calculate_range()
	{
		rcptr<cell> r = m_cell;
		if (!!r)
			cell::calculate_range(*r);
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		m_childPosition = b.get_position();
		cell::reshape(b, oldOrigin);
		rcptr<cell> c = m_cell;
		if (!!c)
			cell::reshape(*c, b, oldOrigin);
	}
};


class override_default_size_frame : public frame
{
private:
	size m_defaultSize;

public:
	override_default_size_frame(const rcref<cell>& r, const size& defaultSize)
		: frame(r),
		m_defaultSize(defaultSize)
	{ }

	virtual size get_default_size() const { return propose_size(m_defaultSize); }
};


class inset_frame : public frame
{
private:
	margin m_margin;

public:
	inset_frame(const rcref<cell>& r, const margin& m)
		: frame(r),
		m_margin(m)
	{ }

	virtual size get_default_size() const
	{
		return frame::get_default_size() + m_margin;
	}

	virtual range get_range() const
	{
		return frame::get_range() + m_margin;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double marginTotal = m_margin[d].get_size();
		if (proposed > marginTotal)
			proposed -= marginTotal;
		else
			proposed = 0;
		proposed = frame::propose_length(d, proposed, rtnOtherRange);
		rtnOtherRange += m_margin[!d].get_size();
		return proposed + marginTotal;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size newProposedSize = proposedSize;
		double marginTotalWidth = m_margin.get_width();
		if (newProposedSize.get_width() > marginTotalWidth)
			newProposedSize.get_width() -= marginTotalWidth;
		else
			newProposedSize.get_width() = 0;
		double marginTotalHeight = m_margin.get_height();
		if (newProposedSize.get_height() > marginTotalHeight)
			newProposedSize.get_height() -= marginTotalHeight;
		else
			newProposedSize.get_height() = 0;
		return frame::propose_lengths(d, newProposedSize) + m_margin.get_size();
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size newProposedSize = proposedSize;
		double marginTotalWidth = m_margin.get_width();
		if (newProposedSize.get_width() > marginTotalWidth)
			newProposedSize.get_width() -= marginTotalWidth;
		else
			newProposedSize.get_width() = 0;
		double marginTotalHeight = m_margin.get_height();
		if (newProposedSize.get_height() > marginTotalHeight)
			newProposedSize.get_height() -= marginTotalHeight;
		else
			newProposedSize.get_height() = 0;
		return frame::propose_size(newProposedSize) + m_margin.get_size();
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size topLeftMargin = m_margin.get_top_left();
		size calculatedSize = b.get_size();
		double marginTotalWidth = m_margin.get_width();
		if (calculatedSize.get_width() > marginTotalWidth)
			calculatedSize.get_width() -= marginTotalWidth;
		else
		{
			topLeftMargin.get_width() = calculatedSize.get_width();
			calculatedSize.get_width() = 0;
		}
		double marginTotalHeight = m_margin.get_height();
		if (calculatedSize.get_height() > marginTotalHeight)
			calculatedSize.get_height() -= marginTotalHeight;
		else
		{
			topLeftMargin.get_height() = calculatedSize.get_height();
			calculatedSize.get_height() = 0;
		}

		point childOldPosition = get_child_position();
		bounds r2(b.get_position() + topLeftMargin, calculatedSize);
		point childOldOrigin = oldOrigin + (childOldPosition - r2.get_position());
		frame::reshape(r2, childOldOrigin);
	}
};


class fixed_default_size_frame : public frame
{
private:
	alignment m_alignment;

public:
	fixed_default_size_frame(const rcref<cell>& r, const alignment& a = alignment::center())
		: frame(r),
		m_alignment(a)
	{
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

	virtual range get_range() const
	{
		size sz = get_default_size();
		range r(sz, sz, true, true);
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		size sz = get_default_size();
		rtnOtherRange.set_fixed(sz[!d]);
		return sz[d];
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		return get_default_size();
	}

	virtual size propose_size(const size& proposedSize) const
	{
		return get_default_size();
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		aligned_reshape(m_alignment, get_default_size(), b, oldOrigin);
	}

};


class fixed_size_frame : public frame
{
private:
	size m_size;

public:
	fixed_size_frame(const rcref<cell>& r, const size& sz)
		: frame(r),
		m_size(sz)
	{ }

	fixed_size_frame(const rcref<cell>& r)
		: frame(r),
		m_size(0, 0)
	{ }

	size& get_fixed_size() { return m_size; }
	const size& get_fixed_size() const { return m_size; }

	double& get_fixed_size(dimension d) { return m_size[d]; }
	const double get_fixed_size(dimension d) const { return m_size[d]; }

	double& get_fixed_height() { return m_size.get_height(); }
	const double get_fixed_height() const { return m_size.get_height(); }

	double& get_fixed_width() { return m_size.get_width(); }
	const double get_fixed_width() const { return m_size.get_width(); }

	virtual size get_default_size() const
	{
		return frame::propose_size(m_size);
	}

	virtual range get_range() const
	{
		size sz = frame::propose_size(m_size);
		range r(sz, sz, true, true);
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		size sz = frame::propose_lengths(d, m_size);
		rtnOtherRange.set_fixed(sz[!d]);
		return sz[d];
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		return frame::propose_lengths(d, m_size);
	}

	virtual size propose_size(const size& proposedSize) const
	{
		return frame::propose_size(m_size);
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(m_size);
		frame::reshape(bounds(b.get_position(), sz), oldOrigin);
	}
};

class fixed_position_frame : public frame
{
private:
	point m_position;

public:
	fixed_position_frame(const rcref<cell>& r, const point& pt)
		: frame(r),
		m_position(pt)
	{ }

	fixed_position_frame(const rcref<cell>& r)
		: frame(r),
		m_position(0, 0)
	{ }

	point& get_position() { return m_position; }
	const point& get_position() const { return m_position; }

	double& get_position(dimension d) { return m_position[d]; }
	const double get_position(dimension d) const { return m_position[d]; }

	double& get_x() { return m_position.get_x(); }
	const double get_x() const { return m_position.get_x(); }

	double& get_y() { return m_position.get_y(); }
	const double get_y() const { return m_position.get_y(); }

	virtual size get_default_size() const
	{
		return frame::get_default_size() + m_position.to_size();
	}

	virtual range get_range() const
	{
		return frame::get_range() + m_position.to_size();
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		if (proposed > m_position[d])
			proposed -= m_position[d];
		else
			proposed = 0;

		double result = frame::propose_length(d, proposed, rtnOtherRange);
		result += m_position[d];
		rtnOtherRange += m_position[!d];
		return result;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size newProposedSize;
		if (proposedSize[dimension::horizontal] > m_position[dimension::horizontal])
			newProposedSize[dimension::horizontal] = proposedSize[dimension::horizontal] - m_position[dimension::horizontal];
		else
			newProposedSize[dimension::horizontal] = 0;
		if (proposedSize[dimension::vertical] > m_position[dimension::vertical])
			newProposedSize[dimension::vertical] = proposedSize[dimension::vertical] - m_position[dimension::vertical];
		else
			newProposedSize[dimension::vertical] = 0;
		return frame::propose_lengths(d, newProposedSize) + m_position.to_size();
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size newProposedSize;
		if (proposedSize[dimension::horizontal] > m_position[dimension::horizontal])
			newProposedSize[dimension::horizontal] = proposedSize[dimension::horizontal] - m_position[dimension::horizontal];
		else
			newProposedSize[dimension::horizontal] = 0;
		if (proposedSize[dimension::vertical] > m_position[dimension::vertical])
			newProposedSize[dimension::vertical] = proposedSize[dimension::vertical] - m_position[dimension::vertical];
		else
			newProposedSize[dimension::vertical] = 0;
		return frame::propose_size(newProposedSize) + m_position.to_size();
	}


protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		bounds newBounds;
		newBounds.set_position(b.get_position() + m_position);
		newBounds.set_size(b.get_size() - m_position.to_size());
		point childOldPosition = get_child_position();
		point childOldOrigin = oldOrigin + (childOldPosition - newBounds.get_position());
		frame::reshape(newBounds, childOldOrigin);
	}
};

class fixed_bounds_frame : public frame
{
private:
	bounds m_bounds;

public:
	fixed_bounds_frame(const rcref<cell>& r, const bounds& b)
		: frame(r),
		m_bounds(b)
	{ }

	fixed_bounds_frame(const rcref<cell>& r)
		: frame(r),
		m_bounds(0, 0, 0, 0)
	{ }

	bounds& get_bounds() { return m_bounds; }
	const bounds& get_bounds() const { return m_bounds; }

	point& get_position() { return m_bounds.get_position(); }
	const point& get_position() const { return m_bounds.get_position(); }

	double& get_position(dimension d) { return m_bounds.get_position(d); }
	const double get_position(dimension d) const { return m_bounds.get_position(d); }

	size& get_fixed_size() { return m_bounds.get_size(); }
	const size& get_fixed_size() const { return m_bounds.get_size(); }

	double& get_fixed_size(dimension d) { return m_bounds.get_size(d); }
	const double get_fixed_size(dimension d) const { return m_bounds.get_size(d); }

	double& get_fixed_height() { return m_bounds.get_height(); }
	const double get_fixed_height() const { return m_bounds.get_height(); }

	double& get_fixed_width() { return m_bounds.get_width(); }
	const double get_fixed_width() const { return m_bounds.get_width(); }

	virtual size get_default_size() const
	{
		if (!m_bounds)
			return frame::get_default_size();
		return frame::propose_size(m_bounds.get_size()) + m_bounds.get_position().to_size();
	}

	virtual range get_range() const
	{
		if (!m_bounds)
			return frame::get_range();

		size sz = frame::propose_size(m_bounds.get_size()) + m_bounds.get_position().to_size();
		range r(sz);
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		if (!m_bounds)
			return propose_length(d, proposed, rtnOtherRange);

		size sz = frame::propose_size(m_bounds.get_size());
		rtnOtherRange.set_fixed(sz[!d]);
		return sz[d];
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		return frame::propose_lengths(d, !m_bounds ? proposedSize : m_bounds.get_size());
	}

	virtual size propose_size(const size& proposedSize) const
	{
		return frame::propose_size(!m_bounds ? proposedSize : m_bounds.get_size());
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		if (!m_bounds)
			frame::reshape(b, oldOrigin);
		else
		{
			size sz = frame::propose_size(m_bounds.get_size());
			bounds newBounds = { m_bounds.get_position() + b.get_position(), sz };
			point childOldPosition = get_child_position();
			point childOldOrigin = oldOrigin + (childOldPosition - newBounds.get_position());
			frame::reshape(newBounds, childOldOrigin);
		}
	}
};


// Like fixed_bounds_frame, but only applies the fixed bounds on reshape().
// All other calls behave as if no fixed size is set.
// This is useful in the case of a parent pane directly handing the shapes of internally managed descendant panes.
class override_bounds_frame : public frame
{
private:
	bounds m_bounds;

public:
	override_bounds_frame(const rcref<cell>& r, const bounds& b)
		: frame(r),
		m_bounds(b)
	{ }

	override_bounds_frame(const rcref<cell>& r)
		: frame(r),
		m_bounds(0, 0, 0, 0)
	{ }

	bounds& get_fixed_bounds() { return m_bounds; }
	const bounds& get_fixed_bounds() const { return m_bounds; }

	point& get_position() { return m_bounds.get_position(); }
	const point& get_position() const { return m_bounds.get_position(); }

	double& get_position(dimension d) { return m_bounds.get_position(d); }
	const double get_position(dimension d) const { return m_bounds.get_position(d); }

	size& get_fixed_size() { return m_bounds.get_size(); }
	const size& get_fixed_size() const { return m_bounds.get_size(); }

	double& get_fixed_size(dimension d) { return m_bounds.get_size(d); }
	const double get_fixed_size(dimension d) const { return m_bounds.get_size(d); }

	double& get_fixed_height() { return m_bounds.get_height(); }
	const double get_fixed_height() const { return m_bounds.get_height(); }

	double& get_fixed_width() { return m_bounds.get_width(); }
	const double get_fixed_width() const { return m_bounds.get_width(); }

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		if (!m_bounds)
			frame::reshape(b, oldOrigin);
		else
		{
			size sz = frame::propose_size(m_bounds.get_size());
			bounds newBounds = { m_bounds.get_position() + b.get_position(), sz };
			point childOldPosition = get_child_position();
			point childOldOrigin = oldOrigin + (childOldPosition - newBounds.get_position());
			frame::reshape(newBounds, childOldOrigin);
		}
	}
};


// fixed_width_frame?
// fixed_height_frame?


class limit_range_frame : public frame
{
private:
	range m_range;

public:
	limit_range_frame(const rcref<cell>& r, const range& rng)
		: frame(r),
		m_range(rng)
	{ }

	virtual size get_default_size() const { return get_range().limit(frame::get_default_size()); }

	virtual range get_range() const { return m_range & frame::get_range(); }

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		range r = get_range();
		double newProposed = r[d].limit(proposed);
		newProposed = frame::propose_length(d, newProposed, rtnOtherRange);
		rtnOtherRange &= r[!d];
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_lengths(d, sz);
	}

	virtual size propose_size(const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_size(sz);
	}
};


class unstretchable_frame : public frame
{
public:
	unstretchable_frame(const rcref<cell>& r)
		: frame(r)
	{ }

	virtual range get_range() const
	{
		range stretchRange(size(0, 0), get_default_size(), true, true);
		return frame::get_range() & stretchRange;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		range r = get_range();
		double newProposed = r[d].limit(proposed);
		newProposed = frame::propose_length(d, newProposed, rtnOtherRange);
		rtnOtherRange &= r[!d];
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_lengths(d, sz);
	}

	virtual size propose_size(const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_size(sz);
	}
};


class unshrinkable_frame : public frame
{
public:
	unshrinkable_frame(const rcref<cell>& r)
		: frame(r)
	{ }

	virtual range get_range() const
	{
		range shrinkRange(get_default_size(), size(0, 0), false, false);
		return frame::get_range() & shrinkRange;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		range r = get_range();
		double newProposed = r[d].limit(proposed);
		newProposed = frame::propose_length(d, newProposed, rtnOtherRange);
		rtnOtherRange &= r[!d];
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_lengths(d, sz);
	}

	virtual size propose_size(const size& proposedSize) const
	{
		range r = get_range();
		size sz = r.limit(proposedSize);
		return frame::propose_size(sz);
	}
};


// Maybe need 2 separate classes?
// propose_aspect_ratio_frame - Proposes new size with aspect ratio, but allows content cell to decide
// maintain_aspect_ratio_frame2? - Alway maintains the aspect ratio (based on default size), and aligns contents if necessary.

class propose_aspect_ratio_frame : public frame
{
public:
	propose_aspect_ratio_frame(const rcref<cell>& r)
		: frame(r)
	{ }

	virtual range get_range() const
	{
		range r = frame::get_range();
		size sz = frame::get_default_size();
		if (!!sz.get_width() && !!sz.get_height())
		{
			if (r.get_min_width() != 0)
			{
				double newMinHeight = (r.get_min_width() * sz.get_width()) / sz.get_height();
				if (r.get_min_height() != 0)
				{
					if (r.get_min_height() <= newMinHeight)
						r.get_min_height() = newMinHeight;
					else
					{
						double newMinWidth = (r.get_min_height() * sz.get_height()) / sz.get_width();
						COGS_ASSERT(r.get_min_width() < newMinWidth);
						r.get_min_width() = newMinWidth;
					}
				}
				else
					r.get_min_height() = newMinHeight;
			}
			else if (r.get_min_height() != 0)
			{
				double newMinWidth = (r.get_min_height() * sz.get_height()) / sz.get_width();
				r.get_min_width() = newMinWidth;
			}

			if (r.has_max_width())
			{
				double newMaxHeight = (r.get_max_width() * sz.get_width()) / sz.get_height();
				if (r.has_max_height())
				{
					if (r.get_max_height() >= newMaxHeight)
						r.get_max_height() = newMaxHeight;
					else
					{
						double newMaxWidth = (r.get_max_height() * sz.get_height()) / sz.get_width();
						COGS_ASSERT(r.get_max_width() < newMaxWidth);
						r.get_max_width() = newMaxWidth;
					}
				}
				else
				{
					r.get_max_height() = newMaxHeight;
					r.get_max_height() = true;
				}
			}
			else if (r.has_max_height())
			{
				double newMaxWidth = (r.get_max_height() * sz.get_height()) / sz.get_width();
				r.get_max_width() = newMaxWidth;
				r.has_max_width() = true;
			}
		}

		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		size newSize;
		range r = get_range();
		newSize[d] = r[d].limit(proposed);
		size sz = frame::get_default_size();
		newSize[!d] = (newSize[d] * sz[!d]) / sz[d];
		newSize = frame::propose_size(newSize);
		rtnOtherRange.set_fixed(newSize[!d]);
		return newSize[d];
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size newProposedSize = get_range().limit(proposedSize);
		size sz = frame::get_default_size();
		sz[!d] = ((newProposedSize[d] * sz[!d]) / sz[d]);
		sz[d] = newProposedSize[d];
		return frame::propose_size(sz);
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size result;
		size newProposedSize = get_range().limit(proposedSize);
		size sz = frame::get_default_size();
		dimension d = get_primary_flow_dimension();
		result[!d] = cogs::ceil((newProposedSize[d] * sz[!d]) / sz[d]);
		if (result[!d] <= newProposedSize[!d])
			result[d] = newProposedSize[d];
		else
		{
			result[!d] = newProposedSize[!d];
			result[d] = cogs::ceil((newProposedSize[!d] * sz[d]) / sz[!d]);
		}
		return result;
	}
};


class proportional_frame : public frame
{
private:
	proportion m_proportionalSize; // proportion of enclosing area to fill
	alignment m_alignment; // positioning in parent

public:
	proportional_frame(const rcref<cell>& r, const proportion& p, const alignment& a = alignment::center())
		: frame(r),
		m_proportionalSize(p),
		m_alignment(a)
	{
		if (m_proportionalSize[dimension::horizontal] > 1.0)
			m_proportionalSize[dimension::horizontal] = 1.0;
		if (m_proportionalSize[dimension::vertical] > 1.0)
			m_proportionalSize[dimension::vertical] = 1.0;
		if (m_proportionalSize[dimension::horizontal] < 0.0)
			m_proportionalSize[dimension::horizontal] = 0.0;
		if (m_proportionalSize[dimension::vertical] < 0.0)
			m_proportionalSize[dimension::vertical] = 0.0;
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

	virtual size get_default_size() const
	{
		size sz = frame::get_default_size();
		sz /= m_proportionalSize;
		return sz;
	}

	virtual range get_range() const
	{
		range r = frame::get_range();
		r /= m_proportionalSize;
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double newProposed = proposed * m_proportionalSize[d];
		newProposed = frame::propose_length(d, newProposed, rtnOtherRange);
		rtnOtherRange /= m_proportionalSize[!d];
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size sz = proposedSize * m_proportionalSize;
		return frame::propose_lengths(d, sz) / m_proportionalSize;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size sz = proposedSize * m_proportionalSize;
		return frame::propose_size(sz) / m_proportionalSize;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = b.get_size() * m_proportionalSize;
		aligned_reshape(m_alignment, sz, b, oldOrigin);
	}
};


class unconstrained_frame : public frame
{
private:
	alignment m_alignment; // positioning in parent, if smaller than requested size

public:
	unconstrained_frame(const rcref<cell>& r, const alignment& a = alignment::center())
		: frame(r),
		m_alignment(a)
	{
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

	virtual range get_range() const
	{
		range r(size(0, 0), size(0, 0), false, false);
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		return proposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		return proposedSize;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		return proposedSize;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size());
		aligned_reshape(m_alignment, sz, b, oldOrigin);
	}
};



class unconstrained_min_frame : public frame
{
private:
	alignment m_alignment;

public:
	unconstrained_min_frame(const rcref<cell>& r, const alignment& a = alignment::center())
		: frame(r),
		m_alignment(a)
	{
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.get_min_height() = 0;
		r.get_min_width() = 0;
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double newProposed = frame::propose_length(d, proposed, rtnOtherRange);
		if (newProposed > proposed)
			newProposed = proposed;
		rtnOtherRange.get_min() = 0;
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size sz = frame::propose_lengths(d, proposedSize);
		if (sz.get_height() > proposedSize.get_height())
			sz.get_height() = proposedSize.get_height();
		if (sz.get_width() > proposedSize.get_width())
			sz.get_width() = proposedSize.get_width();
		return sz;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size sz = frame::propose_size(proposedSize);
		if (sz.get_height() > proposedSize.get_height())
			sz.get_height() = proposedSize.get_height();
		if (sz.get_width() > proposedSize.get_width())
			sz.get_width() = proposedSize.get_width();
		return sz;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size());
		aligned_reshape(m_alignment, sz, b, oldOrigin);
	}

};

class unconstrained_max_frame : public frame
{
private:
	alignment m_alignment;

public:
	unconstrained_max_frame(const rcref<cell>& r, const alignment& a = alignment::center())
		: frame(r),
		m_alignment(a)
	{
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.has_max_height() = false;
		r.has_max_width() = false;
		return r;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double newProposed = frame::propose_length(d, proposed, rtnOtherRange);
		if (newProposed < proposed)
			newProposed = proposed;
		rtnOtherRange.has_max() = false;
		return newProposed;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size sz = frame::propose_lengths(d, proposedSize);
		if (sz.get_height() < proposedSize.get_height())
			sz.get_height() = proposedSize.get_height();
		if (sz.get_width() < proposedSize.get_width())
			sz.get_width() = proposedSize.get_width();
		return sz;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		size sz = frame::propose_size(proposedSize);
		if (sz.get_height() < proposedSize.get_height())
			sz.get_height() = proposedSize.get_height();
		if (sz.get_width() < proposedSize.get_width())
			sz.get_width() = proposedSize.get_width();
		return sz;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size());
		aligned_reshape(m_alignment, sz, b, oldOrigin);
	}
};


}
}


#endif
