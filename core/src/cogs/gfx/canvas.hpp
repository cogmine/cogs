//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GFX_CANVAS
#define COGS_HEADER_GFX_CANVAS

#include <optional>
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
//typedef geometry::proportional_sizing_group proportional_sizing_group;
//typedef geometry::fair_sizing_group fair_sizing_group;
//typedef geometry::equal_sizing_group equal_sizing_group;
template <geometry::sizing_disposition disposition>
using sizing_group = geometry::sizing_group<disposition>;
typedef geometry::sizing_disposition sizing_disposition;
typedef geometry::sizing_cell sizing_cell;
typedef geometry::sizing_group_base sizing_group_base;

// A canvas is something that can be drawn to.
class canvas;


//class canvas::vector_image; // Base class for implementation dependent vector images.

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
	static composite_string to_string(const unit_t& n) { return to_string_t<wchar_t, unit_t>(); }

	template <typename unit_t>
	static composite_cstring to_cstring(const unit_t& n) { return to_string_t<char, unit_t>(); }
};



// A font_parameters object describes a particular font and style, etc.
// A font_parameters_list object contains a priority ordered list of font_parameters.
// In canvas::load_font(), the first font_parameters object in a font_parameters_list
// that can be successully matched to an available font will be used.

struct font_parameters;

typedef container_dlist<font_parameters> font_parameters_list;

struct font_parameters
{
	composite_string fontName;
	double pointSize = 0.0; // 0 point size implies default point size
	bool isItalic = false;
	bool isBold = false;
	bool isUnderlined = false;
	bool isStrikeOut = false;

	auto operator<=>(const font_parameters&) const = default;
	explicit operator font_parameters_list() const
	{
		return font_parameters_list(*this);
	}
};


/// @brief A base class for implementation specific fonts, returned by canvas::load_find().
class font
{
public:
	struct metrics
	{
		double ascent;
		double descent;
		double leading;
		double height;
		double spacing;
	};

	virtual metrics get_metrics() const = 0;
	virtual size calc_text_bounds(const composite_string& s) const = 0;
};

class bitmap;


/// @brief A base class for implementation specific B&W images (bit masks), returned by
/// canvas::load_bitmask() or canvas::create_bitmask().  Color values can be applied to each channel.
class bitmask
{
public:
	virtual size get_size() const = 0;

	enum class fill_mode
	{
		set_mode,
		clear_mode,
		invert_mode
	};

	virtual void fill(const bounds& b, fill_mode fillMode = fill_mode::set_mode) = 0;
	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, fill_mode fillMode = fill_mode::set_mode) = 0;

	virtual rcref<font> load_font(const font_parameters_list& f = font_parameters_list()) = 0;
	virtual string get_default_font_name() const = 0;

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& f = 0, bool value = true) = 0;

	enum class composite_mode
	{
		copy_mode,
		copy_inverted_mode,
		clear_mode,
		or_mode,
		and_mode,
		xor_mode
	};

	virtual void draw_bitmask(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode) = 0;

	virtual rcref<bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt) = 0;

	virtual void save_clip() = 0;
	virtual void restore_clip() = 0;
	virtual void clip_out(const bounds& b) = 0;
	virtual void clip_to(const bounds& b) = 0;
	virtual bool is_unclipped(const bounds& b) const = 0;
};

///// @brief A base class for implementation specific alpha masks returned by
///// canvas::load_alpha_mask or canvas::create_alpha_mask().
//class alpha_mask
//{
//public:
//	enum class fill_mode
//	{
//		set_mode,
//		multiply_mode,
//		subtract_mode,
//		add_mode
//	};
//
//	virtual void fill(const bounds& b, std::uint8_t alpha = 0xFF, fill_mode fillMode = fill_mode::set_mode) = 0;
//	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, std::uint8_t alpha = 0xFF, fill_mode fillMode = fill_mode::set_mode) = 0;
//
//	virtual rcref<font> load_font(const font_parameters_list& f = font_parameters_list()) = 0;
//	virtual string get_default_font_name() const = 0;
//
//	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& f = 0, std::uint8_t alpha = 0xFF) = 0;
//
//	enum class composite_mode
//	{
//		set_mode,
//		multiply_mode,
//		subtract_mode,
//		add_mode
//	};
//
//	virtual void draw_bitmask(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode) = 0;
//	virtual void draw_alpha_mask(const alpha_mask& msk, const bounds& mskBounds, const bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode) = 0;
//
//	virtual rcref<bitmask> create_alpha_mask(const size& sz) = 0;
//
//	virtual void save_clip() = 0;
//	virtual void restore_clip() = 0;
//	virtual void clip_out(const bounds& b) = 0;
//	virtual void clip_to(const bounds& b) = 0;
//	virtual bool is_unclipped(const bounds& b) const = 0;
//};

