//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
#include "cogs/collections/set.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/collections/macos_strings.hpp"


namespace cogs {
namespace os {


static NSRect make_NSRect(const gfx::bounds& b)
{
	return NSMakeRect(b.calc_left(), b.calc_top(), b.calc_width(), b.calc_height());
}

static NSRect make_NSRect(const gfx::size& sz)
{
	return NSMakeRect(0, 0, sz.get_width(), sz.get_height());
}

static NSRect make_NSRect(const gfx::point& pt, const gfx::size& sz)
{
	return NSMakeRect(pt.get_x(), pt.get_y(), sz.get_width(), sz.get_height());
}

static NSPoint make_NSPoint(const gfx::point& pt)
{
	return NSMakePoint(pt.get_x(), pt.get_y());
}

static NSSize make_NSSize(const gfx::size& sz)
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

static CGRect make_CGRect(const gfx::bounds& b)
{
	return CGRectMake(b.calc_left(), b.calc_top(), b.calc_width(), b.calc_height());
}

static CGRect make_CGRect(const gfx::size& sz)
{
	return CGRectMake(0, 0, sz.get_width(), sz.get_height());
}

static CGRect make_CGRect(const gfx::point& pt, const gfx::size& sz)
{
	return CGRectMake(pt.get_x(), pt.get_y(), sz.get_width(), sz.get_height());
}

static gfx::point make_point(const NSPoint& pt)
{
	return gfx::point(pt.x, pt.y);
}

static gfx::point make_point(const NSRect& r)
{
	return gfx::point(r.origin.x, r.origin.y);
}

static gfx::size make_size(const NSSize& sz)
{
	return gfx::size(sz.width, sz.height);
}

static gfx::size make_size(const NSRect& r)
{
	return gfx::size(r.size.width, r.size.height);
}

static gfx::bounds make_bounds(const NSPoint& pt, const NSSize& sz)
{
	return gfx::bounds(make_point(pt), make_size(sz));
}

static gfx::bounds make_bounds(const NSRect& b)
{
	return gfx::bounds(make_point(b.origin), make_size(b.size));
}


class bitmap_graphics_context;
class bitmap;
class bitmask;


class graphics_context
{
public:
	class state_token
	{
	private:
		CGContextRef m_cgContext;

	public:
		state_token()
			: m_cgContext([[NSGraphicsContext currentContext] CGContext])
		{
			[NSGraphicsContext saveGraphicsState];
			CGContextSaveGState(m_cgContext);
		}

		state_token(const CGContextRef& ctx)
			: m_cgContext(ctx)
		{
			[NSGraphicsContext saveGraphicsState];
			CGContextSaveGState(m_cgContext);
		}

		~state_token()
		{
			CGContextRestoreGState(m_cgContext);
			[NSGraphicsContext restoreGraphicsState];
		}

		const CGContextRef& get_CGContext() const { return m_cgContext; }
	};

	class font : public gfx::font
	{
	private:
		CTFontRef m_font;
		CFDictionaryRef m_attributes;
		CGContextRef m_context;
		font::metrics m_metrics;

		// Uses a nonvolatile_set as creation is synchronized and is then read-only
		class fontlist : public nonvolatile_set<composite_string, true, case_insensitive_comparator<composite_string> >
		{
		protected:
			fontlist()
			{
				// Load fonts.  (Fonts added or removed will not be observed until relaunch)
				__strong NSArray* fonts = [[NSFontManager sharedFontManager]availableFontFamilies];
				size_t numFonts = [fonts count];
				for (size_t i = 0; i < numFonts; ++i)
					insert_unique(NSString_to_string(fonts[i]));
			}
		};

