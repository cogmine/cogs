//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_BITMAP
#define COGS_HEADER_OS_GFX_BITMAP

#include <optional>
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

class bitmap : public device_context, public canvas::bitmap, public canvas::bitmask
{
private:
	class gdi_bitmap
	{
	private:
		HBITMAP m_bitMap;
		HDC m_hDC;
		SIZE m_size;
		int m_depth;
		void* m_bits;
		LONG m_widthBytes;

		gdi_bitmap(const gdi_bitmap&) = delete;
		gdi_bitmap& operator=(const gdi_bitmap&) = delete;

		gdi_bitmap()
		{ }

		gdi_bitmap(HDC dc)
			: m_hDC(dc)
		{ }

		void release()
		{
			if (m_hDC != NULL)
				DeleteDC(m_hDC);
			if (m_bitMap != NULL)
				DeleteObject(m_bitMap);
		}

	public:
		static gdi_bitmap load(const composite_string& location)
		{
			gdi_bitmap result(CreateCompatibleDC(NULL));

			result.m_bitMap = (HBITMAP)LoadImage(GetModuleHandle(0), location.composite().cstr(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			COGS_ASSERT(!!result.m_bitMap);
			SelectObject(result.m_hDC, result.m_bitMap);

			BITMAP bm;
			GetObject((HANDLE)result.m_bitMap, sizeof(bm), &bm);
			COGS_ASSERT(!!bm.bmBits);

			result.m_size.cx = bm.bmWidth;
			result.m_size.cy = bm.bmHeight;
			result.m_depth = bm.bmBitsPixel;
			result.m_bits = (BYTE*)bm.bmBits;
			result.m_widthBytes = bm.bmWidthBytes;
			
			return result;
		}

		static gdi_bitmap load_monochrome(const composite_string& location)
		{
			gdi_bitmap result = load(location);
			if (result.m_depth != 1)
				result.resample(1);
			return result;
		}

		static gdi_bitmap load_rgba(const composite_string& location, bool* isOpaque = nullptr)
		{
			gdi_bitmap result = load(location);
			bool isBitmapOpaque = result.m_depth != 32;
			if (isOpaque)
				*isOpaque = isBitmapOpaque;
			if (isBitmapOpaque)
				result.resample(32);
			return result;
		}

		gdi_bitmap(SIZE sz, int depth)
			: m_hDC(CreateCompatibleDC(NULL)),
			m_size(sz),
			m_depth(depth)
		{
			COGS_ASSERT(!!m_hDC);
			if (depth == 1)
				m_bitMap = CreateBitmap(sz.cx, sz.cy, 1, 1, NULL);
			else
			{
				BITMAPINFO bmi = { };
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biWidth = sz.cx;
				bmi.bmiHeader.biHeight = -sz.cy;
				bmi.bmiHeader.biBitCount = depth;
				bmi.bmiHeader.biCompression = BI_RGB;

				m_bitMap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&m_bits, NULL, 0);

				BITMAP bm;
				GetObject((HANDLE)m_bitMap, sizeof(bm), &bm);
				m_widthBytes = bm.bmWidthBytes;
			}
			COGS_ASSERT(m_bitMap);
			SelectObject(m_hDC, m_bitMap);
		}

		gdi_bitmap(gdi_bitmap&& src)
			: m_bitMap(src.m_bitMap),
			m_hDC(src.m_hDC),
			m_size(src.m_size),
			m_depth(src.m_depth),
			m_bits(src.m_bits),
			m_widthBytes(src.m_widthBytes)
		{
			src.m_bitMap = NULL;
			src.m_hDC = NULL;
		}

		gdi_bitmap& operator=(gdi_bitmap&& src)
		{
			release();
			m_bitMap = src.m_bitMap;
			m_hDC = src.m_hDC;
			m_size = src.m_size;
			m_depth = src.m_depth;
			m_bits = src.m_bits;
			m_widthBytes = src.m_widthBytes;
			src.m_bitMap = NULL;
			src.m_hDC = NULL;
			return *this;
		}

		// Swaps bitmap, but preserves DC
		void replace_with(gdi_bitmap&& src)
		{
			DeleteDC(src.m_hDC);
			src.m_hDC = NULL;
			DeleteObject(m_bitMap);
			m_bitMap = src.m_bitMap;
			src.m_bitMap = NULL;
			SelectObject(m_hDC, m_bitMap);
			m_size = src.m_size;
			m_depth = src.m_depth;
			m_bits = src.m_bits;
			m_widthBytes = src.m_widthBytes;
		}

		~gdi_bitmap() { release(); }

		HBITMAP get_HBITMAP() const { return m_bitMap; }
		HDC get_HDC() const { return m_hDC; }
		SIZE get_size() const { return m_size; }
		int get_depth() const { return m_depth; }
		DWORD get_width_bytes() const { return m_widthBytes; }

		gdi_bitmap clone()
		{
			return get_resampled(m_depth);
		}

		void resample(int depth)
		{
			if (depth != m_depth)
				replace_with(get_resampled(depth));
		}

		gdi_bitmap get_resampled(int depth)
		{
			gdi_bitmap result(m_size, depth);
			const BOUNDS bounds = { { 0, 0 }, m_size };
			if (depth == m_depth || depth < 32)
				blit(result.get_HDC(), bounds, { 0, 0 });
			else //if (depth != m_depth && depth == 32)
			{
				// fill with opaque pixels, as AlphaBlend will not set alpha when copying from <32 to 32 bits
				result.fill(bounds, color::black, false);
				alpha_blend(result.get_HDC(), { 0, 0 }, bounds);
			}

			return result;
		}

		void fill(const BOUNDS& b, const color& c, bool blendAlpha)
		{
			Gdiplus::SolidBrush solidBrush(Gdiplus::Color(c.get_alpha(), c.get_red(), c.get_green(), c.get_blue()));
			Gdiplus::Graphics gfx(m_hDC);
			if (!blendAlpha || c.is_opaque())
				gfx.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
			else
				gfx.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
			gfx.FillRectangle(&solidBrush, b.pt.x, b.pt.y, b.sz.cx, b.sz.cy);
		}

		static DWORD create_rop(composite_mode compositeMode)
		{
			switch (compositeMode)
			{
			case composite_mode::and_mode:
				return SRCAND;
			case composite_mode::or_mode:
				return SRCPAINT;
			case composite_mode::xor_mode:
				return SRCINVERT;
			case composite_mode::clear_mode:
				return NOTSRCERASE;
			case composite_mode::copy_inverted_mode:
				return NOTSRCCOPY;
			case composite_mode::copy_mode:
			default:
				return SRCCOPY;
			}
		}

		void blit(HDC dstDC, const BOUNDS& dstBounds, const POINT& srcPt, composite_mode compositeMode = composite_mode::copy_mode) const
		{
			DWORD rop = create_rop(compositeMode);
			BOOL b = BitBlt(dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy, m_hDC, srcPt.x, srcPt.y, rop);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void blit(HDC dstDC, const BOUNDS& dstBounds, const POINT& srcPt, const COLORREF& fore, const COLORREF& back) const
		{
			SetTextColor(dstDC, make_COLORREF(color::black));
			SetBkColor(dstDC, make_COLORREF(color::white));
			BOOL b = BitBlt(dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy, m_hDC, srcPt.x, srcPt.y, SRCCOPY);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void alpha_blend(HDC dstDC, const POINT& dstPt, const BOUNDS& srcBounds, UINT sourceConstantAlpha = 0xFF) const
		{
			BLENDFUNCTION blendFunc = { };
			blendFunc.BlendOp = AC_SRC_OVER;
			blendFunc.BlendFlags = 1; // AC_USE_HIGHQUALITYFILTER.
			// Without it, blending with bitmaps without alpha channels (<32-bit) will result in an empty alpha channel.
			blendFunc.SourceConstantAlpha = sourceConstantAlpha;
			blendFunc.AlphaFormat = (m_depth == 32) ? AC_SRC_ALPHA : 0;
			BOOL b = AlphaBlend(
				dstDC, dstPt.x, dstPt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				blendFunc);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void stretch_and_preserve_alpha(HDC dstDC, const BOUNDS& dstBounds, const BOUNDS& srcBounds, UINT sourceConstantAlpha = 0xFF) const
		{
			COGS_ASSERT(dstBounds.sz != srcBounds.sz);	// Will set alpha properly (not preserve it) if not resizing.
			SetStretchBltMode(dstDC, HALFTONE);
			SetBrushOrgEx(dstDC, 0, 0, NULL);
			BLENDFUNCTION blendFunc = { };
			blendFunc.BlendOp = AC_SRC_OVER;
			blendFunc.BlendFlags = 1; // AC_USE_HIGHQUALITYFILTER.  Filters, but leaves alpha channel untouched.
			blendFunc.SourceConstantAlpha = sourceConstantAlpha;
			blendFunc.AlphaFormat = (m_depth == 32) ? AC_SRC_ALPHA : 0;
			BOOL b = AlphaBlend(
				dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				blendFunc);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void alpha_blend_stretch(HDC dstDC, const BOUNDS& dstBounds, const BOUNDS& srcBounds, UINT sourceConstantAlpha = 0xFF) const
		{
			SetStretchBltMode(dstDC, HALFTONE);
			SetBrushOrgEx(dstDC, 0, 0, NULL);
			BLENDFUNCTION blendFunc = { };
			blendFunc.BlendOp = AC_SRC_OVER;
			blendFunc.BlendFlags = 0;
			blendFunc.SourceConstantAlpha = sourceConstantAlpha;
			blendFunc.AlphaFormat = (m_depth == 32) ? AC_SRC_ALPHA : 0;
			BOOL b = AlphaBlend(
				dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				blendFunc);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		// Takes advantage of the fact that TransparentBlt will write 0 values to the alpha it writes, but leaves
		// alpha alone where it did not copy due to transparent color.
		void mask_out(HDC dstDC, const BOUNDS& dstBounds, const BOUNDS& srcBounds, bool transparentPlane) const
		{
			COGS_ASSERT(m_depth == 1);
			SetStretchBltMode(dstDC, COLORONCOLOR);
			if (transparentPlane)
				SetBkColor(dstDC, RGB(0, 0, 0));
			else
				SetTextColor(dstDC, RGB(0, 0, 0));
			BOOL b = TransparentBlt(
				dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				transparentPlane ? RGB(255, 255, 255) : RGB(0, 0, 0));
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void stretch(HDC dstDC, const BOUNDS& dstBounds, const BOUNDS& srcBounds) const
		{
			COGS_ASSERT(m_depth < 32);	// Sets alpha to 0, so shouldn't be used for 32-bit bitmaps
			int mode = ((m_depth == 1) || (dstBounds.sz == srcBounds.sz)) ? COLORONCOLOR : HALFTONE;
			SetStretchBltMode(dstDC, mode);
			SetBrushOrgEx(dstDC, 0, 0, NULL);
			BOOL b = StretchBlt(
				dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				SRCCOPY);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		void stretch(HDC dstDC, const BOUNDS& dstBounds, const BOUNDS& srcBounds, const COLORREF& fore, const COLORREF& back) const
		{
			COGS_ASSERT(m_depth == 1);
			SetBkColor(dstDC, fore);
			SetTextColor(dstDC, back);
			SetStretchBltMode(dstDC, COLORONCOLOR);
			SetBrushOrgEx(dstDC, 0, 0, NULL);
			BOOL b = StretchBlt(
				dstDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy,
				m_hDC, srcBounds.pt.x, srcBounds.pt.y, srcBounds.sz.cx, srcBounds.sz.cy,
				SRCCOPY);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
		}

		static DWORD create_rop(fill_mode fillMode)
		{
			switch (fillMode)
			{
			case fill_mode::clear_mode:
				return BLACKNESS;
			case fill_mode::invert_mode:
				return DSTINVERT;
			default:
			case fill_mode::set_mode:
				return WHITENESS;
			}
		}

		void bit_fill(const BOUNDS& dstBounds, fill_mode fillMode)
		{
			HANDLE oldBrush = SelectObject(m_hDC, (HANDLE)GetStockObject(WHITE_BRUSH));
			DWORD rop = create_rop(fillMode);
			BOOL b = PatBlt(m_hDC, dstBounds.pt.x, dstBounds.pt.y, dstBounds.sz.cx, dstBounds.sz.cy, rop);
			DWORD err = GetLastError();
			COGS_ASSERT(b);
			SelectObject(m_hDC, oldBrush);
		}

		static DWORD create_rop2(fill_mode fillMode)
		{
			switch (fillMode)
			{
			case fill_mode::clear_mode:
				return R2_NOTCOPYPEN;
			case fill_mode::invert_mode:
				return R2_XORPEN;
			default:
			case fill_mode::set_mode:
				return R2_COPYPEN;
			}
		}

		void draw_line(const POINT& startPt, const POINT& endPt, int width, fill_mode fillMode)
		{
			HPEN pen = CreatePen(PS_SOLID, width, RGB(255, 255, 255));
			SelectObject(m_hDC, pen);
			DWORD rop2 = create_rop2(fillMode);
			SetROP2(m_hDC, rop2);
			MoveToEx(m_hDC, startPt.x, startPt.y, NULL);
			LineTo(m_hDC, endPt.x, endPt.y);
			SelectObject(m_hDC, m_bitMap);
			DeleteObject(pen);
		}
	};

	gdi_bitmap m_gdiBitmap;
	size m_logicalDipSize;
	SIZE m_logicalSize;
	bool m_isOpaque;

	inline static thread_local std::optional<gdi_bitmap> scratch1;
	inline static thread_local std::optional<gdi_bitmap> scratch24;
	inline static thread_local std::optional<gdi_bitmap> scratch32;

	static gdi_bitmap& setup_scratch_1(const SIZE& sz)
	{
		if (!scratch1.has_value() || scratch1->get_size().cx < sz.cx || scratch1->get_size().cy < sz.cy)
			scratch1.emplace(sz, 1);
		return *scratch1;
	}

	static gdi_bitmap& setup_scratch_24(const SIZE& sz)
	{
		if (!scratch24.has_value() || scratch24->get_size().cx < sz.cx || scratch24->get_size().cy < sz.cy)
			scratch24.emplace(sz, 24);
		return *scratch24;
	}

	static gdi_bitmap& setup_scratch_32(const SIZE& sz)
	{
		if (!scratch32.has_value() || scratch32->get_size().cx < sz.cx || scratch32->get_size().cy < sz.cy)
			scratch32.emplace(sz, 32);
		return *scratch32;
	}

	friend class device_context;

	enum class image_type {
		monochrome = 1,
		rgb = 24,
		rgba = 32
	};

	void update_opacity(const BOUNDS& dstBounds, bool isSourceOpaque, bool blendAlpha)
	{
		if (m_isOpaque != isSourceOpaque)
		{
			if (!isSourceOpaque)
				m_isOpaque = blendAlpha;
			else
				m_isOpaque = dstBounds.pt.x <= 0 && (dstBounds.sz.cx - dstBounds.pt.x) >= m_logicalSize.cx
						&& dstBounds.pt.y <= 0 && (dstBounds.sz.cy - dstBounds.pt.y) >= m_logicalSize.cy;
		}
	}

protected:
	bitmap(const ptr<rc_obj_base>& desc, const size& sz, image_type imageType, std::optional<color> fillColor = std::nullopt, double dpi = dip_dpi)
		: device_context(desc, dpi),
		m_gdiBitmap(make_SIZE(sz), (int)imageType)
	{
		set_HDC(m_gdiBitmap.get_HDC());
		m_logicalSize = m_gdiBitmap.get_size();
		m_logicalDipSize = sz;
		if (m_gdiBitmap.get_depth() < 32)
			m_isOpaque = true;
		else
		{
			// A newly allocated GDI bitmap is zero-initialized (transparent)
			if (!fillColor.has_value() || !fillColor->is_fully_transparent())
				m_isOpaque = false;
			else
			{
				m_gdiBitmap.fill({ { 0, 0 }, m_logicalSize }, *fillColor, false);
				m_isOpaque = fillColor->is_opaque();
			}
		}
	}

	bitmap(const ptr<rc_obj_base>& desc, const composite_string& location, image_type imageType)
		: device_context(desc, dip_dpi),
		m_gdiBitmap(gdi_bitmap::load_rgba(location, &m_isOpaque)),
		m_logicalSize(m_gdiBitmap.get_size())
	{
		set_HDC(m_gdiBitmap.get_HDC());
		m_logicalDipSize = make_size(m_logicalSize);
		COGS_ASSERT(get_HDC());
	}

public:
	virtual size get_size() const { return m_logicalDipSize; }

	SIZE get_pixel_size() const { return m_logicalSize; }

	virtual bool is_opaque() const { return m_isOpaque; }

	HBITMAP get_HBITMAP() const { return m_gdiBitmap.get_HBITMAP(); }

	// canvas interface functions

	virtual void fill(const bounds& b, const color& c, bool blendAlpha = true)
	{
		if (!blendAlpha || !c.is_fully_transparent())
		{
			BOUNDS b2 = make_BOUNDS(b);
			m_gdiBitmap.fill(b2, c, blendAlpha);
			update_opacity(b2, c.is_opaque(), blendAlpha);
		}
	}

	virtual void fill(const bounds& b, fill_mode fillMode = fill_mode::set_mode)
	{
		// masks don't vary in DPI
		bounds b2 = b.normalized();
		BOUNDS b3;
		b3.pt.x = (LONG)std::lround(b2.get_x());
		b3.pt.y = (LONG)std::lround(b2.get_y());
		b3.sz.cx = (LONG)std::lround((b2.get_x() + b2.get_width())) - b3.pt.x;
		b3.sz.cy = (LONG)std::lround((b2.get_y() + b2.get_height())) - b3.pt.y;

		m_gdiBitmap.bit_fill(b3, fillMode);
	}

	virtual void invert(const bounds& b)
	{
		if (m_gdiBitmap.get_depth() != 32)
			device_context::invert(b);
		else
		{
			BOUNDS b2 = make_BOUNDS(b);
			gdi_bitmap& tmpBmp = setup_scratch_24(b2.sz);
			BOOL b3 = BitBlt(tmpBmp.get_HDC(), 0, 0, b2.sz.cx, b2.sz.cy, get_HDC(), b2.pt.x, b2.pt.y, NOTSRCCOPY);
			DWORD err = GetLastError();
			COGS_ASSERT(b3);

			BOUNDS tmpBounds{ { 0, 0 }, b2.sz };
			tmpBmp.alpha_blend(get_HDC(), b2.pt, tmpBounds);
		}
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width , const color& c, bool blendAlpha = true)
	{
		device_context::draw_line(startPt, endPt, width, c, blendAlpha);
		if (!blendAlpha && !c.is_opaque())
			m_isOpaque = false;
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, fill_mode fillMode = fill_mode::set_mode)
	{
		// masks don't vary in DPI
		POINT startPt2;
		startPt2.x = (LONG)std::lround(startPt.get_x());
		startPt2.y = (LONG)std::lround(startPt.get_y());

		POINT endPt2;
		endPt2.x = (LONG)std::lround(endPt.get_x());
		endPt2.y = (LONG)std::lround(endPt.get_y());

		int width2 = (LONG)std::lround(width);

		m_gdiBitmap.draw_line(startPt2, endPt2, width2, fillMode);
	}

	virtual rcref<canvas::font> load_font(const gfx::font& guiFont)
	{
		return device_context::load_font(guiFont);
	}

	virtual gfx::font get_default_font() const
	{
		return device_context::get_default_font();
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<canvas::font>& f = 0, const color& c = color::black)
	{
		device_context::draw_text(s, b, f, c);
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<canvas::font>& f = 0, bool value = true)
	{
		color c = value ? color::white : color::black;
		device_context::draw_text(s, b, f, c);
	}

	virtual void draw_bitmap(const canvas::bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
	{
		if (!!srcBounds && !!dstBounds)
		{
			const bitmap* srcImage = static_cast<const bitmap*>(&src);
			BOUNDS srcBounds2 = srcImage->make_BOUNDS(srcBounds);
			BOUNDS dstBounds2 = make_BOUNDS(dstBounds);
			device_context::draw_bitmap_inner(this, *srcImage, srcBounds2, dstBounds2, blendAlpha);
			update_opacity(dstBounds2, srcImage->is_opaque(), blendAlpha);
		}
	}

	virtual void draw_bitmask(const canvas::bitmask& src, const bounds& srcBounds, const bounds& dstBounds, const color& fore, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		const bitmap* srcMask = static_cast<const bitmap*>(&src);

		uint8_t foreAlpha = fore.get_alpha();
		uint8_t backAlpha = back.get_alpha();
		bool areBothFullyTransparent = blendForeAlpha && blendBackAlpha && !foreAlpha && !backAlpha;
		if (!areBothFullyTransparent)
		{
			if (foreAlpha == 0xFF)
				blendForeAlpha = true;
			if (backAlpha == 0xFF)
				blendBackAlpha = true;

			BOUNDS dstBounds2 = make_BOUNDS(dstBounds);

			// masks don't vary in DPI
			//BOUNDS srcBounds2 = make_BOUNDS(srcBounds);
			BOUNDS srcBounds2;
			canvas::bounds b = srcBounds.normalized();
			srcBounds2.pt.x = (LONG)std::lround(b.get_x());
			srcBounds2.pt.y = (LONG)std::lround(b.get_y());
			srcBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - srcBounds2.pt.x;
			srcBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - srcBounds2.pt.y;

			device_context::draw_bitmask_inner(*srcMask, srcBounds2, dstBounds2, fore, back, blendForeAlpha, blendBackAlpha);
			if ((!blendForeAlpha && !fore.is_opaque()) || (!blendBackAlpha && !back.is_opaque()))
				m_isOpaque = false;
			else if (!blendForeAlpha && !blendBackAlpha && fore.is_opaque() && back.is_opaque())
				update_opacity(dstBounds2, true, false);
		}
	}

	virtual void draw_bitmask(const canvas::bitmask& src, const bounds& srcBounds, const bounds& dstBounds, composite_mode compositeMode = composite_mode::copy_mode)
	{
		const bitmap* srcImage = static_cast<const bitmap*>(&src);
		
		// masks don't vary in DPI
		BOUNDS srcBounds2;
		bounds b = srcBounds.normalized();
		srcBounds2.pt.x = (LONG)std::lround(b.get_x());
		srcBounds2.pt.y = (LONG)std::lround(b.get_y());
		srcBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - srcBounds2.pt.x;
		srcBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - srcBounds2.pt.y;

		BOUNDS dstBounds2;
		b = dstBounds.normalized();
		dstBounds2.pt.x = (LONG)std::lround(b.get_x());
		dstBounds2.pt.y = (LONG)std::lround(b.get_y());
		dstBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - dstBounds2.pt.x;
		dstBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - dstBounds2.pt.y;
		if (srcBounds2.sz == dstBounds2.sz)
			srcImage->m_gdiBitmap.blit(get_HDC(), dstBounds2, srcBounds2.pt, compositeMode);
		else
		{
			bitmap::gdi_bitmap& tmpBmp = bitmap::setup_scratch_1(dstBounds2.sz);
			srcImage->m_gdiBitmap.stretch(tmpBmp.get_HDC(), { { 0, 0 }, dstBounds2.sz }, srcBounds2);
			srcImage->m_gdiBitmap.blit(get_HDC(), dstBounds2, { 0, 0 }, compositeMode);
		}
	}

	virtual void mask_out(const canvas::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false)
	{
		device_context::mask_out(msk, mskBounds, dstBounds, inverted);
		m_isOpaque = false;
	}

	virtual void draw_bitmap_with_bitmask(const canvas::bitmap& src, const bounds& srcBounds, const canvas::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false)
	{
		device_context::draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted);
		m_isOpaque = m_isOpaque && src.is_opaque();
	}

	virtual rcref<canvas::bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return device_context::create_bitmap(sz, fillColor, dip_dpi);
	}

	virtual rcref<canvas::bitmap> load_bitmap(const composite_string& location)
	{
		return device_context::load_bitmap(location);
	}

	virtual rcref<canvas::bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt)
	{
		return device_context::create_bitmask(sz, value);
	}

	virtual rcref<canvas::bitmask> load_bitmask(const composite_string& location)
	{
		return device_context::load_bitmask(location);
	}

	virtual void save_clip()
	{
		return device_context::save_clip();
	}

	virtual void restore_clip()
	{
		return device_context::restore_clip();
	}

	virtual void clip_out(const bounds& b)
	{
		return device_context::clip_out(b);
	}

	virtual void clip_to(const bounds& b)
	{
		return device_context::clip_to(b);
	}

	virtual bool is_unclipped(const bounds& b) const
	{
		return device_context::is_unclipped(b);
	}

	virtual double get_dpi() const
	{
		return device_context::get_dpi();
	}

	virtual void set_dpi(double dpi)
	{
		device_context::set_dpi(dpi);
		m_logicalDipSize = make_size(m_logicalSize);
	}

	virtual void set_size(const size& newSize, const size& growPadding = size(100, 100), bool trimIfShrinking = false)
	{
		SIZE newLogicalPixelSize = make_SIZE(newSize);
		if (newLogicalPixelSize != m_logicalSize)
		{
			SIZE actualSize = m_gdiBitmap.get_size();
			size newActualSize = newSize + growPadding;
			SIZE newActualPixelSize = make_SIZE(newActualSize);
			bool widthOverflow = newLogicalPixelSize.cx > actualSize.cx;
			bool heightOverflow = newLogicalPixelSize.cy > actualSize.cy;
			bool reshape = (widthOverflow || heightOverflow);
			if (!reshape && trimIfShrinking)
			{
				bool widthDecrease = newActualPixelSize.cx < actualSize.cx;
				bool heightDecrease = newActualPixelSize.cy < actualSize.cy;
				reshape = (widthDecrease || heightDecrease);
			}
			if (reshape)
			{
				if (!trimIfShrinking)
				{
					if (newActualPixelSize.cx < actualSize.cx)
						newActualPixelSize.cx = actualSize.cx;
					if (newActualPixelSize.cy < actualSize.cy)
						newActualPixelSize.cy = actualSize.cy;
				}
				gdi_bitmap newBitmap(newActualPixelSize, m_gdiBitmap.get_depth());
				m_gdiBitmap.blit(newBitmap.get_HDC(), { { 0, 0 }, m_logicalSize }, { 0, 0 });
				m_gdiBitmap.replace_with(std::move(newBitmap));
			}
			bool widthIncreased = newLogicalPixelSize.cx > m_logicalSize.cx;
			if (widthIncreased)
			{
				LONG widthDifference = newLogicalPixelSize.cx - m_logicalSize.cx;
				m_gdiBitmap.fill({ { m_logicalSize.cx, 0 }, { widthDifference, newLogicalPixelSize.cy } }, m_isOpaque ? color::black : color::transparent, false);
			}
			if (newLogicalPixelSize.cy > m_logicalSize.cy)
			{
				LONG heightDifference = newLogicalPixelSize.cy - m_logicalSize.cy;
				LONG width;
				if (widthIncreased)
					width = m_logicalSize.cx;
				else
					width = newLogicalPixelSize.cx;
				m_gdiBitmap.fill({ { 0, m_logicalSize.cy }, { width, heightDifference } }, m_isOpaque ? color::black : color::transparent, false);
			}
			m_logicalSize = newLogicalPixelSize;
			m_logicalDipSize = newSize;
		}
	}
};

inline rcref<canvas::bitmap> device_context::create_bitmap(const canvas::size& sz, std::optional<color> fillColor, double dpi)
{
	return rcnew(bypass_constructor_permission<gdi::bitmap>, sz, gdi::bitmap::image_type::rgba, fillColor, dpi);
}

inline rcref<canvas::bitmask> device_context::create_bitmask(const canvas::size& sz, std::optional<bool> value)
{
	return rcnew(bypass_constructor_permission<gdi::bitmap>, sz, gdi::bitmap::image_type::monochrome, value ? color::white : color::transparent);
}

inline rcref<canvas::bitmap> device_context::load_bitmap(const composite_string& location)
{
	return rcnew(bypass_constructor_permission<gdi::bitmap>, location, gdi::bitmap::image_type::rgba);
}

inline rcref<canvas::bitmask> device_context::load_bitmask(const composite_string& location)
{
	return rcnew(bypass_constructor_permission<gdi::bitmap>, location, gdi::bitmap::image_type::monochrome);
}

inline void device_context::draw_bitmap(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, bool blendAlpha)
{
	if (!!srcBounds && !!dstBounds)
	{
		const gdi::bitmap* srcImage = static_cast<const gdi::bitmap*>(&src);
		BOUNDS srcBounds2 = srcImage->make_BOUNDS(srcBounds);
		BOUNDS dstBounds2 = make_BOUNDS(dstBounds);
		draw_bitmap_inner(nullptr, *srcImage, srcBounds2, dstBounds2, blendAlpha);
	}
}

inline void device_context::draw_bitmap_inner(gdi::bitmap* bmp, const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, bool blendAlpha)
{
	COGS_ASSERT(!!m_hDC);

	if (src.is_opaque())
		blendAlpha = false;

	if (blendAlpha)
		src.m_gdiBitmap.alpha_blend_stretch(m_hDC, dstBounds, srcBounds);
	else
	{
		// If same size and not blending alpha, do a simple blit
		if (srcBounds.sz == dstBounds.sz)
			src.m_gdiBitmap.blit(m_hDC, dstBounds, srcBounds.pt);
		else if (!src.is_opaque())
			src.m_gdiBitmap.alpha_blend_stretch(m_hDC, dstBounds, srcBounds);
		else
		{
			if (!!bmp && !bmp->is_opaque())
				bmp->m_gdiBitmap.fill(dstBounds, color::black, false);
			src.m_gdiBitmap.stretch_and_preserve_alpha(m_hDC, dstBounds, srcBounds);
		}
	}
}

inline void device_context::draw_bitmask(const canvas::bitmask& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	const gdi::bitmap* srcMask = static_cast<const gdi::bitmap*>(&src);

	uint8_t foreAlpha = fore.get_alpha();
	uint8_t backAlpha = back.get_alpha();
	bool areBothFullyTransparent = blendForeAlpha && blendBackAlpha && !foreAlpha && !backAlpha;
	if (!areBothFullyTransparent)
	{
		if (foreAlpha == 0xFF)
			blendForeAlpha = true;
		if (backAlpha == 0xFF)
			blendBackAlpha = true;

		BOUNDS dstBounds2 = make_BOUNDS(dstBounds);

		// masks don't vary in DPI
		//BOUNDS srcBounds2 = make_BOUNDS(srcBounds);
		BOUNDS srcBounds2;
		canvas::bounds b = srcBounds.normalized();
		srcBounds2.pt.x = (LONG)std::lround(b.get_x());
		srcBounds2.pt.y = (LONG)std::lround(b.get_y());
		srcBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - srcBounds2.pt.x;
		srcBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - srcBounds2.pt.y;

		draw_bitmask_inner(*srcMask, srcBounds2, dstBounds2, fore, back, blendForeAlpha, blendBackAlpha);
	}
}

inline void device_context::draw_bitmask_inner(const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha)
{
	COGS_ASSERT(!!get_HDC());

	BOUNDS tmpBounds{ { 0, 0 }, dstBounds.sz };

	uint8_t foreAlpha = fore.get_alpha();
	uint8_t backAlpha = back.get_alpha();
	if (foreAlpha == 0xFF)
		blendForeAlpha = true;
	if (backAlpha == 0xFF)
		blendBackAlpha = true;

	bool blendingBoth = blendForeAlpha && blendBackAlpha;
	if (blendingBoth && foreAlpha == backAlpha)
	{
		gdi::bitmap::gdi_bitmap& tmpBmp = gdi::bitmap::setup_scratch_24(dstBounds.sz);
		src.m_gdiBitmap.stretch(tmpBmp.get_HDC(), tmpBounds, srcBounds, make_COLORREF(fore), make_COLORREF(back));
		tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds, foreAlpha);
	}
	else
	{
		gdi::bitmap::gdi_bitmap& tmpBmp = gdi::bitmap::setup_scratch_32(dstBounds.sz);
		if (blendingBoth)
		{
			if (foreAlpha != 0)
			{
				tmpBmp.fill(tmpBounds, fore, false);
				src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, 1);
				tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds);
			}
			if (backAlpha != 0)
			{
				tmpBmp.fill(tmpBounds, back, false);
				src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, false);
				tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds);
			}
		}
		else if (blendBackAlpha == blendForeAlpha)
		{
			tmpBmp.fill(tmpBounds, fore, false);
			src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, true);
			tmpBmp.blit(get_HDC(), dstBounds, { 0, 0 });
			tmpBmp.fill(tmpBounds, back, false);
			src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, false);
			tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds);
		}
		else
		{
			color blendedColor;
			color nonBlendedColor;
			UINT transparentColor1;
			UINT transparentColor2;
			if (blendForeAlpha)
			{
				blendedColor = fore;
				nonBlendedColor = back;
				transparentColor1 = RGB(255, 255, 255);
				transparentColor2 = RGB(0, 0, 0);
			}
			else
			{
				blendedColor = back;
				nonBlendedColor = fore;
				transparentColor1 = RGB(0, 0, 0);
				transparentColor2 = RGB(255, 255, 255);
			}
			src.m_gdiBitmap.mask_out(get_HDC(), dstBounds, srcBounds, blendForeAlpha);
			tmpBmp.fill(tmpBounds, nonBlendedColor, false);
			src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, blendBackAlpha);
			tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds);
			if (blendedColor.get_alpha() != 0)
			{
				tmpBmp.fill(tmpBounds, blendedColor, false);
				src.m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, srcBounds, blendForeAlpha);
				tmpBmp.alpha_blend(get_HDC(), dstBounds.pt, tmpBounds);
			}
		}
	}
}


