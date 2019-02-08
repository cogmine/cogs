//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_CELL
#define COGS_CELL


#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/geometry/point.hpp"
#include "cogs/geometry/bounds.hpp"
#include "cogs/geometry/size.hpp"
#include "cogs/geometry/point.hpp"
#include "cogs/geometry/margin.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


namespace cogs {
namespace geometry {
namespace planar {


class cell
{
private:
	size m_currentSize;

public:
	cell()
		: m_currentSize(0, 0)
	{ }

	size get_size() const { return m_currentSize; };

	virtual size get_default_size() const = 0;

	virtual range get_range() const { range r; return r; }

	virtual dimension get_primary_flow_dimension() const { return dimension::horizontal; }

	virtual bool is_frame() const { return false; }	// used to support nesting frames


	// propose functions propose a size for 'enclosing area' of the cell.
	// If a frame does not impose constraints on its enclosure, it should return whatever proposed.
	// The base cell will constrain the 'enclosure area' by default.

	virtual double propose_length(dimension d, double proposed, linear::range& rtnOtherRange) const
	{
		double rtn = proposed;
		range planarRange = get_range();
		rtnOtherRange = planarRange[!d];
		const linear::range& linearRange = planarRange[d];
		if (rtn < linearRange.get_min())
			rtn = linearRange.get_min();
		else if (linearRange.has_max() && linearRange.get_max() < rtn)
			rtn = linearRange.get_max();
		return rtn;
	}

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size newSize;
		linear::range otherRange;
		newSize[d] = cell::propose_length(d, proposedSize[d], otherRange);
		newSize[!d] = otherRange.limit(proposedSize[!d]);
		return newSize;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		dimension d = get_primary_flow_dimension();
		return cell::propose_lengths(d, proposedSize);
	}

protected:
	virtual void calculate_range() { }

	// newBounds is in parent coordinates.  Generally, its position is (0,0), unless invoked by a frame.
	//
	// oldOrigin is in new local coordinates.  This point maps to the origin location where the cell was
	//		last positioned.  This is useful to determine the area to be redrawn, if prior contents may be preserved.
	//
	// A) As a child pane moves from (5,5) to (10,10), the oldOrigin will be (-5,-5).
	//
	// B) An OS window has its own coodination system.  A move of an OS window is processed as a position of (0,0),
	//		with an oldOrigin containing the distance moved.
	//
	// C) scroll_pane processes scrolling of its child pane as a move, while always representing (0,0) as the top left of the
	//		visible area.  The origin of its contents are equal to the negative of the scroll position.  i.e. Scrolling from position
	//		(0,0) to position (5,5) moves the contents origin from (0,0) to (-5,-5), with an oldOrigin of (5,5).  The origin of
	//		scroll_pane contents will never be >0.
	//
	// D) If a child pane moved from (5,5) to (10,10) but indicates an oldOrigin of (0,0), the child pane
	//		was effectively not moved.  This can happen due to combination of movements of parent panes, leaving
	//		the child pane in the same location in a grandparent pane, as the child pane position refers only the the coordinate
	//		system of the immediate parent.  This is still processed as a reshape, in case parent coordinate system changes are relevant.
	//		This is essentially a notification that the coordinate system changed out from under it.  Contents may need to be moved,
	//		but it may not be necessary to redraw them.  

	virtual void reshape(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		// Preserve size from last reshape
		m_currentSize = newBounds.get_size();
	}

	// accessors
	static void calculate_range(cell& c) { c.calculate_range(); }
	static void reshape(cell& c, const bounds& newBounds, const point& oldOrigin = point(0, 0)) { c.reshape(newBounds, oldOrigin);  }
};


}


}
}

#pragma warning(pop)


#endif

