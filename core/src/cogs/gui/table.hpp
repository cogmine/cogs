////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: Placeholder
//
//#ifndef COGS_HEADER_GUI_TABLE
//#define COGS_HEADER_GUI_TABLE
//
//
//#include "cogs/collections/vector.hpp"
//#include "cogs/gui/pane.hpp"
//
//
//namespace cogs {
//namespace gui {
//
//
//// A table is similar to an HTML table.  It contains a table of frames.
//class table : public pane
//{
//private:
//	// child nodes indexed by start row
//	vector< vector<rcref<frame> > > m_rows;
//
//	// child nodes indexed by start columns
//	vector< vector<rcref<frame> > > m_columns;
//
//	// default row height, indexed by row
//	// cached since last reshape.
//	vector<size2D_t> m_rowDefaultHeights;
//
//	// default column widths, indexed by column
//	// cached since last reshape.
//	vector<size2D_t> m_columnDefaultWidths;
//
//	// default row height ranges, indexed by row
//	// cached since last reshape.
//	vector<range1D_t> m_rowHeightRanges;
//
//	// default column width ranges, indexed by column
//	// cached since last reshape.
//	vector<range1D_t> m_columnWidthRanges;
//
//	class cell_frame
//	{
//	public:
//		size_t m_row;
//		size_t m_column;
//		size_t m_rowHeight;
//		size_t m_columnWidth;
//
//		rcref<frame> m_userReshaper;
//
//		cell_frame(size_t row, size_t column, size_t rowHeight, size_t columnWidth, const rcref<frame>& userReshaper)
//			:	m_row(row),
//				m_column(column),
//				m_rowHeight(rowHeight),
//				m_columnWidth(columnWidth),
//				m_userReshaper(userReshaper)
//		{ }
//
//		virtual void reshape(frame& p, const point& oldOrigin = point(0, 0))
//		{
//			table* t = (table*)&p;
//			p.reshape(parentSize, oldOrigin);
//		}
//	};
//
//
//	void recalculate()
//	{
//		size_t numRows = m_rows.size();
//		size_t numColumns = m_columns.size();
//
//		size_t newValue = 0;
//		m_rowHeights.assign(newValue, numRows);
//		m_columnWidths.assign(newValue, numColumns);
//
//		size_t totalHeight = 0;
//		size_t totalWidth = 0;
//		size_t i;
//		size_t i2;
//		for (i = 0; i < numRows; i++)
//		{
//			size_t curHeight = 0;
//			vector<rcref<frame> >& v = m_rows[i];
//			size_t numFrames = v.size();
//			for (i2 = 0; i2 < numFrames; i2++)
//			{
//				const rcref<frame>& f = v[i2];
//				const rcptr<cell_frame>& frame = f->get_frame();
//				cell_frame* cellReshaper = frame.get();
//#error defaultSize?  current size?
//				size_t index = i + cellReshaper->m_rowHeight;
//				size_t oldValue = m_rowHeights[index];
//
//			}
//		}
//		
//		for (i = 0; i < numColumns; i++)
//		{
//			size_t curWidth = 0;
//			vector<rcref<frame> >& v = m_columns[i];
//			size_t numFrames = v.size();
//			for (i2 = 0; i2 < numFrames; i2++)
//			{
//				const rcref<frame>& f = v[i2];
//				const rcptr<cell_frame>& frame = f->get_frame();
//				cell_frame* cellReshaper = frame.get();
//
//				;
//			}
//		}
//	}
//
//	//friend class cell_frame;
//
//public:
//	table()
//	{ }
//
//	table(int numRows, int numColumns)
//	{ }
//
//	size_t get_num_rows() const		{ return m_rows.size(); }
//	size_t get_num_columns() const	{ return m_columns.size(); }
//
//	// row and column are 0-based
//	void nest(const rcref<frame>& child, size_t row, size_t column, size_t numRows = 1, size_t numColumns = 1)
//	{
//		if (numRows && numColumns)
//		{
//			bool needsReshape = false;
//			size_t pastEndRow = row + numRows;
//			size_t pastEndColumn = row + numColumns;
//
//			if (pastEndRow > m_rows.size())
//			{
//				m_rows.resize(pastEndRow);
//				needsReshape = true;
//			}
//
//			if (pastEndColumn > m_numColumns)
//			{
//				m_numColumns = pastEndColumn;
//				needsReshape = true;
//			}
//
//			if (needsReshape)
//				reshape_children();
//
//			panel::nest(child, rcnew(cell_frame, row, column, numRows, numColumns));
//		}
//	}
//
//	// TO DO: When elements are removed, max rows/columns might change
//
//
//};
//
//
//}
//}
//
//
//#endif
//
