//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_BACKGROUND
#define COGS_HEADER_GUI_BACKGROUND


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/dependency_property.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief A pane that fills with a given color
class background : public pane, public virtual pane_container
{
private:
	volatile color m_color;

	delegated_dependency_property<color> m_colorProperty;

public:
	background(const ptr<rc_obj_base>& desc, const color& c, compositing_behavior cb = compositing_behavior::no_buffer)
		: pane(desc, cb),
		m_color(c),
		m_colorProperty(desc, *this, [this]()
			{
				return m_color;
			}, [this](color c)
			{
				m_color = c;
				m_colorProperty.set_complete(true);
				invalidate(get_size());
			})
	{
	}

	virtual bool is_opaque() const
	{
		return m_color.is_opaque();
	}

	virtual void drawing()
	{
		color c = m_color;
		fill(get_size(), c);
	}

	rcref<dependency_property<color> > get_color_property() { return get_self_rcref(&m_colorProperty); }

	color get_color() const
	{
		return m_colorProperty.get();
	}

	void set_color(const color& c)
	{
		m_colorProperty.set(c);
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }
};


}
}


#endif
