//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_GRAPHICS_CONTEXT
#define COGS_HEADER_OS_GFX_GRAPHICS_CONTEXT


#include "cogs/env.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/collections/macos_strings.hpp"


namespace cogs {
namespace gfx {
namespace os {


// MacOS canvas contextual to current drawing device.  Uses NSGraphicsContext mostly.

// To avoid multiply deriving from canvas, encapsulate a graphics_context by value.

class graphics_context : public canvas
{
public:
	class font : public canvas::font
	{
	public:
		NSFont* m_nsFont;
		bool	m_isUnderlined;

		font(NSFont* nsFont, bool isUnderlined)
			:	m_nsFont(nsFont),
				m_isUnderlined(isUnderlined)
		{
			[nsFont retain];
		}

		~font()
		{
			[m_nsFont release];
		}
	};

	// Drawing primatives
	virtual void fill(const canvas::bounds_t& r, const color& c = color::black, bool blendAlpha = true)
	{
		if (!!c.is_fully_transparent())	// fill with transparent means nothing
			return;

		NSRect dstRect2 = make_NSRect(r);
		
		NSColor* fillColor = make_NSColor(c);
		[fillColor set];

		NSRectFillUsingOperation(dstRect2, (!!blendAlpha && !c.is_opaque()) ? NSCompositeSourceOver : NSCompositeCopy);
	}

	virtual void invert(const canvas::bounds_t& r)
	{
		NSRect dstRect2 = make_NSRect(r);

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		[curContext setCompositingOperation: NSCompositeSourceOver];

		CGContextRef context = (CGContextRef)[curContext graphicsPort];
		CGContextSaveGState (context);
		CGContextSetBlendMode(context, kCGBlendModeDifference);
		CGContextSetRGBFillColor (context, 1.0, 1.0, 1.0, 1.0);
		CGContextFillRect (context, *(CGRect*)&dstRect2);
		CGContextRestoreGState (context);
	}

	virtual void draw_line(const canvas::point& startPt, const canvas::point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true )
	{
		NSPoint pt1 = NSMakePoint(0.5 + startPt.get_x(), 0.5 + startPt.get_y()); 
		NSPoint pt2 = NSMakePoint(0.5 + endPt.get_x(), 0.5 + endPt.get_y());

		NSBezierPath* bPath = [NSBezierPath bezierPath];
		[bPath moveToPoint: pt1];
		[bPath lineToPoint: pt2];
		[bPath setLineWidth: 0];

		NSColor* foreColor = make_NSColor(c);
		[foreColor setStroke];

		[bPath stroke];
	}

	virtual void scroll(const canvas::bounds_t& r, const canvas::point& pt = canvas::point(0,0)) = 0;

	// text and font primatives
	virtual rcref<canvas::font> load_font(const gfx::font& guiFont)
	{
		NSFont* nsFont;
		CGFloat fontSize = guiFont.get_point_size();
		size_t numFontNames = guiFont.get_num_font_names();
		for (size_t i = 0; ; i++)
		{
			if (i == numFontNames)	// give up, use default font
			{
				nsFont = [NSFont messageFontOfSize:fontSize];
				COGS_ASSERT(!!nsFont);
			}
			else
			{
				NSString* fontName = string_to_NSString(guiFont.get_font_names()[i]);
				nsFont = [NSFont fontWithName:fontName size:fontSize];
				[fontName release];
				COGS_ASSERT(!!nsFont);	// TBD <- if this is never hit, we don't get NULL on failure.  doh.
			}
			if (!!nsFont)
			{
				NSFontManager* nsFontManager = [NSFontManager sharedFontManager];
				if (guiFont.is_italic())
					[nsFontManager convertFont:nsFont toHaveTrait:NSItalicFontMask];
				if (guiFont.is_bold())
					[nsFontManager convertFont:nsFont toHaveTrait:NSBoldFontMask];
				return rcnew(font, nsFont, guiFont.is_underlined());
			}
		}
#error Needs to fall back to default font
		return 0;
	}

	//virtual canvas::size calc_text_bounds(const composite_string& s, const rcptr<canvas::font>& f)
	//{
	//	ptr<font> derivedFont = f.get_obj().static_cast_to<font>();
	//	NSFont* nsFont = derivedFont->m_nsFont;
	//
	//	NSString* str = string_to_NSString(s);
	//	NSMutableDictionary* attribs = [[NSMutableDictionary alloc] initWithCapacity:2];
	//	[attribs setObject:nsFont forKey:NSFontAttributeName]; 
	//	if (derivedFont->m_isUnderlined)
	//		[attribs setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName]; 
	//	NSRect r = [str boundingRectWithSize:NSMakeSize(16000.0, 16000.0) options:NSStringDrawingUsesLineFragmentOrigin attributes:attribs];	// NSStringDrawingUsesDeviceMetrics | 
	//	[attribs release];
	//	[str release];
	//	return canvas::size(r.size.width, r.size.height);
	//}
	
