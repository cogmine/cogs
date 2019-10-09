//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_GRAPHICS_CONTEXT
#define COGS_HEADER_OS_GFX_GRAPHICS_CONTEXT

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <CoreGraphics/CGContext.h>

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

class graphics_context
{
public:
	class state_token
	{
	private:
		NSGraphicsContext* m_nsGraphicsContext;
		CGContextRef m_cgContext;

	public:
		state_token()
			: m_nsGraphicsContext([NSGraphicsContext currentContext]),
			m_cgContext([m_nsGraphicsContext CGContext])
		{
			[NSGraphicsContext saveGraphicsState];
		}

		state_token(NSGraphicsContext* ctx)
			: m_nsGraphicsContext(ctx)
		{
			[NSGraphicsContext saveGraphicsState];
			[NSGraphicsContext setCurrentContext : ctx];
		}

		~state_token()
		{
			[NSGraphicsContext restoreGraphicsState];
		}

		NSGraphicsContext* get_NSGraphicsContext() const { return m_nsGraphicsContext; }
		const CGContextRef& get_CGContent() const { return m_cgContext; }
	};


	class font : public canvas::font
	{
	private:
		friend class graphics_context;

		__strong NSFont* m_nsFont;
		bool	m_isUnderlined;

	public:
		font(NSFont* nsFont, bool isUnderlined)
			:	m_nsFont(nsFont),
				m_isUnderlined(isUnderlined)
		{
		}

		NSFont* get_NSFont()
		{
			return m_nsFont;
		}

		virtual font::metrics get_metrics() const
		{
			font::metrics result;
			result.m_ascent = [m_nsFont ascender];
			result.m_descent = -[m_nsFont descender];
			result.m_spacing = result.m_ascent + result.m_descent + [m_nsFont leading];
			return result;
		}

		virtual canvas::size calc_text_bounds(const composite_string& s) const
		{
			__strong NSString* str = string_to_NSString(s);
			__strong NSMutableDictionary* attribs = [[NSMutableDictionary alloc] initWithCapacity:2];
			[attribs setObject:m_nsFont forKey:NSFontAttributeName]; 
			if (m_isUnderlined)
				[attribs setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName]; 
			NSRect r = [str boundingRectWithSize:NSMakeSize(16000.0, 16000.0) options:NSStringDrawingUsesLineFragmentOrigin attributes:attribs];	// NSStringDrawingUsesDeviceMetrics | 
			return canvas::size(r.size.width, r.size.height);
		}
	};

	class default_font : public gfx::font
	{
	public:
		default_font()
		{
			CGFloat fontSize = [NSFont systemFontSize];
			set_point_size(fontSize);

			NSFont *nsFont = [NSFont systemFontOfSize:fontSize];
			string str = NSString_to_string([nsFont fontName]);
			append_font_name(str);

			set_italic(false);
			set_bold(false);
			set_underlined(false);
			set_strike_out(false);
		}
	};

	// Drawing primatives
	static void fill(const canvas::bounds& b, const color& c = color::black, bool blendAlpha = true, const state_token& token = state_token())
	{
		if (!blendAlpha || !c.is_fully_transparent())
		{
			auto& ctx = token.get_CGContent();
			CGColorRef c2 = make_CGColorRef(c);
			CGContextSetFillColorWithColor(ctx, c2);
			CGContextSetBlendMode(ctx, blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy);
			CGContextFillRect(ctx, make_CGRect(b));
		}
	}

	static void invert(const canvas::bounds& b, const state_token& token = state_token())
	{
		auto& ctx = token.get_CGContent();
		CGContextSetBlendMode(ctx, kCGBlendModeDifference);
		CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
		CGContextFillRect(ctx, make_CGRect(b));
	}

