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

public:
	const rcptr<cell>& get_cell() const { return m_cell; }

	const rcptr<cell>& get_innermost_cell() const
	{
		rcptr<const frame> f = this;
		rcptr<cell> c = m_cell;
		while (!!c && c->is_frame())
		{
			f = c.template static_cast_to<frame>();
			c = f->get_cell();
		}
		return f->get_cell();
	}

	frame(const rcref<cell>& c)
		: m_cell(c),
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
		rcptr<cell> c = m_cell;
		while (!!c && c->is_frame())
		{
			f = c.template static_cast_to<frame>();
			c = f->get_cell();
		}
		return f->get_child_position();
	}

	virtual size get_default_size() const
	{
		rcptr<cell> c = m_cell;
		if (!!c)
			return c->get_default_size();
		COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
		return size(0, 0);
	}

	virtual range get_range() const
	{
		rcptr<cell> c = m_cell;
		if (!!c)
			return c->get_range();
		COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
		return cell::get_range();
	}

	virtual dimension get_primary_flow_dimension() const
	{
		rcptr<cell> c = m_cell;
		if (!!c)
			return c->get_primary_flow_dimension();
		COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
		return cell::get_primary_flow_dimension();
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		rcptr<cell> c = m_cell;
		if (!!c)
			return c->propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
		COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
		return cell::propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
	}

protected:
	virtual void calculate_range()
	{
		rcptr<cell> c = m_cell;
		if (!!c)
			cell::calculate_range(*c);
		else
			COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		m_childPosition = b.get_position();
		cell::reshape(b, oldOrigin);
		rcptr<cell> c = m_cell;
		if (!!c)
			cell::reshape(*c, b, oldOrigin);
		else
			COGS_ASSERT(false && "cogs::frame is not yet associated with a cell");
	}
};


// A frame that overrides the default size of the cell
class override_default_size_frame : public frame
{
private:
	size m_defaultSize;
	size m_calculatedDefaultSize;

	// Changes to a default size require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on our default size.  If the specified default size is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	override_default_size_frame(const rcref<cell>& c, const size& defaultSize)
		: frame(c),
		m_defaultSize(defaultSize)
	{ }

	override_default_size_frame(const rcref<cell>& c)
		: frame(c),
		m_defaultSize(0, 0)
	{ }

	size& get_default_size_override(const size& sz) { return m_defaultSize; }
	const size& get_default_size_override(const size& sz) const { return m_defaultSize; }
	void set_default_size_override(const size& sz) { m_defaultSize = sz; }

	virtual size get_default_size() const { return m_calculatedDefaultSize; }

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedDefaultSize = propose_size(m_defaultSize).find_first_valid_size(get_primary_flow_dimension());
	}
};


class aligned_frame_base : public frame
{
private:
	alignment m_alignment;

	void validate_alignment()
	{
		if (m_alignment[dimension::horizontal] > 1.0)
			m_alignment[dimension::horizontal] = 1.0;
		if (m_alignment[dimension::horizontal] < 0.0)
			m_alignment[dimension::horizontal] = 0.0;
		if (m_alignment[dimension::vertical] > 1.0)
			m_alignment[dimension::vertical] = 1.0;
		if (m_alignment[dimension::vertical] < 0.0)
			m_alignment[dimension::vertical] = 0.0;
	}

public:
	aligned_frame_base(const rcref<cell>& c, const alignment& a = alignment::center())
		: frame(c),
		m_alignment(a)
	{
		validate_alignment();
	}

	const alignment get_alignment() const { return m_alignment; }

	void set_alignment(const alignment& a)
	{
		m_alignment = a;
		validate_alignment();
	}

protected:
	void aligned_reshape(const bounds& calculatedBounds, const bounds& originalBounds, const point& oldOrigin = point(0, 0))
	{
		size calculatedSize = calculatedBounds.get_size() + calculatedBounds.get_position();
		size remaining = originalBounds.get_size() - calculatedSize;
		size newChildPosition = remaining * m_alignment;
		point childOldPosition = get_child_position(); // not set first time called, but oldOrigin is undefined then anyway.
		bounds b(originalBounds.get_position() + newChildPosition + calculatedBounds.get_position(), calculatedBounds.get_size());
		frame::reshape(b, oldOrigin + (childOldPosition - b.get_position()));
	}
};


