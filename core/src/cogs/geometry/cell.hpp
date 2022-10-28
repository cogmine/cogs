//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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

class cell_base
{
public:
	enum class quadrant_flag : unsigned int
	{
		lesser_x_lesser_y    = (1 << 0),
		lesser_x_greater_y   = (1 << 1),
		greater_x_lesser_y   = (1 << 2),
		greater_x_greater_y  = (1 << 3)
	};

	enum class dependent_size_info_flag : unsigned int
	{
		width_stretch_may_stretch_height = (1 << 0), // 0001
		height_shrink_may_shrink_width   = (1 << 0), // 0001

		width_stretch_may_shrink_height  = (1 << 1), // 0010
		height_stretch_may_shrink_width  = (1 << 1), // 0010

		width_shrink_may_stretch_height  = (1 << 2), // 0100
		height_shrink_may_stretch_width  = (1 << 2), // 0100

		width_shrink_may_shrink_height   = (1 << 3), // 1000
		height_stretch_may_stretch_width = (1 << 3), // 1000
	};
};

}
}


template <> struct is_flag_enum<geometry::planar::cell_base::quadrant_flag> : public std::true_type { };
template <> struct is_flag_enum<geometry::planar::cell_base::dependent_size_info_flag> : public std::true_type { };


namespace geometry {
namespace planar {


class cell : public cell_base
{
private:
	size m_currentSize;

public:
	using cell_base::quadrant_flag;
	using cell_base::dependent_size_info_flag;

	typedef flag_mask_t<quadrant_flag> quadrant_mask;

	static constexpr quadrant_mask all_quadrants =
		quadrant_flag::lesser_x_lesser_y
		| quadrant_flag::lesser_x_greater_y
		| quadrant_flag::greater_x_lesser_y
		| quadrant_flag::greater_x_greater_y;

	typedef flag_mask_t<dependent_size_info_flag> dependent_size_info_mask;

	static constexpr dependent_size_info_mask either_stretch_can_cause_other_change =
		dependent_size_info_flag::width_stretch_may_stretch_height
		| dependent_size_info_flag::width_stretch_may_shrink_height
		| dependent_size_info_flag::width_shrink_may_shrink_height;

	static constexpr dependent_size_info_mask either_change_can_cause_other_shrink = either_stretch_can_cause_other_change;

	static constexpr dependent_size_info_mask either_shrink_can_cause_other_change =
		dependent_size_info_flag::height_shrink_may_shrink_width
		| dependent_size_info_flag::width_shrink_may_stretch_height
		| dependent_size_info_flag::width_shrink_may_shrink_height;

	static constexpr dependent_size_info_mask either_change_can_cause_other_stretch = either_shrink_can_cause_other_change;

	static constexpr dependent_size_info_mask either_stretch_can_cause_other_stretch =
		dependent_size_info_flag::height_shrink_may_shrink_width
		| dependent_size_info_flag::width_shrink_may_shrink_height;

	static constexpr dependent_size_info_mask either_shrink_can_cause_other_shrink = either_stretch_can_cause_other_stretch;

	static constexpr dependent_size_info_mask either_shrink_can_cause_other_stretch = *dependent_size_info_flag::width_shrink_may_stretch_height;
	static constexpr dependent_size_info_mask either_stretch_can_cause_other_shrink = *dependent_size_info_flag::width_stretch_may_shrink_height;

	static constexpr dependent_size_info_mask either_change_can_cause_other_change =
		dependent_size_info_flag::width_stretch_may_stretch_height
		| dependent_size_info_flag::width_shrink_may_stretch_height
		| dependent_size_info_flag::width_stretch_may_shrink_height
		| dependent_size_info_flag::width_shrink_may_shrink_height;

	cell()
		: m_currentSize(0, 0)
	{ }

	size get_size() const { return m_currentSize; };

	virtual std::optional<size> get_default_size() const { return std::nullopt; }

	virtual range get_range() const { return range::make_unbounded(); }

	virtual dimension get_primary_flow_dimension() const { return dimension::horizontal; }

	union collaborative_sizes
	{
		std::optional<size> indexed_sizes[4];
		std::optional<size> sizes[2][2];

		collaborative_sizes()
			: indexed_sizes{}
		{ }

		explicit collaborative_sizes(const size& sz)
			: indexed_sizes{{sz}, {sz}, {sz}, {sz}}
		{ }

		explicit collaborative_sizes(const std::optional<size>& sz)
			: indexed_sizes{{sz}, {sz}, {sz}, {sz}}
		{ }

