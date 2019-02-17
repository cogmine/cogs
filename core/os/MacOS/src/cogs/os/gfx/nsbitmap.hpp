//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_NSBITMAP
#define COGS_HEADER_OS_GFX_NSBITMAP


#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/os/collections/macos_strings.hpp"
#include "cogs/os/gfx/graphics_context.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {
namespace gfx {
namespace os {


class nsbitmap : public canvas::pixel_mask	// already derived from pixel_image through pixel_image_canvas
{
protected:
	NSBitmapImageRep*		m_imageRep;
	CGImageRef				m_mask;

public:
	// creates bitmap from monochome image format
	nsbitmap(const composite_string& name, bool resource = false)	// !resource implies filename
	{
		NSString* n = string_to_NSString(name);
		NSString* imageName;
		if (resource)
			imageName = [[NSBundle mainBundle] pathForResource:n ofType:@"bmp"]; 
		else
			imageName = n;
		
		m_imageRep = [NSBitmapImageRep imageRepWithData:[NSData dataWithContentsOfFile:imageName]];
		
		[n release];

		CGImageRef cgImage = [m_imageRep CGImage];
		
		m_mask = CGImageMaskCreate(CGImageGetWidth(cgImage),
				CGImageGetHeight(cgImage),
				CGImageGetBitsPerComponent(cgImage),
				CGImageGetBitsPerPixel(cgImage),
				CGImageGetBytesPerRow(cgImage),
				CGImageGetDataProvider(cgImage), NULL, false);
	}

	~nsbitmap()
	{
		CGImageRelease(m_mask);
	}

	CGImageRef get_CGImageRef() const	{ return m_mask; }

	canvas::size get_size() const
	{
		size_t w = CGImageGetWidth(m_mask);
		size_t h = CGImageGetHeight(m_mask);
		return canvas::size(w, h);
	}
};


inline rcref<canvas::pixel_mask> graphics_context::load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi)
{
	return rcnew(nsbitmap, location, dpi);
}


inline void graphics_context::composite_pixel_mask(const canvas::pixel_mask& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	const nsbitmap* im = dynamic_cast<const nsbitmap*>(&src);
	if (!!im)
	{
		NSRect srcRect2 = make_NSRect(srcBounds);
		NSRect dstRect2 = make_NSRect(srcBounds.get_size());

		NSColor* foreColor = make_NSColor(fore);
		NSColor* backColor = make_NSColor(back);

		NSGraphicsContext* curContext = [NSGraphicsContext currentContext];
		[curContext saveGraphicsState];
		[curContext setCompositingOperation: NSCompositeCopy];

		CGContextTranslateCTM((CGContextRef)[curContext graphicsPort], dstPt.get_x(), dstPt.get_y() + srcBounds.get_height());
		CGContextScaleCTM((CGContextRef)[curContext graphicsPort], 1.0, -1.0);   

		CGImageRef img1 = CGImageCreateWithImageInRect(im->get_CGImageRef(), NSRectToCGRect(srcRect2));

		[backColor setFill];
		[NSBezierPath fillRect: dstRect2];

		[foreColor setFill];
		CGContextDrawImage((CGContextRef)[curContext graphicsPort], NSRectToCGRect(dstRect2), img1);

		CGImageRelease(img1);

		[curContext restoreGraphicsState];
	}
}


}
}
}


#endif