// A frame with a size fixed to the default size of the cell
class fixed_default_size_frame : public aligned_frame_base
{
public:
	fixed_default_size_frame(const rcref<cell>& c, const alignment& a = alignment::center())
		: aligned_frame_base(c, a)
	{ }

	virtual range get_range() const { return range::make_fixed(get_default_size()); }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		size sz2 = get_default_size();
		if (!r.contains(sz2))
			result.set_empty();
		else
		{
			result.set(sz2);
			result.make_relative(sz);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		aligned_reshape(get_default_size(), b, oldOrigin);
	}
};


class inset_frame : public frame
{
private:
	margin m_margin;
	range m_calculatedRange;
	size m_calculatedDefaultSize;

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified margin is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	inset_frame(const rcref<cell>& c, const margin& m)
		: frame(c),
		m_margin(m)
	{ }

	inset_frame(const rcref<cell>& c)
		: frame(c)
	{ }

	margin& get_margin() { return m_margin; }
	const margin& get_margin() const { return m_margin; }

	void set_margin(const margin& m) { m_margin = m; }

	virtual size get_default_size() const { return m_calculatedDefaultSize; }

	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			size marginSize = m_margin.get_size();
			bool doesMarginWidthFit = sz.get_width() >= marginSize.get_width();
			bool doesMarginHeightFit = sz.get_height() >= marginSize.get_height();
			size adjustedBy;
			adjustedBy.get_width() = doesMarginWidthFit ? marginSize.get_width() : sz.get_width();
			adjustedBy.get_height() = doesMarginHeightFit ? marginSize.get_height() : sz.get_height();
			size newSize = sz - adjustedBy;
			result = frame::propose_size(newSize, resizeDimension, r - adjustedBy, horizontalMode, verticalMode);
			result.m_sizes[0].m_size += marginSize;
			result.m_sizes[1].m_size += marginSize;
			result.m_sizes[2].m_size += marginSize;
			result.m_sizes[3].m_size += marginSize;
			if (!doesMarginWidthFit)
				result.m_sizes[0].m_isValid = result.m_sizes[1].m_isValid = false;
			if (!doesMarginHeightFit)
				result.m_sizes[0].m_isValid = result.m_sizes[2].m_isValid = false;
		}
		return result;
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedRange = frame::get_range();
		if (m_calculatedRange.is_empty())
			m_calculatedDefaultSize.set(0, 0);
		else
		{
			m_calculatedRange += m_margin;
			m_calculatedDefaultSize = frame::get_default_size() + m_margin;
		}
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		bounds newBounds;
		size topLeftMargin = m_margin.get_top_left();
		newBounds.get_size() = b.get_size();
		size marginSize = m_margin.get_size();
		if (newBounds.get_width() >= marginSize.get_width())
			newBounds.get_width() -= marginSize.get_width();
		else
		{
			topLeftMargin.get_width() = 0; // Keep empty child within bounds?
			newBounds.get_width() = 0;
		}
		if (newBounds.get_height() >= marginSize.get_height())
			newBounds.get_height() -= marginSize.get_height();
		else
		{
			topLeftMargin.get_height() = 0; // Keep empty child within bounds?
			newBounds.get_height() = 0;
		}
		newBounds.get_position() = b.get_position() + topLeftMargin;
		point childOldPosition = get_child_position();
		point childOldOrigin = oldOrigin + (childOldPosition - newBounds.get_position());
		frame::reshape(newBounds, childOldOrigin);
	}
};


class fixed_size_frame : public aligned_frame_base
{
private:
	size m_size;
	size m_calculatedSize;

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified size is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	fixed_size_frame(const rcref<cell>& c, const size& sz, const alignment& a = alignment::center())
		: aligned_frame_base(c, a),
		m_size(sz)
	{ }

	fixed_size_frame(const rcref<cell>& c, const alignment& a = alignment::center())
		: aligned_frame_base(c, a),
		m_size(0, 0)
	{ }

	size& get_fixed_size() { return m_size; }
	const size& get_fixed_size() const { return m_size; }
	void set_fixed_size(const size& sz) { m_size = sz; }

