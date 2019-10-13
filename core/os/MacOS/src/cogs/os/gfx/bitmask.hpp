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


class bitmask : public canvas::bitmask, public graphics_context
{
protected:
	__strong NSBitmapImageRep* m_imageRep;
	//__strong NSGraphicsContext* m_graphicsContext;
	canvas::size m_size;
	mutable CGImageRef m_mask;
	mutable CGImageRef m_invertedMask;
	mutable bool m_maskNeedsUpdate = true;

	CGContextRef m_cgContextRef;

private:
	class state_token
	{
	private:
		CGContextRef m_cgContext;

	public:
		state_token(CGContextRef ctx)
			: m_cgContext(ctx)
		{
			CGContextSaveGState(ctx);
		}

		~state_token()
		{
			CGContextRestoreGState(m_cgContext);
		}

		const CGContextRef& get_CGContent() const { return m_cgContext; }
	};

	void update_mask() const
	{
		if (m_maskNeedsUpdate)
		{
			CGImageRef img = [m_imageRep CGImage];
			size_t w = CGImageGetWidth(img);
			size_t h = CGImageGetHeight(img);
			size_t bitsPerComponent = CGImageGetBitsPerComponent(img);
			size_t bitsPerPixel = CGImageGetBitsPerPixel(img);
			size_t bytesPerRow = CGImageGetBytesPerRow(img);
			CGDataProviderRef provider = CGImageGetDataProvider(img);
			CGFloat invertDecodeArray[] = { 1.0, 0.0, 1.0, 0.0, 1.0, 0.0 };

			m_mask = CGImageMaskCreate(w, h, bitsPerComponent, bitsPerPixel, bytesPerRow, provider, NULL, false);
			m_invertedMask = CGImageMaskCreate(w, h, bitsPerComponent, bitsPerPixel, bytesPerRow, provider, invertDecodeArray, false);
			m_maskNeedsUpdate = false;
		}
	}

	CGBlendMode create_op(fill_mode fillMode)
	{
		switch (fillMode)
		{
		case fill_mode::clear_mode:
			return kCGBlendModeClear;
		case fill_mode::invert_mode:
			return kCGBlendModeSourceOut;
		case fill_mode::set_mode:
		default:
			return kCGBlendModeCopy;
		};
	}

	static CGBlendMode create_rop(composite_mode compositeMode)
	{
		switch (compositeMode)
		{
		case composite_mode::and_mode:
			return kCGBlendModeMultiply;
		case composite_mode::or_mode:
			return kCGBlendModeNormal;
		case composite_mode::xor_mode:
			return kCGBlendModeXOR;
		case composite_mode::clear_mode:
			return kCGBlendModeClear;
		case composite_mode::copy_inverted_mode:
			return kCGBlendModeSourceOut;
		case composite_mode::copy_mode:
		default:
			return kCGBlendModeCopy;
		}
	}

public:
	// creates bitmask from monochome image format
	bitmask(const composite_string& name, bool resource = false) // !resource implies filename
	{
		__strong NSString* n = string_to_NSString(name);
		__strong NSString* imageName;
		if (resource)
			imageName = [[NSBundle mainBundle] pathForResource:n ofType:@"bmp"]; 
		else
			imageName = n;

		m_imageRep = [NSBitmapImageRep imageRepWithData:[NSData dataWithContentsOfFile:imageName]];

		//m_graphicsContext = [NSGraphicsContext graphicsContextWithBitmapImageRep: m_imageRep];

		m_size = canvas::size([m_imageRep pixelsWide], [m_imageRep pixelsHigh]);
	}

	bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt)
		: m_size(sz)
	{
		m_cgContextRef = CGBitmapContextCreate(NULL, std::lround(sz.get_width()), std::lround(sz.get_height()), 8, 0, CGColorSpaceCreateDeviceGray(), 0);

		//m_imageRep = [[NSBitmapImageRep alloc]
		//	initWithBitmapDataPlanes: NULL
		//	pixelsWide: std::lround(sz.get_width())
		//	pixelsHigh: std::lround(sz.get_height())
		//	bitsPerSample: 8
		//	samplesPerPixel: 1
		//	hasAlpha: NO
		//	isPlanar: NO
		//	colorSpaceName: NSCalibratedWhiteColorSpace
		//	bytesPerRow: 0
		//	bitsPerPixel: 0];

		//m_graphicsContext = [NSGraphicsContext graphicsContextWithBitmapImageRep: m_imageRep];

		if (value.has_value())
		{
			// Already zero initialized?
			if (*value)
				fill(sz, fill_mode::set_mode); 
		}
	}

	~bitmask()
	{
		CGImageRelease(m_mask);
	}

	CGImageRef get_mask() const
	{
		update_mask();
		return m_mask;
	}

	CGImageRef get_inverted_mask() const
	{
		update_mask();
		return m_invertedMask;
	}

	virtual canvas::size get_size() const { return m_size; }

	virtual void fill(const canvas::bounds& b, fill_mode fillMode = fill_mode::set_mode)
	{
		state_token token(m_cgContextRef);
		auto ctx = token.get_CGContent();
		CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
		CGContextSetBlendMode(ctx, create_op(fillMode));
		CGContextFillRect(ctx, graphics_context::make_CGRect(b));
		m_maskNeedsUpdate = true;
	}

	virtual void invert(const canvas::bounds& b)
	{
		state_token token(m_cgContextRef);
		graphics_context::invert(b);
		//graphics_context::invert(b, token);
		m_maskNeedsUpdate = true;
	}

	virtual void draw_line(const canvas::point& startPt, const canvas::point& endPt, double width = 1, fill_mode fillMode = fill_mode::set_mode)
	{
		state_token token(m_cgContextRef);
		auto& ctx = token.get_CGContent();
		CGContextSetRGBStrokeColor(ctx, 1.0, 1.0, 1.0, 1.0);
		CGContextSetBlendMode(ctx, create_op(fillMode));
		CGContextSetLineWidth(ctx, width);
		CGContextMoveToPoint(ctx, startPt.get_x(), startPt.get_y());
		CGContextAddLineToPoint(ctx, endPt.get_x(), endPt.get_y());
		CGContextStrokePath(ctx);
		m_maskNeedsUpdate = true;
	}

	virtual rcref<canvas::font> load_font(const gfx::font& guiFont) { return graphics_context::load_font(guiFont); }

	virtual gfx::font get_default_font() const
	{
		return graphics_context::get_default_font();
	}

	virtual void draw_text(const composite_string& s, const canvas::bounds& b, const rcptr<canvas::font>& f = 0, bool value = true)
	{
		state_token token(m_cgContextRef);
		graphics_context::draw_text(s, b, f, value ? color::white : color::black);
		m_maskNeedsUpdate = true;
	}

	virtual void draw_bitmask(const canvas::bitmask& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode)
	{
		const bitmask* src2 = static_cast<const bitmask*>(&src);
		CGRect srcRect = graphics_context::make_CGRect(srcBounds);
		CGRect dstRect = graphics_context::make_CGRect(dstBounds.get_size());

		state_token token(m_cgContextRef);
		auto& ctx = token.get_CGContent();

		CGContextTranslateCTM(ctx, dstBounds.get_x(), dstBounds.get_y() + dstBounds.get_height());
		CGContextScaleCTM(ctx, 1.0, -1.0);


		CGImageRef msk = src2->get_mask();
		CGImageRef adjustedMask = CGImageCreateWithImageInRect(msk, srcRect);
		CGContextSetBlendMode(ctx, create_rop(compositeMode));

		CGContextDrawImage(ctx, dstRect, adjustedMask);

		CGImageRelease(adjustedMask);
		m_maskNeedsUpdate = true;
	}

	virtual rcref<canvas::bitmask> create_bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt)
	{
		return graphics_context::create_bitmask(sz, value);
	}

	virtual void save_clip()
	{
		state_token token(m_cgContextRef);
		graphics_context::save_clip();
	}

	virtual void restore_clip()
	{
		state_token token(m_cgContextRef);
		graphics_context::restore_clip();
	}

	virtual void clip_out(const canvas::bounds& b)
	{
		state_token token(m_cgContextRef);
		graphics_context::clip_out(b, m_size);
	}

	virtual void clip_to(const canvas::bounds& b)
	{
		state_token token(m_cgContextRef);
		graphics_context::clip_to(b);
	}

	virtual bool is_unclipped(const canvas::bounds& b) const
	{
		state_token token(m_cgContextRef);
		return graphics_context::is_unclipped(b);
	}
};


