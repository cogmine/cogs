//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
namespace gfx {
namespace os {


class bitmap : public canvas::bitmap, public graphics_context
{
private:
	__strong NSImage* m_image;
	bool m_isOpaque = true;
	size m_logicalSize;
	size m_actualSize;

	void update_opacity(const canvas::bounds& dstBounds, bool isSourceOpaque, bool blendAlpha)
	{
		if (m_isOpaque != isSourceOpaque)
		{
			if (!isSourceOpaque)
				m_isOpaque = blendAlpha;
			else
				m_isOpaque = dstBounds.get_x() <= 0 && (dstBounds.get_width() - dstBounds.get_x()) >= m_logicalSize.get_width()
				&& dstBounds.get_y() <= 0 && (dstBounds.get_height() - dstBounds.get_y()) >= m_logicalSize.get_height();
		}
	}

	class lock_scope
	{
	private:
		NSImage* m_image;

	public:
		lock_scope(NSImage* image)
			: m_image(image)
		{
			[m_image lockFocus];
		}

		~lock_scope()
		{
			[m_image unlockFocus];
		}
	};

public:
	bitmap(const composite_string& name, bool resource = true) // !resource implies filename
	{
		__strong NSString* name2 = string_to_NSString(name);
		if (resource)
			m_image = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:name2 ofType:@"bmp"]];
		else
			m_image = [[NSImage alloc] initWithContentsOfFile:name2];

		// If any representations have an alpha channel?
		__strong NSArray<NSImageRep*>* reps = [m_image representations];
		auto n = [reps count];
		COGS_ASSERT(n > 0);
		while (n != 0)
		{
			--n;
			NSImageRep* rep = [reps objectAtIndex: n];
			if (![rep isOpaque])
			{
				m_isOpaque = false;
				break;
			}
		}

		NSSize sz = [m_image size];
		m_actualSize = m_logicalSize = size(sz.width, sz.height);
	}

	bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
		: m_actualSize(sz),
		m_logicalSize(sz)
	{
		NSSize nsSize = graphics_context::make_NSSize(sz);
		m_image = [[NSImage alloc] initWithSize:nsSize];
		if (!fillColor.has_value() || !fillColor->is_fully_transparent())
			m_isOpaque = false;
		else
		{
			fill(sz, *fillColor, false);
			m_isOpaque = fillColor->is_opaque();
		}
	}

	NSImage* get_NSImage() const { return m_image; }

	virtual size get_size() const { return m_logicalSize; }

	virtual bool is_opaque() const { return m_isOpaque; }

	void set_size(const size& sz)
	{
		NSSize nsSize = graphics_context::make_NSSize(sz);
		[m_image setSize: nsSize];
	}

	virtual void fill(const bounds& b, const color& c = color::black, bool blendAlpha = true)
	{
		lock_scope token(m_image);
		graphics_context::fill(b, c, blendAlpha);
	}

	virtual void invert(const bounds& b)
	{
		lock_scope token(m_image);
		graphics_context::invert(b);
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
	{
		lock_scope token(m_image);
		graphics_context::draw_line(startPt, endPt, width, c, blendAlpha);
	}

	virtual rcref<canvas::font> load_font(const gfx::font& guiFont) { return graphics_context::load_font(guiFont); }

	virtual gfx::font get_default_font() const
	{
		return graphics_context::get_default_font();
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<canvas::font>& f = 0, const color& c = color::black)
	{
		lock_scope token(m_image);
		graphics_context::draw_text(s, b, f, c);
	}

	virtual void draw_bitmap(const canvas::bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
	{
		const bitmap* srcImage = static_cast<const bitmap*>(&src);
		lock_scope token(m_image);
		graphics_context::draw_bitmap(src, srcBounds, dstBounds, blendAlpha);
		update_opacity(dstBounds, srcImage->is_opaque(), blendAlpha);
	}

	virtual void draw_bitmask(const canvas::bitmask& src, const bounds& srcBounds, const bounds& dstBounds, const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		uint8_t foreAlpha = fore.get_alpha();
		uint8_t backAlpha = back.get_alpha();
		bool areBothFullyTransparent = blendForeAlpha && blendBackAlpha && !foreAlpha && !backAlpha;
		if (!areBothFullyTransparent)
		{
			if (foreAlpha == 0xFF)
				blendForeAlpha = true;
			if (backAlpha == 0xFF)
				blendBackAlpha = true;

			lock_scope token(m_image);
			graphics_context::draw_bitmask(src, srcBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha);

			if ((!blendForeAlpha && !fore.is_opaque()) || (!blendBackAlpha && !back.is_opaque()))
				m_isOpaque = false;
			else if (!blendForeAlpha && !blendBackAlpha && fore.is_opaque() && back.is_opaque())
				update_opacity(dstBounds, true, false);
		}
	}

	virtual void mask_out(const canvas::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false)
	{
		lock_scope token(m_image);
		graphics_context::mask_out(msk, mskBounds, dstBounds, inverted);
	}


	virtual void draw_bitmap_with_bitmask(const canvas::bitmap& src, const bounds& srcBounds, const canvas::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false)
	{
		lock_scope token(m_image);
		graphics_context::draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted);
		m_isOpaque = m_isOpaque && src.is_opaque();
	}

	virtual rcref<canvas::bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return graphics_context::create_bitmap(sz, fillColor);
	}