	double& get_fixed_size(dimension d) { return m_size[d]; }
	const double get_fixed_size(dimension d) const { return m_size[d]; }
	void set_fixed_size(dimension d, double sz) { m_size[d] = sz; }

	double& get_fixed_height() { return m_size.get_height(); }
	const double get_fixed_height() const { return m_size.get_height(); }
	void set_fixed_height(double sz) { m_size.set_height(sz); }

	double& get_fixed_width() { return m_size.get_width(); }
	const double get_fixed_width() const { return m_size.get_width(); }
	void set_fixed_width(double sz) { m_size.set_width(sz); }

	virtual size get_default_size() const { return m_calculatedSize; }

	virtual range get_range() const { return range(m_calculatedSize, m_calculatedSize, true, true); }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (!r.contains(m_calculatedSize))
			result.set_empty();
		else
		{
			result.set(m_calculatedSize);
			result.make_relative(sz);
		}
		return result;
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedSize = frame::propose_size(m_size).find_first_valid_size(get_primary_flow_dimension());
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		aligned_reshape(m_calculatedSize, b, oldOrigin);
	}
};


// Does not modify the range, but applies a bounds on reshape().
// This is useful in the case of a parent pane internally handling the sizes and positions of descendant panes.
class override_bounds_frame : public aligned_frame_base
{
private:
	bounds m_bounds;

public:
	override_bounds_frame(const rcref<cell>& c, const bounds& b, const alignment& a = alignment::top_left())
		: aligned_frame_base(c, a),
		m_bounds(b)
	{ }

	override_bounds_frame(const rcref<cell>& c, const alignment& a = alignment::top_left())
		: aligned_frame_base(c, a),
		m_bounds(0, 0, 0, 0)
	{ }

	bounds& get_bounds() { return m_bounds; }
	const bounds& get_bounds() const { return m_bounds; }
	void set_bounds(const bounds& b) { m_bounds = b; }

	point& get_position() { return m_bounds.get_position(); }
	const point& get_position() const { return m_bounds.get_position(); }
	void set_position(const point& pt) { m_bounds.set_position(pt); }

	double& get_position(dimension d) { return m_bounds.get_position(d); }
	const double get_position(dimension d) const { return m_bounds.get_position(d); }
	void set_position(dimension d, double d2) { return m_bounds.set_position(d, d2); }

	size& get_fixed_size() { return m_bounds.get_size(); }
	const size& get_fixed_size() const { return m_bounds.get_size(); }
	void set_fixed_size(const size& sz) { m_bounds.set_size(sz); }

	double& get_fixed_size(dimension d) { return m_bounds.get_size(d); }
	const double get_fixed_size(dimension d) const { return m_bounds.get_size(d); }
	void set_fixed_size(dimension d, double d2) { m_bounds.set_size(d, d2); }

	double& get_fixed_height() { return m_bounds.get_height(); }
	const double get_fixed_height() const { return m_bounds.get_height(); }
	void set_fixed_height(double d) { m_bounds.set_height(d); }

	double& get_fixed_width() { return m_bounds.get_width(); }
	const double get_fixed_width() const { return m_bounds.get_width(); }
	void set_fixed_width(double d) { m_bounds.set_width(d); }

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		aligned_reshape(m_bounds, b, oldOrigin);
	}
};


class limit_range_frame : public frame
{
private:
	range m_range;
	range m_calculatedRange;
	size m_calculatedDefaultSize;

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified size is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	limit_range_frame(const rcref<cell>& c, const range& rng)
		: frame(c),
		m_range(rng)
	{ }

	limit_range_frame(const rcref<cell>& c)
		: frame(c)
	{ }

	const range& get_limit_range() const { return m_range; }
	range& get_limit_range() { return m_range; }
	void set_limit_range(const range& r) { m_range = r; }

	virtual size get_default_size() const { return m_calculatedDefaultSize; }

	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		return frame::propose_size(sz, resizeDimension, r & m_calculatedRange, horizontalMode, verticalMode);
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedRange = m_range & frame::get_range();
		if (m_calculatedRange.is_empty())
			m_calculatedDefaultSize.set(0, 0);
		else
			m_calculatedDefaultSize = propose_size(frame::get_default_size()).find_first_valid_size(get_primary_flow_dimension());
	}
};


