//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_BITMAP
#define COGS_HEADER_OS_GFX_BITMAP


#include "cogs/env.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/gfx/device_context.hpp"


namespace cogs {
namespace gfx {
namespace os {
namespace gdi {


// On Windows 7 or later (WDDM 1.1), the following functions are hardware accelerated:
//		AlphaBlend
//		BitBlt
//		StretchBlt
//		TransparentBlt
// These are accelerated even for DIBs, as DIB contents are stored in aparture memory (video memory mapped to system memory).
// GDI's support for the alpha channel is very limited.  Having direct access to the bits of the DIBs is useful to work around
// those limitations.
#define COGS_USE_DEVICE_DEPENDENT_BITMAPS 0


class bitmap : public device_context, public canvas::pixel_image_canvas, public canvas::pixel_mask	// already derived from pixel_image through pixel_image_canvas
{
private:
	HBITMAP	m_bitMap;
	BYTE* m_bits;
	mutable HBRUSH m_brush = NULL;
	size m_logicalDipSize;
	SIZE m_logicalPixelSize;
	SIZE m_actualPixelSize;
	LONG m_widthBytes;
	bool m_isOpaque = false;
	double m_originalDpi;
	int m_bitsPerPixel;

protected:

#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
	bitmap(HDC hDC, const size& sz, double dpi = canvas::dip_dpi)
		: device_context(CreateCompatibleDC(hDC))
	{
		m_originalDpi = dpi;
		device_context::set_dpi(dpi);
		m_logicalDipSize = sz;
		m_logicalPixelSize = make_SIZE(sz);
		m_actualPixelSize = m_logicalPixelSize;
		m_bitMap = CreateCompatibleBitmap(hDC, m_logicalPixelSize.cx, m_logicalPixelSize.cy);
		COGS_ASSERT(!!m_bitMap);
		m_bits = 0;
		m_widthBytes = 0;
		SelectObject(get_HDC(), m_bitMap);
	}
#endif

	bitmap(HDC hDC, const size& sz, int bitsPerPixel, double dpi = canvas::dip_dpi)
		: device_context(CreateCompatibleDC(NULL))
	{
		m_bitsPerPixel = bitsPerPixel;
		m_originalDpi = dpi;
		device_context::set_dpi(dpi);
		m_logicalDipSize = sz;
		m_logicalPixelSize = make_SIZE(sz);
		m_actualPixelSize = m_logicalPixelSize;
		BITMAPINFO bmi = { };
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = m_logicalPixelSize.cx;
		bmi.bmiHeader.biHeight = -m_logicalPixelSize.cy;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = bitsPerPixel;
		bmi.bmiHeader.biCompression = BI_RGB;
		m_bitMap = CreateDIBSection(get_HDC(), &bmi, DIB_RGB_COLORS, (void**)&(m_bits), NULL, 0);
		COGS_ASSERT(!!m_bitMap);
		COGS_ASSERT(!!m_bits);
		SelectObject(get_HDC(), m_bitMap);
		BITMAP bm;
		GetObject((HANDLE)m_bitMap, sizeof(bm), &bm);
		m_widthBytes = bm.bmWidthBytes;
	}

