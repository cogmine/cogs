////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_GUI_LABELED_LIST
//#define COGS_HEADER_GUI_LABELED_LIST
//
//
//#include "cogs/collections/multimap.hpp"
//#include "cogs/gui/pane.hpp"
//#include "cogs/gui/label.hpp"
//#include "cogs/gui/grid.hpp"
//
//
//namespace cogs {
//namespace gui {
//
//
//// A labeled_list is a top-down list with (usually) 2 columns.  The first column contains a label, the second contains a content pane.
//// If a label/content pair is too wide for a single row, it may be be wrapped to 2 rows.
//template <bool repositionable_offset = false, bool allow_wrap = true, bool scriptFlowsToTheRight = true>
//class labeled_list : public pane
//{
//private:
//	typedef grid<dimension::horizontal, fair_sizing_group_t> grid_t;
//
//	class label_cell_t;
//	class content_cell_t;
//	class cell_t;
//
//	nonvolatile_multimap<double, rcref<cell_t>, true> m_sortedByLabelWidth; // computed offset is default width of last label in the list
//
//	bool m_useManualOffset; // overrides label width
//	double m_manualOffset;
//	grid_t m_grid;
//
//	class label_cell_t : public frame
//	{
//	public:
//		bounds m_bounds;
//
//		virtual void reshape(const bounds&, const point& oldOrigin = point(0, 0))
//		{
//			frame::reshape(m_bounds, oldOrigin);
//		}
//	};
//
//	class content_cell_t : public frame
//	{
//	public:
//		bounds m_bounds;
//
//		virtual void reshape(const bounds&, const point& oldOrigin = point(0, 0))
//		{
//			frame::reshape(m_bounds, oldOrigin);
//		}
//	};
//
//	class cell_t
//	{
//	public:
//		typename nonvolatile_multimap<size_t, rcref<cell_t>, true>::iterator m_sortedByLabelWidthRemoveToken;
//
//		weak_rcptr<labeled_list> m_labeledList;
//		rcref<label_cell_t> m_label;
//		rcref<content_cell_t> m_content;
//		range m_range;
//		size m_defaultSize;
//
//		cell_t(const rcref<labeled_list>& labaledList, const rcref<label_cell_t>& lbl, const rcref<content_cell_t>& cntnt)
//			: m_labeledList(labaledList),
//			m_label(lbl),
//			m_content(cntnt)
//		{ }
//
//		virtual range get_current_range() const { return m_range; }
//		virtual size get_current_default_size() const { return m_defaultSize; }
//
//		virtual void calculate_range()
//		{
//			rcptr<labeled_list>	labeledList = m_labeledList;
//			if (!!labeledList)
//			{
//				// default size is side-by-side on the same row
//				size m_defaultSize = m_label->get_current_default_size();
//
//				if (!!m_sortedByLabelWidthRemoveToken)
//					m_sortedByLabelWidth.remove(m_sortedByLabelWidthRemoveToken); // refresh sorted list
//				m_sortedByLabelWidthRemoveToken = m_sortedByLabelWidth.insert(m_defaultSize.get_width(), this_rcref);
//				if (!!m_useManualOffset)
//				{
//					if (m_defaultSize.get_width() > m_manualOffset)
//						m_defaultSize.get_width() = m_manualOffset;
//				}
//				m_defaultSize.get_width() += contentSize.get_width();
//				if (m_defaultSize.get_height() > contentSize.get_height())
//					m_defaultSize.set_height(labelSize.get_height());
//				else
//					m_defaultSize.set_height(contentSize.get_height());
//
//				// get the greater mins and sum maxes.
//				range labelRange = m_label->get_current_parent_range();
//				range contentRange = m_content->get_current_parent_range();
//
//				if (labelRange.get_min_height() > contentRange.get_min_height())
//					m_range.set_min_height(labelRange.get_min_height());
//				else
//					m_range.set_min_height(contentRange.get_min_height());
//
//				if (labelRange.get_min_width() > contentRange.get_min_width())
//					m_range.set_min_width(labelRange.get_min_width());
//				else
//					m_range.set_min_width(contentRange.get_min_width());
//
//				if (!labelRange.has_max_height() || !contentRange.has_max_height())
//					m_range.clear_max_height();
//				else
//				{
//					double newMaxHeight = labelRange.get_max_height();
//					newMaxHeight += contentRange.get_max_height();
//					m_range.set_max_height(newMaxHeight);
//				}
//				if (!labelRange.has_max_width() || !contentRange.has_max_width())
//					m_range.clear_max_width();
//				else
//				{
//					double newMaxWidth = labelRange.get_max_width();
//					newMaxWidth += contentRange.get_max_width();
//					m_range.set_max_width(newMaxWidth);
//				}
//			}
//		}
//
//		virtual double propose_dimension(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
//		{
//			rtnOtherRange.clear();
//			double rtn = proposed;
//			if (d == dimension::horizontal)
//			{
//				range labelRange = m_label->get_current_parent_range();
//				range contentRange = m_content->get_current_parent_range();
//
//				double minWidth = labelRange.get_min_width();
//				minWidth += contentRange.get_min_width();
//				if (rtn < minWidth) // needs to be 2 lines.
//				{
//					if (labelRange.has_max_width() && (rtn > labelRange.get_max_width()))
//						rtn = labelRange.get_max_width();
//					if (labelRange.has_max_width() && (rtn > contentRange.get_max_width()))
//						rtn = contentRange.get_max_width();
//
//					if (rtn < labelRange.get_min_width())
//						rtn = labelRange.get_min_width();
//					if (rtn < contentRange.get_min_width())
//						rtn = contentRange.get_min_width();
//
//					range::linear_t otherLabelRange;
//					range::linear_t otherContentRange;
//					m_label->propose_dimension(dimension::horizontal, rtn, otherLabelRange); // return value is ignored
//					m_content->propose_dimension(dimension::horizontal, rtn, otherContentRange); // return value is ignored
//
//					if (!otherLabelRange.has_max() || !otherContentRange.has_max())
//						rtnOtherRange.clear_max();
//					else
//					{
//						rtnOtherRange.set_max(otherLabelRange.get_max());
//						rtnOtherRange.get_max() += otherContentRange.get_max();
//					}
//					rtnOtherRange.set_min(otherLabelRange.get_min());
//					rtnOtherRange.get_min() += otherContentRange.get_min();
//				}
//				else	// both on 1 line
//				{
//					if (labelRange.has_max_width() && contentRange.has_max_width())
//					{
//						double maxWidth = labelRange.get_max_width();
//						maxWidth += contentRange.get_max_width();
//						if (rtn > maxWidth)
//							rtn = maxWidth;
//
//						range::linear_t otherLabelRange;
//						range::linear_t otherContentRange;
//						double newLabelWidth = rtn;
//						newLabelWidth /= 2;
//						double labelWidthUsed = m_label->propose_dimension(dimension::horizontal, newLabelWidth, otherLabelRange);
//						double newContentWidth = rtn;
//						newContentWidth -= labelWidthUsed;
//						double contentWidthUsed = m_content->propose_dimension(dimension::horizontal, newContentWidth, otherContentRange);
//
//						if (!otherLabelRange.has_max() || !otherContentRange.has_max())
//							rtnOtherRange.clear_max();
//						else
//						{
//							if (otherLabelRange.get_max() > otherContentRange.get_max())
//								rtnOtherRange.set_max(otherLabelRange.get_max());
//							else
//								rtnOtherRange.set_max(otherContentRange.get_max());
//						}
//					}
//				}
//			}
//			return rtn;
//		}
//
//		virtual size propose_lengths(dimension d, const size& proposedSize) const
//		{
//			//#error use d?
//			//dimension d = dimension::horizontal; //geometry::planar::get_primary_flow_dimension(scriptFlow);
//			range::linear_t otherRange;
//			size newSize = proposedSize;
//			newSize[d] = propose_dimension(d, proposedSize[d], otherRange);
//
//			if (proposedSize[!d] < otherRange.get_min())
//				newSize[!d] = otherRange.get_min();
//			else if (otherRange.has_max() && proposedSize[!d] > otherRange.get_max())
//				newSize[!d] = otherRange.get_max();
//			return newSize;
//		}
//
//		virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//		{
//			dimension d = dimension::horizontal; // Compute aligned locations of label and content
//			range labelRange = m_label->get_current_parent_range();
//			range contentRange = m_content->get_current_parent_range();
//
//			m_label->m_bounds.set_position(point(0, 0));
//			m_content->m_bounds.set_position(point(0, 0));
//
//			double minWidth = labelRange.get_min_width();
//			minWidth += contentRange.get_min_width();
//			if (b.get_width() < minWidth) // needs to be 2 lines.
//			{
//				range::linear_t labelHeightRange; // size header first, width first
//				range::linear_t contentHeightRange;
//				double newLabelWidth = m_label->propose_dimension(d, b.get_size(d), labelHeightRange);
//				double newContentWidth = m_content->propose_dimension(d, b.get_size(d), contentHeightRange);
//
//				m_label->m_bounds.get_size().get_width() = newLabelWidth;
//				m_content->m_bounds.get_size().get_width() = newContentWidth;
//
//				if (!scriptFlowsToTheRight) // Align to right on left-right script languages.
//				{
//					m_label->m_bounds.get_position().get_x() = b.get_size().get_width();
//					m_content->m_bounds.get_position().get_x() = b.get_size().get_width();
//					m_label->m_bounds.get_position().get_x() -= newLabelWidth;
//					m_content->m_bounds.get_position().get_x() -= newContentWidth;
//				}
//
//				double newLabelHeight = m_label->get_current_parent_default_size()[!d];
//				if (newLabelHeight < labelHeightRange.get_min())
//					newLabelHeight = labelHeightRange.get_min();
//				else if (labelHeightRange.has_max() && (newLabelHeight > labelHeightRange.get_max()))
//					newLabelHeight = labelHeightRange.get_max();
//
//				double newContentHeight = m_content->get_current_parent_default_size()[!d];
//				if (newContentHeight < labelContentRange.get_min())
//					newContentHeight = labelContentRange.get_min();
//				else if (labelContentRange.has_max() && (newContentHeight > labelContentRange.get_max()))
//					newContentHeight = labelContentRange.get_max();
//
//				// If not enough space, tweak it by taking some from the label, then the content.
//				double usedHeight = newLabelHeight;
//				usedHeight += newContentHeight;
//				if (usedHeight > b.get_height())
//				{
//					double overage = usedHeight;
//					overage -= b.get_height();
//					newLabelHeight -= overage;
//					if (newLabelHeight < labelHeightRange.get_min())
//						newLabelHeight = labelHeightRange.get_min();
//				}
//				else if (usedHeight < b.get_height()) // If too much space, allocate evenly until max, then give to the other until max.
//				{
//					double remaining = b.get_height();
//					remaining -= usedHeight;
//
//					double halfRoundedDown = remaining;
//					double halfRoundedUp = remaining;
//					halfRoundedDown /= 2;
//					halfRoundedUp -= halfRoundedDown;
//
//					newLabelHeight += halfRoundedUp;
//					if ((labelHeightRange.has_max()) && (newLabelHeight > labelHeightRange.get_max()))
//						newLabelHeight = labelHeightRange.get_max();
//				}
//				newContentHeight = b.get_size().get_height();
//				newContentHeight -= newLabelHeight;
//					if ((contentHeightRange.has_max()) && (newContentHeight > contentHeightRange.get_max()))
//						newContentHeight = contentHeightRange.get_max();
//
//				m_label->m_bounds.get_size().get_height() = newLabelHeight;
//				m_content->m_bounds.get_position().get_y() += newLabelHeight;
//				m_content->m_bounds.get_size().get_height() = newContentHeight;
//			}
//			else	// both fit on 1 line
//			{
//				rcptr<labeled_list>	labeledList = m_labeledList;
//				if (!!labeledList)
//				{
//					m_label->m_bounds.get_size().get_height() = b.get_height();
//					m_content->m_bounds.get_size().get_height() = b.get_height();
//
//					double maxLabelWidth = labeledList->m_sortedByLabelWidth.get_last().get_key();
//					double columnWidth;
//					if ((!m_useManualOffset) || (maxLabelWidth < m_manualOffset))
//						columnWidth = maxLabelWidth;
//					else
//						columnWidth = m_manualOffset;
//					double boundsMinusColumnWidth = b.get_width();
//					boundsMinusColumnWidth -= columnWidth;
//
//					range::linear_t labelHeightRange; // size header first, width first
//					range::linear_t contentHeightRange;
//					double newLabelWidth = m_label->propose_dimension(d, columnWidth, labelHeightRange);
//					double newContentWidth = m_content->propose_dimension(d, boundsMinusColumnWidth, contentHeightRange);
//					double newLabelHeight = m_label->get_current_parent_default_size()[!d];
//					double newContentHeight = m_content->get_current_parent_default_size()[!d];
//
//					if (newLabelHeight < labelHeightRange.get_min())
//						newLabelHeight = labelHeightRange.get_min();
//					else if (labelHeightRange.has_max() && (newLabelHeight > labelHeightRange.get_max()))
//						newLabelHeight = labelHeightRange.get_max();
//
//					if (newContentHeight < labelContentRange.get_min())
//						newContentHeight = labelContentRange.get_min();
//					else if (labelContentRange.has_max() && (newContentHeight > labelContentRange.get_max()))
//						newContentHeight = labelContentRange.get_max();
//
//					m_label->m_bounds.get_size().get_width() = newLabelWidth;
//					m_label->m_bounds.get_size().get_height() = newLabelHeight;
//					m_content->m_bounds.get_size().get_width() = newContentWidth;
//					m_content->m_bounds.get_size().get_height() = newContentHeight;
//
//					if (!!scriptFlowsToTheRight)
//					{
//						m_content->m_bounds.get_position().get_x() = columnWidth;
//						double labelStart = columnWidth;
//						labelStart -= newLabelWidth;
//						m_label->get_position().get_x() = labelStart;
//					}
//					else
//					{
//						m_label->m_bounds.get_position().get_x() = boundsMinusColumnWidth;
//						double columnStart = boundsMinusColumnWidth;
//						columnStart -= newContentWidth;
//						m_column->get_position().get_x() = columnStart;
//					}
//
//					if (newLabelHeight < b.get_height())
//					{
//						double dif = b.get_height();
//						dif -= newLabelHeight;
//						dif /= 2;
//						m_label->m_bounds.get_position().get_y() += dif;
//					}
//
//					if (newContentHeight < b.get_height())
//					{
//						double dif = b.get_height();
//						dif -= newContentHeight;
//						dif /= 2;
//						m_label->m_bounds.get_position().get_y() += dif;
//					}
//				}
//			}
//		}
//	};
//
//	labeled_list()
//		: m_grid(rcnew(grid_t)),
//		m_useManualOffset(false)
//	{
//		pane::nest(m_grid);
//	}
//
//	virtual void nest_last(const rcref<pane>& child);
//	virtual void nest_first(const rcref<pane>& child);
//	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis);
//	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis);
//
//	/*
//	collection<rcref<cell_t> > m_cells;
//	collection<rcref<row_t> > m_rows;
//	double m_cachedLength;
//	secondary_sizing_group_t m_secondarySizingGroup;
//	range m_currentRange;
//	size m_currentDefaultSize;
//
//	virtual range get_current_range() const { return m_currentRange; }
//	virtual size get_current_default_size() const { return m_currentDefaultSize; }
//
//	virtual void calculate_range()
//	{
//		// determines default size and range, but does not build either sizing group
//
//		size defaultSize;
//		m_currentDefaultSize.clear();
//		m_currentRange.set_fixed_width(0);
//		m_currentRange.set_fixed_height(0);
//
//		int d = geometry::planar::get_primary_flow_dimension(scriptFlow);
//
//		typename collection<rcref<cell_t> >::iterator cellItor = m_cells.get_first();
//		while (!!cellItor)
//		{
//			cell_t& cell = **cellItor;
//
//			range cellRange = cell.get_current_parent_range();
//			size cellDefaultSize = cellRange.limit(cell.get_current_parent_default_size()); // necessary?
//
//			// default size is one row, unwrapped
//			m_currentDefaultSize[d] += cellDefaultSize[d]; // compute other dimension later
//			if (m_currentDefaultSize[!d] < cellDefaultSize[!d])
//				m_currentDefaultSize[!d] = cellDefaultSize[!d];
//
//			double newMin;
//			newMin = cellRange.get_min_width();
//			if (m_currentRange.get_min_width() < newMin)
//				m_currentRange.set_min_width(newMin);
//
//			newMin = cellRange.get_min_height();
//			if (m_currentRange.get_min_height() < newMin)
//				m_currentRange.set_min_height(newMin);
//
//			if (m_currentRange.has_max_width())
//			{
//				if (!cellRange.has_max_width())
//					m_currentRange.clear_max_width();
//				else
//					m_currentRange.get_max_width() += cellRange.get_max_width();
//			}
//
//			if (m_currentRange.has_max_height())
//			{
//				if (!cellRange.has_max_height())
//					m_currentRange.clear_max_height();
//				else
//					m_currentRange.get_max_height() += cellRange.get_max_height();
//			}
//
//			++cellItor;
//		}
//
//		// One last tweak.  If the preferred dimension is at minumum, compute the
//		// range of the other dimension, and use it's max.
//		if (m_currentRange[!d].has_max())
//		{
//			range::linear_t rtnOtherRange;
//			propose_dimension(d, m_currentRange[d].get_min(), rtnOtherRange);
//			COGS_ASSERT(!!rtnOtherRange.has_max());
//			m_currentRange.set_max(!d, rtnOtherRange.get_max());
//		}
//
//		range::linear_t rtnOtherRange;
//		propose_dimension(d, m_currentDefaultSize[d], rtnOtherRange); // sets m_cachedLength
//		if (rtnOtherRange.has_max() && (m_currentDefaultSize[!d] > rtnOtherRange.get_max()))
//			m_currentDefaultSize[!d] = rtnOtherRange.get_max();
//		if (m_currentDefaultSize[!d] < rtnOtherRange.get_min())
//			m_currentDefaultSize[!d] = rtnOtherRange.get_min();
//	}
//
//	virtual double propose_dimension(planar::dimension d, double proposed, range::linear_t& rtnOtherRange) const
//	{
//		double rtn = proposed;// = pane::propose_dimension(d, proposed, rtnOtherRange);
//
//		if (d == geometry::planar::get_primary_flow_dimension(scriptFlow))
//		{
//			m_cachedLength = rtn;
//			m_secondarySizingGroup.clear();
//			m_rows.clear();
//
//			// Start building a row using default lengths, until we exceed the proposed width, then wrap.
//			typename collection<rcref<cell_t> >::iterator lastCellItor = m_cells.get_last();
//			typename collection<rcref<cell_t> >::iterator cellItor = m_cells.get_first();
//			if (!!cellItor)
//			{
//				rcref<row_t> row = rcnew(row_t); // start a new row
//
//				double remaining = rtn;
//				double largestRowLength;
//				largestRowLength.clear();
//
//				rcref<cell_t> cell = *cellItor;
//				size cellSize = cell->get_current_parent_default_size();
//
//				for (;;)
//				{
//					bool insufficientSpace = (cellSize[d] > remaining);
//					if (insufficientSpace)
//					{
//						if (row->m_cells.empty()) // If nothing in the list, when need to shrink this one into place
//						{ // We shouldn't be asked to size smaller than min, as we've already established our mins.
//							cellSize[d] = remaining;
//							insufficientSpace = false;
//						}
//					}
//
//					bool cellsDone = false;
//					if (!insufficientSpace)
//					{
//						if (row->m_default < cellSize[!d])
//							row->m_default = cellSize[!d];
//
//						remaining -= cellSize[d];
//
//						row->m_cells.append(*cellItor);
//						row->m_primarySizingGroup.add_cell(*cellItor);
//
//						cell->m_row = row;
//
//						++cellItor;
//						cellsDone = !cellItor;
//						if (!cellsDone)
//						{
//							cell = *cellItor;
//							cellSize = cell->get_current_parent_default_size();
//						}
//					}
//
//					bool rowDone = insufficientSpace || cellsDone;
//					if (rowDone) // Need to start a new row.
//					{
//						row->m_primarySizingGroup.calculate_sizes(rtn);
//
//						double rowLength = row->m_primarySizingGroup.get_length();
//						row->m_bounds.get_size(d) = rowLength;
//
//						if (largestRowLength < rowLength)
//							largestRowLength = rowLength;
//
//						m_rows.append(row);
//
//						if (cellsDone)
//							break;
//
//						row = rcnew(row_t);
//						remaining = rtn;
//					}
//					// continue;
//				}
//
//				// Reduce rtn to the longest detected
//				rtn = largestRowLength;
//
//				double ignoredRtn;
//				range::linear_t otherRange;
//
//				cellItor = m_cells.get_first();
//				while (!!cellItor)
//				{
//					cell_t& cell = **cellItor;
//					rcptr<row_t> row = cell.m_row;
//
//
//					cell.propose_dimension(d, cell.get_length(), otherRange); // return value is ignored
//
//					row->m_range ^= otherRange;
//
//					++cellItor;
//				}
//
//				typename collection<rcref<row_t> >::iterator rowItor = m_rows.get_first();
//				while (!!rowItor)
//				{
//					m_secondarySizingGroup.add_cell(*rowItor);
//					++rowItor;
//				}
//
//				rtnOtherRange = m_secondarySizingGroup.get_range();
//			}
//		}
//		return rtn;
//	}
//
//	virtual size propose_lengths(dimension d, const size& proposedSize) const
//	{
//		//int d = geometry::planar::get_primary_flow_dimension(scriptFlow);
//		range::linear_t otherRange;
//		size newSize = proposedSize;
//		newSize[d] = propose_dimension(d, proposedSize[d], otherRange);
//		if (proposedSize[!d] < otherRange.get_min())
//			newSize[!d] = otherRange.get_min();
//		else if (otherRange.has_max() && proposedSize[!d] > otherRange.get_max())
//			newSize[!d] = otherRange.get_max();
//		return newSize;
//	}
//
//	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//	{
//		int d = geometry::planar::get_primary_flow_dimension(scriptFlow);
//
//		double oldCachedLength = m_cachedLength;
//		size newSize(b.get_size());
//
//		if (b.get_size(d) != oldCachedLength)
//			newSize = propose_size(b.get_size());
//
//		m_secondarySizingGroup.calculate_sizes(newSize[!d]);
//
//		bool reverseDirection[2];
//		if (d == dimension::horizontal)
//		{
//			reverseDirection[d] = !(scriptFlow & script_flow::horizontal_ascending_mask);
//			reverseDirection[!d] = !(scriptFlow & script_flow::vertical_ascending_mask);
//		}
//		else
//		{
//			reverseDirection[d] = !(scriptFlow & script_flow::vertical_ascending_mask);
//			reverseDirection[!d] = !(scriptFlow & script_flow::horizontal_ascending_mask);
//		}
//
//		double rowOffset =
//			(newSize[!d] > m_secondarySizingGroup.get_length())
//				? (m_alignment[!d].align(newSize[!d] - m_secondarySizingGroup.get_length()))
//				: 0;
//		if (reverseDirection[!d])
//		{
//			double oldRowOffset = rowOffset;
//			rowOffset = newSize[!d];
//			rowOffset -= oldRowOffset;
//		}
//
//		typename collection<rcref<row_t> >::iterator rowItor = m_rows.get_first();
//		while (!!rowItor)
//		{
//			rcref<row_t> rowRef = *rowItor;
//			row_t& row = *rowRef;
//
//			if (reverseDirection[!d])
//				rowOffset -= row.get_length();
//
//			double cellOffset =
//				row.m_bounds.get_position(d) = (newSize[d] > row.m_primarySizingGroup.get_length())
//					? (m_alignment[d].align(newSize[d] - row.m_primarySizingGroup.get_length()))
//					: 0;
//			row.m_bounds.get_position(!d) = rowOffset;
//			row.m_bounds.get_size(!d) = row.m_primarySizingGroup.get_length();
//
//			if (reverseDirection[d])
//			{
//				double oldCellOffset = cellOffset;
//				cellOffset = newSize[d];
//				cellOffset -= oldCellOffset;
//			}
//
//			typename collection<rcref<cell_t> >::iterator cellItor = row.m_cells.get_first();
//			while (!!cellItor)
//			{
//				cell_t& cell = **cellItor;
//
//				if (reverseDirection[d])
//					cellOffset -= cell.get_length();
//
//				cell.m_bounds.get_position(d) = cellOffset;
//				cell.m_bounds.get_position(!d) = rowOffset;
//
//				cell.m_bounds.get_size(d) = cell.get_length();
//				cell.m_bounds.get_size(!d) = row.get_length();
//
//				if (!reverseDirection[d])
//					cellOffset += cell.get_length();
//				++cellItor;
//			}
//
//			if (!reverseDirection[!d])
//				rowOffset += row.get_length();
//			++rowItor;
//		}
//
//		pane::reshape(b, oldOrigin);
//	}
//
//public:
//	class nest_token
//	{
//	protected:
//		friend class wrap_list;
//		typename collection<rcref<cell_t> >::remove_token m_removeToken;
//	};
//
//	wrap_list()
//	{
//		m_alignment[dimension::vertical] = linear::alignment::minimum();
//		m_alignment[dimension::horizontal] = linear::alignment::minimum();
//	}
//
//	wrap_list(linear::alignment primaryAlignment)
//	{
//		int d = geometry::planar::get_primary_flow_dimension(scriptFlow);
//		m_alignment[d] = primaryAlignment;
//		m_alignment[!d] = linear::alignment::minimum();
//	}
//
//	wrap_list(linear::alignment primaryAlignment, linear::alignment secondaryAlignment)
//	{
//		int d = geometry::planar::get_primary_flow_dimension(scriptFlow);
//		m_alignment[d] = primaryAlignment;
//		m_alignment[!d] = secondaryAlignment;
//	}
//
//	nest_token nest_before(const rcref<pane>& child, const nest_token& beforeThis, const rcptr<frame>& userReshaper = 0)
//	{
//		nest_token nt;
//		rcref<cell_t> cell = rcnew(cell_t)(child, userReshaper);
//		nt.m_removeToken = cell->m_removeToken = m_cells.insert_before(cell, beforeThis.m_removeToken);
//		pane::nest(child, cell);
//		return nt;
//	}
//
//	nest_token nest_after(const rcref<pane>& child, const nest_token& afterThis, const rcptr<frame>& userReshaper = 0)
//	{
//		nest_token nt;
//		rcref<cell_t> cell = rcnew(cell_t)(child, userReshaper);
//		nt.m_removeToken = cell->m_removeToken = m_cells.insert_after(cell, afterThis.m_removeToken);
//		pane::nest(child, cell);
//		return nt;
//	}
//
//	nest_token nest_first(const rcref<pane>& child, const rcptr<frame>& userReshaper = 0)
//	{
//		nest_token nt;
//		rcref<cell_t> cell = rcnew(cell_t)(child, userReshaper);
//		nt.m_removeToken = cell->m_removeToken = m_cells.prepend(cell);
//		pane::nest(child, cell);
//		return nt;
//	}
//
//	nest_token nest_last(const rcref<pane>& child, const rcptr<frame>& userReshaper = 0)
//	{
//		nest_token nt;
//		rcref<cell_t> cell = rcnew(cell_t)(child, userReshaper);
//		nt.m_removeToken = cell->m_removeToken = m_cells.append(cell);
//		pane::nest(child, cell);
//		return nt;
//	}
//
//	nest_token nest(const rcref<pane>& child, const rcptr<frame>& userReshaper = 0)
//	{
//		return nest_last(child, userReshaper);
//	}
//
//	virtual void detaching_child(const rcref<pane>& p)
//	{
//		rcptr<cell_t> cell = p->get_frame().template static_cast_to<cell_t>();
//		m_cells.remove(cell->m_removeToken);
//		pane::detaching_child(p);
//		reshape(get_bounds());
//	}
//
//	void clear()
//	{
//		detach_children();
//	}
//	*/
//};
//
//
//}
//}
//
//
//#endif
//