	public:
		font(const gfx::font_parameters_list& f, const CGContextRef& ctx)
			: m_context(ctx)
		{
			CGContextRetain(ctx);
			gfx::font_parameters default_font_params = get_default_font();

			auto update = [&](const gfx::font_parameters& fp)
			{
				// Get traits as CTFontSymbolicTraits
				CTFontSymbolicTraits traits = 0;
				if (fp.isBold)
					traits |= kCTFontTraitBold;
				if (fp.isItalic)
					traits |= kCTFontTraitItalic;
				// TBD: Need to redesign this to use attributed strings.

				CGFloat pt = fp.pointSize;
				if (pt == 0)
					pt = default_font_params.pointSize;

				// Get font name as NSString
				__strong NSString* fontName = string_to_NSString(fp.fontName);

				__strong NSMutableDictionary* fontAttributes = [NSMutableDictionary dictionary];
				__strong NSMutableDictionary* traitsDict = [NSMutableDictionary dictionary];
				if (traits)
				{
					[traitsDict setObject: [NSNumber numberWithUnsignedInt: traits] forKey: (id)kCTFontSymbolicTrait] ;
					[fontAttributes setObject: traitsDict forKey: (id)kCTFontTraitsAttribute] ;
				}
				[fontAttributes setObject: fontName forKey: (id)kCTFontNameAttribute];

				// Create CTFontDescriptior
				CTFontDescriptorRef fontDescriptor = CTFontDescriptorCreateWithAttributes((__bridge CFDictionaryRef)fontAttributes);

				// Create CTFont
				m_font = CTFontCreateWithFontDescriptor(fontDescriptor, pt, NULL);
				m_metrics.ascent = CTFontGetAscent(m_font);
				m_metrics.descent = CTFontGetDescent(m_font);
				m_metrics.spacing = m_metrics.ascent + m_metrics.descent + CTFontGetLeading(m_font);

				// Create set of attribites containing underlined state, to pass to CFAttributedString later
				if (fp.isUnderlined)
				{
					SInt32 underlineType = kCTUnderlineStyleSingle;
					CFNumberRef underlineRef = CFNumberCreate(NULL, kCFNumberSInt32Type, &underlineType);
					CFTypeRef keys[] = { kCTFontAttributeName, kCTUnderlineStyleAttributeName };
					CFTypeRef vals[] = { m_font, underlineRef };
					m_attributes = CFDictionaryCreate(NULL, keys, vals, sizeof(keys), NULL, &kCFTypeDictionaryValueCallBacks);
					CFRelease(underlineRef);
				}
				else
				{
					const void* keys[] = { kCTFontAttributeName };
					const void* vals[] = { m_font };
					m_attributes = CFDictionaryCreate(NULL, keys, vals, std::size(keys), NULL, &kCFTypeDictionaryValueCallBacks);
				}
			};

			for (const auto& font_params : f)
			{
				if (font_params.fontName.is_empty())
				{
					// Blank font name implies default font, but override point size and merge style.
					auto fp = default_font_params;
					fp.pointSize = font_params.pointSize;
					fp.isItalic |= font_params.isItalic;
					fp.isBold |= font_params.isBold;
					fp.isUnderlined |= font_params.isUnderlined;
					fp.isStrikeOut |= font_params.isStrikeOut;
					update(fp);
					return;
				}

				auto itor = singleton<fontlist>::get()->find_equal(font_params.fontName);
				if (!itor)
					continue;
				update(font_params);
				return;
			}
			update(default_font_params);
		}

		~font()
		{
			CFRelease(m_font);
			CFRelease(m_attributes);
			CGContextRelease(m_context);
		}

		const CTFontRef& get_CTFont() const { return m_font; }
		NSFont* get_NSFont() const { return (__bridge NSFont*)m_font; }
		const CGContextRef& get_CGContext() const { return m_context; }

		virtual font::metrics get_metrics() const { return m_metrics; }
		CFDictionaryRef get_attributes() const { return m_attributes; }

		virtual gfx::size calc_text_bounds(const composite_string& s) const
		{
			__strong NSString* str = string_to_NSString(s);
			CFStringRef strRef = (__bridge CFStringRef)str;
			CFAttributedStringRef attribStr = CFAttributedStringCreate(NULL, strRef, m_attributes);
			CTLineRef line = CTLineCreateWithAttributedString(attribStr);
			CGRect r = CTLineGetImageBounds(line, m_context);
			CFRelease(line);
			CFRelease(attribStr);
			return make_size(r);
		}
	};

	// Drawing primatives
	static void fill(const gfx::bounds& b, const color& c = color::constant::black, bool blendAlpha = true, const state_token& token = state_token())
	{
		if (!blendAlpha || !c.is_fully_transparent())
		{
			CGContextSetRGBFillColor(token.get_CGContext(), (CGFloat)c.get_red() / 0xFF, (CGFloat)c.get_green() / 0xFF, (CGFloat)c.get_blue() / 0xFF, (CGFloat)c.get_alpha() / 0xFF);
			CGBlendMode blendMode = blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy;
			fill(b, blendMode, token);
		}
	}

	static void fill(const gfx::bounds& b, CGBlendMode blendMode, const state_token& token = state_token())
	{
		CGContextRef ctx = token.get_CGContext();
		CGContextSetBlendMode(ctx, blendMode);
		CGContextFillRect(ctx, make_CGRect(b));
	}

	static void invert(const gfx::bounds& b, const state_token& token = state_token())
	{
		CGContextRef ctx = token.get_CGContext();
		CGContextSetBlendMode(ctx, kCGBlendModeDifference);
		CGContextSetRGBFillColor(ctx, 1.0, 1.0, 1.0, 1.0);
		CGContextFillRect(ctx, make_CGRect(b));
	}

	static void draw_line(const gfx::point& startPt, const gfx::point& endPt, double width, CGBlendMode blendMode, const state_token& token = state_token())
	{
		CGContextRef ctx = token.get_CGContext();
		CGContextSetBlendMode(ctx, blendMode);
		CGContextSetLineWidth(ctx, width);
		CGContextMoveToPoint(ctx, startPt.get_x(), startPt.get_y());
		CGContextAddLineToPoint(ctx, endPt.get_x(), endPt.get_y());
		CGContextStrokePath(ctx);
	}

