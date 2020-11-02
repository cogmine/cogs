//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_BITMAP
#define COGS_HEADER_OS_GFX_BITMAP


#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/os/collections/macos_strings.hpp"
#include "cogs/os/gfx/graphics_context.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {
namespace os {

class bitmap_graphics_context
{
protected:
	CGContextRef m_context;
	mutable CGImageRef m_image = NULL;
	size_t m_logicalWidth;
	size_t m_logicalHeight;
	size_t m_actualWidth;
	size_t m_actualHeight;
	CGColorSpaceRef m_colorSpace;
	uint32_t m_bitmapInfo;

	void cache_image() const
	{
		if (m_image == NULL)
		{
			CGImageRef img = CGBitmapContextCreateImage(m_context);
			if (m_actualWidth != m_logicalWidth || m_actualHeight != m_logicalHeight)
			{
				CGRect r = { { 0, 0 }, { (CGFloat)m_logicalWidth, (CGFloat)m_logicalHeight } };
				r.origin.y = (m_actualHeight - r.origin.y) - r.size.height;
				CGImageRef croppedImage = CGImageCreateWithImageInRect(img, r);
				CGImageRelease(img);
				img = croppedImage;
			}
			m_image = img;
		}
	}

	CGImageRef load(const composite_string& name, bool resource)
	{
		CGImageRef cgImage;
		__strong NSString* name2 = string_to_NSString(name);
		__strong NSData* data;
		if (resource)
			data = [NSData dataWithContentsOfFile: [[NSBundle mainBundle] pathForResource:name2 ofType: nil]];
		else
			data = [NSData dataWithContentsOfFile: name2];
		__strong NSBitmapImageRep* imageRep = [NSBitmapImageRep imageRepWithData: data];
		cgImage = [imageRep CGImage];
		m_actualWidth = m_logicalWidth = CGImageGetWidth(cgImage);
		m_actualHeight = m_logicalHeight = CGImageGetHeight(cgImage);
		m_context = CGBitmapContextCreate(NULL, m_actualWidth, m_actualHeight, 8, 0, m_colorSpace, m_bitmapInfo);
		CGContextSaveGState(m_context);
		CGContextTranslateCTM(m_context, 0, m_actualHeight);
		CGContextScaleCTM(m_context, 1.0, -1.0);
		CGContextSetBlendMode(m_context, kCGBlendModeCopy);
		CGRect r = { { 0, 0 }, { (CGFloat)m_logicalWidth, (CGFloat)m_logicalHeight } };
		CGContextDrawImage(m_context, r, cgImage);
		CGContextRestoreGState(m_context);
		return cgImage;
	}

	friend class graphics_context;

public:
	bitmap_graphics_context(const composite_string& name, bool resource, bool& isOpaque) // loads as bitmap
		: m_colorSpace(CGColorSpaceCreateDeviceRGB()),
		m_bitmapInfo(kCGImageAlphaPremultipliedFirst)
	{
		CGImageRef img = load(name, resource);
		CGImageAlphaInfo alpha = CGImageGetAlphaInfo(img);
		switch (alpha)
		{
		case kCGImageAlphaFirst:
		case kCGImageAlphaLast:
		case kCGImageAlphaPremultipliedFirst:
		case kCGImageAlphaPremultipliedLast:
			isOpaque = false;
		default:
			isOpaque = true;
		};
	}

	bitmap_graphics_context(const composite_string& name, bool resource) // loads as bitmask
		: m_colorSpace(CGColorSpaceCreateDeviceGray()),
		m_bitmapInfo(kCGImageAlphaNone)
	{
		load(name, resource);
	}

	bitmap_graphics_context(size_t width, size_t height, CGColorSpaceRef colorSpace, uint32_t bitmapInfo)
		: m_actualWidth(width),
		m_actualHeight(height),
		m_logicalWidth(width),
		m_logicalHeight(height),
		m_colorSpace(colorSpace),
		m_bitmapInfo(bitmapInfo)
	{
		m_context = CGBitmapContextCreate(NULL, width, height, 8, 0, m_colorSpace, bitmapInfo);
	}

	~bitmap_graphics_context()
	{
		if (m_image != NULL)
			CGImageRelease(m_image);
		CGContextRelease(m_context);
		CGColorSpaceRelease(m_colorSpace);
	}

	gfx::size get_size() const { return gfx::size(m_logicalWidth, m_logicalHeight); }

	virtual void clear_cached() const
	{
		if (m_image != NULL)
		{
			CGImageRelease(m_image);
			m_image = NULL;
		}
	}

