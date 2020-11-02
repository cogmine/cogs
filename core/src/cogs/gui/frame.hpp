//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_GUI_FRAME
#define COGS_HEADER_GUI_FRAME


#include "cogs/collections/container_dlist.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/mem/rc_container.hpp"


namespace cogs {
namespace gui {


typedef gfx::size size;
typedef gfx::point point;
typedef gfx::bounds bounds;
typedef gfx::range range;
typedef gfx::margin margin;
typedef gfx::proportion proportion;
typedef gfx::direction direction;
typedef gfx::dimension dimension;
typedef gfx::flow flow;
typedef gfx::script_flow script_flow;
typedef gfx::cell cell;
typedef gfx::alignment alignment;
//typedef gfx::proportional_sizing_group proportional_sizing_group;
//typedef gfx::fair_sizing_group fair_sizing_group;
//typedef gfx::equal_sizing_group equal_sizing_group;
template <gfx::sizing_disposition disposition>
using sizing_group = gfx::sizing_group<disposition>;
typedef gfx::sizing_disposition sizing_disposition;
typedef gfx::sizing_cell sizing_cell;
typedef gfx::canvas sizing_group_base;

typedef gfx::font font;
typedef gfx::bitmap bitmap;
typedef gfx::bitmask bitmask;


// frame and cell facilitate sizing/resizing behavior of 2D rectangular elements.

class frameable;
class frame;

typedef container_dlist<rcref<frame> > frame_list;

// The base frame passes everything through to the equivalent methods in contained cell.
class frame : public cell, public object
{
private:
	frame_list::remove_token m_siblingIterator;
	weak_rcptr<cell> m_frameable;

	// Preserved position from last reshape, in parent coordinates (in which 0,0 is parent's origin).
	point m_childPosition = point(0, 0);

	friend class frameable;

	frame(frame&) = delete;
	frame(frame&&) = delete;

public:
	frame() { }

	virtual point get_child_position() const { return m_childPosition; }

	virtual std::optional<size> get_default_size() const
	{
		frame_list::iterator itor = m_siblingIterator;
		if (!!itor)
		{
			++itor;
			rcptr<cell> c;
			if (!!itor)
				c = *itor;
			if (!c)
				c = m_frameable;
			if (!!c)
				return c->get_default_size();
		}
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
		return std::nullopt;
	}

	virtual range get_range() const
	{
		frame_list::iterator itor = m_siblingIterator;
		if (!!itor)
		{
			++itor;
			rcptr<cell> c;
			if (!!itor)
				c = *itor;
			if (!c)
				c = m_frameable;
			if (!!c)
				return c->get_range();
		}
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
		return cell::get_range();
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		frame_list::iterator itor = m_siblingIterator;
		if (!!itor)
		{
			++itor;
			rcptr<cell> c;
			if (!!itor)
				c = *itor;
			if (!c)
				c = m_frameable;
			if (!!c)
				return c->propose_size(sz, r, resizeDimension, sizingMask);
		}
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
		return cell::propose_size(sz, r, resizeDimension, sizingMask);
	}

protected:
	virtual void calculate_range()
	{
		frame_list::iterator itor = m_siblingIterator;
		if (!!itor)
		{
			++itor;
			rcptr<cell> c;
			if (!!itor)
				c = *itor;
			if (!c)
				c = m_frameable;
			if (!!c)
			{
				cell::calculate_range(*c);
				return;
			}
		}
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		m_childPosition = b.get_position();
		cell::reshape(b, oldOrigin);

		frame_list::iterator itor = m_siblingIterator;
		if (!!itor)
		{
			++itor;
			rcptr<cell> c;
			if (!!itor)
				c = *itor;
			if (!c)
				c = m_frameable;
			if (!!c)
			{
				cell::reshape(*c, b, oldOrigin);
				return;
			}
		}
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
	}