	static void draw_line(const gfx::point& startPt, const gfx::point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true, const state_token& token = state_token())
	{
		CGContextRef ctx = token.get_CGContext();
		CGContextSetRGBStrokeColor(ctx, (CGFloat)c.get_red() / 0xFF, (CGFloat)c.get_green() / 0xFF, (CGFloat)c.get_blue() / 0xFF, (CGFloat)c.get_alpha() / 0xFF);
		CGBlendMode blendMode = blendAlpha ? kCGBlendModeNormal : kCGBlendModeCopy;
		draw_line(startPt, endPt, width, blendMode, token);
	}

	// text and font primatives
	static rcref<gfx::font> load_font(const gfx::font_parameters_list& f, const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		return rcnew(font)(f, ctx);
	}

	static string get_default_font_name()
	{
		return NSString_to_string([[NSFont systemFontOfSize: [NSFont systemFontSize]] fontName]);
	}

	static gfx::font_parameters get_default_font()
	{
		CGFloat fontSize = [NSFont systemFontSize];
		return gfx::font_parameters{
			.pointSize = fontSize,
			.fontName = get_default_font_name()
		};
	}

	static void draw_text(const composite_string& s, const gfx::bounds& b, const rcptr<gfx::font>& f, const color& c = color::constant::black, const state_token& token = state_token())
	{
		CGColorRef colorRef = CGColorCreateGenericRGB((CGFloat)c.get_red() / 0xFF, (CGFloat)c.get_green() / 0xFF, (CGFloat)c.get_blue() / 0xFF, (CGFloat)c.get_alpha() / 0xFF);
		draw_text(s, b, f, colorRef, token);
		CGColorRelease(colorRef);
	}

	static void draw_text(const composite_string& s, const gfx::bounds& b, const rcptr<gfx::font>& f, const CGColorRef& c, const state_token& token = state_token())
	{
		auto inner = [&](font& f2)
		{
			__strong NSString* str = string_to_NSString(s);
			CFStringRef strRef = (__bridge CFStringRef)str;
			CFAttributedStringRef attribStr = CFAttributedStringCreate(NULL, strRef, f2.get_attributes());
			CTLineRef line = CTLineCreateWithAttributedString(attribStr);
			CGContextRef ctx = token.get_CGContext();
			CGRect r = make_CGRect(b);
			CGContextClipToRect(ctx, r);
			CGContextSetTextMatrix(ctx, CGAffineTransformMakeScale(1.0f, -1.0f));
			CGContextTranslateCTM(ctx, 0.0f, r.size.height - CTFontGetDescent(f2.get_CTFont()));
			CTLineDraw(line, ctx);
			CFRelease(line);
			CFRelease(attribStr);
		};
		if (!f)
			inner(*load_font(gfx::font_parameters_list()).template static_cast_to<font>());
		else
		{
			font& derivedFont = *f.template static_cast_to<font>().get_ptr();
			inner(derivedFont);
		}
	}

	// Compositing images
	static void draw_image(const bitmap_graphics_context& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, CGBlendMode blendMode, const state_token& token = state_token());

	static void draw_bitmap(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, bool blendAlpha = true, const state_token& token = state_token());
	static void draw_bitmask(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true, const state_token& token = state_token());
	static void mask_out(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool inverted = false, const state_token& token = state_token());
	static void draw_bitmap_with_bitmask(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool blendAlpha = true, bool inverted = false, const state_token& token = state_token());

	static rcref<gfx::bitmap> create_bitmap(const gfx::size& sz, std::optional<color> fillColor = std::nullopt);
	static rcref<gfx::bitmap> load_bitmap(const composite_string& location);
	static rcref<gfx::bitmask> create_bitmask(const gfx::size& sz, std::optional<bool> value = std::nullopt);
	static rcref<gfx::bitmask> load_bitmask(const composite_string& location);

	static void save_clip(const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		CGContextSaveGState(ctx);
	}

	static void restore_clip(const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		CGContextRestoreGState(ctx);
	}

	static void clip_out(const gfx::bounds& b, const gfx::size& cellSize, const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		CGRect outerRect = make_CGRect(cellSize);
		CGRect r2 = make_CGRect(b);
		CGContextBeginPath(ctx);
		CGContextAddRect(ctx, outerRect);
		CGContextAddRect(ctx, r2);
		CGContextClip(ctx);
	}

	static void clip_to(const gfx::bounds& b, const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		CGContextClipToRect(ctx, make_CGRect(b));
	}

	static bool is_unclipped(const gfx::bounds& b, const CGContextRef& ctx = [[NSGraphicsContext currentContext] CGContext])
	{
		CGRect clipBox = CGContextGetClipBoundingBox(ctx);
		CGRect testRect = make_CGRect(b);
		return CGRectIntersectsRect(clipBox, testRect);
	}
};


}
}


#include "cogs/os/gfx/bitmap.hpp"


#endif