inline void graphics_context::draw_bitmask(const canvas::bitmask& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	const bitmask* src2 = static_cast<const bitmask*>(&src);
	CGRect srcRect = graphics_context::make_CGRect(srcBounds);
	CGRect dstRect = graphics_context::make_CGRect(dstBounds.get_size());

	state_token token;
	auto& ctx = token.get_CGContent();

	CGContextTranslateCTM(ctx, dstBounds.get_x(), dstBounds.get_y() + dstBounds.get_height());
	CGContextScaleCTM(ctx, 1.0, -1.0);

	if (!fore.is_fully_transparent() || !blendForeAlpha)
	{
		CGImageRef msk = src2->get_mask();
		CGImageRef adjustedMask = CGImageCreateWithImageInRect(msk, srcRect);
		CGColorRef c = graphics_context::make_CGColorRef(fore);
		CGContextSetFillColorWithColor(ctx, c);
		CGContextSetBlendMode(ctx, blendForeAlpha ? kCGBlendModeNormal : kCGBlendModeCopy);
		CGContextDrawImage(ctx, dstRect, adjustedMask);
		CGImageRelease(adjustedMask);
	}

	if (!back.is_fully_transparent() || !blendBackAlpha)
	{
		CGImageRef msk = src2->get_inverted_mask();
		CGImageRef adjustedMask = CGImageCreateWithImageInRect(msk, srcRect);
		CGColorRef c = graphics_context::make_CGColorRef(back);
		CGContextSetFillColorWithColor(ctx, c);
		CGContextSetBlendMode(ctx, blendBackAlpha ? kCGBlendModeNormal : kCGBlendModeCopy);
		CGContextDrawImage(ctx, dstRect, adjustedMask);
		CGImageRelease(adjustedMask);
	}
}


inline void graphics_context::mask_out(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool inverted)
{
	const bitmask* msk2 = static_cast<const bitmask*>(&msk);
	CGRect mskRect = graphics_context::make_CGRect(mskBounds);
	CGRect dstRect = graphics_context::make_CGRect(dstBounds.get_size());

	state_token token;
	auto& ctx = token.get_CGContent();

	CGContextTranslateCTM(ctx, dstBounds.get_x(), dstBounds.get_y() + dstBounds.get_height());
	CGContextScaleCTM(ctx, 1.0, -1.0);

	CGImageRef msk3 = inverted ? msk2->get_inverted_mask() : msk2->get_mask();

	CGImageRef adjustedMask = CGImageCreateWithImageInRect(msk3, mskRect);

	CGFloat c[4] = { 0.0, 0.0, 0.0, 0.0 };
	CGColorRef c2 = CGColorCreate(CGColorSpaceCreateDeviceRGB(), c);
	CGContextSetFillColorWithColor(ctx, c2);
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);

	CGContextDrawImage(ctx, dstRect, adjustedMask);
	//CGContextClipToMask(ctx, dstRect, adjustedMask);
	//CGContextFillRect(ctx, dstRect);

	CGImageRelease(adjustedMask);
}

inline void graphics_context::draw_bitmap_with_bitmask(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool blendAlpha, bool inverted)
{
	const bitmap* src2 = static_cast<const bitmap*>(&src);
	const bitmask* msk2 = static_cast<const bitmask*>(&msk);

	//CGRect srcRect = graphics_context::make_CGRect(srcBounds);
	CGRect mskRect = graphics_context::make_CGRect(mskBounds);
	CGRect dstRect = graphics_context::make_CGRect(dstBounds.get_size());

	state_token token;
	auto& ctx = token.get_CGContent();

	CGContextTranslateCTM(ctx, dstBounds.get_x(), dstBounds.get_y() + dstBounds.get_height());
	CGContextScaleCTM(ctx, 1.0, -1.0);

	CGImageRef msk3 = inverted ? msk2->get_inverted_mask() : msk2->get_mask();

	CGImageRef adjustedMask = CGImageCreateWithImageInRect(msk3, mskRect);
	CGContextClipToMask(ctx, dstRect, adjustedMask);
	CGContextSetBlendMode(ctx, blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy);

	NSRect srcRect2 = NSMakeRect(srcBounds.get_x(), srcBounds.get_y(), srcBounds.get_width(), srcBounds.get_height());
	CGImageRef adjustedSrc = [src2->get_NSImage() CGImageForProposedRect: &srcRect2 context: token.get_NSGraphicsContext() hints: nil];

	//CGImageRef adjustedSrc = CGImageCreateWithImageInRect(src2->get_CGImageRef(), srcRect);
	CGContextDrawImage(ctx, dstRect, adjustedSrc);

	CGImageRelease(adjustedSrc);
	CGImageRelease(adjustedMask);
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
