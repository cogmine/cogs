//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_GUI_WRAP_LIST
#define COGS_GUI_WRAP_LIST


#include "cogs/collections/container_dlist.hpp"
#include "cogs/gui/pane.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief A pane that displays child panes in a wrapping list
template <script_flow scriptFlow = script_flow::left_to_right_top_to_bottom, class primary_sizing_group_t = proportional_sizing_group, class secondary_sizing_group_t = primary_sizing_group_t>
class wrap_list : public pane, public virtual pane_container
{
private:
	double	m_verticalAlignment;
	double	m_horizontalAlignment;

	double& get_alignment(dimension d)
	{
		if (d == dimension::vertical)
			return m_verticalAlignment;
		return m_horizontalAlignment;
	}

	class cell_t;

	class row_t : public secondary_sizing_group_t::cell
	{
	public:
		using secondary_sizing_group_t::cell::m_default;
		using secondary_sizing_group_t::cell::m_range;

		container_dlist<rcref<cell_t> >	m_cells;
		bounds						m_bounds;
		primary_sizing_group_t		m_primarySizingGroup;
	
		row_t()
			:	m_bounds(point(0, 0), size(0, 0))
		{
			m_range.set_fixed(0);
		}
	};

	class cell_t : public override_bounds_frame, public primary_sizing_group_t::cell
	{
	public:
		using primary_sizing_group_t::cell::m_default;
		using primary_sizing_group_t::cell::m_range;

		weak_rcptr<row_t>	m_row;

		typename container_dlist<rcref<cell_t> >::remove_token			m_removeToken;

		cell_t(const rcref<canvas::cell>& f)
			: override_bounds_frame(f)
		{ }

		virtual void calculate_range()
		{
			dimension d = geometry::planar::get_primary_flow_dimension(scriptFlow);
			frame::calculate_range();
			m_default = frame::get_default_size()[d];
			m_range = frame::get_range()[d];
		}
	};
	
	container_dlist<rcref<cell_t> > m_cells;
	mutable container_dlist<rcref<row_t> > m_rows;
	mutable double m_cachedLength;
	mutable secondary_sizing_group_t m_secondarySizingGroup;
	range m_currentRange;
	size m_currentDefaultSize;

	virtual range get_range() const	{ return m_currentRange; }
	virtual size get_default_size() const	{ return m_currentDefaultSize; }

	virtual void calculate_range()
	{
		pane::calculate_range();

		// determines default size and range, but does not build either sizing group

		size defaultSize;
		m_currentDefaultSize.clear();
		m_currentRange.set_fixed_width(0);
		m_currentRange.set_fixed_height(0);
		
		dimension d = geometry::planar::get_primary_flow_dimension(scriptFlow);

		typename container_dlist<rcref<cell_t> >::iterator cellItor = m_cells.get_first();
		while (!!cellItor)
		{
			cell_t& c = **cellItor;

			range cellRange = c.get_range();
			size cellDefaultSize = cellRange.limit(c.get_default_size());	// necessary?

			// default size is one row, unwrapped
			m_currentDefaultSize[d] += cellDefaultSize[d];	// compute other dimension later
			if (m_currentDefaultSize[!d] < cellDefaultSize[!d])
				m_currentDefaultSize[!d] = cellDefaultSize[!d];

			double newMin;
			newMin = cellRange.get_min_width();
			if (m_currentRange.get_min_width() < newMin)
				m_currentRange.set_min_width(newMin);

			newMin = cellRange.get_min_height();
			if (m_currentRange.get_min_height() < newMin)
				m_currentRange.set_min_height(newMin);

			if (m_currentRange.has_max_width())
			{
				if (!cellRange.has_max_width())
					m_currentRange.clear_max_width();
				else
					m_currentRange.get_max_width() += cellRange.get_max_width();
			}
						
			if (m_currentRange.has_max_height())
			{
				if (!cellRange.has_max_height())
					m_currentRange.clear_max_height();
				else
					m_currentRange.get_max_height() += cellRange.get_max_height();
			}

			++cellItor;
		}

		// One last tweak.  If the preferred dimension is at minumum, compute the
		// range of the other dimension, and use it's max.
		if (m_currentRange[!d].has_max())
		{
			range::linear_t rtnOtherRange;
			propose_length(d, m_currentRange[d].get_min(), rtnOtherRange);
			COGS_ASSERT(!!rtnOtherRange.has_max());
			m_currentRange.set_max(!d, rtnOtherRange.get_max());
		}
		
		range::linear_t rtnOtherRange;
		propose_length(d, m_currentDefaultSize[d], rtnOtherRange);	// sets m_cachedLength
		if (rtnOtherRange.has_max() && (m_currentDefaultSize[!d] > rtnOtherRange.get_max()))
			m_currentDefaultSize[!d] = rtnOtherRange.get_max();
		if (m_currentDefaultSize[!d] < rtnOtherRange.get_min())
			m_currentDefaultSize[!d] = rtnOtherRange.get_min();
	}

