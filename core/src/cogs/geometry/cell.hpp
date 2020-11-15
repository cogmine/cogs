//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
#include "cogs/gfx/canvas.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/mem/flag_enum.hpp"


namespace cogs {
namespace geometry {
namespace planar {


enum class cell_sizing_type : unsigned int
{
	lesser_width_lesser_height    = (1 << 0), // [0][0]
	lesser_width_greater_height   = (1 << 1), // [0][1]
	greater_width_lesser_height   = (1 << 2), // [1][0]
	greater_width_greater_height  = (1 << 3), // [1][1]
};


}
}


template <> struct is_flag_enum<geometry::planar::cell_sizing_type> : public std::true_type { };


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

	virtual std::optional<size> get_default_size() const { return std::nullopt; }

	virtual range get_range() const { return range::make_unbounded(); }

	virtual dimension get_primary_flow_dimension() const { return dimension::horizontal; }

	// A sizing_mask is mask of sizing_type's indicating each potential variation of a sizing result requested by the caller.
	// Using a sizing_mask other than all_sizing_types in calls to propose_size() allows implementions to avoid
	// unnecessary work to determine values that will not be used by the caller.  Do not exclude a result if it's
	// still needed if all other variations are not possible.  Use of a sizing_mask other than all_sizing_types
	// is generally isolated to parent cells that need to probe sizes of mutliple interdependent child cells.
	// (i.e. pane::propose_size())
	typedef cell_sizing_type sizing_type;
	typedef flag_mask_t<sizing_type> sizing_mask;

	// all_sizing_types indicates that all candidate sizes should be returned.  The only reason to pass a value other than
	// all_sizing_types would be if certain candidates would not be used, allowing the sizing algorithm to avoid
	// calculating them, as an optimization.
	static constexpr sizing_mask all_sizing_types =
		sizing_type::lesser_width_lesser_height
		| sizing_type::lesser_width_greater_height
		| sizing_type::greater_width_lesser_height
		| sizing_type::greater_width_greater_height;

	static constexpr sizing_mask all_lesser_width_sizing_types =
		sizing_type::lesser_width_lesser_height
		| sizing_type::lesser_width_greater_height; // [0][?]

	static constexpr sizing_mask all_greater_width_sizing_types =
		sizing_type::greater_width_lesser_height
		| sizing_type::greater_width_greater_height; // [1][?]

	static constexpr sizing_mask all_lesser_height_sizing_types =
		sizing_type::lesser_width_lesser_height
		| sizing_type::greater_width_lesser_height; // [?][0]

	static constexpr sizing_mask all_greater_height_sizing_types =
		sizing_type::lesser_width_greater_height
		| sizing_type::greater_width_greater_height; // [?][1]

	// Up to 4 different sizes might be returned from propose_size()
	//
	// Sizes are returned in the following order:
	//
	// [<= width][<= height] - lesser or equal width and lesser or equal height
	// [<= width][>= height] - lesser or equal width and greater or equal height
	// [>= width][<= height] - greater or equal width and lesser or equal height
	// [>= width][>= height] - greater or equal width and greater or equal height
	//
	// When a size can be matched exactly, all entries will contain that size.
	// When a length cannot be satisfied as proposed, lesser and/or greater values are returned at their associated indexes.
	// When a lesser or equal length is not possible, the lesser length index will be empty (std::nullopt).
	// When a greater or equal length is not possible, the greater length index will be empty (std::nullopt).
	// If the call is incapable of being successfully sized to any size, all indexes will be empty (std::nullopt).

	union propose_size_result
	{
		std::optional<size> sizes[2][2];
		std::optional<size> indexed_sizes[4];

		propose_size_result()
			: sizes{}
		{ }

		propose_size_result(const propose_size_result& src)
			: sizes{ { src.sizes[0][0], src.sizes[0][1] }, { src.sizes[1][0], src.sizes[1][1] } }
		{ }

		propose_size_result& operator=(const propose_size_result& src)
		{
			sizes[0][0] = src.sizes[0][0];
			sizes[0][1] = src.sizes[0][1];
			sizes[1][0] = src.sizes[1][0];
			sizes[1][1] = src.sizes[1][1];
			return *this;
		}