		explicit collaborative_sizes(const size& sz, const size& relative_to)
		{
			if (sz.get_width() > relative_to.get_width())
			{
				if (sz.get_height() <= relative_to.get_height())
				{
					sizes[1][0] = sz;
					if (sz.get_height() != relative_to.get_height())
						return;
				}
				sizes[1][1] = sz;
				return;
			}
			if (sz.get_width() != relative_to.get_width())
			{
				if (sz.get_height() <= relative_to.get_height())
				{
					sizes[0][0] = sz;
					if (sz.get_height() != relative_to.get_height())
						return;
				}
			}
			else
			{
				if (sz.get_height() <= relative_to.get_height())
				{
					sizes[0][0] = sizes[1][0] = sz;
					if (sz.get_height() != relative_to.get_height())
						return;
				}
				sizes[1][1] = sz;
			}
			sizes[0][1] = sz;
		}

		explicit collaborative_sizes(const std::optional<size>& sz, const size& relative_to)
		{
			if (!sz.has_value())
				return;
			if (sz->get_width() > relative_to.get_width())
			{
				if (sz->get_height() <= relative_to.get_height())
				{
					sizes[1][0] = sz;
					if (sz->get_height() != relative_to.get_height())
						return;
				}
				sizes[1][1] = sz;
				return;
			}
			if (sz->get_width() != relative_to.get_width())
			{
				if (sz->get_height() <= relative_to.get_height())
				{
					sizes[0][0] = sz;
					if (sz->get_height() != relative_to.get_height())
						return;
				}
			}
			else
			{
				if (sz->get_height() <= relative_to.get_height())
				{
					sizes[0][0] = sizes[1][0] = sz;
					if (sz->get_height() != relative_to.get_height())
						return;
				}
				sizes[1][1] = sz;
			}
			sizes[0][1] = sz;
		}

		collaborative_sizes(const collaborative_sizes& src)
			: sizes{ { src.sizes[0][0], src.sizes[0][1] }, { src.sizes[1][0], src.sizes[1][1] } }
		{ }

		collaborative_sizes& operator=(const collaborative_sizes& src)
		{
			sizes[0][0] = src.sizes[0][0];
			sizes[0][1] = src.sizes[0][1];
			sizes[1][0] = src.sizes[1][0];
			sizes[1][1] = src.sizes[1][1];
			return *this;
		}

		void set(const size& sz)
		{
			sizes[0][0] = sz;
			sizes[0][1] = sz;
			sizes[1][0] = sz;
			sizes[1][1] = sz;
		}

		void set(const std::optional<size>& sz)
		{
			sizes[0][0] = sz;
			sizes[0][1] = sz;
			sizes[1][0] = sz;
			sizes[1][1] = sz;
		}

		void set_relative_to(const size& sz, const size& relative_to)
		{
			if (sz.get_width() > relative_to.get_width())
			{
				sizes[0][0].reset();
				sizes[0][1].reset();
				if (sz.get_height() > relative_to.get_height())
					sizes[1][0].reset();
				else
				{
					sizes[1][0] = sz;
					if (sz.get_height() != relative_to.get_height())
					{
						sizes[1][1].reset();
						return;
					}
				}
				sizes[1][1] = sz;
				return;
			}
			if (sz.get_width() != relative_to.get_width())
			{
				sizes[1][0].reset();
				sizes[1][1].reset();
				if (sz.get_height() > relative_to.get_height())
					sizes[0][0].reset();
				else
				{
					sizes[0][0] = sz;
					if (sz.get_height() != relative_to.get_height())
					{
						sizes[0][1].reset();
						return;
					}
				}
			}
			else
			{
				if (sz.get_height() > relative_to.get_height())
				{
					sizes[0][0].reset();
					sizes[1][0].reset();
				}
				else
				{
					sizes[0][0] = sz;
					sizes[1][0] = sz;
					if (sz.get_height() != relative_to.get_height())
					{
						sizes[0][1].reset();
						sizes[1][1].reset();
						return;
					}
				}
				sizes[1][1] = sz;
			}
			sizes[0][1] = sz;
		}

		void set_relative_to(const std::optional<size>& sz, const size& relative_to)
		{
			if (sz.has_value())
				set_relative_to(*sz, relative_to);
			else
				set_empty();
		}

		bool is_empty() const { return !sizes[0][0].has_value() && !sizes[0][1].has_value() && !sizes[1][0].has_value() && !sizes[1][1].has_value(); }
		void set_empty()
		{
			sizes[0][0].reset();
			sizes[0][1].reset();
			sizes[1][0].reset();
			sizes[1][1].reset();
		}

