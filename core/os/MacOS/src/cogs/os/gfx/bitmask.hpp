//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_BITMASK
#define COGS_HEADER_OS_GFX_BITMASK


#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/os/collections/macos_strings.hpp"
#include "cogs/os/gfx/graphics_context.hpp"
#include "cogs/os/gfx/bitmap.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {
namespace gfx {
namespace os {


class bitmask : public canvas::bitmask, public bitmap_graphics_context
{
private:
	mutable CGImageRef m_invertedMask = NULL;

	void cache_inverted_mask() const
	{
		if (m_invertedMask == NULL)
		{
			cache_image();
			size_t bytesPerRow = CGBitmapContextGetBytesPerRow(m_context);
			CGDataProviderRef provider = CGImageGetDataProvider(m_image);
			m_invertedMask = CGImageMaskCreate(m_logicalWidth, m_logicalHeight, 8, 8, bytesPerRow, provider, NULL, false);
		}
	}

	virtual void clear_cached() const
	{
		if (m_invertedMask != NULL)
		{
			CGImageRelease(m_invertedMask);
			m_invertedMask = NULL;
		}
		bitmap_graphics_context::clear_cached();
	}

	friend class graphics_context;

public:
	bitmask(const composite_string& name, bool resource = true)
		: bitmap_graphics_context(name, resource)
	{ }

	bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt)
		: bitmap_graphics_context(std::lround(sz.get_width()), std::lround(sz.get_height()), CGColorSpaceCreateDeviceGray(), kCGImageAlphaNone)
	{
		if (value.has_value())
			fill(canvas::size(m_logicalWidth, m_logicalHeight), *value ? fill_mode::set_mode : fill_mode::clear_mode);
	}

	~bitmask()
	{
		if (m_invertedMask != NULL)
			CGImageRelease(m_invertedMask);
	}

	virtual canvas::size get_size() const { return canvas::size(m_logicalWidth, m_logicalHeight); }

	virtual void fill(const canvas::bounds& b, fill_mode fillMode = fill_mode::set_mode)
	{
		clear_cached();
		CGBlendMode blendMode;
		switch (fillMode)
		{
		case fill_mode::invert_mode:
			blendMode = kCGBlendModeDifference;
			break;
		case fill_mode::clear_mode:
			blendMode = kCGBlendModeClear;
			break;
		case fill_mode::set_mode:
		default:
			blendMode = kCGBlendModeCopy;
			break;
		}
		graphics_context::state_token token(m_context);
		CGContextSetGrayFillColor(m_context, 1.0, 1.0);
		graphics_context::fill(b, blendMode, token);
	}

	virtual void draw_line(const canvas::point& startPt, const canvas::point& endPt, double width = 1, fill_mode fillMode = fill_mode::set_mode)
	{
		clear_cached();
		CGBlendMode blendMode;
		switch (fillMode)
		{
		case fill_mode::clear_mode:
			blendMode = kCGBlendModeClear;
			break;
		case fill_mode::invert_mode:
			blendMode = kCGBlendModeDifference;
			break;
		case fill_mode::set_mode:
		default:
			blendMode = kCGBlendModeCopy;
			break;
		}
		graphics_context::state_token token(m_context);
		CGContextSetGrayStrokeColor(m_context, 1.0, 1.0);
		graphics_context::draw_line(startPt, endPt, width, blendMode, token);
	}

	virtual rcref<canvas::font> load_font(const gfx::font& f) { return graphics_context::load_font(f, m_context); }

	virtual gfx::font get_default_font() const { return graphics_context::get_default_font(); }

	virtual void draw_text(const composite_string& s, const canvas::bounds& b, const rcptr<canvas::font>& f = 0, bool value = true)
	{
		clear_cached();
		CGColorRef colorRef = CGColorCreateGenericGray(value ? 1.0 : 0.0, 1.0);
		graphics_context::draw_text(s, b, f, colorRef, m_context);
		CGColorRelease(colorRef);
	}

