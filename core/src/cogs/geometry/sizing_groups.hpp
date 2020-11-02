//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_SIZING_GROUPS
#define COGS_HEADER_GEOMETRY_SIZING_GROUPS


#include "cogs/collections/multimap.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/geometry/proportion.hpp"
#include "cogs/geometry/range.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace geometry {


// sizing_group is a utility class for rects with subrects arranged in rows or columns, which need to grow
// or shrink according to some set of rules.


enum class sizing_disposition
{
	// An equivalent amount of space is added to or removed from each node
	none,

	// The default proportions of nodes are retained
	proportional,

	// Smaller nodes are given additional space first.  Larger nodes have space removed first.
	converging
};


class sizing_group_base;


template <sizing_disposition disposition>
class sizing_group
{
};


class sizing_cell
{
private:
	double m_sizedLength;

	double m_highWaterTotal;
	double m_lowWaterTotal;

	size_t m_cellPosition;
	size_t m_cellReversePosition;

	friend class sizing_group_base;
	template <sizing_disposition>
	friend class sizing_group;

	linear::range m_range;
	double m_default;

public:
	sizing_cell()
	{ }

	sizing_cell(const linear::range& r, double defaultSize)
		: m_range(r),
		m_default(defaultSize)
	{ }

	void reset_cell(const linear::range& r, double defaultSize)
	{
		m_range = r;
		m_default = defaultSize;
	}

	void set_sized_length(double d) { m_sizedLength = d; }
	double get_sized_length() const { return m_sizedLength; }
};


class sizing_group_base
{
protected:
	linear::range m_range;
	double m_default = 0.0;

public:
	const linear::range& get_range() const { return m_range; }
	double get_default() const { return m_default; }

	sizing_group_base()
	{
		m_range.set_fixed();
	}

	virtual void clear()
	{
		m_default = 0;
		m_range.set_fixed();
	}

	virtual void add_cell(const rcref<sizing_cell>& cellRef)
	{
		sizing_cell& c = *cellRef;
		m_range += c.m_range;
		COGS_ASSERT(m_range.contains(c.m_default));
		m_default += c.m_default;
	}

	virtual double calculate_sizes(double proposedSize) = 0;
};


// sizing_group<none> allocates space to, or removes space from, all cells equally.
// Based on the default size, if additional space is available, it's allocated equally beyond the default size.
// If less space than default is available, space will be removed equally until cells reach their minimum sizes.
template <>
class sizing_group<sizing_disposition::none> : public sizing_group_base
{
private:
	nonvolatile_multimap<linear::range, rcref<sizing_cell>, false> m_sortedByDefaultMaxRange;
	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_sortedByMinDefaultGap;

public:
	virtual void clear()
	{
		sizing_group_base::clear();
		m_sortedByMinDefaultGap.clear();
		m_sortedByDefaultMaxRange.clear();
	}

	virtual void add_cell(const rcref<sizing_cell>& cellRef)
	{
		sizing_group_base::add_cell(cellRef);
		sizing_cell& c = *cellRef;
		m_sortedByDefaultMaxRange.insert(linear::range(c.m_default, c.m_range.get_max(), c.m_range.has_max()), cellRef);
		double gap(c.m_default);
		gap -= c.m_range.get_min();
		m_sortedByMinDefaultGap.insert(gap, cellRef);
	}