		bool operator==(const collaborative_sizes& src)
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

		collaborative_sizes abs() const
		{
			collaborative_sizes result(*this);
			result.assign_abs();
			return result;
		}

		const collaborative_sizes& pre_assign_abs()
		{
			assign_abs();
			return *this;
		}

		collaborative_sizes post_assign_abs()
		{
			collaborative_sizes result(*this);
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

		collaborative_sizes ceil() const
		{
			collaborative_sizes result(*this);
			result.assign_ceil();
			return result;
		}

		const collaborative_sizes& pre_assign_ceil()
		{
			assign_ceil();
			return *this;
		}

		collaborative_sizes post_assign_ceil()
		{
			collaborative_sizes result(*this);
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

		collaborative_sizes floor() const
		{
			collaborative_sizes result(*this);
			result.assign_floor();
			return result;
		}

		const collaborative_sizes& pre_assign_floor()
		{
			assign_floor();
			return *this;
		}

		collaborative_sizes post_assign_floor()
		{
			collaborative_sizes result(*this);
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

		collaborative_sizes round() const
		{
			collaborative_sizes result(*this);
			result.assign_round();
			return result;
		}

		const collaborative_sizes& pre_assign_round()
		{
			assign_round();
			return *this;
		}

		collaborative_sizes post_assign_round()
		{
			collaborative_sizes result(*this);
			assign_floor();
			return result;
		}

		collaborative_sizes& operator+=(const size& sz)
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

		collaborative_sizes operator+(const size& sz) const
		{
			collaborative_sizes result(*this);
			result += sz;
			return result;
		}

		collaborative_sizes& operator-=(const size& sz)
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

		collaborative_sizes operator-(const size& sz) const
		{
			collaborative_sizes result(*this);
			result -= sz;
			return result;
		}

		collaborative_sizes& operator/=(const proportion& p)
		{
			if (sizes[0][0].has_value())
				*sizes[0][0] /= p;
			if (sizes[0][1].has_value())
				*sizes[0][1] /= p;
			if (sizes[1][0].has_value())
				*sizes[1][0] /= p;
			if (sizes[1][1].has_value())
				*sizes[1][1] /= p;
			return *this;
		}

		collaborative_sizes operator/(const proportion& p) const
		{
			collaborative_sizes result(*this);
			result /= p;
			return result;
		}

		collaborative_sizes& operator|=(const size& sz)
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

		collaborative_sizes operator|(const size& sz) const
		{
			collaborative_sizes result(*this);
			result |= sz;
			return result;
		}

		collaborative_sizes& operator&=(const size& sz)
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

		collaborative_sizes operator&(const size& sz) const
		{
			collaborative_sizes result(*this);
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

		// When resizing from a side (not the corner), instead of using the lesser/lesser size,
		// the nearest lesser length in the resize dimension is used.
		std::optional<size> get_nearest(const size& sz, dimension d, bool greater = false, bool greaterOther = false) const
		{
			// If there is only 1 that has an equal value, use it.
			// If there are 2 with an equal value, use preferred other.
			// Impossible for 3 to be equal.  All 4 equal is handled by previous case.
			const auto& sz1 = get_size(d, greater, greaterOther);
			if (sz1.has_value() && (*sz1)[d] == sz[d])
				return sz1;
			const auto& sz2 = get_size(d, greater, !greaterOther);
			if (sz2.has_value() && (*sz2)[d] == sz[d])
				return sz2;
			const auto& sz3 = get_size(d, !greater, greaterOther);
			if (sz3.has_value() && (*sz3)[d] == sz[d])
				return sz3;
			const auto& sz4 = get_size(d, !greater, !greaterOther);
			if (sz4.has_value() && (*sz4)[d] == sz[d])
				return sz4;
			if (sz1.has_value())
			{
				if (!sz2.has_value())
					return sz1;
				if ((*sz1)[d] == (*sz2)[d])
				{
					if (greaterOther && (*sz1)[!d] > (*sz2)[!d])
						return sz1;
					return sz2;
				}
				if (greater && (*sz1)[d] > (*sz2)[d])
					return sz1;
				return sz2;
			}
			if (sz2.has_value())
				return sz2;
			if (sz3.has_value())
			{
				if (!sz4.has_value())
					return sz3;
				if ((*sz3)[d] == (*sz4)[d])
				{
					if (greaterOther && (*sz3)[!d] > (*sz4)[!d])
						return sz3;
					return sz4;
				}
				if (!greater && (*sz3)[d] < (*sz4)[d])
					return sz3;
				return sz4;
			}
			if (sz4.has_value())
				return sz4;
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

		collaborative_sizes relative_to(const size& sz, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			collaborative_sizes result;
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

		void update_relative_to(const size& sz, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
		{
			*this = relative_to(sz, primaryDimension, resizeDimension);
		}

		// this must already be relative to relativeTo
		void merge_relative_to(const collaborative_sizes& src, const size& relativeTo, dimension primaryDimension, const std::optional<dimension>& resizeDimension = std::nullopt)
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

	virtual dependent_size_info_mask get_dependent_size_info() const
	{
		return dependent_size_info_mask::none;
	}

	bool has_dependent_size() const
	{
		return get_dependent_size_info() != dependent_size_info_mask::none;
	}

	bool has_dependent_size(dimension d, bool increasing) const
	{
		// Look Ma, no branches.
		static constexpr dependent_size_info_mask maskTable[4] = {
			dependent_size_info_flag::width_shrink_may_stretch_height | dependent_size_info_flag::width_shrink_may_shrink_height,   // 1100
			dependent_size_info_flag::width_stretch_may_stretch_height | dependent_size_info_flag::width_stretch_may_shrink_height, // 0011
			dependent_size_info_flag::height_shrink_may_stretch_width | dependent_size_info_flag::height_shrink_may_shrink_width,   // 0101
			dependent_size_info_flag::height_stretch_may_stretch_width | dependent_size_info_flag::height_stretch_may_shrink_width  // 1010
		};
		auto index = (static_cast<std::underlying_type_t<dimension>>(d) << 1) | static_cast<unsigned int>(increasing);
		return (get_dependent_size_info() & maskTable[index]) != dependent_size_info_mask::none;
	}

	// When a size is proposed and a cell cannot exactly match that size, it returns up to 4 sizes.
	// quadrant_flag reflects those sizes as quadrants relevative to the requested size.

	// In a simple resizing scenario (not involving collaborative sibling cells), only one size is returned.
	//
	// The priority order is:
	//    (  1  ) - lesser_x_lesser_y
	//    (2 & 3) - lesser_x_greater_y or greater_x_lesser_y, depending on the flow direction.
	//                  greater_x_lesser_y first, if the flow dimension is horizontal.
	//                  lesser_x_greater_y first, if the flow dimension is vertical.
	//    (  4  ) - greater_x_greater_y

	std::optional<size> calculate_size(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<dimension>& resizeDimension = std::nullopt,
		bool preferGreaterWidth = false,
		bool preferGreaterHeight = false,
		bool nearestGreater = false) const
	{
		std::optional<quadrant_mask> quadrants;
		if (preferGreaterWidth || preferGreaterHeight || nearestGreater)
			quadrants = all_quadrants;
		collaborative_sizes result = calculate_collaborative_sizes(sz, r, quadrants, resizeDimension);
		std::optional<size> sz2;
		if (resizeDimension.has_value())
			sz2 = result.get_nearest(sz, *resizeDimension, nearestGreater);
		else
			sz2 = result.find_first_valid_size(get_primary_flow_dimension(), preferGreaterWidth, preferGreaterHeight);
		return sz2;
	}

	// Additional sizes are needed to by parent cells (such as stack_panel) that calculate their
	// size based on interdependent sizes of its children.

	// Up to 4 different sizes may be returned from calculate_collaborative_sizes()
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

	// The quadrants argument indicates which quadrants the caller needs values for, in case there can be an
	// efficiency gain by omitting unnecessary quadrants.

	// A quadrants of nullopt indicates that a result from only 1 quadarant is needed, but it's unknown which
	// quadrant that might be.

	// A quadrant_mask::none is invalid, and may result in no values being returned.

	virtual collaborative_sizes calculate_collaborative_sizes(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<quadrant_mask>& quadrants = std::nullopt,
		const std::optional<dimension>& resizeDimension = std::nullopt) const
	{
		(void)resizeDimension;
		(void)quadrants;
		collaborative_sizes result;
		range r2 = get_range() & r;
		if (!r2.is_invalid())
		{
			size sz2 = r2.get_limit(sz);
			result.set_relative_to(sz2, sz);
		}
		return result;
	}

protected:
	virtual void calculate_range() { calculating_range(); }
	virtual void calculating_range() { }

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
	static void calculating_range(cell& c) { c.calculating_range(); }
	static void reshape(cell& c, const bounds& newBounds, const point& oldOrigin = point(0, 0)) { c.reshape(newBounds, oldOrigin); }
};


}
}
}


#endif