	bitmap(HDC hDC, const composite_string& location, double dpi = canvas::dip_dpi)
		: device_context(CreateCompatibleDC(hDC))
	{
		m_originalDpi = dpi;
		device_context::set_dpi(dpi);
		HBITMAP resBitmap = (HBITMAP)LoadImage(GetModuleHandle(0), location.composite().cstr(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		COGS_ASSERT(!!resBitmap);
		BITMAP bm;
		GetObject((HANDLE)resBitmap, sizeof(bm), &bm);
		m_widthBytes = bm.bmWidthBytes;
		m_logicalPixelSize.cx = bm.bmWidth;
		m_logicalPixelSize.cy = bm.bmHeight;
		m_actualPixelSize = m_logicalPixelSize;
		m_logicalDipSize = make_size(m_logicalPixelSize);
		BYTE* resBits = (BYTE*)bm.bmBits;
		COGS_ASSERT(!!resBits);
#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
		m_bits = 0;
#else
		m_bits = resBits;
#endif
		if (bm.bmBitsPixel == 32)
		{
#if !COGS_USE_DEVICE_DEPENDENT_BITMAPS
			DeleteObject(resBitmap);
			// Load again without the DIB section
			resBitmap = (HBITMAP)LoadImage(GetModuleHandle(0), location.composite().cstr(), IMAGE_BITMAP, 0, 0, 0);
			SelectObject(get_HDC(), resBitmap);
			m_bitMap = resBitmap;
			GetObject((HANDLE)resBitmap, sizeof(bm), &bm);
			m_widthBytes = bm.bmWidthBytes;
#endif
		}
		else //if (bm.bmBitsPixel < 32)
		{
			m_isOpaque = true;
			BOOL b;
			HDC dc2 = CreateCompatibleDC(hDC);
			SelectObject(dc2, resBitmap);
#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
			m_bitMap = CreateCompatibleBitmap(GetDC(NULL), bm.bmWidth, bm.bmHeight);
#else
			BITMAPINFO bmi = { };
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = bm.bmWidth;
			bmi.bmiHeader.biHeight = -bm.bmHeight;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			m_bitMap = CreateDIBSection(get_HDC(), &bmi, DIB_RGB_COLORS, (void**)&(m_bits), NULL, 0);
#endif
			COGS_ASSERT(!!m_bitMap);
			SelectObject(get_HDC(), m_bitMap);
			GetObject((HANDLE)m_bitMap, sizeof(bm), &bm);
			m_widthBytes = bm.bmWidthBytes;

			// fill with opaque
			fill_inner(0, 0, bm.bmWidth, bm.bmHeight, true);
			int i = SetStretchBltMode(get_HDC(), COLORONCOLOR);
			b = SetBrushOrgEx(get_HDC(), 0, 0, NULL);
			COGS_ASSERT(b);
			BLENDFUNCTION blendFunc = { };
			blendFunc.BlendOp = AC_SRC_OVER;
			blendFunc.BlendFlags = 1;
			blendFunc.SourceConstantAlpha = 255;
			blendFunc.AlphaFormat = 0;
			b = AlphaBlend(get_HDC(),
				0,
				0,
				bm.bmWidth, bm.bmHeight,
				dc2,
				0,
				0,
				bm.bmWidth, bm.bmHeight,
				blendFunc);
			//DWORD err = GetLastError();
			COGS_ASSERT(b);
			DeleteDC(dc2);
			DeleteObject(resBitmap);
		}
	}

public:
	~bitmap()
	{
		GdiFlush();
		if (!!m_brush)
			DeleteObject(m_brush);
		DeleteObject(m_bitMap);
		DeleteDC(get_HDC());
	}

#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
	static rcref<bitmap> create_compatible_bitmap(HDC hDC, const size& sz, double dpi = canvas::dip_dpi)
	{
		return rcnew(bypass_constructor_permission<bitmap>, hDC, sz, dpi);
	}
#endif

	static rcref<bitmap> create_memory_bitmap(HDC hDC, const size& sz, int bitsPerPixel = 32, double dpi = canvas::dip_dpi)
	{
		return rcnew(bypass_constructor_permission<bitmap>, hDC, sz, bitsPerPixel, dpi);
	}

	static rcref<bitmap> load_bitmap(HDC hDC, const composite_string& location, double dpi = canvas::dip_dpi)
	{
		return rcnew(bypass_constructor_permission<bitmap>, hDC, location, dpi);
	}

	virtual size get_size() const { return m_logicalDipSize; }

	SIZE get_pixel_size() const { return m_logicalPixelSize; }

	double get_original_dpi() const { return m_originalDpi; }

	HBITMAP	get_HBITMAP() const { return m_bitMap; }
	BYTE* get_bits() const { return m_bits; }
	virtual bool is_opaque() const { return m_isOpaque; }

	HBRUSH get_BRUSH() const
	{
		if (!!m_brush)
			DeleteObject(m_brush);
		m_brush = CreatePatternBrush(m_bitMap);
		return m_brush;
	}

	// canvas interface functions

	virtual void fill(const bounds& b, const color& c, bool blendAlpha = true)
	{
		device_context::fill_inner(m_isOpaque, m_logicalPixelSize, b, c, blendAlpha);
	}

	virtual void invert(const bounds& r)
	{
		// TBD: Does GDI handle Alpha correctly for InvertRect ?
		device_context::invert(r);
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
	{
		device_context::draw_line(startPt, endPt, width, c, blendAlpha);

		if (!blendAlpha && !c.is_opaque())
			m_isOpaque = false;
	}

	void set_pixel(const point& pt, const color& c = color::black, bool blendAlpha = true)
	{
		POINT pt2 = make_POINT(pt);
		BYTE alph = c.get_alpha();
		BYTE rd = c.get_red();
		BYTE grn = c.get_green();
		BYTE bl = c.get_blue();
#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
		Gdiplus::Bitmap bmp(m_bitmap, NULL);
		bmp.SetPixel(pt2.x, pt2.y, Gdiplus::Color(alph, rd, grn, bl));
#else
		if (c.is_opaque())
			blendAlpha = false;
		size_t pixelIndex = (pt2.y * m_widthBytes) + (pt2.x * 4);
		GdiFlush();
		DWORD dw;
		if (blendAlpha)
		{
			// premultiply
			rd = (rd * (int)alph) / 255;
			grn = (grn * (int)alph) / 255;
			bl = (bl * (int)alph) / 255;
			// Blend in the original 
			DWORD dw = *((DWORD*)(m_bits[pixelIndex]));
			BYTE alph2 = (BYTE)(dw >> 24);
			BYTE rd2 = (BYTE)(dw >> 16);
			BYTE grn2 = (BYTE)(dw >> 8);
			BYTE bl2 = (BYTE)dw;
			rd += rd2 * ((255 - (int)alph) / 255);
			grn += grn2 * ((255 - (int)alph) / 255);
			bl += bl2 * ((255 - (int)alph) / 255);
			alph += alph2 * ((255 - (int)alph) / 255);
		}
		dw = (DWORD)alph << 24
			| (DWORD)rd << 16
			| (DWORD)grn << 8
			| (DWORD)bl;
		*((DWORD*)(m_bits[pixelIndex])) = dw;
#endif

		if (!blendAlpha && !c.is_opaque())
			m_isOpaque = false;
	}

	color get_pixel(const POINT& pt)  const
	{
#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
		Gdiplus::Bitmap bmp(m_bitmap, NULL);
		Gdiplus::Color c;
		bmp.GetPixel(pt.x, pt.y, &c);
		color c2(c.GetR(), c.GetG(), c.GetB(), c.GetA());
		return c2;
#else
		size_t pixelIndex = (pt.y * m_widthBytes) + (pt.x * 4);
		GdiFlush();
		DWORD dw = ((DWORD*)m_bits)[pixelIndex];
		BYTE alph = (BYTE)(dw >> 24);
		BYTE rd = (BYTE)(dw >> 16);
		BYTE grn = (BYTE)(dw >> 8);
		BYTE bl = (BYTE)dw;
		color c(rd, grn, bl, alph);
		return c;
#endif
	}

	virtual void scroll(const bounds& r, const point& pt = point(0,0))
	{
		device_context::scroll(r, pt);
	}

	virtual rcref<canvas::font> load_font(const gfx::font& guiFont)
	{
		return device_context::load_font(guiFont);
	}

	virtual gfx::font get_default_font()
	{
		return device_context::get_default_font();
	}

	virtual void draw_text(const composite_string& s, const bounds& r, const rcptr<canvas::font>& f = 0, const color& c = color::black, bool blendAlpha = true)
	{
		device_context::draw_text(s, r, f, c, blendAlpha);

		if (!blendAlpha && !c.is_opaque())
			m_isOpaque = false;
	}

	virtual void composite_pixel_image(const pixel_image& src, const bounds& srcBounds, const point& dstPt = point(0, 0), bool blendAlpha = true)
	{
		const bitmap* srcImage = static_cast<const bitmap*>(&src);
		device_context::composite_pixel_image_inner(*srcImage, srcBounds, dstPt, blendAlpha);
		BOUNDS b = make_BOUNDS(dstPt, srcBounds.get_size());
		if (blendAlpha)
		{
			if (srcImage->is_opaque() && !b.pt && b.sz == m_logicalPixelSize)
				m_isOpaque = true;
		}
		else if (!srcImage->is_opaque())
			m_isOpaque = false;
		else if (!b.pt && b.sz == m_logicalPixelSize)	// If filling the entire contents
			m_isOpaque = true;
	}
	
	virtual void composite_scaled_pixel_image(const pixel_image& src, const bounds& srcBounds, const bounds& dstBounds)
	{
		const bitmap* srcImage = static_cast<const bitmap*>(&src);
		device_context::composite_scaled_pixel_image_inner(m_isOpaque, *srcImage, srcBounds, dstBounds);
		BOUNDS b = make_BOUNDS(dstBounds);
		if (!srcImage->is_opaque())
			m_isOpaque = false;
		else if (!b.pt && b.sz == m_logicalPixelSize)	// If filling the entire contents
			m_isOpaque = true;
	}

	virtual void composite_pixel_mask(const pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true )
	{
		device_context::composite_pixel_mask(src, srcBounds, dstPt, fore, back, blendForeAlpha, blendBackAlpha);
	}
	
	virtual rcref<pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi)
	{
		return device_context::create_pixel_image_canvas(sz, isOpaque, dpi);
	}
	
	virtual rcref<pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)
	{
		return device_context::load_pixel_image(location, dpi);
	}
	
	virtual rcref<pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi)
	{
		return device_context::load_pixel_mask(location, dpi);
	}
	
