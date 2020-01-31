//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_CELL
#define COGS_HEADER_GEOMETRY_CELL


#include <optional>

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
#pragma warning (disable: 4521) // multiple copy constructors specified


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

	virtual bool is_frame() const { return false; } // used to support nesting frames

	// Using a size_mode other than size_mode::both in calls to propose_size() allows
	// implementions to avoid unnecessary work to determine values that will not be used by the caller.
	// Because determining the size of a cell may require requesting all possible sizes to find a valid one,
	// use of a size_mode other than size_mode::both is generally isolated to parent cell sizing of multiple child cells.
	enum class size_mode
	{
		// Only the lesser value is needed.  A valid greater value may not be returned, even if possible.
		lesser = -1,

		// Both lesser and greater values should be returned, if possible.
		both = 0,

		// Only the greater value is needed.  A valid lesser value may not be returned, even if possible.
		greater = 1
	};
	
	// When proposing a size, 4 values are returned.
	//
	// If a length cannot be satisfied as proposed, lesser and greater values are returned.
	// If a lesser length is not possible, the lesser value will be omitted/invalid.
	// If a greater length is not possible, the greater value will be omitted/invalid.
	// If the value can be matched exactly, all lesser and greater values will be set/valid and equal to the proposed value.
	// If the call is incapable of being successfully sized to any size, no values will be set/valid.

	struct proposed_size
	{
		size m_size;

		// False if this index cannot be matched due to a minimum or maximum being imposed.
		// For example, if the requested size is below minimums for both height/width, only the greater/greater position will be valid,
		// and set to minimum lengths.
		bool m_isValid;

		void set(const size& sz)
		{
			m_size = sz;
			m_isValid = true;
		}
	};

	struct propose_size_result
	{
		// Sizes are in the following order:
		//     [<=, <=] - lesser or equal width and lesser or equal height
		//     [<=, >=] - lesser or equal width and greater or equal height
		//     [>=, <=] - greater or equal width and lesser or equal height
		//     [>=, >=] - greater or equal width and greater or equal height
		proposed_size m_sizes[4];

		static int get_index(bool greaterWidth, bool greaterHeight) { return greaterWidth ? (greaterHeight ? 3 : 2) : (greaterHeight ? 1 : 0); }

		size& get_size(bool greaterWidth, bool greaterHeight) { return m_sizes[get_index(greaterWidth, greaterHeight)].m_size; }
		const size& get_size(bool greaterWidth, bool greaterHeight) const { return m_sizes[get_index(greaterWidth, greaterHeight)].m_size; }

		void set(const size& sz)
		{
			m_sizes[0].set(sz);
			m_sizes[1].set(sz);
			m_sizes[2].set(sz);
			m_sizes[3].set(sz);
		}

		enum class first_valid_size_order : int
		{
			order_0_1_2_3 = 0x0123,
			order_0_1_3_2 = 0x0132,
			order_0_2_1_3 = 0x0213,
			order_0_2_3_1 = 0x0231,
			order_0_3_1_2 = 0x0312,
			order_0_3_2_1 = 0x0321,
			order_1_0_2_3 = 0x1023,
			order_1_0_3_2 = 0x1032,
			order_1_2_0_3 = 0x1203,
			order_1_2_3_0 = 0x1230,
			order_1_3_0_2 = 0x1302,
			order_1_3_2_0 = 0x1320,
			order_2_0_1_3 = 0x2013,
			order_2_0_3_1 = 0x2031,
			order_2_1_0_3 = 0x2103,
			order_2_1_3_0 = 0x2130,
			order_2_3_0_1 = 0x2301,
			order_2_3_1_0 = 0x2310,
			order_3_0_1_2 = 0x3012,
			order_3_0_2_1 = 0x3021,
			order_3_1_0_2 = 0x3102,
			order_3_1_2_0 = 0x3120,
			order_3_2_0_1 = 0x3201,
			order_3_2_1_0 = 0x3210
		};

		size find_first_valid_size(first_valid_size_order order = first_valid_size_order::order_0_1_2_3)
		{
			for (int i = 0; i < 16; i += 4)
			{
				int i2 = ((int)order >> i) & 0x0F;
				if (m_sizes[i2].m_isValid)
					return m_sizes[i2].m_size;
			}
			return size(0, 0);
		}

		size find_first_valid_size(dimension primaryDimension, bool preferGreater = false)
		{
			first_valid_size_order order = (primaryDimension == dimension::horizontal)
				? (preferGreater ? first_valid_size_order::order_3_2_1_0 : first_valid_size_order::order_0_1_2_3)
				: (preferGreater ? first_valid_size_order::order_3_1_2_0 : first_valid_size_order::order_0_2_1_3);
			return find_first_valid_size(order);
		}

		bool is_empty() const { return !m_sizes[0].m_isValid && !m_sizes[1].m_isValid && !m_sizes[2].m_isValid && !m_sizes[3].m_isValid; }
		void set_empty() { m_sizes[0].m_isValid = m_sizes[1].m_isValid = m_sizes[2].m_isValid = m_sizes[3].m_isValid = false; }

		void make_relative(const size& sz)
		{
			m_sizes[0].m_isValid &= m_sizes[0].m_size.get_width() <= sz.get_width() && m_sizes[0].m_size.get_height() <= sz.get_height();
			m_sizes[1].m_isValid &= m_sizes[1].m_size.get_width() <= sz.get_width() && m_sizes[1].m_size.get_height() >= sz.get_height();
			m_sizes[2].m_isValid &= m_sizes[2].m_size.get_width() >= sz.get_width() && m_sizes[2].m_size.get_height() <= sz.get_height();
			m_sizes[3].m_isValid &= m_sizes[3].m_size.get_width() >= sz.get_width() && m_sizes[3].m_size.get_height() >= sz.get_height();
		}

	};

	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode horizontalMode = size_mode::both, size_mode verticalMode = size_mode::both) const
	{
		propose_size_result result;
		range r2 = get_range() & r;
		if (r2.is_empty())
			result.set_empty();
		else
		{
			size sz2 = r2.limit(sz);
			result.set(sz2);
			result.make_relative(sz);
		}
		return result;
	}