inline void device_context::mask_out(const canvas::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted)
{
	const gdi::bitmap* srcMask = static_cast<const gdi::bitmap*>(&msk);

	BOUNDS dstBounds2 = make_BOUNDS(dstBounds);

	// masks don't vary in DPI
	BOUNDS mskBounds2;
	canvas::bounds b = mskBounds.normalized();
	mskBounds2.pt.x = (LONG)std::lround(b.get_x());
	mskBounds2.pt.y = (LONG)std::lround(b.get_y());
	mskBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - mskBounds2.pt.x;
	mskBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - mskBounds2.pt.y;

	srcMask->m_gdiBitmap.mask_out(get_HDC(), dstBounds2, mskBounds2, inverted);
}


inline void device_context::draw_bitmap_with_bitmask(const canvas::bitmap& src, const bounds& srcBounds, const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha, bool inverted)
{
	const gdi::bitmap* srcImage = static_cast<const gdi::bitmap*>(&src);
	const gdi::bitmap* srcMask = static_cast<const gdi::bitmap*>(&msk);

	BOUNDS dstBounds2 = make_BOUNDS(dstBounds);
	BOUNDS srcBounds2 = srcImage->make_BOUNDS(srcBounds);

	// masks don't vary in DPI
	BOUNDS mskBounds2;
	canvas::bounds b = mskBounds.normalized();
	mskBounds2.pt.x = (LONG)std::lround(b.get_x());
	mskBounds2.pt.y = (LONG)std::lround(b.get_y());
	mskBounds2.sz.cx = (LONG)std::lround((b.get_x() + b.get_width())) - mskBounds2.pt.x;
	mskBounds2.sz.cy = (LONG)std::lround((b.get_y() + b.get_height())) - mskBounds2.pt.y;

	if (!blendAlpha)
		srcMask->m_gdiBitmap.mask_out(get_HDC(), dstBounds2, mskBounds2, !inverted);

	BOUNDS tmpBounds = { { 0, 0 }, dstBounds2.sz };
	gdi::bitmap::gdi_bitmap& tmpBmp = gdi::bitmap::setup_scratch_32(dstBounds2.sz);
	if (dstBounds2.sz == srcBounds2.sz)
		srcImage->m_gdiBitmap.blit(tmpBmp.get_HDC(), tmpBounds, srcBounds2.pt);
	else if (srcImage->is_opaque())
	{
		tmpBmp.fill(tmpBounds, color::black, false);
		srcImage->m_gdiBitmap.stretch_and_preserve_alpha(tmpBmp.get_HDC(), tmpBounds, srcBounds2);
	}
	else
	{
		tmpBmp.fill(tmpBounds, color::transparent, false);
		srcImage->m_gdiBitmap.alpha_blend_stretch(tmpBmp.get_HDC(), tmpBounds, srcBounds2);
	}

	srcMask->m_gdiBitmap.mask_out(tmpBmp.get_HDC(), tmpBounds, mskBounds2, inverted);
	tmpBmp.alpha_blend(get_HDC(), dstBounds2.pt, tmpBounds);
}


}
}
}
}


#endif