	virtual dimension get_primary_flow_dimension() const
	{
		rcptr<cell> c = m_frameable;
		if (!!c)
			return c->get_primary_flow_dimension();
		COGS_ASSERT(false && "cogs::frame used when not associated with a frameable?");
		return cell::get_primary_flow_dimension();
	}
};


class frameable : public cell, public object
{
private:
	frame_list m_frames;

public:
	frameable(frameable&) = delete;
	frameable(frameable&&) = delete;

	explicit frameable(frame_list&& frames)
		: m_frames(std::move(frames))
	{
		frame_list::iterator itor = m_frames.get_first();
		while (!!itor)
		{
			COGS_ASSERT(!(*itor)->m_siblingIterator);
			(*itor)->m_siblingIterator = itor;
			(*itor)->m_frameable = this_rcref;
			++itor;
		}
	}

	virtual void insert_before_frame(const rcref<frame>& f, const rcref<frame>& beforeThis)
	{
		COGS_ASSERT(!f->m_siblingIterator);
		COGS_ASSERT(!!beforeThis->m_siblingIterator);
		f->m_siblingIterator = m_frames.insert_before(beforeThis->m_siblingIterator, f);
		f->m_frameable = this_rcref;
	}

	virtual void insert_after_frame(const rcref<frame>& f, const rcref<frame>& afterThis)
	{
		COGS_ASSERT(!f->m_siblingIterator);
		COGS_ASSERT(!!afterThis->m_siblingIterator);
		f->m_siblingIterator = m_frames.insert_after(afterThis->m_siblingIterator, f);
		f->m_frameable = this_rcref;
	}

	virtual void append_frame(const rcref<frame>& f)
	{
		COGS_ASSERT(!f->m_siblingIterator);
		f->m_siblingIterator = m_frames.append(f);
		f->m_frameable = this_rcref;
	}

	virtual void prepend_frame(const rcref<frame>& f)
	{
		COGS_ASSERT(!f->m_siblingIterator);
		f->m_siblingIterator = m_frames.prepend(f);
		f->m_frameable = this_rcref;
	}

	virtual void remove_frame(frame& f)
	{
		COGS_ASSERT(!!f.m_siblingIterator);
		COGS_ASSERT(f.m_frameable == this);
		m_frames.remove(f.m_siblingIterator);
	}

	bool has_frames() const { return !!m_frames; }

	point get_position() const
	{
		frame_list::iterator itor = m_frames.get_last();
		if (!!itor)
			return (*itor)->get_child_position();
		point pt(0, 0);
		return pt;
	}

	virtual std::optional<size> get_frame_default_size() const
	{
		frame_list::iterator itor = m_frames.get_first();
		if (!!itor)
			return (*itor)->get_default_size();
		return get_default_size();
	}

	virtual range get_frame_range() const
	{
		frame_list::iterator itor = m_frames.get_first();
		if (!!itor)
			return (*itor)->get_range();
		return get_range();
	}

	virtual propose_size_result propose_frame_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		frame_list::iterator itor = m_frames.get_first();
		if (!!itor)
			return (*itor)->propose_size(sz, r, resizeDimension, sizingMask);
		return propose_size(sz, r, resizeDimension, sizingMask);
	}

	using cell::propose_size_best;

protected:
	frameable() { }

	virtual void calculate_frame_range()
	{
		frame_list::iterator itor = m_frames.get_first();
		if (!!itor)
			return cell::calculate_range(**itor);
		return calculate_range();
	}