	virtual dimension get_primary_flow_dimension() const { return geometry::planar::get_primary_flow_dimension(scriptFlow); }

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double rtn = proposed;// = pane::propose_length(d, proposed, rtnOtherRange);

		if (d == geometry::planar::get_primary_flow_dimension(scriptFlow))
		{
			m_cachedLength = rtn;
			m_secondarySizingGroup.clear();
			m_rows.clear();

			// Start building a row using default lengths, until we exceed the proposed width, then wrap.
			typename container_dlist<rcref<cell_t> >::iterator lastCellItor = m_cells.get_last();
			typename container_dlist<rcref<cell_t> >::iterator cellItor = m_cells.get_first();
			if (!!cellItor)
			{
				rcref<row_t> row = rcnew(row_t);	// start a new row
				
				double remaining = rtn;
				double largestRowLength = 0;

				rcref<cell_t> c = *cellItor;
				size cellSize = c->get_default_size();
	
				for (;;)
				{
					bool insufficientSpace = (cellSize[d] > remaining);
					if (insufficientSpace)
					{
						if (row->m_cells.is_empty())	// If nothing in the list, when need to shrink this one into place
						{								// We shouldn't be asked to size smaller than min, as we've already established our mins.
							cellSize[d] = remaining;
							insufficientSpace = false;
						}
					}

					bool cellsDone = false;
					if (!insufficientSpace)
					{
						if (row->m_default < cellSize[!d])
							row->m_default = cellSize[!d];

						remaining -= cellSize[d];

						row->m_cells.append(*cellItor);
						row->m_primarySizingGroup.add_cell(*cellItor);

						c->m_row = row;

						++cellItor;
						cellsDone = !cellItor;
						if (!cellsDone)
						{
							c = *cellItor;
							cellSize = c->get_default_size();
						}
					}

					bool rowDone = insufficientSpace || cellsDone;
					if (rowDone)	// Need to start a new row.
					{
						row->m_primarySizingGroup.calculate_sizes(rtn);

						double rowLength = row->m_primarySizingGroup.get_length();
						row->m_bounds.get_size(d) = rowLength;

						if (largestRowLength < rowLength)
							largestRowLength = rowLength;

						m_rows.append(row);

						if (cellsDone)
							break;

						row = rcnew(row_t);
						remaining = rtn;
					}
					// continue;
				}

				// Reduce rtn to the longest detected
				rtn = largestRowLength;

				range::linear_t otherRange;
					
				cellItor = m_cells.get_first();
				while (!!cellItor)
				{
					cell_t& c = **cellItor;
					rcptr<row_t> row = c.m_row;

					c.propose_length(d, c.get_length(), otherRange);	// return value is ignored

					row->m_range ^= otherRange;

					++cellItor;
				}

				typename container_dlist<rcref<row_t> >::iterator rowItor = m_rows.get_first();
				while (!!rowItor)
				{
					m_secondarySizingGroup.add_cell(*rowItor);
					++rowItor;
				}

				rtnOtherRange = m_secondarySizingGroup.get_range();
			}
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
		invalidate(r.get_size());

		dimension d = geometry::planar::get_primary_flow_dimension(scriptFlow);

		double oldCachedLength = m_cachedLength;
		size newSize(r.get_size());

		if (r.get_size(d) != oldCachedLength)
			newSize = propose_size(r.get_size());

		m_secondarySizingGroup.calculate_sizes(newSize[!d]);

		bool reverseDirection[2];
		if (d == dimension::horizontal)
		{
			reverseDirection[(int) d] = !((int)scriptFlow & (int)script_flow::horizontal_ascending_mask);
			reverseDirection[(int)!d] = !((int)scriptFlow & (int)script_flow::vertical_ascending_mask);
		}
		else
		{
			reverseDirection[(int)d] = !((int)scriptFlow & (int)script_flow::vertical_ascending_mask);
			reverseDirection[(int)!d] = !((int)scriptFlow & (int)script_flow::horizontal_ascending_mask);
		}

		double rowOffset = (newSize[!d] > m_secondarySizingGroup.get_length())	? (get_alignment(!d) * (newSize[!d] - m_secondarySizingGroup.get_length()))
																				:	0;
		if (reverseDirection[(int)!d])
		{
			double oldRowOffset = rowOffset;
			rowOffset = newSize[!d];
			rowOffset -= oldRowOffset;
		}

		typename container_dlist<rcref<row_t> >::iterator rowItor = m_rows.get_first();
		while (!!rowItor)
		{
			rcref<row_t> rowRef = *rowItor;
			row_t& row = *rowRef;

			if (reverseDirection[(int)!d])
				rowOffset -= row.get_length();;

			double cellOffset =
				row.m_bounds.get_position(d) = (newSize[d] > row.m_primarySizingGroup.get_length())	?	(get_alignment(d) * (newSize[d] - row.m_primarySizingGroup.get_length()))
																										:	0;
			row.m_bounds.get_position(!d) = rowOffset;
			row.m_bounds.get_size(!d) = row.m_primarySizingGroup.get_length();
			
			if (reverseDirection[(int)d])
			{
				double oldCellOffset = cellOffset;
				cellOffset = newSize[d];
				cellOffset -= oldCellOffset;
			}

			typename container_dlist<rcref<cell_t> >::iterator cellItor = row.m_cells.get_first();
			while (!!cellItor)
			{
				cell_t& c = **cellItor;

				if (reverseDirection[(int)d])
					cellOffset -= c.get_length();
				
				c.get_position(d) = cellOffset;
				c.get_position(!d) = rowOffset;

				c.get_fixed_size(d) = c.get_length();
				c.get_fixed_size(!d) = row.get_length();

				if (!reverseDirection[(int)d])
					cellOffset += c.get_length();
				++cellItor;
			}

			if (!reverseDirection[(int)!d])
				rowOffset += row.get_length();
			++rowItor;
		}

		pane::reshape(r, oldOrigin);
	}