	virtual double calculate_sizes(double proposedSize)
	{
		size_t numCells = m_sortedByMinDefaultGap.size();
		if (!numCells)
			return 0;
		if (proposedSize == m_default) // use default size
		{
			// doesn't matter which map we use to iterate through all cells
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_default; // use pre-calculated default clipped to range
				++itor;
			}
			return m_default;
		}
		if (proposedSize <= m_range.get_min()) // use min size
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_range.get_min();
				++itor;
			}
			return m_range.get_min();
		}
		if (m_range.has_max() && (m_range.get_max() <= proposedSize)) // use max size
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_range.get_max();
				++itor;
			}
			return m_range.get_max();
		}
		if (proposedSize > m_default) // stretching beyond default
		{
			double growBy = proposedSize;
			growBy -= m_default;
			typename nonvolatile_multimap<linear::range, rcref<sizing_cell>, false>::iterator itor = m_sortedByDefaultMaxRange.get_first();
			while (!!itor)
			{
				sizing_cell* c = itor->value.get_ptr();
				double targetGrowBy = growBy;
				targetGrowBy /= numCells;
				--numCells;
				double targetSize(c->m_default);
				targetSize += targetGrowBy;
				if (c->m_range.has_max() && (targetSize >= c->m_range.get_max()))
				{
					c->m_sizedLength = c->m_range.get_max();
					growBy += c->m_default;
					growBy -= c->m_range.get_max();
				}
				else
				{
					for (;;)
					{
						c->m_sizedLength = targetSize;
						growBy -= targetGrowBy;
						if (!++itor)
							break;
						c = itor->value.get_ptr();
						targetGrowBy = growBy;
						targetGrowBy /= numCells;
						--numCells;
						targetSize = c->m_default;
						targetSize += targetGrowBy;
					}
					break;
				}
				++itor;
			}
		}
		else // if (proposedSize < m_default)
		{
			double shrinkBy = m_default;
			shrinkBy -= proposedSize;
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				sizing_cell* c = itor->value.get_ptr();
				double targetShrinkBy = shrinkBy;
				targetShrinkBy /= numCells;
				--numCells;
				if (targetShrinkBy >= itor.get_key())
				{
					c->m_sizedLength = c->m_range.get_min();
					shrinkBy += c->m_range.get_min();
					shrinkBy -= c->m_default;
				}
				else
				{
					for (;;)
					{
						c->m_sizedLength = c->m_default;
						c->m_sizedLength -= targetShrinkBy;
						shrinkBy -= targetShrinkBy;
						if (!++itor)
							break;
						c = itor->value.get_ptr();
						targetShrinkBy = shrinkBy;
						targetShrinkBy /= numCells;
						--numCells;
					}
					break;
				}
				++itor;
			}
		}
		return proposedSize;
	}
};


template <>
class sizing_group<sizing_disposition::proportional> : public sizing_group_base
{
private:
	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_minProportions;
	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_maxProportions;

public:
	virtual void clear()
	{
		sizing_group_base::clear();
		m_minProportions.clear();
		m_maxProportions.clear();
	}

	virtual void add_cell(const rcref<sizing_cell>& cellRef)
	{
		sizing_group_base::add_cell(cellRef);
		sizing_cell& c = *cellRef;
		double maxProportions = 0;
		if (c.m_range.has_max())
			maxProportions = c.m_range.get_max() / c.m_default;
		m_maxProportions.insert(maxProportions, cellRef);
		m_minProportions.insert(c.m_range.get_min() / c.m_default, cellRef);
	}

	virtual double calculate_sizes(double proposedSize)
	{
		size_t numCells = m_minProportions.size();
		if (!numCells)
			return 0 ;
		if (proposedSize <= m_range.get_min()) // size to min
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_minProportions.get_first(); // doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_range.get_min();
				++itor;
			}
			return m_range.get_min();
		}
		if (proposedSize == m_default) // size to default
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_minProportions.get_first(); // doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_default;
				++itor;
			}
			return m_default;
		}
		double result = 0;
		double remaining = proposedSize;
		double remainingDefault = m_default;
		if (proposedSize < m_default) // shrink proportionally to default size
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_minProportions.get_last();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				double targetProportion = remaining / remainingDefault;
				double newSize;
				if (targetProportion < itor.get_key())
					newSize = (c.m_range.get_min());
				else
					newSize = (c.m_default * targetProportion);
				remaining -= newSize;
				remainingDefault -= c.m_default;
				c.m_sizedLength = newSize;
				result += newSize;
				--itor;
			}
			return result;
		}
		//else // if (proposedSize > m_default) // grow proportionally to default size
		typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor = m_maxProportions.get_last();
		while (!!itor)
		{
			if (!remainingDefault)
				break; // All remaining elements have 0 default size, so have no proportional size

			sizing_cell& c = *itor->value;
			double targetProportion = remaining / remainingDefault;
			double newSize;
			if (c.m_range.has_max() && (targetProportion > itor.get_key()))
				newSize = (c.m_range.get_max());
			else
				newSize = (c.m_default * targetProportion);
			remaining -= newSize;
			remainingDefault -= c.m_default;
			c.m_sizedLength = newSize;
			result += newSize;
			--itor;
		}
		return result;
	}
};


