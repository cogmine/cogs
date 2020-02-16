////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_GUI_LIST
//#define COGS_HEADER_GUI_LIST
//
//
//#include "cogs/collections/multimap.hpp"
//#include "cogs/gui/pane.hpp"
//
//
//namespace cogs {
//namespace gui {
//
//
//template <int list_dimension = dimension::horizontal, typename key_t = size_t, class sizing_group_t = proportional_sizing_group>
//class sorted_list : public pane
//{
//private:
//	typedef sorted_list<list_dimension, key_t, sizing_group_t> this_t;
//
//	class row_t;
//
//	typedef nonvolatile_multimap<key_t, rcref<row_t>, true> map_t;
//
//	class cell_t : public frame
//	{
//	public:
//		weak_rcptr<this_t> m_sortedList;
//		rcptr<frame> m_frame;
//		typename map_t::iterator m_removeToken;
//		double m_offset;
//		double m_cachedLength;
//
//		cell_t(const rcref<this_t>& sortedList, const rcref<pane>& p, const rcptr<frame>& r)
//			: m_sortedList(sortedList),
//			frame(p),
//			m_frame(r)
//		{ }
//
//		virtual size get_current_parent_default_size() const
//		{
//			if (!!m_frame)
//				return m_frame->get_current_parent_default_size();
//			return frame::get_current_parent_default_size();
//		}
//
//		virtual range get_current_parent_range() const
//		{
//			if (!!m_frame)
//				return m_frame->get_current_parent_range();
//			return frame::get_current_parent_range();
//		}
//
//		virtual void calculate_range()
//		{
//			if (!!m_frame)
//				m_frame->calculate_range();
//			else
//				frame::calculate_range();
//		}
//
//		virtual double propose_dimension(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
//		{
//			if (!!m_frame)
//				return m_frame->propose_dimension(d, proposed, rtnOtherRange);
//			return frame::propose_dimension(d, proposed, rtnOtherRange);
//		}
//
//		virtual size propose_lengths(dimension d, const size& proposedSize) const
//		{
//			if (!!m_frame)
//				return m_frame->propose_lengths(d, proposedSize);
//			return frame::propose_lengths(d, proposedSize);
//		}
//
//		virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//		{
//			bounds newBounds;
//			rcptr<this_t> sortedList = m_sortedList;
//			if (!sortedList)
//				newBounds = b;
//			else
//			{
//				if (list_dimension == dimension::horizontal)
//					newBounds.set(point(m_offset, 0), size(m_cachedLength, b.get_height()));
//				else
//					newBounds.set(point(0, m_offset), size(b.get_width(), m_cachedLength));
//			}
//
//			if (!!m_frame)
//				m_frame->reshape(newBounds, oldOrigin);
//			else
//				frame::reshape(newBounds, oldOrigin);
//		}
//	};
//
//	//----Configuration data
//	map_t m_cells;
//	sizing_group_t m_sizingGroup;
//
//	//----State data
//	bool m_wasRecalculated;
//	double m_cachedLength;
//	double m_cachedWidth;
//
//	range m_currentRange;
//	size m_currentDefaultSize;
//
//	virtual range get_current_range() const { return m_currentRange; }
//	virtual size get_current_default_size() const { return m_currentDefaultSize; }
//
//	virtual void calculate_range()
//	{
//		range rng;
//		rng.set_fixed(0, 0);
//		size defaultSize(0, 0);
//
//		m_sizingGroup.clear();
//		typename map_t::iterator itor = m_cells.get_first();
//		while (!!itor)
//		{
//			cell_t& c = **itor;
//
//			range cellRange = c.get_current_parent_range();
//			size cellDefaultSize = c.get_current_parent_default_size();
//
//			c.m_range = cellRange[list_dimension];
//			c.m_default = cellDefaultSize[list_dimension];
//
//			// intersection of all ranges, in non-primary dimension
//			rng[!list_dimension] &= cellRange[!list_dimension];
//
//			// Combine mins in primary dimension
//			rng[list_dimension].get_min() += c.m_range.get_min();
//			if (rng[list_dimension].has_max())
//			{
//				if (c.m_range.has_max())
//					rng[list_dimension].set_max(c.m_range.get_max());
//				else
//					rng[list_dimension].clear_max();
//			}
//
//			defaultSize[list_dimension] += c.m_default;
//			if (defaultSize[!list_dimension] < cellDefaultSize[!list_dimension])
//				defaultSize[!list_dimension] = cellDefaultSize[!list_dimension];
//
//			m_sizingGroup.add_cell(*itor);
//			++itor;
//		}
//
//		m_currentDefaultSize = rng.limit(defaultSize);
//		m_currentRange = rng;
//		m_wasRecalculated = true;
//	}
//
//	virtual double propose_dimension(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
//	{
//		double rtn = proposed;
//		if (d == list_dimension)
//		{
//			m_sizingGroup.calculate_sizes(proposed);
//			rtn = m_sizingGroup.get_length();
//			m_cachedLength = rtn;
//
//			typename map_t::iterator itor = m_cells.get_first();
//			while (!!itor)
//			{
//				cell_t& cell = **itor;
//				range::linear_t otherRange;
//				double ignoredRtn = cell.propose_dimension(d, r2.get_length(), otherRange);
//#error
//				b.m_range &= otherRange;
//
//				++itor;
//			}
//
//			rtnOtherRange = m_secondarySizingGroup.get_range();
//		}
//		else
//		{
//			;
//		}
//		return rtn;
//	}
//
//	virtual size propose_lengths(dimension d, const size& proposedSize) const
//	{
//#error
//		size newSize = proposedSize;
//		range::linear_t otherRange;
//		newSize[list_dimension] = propose_dimension(list_dimension, proposedSize[list_dimension], otherRange);
//		if (proposedSize[!list_dimension] < otherRange.get_min())
//			newSize[!list_dimension] = otherRange.get_min();
//		else if (otherRange.has_max() && proposedSize[!list_dimension] > otherRange.get_max())
//			newSize[!list_dimension] = otherRange.get_max();
//		return newSize;
//	}
//
//	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//	{
//		double oldCachedLength = m_cachedLength;
//		size newSize(b.get_size());
//
//		if (m_wasRecalculated || (b.get_size(list_dimension) != oldCachedLength))
//		{
//			m_wasRecalculated = false;
//			newSize = propose_size(b.get_size());
//		}
//
//		m_secondarySizingGroup.calculate_sizes(newSize[!list_dimension]);
//
//		typename primary_map_t::iterator itor = m_primaryRows.get_first(); //m_rowsAndColumns[dimension::horizontal].get_first();
//		double curOffset = 0;
//		while (!!itor)
//		{
//			primary_row_t& r = **itor;
//			r.m_offset = curOffset;
//			r.m_cachedLength = r.get_length();
//			curOffset += r.m_cachedLength;
//			++itor;
//		}
//
//		typename secondary_map_t::iterator itor2 = m_secondaryRows.get_first(); //m_rowsAndColumns[dimension::vertical].get_first();
//		curOffset = 0;
//		while (!!itor2)
//		{
//			secondary_row_t& r = **itor2;
//			r.m_offset = curOffset;
//			r.m_cachedLength = r.get_length();
//			curOffset += r.m_cachedLength;
//			++itor2;
//		}
//
//		pane::reshape(r, oldOrigin);
//	}
//
//public:
//	grid()
//		: m_wasRecalculated(false)
//	{ }
//
//	~grid()
//	{ }
//
//	void nest(const rcref<pane>& child, const primary_key_t& primaryKey, const secondary_key_t& secondaryKey, const rcptr<frame>& userReshaper = 0)
//	{
//		rcref<cell_t> cell = rcnew(cell_t, child, userReshaper);
//		rcptr<primary_row_t> r;
//		typename primary_map_t::iterator itor = m_primaryRows.find_any_equal(primaryKey);
//		if (!!itor)
//			r = *itor;
//		else
//		{
//			rcref<primary_row_t> newRow = rcnew(primary_row_t);
//			newRow->m_removeToken = m_primaryRows.insert(primaryKey, newRow);
//			r = newRow;
//		}
//		cell->m_primaryRow = r;
//		cell->m_removeTokens[list_dimension] = r->m_cells.append(cell);
//
//		rcptr<secondary_row_t> r2;
//		typename secondary_map_t::iterator itor2 = m_secondaryRows.find_any_equal(secondaryKey);
//		if (!!itor2)
//			r2 = *itor2;
//		else
//		{
//			rcref<secondary_row_t> newRow = rcnew(secondary_row_t);
//			newRow->m_removeToken = m_secondaryRows.insert(secondaryKey, newRow);
//			r2 = newRow;
//		}
//		cell->m_secondaryRow = r2;
//		cell->m_removeTokens[!list_dimension] = r2->m_cells.append(cell);
//
//		pane::nest(child, cell);
//	}
//
//	virtual void detaching_child(const rcref<pane>& p)
//	{
//		rcptr<cell_t> c = p->get_frame().template static_cast_to<cell_t>();
//
//		rcptr<primary_row_t> primaryRow = c->m_primaryRow;
//		rcptr<secondary_row_t> secondaryRow = c->m_secondaryRow;
//
//		primaryRow->m_cells.remove(c->m_removeTokens[list_dimension]);
//		if (!primaryRow->m_cells) // remove row
//			m_primaryRows.remove(primaryRow->m_removeToken);
//
//		secondaryRow->m_cells.remove(c->m_removeTokens[!list_dimension]);
//		if (!secondaryRow->m_cells) // remove row
//			m_secondaryRows.remove(secondaryRow->m_removeToken);
//
//		pane::detaching_child(p);
//	}
//
//};
//
//
//}
//}
//
//
//#endif