		bool operator==(const propose_size_result& src)
		{
			return sizes[0][0] == src.sizes[0][0]
				&& sizes[0][1] == src.sizes[0][1]
				&& sizes[1][0] == src.sizes[1][0]
				&& sizes[1][1] == src.sizes[1][1];
		}

		bool operator==(const size& sz)
		{
			return sizes[0][0] == sz
				&& sizes[0][1] == sz
				&& sizes[1][0] == sz
				&& sizes[1][1] == sz;
		}

		void assign_abs()
		{
			if (sizes[0][0].has_value())
				sizes[0][0]->abs();
			if (sizes[0][1].has_value())
				sizes[0][1]->abs();
			if (sizes[1][0].has_value())
				sizes[1][0]->abs();
			if (sizes[1][1].has_value())
				sizes[1][1]->abs();
		}

		propose_size_result abs() const
		{
			propose_size_result result(*this);
			result.assign_abs();
			return result;
		}

		const propose_size_result& pre_assign_abs()
		{
			assign_abs();
			return *this;
		}

		propose_size_result post_assign_abs()
		{
			propose_size_result result(*this);
			assign_abs();
			return result;
		}

		void assign_ceil()
		{
			if (sizes[0][0].has_value())
				sizes[0][0]->ceil();
			if (sizes[0][1].has_value())
				sizes[0][1]->ceil();
			if (sizes[1][0].has_value())
				sizes[1][0]->ceil();
			if (sizes[1][1].has_value())
				sizes[1][1]->ceil();
		}

		propose_size_result ceil() const
		{
			propose_size_result result(*this);
			result.assign_ceil();
			return result;
		}

		const propose_size_result& pre_assign_ceil()
		{
			assign_ceil();
			return *this;
		}

		propose_size_result post_assign_ceil()
		{
			propose_size_result result(*this);
			assign_ceil();
			return result;
		}

		void assign_floor()
		{
			if (sizes[0][0].has_value())
				sizes[0][0]->floor();
			if (sizes[0][1].has_value())
				sizes[0][1]->floor();
			if (sizes[1][0].has_value())
				sizes[1][0]->floor();
			if (sizes[1][1].has_value())
				sizes[1][1]->floor();
		}

		propose_size_result floor() const
		{
			propose_size_result result(*this);
			result.assign_floor();
			return result;
		}

		const propose_size_result& pre_assign_floor()
		{
			assign_floor();
			return *this;
		}

		propose_size_result post_assign_floor()
		{
			propose_size_result result(*this);
			assign_floor();
			return result;
		}

		void assign_round()
		{
			if (sizes[0][0].has_value())
				sizes[0][0]->round();
			if (sizes[0][1].has_value())
				sizes[0][1]->round();
			if (sizes[1][0].has_value())
				sizes[1][0]->round();
			if (sizes[1][1].has_value())
				sizes[1][1]->round();
		}

		propose_size_result round() const
		{
			propose_size_result result(*this);
			result.assign_round();
			return result;
		}

		const propose_size_result& pre_assign_round()
		{
			assign_round();
			return *this;
		}

		propose_size_result post_assign_round()
		{
			propose_size_result result(*this);
			assign_floor();
			return result;
		}

		propose_size_result& operator+=(const size& sz)
		{
			if (sizes[0][0].has_value())
				*sizes[0][0] += sz;
			if (sizes[0][1].has_value())
				*sizes[0][1] += sz;
			if (sizes[1][0].has_value())
				*sizes[1][0] += sz;
			if (sizes[1][1].has_value())
				*sizes[1][1] += sz;
			return *this;
		}

		propose_size_result operator+(const size& sz) const
		{
			propose_size_result result(*this);
			result += sz;
			return result;
		}

		propose_size_result& operator-=(const size& sz)
		{
			if (sizes[0][0].has_value())
				*sizes[0][0] -= sz;
			if (sizes[0][1].has_value())
				*sizes[0][1] -= sz;
			if (sizes[1][0].has_value())
				*sizes[1][0] -= sz;
			if (sizes[1][1].has_value())
				*sizes[1][1] -= sz;
			return *this;
		}

		propose_size_result operator-(const size& sz) const
		{
			propose_size_result result(*this);
			result -= sz;
			return result;
		}

