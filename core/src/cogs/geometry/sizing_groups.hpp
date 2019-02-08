//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_SIZING_GROUPS
#define COGS_SIZING_GROUPS


#include "cogs/collections/multimap.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/geometry/proportion.hpp"
#include "cogs/geometry/range.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


namespace cogs {
namespace geometry {



// sizing_group is a utility class for rects with subrects arranged in rows or columns, which need to grow
// or shrink according to some set of rules.


class proportional_sizing_group
{
public:
	class cell
	{
	protected:
		friend class proportional_sizing_group;

		double	m_length;

	public:
		linear::range	m_range;
		double	m_default;
		
		cell()
			:	m_default(0),
				m_length(0)
		{ }
		
		cell(double defaultSize, const linear::range& r)
			:	m_default(defaultSize),
				m_range(r),
				m_length(0)
		{}

		double get_length() const		{ return m_length; }
	};

private:
	linear::range	m_range;
	double	m_default;
	double	m_length;

	nonvolatile_multimap<double, rcref<cell>, false>	m_minProportions;
	nonvolatile_multimap<double, rcref<cell>, false>	m_maxProportions;

public:
	proportional_sizing_group()
		:	m_default(0),
			m_length(0)
	{
		m_range.set_fixed(0);
	}

	void clear()
	{
		m_length = 0;
		m_default = 0;
		m_range.set_fixed(0);
		m_minProportions.clear();
		m_maxProportions.clear();
	}

	const linear::range& get_range() const	{ return m_range; }
	double get_default() const		{ return m_default; }
	double get_length() const		{ return m_length; }

	void add_cell(const rcref<cell>& cellRef)
	{
		cell& c = *cellRef;
		m_range += c.m_range;
		double tmpDefault = c.m_default;
			
		if (c.m_range.has_max() && (tmpDefault > c.m_range.get_max()))
			tmpDefault = c.m_range.get_max();
		else if (tmpDefault < c.m_range.get_min())
			tmpDefault = c.m_range.get_min();
		m_default += tmpDefault;

		double maxProportions = 0;
		if (c.m_range.has_max())
			maxProportions = c.m_range.get_max() / tmpDefault;
		
		m_maxProportions.insert(maxProportions, cellRef);
		m_minProportions.insert(c.m_range.get_min() / tmpDefault, cellRef);
	}

	void calculate_sizes(double proposedSize)
	{
		size_t numCells = m_minProportions.size();
		if (!numCells)
		{
			m_length = 0;
			return;
		}
		if (proposedSize <= m_range.get_min())	// size to min
		{
			m_length = m_range.get_min();
			typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_minProportions.get_first();	// doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_range.get_min();
				++itor;
			}
		}
		else if (proposedSize == m_default)		// size to default
		{
			m_length = m_default;
			typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_minProportions.get_first();	// doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_default;
				++itor;
			}
		}
		else
		{
			m_length = 0;
			double remaining = proposedSize;
			double remainingDefault = m_default;
			if (proposedSize < m_default)		// shrink proportionally to default size
			{
				typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_minProportions.get_last();
				while (!!itor)
				{
					cell& c = (**itor);
					double targetProportion = remaining / remainingDefault;
					double newSize;
					if (targetProportion < itor.get_key())
						newSize = (c.m_range.get_min());
					else
						newSize = (c.m_default * targetProportion);
					remaining -= newSize;
					remainingDefault -= c.m_default;
					c.m_length = newSize;
					m_length += newSize;
					--itor;
				}
			}
			else // if (proposedSize > m_default)	// grow proportionally to default size
			{
				typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_maxProportions.get_last();
				while (!!itor)
				{
					if (!remainingDefault)
						break; // All remaining elements have 0 default size, so have no proportional size

					cell& c = (**itor);
					double targetProportion = remaining / remainingDefault;
					double newSize;
					if (c.m_range.has_max() && (targetProportion > itor.get_key()))
						newSize = (c.m_range.get_max());
					else
						newSize = (c.m_default * targetProportion);
					remaining -= newSize;
					remainingDefault -= c.m_default;
					c.m_length = newSize;
					m_length += newSize;
					--itor;
				}
			}
		}
	}
};


// equal_sizing_group allocates space to, or removes space from, all cells equally.
// Based on the default size, if additional space is available, it's allocated equally beyond the default size.
// If less space than default is available, space will be removed equally until cells reach their minimum sizes.