	static void draw_line(const canvas::point& startPt, const canvas::point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true, const state_token& token = state_token())
	{
		auto& ctx = token.get_CGContent();
		CGColorRef c2 = make_CGColorRef(c);
		CGContextSetStrokeColorWithColor(ctx, c2);
		CGContextSetBlendMode(ctx, blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy); 
		CGContextSetLineWidth(ctx, width);
		CGContextMoveToPoint(ctx, startPt.get_x(), startPt.get_y());
		CGContextAddLineToPoint(ctx, endPt.get_x(), endPt.get_y());
		CGContextStrokePath(ctx);
	}

	// text and font primatives
	static rcref<canvas::font> load_font(const gfx::font& guiFont)
	{
		__strong NSFont* nsFont;
		CGFloat fontSize;
		if (!guiFont.get_point_size())
			fontSize = [NSFont systemFontSize];
		else
			fontSize = guiFont.get_point_size();

		size_t numFontNames = guiFont.get_num_font_names();
		size_t i = 0;
		for (; i < numFontNames; i++)
		{
			__strong NSString* fontName = string_to_NSString(guiFont.get_font_names()[i]);
			nsFont = [NSFont fontWithName:fontName size:fontSize];
			if (!!nsFont)
				break;
		}

		if (i == numFontNames)
		{
			nsFont = [NSFont systemFontOfSize:fontSize];
			COGS_ASSERT(!!nsFont);
		}

		NSFontManager* nsFontManager = [NSFontManager sharedFontManager];
		if (guiFont.is_italic())
			[nsFontManager convertFont:nsFont toHaveTrait:NSItalicFontMask];
		if (guiFont.is_bold())
			[nsFontManager convertFont:nsFont toHaveTrait:NSBoldFontMask];
		return rcnew(font, nsFont, guiFont.is_underlined());
	}

	static gfx::font get_default_font()
	{
		return *singleton<default_font>::get();
	}
	
	static void draw_text(const composite_string& s, const canvas::bounds& b, const rcptr<canvas::font>& f, const color& c = color::black)
	{
		font* derivedFont = f.template static_cast_to<font>().get_ptr();
		NSFont* nsFont = derivedFont->get_NSFont();

		__strong NSString* str = string_to_NSString(s);
		__strong NSMutableDictionary* attribs = [[NSMutableDictionary alloc] initWithCapacity:2];
		[attribs setObject:nsFont forKey:NSFontAttributeName];

		__strong NSColor* nsColor = make_NSColor(c);
		[attribs setObject:nsColor forKey:NSForegroundColorAttributeName]; 

		if (derivedFont->m_isUnderlined)
			[attribs setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName]; 

		NSRect r2 = make_NSRect(b);
		[str drawWithRect:r2 options:NSStringDrawingUsesLineFragmentOrigin attributes:attribs];
	}

	// Compositing images
	static void draw_bitmap(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, bool blendAlpha = true);
	static void draw_bitmask(const canvas::bitmask& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true);
	static void mask_out(const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool inverted = false);
	static void draw_bitmap_with_bitmask(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bitmask& msk, const canvas::bounds& mskBounds, const canvas::bounds& dstBounds, bool blendAlpha = true, bool inverted = false);