	virtual rcref<canvas::bitmap> load_bitmap(const composite_string& location)
	{
		return graphics_context::load_bitmap(location);
	}

	virtual rcref<canvas::bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt)
	{
		return graphics_context::create_bitmask(sz, value);
	}

	virtual rcref<canvas::bitmask> load_bitmask(const composite_string& location)
	{
		return graphics_context::load_bitmask(location);
	}

	virtual void save_clip()
	{
		lock_scope token(m_image);
		graphics_context::save_clip();
	}

	virtual void restore_clip()
	{
		lock_scope token(m_image);
		graphics_context::restore_clip();
	}

	virtual void clip_out(const bounds& b)
	{
		lock_scope token(m_image);
		graphics_context::clip_out(b, m_logicalSize);
	}

	virtual void clip_to(const bounds& b)
	{
		lock_scope token(m_image);
		graphics_context::clip_to(b);
	}

	virtual bool is_unclipped(const bounds& b) const
	{
		lock_scope token(m_image);
		return graphics_context::is_unclipped(b);
	}

	virtual void set_size(const size& newSize, const size& growPadding = size(100, 100), bool trimIfShrinking = false)
	{
		if (newSize != m_logicalSize)
		{
			size newActualSize = newSize + growPadding;
			bool widthOverflow = newSize.get_width() > m_actualSize.get_width();
			bool heightOverflow = newSize.get_height() > m_actualSize.get_height();
			bool reshape = (widthOverflow || heightOverflow);
			if (!reshape && trimIfShrinking)
			{
				bool widthDecrease = newActualSize.get_width() < m_actualSize.get_width();
				bool heightDecrease = newActualSize.get_height() < m_actualSize.get_height();
				reshape = (widthDecrease || heightDecrease);
			}
			if (reshape)
			{
				if (!trimIfShrinking)
				{
					if (newActualSize.get_width() < m_actualSize.get_width())
						newActualSize.get_width() = m_actualSize.get_width();
					if (newActualSize.get_height() < m_actualSize.get_height())
						newActualSize.get_height() = m_actualSize.get_height();
				}
				NSSize nsSize = graphics_context::make_NSSize(newActualSize);
				NSRect nsRect = make_NSRect(newActualSize);
				__strong NSImage* newImage = [[NSImage alloc] initWithSize: nsSize];
				lock_scope token(newImage);
				[m_image drawInRect: nsRect
					fromRect: nsRect
					operation: NSCompositingOperationCopy
					fraction: 1.0
					respectFlipped: YES
					hints: nil];
				m_image = newImage;
				m_actualSize = newActualSize;
			}
			lock_scope token(m_image);
			bool widthIncreased = newSize.get_width() > m_logicalSize.get_width();
			if (widthIncreased)
			{
				double widthDifference = newSize.get_width() - m_logicalSize.get_width();
				graphics_context::fill(bounds(m_logicalSize.get_width(), 0, widthDifference, newSize.get_height()), m_isOpaque ? color::transparent : color::black, false);
			}
			if (newSize.get_height() > m_logicalSize.get_height())
			{
				double heightDifference = newSize.get_height() - m_logicalSize.get_height();
				double width;
				if (widthIncreased)
					width = m_logicalSize.get_width();
				else
					width = newSize.get_width();
				graphics_context::fill(bounds(0, m_logicalSize.get_height(), width, heightDifference), m_isOpaque ? color::transparent : color::black, false);
			}
			m_logicalSize = newSize;
		}
	}
};

inline rcref<canvas::bitmap> graphics_context::create_bitmap(const canvas::size& sz, std::optional<color> fillColor)
{
	return rcnew(bitmap, sz, fillColor);
}

inline rcref<canvas::bitmap> graphics_context::load_bitmap(const composite_string& location)
{
	return rcnew(bitmap, location);
}

inline void graphics_context::draw_bitmap(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, bool blendAlpha)
{
	if (!!srcBounds && !!dstBounds)
	{
		if (src.is_opaque())
			blendAlpha = false;

		const bitmap* src2 = static_cast<const bitmap*>(&src);
		NSRect srcRect2 = graphics_context::make_NSRect(srcBounds);
		NSRect dstRect2 = graphics_context::make_NSRect(dstBounds);

		state_token token;
		NSGraphicsContext* curContext = token.get_NSGraphicsContext();

		if (blendAlpha)
			[curContext setCompositingOperation: NSCompositingOperationSourceOver];
		else
			[curContext setCompositingOperation: NSCompositingOperationCopy];

		NSImage* nsImage = src2->get_NSImage();
		[nsImage drawInRect: dstRect2
			fromRect: srcRect2
			operation: NSCompositingOperationCopy
			fraction: 1.0
			respectFlipped: YES
			hints: nil];
	}
}


}
}
}


#endif