	virtual void draw_bitmask(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode)
	{
		if (!!mskBounds && !!mskBounds)
		{
			const bitmask& msk2 = *static_cast<const bitmask*>(&msk);
			CGBlendMode blendMode;
			switch (compositeMode)
			{
			case composite_mode::and_mode:
				blendMode = kCGBlendModeMultiply;
				break;
			case composite_mode::or_mode:
				blendMode = kCGBlendModeNormal;
				break;
			case composite_mode::xor_mode:
				blendMode = kCGBlendModeXOR;
				break;
			case composite_mode::clear_mode:
				blendMode = kCGBlendModeClear;
				break;
			case composite_mode::copy_inverted_mode:
				blendMode = kCGBlendModeDifference;
				break;
			case composite_mode::copy_mode:
			default:
				blendMode = kCGBlendModeCopy;
				break;
			}
			graphics_context::draw_image(msk2, mskBounds, dstBounds, blendMode, m_context);
			clear_cached();
		}
	}

	virtual rcref<canvas::bitmap> create_bitmap(const canvas::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return graphics_context::create_bitmap(sz, fillColor);
	}

	virtual rcref<canvas::bitmap> load_bitmap(const composite_string& location)
	{
		return graphics_context::load_bitmap(location);
	}

	virtual rcref<canvas::bitmask> create_bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt)
	{
		return graphics_context::create_bitmask(sz, value);
	}

	virtual rcref<canvas::bitmask> load_bitmask(const composite_string& location)
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

	virtual void clip_out(const canvas::bounds& b)
	{
		graphics_context::clip_out(b, canvas::size(m_logicalWidth, m_logicalHeight), m_context);
	}

	virtual void clip_to(const canvas::bounds& b)
	{
		graphics_context::clip_to(b, m_context);
	}

	virtual bool is_unclipped(const canvas::bounds& b) const
	{
		return graphics_context::is_unclipped(b, m_context);
	}

	virtual void set_size(const canvas::size& newSize, const canvas::size& growPadding = canvas::size(100, 100), bool trimIfShrinking = false)
	{
		bitmap_graphics_context::set_size(newSize, growPadding, trimIfShrinking);
	}
};

inline void bitmap::draw_bitmask(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	if (!!mskBounds && !!dstBounds)
	{
		clear_cached();
		uint8_t foreAlpha = fore.get_alpha();
		uint8_t backAlpha = back.get_alpha();
		bool areBothFullyTransparent = blendForeAlpha && blendBackAlpha && !foreAlpha && !backAlpha;
		if (!areBothFullyTransparent)
		{
			if (foreAlpha == 0xFF)
				blendForeAlpha = true;
			if (backAlpha == 0xFF)
				blendBackAlpha = true;
			const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
			graphics_context::draw_bitmask(msk2, mskBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha, m_context);
			if ((!blendForeAlpha && !fore.is_opaque()) || (!blendBackAlpha && !back.is_opaque()))
				m_isOpaque = false;
			else if (!blendForeAlpha && !blendBackAlpha && fore.is_opaque() && back.is_opaque())
				update_opacity(dstBounds, true, false);
		}
	}
}

inline void bitmap::draw_bitmap_with_bitmask(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool blendAlpha, bool inverted)
{
	if (!!srcBounds && !!mskBounds)
	{
		const os::bitmap& src2 = *static_cast<const os::bitmap*>(&src);
		const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
		graphics_context::draw_bitmap_with_bitmask(src2, srcBounds, msk2, mskBounds, dstBounds, blendAlpha, inverted, m_context);
		m_isOpaque = m_isOpaque && src.is_opaque();
		clear_cached();
	}
}

inline void bitmap::mask_out(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool inverted)
{
	clear_cached();
	const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
	graphics_context::mask_out(msk2, mskBounds, dstBounds, inverted, m_context);
}

inline void graphics_context::mask_out(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool inverted, const state_token& token)
{
	const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
	CGImageRef originalMaskImage;
	if (inverted)
	{
		msk2.cache_inverted_mask();
		originalMaskImage = msk2.m_invertedMask;
	}
	else
	{
		msk2.cache_image();
		originalMaskImage = msk2.m_image;
	}
	CGImageRef maskImage = originalMaskImage;
	CGImageRef croppedMaskImage = NULL;
	bool isMaskCropped = mskBounds.get_size() != msk2.get_size();
	if (isMaskCropped)
	{
		CGRect r = make_CGRect(mskBounds);
		r.origin.y = (msk2.m_actualHeight - r.origin.y) - r.size.height;
		croppedMaskImage = CGImageCreateWithImageInRect(originalMaskImage, r);
		maskImage = croppedMaskImage;
	}

	CGContextRef ctx = token.get_CGContext();
	CGRect dstRect = make_CGRect(dstBounds);
	CGContextClipToMask(ctx, dstRect, maskImage);
	CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 0.0);
	graphics_context::fill(dstBounds, kCGBlendModeCopy, token);

	if (isMaskCropped)
		CGImageRelease(croppedMaskImage);
}