class equal_sizing_group
{
public:
	class cell
	{
	protected:
		friend class equal_sizing_group;
			
		double	m_length;

	public:
		linear::range	m_range;
		double	m_default;
		
		cell()
			:	m_default(0),
				m_length(0)
		{ }
		
		cell(double defaultSize, const linear::range& r)
			:	m_default(defaultSize),
				m_range(r),
				m_length(0)
		{}

		double get_length() const		{ return m_length; }
	};

private:
	linear::range	m_range;
	double	m_default;
	double	m_length;
				
	nonvolatile_multimap<linear::range, rcref<cell>, false>	m_sortedByDefaultMaxRange;
	nonvolatile_multimap<double, rcref<cell>, false>	m_sortedByMinDefaultGap;
		
public:
	equal_sizing_group()
		:	m_default(0),
			m_length(0)
	{
		m_range.set_fixed(0);
	}

	void clear()
	{
		m_length = 0;
		m_default = 0;
		m_range.set_fixed(0);
		m_sortedByMinDefaultGap.clear();
		m_sortedByDefaultMaxRange.clear();
	}

	const linear::range& get_range() const		{ return m_range; }
	double get_default() const		{ return m_default; }
	double get_length() const		{ return m_length; }

	void add_cell(const rcref<cell>& cellRef)
	{
		cell& c = *cellRef;
		m_range += c.m_range;
		double tmpDefault = c.m_default;
		if (c.m_range.has_max() && (tmpDefault > c.m_range.get_max()))
			tmpDefault = c.m_range.get_max();
		else if (tmpDefault < c.m_range.get_min())
			tmpDefault = c.m_range.get_min();
		m_default += tmpDefault;

		m_sortedByDefaultMaxRange.insert(linear::range(c.m_default, c.m_range.get_max(), c.m_range.has_max()), cellRef);
		double gap(c.m_default);
		gap -= c.m_range.get_min();
		m_sortedByMinDefaultGap.insert(gap, cellRef);
	}