	virtual void reshape_frame(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		frame_list::iterator itor = m_frames.get_first();
		if (!!itor)
			return cell::reshape(**itor, newBounds, oldOrigin);
		return reshape(newBounds, oldOrigin);
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
	explicit override_default_size_frame(const size& defaultSize = size(0, 0))
		: m_defaultSize(defaultSize)
	{ }

	size& get_default_size_override() { return m_defaultSize; }
	const size& get_default_size_override() const { return m_defaultSize; }
	void set_default_size_override(const size& sz) { m_defaultSize = sz; }

	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		std::optional<size> opt = propose_size_best(m_defaultSize);
		m_calculatedDefaultSize = opt.has_value() ? *opt : size(0, 0);
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
	explicit aligned_frame_base(const alignment& a = alignment::center())
		: m_alignment(a)
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
	explicit fixed_default_size_frame(const alignment& a = alignment::center())
		: aligned_frame_base(a)
	{ }

	virtual range get_range() const
	{
		std::optional<size> defaultSize = get_default_size();
		if (defaultSize.has_value())
			return range::make_fixed(*defaultSize);
		COGS_ASSERT(false);
		return range::make_empty();
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask = all_sizing_types) const
	{
		propose_size_result result;
		std::optional<size> defaultSize = get_default_size();
		if (!defaultSize.has_value())
			COGS_ASSERT(false);
		else
		{
			if (r.contains(*defaultSize))
			{
				result.set(*defaultSize);
				result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
			}
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		std::optional<size> defaultSizeOpt = get_default_size();
		size defaultSize = defaultSizeOpt.has_value() ? *defaultSizeOpt : size(0, 0);
		aligned_reshape(defaultSize, b, oldOrigin);
	}
};


class inset_frame : public frame
{
private:
	margin m_margin;
	range m_calculatedRange;
	std::optional<size> m_calculatedDefaultSize;

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified margin is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	inset_frame() { }

	explicit inset_frame(const margin& m)
		: m_margin(m)
	{ }

	margin& get_margin() { return m_margin; }
	const margin& get_margin() const { return m_margin; }

	void set_margin(const margin& m) { m_margin = m; }

	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }

	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		range r2 = r & m_calculatedRange;
		if (!r2.is_empty())
		{
			size sz2 = r2.limit(sz);
			size marginSize = m_margin.get_size();
			bool doesMarginWidthFit = sz2.get_width() >= marginSize.get_width();
			bool doesMarginHeightFit = sz2.get_height() >= marginSize.get_height();
			size adjustedBy;
			adjustedBy.get_width() = doesMarginWidthFit ? marginSize.get_width() : sz2.get_width();
			adjustedBy.get_height() = doesMarginHeightFit ? marginSize.get_height() : sz2.get_height();
			size newSize = sz2 - adjustedBy;
			result = frame::propose_size(newSize, r - adjustedBy, resizeDimension, sizingMask);
			if (!doesMarginWidthFit)
			{
				result.sizes[0][0].reset();
				result.sizes[0][1].reset();
				if (!doesMarginHeightFit)
					result.sizes[1][0].reset();
				else if (result.sizes[1][0].has_value())	// Same as [0][1][0].has_value()
					*result.sizes[1][0] += marginSize;
			}
			else
			{
				if (!doesMarginHeightFit)
				{
					result.sizes[0][0].reset();
					result.sizes[1][0].reset();
				}
				else
				{
					if (result.sizes[0][0].has_value())	// Same as [1][0][0].has_value()
						*result.sizes[0][0] += marginSize;
					if (result.sizes[1][0].has_value())	// Same as [1][1][0].has_value()
						*result.sizes[1][0] += marginSize;
				}
				if (result.sizes[0][1].has_value())	// Same as [1][0][1].has_value()
					*result.sizes[0][1] += marginSize;
			}
			if (result.sizes[1][1].has_value())	// Same as [1][1][1].has_value()
				*result.sizes[1][1] += marginSize;
			if (sz != newSize)
				result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedRange = frame::get_range();
		m_calculatedDefaultSize.reset();
		if (!m_calculatedRange.is_empty())
		{
			m_calculatedRange += m_margin;
			std::optional<size> defaultSize = frame::get_default_size();
			if (defaultSize.has_value())
				m_calculatedDefaultSize = *defaultSize + m_margin;
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
	explicit fixed_size_frame(const size& sz, const alignment& a = alignment::center())
		: aligned_frame_base(a),
		m_size(sz)
	{ }

	explicit fixed_size_frame(const alignment& a = alignment::center())
		: aligned_frame_base(a),
		m_size(0, 0)
	{ }

	size& get_fixed_size() { return m_size; }
	const size& get_fixed_size() const { return m_size; }
	void set_fixed_size(const size& sz) { m_size = sz; }

	double& get_fixed_size(dimension d) { return m_size[d]; }
	double get_fixed_size(dimension d) const { return m_size[d]; }
	void set_fixed_size(dimension d, double sz) { m_size[d] = sz; }

	double& get_fixed_height() { return m_size.get_height(); }
	double get_fixed_height() const { return m_size.get_height(); }
	void set_fixed_height(double sz) { m_size.set_height(sz); }

	double& get_fixed_width() { return m_size.get_width(); }
	double get_fixed_width() const { return m_size.get_width(); }
	void set_fixed_width(double sz) { m_size.set_width(sz); }

	virtual std::optional<size> get_default_size() const { return m_calculatedSize; }

	virtual range get_range() const { return range(m_calculatedSize, m_calculatedSize, true, true); }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask = all_sizing_types) const
	{
		propose_size_result result;
		if (r.contains(m_calculatedSize))
		{
			result.set(m_calculatedSize);
			result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		std::optional<size> sz = frame::propose_size(m_size).find_first_valid_size(get_primary_flow_dimension());
		if (!sz.has_value())
			m_calculatedSize.clear();
		m_calculatedSize = *sz;
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
	explicit override_bounds_frame(const bounds& b, const alignment& a = alignment::top_left())
		: aligned_frame_base(a),
		m_bounds(b)
	{ }

	explicit override_bounds_frame(const alignment& a = alignment::top_left())
		: aligned_frame_base(a),
		m_bounds(0, 0, 0, 0)
	{ }

	bounds& get_bounds() { return m_bounds; }
	const bounds& get_bounds() const { return m_bounds; }
	void set_bounds(const bounds& b) { m_bounds = b; }

	point& get_position() { return m_bounds.get_position(); }
	const point& get_position() const { return m_bounds.get_position(); }
	void set_position(const point& pt) { m_bounds.set_position(pt); }

	double& get_position(dimension d) { return m_bounds.get_position(d); }
	double get_position(dimension d) const { return m_bounds.get_position(d); }
	void set_position(dimension d, double d2) { return m_bounds.set_position(d, d2); }

	size& get_fixed_size() { return m_bounds.get_size(); }
	const size& get_fixed_size() const { return m_bounds.get_size(); }
	void set_fixed_size(const size& sz) { m_bounds.set_size(sz); }

	double& get_fixed_size(dimension d) { return m_bounds.get_size(d); }
	double get_fixed_size(dimension d) const { return m_bounds.get_size(d); }
	void set_fixed_size(dimension d, double d2) { m_bounds.set_size(d, d2); }

	double& get_fixed_height() { return m_bounds.get_height(); }
	double get_fixed_height() const { return m_bounds.get_height(); }
	void set_fixed_height(double d) { m_bounds.set_height(d); }

	double& get_fixed_width() { return m_bounds.get_width(); }
	double get_fixed_width() const { return m_bounds.get_width(); }
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
	std::optional<size> m_calculatedDefaultSize;

	// Changes to default size and range require recalculation, as ranges and default sizes of enclosing cells/frames may
	// be dependant on them.  If the specified size is modified, the caller is exptected to trigger
	// recalculation (calculate_range(), or pane::recompose() to queue it against the UI thread).

public:
	limit_range_frame() { }

	explicit limit_range_frame(const range& rng)
		: m_range(rng)
	{ }

	const range& get_limit_range() const { return m_range; }
	range& get_limit_range() { return m_range; }
	void set_limit_range(const range& r) { m_range = r; }

	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }

	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		return frame::propose_size(sz, r & m_calculatedRange, resizeDimension, sizingMask);
	}

protected:
	virtual void calculate_range()
	{
		frame::calculate_range();
		m_calculatedRange = m_range & frame::get_range();
		m_calculatedDefaultSize.reset();
		if (!m_calculatedRange.is_empty())
		{
			std::optional<size> defaultSize = frame::get_default_size();
			if (defaultSize.has_value())
			{
				std::optional<size> opt = propose_size_best(*defaultSize);
				m_calculatedDefaultSize = opt.has_value() ? *opt : size(0, 0);

			}
		}
	}
};


class unstretchable_frame : public frame
{
public:
	virtual range get_range() const
	{
		std::optional<size> defaultSize = get_default_size();
		if (defaultSize.has_value())
		{
			range stretchRange(size(0, 0), *defaultSize, true, true);
			return frame::get_range() & stretchRange;
		}
		else
		{
			COGS_ASSERT(false);
			return range::make_empty();
		}
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		std::optional<size> defaultSize = get_default_size();
		if (defaultSize.has_value())
		{
			range stretchRange(size(0, 0), *defaultSize, true, true);
			result = frame::propose_size(sz, r & stretchRange, resizeDimension, sizingMask);
		}
		else
			COGS_ASSERT(false);
		return result;
	}
};


class unshrinkable_frame : public frame
{
public:
	virtual range get_range() const
	{
		std::optional<size> defaultSize = get_default_size();
		if (defaultSize.has_value())
		{
			range shrinkRange(*defaultSize, size(0, 0), false, false);
			return frame::get_range() & shrinkRange;
		}
		else
		{
			COGS_ASSERT(false);
			return range::make_empty();
		}
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		std::optional<size> defaultSize = get_default_size();
		if (defaultSize.has_value())
		{
			range shrinkRange(*defaultSize, size(0, 0), false, false);
			return frame::propose_size(sz, r & shrinkRange, resizeDimension, sizingMask);
		}
		else
			COGS_ASSERT(false);
		return result;
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
		std::optional<size> defaultSize = frame::get_default_size();
		if (!defaultSize.has_value())
		{
			COGS_ASSERT(false);
			m_calculatedRange = range::make_empty();
			return;
		}

		dimension d = frame::get_primary_flow_dimension();
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
					newSize.get_height() = (r.get_max_width() * defaultSize->get_height()) / defaultSize->get_width();
					if (r.has_max_height() && r.get_max_height() != newSize.get_height())
					{
						double otherWidth = (r.get_max_height() * defaultSize->get_width()) / defaultSize->get_height();
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
					newSize.get_width() = (r.get_max_height() * defaultSize->get_width()) / defaultSize->get_height();
				}
				auto result = frame::propose_size(newSize).find_first_valid_size(d, true, true);
				if (result.has_value())
				{
					m_calculatedRange.set_max(*result);
					break;
				}
				m_calculatedRange.set_empty();
				return;
			}
			for (;;)
			{
				if (r.get_min_width() > 0)
				{
					newSize.get_width() = r.get_min_width();
					newSize.get_height() = (r.get_min_width() * defaultSize->get_height()) / defaultSize->get_width();
					if (r.get_min_height() != 0 && r.get_min_height() != newSize.get_height())
					{
						double otherWidth = (r.get_min_height() * defaultSize->get_width()) / defaultSize->get_height();
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
					newSize.get_width() = (r.get_min_height() * defaultSize->get_width()) / defaultSize->get_height();
				}
				auto result = frame::propose_size(newSize).find_first_valid_size(d, false, false);
				COGS_ASSERT(result.has_value());
				if (result.has_value())
				{
					m_calculatedRange.set_min(*result);
					break;
				}
				m_calculatedRange.set_empty();
				return;
			}
		}
	}

public:
	virtual range get_range() const { return m_calculatedRange; }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		std::optional<size> defaultSize = frame::get_default_size();
		if (defaultSize.has_value())
		{
			range r2 = r & m_calculatedRange;
			if (!r2.is_empty())
			{
				size sz2 = r2.limit(sz);
				auto getMatchSize = [&](dimension d, size& newSize)
				{
					newSize[!d] = (sz2[d] * (*defaultSize)[!d]) / (*defaultSize)[d];
					if (r2.contains(!d, newSize[!d]))
						newSize[d] = sz2[d];
					else if (newSize[!d] < r2.get_min(!d))
					{
						newSize[d] = (r2.get_min(!d) * (*defaultSize)[d]) / (*defaultSize)[!d];
						if (!r2.contains(d, newSize[d]))
							return false;
						newSize[!d] = r2.get_min(!d);
					}
					else //if (r2.has_max(!d) && newSize[!d] > r2.get_max(!d))
					{
						newSize[d] = (r2.get_max(!d) * (*defaultSize)[d]) / (*defaultSize)[!d];
						if (!r2.contains(d, newSize[d]))
							return false;
						newSize[!d] = r2.get_max(!d);
					}
					return true;
				};
				for (;;)
				{
					size matchSize1;
					if (!getMatchSize(dimension::vertical, matchSize1))
						break;
					result = frame::propose_size(matchSize1, r2, resizeDimension, sizingMask);
					dimension d = get_primary_flow_dimension();
					if (matchSize1 != sz)
					{
						result.set_relative_to(sz, d, resizeDimension);
						size matchSize2;
						if (!getMatchSize(dimension::horizontal, matchSize2))
							break;
						if (matchSize2 != matchSize1)
						{
							propose_size_result result2 = frame::propose_size(matchSize2, r2, resizeDimension, sizingMask);
							result.merge_relative_to(result2, sz, d, resizeDimension);
						}
					}
					break;
				}
			}
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
	explicit proportional_frame(const proportion& p, const alignment& a = alignment::center())
		: aligned_frame_base(a),
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

	virtual std::optional<size> get_default_size() const
	{
		std::optional<size> defaultSize = frame::get_default_size();
		if (defaultSize.has_value())
			*defaultSize /= m_proportion;
		return defaultSize;
	}

	virtual range get_range() const { return frame::get_range() / m_proportion; }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		if (!r.is_empty())
		{
			// Do we have rounding issues here?
			size sz2 = sz * m_proportion;
			result = frame::propose_size(sz2, r, resizeDimension, sizingMask);
			if (result.sizes[0][0].has_value())
				*result.sizes[0][0] /= m_proportion;
			if (result.sizes[0][1].has_value())
				*result.sizes[0][1] /= m_proportion;
			if (result.sizes[1][0].has_value())
				*result.sizes[1][0] /= m_proportion;
			if (result.sizes[1][1].has_value())
				*result.sizes[1][1] /= m_proportion;
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
	explicit unconstrained_frame(const alignment& a = alignment::center())
		: aligned_frame_base(a)
	{ }

	virtual range get_range() const { return range::make_unbounded(); }

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask = all_sizing_types) const
	{
		propose_size_result result;
		if (!r.is_empty())
		{
			size sz2 = r.limit(sz);
			result.set(sz2);
			result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz2;
		std::optional<size> sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		if (sz.has_value())
			sz2 = *sz;
		else
			sz2.clear();
		aligned_reshape(sz2, b, oldOrigin);
	}
};


class unconstrained_min_frame : public aligned_frame_base
{
public:
	explicit unconstrained_min_frame(const alignment& a = alignment::center())
		: aligned_frame_base(a)
	{ }

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.get_min_height() = 0;
		r.get_min_width() = 0;
		return r;
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		if (!r.is_empty())
		{
			size sz2 = r.limit(sz);
			result = frame::propose_size(sz2, range::make_unbounded(), resizeDimension, sizingMask);
			if (!result.sizes[0][0].has_value())
			{
				if (!result.sizes[0][1].has_value())
				{
					// There is only a greater/greater result (or no result).  Set all to the requested size.
					if (!result.sizes[1][0].has_value())
					{
						result.sizes[0][0] = sz2;
						result.sizes[0][1] = sz2;
						result.sizes[1][0] = sz2;
						result.sizes[1][1] = sz2;
					}
					else
					{
						result.sizes[1][0]->get_width() = sz2.get_width();
						if (result.sizes[1][1].has_value())
						{
							// There were no lesser width results, but there were both lesser and greater height results.
							// Use the height results, and use our width.
							result.sizes[1][1]->get_width() = sz2.get_width();
							result.sizes[0][1] = *result.sizes[1][1];
						}
						// else // If only greater/lesser is provided, use lesser height and our width
						result.sizes[0][0] = *result.sizes[1][0];
					}
				}
				else
				{
					if (!result.sizes[1][0].has_value())
					{
						result.sizes[0][1]->get_height() = sz2.get_height();
						if (result.sizes[1][1].has_value())
						{
							// We have greater heights, and both lesser and greater widths.
							// Use those widths.  Use our height.
							result.sizes[1][1]->get_height() = sz2.get_height();
							result.sizes[1][0] = *result.sizes[1][1];
						}
						// else // The only provided size was a lesser width and greater height size.
						// Use the lesser width and use our height instead.  Don't provider geater width.
						result.sizes[0][0] = *result.sizes[0][1];
					}
					else
					{
						// Both of these scenarios involve imposing minimums in one dimension to allow
						// a lesser value in the other.  Rather than pick one of those, consider either
						// of them to be  imposing a minimum we want to override.  Use our own sizes.
						result.sizes[0][0] = sz2;
						result.sizes[0][1] = sz2;
						result.sizes[1][0] = sz2;
						result.sizes[1][1] = sz2;
					}
				}
			}
			result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz2;
		std::optional<size> sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		if (sz.has_value())
			sz2 = *sz;
		else
			sz2.clear();
		aligned_reshape(sz2, b, oldOrigin);
	}
};


class unconstrained_max_frame : public aligned_frame_base
{
public:
	explicit unconstrained_max_frame(const alignment& a = alignment::center())
		: aligned_frame_base(a)
	{ }

	virtual range get_range() const
	{
		range r = frame::get_range();
		r.has_max_height() = false;
		r.has_max_width() = false;
		return r;
	}

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result;
		if (!r.is_empty())
		{
			size sz2 = r.limit(sz);
			result = frame::propose_size(sz2, range::make_unbounded(), resizeDimension, sizingMask);
			if (!result.sizes[1][1].has_value())
			{
				if (!result.sizes[1][0].has_value())
				{
					// If only a lesser/lesser result, or not result, use ours.
						// If there is only lesser width, greater height
						// Consider this to be imposing a maximum.  Use ours.
					if (!result.sizes[0][1].has_value())
					{
						result.sizes[0][0] = sz2;
						result.sizes[0][1] = sz2;
						result.sizes[1][0] = sz2;
						result.sizes[1][1] = sz2;
					}
					else
					{
						result.sizes[0][1]->get_width() = sz2.get_width();
						if (result.sizes[0][0].has_value())
						{
							// If there were no greater widths, but there were lesser and greater height results.
							// Use the height results, and use our width;
							result.sizes[0][0]->get_width() = sz2.get_width();
							result.sizes[1][0] = *result.sizes[0][0];
						}
						// else // If only lesser/greater, use greater height and our width instead.
						result.sizes[1][1] = *result.sizes[0][1];
					}
				}
				else
				{
					if (!result.sizes[0][1].has_value())
					{
						result.sizes[1][0]->get_height() = sz2.get_height();
						if (result.sizes[0][0].has_value())
						{
							// We have lesser heights, and both lesser and greater widths.
							// Use those widths.  Use our height.
							result.sizes[0][0]->get_height() = sz2.get_height();
							result.sizes[0][1] = *result.sizes[0][0];
						}
						// else // The only provided size was a greater width and lesser height size.
						// Use the greater width and use our height instead.  Don't provider lesser width.
						result.sizes[1][1] = *result.sizes[1][0];
					}
					else
					{
						// Both of these scenarios involve imposing maximums in one dimension to allow
						// a greater value in the other.  Rather than pick one of those, consider either
						// of them to be  imposing a maximum we want to override.  Use our own sizes.
						result.sizes[0][0] = sz2;
						result.sizes[0][1] = sz2;
						result.sizes[1][0] = sz2;
						result.sizes[1][1] = sz2;
					}
				}
			}
			result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

protected:
	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size sz2;
		std::optional<size> sz = frame::propose_size(b.get_size()).find_first_valid_size(get_primary_flow_dimension());
		if (sz.has_value())
			sz2 = *sz;
		else
			sz2.clear();
		aligned_reshape(sz2, b, oldOrigin);
	}
};


}
}


#endif