		propose_size_result& operator|=(const size& sz)
		{
			if (sizes[0][0].has_value())
				*sizes[0][0] |= sz;
			if (sizes[0][1].has_value())
				*sizes[0][1] |= sz;
			if (sizes[1][0].has_value())
				*sizes[1][0] |= sz;
			if (sizes[1][1].has_value())
				*sizes[1][1] |= sz;
			return *this;
		}

		propose_size_result operator|(const size& sz) const
		{
			propose_size_result result(*this);
			result |= sz;
			return result;
		}

		propose_size_result& operator&=(const size& sz)
		{
			if (sizes[0][0].has_value())
				*sizes[0][0] &= sz;
			if (sizes[0][1].has_value())
				*sizes[0][1] &= sz;
			if (sizes[1][0].has_value())
				*sizes[1][0] &= sz;
			if (sizes[1][1].has_value())
				*sizes[1][1] &= sz;
			return *this;
		}

		propose_size_result operator&(const size& sz) const
		{
			propose_size_result result(*this);
			result &= sz;
			return result;
		}

		std::optional<size>(&operator[](size_t i))[2]{ return sizes[i]; }
		const std::optional<size>(&operator[](size_t i) const)[2]{ return sizes[i]; }

		std::optional<size>& get_size(bool greaterWidth, bool greaterHeight) { return sizes[greaterWidth][greaterHeight]; }
		const std::optional<size>& get_size(bool greaterWidth, bool greaterHeight) const { return sizes[greaterWidth][greaterHeight]; }

		std::optional<size>& get_size(dimension d, bool greater, bool greaterOther)
		{
			if (d != dimension::horizontal)
				std::swap(greater, greaterOther);
			return get_size(greater, greaterOther);
		}

		const std::optional<size>& get_size(dimension d, bool greater, bool greaterOther) const
		{
			if (d != dimension::horizontal)
				std::swap(greater, greaterOther);
			return get_size(greater, greaterOther);
		}

		bool has_size(bool greaterWidth, bool greaterHeight) const { return get_size(greaterWidth, greaterHeight).has_value(); }
		bool has_size(dimension d, bool greater, bool greaterOther) const { return get_size(d, greater, greaterOther).has_value(); }

		size& get_size_value(bool greaterWidth, bool greaterHeight) { return *get_size(greaterWidth, greaterHeight); }
		const size& get_size_value(bool greaterWidth, bool greaterHeight) const { return *get_size(greaterWidth, greaterHeight); }

		size& get_size_value(dimension d, bool greater, bool greaterOther) { return *get_size(d, greater, greaterOther); }
		const size& get_size_value(dimension d, bool greater, bool greaterOther) const { return *get_size(d, greater, greaterOther); }

		void set(const size& sz)
		{
			sizes[0][0] = sz;
			sizes[0][1] = sz;
			sizes[1][0] = sz;
			sizes[1][1] = sz;
		}

		bool is_empty() const { return !sizes[0][0].has_value() && !sizes[0][1].has_value() && !sizes[1][0].has_value() && !sizes[1][1].has_value(); }
		void set_empty()
		{
			sizes[0][0].reset();
			sizes[0][1].reset();
			sizes[1][0].reset();
			sizes[1][1].reset();
		}

		bool contains(const size& sz)
		{
			return sizes[0][0] == sz || sizes[0][1] == sz || sizes[1][0] == sz || sizes[1][1] == sz;
		}

		bool has_same(dimension d)
		{
			if (sizes[0][0].has_value())
			{
				if (sizes[0][1].has_value() && (*sizes[0][1])[d] != (*sizes[0][0])[d])
					return false;
				if (sizes[1][0].has_value() && (*sizes[1][0])[d] != (*sizes[0][0])[d])
					return false;
				if (sizes[1][1].has_value() && (*sizes[1][1])[d] != (*sizes[0][0])[d])
					return false;
			}
			else if (sizes[0][1].has_value())
			{
				if (sizes[1][0].has_value() && (*sizes[1][0])[d] != (*sizes[0][1])[d])
					return false;
				if (sizes[1][1].has_value() && (*sizes[1][1])[d] != (*sizes[0][1])[d])
					return false;
			}
			else if (sizes[1][0].has_value())
			{
				if (sizes[1][1].has_value() && (*sizes[1][1])[d] != (*sizes[1][0])[d])
					return false;
			}
			return true;
		}

