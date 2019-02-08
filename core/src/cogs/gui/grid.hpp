//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_GUI_GRID
#define COGS_GUI_GRID


#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/multimap.hpp"
#include "cogs/geometry/dimension.hpp"
#include "cogs/gui/pane.hpp"


namespace cogs {
namespace gui {

/// @ingroup GUI
/// @brief A pane that divides child panes into rows and/or columns.
/// 
/// A grid can be configured to allocate space to cells either fairly, or proportional to default cell sizes.
/// Panes nested in cells can impose constraints on the cell they are in, and suggest default sizes.  Those ranges
/// and defaults are used to determine the ranges and defaults of rows and columns.
/// Cells cannot span rows or span columns.  Rows or columns without cells are consider empty and not allocated any space.
template <dimension primary_dimension = dimension::horizontal, typename primary_key_t = size_t, typename secondary_key_t = size_t, class primary_sizing_group_t = proportional_sizing_group, class secondary_sizing_group_t = primary_sizing_group_t>
class grid : public pane
{
private:
	class primary_row_t;
	class secondary_row_t;

	typedef nonvolatile_multimap<  primary_key_t, rcref<  primary_row_t>, true>	  primary_map_t;
	typedef nonvolatile_multimap<secondary_key_t, rcref<secondary_row_t>, true>	secondary_map_t;
	
	class cell_t : public frame
	{
	public:
		weak_rcptr<primary_row_t>		m_primaryRow;
		weak_rcptr<secondary_row_t>		m_secondaryRow;

		typename container_dlist<rcref<cell_t> >::remove_token m_verticalRemoveTokens;
		typename container_dlist<rcref<cell_t> >::remove_token m_horizontalRemoveTokens;

		typename container_dlist<rcref<cell_t> >::remove_token& get_remove_token(dimension d)
		{
			if (d == dimension::vertical)
				return m_verticalRemoveTokens;
			return m_horizontalRemoveTokens;
		}

		cell_t(const rcref<cell>& c)
			: frame(c)
		{ }

		virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
		{
			bounds newBounds;

			rcptr<primary_row_t> primaryRow = m_primaryRow;
			rcptr<secondary_row_t> secondaryRow = m_secondaryRow;

			if (primary_dimension == dimension::horizontal)
				newBounds.set(point(primaryRow->m_offset, secondaryRow->m_offset), size(primaryRow->m_cachedLength, secondaryRow->m_cachedLength));
			else
				newBounds.set(point(secondaryRow->m_offset, primaryRow->m_offset), size(secondaryRow->m_cachedLength, primaryRow->m_cachedLength));

			frame::reshape(newBounds, oldOrigin);
		}
	};

	// A row or column object exists only for rows or columns with panes nested in them.
	class primary_row_t : public primary_sizing_group_t::cell
	{
	public:
		typename primary_map_t::iterator	m_removeToken;

		container_dlist<rcref<cell_t> >	m_cells;

		double m_offset;
		double m_cachedLength;
	};
	
	class secondary_row_t : public secondary_sizing_group_t::cell
	{
	public:
		typename secondary_map_t::iterator	m_removeToken;

		container_dlist<rcref<cell_t> >	m_cells;

		double m_offset;
		double m_cachedLength;
	};
	
	//----Configuration data
	primary_map_t		m_primaryRows;
	secondary_map_t		m_secondaryRows;

	mutable primary_sizing_group_t		m_primarySizingGroup;
	mutable secondary_sizing_group_t	m_secondarySizingGroup;
	
	//----State data
	bool m_wasRecalculated;
	mutable double m_cachedLength;

	range m_currentRange;
	size m_currentDefaultSize;
	
	virtual range get_range() const	{ return m_currentRange; }
	virtual size get_default_size() const	{ return m_currentDefaultSize; }
	
