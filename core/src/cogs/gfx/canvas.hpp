//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CANVAS
#define COGS_CANVAS

#include "cogs/collections/string.hpp"
#include "cogs/geometry/alignment.hpp"
#include "cogs/geometry/point.hpp"
#include "cogs/geometry/size.hpp"
#include "cogs/geometry/bounds.hpp"
#include "cogs/geometry/direction.hpp"
#include "cogs/geometry/flow.hpp"
#include "cogs/geometry/cell.hpp"
#include "cogs/geometry/sizing_groups.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/gfx/font.hpp"
#include "cogs/math/measurement_types.hpp"
#include "cogs/math/vec.hpp"


namespace cogs {

/// @defgroup Graphics Graphics
/// @{
/// @brief Graphics
/// @}

/// @ingroup Graphics
/// @brief Namespace for graphics
namespace gfx {


// A canvas is something that can be drawn to.
class canvas;

//class canvas::font;			// Base class for implementation dependent fonts.
//class canvas::pixel_image;	// Base class for implementation dependent pixel/raster images.
//class canvas::pixel_mask;		// Base class for implementation dependent bit masks,
								// which can have color values applied to each channel.
//class canvas::vector_image;	// Base class for implementation dependent vector images.

//class canvas::pixel_image_canvas;	// A pixel_image_canvas is essentially an off-screen pixel buffer for blitting
									// onto a canvas.  It's both a canvas and a pixel_image.

/// @ingroup Graphics
/// @ingroup UnitBases
/// @brief Pixels unit base
class pixels : public unit_type<distance>
{
public:
	template <typename char_t, typename unit_t>
	static composite_string_t<char_t> to_string_t(const unit_t& n)
	{
		static constexpr char_t part2 = (char_t)'p';
		composite_string_t<char_t> result(n.template to_string_t<char_t>());
		result += string_t<char_t>::contains(&part2, 1);
		return result;
	}

	template <typename unit_t>
	static composite_string to_string(const unit_t& n)		{ return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n)	{ return to_string_t<char, unit_t>(); }
};


/// @ingroup Graphics
/// @brief An interface for objects that can be drawn to, such as images or a UI window.
class canvas
{
public:
	typedef geometry::planar::size size;
	typedef geometry::planar::point point;
	typedef geometry::planar::bounds bounds;
	typedef geometry::planar::range range;
	typedef geometry::planar::margin margin;
	typedef geometry::planar::proportion proportion;
	typedef geometry::planar::direction direction;
	typedef geometry::planar::dimension dimension;
	typedef geometry::planar::flow flow;
	typedef geometry::planar::script_flow script_flow;
	typedef geometry::planar::cell cell;
	typedef geometry::planar::alignment alignment;
	typedef geometry::proportional_sizing_group proportional_sizing_group;
	typedef geometry::fair_sizing_group fair_sizing_group;
	typedef geometry::equal_sizing_group equal_sizing_group;

	/// @brief A base class for implementation specific fonts, returned by canvas::load_find().
	class font
	{
	public:
		struct metrics
		{
			double m_ascent;
			double m_descent;
			double m_spacing;
		};

		virtual font::metrics get_metrics() const = 0;
		virtual size calc_text_bounds(const composite_string& s) const = 0;
	};

	/// @brief A base class for implementation specific images, returned by canvas::load_pixel_image().
	class pixel_image
	{
	public:
		virtual size get_size() const = 0;
		virtual double get_dpi() const = 0;
		virtual void set_dpi(double dpi) = 0;
		virtual bool is_opaque() const = 0;

		// get_pixel?

		double get_dpi_scale() const { return get_dpi() / canvas::dip_dpi; }
	};

	/// @brief A base class for implementation specific B&W images (bit masks), returned by canvas::load_pixel_mask().
	class pixel_mask
	{
	public:
		virtual size get_size() const = 0;
		virtual double get_dpi() const = 0;
		virtual void set_dpi(double dpi) = 0;

		// get_pixel?

		double get_dpi_scale() const { return get_dpi() / canvas::dip_dpi; }
	};

	//class vector_image
	//{
	//};

	class pixel_image_canvas;

	// Drawing primatives
	virtual void fill(const bounds& r, const color& c = color::black, bool blendAlpha = true) = 0;
	virtual void invert(const bounds& r) = 0;
	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true) = 0;
	virtual void scroll(const bounds& r, const point& pt = point(0,0)) = 0;

	// text and font primatives
	virtual rcref<font> load_font(const gfx::font& guiFont = gfx::font()) = 0;
	virtual gfx::font get_default_font() = 0;

	virtual void draw_text(const composite_string& s, const bounds& r, const rcptr<font>& f = 0, const color& c = color::black, bool blendAlpha = true) = 0;