		bool has_same_width() { return has_same(dimension::horizontal); }
		bool has_same_height() { return has_same(dimension::vertical); }

		bool has_only(dimension d, double v)
		{
			bool has_any = false;
			if (sizes[0][0].has_value())
			{
				if ((*sizes[0][0])[d] != v)
					return false;
				has_any = true;
			}
			if (sizes[0][1].has_value())
			{
				if ((*sizes[0][1])[d] != v)
					return false;
				has_any = true;
			}
			if (sizes[1][0].has_value())
			{
				if ((*sizes[1][0])[d] != v)
					return false;
				has_any = true;
			}
			if (sizes[1][1].has_value())
			{
				if ((*sizes[1][1])[d] != v)
					return false;
				has_any = true;
			}
			return has_any;
		}

		bool has_only_width(double d) { return has_only(dimension::horizontal, d); }
		bool has_only_height(double d) { return has_only(dimension::vertical, d); }

		// When resizing from a side (not the corner), instead of using the lesser/lesser size,
		// the nearest lesser length in the resize dimension is used.
		std::optional<size> get_nearest(dimension d, bool greater = false, bool greaterOther = false) const
		{
			auto sz = get_size(d, greater, greaterOther);
			if (sz.has_value())
			{
				auto sz2 = get_size(d, greater, !greaterOther);
				if (!sz2.has_value())
					return sz;
				if ((*sz)[d] == (*sz2)[d])
				{
					if (greaterOther && (*sz)[!d] > (*sz2)[!d])
						return sz;
					return sz2;
				}
				if (greater && (*sz)[d] > (*sz2)[d])
					return sz;
				return sz2;
			}
			auto sz2 = get_size(d, greater, !greaterOther);
			if (sz2.has_value())
				return sz2;
			sz = get_size(d, !greater, greaterOther);
			if (sz.has_value())
			{
				sz2 = get_size(d, !greater, !greaterOther);
				if (!sz2.has_value())
					return sz;
				if ((*sz)[d] == (*sz2)[d])
				{
					if (greaterOther && (*sz)[!d] > (*sz2)[!d])
						return sz;
					return sz2;
				}
				if (!greater && (*sz)[d] < (*sz2)[d])
					return sz;
				return sz2;
			}
			sz2 = get_size(d, !greater, !greaterOther);
			if (sz2.has_value())
				return sz2;
			return std::nullopt;
		}

		std::optional<size> find_first_valid_size(dimension d, bool preferGreaterWidth = false, bool preferGreaterHeight = false) const
		{
			bool horizontalPrimary = d == dimension::horizontal;
			const std::optional<size>& sz = sizes[preferGreaterWidth][preferGreaterHeight];
			if (sz.has_value())
				return *sz;
			bool index1 = horizontalPrimary == preferGreaterWidth;
			bool index2 = horizontalPrimary != preferGreaterHeight;
			const std::optional<size>& sz1 = sizes[index1][index2];
			if (sz1.has_value())
				return *sz1;
			const std::optional<size>& sz2 = sizes[!index1][!index2];
			if (sz2.has_value())
				return *sz2;
			const std::optional<size>& sz3 = sizes[!preferGreaterWidth][!preferGreaterHeight];
			if (sz3.has_value())
				return *sz3;
			return std::nullopt;
		}