	virtual void calculate_range()
	{
		pane::calculate_range();

		// Need to scan each row and each column, to determine total min, max, default for each.

		// By scanning in one dimension and hitting all cells in those rows, we visit everything.  No need to scan again in the other dimension.
		// ... But, we do need to reset some counters in the other dimension, so reset them now

		m_primarySizingGroup.clear();
		m_secondarySizingGroup.clear();
		
		typename secondary_map_t::iterator itor = m_secondaryRows.get_first();	//m_rowsAndColumns[dimension::vertical].get_first();
		while (!!itor)
		{
			secondary_row_t& r2 = **itor;
			r2.m_range.clear();
			r2.m_default = 0;

			++itor;
		}
		
		typename primary_map_t::iterator itor2 = m_primaryRows.get_first();	//m_rowsAndColumns[dimension::horizontal].get_first();
		while (!!itor2)
		{
			primary_row_t& r = **itor2;
			r.m_range.clear();
			r.m_default = 0;

			typename container_dlist<rcref<cell_t> >::iterator cellItor = r.m_cells.get_first();
			while (!!cellItor)
			{
				cell_t& cell = **cellItor;

				rcptr<secondary_row_t> otherRow = cell.m_secondaryRow;	//cell.m_rows[dimension::vertical];
				secondary_row_t& r2 = *otherRow;

				range cellRange = cell.get_range();
				if (primary_dimension == dimension::horizontal)
					cellRange.set(cellRange[dimension::horizontal] & r.m_range, cellRange[dimension::vertical] & r2.m_range);
				else
					cellRange.set(cellRange[dimension::horizontal] & r2.m_range, cellRange[dimension::vertical] & r.m_range);
				r.m_range = cellRange[primary_dimension];
				r2.m_range = cellRange[!primary_dimension];

				size cellDefaultSize = cellRange.limit(cell.get_default_size());

				if (r.m_default < cellDefaultSize[primary_dimension])
					r.m_default = cellDefaultSize[primary_dimension];
				if (r2.m_default < cellDefaultSize[!primary_dimension])
					r2.m_default = cellDefaultSize[!primary_dimension];

				++cellItor;
			}
			//m_sizingGroups[dimension::horizontal].add_cell(*itor2);
			m_primarySizingGroup.add_cell(*itor2);
			++itor2;
		}
		
		itor = m_secondaryRows.get_first();	//m_rowsAndColumns[dimension::vertical].get_first();
		while (!!itor)
		{
			//secondary_row_t& r2 = **itor;
			//m_sizingGroups[dimension::vertical].add_cell(*itor);
			m_secondarySizingGroup.add_cell(*itor);
			++itor;
		}
		
		range rng;
		size defaultSize;
		if (primary_dimension == dimension::horizontal)
		{
			rng.set(m_primarySizingGroup.get_range(), m_secondarySizingGroup.get_range());
			defaultSize.set(m_primarySizingGroup.get_default(), m_secondarySizingGroup.get_default());
		}
		else
		{
			rng.set(m_secondarySizingGroup.get_range(), m_primarySizingGroup.get_range());
			defaultSize.set(m_secondarySizingGroup.get_default(), m_primarySizingGroup.get_default());
		}

		m_currentDefaultSize = rng.limit(defaultSize);
		m_currentRange = rng;
		m_wasRecalculated = true;
	}

	virtual dimension get_primary_flow_dimension() const { return primary_dimension; }

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double rtn = proposed;//pane::propose_length(d, proposed, rtnOtherRange);

		if (d == primary_dimension)
		{
			m_cachedLength = proposed;

	//		size defaultSize = get_default_size();

			typename secondary_map_t::iterator itor = m_secondaryRows.get_first();	//m_rowsAndColumns[!d].get_first();
			while (!!itor)
			{
				secondary_row_t& r2 = **itor;
				r2.m_range.clear();
				++itor;
			}

			m_primarySizingGroup.calculate_sizes(proposed);
			rtn = m_primarySizingGroup.get_length();

			m_secondarySizingGroup.clear();

			itor = m_secondaryRows.get_first();
			while (!!itor)
			{
				secondary_row_t& r = **itor;
				r.m_range.clear();
				//r.m_default = 0;

				typename container_dlist<rcref<cell_t> >::iterator cellItor = r.m_cells.get_first();
				while (!!cellItor)
				{
					cell_t& cell = **cellItor;
					rcptr<primary_row_t> otherRow = cell.m_primaryRow;	//cell.m_rows[d];
					primary_row_t& r2 = *(otherRow);

					//double ignoredRtn;
					range::linear_t otherRange;
					cell.propose_length(d, r2.get_length(), otherRange);	// return value is ignored

					r.m_range &= otherRange;

					++cellItor;
				}
				m_secondarySizingGroup.add_cell(*itor);
				++itor;
			}

			rtnOtherRange = m_secondarySizingGroup.get_range();
		}
		return rtn;
	}
	
	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		size newSize;
		range::linear_t otherRange;
		newSize[d] = propose_length(d, proposedSize[d], otherRange);
		newSize[!d] = otherRange.limit(proposedSize[!d]);
		return newSize;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		dimension d = get_primary_flow_dimension();
		return propose_lengths(d, proposedSize);
	}

	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		double oldCachedLength = m_cachedLength;
		size newSize(r.get_size());