	// Compositing images
	virtual void composite_pixel_mask(const pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true) = 0;
	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true) = 0;
	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds) = 0;
	//virtual void composite_vector_image(const vector_image& src, const bounds& dstBounds) = 0;

	virtual rcref<pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi) = 0;
	virtual rcref<pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi) = 0;
	virtual rcref<pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi) = 0;
	//virtual rcptr<vector_image> load_vector_image(const composite_string& location) = 0;

	// save_clip and restore_clip must be called in the same thread.
	// If nested, must be fully nested.  Pairs of save_clip/restore_clip must be nested
	// between pairs of save_clip/restore_clip.
	virtual void save_clip() = 0;
	virtual void restore_clip() = 0;
	virtual void clip_out(const bounds& r) = 0;
	virtual void clip_to(const bounds& r) = 0;
	virtual bool is_unclipped(const bounds& r) const = 0;

	static constexpr int dip_dpi = 96;

	virtual double get_dpi() const = 0;
	double get_dpi_scale() const { return get_dpi() / dip_dpi; }

	// For UI elements, all clipping and drawing primatives are intended to be called only
	// within drawing() or a draw-delegate of a pane.
};


/// @brief A base class for implementation specific objects that are both a pixel_image and canvas (can be both drawn to and composited), returned by canvas::create_pixel_image_canvas().
///
/// A pixel_image_canvasis essentially an off-screen buffer for blitting onto a canvas.
class canvas::pixel_image_canvas : public canvas, public canvas::pixel_image
{
public:
	virtual double get_dpi() const = 0;
	using canvas::get_dpi_scale;

	// A pixel_image_canvas has a logical size and an actual size.
	// If resized beyond the actual size, or trimmed to a smaller size, a new buffer is allocated.
	// The new buffer will be the requested size plus growPadding, allowing room for growth.
	// Use trimIfShrinking == true to reduce the padding if the buffer is reduced.  Otherwise,
	// buffers grow but are not trimmed.
	// To trim to a specific size with no padding, use: set_size(sz, size(0, 0), true);
	virtual void set_size(const size& newSize, const size& growPadding = size(100, 100), bool trimIfShrinking = false) = 0;
	
	virtual size get_actual_size() const = 0;

	// get_pixel?
	// set_pixel?

	// Helper functions to convert back and forth between device-independent pixels (DIPs) and actual pixels.

	double dip_to_pixel_size(const double& pos) const	// Only use when position is known to be 0
	{
		double dpiScale = get_dpi_scale();
		return std::round(dpiScale * pos);
	}

	size dip_to_pixel_size(const size& sz) const	// Only use when position is known to be 0,0
	{
		size sz2;
		double dpiScale = get_dpi_scale();
		sz2.get_width() = std::round(dpiScale * sz.get_width());
		sz2.get_height() = std::round(dpiScale * sz.get_height());
		return sz2;
	}

	point dip_to_pixel_point(const point& pt) const
	{
		point pt2;
		double dpiScale = get_dpi_scale();
		pt2.get_x() = std::round(dpiScale * pt.get_x());
		pt2.get_y() = std::round(dpiScale * pt.get_y());
		return pt2;
	}

	point dip_to_pixel_point(const bounds& b) const
	{
		return dip_to_pixel_point(b.calc_position());
	}

	bounds dip_to_pixel_bounds(const point& pt, const size& sz) const
	{
		point topLeft;
		point bottomRight;
		double dpiScale = get_dpi_scale();
		topLeft.get_x() = std::round(dpiScale * pt.get_x());
		topLeft.get_y() = std::round(dpiScale * pt.get_y());
		bottomRight.get_x() = std::round(dpiScale * (pt.get_x() + sz.get_width()));
		bottomRight.get_y() = std::round(dpiScale * (pt.get_y() + sz.get_height()));
		bounds b(topLeft, bottomRight - topLeft);
		return b;
	}

	bounds dip_to_pixel_bounds(const bounds& b) const
	{
		bounds b2 = b.normalized();
		return dip_to_pixel_bounds(b2.get_position(), b2.get_size());
	}

	bounds dip_to_pixel_bounds_for_invalidation(const point& pt, const size& sz) const
	{
		double dpiScale = get_dpi_scale();
		point topLeft(
			std::floor(dpiScale * pt.get_x()),
			std::floor(dpiScale * pt.get_y()));
		point bottomRight(
			std::ceil(dpiScale * (pt.get_x() + sz.get_width())),
			std::ceil(dpiScale * (pt.get_y() + sz.get_height())));
		bounds b(topLeft, bottomRight - topLeft);
		return b;
	}

	bounds dip_to_pixel_bounds_for_invalidation(const bounds& b) const
	{
		bounds b2 = b.normalized();
		return dip_to_pixel_bounds_for_invalidation(b2.get_position(), b2.get_size());
	}

	size pixel_to_dip_size(const size& sz) const
	{
		double dpiScale = get_dpi_scale();
		return size(sz.get_width() / dpiScale, sz.get_height() / dpiScale);
	}

	point pixel_to_dip_point(const point& pt) const
	{
		double dpiScale = get_dpi_scale();
		return point(pt.get_x() / dpiScale, pt.get_y() / dpiScale);
	}

	point pixel_to_dip_point(const bounds& b) const
	{
		return pixel_to_dip_point(b.calc_position());
	}

	bounds pixel_to_dip_bounds(const point& pt, const size& sz) const
	{
		double dpiScale = get_dpi_scale();
		bounds b2(
			point(pt.get_x() / dpiScale, pt.get_y() / dpiScale),
			size(sz.get_width() / dpiScale, sz.get_height() / dpiScale));
		return b2;
	}

	bounds pixel_to_dip_bounds(const bounds& b) const
	{
		bounds b2 = b.normalized();
		return pixel_to_dip_bounds(b2.get_position(), b2.get_size());
	}
};


}
}


#endif