	virtual void detaching_child(const rcref<pane>& p)
	{
		rcptr<cell_t> c = p->get_outermost_frame().static_cast_to<cell_t>();
		m_cells.remove(c->m_removeToken);
		pane::detaching_child(p);
	}


public:
	class nest_token
	{
	protected:
		friend class wrap_list;
		typename container_dlist<rcref<cell_t> >::remove_token	m_removeToken;
		weak_rcptr<pane> m_pane;
	};

	wrap_list()
	{
		m_verticalAlignment = 0;
		m_horizontalAlignment = 0;
	}

	wrap_list(double primaryAlignment, double secondaryAlignment)
	{
		dimension d = geometry::planar::get_primary_flow_dimension(scriptFlow);
		get_alignment(d) = primaryAlignment;
		get_alignment(!d) = secondaryAlignment;
	}

	wrap_list(double primaryAlignment)
	{
		dimension d = geometry::planar::get_primary_flow_dimension(scriptFlow);
		get_alignment(d) = primaryAlignment;
		get_alignment(!d) = 0;
	}

	using pane::nest;

	using pane::get_pane_container;
	using pane::get_pane_container_ref;
	
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		rcptr<canvas::cell> c2 = f;
		if (!c2)
			c2 = child;
		rcref<cell_t> c = rcnew(cell_t, c2.dereference());
		c->m_removeToken = m_cells.prepend(c);
		pane::nest_first(child, c);
	}

	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		rcptr<canvas::cell> c2 = f;
		if (!c2)
			c2 = child;
		rcref<cell_t> c = rcnew(cell_t, c2.dereference());
		c->m_removeToken = m_cells.append(c);
		pane::nest_last(child, c);
	}

	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0)
	{
		rcptr<canvas::cell> c2 = f;
		if (!c2)
			c2 = child;
		rcref<cell_t> c = rcnew(cell_t, c2.dereference());
		rcptr<cell_t> beforeThisCell = beforeThis->get_outermost_frame().static_cast_to<cell_t>();
		m_cells.insert_before(c, beforeThisCell->m_removeToken);
		pane::nest_before(child, beforeThis, c);
	}

	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0)
	{
		rcptr<canvas::cell> c2 = f;
		if (!c2)
			c2 = child;
		rcref<cell_t> c = rcnew(cell_t, c2.dereference());
		rcptr<cell_t> afterThisCell = afterThis->get_outermost_frame().static_cast_to<cell_t>();
		c->m_removeToken = m_cells.insert_after(c, afterThisCell->m_removeToken);
		pane::nest_after(child, afterThis, c);
	}

	void clear()
	{
		detach_children();
	}
};


}
}


#endif 
