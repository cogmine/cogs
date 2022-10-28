//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	struct options
	{
		color backgroundColor = color::constant::white;
		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
		frame_list frames;
		pane_list children;
	};

	background()
		: background(options())
	{ }

	explicit background(options&& o)
		: pane({
			.compositingBehavior = o.compositingBehavior,
			.frames = std::move(o.frames),
			.children = std::move(o.children)
		}),
		m_color(o.backgroundColor),
		m_colorProperty(*this, [this]()
		{
			return m_color;
		}, [this](color c)
		{
			m_color = c;
			m_colorProperty.set_complete(true);
			invalidate(get_size());
		})
	{ }

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
	virtual void nest_last(const rcref<pane>& child) { pane::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane::nest_first(child); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { pane::nest_before(beforeThis, child); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { pane::nest_after(afterThis, child); }
};


}
}


#endif