	virtual void draw_text(const composite_string& s, const bounds_t& r, const rcptr<canvas::font>& f, const color& c = color::black, bool blendAlpha = true)
	{
		ptr<font> derivedFont = f.get_obj().static_cast_to<font>();
		NSFont* nsFont = derivedFont->m_nsFont;

		NSString* str = string_to_NSString(s);
		NSMutableDictionary* attribs = [[NSMutableDictionary alloc] initWithCapacity:2];
		[attribs setObject:nsFont forKey:NSFontAttributeName];

		NSColor* nsColor = make_NSColor(c);
		[attribs setObject:nsColor forKey:NSForegroundColorAttributeName]; 

		if (derivedFont->m_isUnderlined)
			[attribs setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName]; 

		NSRect r2 = make_NSRect(r);
		[str drawWithRect:r2 options:NSStringDrawingUsesLineFragmentOrigin attributes:attribs];	// NSStringDrawingUsesDeviceMetrics | 
		[attribs release];
		[str release];
	}

	// Compositing images
	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true);
	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds);
	//virtual void composite_scaled_pixel_image(const canvas::pixel_image& src, const canvas::bounds_t& srcBounds, const canvas::bounds_t& dstBounds, bool blendAlpha = true);
	virtual void composite_pixel_mask(const canvas::pixel_mask& src, const canvas::bounds_t& srcBounds, const canvas::point& dstPt = canvas::point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true);
	virtual rcref<canvas::pixel_image_canvas> create_pixel_image_canvas(const canvas::size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi);
	virtual rcref<canvas::pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi);
	virtual rcref<canvas::pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi);

	virtual void save_clip()
	{
		[NSGraphicsContext saveGraphicsState];
	}

	virtual void restore_clip()
	{
		[NSGraphicsContext restoreGraphicsState];
	}

	virtual void clip_out(const canvas::bounds_t& r)
	{
		NSRect outerRect = make_NSRect(get_size());
		NSRect r2 = make_NSRect(r);
		NSBezierPath* clipPath = [NSBezierPath bezierPath];
		[clipPath setWindingRule:NSNonZeroWindingRule];
		[clipPath appendBezierPathWithRect:outerRect];
		[clipPath appendBezierPathWithRect:r2];
		[clipPath addClip];
	}

	virtual void clip_to(const canvas::bounds_t& r)
	{
		NSRectClip(make_NSRect(r));
	}

	virtual bool is_unclipped(const canvas::bounds_t& r) const
	{
		// TODO: Test to make sure this is same clip
		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		CGContextRef context = (CGContextRef)[curContext graphicsPort];
		CGRect clipBox = [context boundingBoxOfClipPath];
		NSRect testRect = make_NSRect(r);
		return CGRectIntersectsRect(clipBox, *(CGRect*)&testRect);
	}

	static NSRect make_NSRect(const canvas::bounds_t& r)
	{
		return NSMakeRect(r.calc_left().get_int<int>(), r.calc_top().get_int<int>(), r.calc_width().get_int<int>(), r.calc_height().get_int<int>());
	}

	static NSRect make_NSRect(const canvas::size& sz)
	{
		return NSMakeRect(0, 0, sz.get_width().get_int<int>(), sz.get_height().get_int<int>());
	}

	static NSRect make_NSRect(const canvas::point& pt, const canvas::size& sz)
	{
		return NSMakeRect(pt.get_x().get_int<int>(), pt.get_y().get_int<int>(), sz.get_width().get_int<int>(), sz.get_height().get_int<int>());
	}

	static NSPoint make_NSPoint(const canvas::point& pt)
	{
		return NSMakePoint(pt.get_x().get_int<int>(), pt.get_y().get_int<int>());
	}

	static NSSize make_NSSize(const canvas::size& sz)
	{
		return NSMakeSize(sz.get_width().get_int<int>(), sz.get_height().get_int<int>());
	}

	static NSColor* make_NSColor(const color& c)
	{
		float rf = c.get_red();
		float gf = c.get_green();
		float bf = c.get_blue();
		float af = c.get_alpha();

		return [NSColor colorWithCalibratedRed: rf / 0xFF
											green: gf / 0xFF
											 blue: bf / 0xFF
											alpha: af / 0xFF ];
	}

	static color from_NSColor(NSColor* nsColor)
	{
		CGFloat r;
		CGFloat g;
		CGFloat b;
		CGFloat a;
		[nsColor getRed:&r  green:&g blue:&b alpha:&a];
		return color((uint8_t)(r * (unsigned int)0x00FF), (uint8_t)(g * (unsigned int)0x00FF), (uint8_t)(b * (unsigned int)0x00FF), (uint8_t)(a * (unsigned int)0x00FF));
	}

	virtual size get_size() const = 0;
};


}
}
}


#include "cogs/os/gfx/nsimage.hpp"
#include "cogs/os/gfx/nsbitmap.hpp"


#endif