	static rcref<canvas::bitmap> create_bitmap(const canvas::size& sz, std::optional<color> fillColor = std::nullopt);
	static rcref<canvas::bitmap> load_bitmap(const composite_string& location);
	static rcref<canvas::bitmask> create_bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt);
	static rcref<canvas::bitmask> load_bitmask(const composite_string& location);

	static void save_clip()
	{
		[NSGraphicsContext saveGraphicsState];
	}

	static void restore_clip()
	{
		[NSGraphicsContext restoreGraphicsState];
	}

	static void clip_out(const canvas::bounds& b, const canvas::size& cellSize)
	{
		NSRect outerRect = make_NSRect(cellSize);
		NSRect r2 = make_NSRect(b);
		NSBezierPath* clipPath = [NSBezierPath bezierPath];
		[clipPath setWindingRule:NSNonZeroWindingRule];
		[clipPath appendBezierPathWithRect:outerRect];
		[clipPath appendBezierPathWithRect:r2];
		[clipPath addClip];
	}

	static void clip_to(const canvas::bounds& b)
	{
		NSRectClip(make_NSRect(b));
	}

	static bool is_unclipped(const canvas::bounds& b)
	{
		// TODO: Test to make sure this is same clip
		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		CGContextRef context = [curContext CGContext];
		CGRect clipBox = CGContextGetClipBoundingBox(context);
		NSRect testRect = make_NSRect(b);
		return CGRectIntersectsRect(clipBox, *(CGRect*)&testRect);
	}

	static NSRect make_NSRect(const canvas::bounds& b)
	{
		return NSMakeRect(b.calc_left(), b.calc_top(), b.calc_width(), b.calc_height());
	}

	static NSRect make_NSRect(const canvas::size& sz)
	{
		return NSMakeRect(0, 0, sz.get_width(), sz.get_height());
	}

	static NSRect make_NSRect(const canvas::point& pt, const canvas::size& sz)
	{
		return NSMakeRect(pt.get_x(), pt.get_y(), sz.get_width(), sz.get_height());
	}

	static NSPoint make_NSPoint(const canvas::point& pt)
	{
		return NSMakePoint(pt.get_x(), pt.get_y());
	}

	static NSSize make_NSSize(const canvas::size& sz)
	{
		return NSMakeSize(sz.get_width(), sz.get_height());
	}

	static NSColor* make_NSColor(const color& c)
	{
		CGFloat rf = c.get_red();
		CGFloat gf = c.get_green();
		CGFloat bf = c.get_blue();
		CGFloat af = c.get_alpha();
		return [NSColor colorWithCalibratedRed:(rf / 0xFF) green:(gf / 0xFF) blue:(bf / 0xFF) alpha: (af / 0xFF)];
	}

	static color from_NSColor(NSColor* nsColor)
	{
		CGFloat r;
		CGFloat g;
		CGFloat b;
		CGFloat a;
		[nsColor getRed:&r green:&g blue:&b alpha:&a];
		return color((uint8_t)(r * (unsigned int)0x00FF), (uint8_t)(g * (unsigned int)0x00FF), (uint8_t)(b * (unsigned int)0x00FF), (uint8_t)(a * (unsigned int)0x00FF));
	}

	static CGRect make_CGRect(const canvas::bounds& b)
	{
		return CGRectMake(b.calc_left(), b.calc_top(), b.calc_width(), b.calc_height());
	}

	static CGRect make_CGRect(const canvas::size& sz)
	{
		return CGRectMake(0, 0, sz.get_width(), sz.get_height());
	}

	static CGRect make_CGRect(const canvas::point& pt, const canvas::size& sz)
	{
		return CGRectMake(pt.get_x(), pt.get_y(), sz.get_width(), sz.get_height());
	}

	static CGColorRef make_CGColorRef(const color& c)
	{
		CGFloat c2[4] = {
			((CGFloat)c.get_red() / 0xFF),
			((CGFloat)c.get_green() / 0xFF),
			((CGFloat)c.get_blue() / 0xFF),
			((CGFloat)c.get_alpha() / 0xFF)
		};
		return CGColorCreate(CGColorSpaceCreateDeviceRGB(), c2);
	}


	static canvas::point make_point(const NSPoint& pt)
	{
		return canvas::point(pt.x, pt.y);
	}

	static canvas::point make_point(const NSRect& r)
	{
		return canvas::point(r.origin.x, r.origin.y);
	}

	static canvas::size make_size(const NSSize& sz)
	{
		return canvas::size(sz.width, sz.height);
	}

	static canvas::size make_size(const NSRect& r)
	{
		return canvas::size(r.size.width, r.size.height);
	}


	static canvas::bounds make_bounds(const NSPoint& pt, const NSSize& sz)
	{
		return canvas::bounds(make_point(pt), make_size(sz));
	}

	static canvas::bounds make_bounds(const NSRect& b)
	{
		return canvas::bounds(make_point(b.origin), make_size(b.size));
	}

};


}
}
}


#include "cogs/os/gfx/bitmap.hpp"
#include "cogs/os/gfx/bitmask.hpp"


#endif