///// @brief A base class for implementation specific vector images, returned by
///// canvas::load_vector_image or load_vector_image::create_vector_image().
//class vector_image
//{
//};

/// @ingroup Graphics
/// @brief An interface for objects that can be drawn to, such as images or a GUI window.
class canvas
{
public:
	// Drawing primatives
	virtual void fill(const bounds& b, const color& c = color::constant::black, bool blendAlpha = true) = 0;
	virtual void invert(const bounds& b) = 0;
	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true) = 0;

	// text and font primatives
	virtual rcref<font> load_font(const font_parameters_list& f = font_parameters_list()) = 0;
	virtual string get_default_font_name() const = 0;

	virtual color get_default_text_foreground_color() const { return color::constant::black; }
	virtual color get_default_text_background_color() const { return color::constant::white; }
	virtual color get_default_selected_text_foreground_color() const { return color::constant::white; }
	virtual color get_default_selected_text_background_color() const { return color::constant::black; }
	virtual color get_default_label_foreground_color() const { return color::constant::black; }
	virtual color get_default_background_color() const { return color::constant::white; }

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& f = 0, const color& c = color::constant::black) = 0;

	// Compositing images
	virtual void draw_bitmap(const bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true) = 0;
	virtual void draw_bitmask(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true) = 0;
	//virtual void composite_vector_image(const vector_image& src, const bounds& dstBounds) = 0;

	virtual void mask_out(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false) = 0;
	virtual void draw_bitmap_with_bitmask(const bitmap& src, const bounds& srcBounds, const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false) = 0;
	//virtual void draw_bitmap_with_alpha_mask(const bitmap& src, const bounds& srcBounds, const alpha_mask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false) = 0;

	virtual rcref<bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt) = 0;
	virtual rcref<bitmap> load_bitmap(const composite_string& location) = 0;

	virtual rcref<bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt) = 0;
	virtual rcref<bitmask> load_bitmask(const composite_string& location) = 0;

	//virtual rcref<alpha_mask> create_alpha_mask(const size& sz) = 0;
	//virtual rcref<alpha_mask> load_alpha_mask(const composite_string& location) = 0;

	//virtual rcref<bitmask> create_vector_image(const size& sz) = 0;
	//virtual rcptr<vector_image> load_vector_image(const composite_string& location) = 0;

	// save_clip and restore_clip must be called in the same thread.
	// If nested, must be fully nested.  Pairs of save_clip/restore_clip must be nested
	// between pairs of save_clip/restore_clip.
	virtual void save_clip() = 0;
	virtual void restore_clip() = 0;
	virtual void clip_out(const bounds& b) = 0;
	virtual void clip_to(const bounds& b) = 0;
	virtual bool is_unclipped(const bounds& b) const = 0;

	// For UI elements, all clipping and drawing primatives are intended to be called only
	// within drawing() or a draw-delegate of a pane.
};


/// @brief A base class for implementation specific pixel/raster images, returned by canvas::load_bitmap() or canvas::create_bitmap.
class bitmap : public canvas
{
public:
	virtual size get_size() const = 0;
	virtual bool is_opaque() const = 0;

	// A bitmap has a logical size and an actual size.
	// If resized beyond the actual size, or trimmed to a smaller size, a new buffer is allocated.
	// The new buffer will be the requested size plus growPadding, allowing room for growth.
	// Use trimIfShrinking == true to reduce the padding if the buffer is reduced.  Otherwise,
	// buffers grow but are not trimmed.
	// To trim to a specific size with no padding, use: set_size(sz, size(0, 0), true);
	virtual void set_size(const size& newSize, const size& growPadding = size(100, 100), bool trimIfShrinking = false) = 0;
};


}
}


#endif