	virtual void save_clip()
	{
		return device_context::save_clip();
	}

	virtual void restore_clip()
	{
		return device_context::restore_clip();
	}

	virtual void clip_out(const bounds& r)
	{
		return device_context::clip_out(r);
	}

	virtual void clip_to(const bounds& r)
	{
		return device_context::clip_to(r);
	}

	virtual bool is_unclipped(const bounds& r) const
	{
		return device_context::is_unclipped(r);
	}

	virtual double get_dpi() const
	{
		return device_context::get_dpi();
	}

	virtual void set_dpi(double dpi)
	{
		device_context::set_dpi(dpi);
		m_logicalDipSize = make_size(m_logicalPixelSize);
	}

	virtual size get_actual_size() const
	{
		return make_size(m_actualPixelSize);
	}

	virtual void set_size(const size& newSize, const size& growPadding = size(100, 100), bool trimIfShrinking = false)
	{
		SIZE newLogicalPixelSize = make_SIZE(newSize);
		if (newLogicalPixelSize != m_logicalPixelSize)
		{
			size newActualSize = newSize + growPadding;
			SIZE newActualPixelSize = make_SIZE(newActualSize);
			bool widthOverflow = newLogicalPixelSize.cx > m_actualPixelSize.cx;
			bool heightOverflow = newLogicalPixelSize.cy > m_actualPixelSize.cy;
			bool reshape = (widthOverflow || heightOverflow);
			if (!reshape && trimIfShrinking)
			{
				bool widthDecrease = newActualPixelSize.cx < m_actualPixelSize.cx;
				bool heightDecrease = newActualPixelSize.cy < m_actualPixelSize.cy;
				reshape = (widthDecrease || heightDecrease);
			}
			if (reshape)
			{
				if (!trimIfShrinking)
				{
					if (newActualPixelSize.cx < m_actualPixelSize.cx)
						newActualPixelSize.cx = m_actualPixelSize.cx;
					if (newActualPixelSize.cy < m_actualPixelSize.cy)
						newActualPixelSize.cy = m_actualPixelSize.cy;
				}
				HDC existingDC = get_HDC();
				HDC newDC = CreateCompatibleDC(NULL);
				BYTE* newBits;
				BITMAPINFO bmi = { };
				bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
				bmi.bmiHeader.biWidth = newActualPixelSize.cx;
				bmi.bmiHeader.biHeight = -newActualPixelSize.cy;
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = m_bitsPerPixel;
				bmi.bmiHeader.biCompression = BI_RGB;
				HBITMAP newBitMap = CreateDIBSection(newDC, &bmi, DIB_RGB_COLORS, (void**)&(newBits), NULL, 0);
				COGS_ASSERT(!!newBitMap);
				COGS_ASSERT(!!newBits);
				SelectObject(newDC, newBitMap);
				BITMAP bm;
				GetObject((HANDLE)newBitMap, sizeof(bm), &bm);
				LONG newWidthBytes = bm.bmWidthBytes;
				BOOL b = BitBlt(newDC,
					0,
					0,
					m_logicalPixelSize.cx,
					m_logicalPixelSize.cy,
					existingDC,
					0,
					0,
					SRCCOPY);
				//DWORD err = GetLastError();
				COGS_ASSERT(b);
				GdiFlush();
				if (!!m_brush)
				{
					DeleteObject(m_brush);
					m_brush = NULL;
				}
				DeleteObject(m_bitMap);
				DeleteDC(existingDC);
				m_bitMap = newBitMap;
				get_HDC() = newDC;
				m_bits = newBits;
				m_actualPixelSize = newActualPixelSize;
			}
			bool widthIncrease2 = newLogicalPixelSize.cx > m_logicalPixelSize.cx;
			if (widthIncrease2)
			{
				LONG widthDifference = newLogicalPixelSize.cx - m_logicalPixelSize.cx;
				fill_inner(m_logicalPixelSize.cx, 0, widthDifference, newLogicalPixelSize.cy, m_isOpaque);
			}
			if (newLogicalPixelSize.cy > m_logicalPixelSize.cy)
			{
				LONG heightDifference = newLogicalPixelSize.cy - m_logicalPixelSize.cy;
				LONG width;
				if (widthIncrease2)
					width = m_logicalPixelSize.cx;
				else
					width = newLogicalPixelSize.cx;
				fill_inner(0, m_logicalPixelSize.cy, width, heightDifference, m_isOpaque);
			}
			m_logicalPixelSize = newLogicalPixelSize;
		}
		m_logicalDipSize = newSize;
	}
};

inline rcref<canvas::pixel_image> device_context::load_pixel_image(const composite_string& location, double dpi)
{
	return bitmap::load_bitmap(m_hDC, location, dpi);
}

inline rcref<canvas::pixel_mask> device_context::load_pixel_mask(const composite_string& location, double dpi)
{
	return bitmap::load_bitmap(m_hDC, location, dpi);
}

inline rcref<canvas::pixel_image_canvas> device_context::create_pixel_image_canvas(const canvas::size& sz, bool isOpaque, double dpi)
{
	COGS_ASSERT(!!m_hDC);
#if COGS_USE_DEVICE_DEPENDENT_BITMAPS
	rcref<canvas::pixel_image_canvas> bmp = bitmap::create_compatible_bitmap(m_hDC, sz, dpi);
#else
	rcref<canvas::pixel_image_canvas> bmp = bitmap::create_memory_bitmap(m_hDC, sz, 32, dpi);
#endif
	// By default, GDI bitmaps are created with transparent pixels (0, 0, 0, 0)
	// If the caller needs an opaque buffer, fill with opaque
	if (isOpaque)
		bmp.static_cast_to<bitmap>()->fill_inner(make_BOUNDS(canvas::point(0, 0), bmp->get_size()), true);
	return bmp;
}

inline void device_context::composite_pixel_image(const canvas::pixel_image& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, bool blendAlpha)
{
	return composite_pixel_image_inner(*static_cast<const bitmap*>(&src), srcBounds, dstPt, blendAlpha);
}

inline void device_context::composite_scaled_pixel_image(const canvas::pixel_image& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds)
{
	return composite_scaled_pixel_image_inner(true, *static_cast<const bitmap*>(&src), srcBounds, dstBounds);
}

inline void device_context::composite_pixel_image_inner(const bitmap& src, const BOUNDS& srcBounds, const POINT& dstPt, bool blendAlpha)
{
	COGS_ASSERT(!!m_hDC);
	if (!blendAlpha || src.is_opaque())
	{
		SetTextColor(m_hDC, make_COLORREF(color::black));
		SetBkColor(m_hDC, make_COLORREF(color::white));
		BOOL b = BitBlt(m_hDC,
			dstPt.x,
			dstPt.y,
			srcBounds.sz.cx,
			srcBounds.sz.cy,
			src.get_HDC(),
			srcBounds.pt.x,
			srcBounds.pt.y,
			SRCCOPY);
		DWORD err = GetLastError();
		COGS_ASSERT(b);
	}
	else
	{
		int i = SetStretchBltMode(m_hDC, COLORONCOLOR);
		BOOL b = SetBrushOrgEx(m_hDC, 0, 0, NULL);
		COGS_ASSERT(b);
		BLENDFUNCTION blendFunc = { };
		blendFunc.BlendOp = AC_SRC_OVER;
		blendFunc.BlendFlags = 1;	// Undocumented feature to indicate to leave the alpha channel alone?
		blendFunc.SourceConstantAlpha = 255;
		blendFunc.AlphaFormat = AC_SRC_ALPHA;
		b = AlphaBlend(m_hDC,
			dstPt.x,
			dstPt.y,
			srcBounds.sz.cx,
			srcBounds.sz.cy,
			src.get_HDC(),
			srcBounds.pt.x,
			srcBounds.pt.y,
			srcBounds.sz.cx,
			srcBounds.sz.cy,
			blendFunc);
		DWORD err = GetLastError();
		COGS_ASSERT(b);
	}
}

inline void device_context::composite_pixel_image_inner(const bitmap& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, bool blendAlpha)
{
	if (!!srcBounds)
	{
		BOUNDS srcBounds2 = src.make_BOUNDS(srcBounds);
		POINT dstPt2 = make_POINT(dstPt);
		composite_pixel_image_inner(src, srcBounds2, dstPt2, blendAlpha);
	}
}

inline void device_context::composite_pixel_image_inner(bool& isOpaque, const SIZE& sz, const bitmap& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, bool blendAlpha)
{
	if (!!srcBounds)
	{
		BOUNDS srcBounds2 = src.make_BOUNDS(srcBounds);
		POINT dstPt2 = make_POINT(dstPt);
		composite_pixel_image_inner(src, srcBounds2, dstPt2, blendAlpha);
		update_opacity(isOpaque, sz, srcBounds2, src.is_opaque(), blendAlpha);
	}
}


inline void device_context::composite_scaled_pixel_image_inner(bool isOpaque, const bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds)
{
	COGS_ASSERT(!!m_hDC);

	// Optimization to use BitBlt if no stretching is needed
	bool sameSize = srcBounds.sz == dstBounds.sz;
	if (sameSize)
	{
		SetTextColor(m_hDC, make_COLORREF(color::black));
		SetBkColor(m_hDC, make_COLORREF(color::white));
		BOOL b = BitBlt(m_hDC,
			dstBounds.pt.x,
			dstBounds.pt.y,
			dstBounds.sz.cx,
			dstBounds.sz.cy,
			src.get_HDC(),
			srcBounds.pt.x,
			srcBounds.pt.y,
			SRCCOPY);
		//DWORD err = GetLastError();
		COGS_ASSERT(b);
	}
	else
	{
		// GDI does not have robust support for the alpha channel.
		// A stretch will leave the alpha channel untouched.
		// So, it's necessary to prefill the destination area with opaque pixels, if not already opaque.
		if (!isOpaque)
			fill_inner(dstBounds, true);
		int i = SetStretchBltMode(m_hDC, HALFTONE);
		BOOL b = SetBrushOrgEx(m_hDC, 0, 0, NULL);
		COGS_ASSERT(b);
		BLENDFUNCTION blendFunc = { };
		blendFunc.BlendOp = AC_SRC_OVER;
		blendFunc.BlendFlags = 1;	// Undocumented feature to indicate to leave the alpha channel alone?
		blendFunc.SourceConstantAlpha = 255;
		blendFunc.AlphaFormat = 0;
		b = AlphaBlend(m_hDC,
			dstBounds.pt.x,
			dstBounds.pt.y,
			dstBounds.sz.cx,
			dstBounds.sz.cy,
			src.get_HDC(),
			srcBounds.pt.x,
			srcBounds.pt.y,
			srcBounds.sz.cx,
			srcBounds.sz.cy,
			blendFunc);
		//DWORD err = GetLastError();
		COGS_ASSERT(b);
	}
}


inline void device_context::composite_scaled_pixel_image_inner(bool isOpaque, const bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds)
{
	if (!!srcBounds && !!dstBounds)
	{
		BOUNDS srcBounds2 = src.make_BOUNDS(srcBounds);
		BOUNDS dstBounds2 = make_BOUNDS(dstBounds);
		composite_scaled_pixel_image_inner(isOpaque, src, srcBounds2, dstBounds2);
	}
}


inline void device_context::composite_scaled_pixel_image_inner(bool& isOpaque, const SIZE& sz, const bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds)
{
	if (!!srcBounds && !!dstBounds)
	{
		BOUNDS srcBounds2 = src.make_BOUNDS(srcBounds);
		BOUNDS dstBounds2 = make_BOUNDS(dstBounds);
		composite_scaled_pixel_image_inner(isOpaque, src, srcBounds2, dstBounds2);
		update_opacity(isOpaque, sz, srcBounds2, src.is_opaque(), false);
	}
}

inline void device_context::composite_pixel_mask(const canvas::pixel_mask& src, const canvas::bounds& srcBounds, const canvas::point& dstPt, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	COGS_ASSERT(!!m_hDC);
	const bitmap* myMask = static_cast<const bitmap*>(&src);
	BOUNDS srcBounds2 = myMask->make_BOUNDS(srcBounds);
	POINT dstPt2 = make_POINT(dstPt);
	SetTextColor(m_hDC, make_COLORREF(fore));
	SetBkColor(m_hDC, make_COLORREF(back));
	BOOL b = BitBlt(m_hDC,
		dstPt2.x,
		dstPt2.y,
		srcBounds2.sz.cx,
		srcBounds2.sz.cy,
		myMask->get_HDC(),
		srcBounds2.pt.x,
		srcBounds2.pt.y,
		SRCCOPY);
}


}
}
}
}

#endif
