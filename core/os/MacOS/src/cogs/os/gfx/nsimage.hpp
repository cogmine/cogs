//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_NSIMAGE
#define COGS_OS_NSIMAGE


#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/os/collections/macos_strings.hpp"
#include "cogs/os/gfx/graphics_context.hpp"
#include "cogs/os/gfx/nsbitmap.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {
namespace gfx {
namespace os {


class nsimage : public canvas::pixel_image_canvas, public graphics_context
{
protected:
	NSImage*	m_image;

public:
	nsimage(const composite_string& name, bool resource = true)	// !resource implies filename
	{
		NSString* n = string_to_NSString(name);
		NSString* imageName;
		if (resource)
			imageName = [[NSBundle mainBundle] pathForResource:n ofType:@"bmp"]; 
		else
			imageName = n;
		
		m_image = [[NSImage alloc] initWithContentsOfFile:imageName];
		[m_image setTemplate:YES];
		
		[n release];
	}

	nsimage(const size& sz)
	{
		NSSize nsSize = make_NSSize(sz);
		m_image = [[NSImage alloc] initWithSize:nsSize];
		[m_image setTemplate:YES];
	}

	~nsimage()
	{
		[m_image release];
	}

	NSImage* get_NSImage() const					{ return m_image; }

	virtual canvas::size get_size() const
	{
		NSSize sz = [m_image size];
		canvas::size result(sz.width, sz.height);
		return result;
	}

	void set_size(const size& sz)
	{
		NSSize nsSize = make_NSSize(sz);
		[m_image setSize: nsSize];
	}

	// canvas interface functions

	virtual void fill(const bounds& r, const color& c = color::black, bool blendAlpha = true)
	{
		[m_image lockFocus];
		graphics_context::fill(r, c, blendAlpha);
		[m_image unlockFocus];
	}

	virtual void invert(const bounds& r)
	{
		[m_image lockFocus];
		graphics_context::invert(r);
		[m_image unlockFocus];
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
	{
		[m_image lockFocus];
		graphics_context::draw_line(startPt, endPt, width, c, blendAlpha);
		[m_image unlockFocus];
	}

	virtual void scroll(const bounds& r, const point& pt = point(0,0))
	{
		if (pt == r.get_position())
			return;

		NSPoint pt2 = make_NSPoint(pt);
		NSRect r2 = make_NSRect(r);
		[m_image lockFocus];
		[m_image	drawAtPoint:pt2			// not 100% sure this works.  Need to check.
					fromRect:r2
					operation:NSCompositeCopy
					fraction:1.0];
		[m_image unlockFocus];
	}

	virtual rcref<canvas::font> load_font(const gfx::font& guiFont)							{ return graphics_context::load_font(guiFont); }
	
	virtual void draw_text(const composite_string& s, const bounds& r, const rcptr<canvas::font>& f = 0, const color& c = color::black, bool blendAlpha = true)
	{
		[m_image lockFocus];
		graphics_context::draw_text(s, r, f, c, blendAlpha);
		[m_image unlockFocus];
	}

	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true)
	{
		[m_image lockFocus];
		graphics_context::composite_pixel_image(img, srcBounds, dstPt, blendAlpha);
		[m_image unlockFocus];
	}

	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds)
	{
		[m_image lockFocus];
		graphics_context::composite_scaled_pixel_image(img, srcBounds, dstBounds);
		[m_image unlockFocus];
	}

	//virtual void composite_scaled_pixel_image(const canvas::pixel_image& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
	//{
	//	[m_image lockFocus];
	//	graphics_context::composite_scaled_pixel_image(img, srcBounds, dstBounds, blendAlpha);
	//	[m_image unlockFocus];
	//}


	virtual void composite_pixel_mask(const canvas::pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		[m_image lockFocus];
		graphics_context::composite_pixel_mask(src, srcBounds, dstPt, fore, back, blendForeAlpha, blendBackAlpha);
		[m_image unlockFocus];
	}

	virtual rcref<canvas::pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi)
	{ return graphics_context::create_pixel_image_canvas(sz, isOpaque, dpi); }

	virtual rcref<canvas::pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)
	{ return graphics_context::load_pixel_image(location, dpi); }

	virtual rcref<canvas::pixel_mask> load_pixel_image_mask(const composite_string& location, double dpi = canvas::dip_dpi)
	{ return graphics_context::load_pixel_image_mask(location, dpi); }

	virtual void save_clip()
	{
		[m_image lockFocus];
		graphics_context::save_clip();
		[m_image unlockFocus];
	}

	virtual void restore_clip()
	{
		[m_image lockFocus];
		graphics_context::restore_clip();
		[m_image unlockFocus];
	}

	virtual void clip_out(const bounds& r)
	{
		[m_image lockFocus];
		graphics_context::clip_out(r);
		[m_image unlockFocus];
	}

	virtual void clip_to(const bounds& r)
	{
		[m_image lockFocus];
		graphics_context::clip_to(r);
		[m_image unlockFocus];
	}
};


inline rcref<canvas::pixel_image> graphics_context::load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)
{
	return rcnew(nsimage, location, dpi);
}

inline rcref<canvas::pixel_image_canvas> graphics_context::create_pixel_image_canvas(const canvas::size& sz, bool isOpaque, double dpi = canvas::dip_dpi)
{
	return rcnew(nsimage, sz, dpi);
}


inline void graphics_context::composite_pixel_image(const canvas::pixel_image& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, bool blendAlpha)
{
	const nsimage* im = dynamic_cast<const nsimage*>(&img);
	if (!!im)
	{
		NSRect srcRect2 = make_NSRect(srcBounds);
		NSRect dstRect2 = make_NSRect(dstPt, srcBounds.get_size());

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		[curContext saveGraphicsState];
		if (blendAlpha)
			[curContext setCompositingOperation: NSCompositeSourceOver];
		else
			[curContext setCompositingOperation: NSCompositeCopy];

		NSImage* nsImage = im->get_NSImage();
		[nsImage	drawInRect: dstRect2
						fromRect: srcRect2
						operation: NSCompositeCopy
						fraction : 1.0
						respectFlipped: YES
						hints: nil
						];

		[curContext restoreGraphicsState];
	}
}

inline void graphics_context::composite_scaled_pixel_image(const canvas::pixel_image& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds)
{
	const nsimage* im = dynamic_cast<const nsimage*>(&img);
	if (!!im)
	{
		NSRect srcRect2 = make_NSRect(srcBounds);
		NSRect dstRect2 = make_NSRect(dstBounds);

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		[curContext saveGraphicsState];
		//if (blendAlpha)
		//	[curContext setCompositingOperation : NSCompositeSourceOver];
		//else
			[curContext setCompositingOperation : NSCompositeCopy];

		NSImage* nsImage = im->get_NSImage();
		[nsImage	drawInRect : dstRect2
			fromRect : srcRect2
			operation : NSCompositeCopy
			fraction : 1.0
			respectFlipped : YES
			hints : nil
		];

		[curContext restoreGraphicsState];
	}
}




}
}
}


#endif