	void set_size(const gfx::size& newSize, const gfx::size& growPadding = gfx::size(100, 100), bool trimIfShrinking = false)
	{
		size_t newWidth = std::lround(newSize.get_width());
		size_t newHeight = std::lround(newSize.get_height());
		if (newWidth != m_logicalWidth || newHeight != m_logicalHeight)
		{
			size_t newActualWidth = newWidth + growPadding.get_width();
			size_t newActualHeight = newHeight + growPadding.get_height();
			bool widthOverflow = newWidth > m_actualWidth;
			bool heightOverflow = newHeight > m_actualHeight;
			bool reshape = (widthOverflow || heightOverflow);
			if (!reshape && trimIfShrinking)
			{
				bool widthDecrease = newActualWidth < m_actualWidth;
				bool heightDecrease = newActualHeight < m_actualHeight;
				reshape = (widthDecrease || heightDecrease);
			}
			if (!reshape)
				clear_cached();
			else
			{
				if (!trimIfShrinking)
				{
					if (newActualWidth < m_actualWidth)
						newActualWidth = m_actualWidth;
					if (newActualHeight < m_actualHeight)
						newActualHeight = m_actualHeight;
				}
				cache_image();
				CGContextRef newContext = CGBitmapContextCreate(NULL, newActualWidth, newActualHeight, 8, 0, m_colorSpace, m_bitmapInfo);
				CGContextSaveGState(newContext);
				CGContextSetBlendMode(newContext, kCGBlendModeCopy);
				CGRect r = { { 0, 0 }, { (CGFloat)m_logicalWidth, (CGFloat)m_logicalHeight } };
				CGContextDrawImage(newContext, r, m_image);
				CGContextRestoreGState(newContext);
				clear_cached();
				CGContextRelease(m_context);
				m_context = newContext;
				m_actualWidth = newActualWidth;
				m_actualHeight = newActualHeight;
			}
			m_logicalWidth = newWidth;
			m_logicalHeight = newHeight;
		}
	}
};

class bitmap : public gfx::bitmap, public bitmap_graphics_context
{
private:
	bool m_isOpaque;

	void update_opacity(const gfx::bounds& dstBounds, bool isSourceOpaque, bool blendAlpha)
	{
		if (m_isOpaque != isSourceOpaque)
		{
			if (!isSourceOpaque)
				m_isOpaque = blendAlpha;
			else
				m_isOpaque = dstBounds.get_x() <= 0 && (dstBounds.get_width() - dstBounds.get_x()) >= m_logicalWidth
				&& dstBounds.get_y() <= 0 && (dstBounds.get_height() - dstBounds.get_y()) >= m_logicalHeight;
		}
	}

	friend class graphics_context;

public:
	bitmap(const composite_string& name, bool resource = true)
		: bitmap_graphics_context(name, resource, m_isOpaque)
	{
	}

	bitmap(const gfx::size& sz, std::optional<color> fillColor = std::nullopt)
		: bitmap_graphics_context(std::lround(sz.get_width()), std::lround(sz.get_height()), CGColorSpaceCreateDeviceRGB(), kCGImageAlphaPremultipliedFirst)
	{
		if (!fillColor.has_value() || !fillColor->is_fully_transparent())
			m_isOpaque = false;
		else
		{
			m_isOpaque = fillColor->is_opaque();
			fill(gfx::size(m_logicalWidth, m_logicalHeight), *fillColor, false);
		}
	}

	virtual gfx::size get_size() const { return gfx::size(m_logicalWidth, m_logicalHeight); }

	virtual bool is_opaque() const { return m_isOpaque; }

	virtual void fill(const gfx::bounds& b, const color& c = color::constant::black, bool blendAlpha = true)
	{
		clear_cached();
		graphics_context::fill(b, c, blendAlpha, m_context);
	}

	virtual void invert(const gfx::bounds& b)
	{
		clear_cached();
		graphics_context::invert(b, m_context);
	}

	virtual void draw_line(const gfx::point& startPt, const gfx::point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true)
	{
		clear_cached();
		graphics_context::draw_line(startPt, endPt, width, c, blendAlpha, m_context);
	}

	virtual rcref<gfx::font> load_font(const gfx::font_parameters_list& f) { return graphics_context::load_font(f, m_context); }

	virtual string get_default_font_name() const { return graphics_context::get_default_font_name(); }

	virtual void draw_text(const composite_string& s, const gfx::bounds& b, const rcptr<gfx::font>& f = 0, const color& c = color::constant::black)
	{
		clear_cached();
		graphics_context::draw_text(s, b, f, c, m_context);
	}

	virtual void draw_bitmap(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, bool blendAlpha = true)
	{
		if (!!srcBounds && !!dstBounds)
		{
			const bitmap& src2 = *static_cast<const bitmap*>(&src);
			graphics_context::draw_bitmap(src2, srcBounds, dstBounds, blendAlpha, m_context);
			clear_cached();
			update_opacity(dstBounds, src2.is_opaque(), blendAlpha);
		}
	}