		if (m_wasRecalculated || (r.get_size(primary_dimension) != oldCachedLength))
		{
			m_wasRecalculated = false;
			newSize = propose_size(r.get_size());
		}
		
		m_secondarySizingGroup.calculate_sizes(newSize[!primary_dimension]);

		typename primary_map_t::iterator itor = m_primaryRows.get_first();	//m_rowsAndColumns[dimension::horizontal].get_first();
		double curOffset = 0;
		while (!!itor)
		{
			primary_row_t& r = **itor;
			r.m_offset = curOffset;
			r.m_cachedLength = r.get_length();
			curOffset += r.m_cachedLength;
			++itor;
		}
			
		typename secondary_map_t::iterator itor2 = m_secondaryRows.get_first();	//m_rowsAndColumns[dimension::vertical].get_first();
		curOffset = 0;
		while (!!itor2)
		{
			secondary_row_t& r = **itor2;
			r.m_offset = curOffset;
			r.m_cachedLength = r.get_length();
			curOffset += r.m_cachedLength;
			++itor2;
		}

		pane::reshape(r, oldOrigin);
	}
	
	virtual void detaching_child(const rcref<pane>& p)
	{
		rcptr<cell_t> c = p->get_outermost_frame().static_cast_to<cell_t>();

		rcptr<primary_row_t>	primaryRow = c->m_primaryRow;
		rcptr<secondary_row_t>	secondaryRow = c->m_secondaryRow;

		primaryRow->m_cells.remove(c->get_remove_token(primary_dimension));
		if (!primaryRow->m_cells)	// remove row
			m_primaryRows.remove(primaryRow->m_removeToken);

		secondaryRow->m_cells.remove(c->get_remove_token(!primary_dimension));
		if (!secondaryRow->m_cells)	// remove row
			m_secondaryRows.remove(secondaryRow->m_removeToken);

		pane::detaching_child(p);
	}

public:
	grid()
		:	m_wasRecalculated(false)
	{ }
	
	void nest(const rcref<pane>& child, const primary_key_t& primaryKey, const secondary_key_t& secondaryKey, const rcptr<frame>& f = 0)
	{
		rcptr<canvas::cell> f2 = f;
		if (!f2)
			f2 = child;
		rcref<cell_t> c = rcnew(cell_t, f2.dereference());
		rcptr<primary_row_t> r;
		typename primary_map_t::iterator itor = m_primaryRows.find_any_equal(primaryKey);
		if (!!itor)
			r = *itor;
		else
		{
			rcref<primary_row_t> newRow = rcnew(primary_row_t);
			newRow->m_removeToken = m_primaryRows.insert(primaryKey, newRow);
			r = newRow;
		}
		c->m_primaryRow = r;
		c->get_remove_token(primary_dimension) = r->m_cells.append(c);

		rcptr<secondary_row_t> r2;
		typename secondary_map_t::iterator itor2 = m_secondaryRows.find_any_equal(secondaryKey);
		if (!!itor2)
			r2 = *itor2;
		else
		{
			rcref<secondary_row_t> newRow = rcnew(secondary_row_t);
			newRow->m_removeToken = m_secondaryRows.insert(secondaryKey, newRow);
			r2 = newRow;
		}
		c->m_secondaryRow = r2;
		c->get_remove_token(!primary_dimension) = r2->m_cells.append(c);

		pane::nest(child, c);
	}
	
};


}
}


#endif