class unstretchable_frame : public frame
{
public:
	unstretchable_frame(const rcref<cell>& c)
		: frame(c)
	{ }

	virtual range get_range() const
	{
		range stretchRange(size(0, 0), get_default_size(), true, true);
		return frame::get_range() & stretchRange;
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		range stretchRange(size(0, 0), get_default_size(), true, true);
		return frame::propose_size(sz, resizeDimension, r & stretchRange, horizontalMode, verticalMode);
	}
};


class unshrinkable_frame : public frame
{
public:
	unshrinkable_frame(const rcref<cell>& c)
		: frame(c)
	{ }

	virtual range get_range() const
	{
		range shrinkRange(get_default_size(), size(0, 0), false, false);
		return frame::get_range() & shrinkRange;
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		range shrinkRange(get_default_size(), size(0, 0), false, false);
		return frame::propose_size(sz, resizeDimension, r & shrinkRange, horizontalMode, verticalMode);
	}
};


// Maybe need 2 separate classes?
// propose_aspect_ratio_frame - Proposes new size with aspect ratio, but allows content cell to decide
// maintain_aspect_ratio_frame2? - Alway maintains the aspect ratio (based on default size), and aligns contents if necessary.


// Proposes an aspect ratio based on the default size
class propose_aspect_ratio_frame : public frame
{
private:
	range m_calculatedRange;

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		dimension d = frame::get_primary_flow_dimension();
		size defaultSize = frame::get_default_size();
		range r = frame::get_range();
		if (!r.is_empty())
		{
			size newSize;
			m_calculatedRange.has_max_width() = r.has_max_width();
			m_calculatedRange.has_max_height() = r.has_max_height();
			for (;;)
			{
				if (r.has_max_width())
				{
					newSize.get_width() = r.get_max_width();
					newSize.get_height() = (r.get_max_width() * defaultSize.get_height()) / defaultSize.get_width();
					if (r.has_max_height() && r.get_max_height() != newSize.get_height())
					{
						double otherWidth = (r.get_max_height() * defaultSize.get_width()) / defaultSize.get_height();
						if (newSize.get_width() > otherWidth)
						{
							newSize.get_height() = r.get_max_height();
							newSize.get_width() = otherWidth;
						}
					}
				}
				else
				{
					if (!r.has_max_height())
						break;
					newSize.get_height() = r.get_max_height();
					newSize.get_width() = (r.get_max_height() * defaultSize.get_width()) / defaultSize.get_height();
				}
				m_calculatedRange.set_max(frame::propose_size(newSize).find_first_valid_size(d, true));
				break;
			}

			for (;;)
			{
				if (r.get_min_width() > 0)
				{
					newSize.get_width() = r.get_min_width();
					newSize.get_height() = (r.get_min_width() * defaultSize.get_height()) / defaultSize.get_width();
					if (r.get_min_height() != 0 && r.get_min_height() != newSize.get_height())
					{
						double otherWidth = (r.get_min_height() * defaultSize.get_width()) / defaultSize.get_height();
						if (newSize.get_width() < otherWidth)
						{
							newSize.get_height() = r.get_min_height();
							newSize.get_width() = otherWidth;
						}
					}
				}
				else
				{
					if (r.get_min_height() == 0)
						break;
					newSize.get_height() = r.get_min_height();
					newSize.get_width() = (r.get_min_height() * defaultSize.get_width()) / defaultSize.get_height();
				}
				m_calculatedRange.set_min(frame::propose_size(newSize).find_first_valid_size(d, false));
				break;
			}
		}
	}