protected:
	virtual void calculate_range() { }

	// newBounds is in parent coordinates.  Generally, its position is (0,0), unless invoked by a frame.
	//
	// oldOrigin is in new local coordinates.  This point maps to the origin location where the cell was
	//		last positioned.  This is useful to determine the area to be redrawn, if prior contents may be preserved.
	//
	//	A) As a child pane moves from (5,5) to (10,10), the oldOrigin will be (-5,-5).
	//
	//	B) An OS window has its own coodination system.  A move of an OS window is processed as a position of (0,0),
	//		with an oldOrigin containing the distance moved.
	//
	//	C) scroll_pane processes scrolling of its child pane as a move, while always representing (0,0) as the top left of the
	//		visible area.  The origin of its contents are equal to the negative of the scroll position.  i.e. Scrolling from position
	//		(0,0) to position (5,5) moves the contents origin from (0,0) to (-5,-5), with an oldOrigin of (5,5).  The origin of
	//		scroll_pane contents will never be >0.
	//
	//	D) If a child pane moved from (5,5) to (10,10) but indicates an oldOrigin of (0,0), the child pane
	//		was effectively not moved.  This can happen due to combination of movements of parent panes, leaving
	//		the child pane in the same location in a grandparent pane, as the child pane position refers only the the coordinate
	//		system of the immediate parent.  This is still processed as a reshape, in case parent coordinate system changes are relevant.
	//		This is essentially a notification that the coordinate system changed out from under it.  Contents may need to be moved,
	//		but it might not be necessary to redraw them.  

	virtual void reshape(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		// Preserve size from last reshape
		m_currentSize = newBounds.get_size();
	}

	// accessors
	static void calculate_range(cell& c) { c.calculate_range(); }
	static void reshape(cell& c, const bounds& newBounds, const point& oldOrigin = point(0, 0)) { c.reshape(newBounds, oldOrigin); }
};


}


}
}

#pragma warning(pop)


#endif
