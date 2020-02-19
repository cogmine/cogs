//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_BITMAP_PANE
#define COGS_HEADER_GUI_BITMAP_PANE


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief A pane that displays a bitmap.
class bitmap_pane : public pane
{
private:
	composite_string m_imageLocation;
	rcptr<bitmap> m_image;

public:
	bitmap_pane(const composite_string& imageLocation)
		: m_imageLocation(imageLocation)
	{ }

	virtual void installing()
	{
		m_image = load_bitmap(m_imageLocation);
		pane::installing();
	}

	virtual void uninstalling()
	{
		m_image = 0;
		pane::uninstalling();
	}

	virtual bool is_opaque() const
	{
		if (!!m_image)
			return m_image->is_opaque();
		return false;
	}

	virtual size get_default_size() const
	{
		if (!!m_image)
			return m_image->get_size();
		return size(0, 0);
	}

	virtual range get_range() const
	{
		range r; // unset, no limits.  Image can be stretched by default.
		return r;
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		pane::reshape(b, oldOrigin);
		if (!!m_image)
			invalidate(get_size());
	}

	virtual void drawing()
	{
		if (!!m_image)
		{
			size originalSize = m_image->get_size();
			size stretchTo = get_size();
			draw_bitmap(*m_image, originalSize, stretchTo);
		}
	}
};


}
}


#endif