inline void graphics_context::draw_bitmask(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha, const state_token& token)
{
	if (!!mskBounds && !!dstBounds)
	{
		const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
		bool isBackNeeded = (back != color::transparent || !blendBackAlpha);
		if (isBackNeeded)
		{
			msk2.cache_inverted_mask();
			CGImageRef originalMaskImage;
			originalMaskImage = msk2.m_invertedMask;
			CGImageRef maskImage = originalMaskImage;
			CGImageRef croppedMaskImage = NULL;
			bool isMaskCropped = mskBounds != msk2.get_size();
			if (isMaskCropped)
			{
				CGRect r = make_CGRect(mskBounds);
				r.origin.y = (msk2.m_actualHeight - r.origin.y) - r.size.height;
				croppedMaskImage = CGImageCreateWithImageInRect(originalMaskImage, r);
				maskImage = croppedMaskImage;
			}
			CGContextRef ctx = token.get_CGContext();
			CGRect dstRect = make_CGRect(dstBounds);
			CGContextClipToMask(ctx, dstRect, maskImage);
			graphics_context::fill(dstBounds, back, blendBackAlpha, token);
			if (isMaskCropped)
				CGImageRelease(croppedMaskImage);
		}
		bool isForeNeeded = (fore != color::transparent || !blendForeAlpha);
		if (isForeNeeded)
		{
			msk2.cache_image();
			CGImageRef originalMaskImage;
			originalMaskImage = msk2.m_image;
			CGImageRef maskImage = originalMaskImage;
			CGImageRef croppedMaskImage = NULL;
			bool isMaskCropped = mskBounds != msk2.get_size();
			if (isMaskCropped)
			{
				CGRect r = make_CGRect(mskBounds);
				r.origin.y = (msk2.m_actualHeight - r.origin.y) - r.size.height;
				croppedMaskImage = CGImageCreateWithImageInRect(originalMaskImage, r);
				maskImage = croppedMaskImage;
			}
			CGContextRef ctx = token.get_CGContext();
			CGContextResetClip(ctx);
			CGRect dstRect = make_CGRect(dstBounds);
			CGContextClipToMask(ctx, dstRect, maskImage);
			graphics_context::fill(dstBounds, fore, blendForeAlpha, token);
			if (isMaskCropped)
				CGImageRelease(croppedMaskImage);
		}
	}
}

inline void graphics_context::draw_bitmap_with_bitmask(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool blendAlpha, bool inverted, const state_token& token)
{
	if (!!srcBounds && !!dstBounds)
	{
		const os::bitmask& msk2 = *static_cast<const os::bitmask*>(&msk);
		CGImageRef originalMaskImage;
		if (inverted)
		{
			msk2.cache_image();
			originalMaskImage = msk2.m_image;
		}
		else
		{
			msk2.cache_inverted_mask();
			originalMaskImage = msk2.m_invertedMask;
		}
		CGImageRef maskImage = originalMaskImage;
		CGImageRef croppedMaskImage = NULL;
		bool isMaskCropped = mskBounds != msk2.get_size();
		if (isMaskCropped)
		{
			CGRect r = make_CGRect(mskBounds);
			r.origin.y = (msk2.m_actualHeight - r.origin.y) - r.size.height;
			croppedMaskImage = CGImageCreateWithImageInRect(originalMaskImage, r);
			maskImage = croppedMaskImage;
		}
		CGContextRef ctx = token.get_CGContext();
		CGRect dstRect = make_CGRect(dstBounds);
		CGContextClipToMask(ctx, dstRect, maskImage);
		CGBlendMode blendMode = blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy;
		const os::bitmap& src2 = *static_cast<const os::bitmap*>(&src);
		graphics_context::draw_image(src2, srcBounds, dstBounds, blendMode, token);
		if (isMaskCropped)
			CGImageRelease(croppedMaskImage);
	}
}

inline rcref<canvas::bitmask> graphics_context::create_bitmask(const canvas::size& sz, std::optional<bool> value)
{
	return rcnew(bitmask, sz, value);
}

inline rcref<canvas::bitmask> graphics_context::load_bitmask(const composite_string& location)
{
	return rcnew(bitmask, location);
}


}
}
}


#endif