public:
	propose_aspect_ratio_frame(const rcref<cell>& c)
		: frame(c)
	{ }

	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			size defaultSize = frame::get_default_size();
			size newSize;
			if (resizeDimension.has_value())
			{
				dimension d = resizeDimension.value();
				newSize[!d] = (sz[d] * defaultSize[!d]) / defaultSize[d];
				newSize[d] = sz[d];
			}
			else
			{
				dimension d = get_primary_flow_dimension();
				newSize[!d] = (sz[d] * defaultSize[!d]) / defaultSize[d];
				if (newSize[!d] <= sz[!d])
					newSize[d] = sz[d];
				else
				{
					newSize[!d] = sz[!d];
					newSize[d] = (sz[!d] * defaultSize[d]) / defaultSize[!d];
				}
			}

			result = frame::propose_size(newSize, resizeDimension, r & m_calculatedRange, horizontalMode, verticalMode); // Need m_calculatedRange here?
			result.make_relative(sz);
		}
		return result;
	}
};


class proportional_frame : public aligned_frame_base
{
private:
	proportion m_proportion; // proportion of enclosing area to fill

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified proportion is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

	void validate_proportion()
	{
		if (m_proportion[dimension::horizontal] > 1.0)
			m_proportion[dimension::horizontal] = 1.0;
		if (m_proportion[dimension::horizontal] < 0.0)
			m_proportion[dimension::horizontal] = 0.0;
		if (m_proportion[dimension::vertical] > 1.0)
			m_proportion[dimension::vertical] = 1.0;
		if (m_proportion[dimension::vertical] < 0.0)
			m_proportion[dimension::vertical] = 0.0;
	}

public:
	proportional_frame(const rcref<cell>& c, const proportion& p, const alignment& a = alignment::center())
		: aligned_frame_base(c, a),
		m_proportion(p)
	{
		validate_proportion();
	}

	const proportion& get_proportion() const { return m_proportion; }

	void set_proportion(const proportion& p)
	{
		m_proportion = p;
		validate_proportion();
	}

	virtual size get_default_size() const { return frame::get_default_size() / m_proportion; }

	virtual range get_range() const { return frame::get_range() / m_proportion; }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			// Do we have rounding issues here?
			size sz2 = sz * m_proportion;
			propose_size_result result = frame::propose_size(sz2, resizeDimension, r, horizontalMode, verticalMode);
			result.m_sizes[0].m_size /= m_proportion;
			result.m_sizes[1].m_size /= m_proportion;
			result.m_sizes[2].m_size /= m_proportion;
			result.m_sizes[3].m_size /= m_proportion;
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = b.get_size() * m_proportion;
		aligned_reshape(sz, b, oldOrigin);
	}
};


class unconstrained_frame : public aligned_frame_base
{
public:
	unconstrained_frame(const rcref<cell>& c, const alignment& a = alignment::center())
		: aligned_frame_base(c, a)
	{ }

	virtual range get_range() const { return range::make_unbounded(); }

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			size sz2 = r.limit(sz);
			result.set(sz2);
			result.make_relative(sz);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		aligned_reshape(sz, b, oldOrigin);
	}
};


class unconstrained_min_frame : public aligned_frame_base
{
public:
	unconstrained_min_frame(const rcref<cell>& c, const alignment& a = alignment::center())
		: aligned_frame_base(c, a)
	{ }

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.get_min_height() = 0;
		r.get_min_width() = 0;
		return r;
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			result = frame::propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
			if (!result.m_sizes[0].m_isValid)
			{
				size sz2 = r.limit(sz);
				result.m_sizes[0].m_isValid = true;
				if (!result.m_sizes[1].m_isValid)
				{
					result.m_sizes[0].m_size.get_width() = sz2.get_width();
					result.m_sizes[2].m_size.get_width() = sz2.get_width();
					if (!result.m_sizes[2].m_isValid)
					{
						result.m_sizes[1].m_isValid = true;
						result.m_sizes[2].m_isValid = true;
						result.m_sizes[3].m_isValid = true;
						result.m_sizes[0].m_size.get_height() = sz2.get_height();
						result.m_sizes[2].m_size.get_height() = sz2.get_height();
						result.m_sizes[1].m_size = sz2;
						result.m_sizes[3].m_size = sz2;
					}
					else
					{
						result.m_sizes[0].m_size.get_height() = result.m_sizes[2].m_size.get_height();
						if (result.m_sizes[3].m_isValid)
						{
							result.m_sizes[1].m_isValid = true;
							result.m_sizes[1].m_size.get_height() = result.m_sizes[3].m_size.get_height();
							result.m_sizes[3].m_size.get_width() = sz2.get_width();
							result.m_sizes[1].m_size.get_width() = sz2.get_width();
						}
					}
				}
				else
				{
					result.m_sizes[0].m_size.get_height() = sz2.get_height();
					result.m_sizes[1].m_size.get_height() = sz2.get_height();
					if (!result.m_sizes[2].m_isValid)
					{
						result.m_sizes[0].m_size.get_width() = result.m_sizes[1].m_size.get_width();
						if (result.m_sizes[3].m_isValid)
						{
							result.m_sizes[2].m_isValid = true;
							result.m_sizes[2].m_size.get_width() = result.m_sizes[3].m_size.get_width();
							result.m_sizes[3].m_size.get_height() = sz2.get_height();
							result.m_sizes[2].m_size.get_height() = sz2.get_height();
						}
					}
					else
					{
						result.m_sizes[1].m_isValid = true;
						result.m_sizes[2].m_isValid = true;
						result.m_sizes[3].m_isValid = true;
						result.m_sizes[0].m_size.get_width() = sz2.get_width();
						result.m_sizes[1].m_size.get_width() = sz2.get_width();
						result.m_sizes[2].m_size = sz2;
						result.m_sizes[3].m_size = sz2;
					}
				}
			}
			result.make_relative(sz);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		aligned_reshape(sz, b, oldOrigin);
	}

};