// sizing_group<converging> allocates space to smaller cells, or removes from larger cells, until all cells are equal.
template <>
class sizing_group<sizing_disposition::converging> : public sizing_group_base
{
private:
	nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::maximum_comparator> m_sortedByMaximumSize;
	nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::minimum_comparator> m_sortedByMinimumSize;

	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_sortedByDefaultSize;

	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_sortedByHighWaterTotal;
	nonvolatile_multimap<double, rcref<sizing_cell>, false> m_sortedByLowWaterTotal;

	bool m_updated = false;

public:
	virtual void clear()
	{
		sizing_group_base::clear();
		m_sortedByDefaultSize.clear();
		m_sortedByMaximumSize.clear();
		m_sortedByMinimumSize.clear();
	}

	virtual void add_cell(const rcref<sizing_cell>& cellRef)
	{
		sizing_group_base::add_cell(cellRef);
		sizing_cell& c = *cellRef;
		m_sortedByDefaultSize.insert(c.m_default, cellRef);
		m_sortedByMaximumSize.insert(c.m_range, cellRef);
		m_sortedByMinimumSize.insert(c.m_range, cellRef);
		m_sortedByHighWaterTotal.clear();
		m_sortedByLowWaterTotal.clear();
		m_updated = true;
	}

	virtual double calculate_sizes(double proposedSize)
	{
		size_t numCells = m_sortedByDefaultSize.size();
		if (!numCells)
			return 0;
		typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor;
		if (proposedSize == m_default) // use default size
		{
			itor = m_sortedByDefaultSize.get_first(); // doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = itor.get_key(); // use pre-calculated default clipped to range
				++itor;
			}
			return m_default;
		}
		if (proposedSize <= m_range.get_min()) // use min size
		{
			itor = m_sortedByDefaultSize.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_range.get_min();
				++itor;
			}
			return m_range.get_min();
		}
		if (m_range.has_max() && (m_range.get_max() <= proposedSize)) // use max size
		{
			itor = m_sortedByDefaultSize.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				c.m_sizedLength = c.m_range.get_max(); // use pre-calculated default clipped to range
				++itor;
			}
			return m_range.get_max();
		}

		if (m_updated)
		{
			m_updated = false;
			size_t cellPosition = 0;
			double prevSize = 0;
			double numElementsBefore = 0;
			double prevHighWaterTotal = 0;
			typename nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::maximum_comparator>::iterator maxItor = m_sortedByMaximumSize.get_first();
			itor = m_sortedByDefaultSize.get_first();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;
				double additionalSpace = 0;
				if (numElementsBefore > 0)
				{
					additionalSpace = c.m_default;
					additionalSpace -= prevSize;
					additionalSpace *= numElementsBefore;
					do {
						if (!maxItor->value->m_range.has_max())
							break;
						double max = maxItor->value->m_range.get_max();
						if (c.m_default < max)
							break;
						double gap = c.m_default;
						gap -= max;
						if (gap >= additionalSpace)
							additionalSpace = 0;
						else
							additionalSpace -= gap;
						--numElementsBefore;
						++maxItor;
					} while (!!additionalSpace);
				}
				c.m_cellPosition = ++cellPosition;
				prevHighWaterTotal += additionalSpace;
				c.m_highWaterTotal = prevHighWaterTotal;
				m_sortedByHighWaterTotal.insert(prevHighWaterTotal, itor->value);
				prevSize = c.m_default;
				prevHighWaterTotal = c.m_highWaterTotal;
				++numElementsBefore;
				++itor;
			}

			cellPosition = 0;
			prevSize = 0;
			numElementsBefore = 0;
			double prevLowWaterTotal = 0;
			typename nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::minimum_comparator>::iterator minItor = m_sortedByMinimumSize.get_last();
			itor = m_sortedByDefaultSize.get_last();
			while (!!itor)
			{
				sizing_cell& c = *itor->value;

				double additionalSpace = 0;
				if (numElementsBefore > 0)
				{
					additionalSpace = prevSize;
					additionalSpace -= c.m_default;
					additionalSpace *= numElementsBefore;
					do {
						double min = minItor->value->m_range.get_min();
						if (c.m_default > min)
							break;
						double gap = min;
						gap -= c.m_default;
						if (gap >= additionalSpace)
							additionalSpace = 0;
						else
							additionalSpace -= gap;
						--numElementsBefore;
						--minItor;
					} while (!!additionalSpace);
				}

				c.m_cellReversePosition = ++cellPosition;
				prevLowWaterTotal += additionalSpace;
				c.m_lowWaterTotal = prevLowWaterTotal;
				m_sortedByLowWaterTotal.insert(prevLowWaterTotal, itor->value);

				prevSize = c.m_default;
				prevLowWaterTotal = c.m_lowWaterTotal;

				++numElementsBefore;
				--itor;
			}
		}

		double remaining = proposedSize;
		if (proposedSize > m_default) // stretching beyond default
		{
			double growBy = proposedSize;
			growBy -= m_default;

			itor = m_sortedByHighWaterTotal.find_first_equal_or_nearest_greater_than(growBy);
			if (!!itor) // Set cells above the water line to defaults
			{
				typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor2 = itor;
				do {
					sizing_cell& c = *itor2->value;
					c.m_sizedLength = c.m_default;
					remaining -= c.m_default;
				} while (!!++itor2);
				--itor;
			}
			else
				itor = m_sortedByHighWaterTotal.get_last();

			size_t numLeft = itor->value->m_cellPosition;
			typename nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::maximum_comparator>::iterator maxItor = m_sortedByMaximumSize.get_first();
			do {
				const linear::range& cellRange = maxItor.get_key();
				if (!cellRange.has_max())
					break;
				auto targetSize = (remaining / numLeft);
				if (targetSize <= cellRange.get_max())
					break;
				maxItor->value->m_sizedLength = cellRange.get_max();
				remaining -= cellRange.get_max();
				--numLeft;
			} while (!!++maxItor);

			do {
				sizing_cell& c = *itor->value;
				double targetSize = remaining;
				targetSize /= numLeft;
				if (!c.m_range.has_max() || (c.m_range.get_max() > targetSize))
				{
					c.m_sizedLength = targetSize;
					if (!--numLeft)
						break;
					remaining -= targetSize;
				}
			} while (!!--itor);
			return proposedSize;
		}
		//else // if (proposedSize < m_default)
		double shrinkBy = m_default;
		shrinkBy -= proposedSize;

		itor = m_sortedByLowWaterTotal.find_first_equal_or_nearest_greater_than(shrinkBy);
		if (!!itor) // Set cells below the water line to defaults
		{
			typename nonvolatile_multimap<double, rcref<sizing_cell>, false>::iterator itor2 = itor;
			do {
				sizing_cell& c = *itor2->value;
				c.m_sizedLength = c.m_default;
				remaining -= c.m_default;
			} while (!!++itor2);
			--itor;
		}
		else
			itor = m_sortedByLowWaterTotal.get_last();

		size_t numLeft = itor->value->m_cellReversePosition;
		typename nonvolatile_multimap<linear::range, rcref<sizing_cell>, false, typename linear::range::minimum_comparator>::iterator minItor = m_sortedByMinimumSize.get_last();
		do {
			const linear::range& cellRange = minItor.get_key();
			if (cellRange.get_min() == 0)
				break;
			auto targetSize = (remaining / numLeft);
			if (targetSize >= cellRange.get_min())
				break;
			minItor->value->m_sizedLength = cellRange.get_min();
			remaining -= cellRange.get_min();
			--numLeft;
		} while (!!--minItor);

		do {
			sizing_cell& c = *itor->value;
			double targetSize;
			targetSize = remaining;
			targetSize /= numLeft;
			if (targetSize <= c.m_range.get_min())
				targetSize = c.m_range.get_min();
			else
			{
				c.m_sizedLength = targetSize;
				if (!--numLeft)
					break;
				remaining -= targetSize;
			}
		} while (!!--itor);
		return proposedSize;
	}
};

}
}


#endif