	void calculate_sizes(double proposedSize)
	{
		size_t numCells = m_sortedByMinDefaultGap.size();
		if (!numCells)
		{
			m_length = 0;
			return;
		}
		if (proposedSize == m_default)	// use default size
		{
			m_length = m_default;

			// doesn't matter which map we use to iterate through all cells
			typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_default;	// use pre-calculated default clipped to range
				++itor;
			}
		}
		else if (proposedSize <= m_range.get_min())	// use min size
		{
			m_length = m_range.get_min();
			typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_range.get_min();
				++itor;
			}
		}
		else if (m_range.has_max() && (m_range.get_max() <= proposedSize))	// use max size
		{
			m_length = m_range.get_max();
			typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_range.get_max();
				++itor;
			}
		}
		else
		{
			double remaining = proposedSize;
			m_length = proposedSize;
			if (proposedSize > m_default)	// stretching beyond default
			{
				double growBy = proposedSize;
				growBy -= m_default;
				typename nonvolatile_multimap<linear::range, rcref<cell>, false>::iterator itor = m_sortedByDefaultMaxRange.get_first();
				while (!!itor)
				{
					cell* c = (*itor).get_ptr();
					double targetGrowBy = growBy;
					targetGrowBy /= numCells;
					--numCells;
					double targetSize(c->m_default);
					targetSize += targetGrowBy;
					if (c->m_range.has_max() && (targetSize >= c->m_range.get_max()))
					{
						c->m_length = c->m_range.get_max();
						growBy += c->m_default;
						growBy -= c->m_range.get_max();
					}
					else
					{
						for (;;)
						{
							c->m_length = targetSize;
							growBy -= targetGrowBy;
							if (!++itor)
								break;
							c = (*itor).get_ptr();
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
				typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor = m_sortedByMinDefaultGap.get_first();
				while (!!itor)
				{
					cell* c = (*itor).get_ptr();
					double targetShrinkBy = shrinkBy;
					targetShrinkBy /= numCells;
					--numCells;
					if (targetShrinkBy >= itor.get_key())
					{
						c->m_length = c->m_range.get_min();
						shrinkBy += c->m_range.get_min();
						shrinkBy -= c->m_default;
					}
					else
					{
						for (;;)
						{
							c->m_length = c->m_default;
							c->m_length -= targetShrinkBy;
							shrinkBy -= targetShrinkBy;
							if (!++itor)
								break;
							c = (*itor).get_ptr();
							targetShrinkBy = shrinkBy;
							targetShrinkBy /= numCells;
							--numCells;
						}
						break;
					}
					++itor;
				}
			}
		}
	}
};

	
// fair_sizing_group allocates space to smaller cells, or removes from larger cells, until all cells are equal.
// Cells are then sized equally.
class fair_sizing_group
{
public:
	class cell
	{
	protected:
		friend class fair_sizing_group;
			
		double	m_highWaterTotal;
		double	m_lowWaterTotal;

		size_t	m_cellPosition;
		size_t	m_cellReversePosition;

		double	m_length;

	public:
		linear::range	m_range;
		double	m_default;
		

		cell()
			:	m_default(0),
				m_length(0)
		{ }

		cell(double defaultSize, const linear::range& r)
			:	m_default(defaultSize),
				m_range(r),
				m_length(0)
		{}

		double get_length() const		{ return m_length; }
	};

private:
	linear::range	m_range;
	double	m_default;
	double	m_length;
		
	nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::maximum_comparator>	m_sortedByMaximumSize;
	nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::minimum_comparator>	m_sortedByMinimumSize;

	nonvolatile_multimap<double, rcref<cell>, false>	m_sortedByDefaultSize;

	nonvolatile_multimap<double, rcref<cell>, false>	m_sortedByHighWaterTotal;
	nonvolatile_multimap<double, rcref<cell>, false>	m_sortedByLowWaterTotal;

	bool									m_updated;
	
public:
	fair_sizing_group()
		:	m_default(0),
			m_updated(false),
			m_length(0)
	{
		m_range.set_fixed(0);
	}

	void clear()
	{
		m_length = 0;
		m_default = 0;
		m_range.set_fixed(0);
		m_sortedByDefaultSize.clear();
		m_sortedByMaximumSize.clear();
		m_sortedByMinimumSize.clear();
	}

	const linear::range& get_range() const	{ return m_range; }
	double get_default() const		{ return m_default; }
	double get_length() const		{ return m_length; }

	void add_cell(const rcref<cell>& cellRef)
	{
		m_updated = true;
		m_sortedByHighWaterTotal.clear();
		m_sortedByLowWaterTotal.clear();

		cell& c = *cellRef;
		m_range += c.m_range;
		double tmpDefault = c.m_default;
		if (c.m_range.has_max() && (tmpDefault > c.m_range.get_max()))
			tmpDefault = c.m_range.get_max();
		else if (tmpDefault < c.m_range.get_min())
			tmpDefault = c.m_range.get_min();
		m_default += tmpDefault;

		m_sortedByDefaultSize.insert(tmpDefault, cellRef);
		m_sortedByMaximumSize.insert(c.m_range, cellRef);
		m_sortedByMinimumSize.insert(c.m_range, cellRef);
	}

	void calculate_sizes(double proposedSize)
	{
		size_t numCells = m_sortedByDefaultSize.size();
		if (!numCells)
		{
			m_length = 0;
			return;
		}
		typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor;
		if (proposedSize == m_default)	// use default size
		{
			m_length = m_default;
			itor = m_sortedByDefaultSize.get_first();	// doesn't matter which map we use to iterate through all cells
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = itor.get_key();	// use pre-calculated default clipped to range
				++itor;
			}
		}
		else if (proposedSize <= m_range.get_min())	// use min size
		{
			m_length = m_range.get_min();
			itor = m_sortedByDefaultSize.get_first();
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_range.get_min();
				++itor;
			}
		}
		else if (m_range.has_max() && (m_range.get_max() <= proposedSize))	// use max size
		{
			m_length = m_range.get_max();

			itor = m_sortedByDefaultSize.get_first();
			while (!!itor)
			{
				cell& c = (**itor);
				c.m_length = c.m_range.get_max();	// use pre-calculated default clipped to range
				++itor;
			}		
		}
		else
		{
			if (m_updated)
			{
				m_updated = false;	
				size_t cellPosition = 0;
				double prevSize = 0;
				double numElementsBefore = 0;
				double prevHighWaterTotal = 0;
				typename nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::maximum_comparator>::iterator maxItor = m_sortedByMaximumSize.get_first();
				itor = m_sortedByDefaultSize.get_first();
				while (!!itor)
				{
					cell& c = (**itor);
					double additionalSpace = 0;
					if (numElementsBefore > 0)
					{
						additionalSpace = c.m_default;
						additionalSpace -= prevSize;
						additionalSpace *= numElementsBefore;
						do {
							if (!((*maxItor)->m_range.has_max()))
								break;
							double max = (*maxItor)->m_range.get_max();
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
					m_sortedByHighWaterTotal.insert(prevHighWaterTotal, *itor);
					prevSize = c.m_default;
					prevHighWaterTotal = c.m_highWaterTotal;
					++numElementsBefore;
					++itor;
				}
					
				cellPosition = 0;
				prevSize = 0;
				numElementsBefore = 0;
				double prevLowWaterTotal = 0;
				typename nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::minimum_comparator>::iterator minItor = m_sortedByMinimumSize.get_last();
				itor = m_sortedByDefaultSize.get_last();
				while (!!itor)
				{
					cell& c = (**itor);
							
					double additionalSpace = 0;
					if (numElementsBefore > 0)
					{
						additionalSpace = prevSize;
						additionalSpace -= c.m_default;
						additionalSpace *= numElementsBefore;
						do {
							double min = (*minItor)->m_range.get_min();
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
					m_sortedByLowWaterTotal.insert(prevLowWaterTotal, *itor);

					prevSize = c.m_default;
					prevLowWaterTotal = c.m_lowWaterTotal;

					++numElementsBefore;
					--itor;
				}
			}

			double remaining = proposedSize;
			m_length = proposedSize;

			if (proposedSize > m_default)	// stretching beyond default
			{
				double growBy = proposedSize;
				growBy -= m_default;

				itor = m_sortedByHighWaterTotal.find_first_equal_or_nearest_greater_than(growBy);
				if (!!itor)	// Set cells above the water line to defaults
				{
					typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor2 = itor;
					do {
						cell& c = (**itor2);
						c.m_length = c.m_default;
						remaining -= c.m_default;
					} while (!!++itor2);
					--itor;
				}
				else
					itor = m_sortedByHighWaterTotal.get_last();
	
				size_t numLeft = (*itor)->m_cellPosition;
				typename nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::maximum_comparator>::iterator maxItor = m_sortedByMaximumSize.get_first();
				do {
					const linear::range& cellRange = maxItor.get_key();
					if (!cellRange.has_max())
						break;
					auto targetSize = (remaining / numLeft);
					if (targetSize <= cellRange.get_max())
						break;
					(*maxItor)->m_length = cellRange.get_max();
					remaining -= cellRange.get_max();
					--numLeft;
				} while (!!++maxItor);
					
				do {
					cell& c = (**itor);
					double targetSize = remaining;
					targetSize /= numLeft;
					if (!c.m_range.has_max() || (c.m_range.get_max() > targetSize))
					{
						c.m_length = targetSize;
						if (!--numLeft)
							break;
						remaining -= targetSize;
					}
				} while (!!--itor);
			}
			else // if (proposedSize < m_default)
			{
				double shrinkBy = m_default;
				shrinkBy -= proposedSize;

				itor = m_sortedByLowWaterTotal.find_first_equal_or_nearest_greater_than(shrinkBy);
				if (!!itor)	// Set cells below the water line to defaults
				{
					typename nonvolatile_multimap<double, rcref<cell>, false>::iterator itor2 = itor;
					do {
						cell& c = (**itor2);
						c.m_length = c.m_default;
						remaining -= c.m_default;
					} while (!!++itor2);
					--itor;
				}
				else
					itor = m_sortedByLowWaterTotal.get_last();

				size_t numLeft = (*itor)->m_cellReversePosition;
				typename nonvolatile_multimap<linear::range, rcref<cell>, false, typename linear::range::minimum_comparator>::iterator minItor = m_sortedByMinimumSize.get_last();
				do {
					const linear::range& cellRange = minItor.get_key();
					if (cellRange.get_min() == 0)
						break;
					auto targetSize = (remaining / numLeft);
					if (targetSize >= cellRange.get_min())
						break;
					(*minItor)->m_length = cellRange.get_min();
					remaining -= cellRange.get_min();
					--numLeft;
				} while (!!--minItor);

				do {
					cell& c = (**itor);
					double targetSize;
					targetSize = remaining;
					targetSize /= numLeft;
					if (targetSize <= c.m_range.get_min())
						targetSize = c.m_range.get_min();
					else
					{
						c.m_length = targetSize;
						if (!--numLeft)
							break;
						remaining -= targetSize;
					}
				} while (!!--itor);
			}
		}
	}
};

}
}

#pragma warning(pop)


#endif