class unconstrained_max_frame : public aligned_frame_base
{
public:
	unconstrained_max_frame(const rcref<cell>& c, const alignment& a = alignment::center())
		: aligned_frame_base(c, a)
	{ }

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.has_max_height() = false;
		r.has_max_width() = false;
		return r;
	}

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		if (r.is_empty())
			result.set_empty();
		else
		{
			result = frame::propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
			size sz2 = r.limit(sz);
			if (!result.m_sizes[3].m_isValid)
			{
				result.m_sizes[3].m_isValid = true;
				if (result.m_sizes[2].m_isValid)
				{
					result.m_sizes[3].m_size.get_height() = sz2.get_height();
					result.m_sizes[2].m_size.get_height() = sz2.get_height();
					if (result.m_sizes[1].m_isValid)
					{
						result.m_sizes[0].m_isValid = true;
						result.m_sizes[0].m_size = sz2;
						result.m_sizes[1].m_size = sz2;
						result.m_sizes[2].m_size.get_width() = sz2.get_width();
						result.m_sizes[3].m_size.get_width() = sz2.get_width();
					}
					else
					{
						result.m_sizes[3].m_size.get_width() = result.m_sizes[2].m_size.get_width();
						if (result.m_sizes[0].m_isValid)
						{
							result.m_sizes[1].m_isValid = true;
							result.m_sizes[1].m_size.get_width() = result.m_sizes[0].m_size.get_width();
							result.m_sizes[1].m_size.get_height() = sz2.get_height();
							result.m_sizes[0].m_size.get_height() = sz2.get_height();
						}
					}
				}
				else
				{
					result.m_sizes[1].m_size.get_width() = sz2.get_width();
					result.m_sizes[3].m_size.get_width() = sz2.get_width();
					if (result.m_sizes[1].m_isValid)
					{
						result.m_sizes[3].m_size.get_height() = result.m_sizes[1].m_size.get_height();
						if (result.m_sizes[0].m_isValid)
						{
							result.m_sizes[2].m_isValid = true;
							result.m_sizes[2].m_size.get_height() = result.m_sizes[0].m_size.get_height();
							result.m_sizes[2].m_size.get_width() = sz2.get_width();
							result.m_sizes[0].m_size.get_width() = sz2.get_width();
						}
					}
					else
					{
						result.m_sizes[0].m_isValid = true;
						result.m_sizes[1].m_isValid = true;
						result.m_sizes[2].m_isValid = true;
						result.m_sizes[1].m_size = sz2;
						result.m_sizes[2].m_size = sz2;
						result.m_sizes[0].m_size.get_height() = sz2.get_height();
						result.m_sizes[3].m_size.get_height() = sz2.get_height();
					}
				}
			}
			result.make_relative(sz);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		aligned_reshape(sz, b, oldOrigin);
	}
};


}
}


#endif
