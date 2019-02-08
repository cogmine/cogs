//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_GUI_BACKGROUND
#define COGS_GUI_BACKGROUND


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/binding.hpp"


namespace cogs {
namespace gui {

#pragma warning(push)
#pragma warning (disable: 4250)

/// @ingroup GUI
/// @brief A pane that fills with a given color
class background : public pane, public virtual pane_container
{
private:
	volatile color m_color;

	delayed_construction<delegated_property<color> > m_colorProperty;

	void create_properties()
	{
		auto colorGetter = [this]()
		{
			return m_color;
		};
		
		auto colorSetter = [this](color c)
		{
			m_color = c;
			m_colorProperty->set_complete(true);
			invalidate(get_size());
		};

		placement_rcnew(this_desc, &m_colorProperty.get(), *this, std::move(colorGetter), std::move(colorSetter));
	}

public:
	background(const color& c)
		: m_color(c)
	{
		create_properties();
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
		
	rcref<property<color> >		get_color_property() { return get_self_rcref(&m_colorProperty.get()).static_cast_to<property<color> >(); }

	color get_color() const
	{
		return m_colorProperty->get();
	}

	void set_color(const color& c)
	{
		m_colorProperty->set(c);
	}

	using pane::get_pane_container;
	using pane::get_pane_container_ref;

	using pane::nest;
	using pane::nest_last;
	using pane::nest_first;
	using pane::nest_before;
	using pane::nest_after;
};

#pragma warning(pop)

}
}


#endif

