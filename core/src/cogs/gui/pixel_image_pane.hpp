//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_PIXEL_IMAGE_PANE
#define COGS_HEADER_GUI_PIXEL_IMAGE_PANE


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief A pane that displays a pixel_image.
class pixel_image_pane : public pane
{
private:
	composite_string m_imageLocation;
	rcptr<pixel_image> m_image;
	double m_dpi;

public:
	pixel_image_pane(const ptr<rc_obj_base>& desc, const composite_string& imageLocation, double dpi = canvas::dip_dpi)
		:	pane(desc),
		m_imageLocation(imageLocation),
		m_dpi(dpi)
	{
	}

	virtual void installing()
	{
		m_image = load_pixel_image(m_imageLocation);
		if (!!m_image)
			m_image->set_dpi(m_dpi);

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

	virtual void dpi_changing(double oldDpi, double newDpi)
	{
		pane::dpi_changing(oldDpi, newDpi);
	}

	virtual range get_range() const
	{
		range r;	// unset, no limits.  Image can be stretched by default.
		return r;
	}
	
	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
	{
		pane::reshape(r, oldOrigin);
		invalidate(get_size());
	}

	virtual void drawing()
	{
		if (!!m_image)
		{
			size originalSize = m_image->get_size();
			size stretchTo = get_bounds().get_size();
			composite_scaled_pixel_image(*m_image, originalSize, stretchTo);
		}
	}
};


}
}


#endif

