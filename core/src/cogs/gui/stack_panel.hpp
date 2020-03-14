////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_GUI_STACK_PANEL
//#define COGS_HEADER_GUI_STACK_PANEL
//
//#include "cogs/gui/pane.hpp"
//
//namespace cogs {
//namespace gui {
//
//
//class stack_panel : public pane, protected virtual pane_container
//{
//private:
//	range m_calculatedRange;
//	size m_calculatedDefaultSize;
//	container_dlist<std::pair<rcref<frame>, rcref<pane> > > m_panes;
//
//public:
//	explicit stack_panel(dimension orientation = dimension::vertical,
//		const std::initializer_list<rcref<frame> >& frames = {},
//		const std::initializer_list<rcref<pane> >& children = {})
//		: pane(frames, children)
//	{
//	}
//
//	virtual range get_range() const { return m_calculatedRange; }
//
//	virtual size get_default_size() const
//	{
//
//		return m_calculatedDefaultSize;
//	}
//
//	using pane_container::nest;
//
//	virtual void nest_last(const rcref<pane>& child) { nest_last(child, 0); }
//	void nest_last(const rcref<pane>& child, int resizeWeight = 0)
//	{
//	}
//
//	virtual void nest_first(const rcref<pane>& child) { nest_first(child, 0); }
//	void nest_first(const rcref<pane>& child, int resizeWeight = 0)
//	{
//	}
//
//	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis) { nest_before(child, beforeThis, 0); }
//	void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, int resizeWidth = 0)
//	{
//	}
//
//	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis) { nest_after(child, afterThis, 0); }
//	void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, int resizeWidth = 0)
//	{
//	}
//
//	virtual void calculate_range()
//	{
//		pane::calculate_range();
//
//	}
//
//	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> = std::nullopt, const range& r = range::make_unbounded(), size_mode = size_mode::both, size_mode = size_mode::both) const
//	{
//		propose_size_result result;
//		range r2 = get_range() & r;
//		if (r2.is_empty())
//			result.set_empty();
//		else
//		{
//			size newSize = r2.limit(sz);
//			
//
//
//			result.set(newSize);
//			result.make_relative(sz);
//		}
//		return result;
//	}
//
//	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//	{
//
//	}
//};
//
//
//}
//}
//
//
//#endif