	virtual void draw_bitmask(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true);

	virtual rcref<gfx::bitmap> create_bitmap(const gfx::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return graphics_context::create_bitmap(sz, fillColor);
	}

	virtual rcref<gfx::bitmap> load_bitmap(const composite_string& location)
	{
		return graphics_context::load_bitmap(location);
	}

	virtual rcref<gfx::bitmask> create_bitmask(const gfx::size& sz, std::optional<bool> value = std::nullopt)
	{
		return graphics_context::create_bitmask(sz, value);
	}

	virtual rcref<gfx::bitmask> load_bitmask(const composite_string& location)
	{
		return graphics_context::load_bitmask(location);
	}

	virtual void save_clip()
	{
		graphics_context::save_clip(m_context);
	}

	virtual void restore_clip()
	{
		graphics_context::restore_clip(m_context);
	}

	virtual void clip_out(const gfx::bounds& b)
	{
		graphics_context::clip_out(b, gfx::size(m_logicalWidth, m_logicalHeight), m_context);
	}

	virtual void clip_to(const gfx::bounds& b)
	{
		graphics_context::clip_to(b, m_context);
	}

	virtual bool is_unclipped(const gfx::bounds& b) const
	{
		return graphics_context::is_unclipped(b, m_context);
	}

	virtual void set_size(const gfx::size& newSize, const gfx::size& growPadding = gfx::size(100, 100), bool trimIfShrinking = false)
	{
		size_t oldWidth = m_logicalWidth;
		size_t oldHeight = m_logicalHeight;
		bitmap_graphics_context::set_size(newSize, growPadding, trimIfShrinking);
		size_t newWidth = m_logicalWidth;
		size_t newHeight = m_logicalHeight;
		bool widthIncreased = newWidth > oldWidth;
		bool heightIncreased = newHeight > oldHeight;
		if (heightIncreased || widthIncreased)
		{
			clear_cached();
			graphics_context::state_token token(m_context);
			if (widthIncreased)
			{
				double widthDifference = newWidth - oldWidth;
				graphics_context::fill({ { (double)oldWidth, 0 }, { (double)widthDifference, (double)oldHeight } }, m_isOpaque ? color::constant::black : color::constant::transparent, false, token);
			}
			if (heightIncreased)
			{
				double heightDifference = newHeight - oldHeight;
				double width;
				if (widthIncreased)
					width = oldWidth;
				else
					width = newWidth;
				graphics_context::fill({ { 0, (double)oldHeight }, { (double)width, (double)heightDifference } }, m_isOpaque ? color::constant::black : color::constant::transparent, false, token);
			}
		}
	}

	virtual void draw_bitmap_with_bitmask(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool blendAlpha = true, bool inverted = false);

	virtual void mask_out(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool inverted = false);
};

inline void graphics_context::draw_image(const bitmap_graphics_context& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, CGBlendMode blendMode, const state_token& token)
{
	src.cache_image();
	CGImageRef sourceImage = src.m_image;
	CGImageRef croppedSourceImage = NULL;
	bool isSourceCropped = srcBounds != src.get_size();
	if (isSourceCropped)
	{
		CGRect r = make_CGRect(srcBounds);
		r.origin.y = (src.m_actualHeight - r.origin.y) - r.size.height;
		croppedSourceImage = CGImageCreateWithImageInRect(sourceImage, r);
		sourceImage = croppedSourceImage;
	}
	CGContextRef ctx = token.get_CGContext();
	CGContextSetBlendMode(ctx, blendMode);
	CGRect r = make_CGRect(dstBounds);
	CGContextDrawImage(ctx, r, sourceImage);
	if (isSourceCropped)
		CGImageRelease(croppedSourceImage);
}

inline void graphics_context::draw_bitmap(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, bool blendAlpha, const state_token& token)
{
	if (!!srcBounds && !!dstBounds)
	{
		const bitmap& src2 = *static_cast<const bitmap*>(&src);
		if (src2.is_opaque())
			blendAlpha = false;
		CGBlendMode blendMode = blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy;
		draw_image(src2, srcBounds, dstBounds, blendMode, token);
	}
}

inline rcref<gfx::bitmap> graphics_context::create_bitmap(const gfx::size& sz, std::optional<color> fillColor)
{
	return rcnew(bitmap)(sz, fillColor);
}

inline rcref<gfx::bitmap> graphics_context::load_bitmap(const composite_string& location)
{
	return rcnew(bitmap)(location);
}


}
}


#include "cogs/os/gfx/bitmask.hpp"


#endif