		bool add_size(const size& relativeTo, const size& cmp, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			auto cmpFunc = [](bool testGreater, double d1, double d2)
			{
				if (testGreater)
					return d1 > d2;
				return d1 < d2;
			};

			auto check = [&](bool greaterWidth, bool greaterHeight)
			{
				bool greater[2] = { greaterWidth, greaterHeight };
				auto& sz2o = sizes[greaterWidth][greaterHeight];
				if (!sz2o.has_value())
					sz2o = cmp;
				else
				{
					size& sz2 = *sz2o;
					// If resizeDimension was specified, choose the nearest length in that dimension.
					if (resizeDimension.has_value())
					{
						if (cmpFunc(!greater[(int)primaryDimension], cmp[primaryDimension], sz2[primaryDimension])
							|| (cmp[primaryDimension] == sz2[primaryDimension]
								&& (cmpFunc(!greater[(int)!primaryDimension], cmp[!primaryDimension], sz2[!primaryDimension]))))
							sz2 = cmp;
					}
					else // Otherwise, prefer the size with least length sums. If same, use one with nearer primary dimension.
					{
						double cmpSum = cmp.get_width() + cmp.get_height();
						double s2zSum = sz2.get_width() + sz2.get_height();
						if (cmpSum != s2zSum)
						{
							if (cmpSum < s2zSum)
								sz2 = cmp;
						}
						else if (cmpFunc(!greater[(int)primaryDimension], cmp[primaryDimension], sz2[primaryDimension]))
							sz2 = cmp;
					}
				}
			};

			if (cmp.get_width() == relativeTo.get_width())
			{
				if (cmp.get_height() == relativeTo.get_height())
				{
					sizes[0][0] = cmp;
					sizes[1][0] = cmp;
					sizes[0][1] = cmp;
					sizes[1][1] = cmp;
					return true;
				}
				else if (cmp.get_height() > relativeTo.get_height())
				{
					check(false, true);
					check(true, true);
				}
				else //if (cmp.get_height() < relativeTo.get_height())
				{
					check(false, false);
					check(true, false);
				}
			}
			else if (cmp.get_width() > relativeTo.get_width())
			{
				if (cmp.get_height() == relativeTo.get_height())
				{
					check(true, false);
					check(true, true);
				}
				else if (cmp.get_height() > relativeTo.get_height())
					check(true, true);
				else //if (cmp.get_height() < relativeTo.get_height())
					check(true, false);
			}
			else //if (cmp.get_width() < relativeTo.get_width())
			{
				if (cmp.get_height() == relativeTo.get_height())
				{
					check(false, false);
					check(false, true);
				}
				else if (cmp.get_height() > relativeTo.get_height())
					check(false, true);
				else //if (cmp.get_height() < relativeTo.get_height())
					check(false, false);
			}
			return false;
		}

		propose_size_result relative_to(const size& sz, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			propose_size_result result;
			if (resizeDimension.has_value())
				primaryDimension = *resizeDimension;

			auto add = [&](const std::optional<size>& cmpOpt)
			{
				if (cmpOpt.has_value())
					return result.add_size(sz, *cmpOpt, primaryDimension, resizeDimension);
				return false;
			};

			if (!add(sizes[0][0]) && !add(sizes[0][1]) && !add(sizes[1][0]))
				add(sizes[1][1]);
			return result;
		}

		void set_relative_to(const size& sz, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			*this = relative_to(sz, primaryDimension, resizeDimension);
		}

		// this must already be relative to relativeTo
		void merge_relative_to(const propose_size_result& src, const size& relativeTo, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			if (resizeDimension.has_value())
				primaryDimension = *resizeDimension;

			auto add = [&](const std::optional<size>& cmpOpt)
			{
				if (cmpOpt.has_value())
					return add_size(relativeTo, *cmpOpt, primaryDimension, resizeDimension);
				return false;
			};

			if (!add(src.sizes[0][0]) && !add(src.sizes[0][1]) && !add(src.sizes[1][0]))
				add(src.sizes[1][1]);
		}
	};

	virtual propose_size_result propose_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		sizing_mask sizingMask = all_sizing_types) const
	{
		(void)sizingMask; // unused
		propose_size_result result;
		range r2 = get_range() & r;
		if (!r2.is_empty())
		{
			size sz2 = r2.limit(sz);
			result.set(sz2);
			result.set_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

	std::optional<size> propose_size_best(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		bool nearestGreater = false,
		bool preferGreaterWidth = false,
		bool preferGreaterHeight = false,
		sizing_mask sizingMask = all_sizing_types) const
	{
		propose_size_result result = propose_size(sz, r, resizeDimension, sizingMask);
		COGS_ASSERT(result.sizes[0][0].has_value() || result.sizes[0][1].has_value() || result.sizes[1][0].has_value() || result.sizes[1][1].has_value());
		std::optional<size> sz2;
		if (resizeDimension.has_value())
			sz2 = result.get_nearest(*resizeDimension, nearestGreater);
		else
			sz2 = result.find_first_valid_size(get_primary_flow_dimension(), preferGreaterWidth, preferGreaterHeight);
		COGS_ASSERT(sz2.has_value());
		return sz2;
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
		(void)oldOrigin;
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


#endif
