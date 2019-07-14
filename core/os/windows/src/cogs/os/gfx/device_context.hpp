//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_DEVICE_CONTEXT
#define COGS_HEADER_OS_GFX_DEVICE_CONTEXT


#include "cogs/env.hpp"
#include "cogs/collections/container_stack.hpp"
#include "cogs/collections/set.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/mem/auto_handle.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"


inline bool operator!(const SIZE& sz) { return !sz.cx || !sz.cy; }
inline bool operator==(const SIZE& sz1, const SIZE& sz2) { return (sz1.cx == sz2.cx) && (sz1.cy == sz2.cy); }
inline bool operator!=(const SIZE& sz1, const SIZE& sz2) { return (sz1.cx != sz2.cx) || (sz1.cy != sz2.cy); }

inline bool operator!(const POINT& pt) { return !pt.x && !pt.y; }
inline bool operator==(const POINT& pt1, const POINT& pt2) { return (pt1.x == pt2.x) && (pt1.y == pt2.y); }
inline bool operator!=(const POINT& pt1, const POINT& pt2) { return (pt1.x != pt2.x) || (pt1.y != pt2.y); }

namespace cogs {
namespace gfx {
namespace os {
namespace gdi {


// GDI device_context canvas.
// Contains a current HDC, but does not own it.
// To avoid multiply deriving from device_context, encapsulate gdi::device_context by value.
class device_context;

class bitmap;	// forward declared here, defined in bitmap.hpp

inline void auto_handle_impl_DeleteObject(HGDIOBJ h) { DeleteObject(h); }
typedef auto_handle<HGDIOBJ, (HGDIOBJ)NULL, auto_handle_impl_DeleteObject> auto_HGDIOBJ;

inline void auto_handle_impl_DeleteObject(HBRUSH h) { DeleteObject(h); }
typedef auto_handle<HBRUSH, (HBRUSH)NULL, auto_handle_impl_DeleteObject> auto_HBRUSH;

inline void auto_handle_impl_DeleteObject(HBITMAP h) { DeleteObject(h); }
typedef auto_handle<HBITMAP, (HBITMAP)NULL, auto_handle_impl_DeleteObject> auto_HBITMAP;

inline void auto_handle_impl_DeleteObject(HFONT h) { DeleteObject(h); }
typedef auto_handle<HFONT, (HFONT)NULL, auto_handle_impl_DeleteObject> auto_HFONT;

inline void auto_handle_impl_DeleteObject(HRGN h) { DeleteObject(h); }
typedef auto_handle<HRGN, (HRGN)NULL, auto_handle_impl_DeleteObject> auto_HRGN;

inline void auto_handle_impl_DeleteObject(HPEN h) { DeleteObject(h); }
typedef auto_handle<HPEN, (HPEN)NULL, auto_handle_impl_DeleteObject> auto_HPEN;

inline void auto_handle_impl_DeleteObject(HPALETTE h) { DeleteObject(h); }
typedef auto_handle<HPALETTE, (HPALETTE)NULL, auto_handle_impl_DeleteObject> auto_HPALETTE;

inline void auto_handle_impl_DeleteColorSpace(HCOLORSPACE h) { DeleteColorSpace(h); }
typedef auto_handle<HCOLORSPACE, (HCOLORSPACE)NULL, auto_handle_impl_DeleteColorSpace> auto_HCOLORSPACE;


#define COGS_RENDER_GDIPLUS_FONTS 0 // If 1, use GDI+ font rendering.  If 0, use renders GDI font rendering
// GDI plus fonts are preferred.  "GDI generally offers better performance and more accurate text measuring (than GDI+)"


struct BOUNDS
{
	POINT pt;
	SIZE sz;
};


// device contexts are assumed to be manipulated only with the same UI thread.  (Does not call GdiFlush())

class device_context : public object, public canvas
{
public:
	class gdi_plus_scope
	{
	public:
		ULONG_PTR m_gdiplusToken;

		gdi_plus_scope()
		{
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
		}

		~gdi_plus_scope()
		{
			Gdiplus::GdiplusShutdown(m_gdiplusToken);
		}
	};

	static rcref<gdi_plus_scope> get_default_gdi_plus_scope()
	{
		return singleton<gdi_plus_scope>::get();
	}

	static constexpr int dip_dpi = 96;

private:
	rcref<gdi_plus_scope> m_gdiPlusScope;
	HDC m_hDC = NULL;
	container_stack<HRGN> m_savedClips;
	double m_dpi = dip_dpi;
	double m_scale = 1.0;

protected:
	void draw_bitmap_inner(gdi::bitmap* bmp, const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, bool blendAlpha);
	void draw_bitmask_inner(const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha);

public:
	class font : public canvas::font
	{
	public:
		class fontlist : public nonvolatile_set<composite_string, true, case_insensitive_comparator<composite_string> >
		{
		private:
			static int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam)
			{
				auto fontList = (fontlist*)lParam;
				fontList->try_insert(lpelfe->lfFaceName);
				return 1;
			}

		protected:
			fontlist()
			{
				// Load fonts.  (Fonts added or removed will not be observed until relaunch)
				LOGFONT logFont = {};
				logFont.lfCharSet = DEFAULT_CHARSET;
				HDC hdc = GetDC(NULL);
				int i = EnumFontFamiliesEx(hdc, &logFont, &EnumFontFamExProc, (LPARAM)this, 0);
				ReleaseDC(NULL, hdc);
			}
		};

	private:
		// Wrapper with own class to avoid use of GDI+ custom new operator
		class gdiplus_font
		{
		public:
			Gdiplus::Font m_font;

			gdiplus_font(
				const WCHAR* familyName,
				IN Gdiplus::REAL emSize,
				IN INT style = Gdiplus::FontStyleRegular,
				IN Gdiplus::Unit unit = Gdiplus::UnitPoint,
				IN const Gdiplus::FontCollection* fontCollection = NULL)
				: m_font(familyName, emSize, style, unit, fontCollection)
			{
				COGS_ASSERT(m_font.IsAvailable());
			}
		};

		rcref<device_context> m_deviceContext;
		gfx::font m_gfxFont;
		mutable std::unique_ptr<gdiplus_font> m_GdiplusFont;
		mutable auto_HFONT m_HFONT;
		mutable double m_savedDpi;

		friend class device_context;

		// Uses prioritized list of font names to find best match font.
		// If no match is found, name of default font is returned.
		static const composite_string& resolve(const vector<composite_string>& fontNames)
		{
			size_t numFontNames = fontNames.get_length();
			composite_string fontName;
			for (size_t i = 0; i < numFontNames; i++)
			{
				auto itor = singleton<fontlist>::get()->find_equal(fontNames[i]);
				if (!!itor)
					return *itor;	// Use case from font list, just in case.  ha
			}
			return singleton<default_font>::get()->get_font_names()[0];
		}

		void create_gdi_fonts(double dpi) const
		{
			COGS_ASSERT(!!m_deviceContext->m_hDC);

			int fontStyle = (int)Gdiplus::FontStyleRegular;
			if (m_gfxFont.is_italic())
				fontStyle |= (int)Gdiplus::FontStyleItalic;
			if (m_gfxFont.is_underlined())
				fontStyle |= (int)Gdiplus::FontStyleUnderline;
			if (m_gfxFont.is_bold())
				fontStyle |= (int)Gdiplus::FontStyleBold;
			if (m_gfxFont.is_strike_out())
				fontStyle |= (int)Gdiplus::FontStyleStrikeout;

			gfx::font defaultFont = m_deviceContext->get_default_font();
			double pt = m_gfxFont.get_point_size();
			if (pt == 0)
				pt = defaultFont.get_point_size();

			// convert pt to pixels
			if (!!dpi)
				m_savedDpi = dpi;
			else
				m_savedDpi = dip_dpi;
			double pixels = pixels = (dpi * pt) / 72;

			composite_string fontName = resolve(m_gfxFont.get_font_names());
			if (!fontName)
				fontName = defaultFont.get_font_names()[0];

			m_GdiplusFont = std::make_unique<gdiplus_font>(
				fontName.composite().cstr(),
				(Gdiplus::REAL)pixels,
				fontStyle,
				Gdiplus::UnitPixel);

			Gdiplus::Graphics gfx(m_deviceContext->m_hDC);
			LOGFONT logFont = {};
			Gdiplus::Status status = m_GdiplusFont->m_font.GetLogFontW(&gfx, &logFont);
			COGS_ASSERT(status == Gdiplus::Status::Ok);
			m_HFONT = CreateFontIndirect(&logFont);
		}

		void handle_dpi_change() const
		{
			double newDpi = m_deviceContext->get_dpi();
			if (newDpi != m_savedDpi)
				create_gdi_fonts(newDpi);
		}

	public:
		font(const gfx::font& gfxFont, const rcref<device_context>& dc)
			: m_deviceContext(dc),
			m_gfxFont(gfxFont)
		{
			create_gdi_fonts(m_deviceContext->get_dpi());
		}

		HFONT get_HFONT() const { return *m_HFONT; }

		virtual font::metrics get_metrics() const
		{
			handle_dpi_change();
			return m_deviceContext->get_font_metrics(*this);
		}

		virtual canvas::size calc_text_bounds(const composite_string& s) const
		{
			handle_dpi_change();
			return m_deviceContext->calc_text_bounds(s, *this);
		}
	};

	class default_font : public gfx::font
	{
	public:
		default_font()
		{
			// Load default font
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);// -sizeof(ncm.iPaddedBorderWidth);?
			BOOL b = SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dip_dpi);

			append_font_name(ncm.lfMessageFont.lfFaceName);
			set_italic(ncm.lfMessageFont.lfItalic != 0);
			set_bold(ncm.lfMessageFont.lfWeight >= 700);
			set_underlined(ncm.lfMessageFont.lfUnderline != 0);
			set_strike_out(ncm.lfMessageFont.lfStrikeOut != 0);
			set_point_size(-ncm.lfMessageFont.lfHeight);
		}
	};

private:
	canvas::font::metrics get_font_metrics(const font& f) const
	{
		COGS_ASSERT(!!m_hDC);
		canvas::font::metrics result = {};

#if COGS_RENDER_GDIPLUS_FONTS
		Gdiplus::FontFamily ff;
		Gdiplus::Status status = f.m_GdiplusFont->m_font.GetFamily(&ff);
		COGS_ASSERT(status == Gdiplus::Status::Ok);

		Gdiplus::REAL size = f.m_GdiplusFont->m_font.GetSize();
		int emHeight = ff.GetEmHeight(Gdiplus::FontStyleRegular);
		int ascent = ff.GetCellAscent(Gdiplus::FontStyleRegular);
		int descent = ff.GetCellDescent(Gdiplus::FontStyleRegular);
		int lineSpacing = ff.GetLineSpacing(Gdiplus::FontStyleRegular);

		result.m_ascent = (((double)size * ascent) / emHeight) / m_scale;
		result.m_descent = (((double)size * descent) / emHeight) / m_scale;
		result.m_spacing = (((double)size * lineSpacing) / emHeight) / m_scale;
#else // GDI
		HFONT savedFont = SelectFont(m_hDC, f.get_HFONT());
		TEXTMETRIC tm = {};
		BOOL b = GetTextMetrics(m_hDC, &tm);
		COGS_ASSERT(b);
		SelectFont(m_hDC, savedFont);
		result.m_ascent = tm.tmAscent / m_scale;
		result.m_descent = tm.tmDescent / m_scale;
		result.m_spacing = (tm.tmHeight + tm.tmExternalLeading) / m_scale;
#endif
		
		return result;
	}

	canvas::size calc_text_bounds(const composite_string& s, const font& f) const
	{
		COGS_ASSERT(!!m_hDC);
		canvas::size result(0, 0);
		if (!!s)
		{
			string s2 = s.composite();

#if COGS_RENDER_GDIPLUS_FONTS
			Gdiplus::Graphics gfx(m_hDC);
			gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
			gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
			Gdiplus::SizeF maxSize(FLT_MAX, FLT_MAX);
			Gdiplus::SizeF resultSize;
			//Gdiplus::StringFormat stringFormat(0, LANG_NEUTRAL);
			Gdiplus::Status status = gfx.MeasureString(
				&(s2[0]),
				(int)(s2.get_length()),
				&f.m_GdiplusFont->m_font,
				maxSize,
				Gdiplus::StringFormat::GenericTypographic(),
				//&stringFormat,
				&resultSize, 0, 0);
			COGS_ASSERT(status == Gdiplus::Status::Ok);

			result = make_size(resultSize);
#else // GDI
			RECT r;
			r.right = r.bottom = 0;
			r.top = r.left = 0;
			HFONT savedFont = SelectFont(m_hDC, f.get_HFONT());
			int i = DrawText(m_hDC, &(s2[0]), (int)s2.get_length(), &r, DT_CALCRECT);
			SelectFont(m_hDC, savedFont);

			//// Because size may be truncated by 1 when translating to int coordinates, pad by 1
			//r.right++;
			//r.left++;

			result = make_size(r);
#endif
		}

		return result;
	}

public:
	explicit device_context(const ptr<rc_obj_base>& desc, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: object(desc),
		m_gdiPlusScope(gdiPlusScope)
	{ }

	explicit device_context(const ptr<rc_obj_base>& desc, HDC hDC, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: object(desc),
		m_hDC(hDC),
		m_gdiPlusScope(gdiPlusScope)
	{ }

	explicit device_context(const ptr<rc_obj_base>& desc, HDC hDC, double dpi, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: object(desc),
		m_hDC(hDC),
		m_gdiPlusScope(gdiPlusScope)
	{
		set_dpi(dpi);
	}

	explicit device_context(const ptr<rc_obj_base>& desc, double dpi, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: object(desc),
		m_gdiPlusScope(gdiPlusScope)
	{
		set_dpi(dpi);
	}

	~device_context()	// Does not free the DC here!
	{
		HRGN hRgn;
		while (!!m_savedClips.pop(hRgn))
		{
			if (!!hRgn)
				DeleteObject(hRgn);
		}
	}

	HDC& get_HDC() { return m_hDC; }
	const HDC& get_HDC() const { return m_hDC; }

	void set_HDC(const HDC& hDC) { m_hDC = hDC; }

	double get_scale() const { return m_scale; }

	virtual void fill(const canvas::bounds& b, const color& c, bool blendAlpha = true)
	{
		if (!blendAlpha || !c.is_fully_transparent())
		{
			BOUNDS b2 = make_BOUNDS(b);
			Gdiplus::SolidBrush solidBrush(Gdiplus::Color(c.get_alpha(), c.get_red(), c.get_green(), c.get_blue()));
			Gdiplus::Graphics gfx(m_hDC);
			if (!blendAlpha || c.is_opaque())
				gfx.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
			else
				gfx.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
			gfx.FillRectangle(&solidBrush, b2.pt.x, b2.pt.y, b2.sz.cx, b2.sz.cy);
		}
	}

	virtual void invert(const canvas::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		InvertRect(m_hDC, &r);
	}

	virtual void draw_line(const canvas::point& startPt, const canvas::point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
	{
		COGS_ASSERT(!!m_hDC);
		
		Gdiplus::REAL startX = (Gdiplus::REAL)(startPt.get_x() * m_scale);
		Gdiplus::REAL startY = (Gdiplus::REAL)(startPt.get_y() * m_scale);
		Gdiplus::REAL endX = (Gdiplus::REAL)(endPt.get_x() * m_scale);
		Gdiplus::REAL endY = (Gdiplus::REAL)(endPt.get_y() * m_scale);

		BYTE alph = c.get_alpha();
		BYTE rd = c.get_red();
		BYTE grn = c.get_green();
		BYTE bl = c.get_blue();

		Gdiplus::Pen pen(Gdiplus::Color(alph, rd, grn, bl), (Gdiplus::REAL)(width * m_scale));
		Gdiplus::Graphics gfx(m_hDC);

		if (!blendAlpha)
			gfx.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		else
			gfx.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
		gfx.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

		gfx.DrawLine(&pen, startX, startY, endX, endY);
	}

	// text and font primatives
	virtual rcref<canvas::font> load_font(const gfx::font& guiFont)
	{
		COGS_ASSERT(!!m_hDC);
		return rcnew(font, guiFont, this_rcref);
	}

	virtual gfx::font get_default_font() const
	{
		return *singleton<default_font>::get();
	}

	virtual void draw_text(const composite_string& s, const canvas::bounds& b, const rcptr<canvas::font>& f, const color& c = color::black)
	{
		COGS_ASSERT(!!m_hDC);
		if (!!s)
		{
			auto inner = [&](font& f2)
			{
				string s2 = s.composite();

				BYTE alph = c.get_alpha();
				BYTE rd = c.get_red();
				BYTE grn = c.get_green();
				BYTE bl = c.get_blue();

#if COGS_RENDER_GDIPLUS_FONTS
				Gdiplus::Graphics gfx(m_hDC);
				gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
				gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
				gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

				// TODO: Add rotation
				//gfx.RotateTransform(degrees);
				Gdiplus::SolidBrush solidBrush(Gdiplus::Color(alph, rd, grn, bl));
				Gdiplus::RectF destRectF(
					(Gdiplus::REAL)(b.get_x() * m_scale),
					(Gdiplus::REAL)(b.get_y() * m_scale),
					(Gdiplus::REAL)(b.get_width() * m_scale),
					(Gdiplus::REAL)(b.get_height() * m_scale));
				Gdiplus::Status status = gfx.DrawString(
					&(s2[0]),
					(int)(s2.get_length()),
					&f2.m_GdiplusFont->m_font,
					destRectF,
					Gdiplus::StringFormat::GenericTypographic(),
					&solidBrush);
				COGS_ASSERT(status == Gdiplus::Status::Ok);
#else // GDI
				HBRUSH brush = CreateSolidBrush(make_COLORREF(c));
				HGDIOBJ savedBrush = SelectObject(m_hDC, brush);
				HFONT savedFont = SelectFont(m_hDC, f2.get_HFONT());
				SetTextColor(m_hDC, make_COLORREF(c));
				UINT savedTextAlign = GetTextAlign(m_hDC);
				SetTextAlign(m_hDC, TA_TOP | TA_LEFT);
				SetBkMode(m_hDC, TRANSPARENT);
				RECT r = make_RECT(b);
				ExtTextOut(m_hDC, r.left, r.top, ETO_CLIPPED, &r, &(s2[0]), (int)(s.get_length()), NULL);
				SetTextAlign(m_hDC, savedTextAlign);
				SelectFont(m_hDC, savedFont);
				SelectObject(m_hDC, savedBrush);
				DeleteObject(brush);
#endif
			};

			if (!f)
				inner(*load_font(gfx::font()).template static_cast_to<font>());
			else
			{
				font* f4 = static_cast<font*>(f.get_ptr());
				f4->handle_dpi_change();
				inner(*f4);
			}
		}
	}
	
	virtual void draw_bitmap(const canvas::bitmap& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, bool blendAlpha = true);	
	virtual void draw_bitmask(const canvas::bitmask& src, const canvas::bounds& srcBounds, const canvas::bounds& dstBounds, const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true);
	virtual void mask_out(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false);
	virtual void draw_bitmap_with_bitmask(const canvas::bitmap& src, const bounds& srcBounds, const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false);

	virtual rcref<canvas::bitmap> create_bitmap(const canvas::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return create_bitmap(sz, fillColor, dip_dpi);
	}

	virtual rcref<canvas::bitmap> load_bitmap(const composite_string& location);

	virtual rcref<bitmask> create_bitmask(const canvas::size& sz, std::optional<bool> value = std::nullopt);

	virtual rcref<canvas::bitmask> load_bitmask(const composite_string& location);

	rcref<canvas::bitmap> create_bitmap(const canvas::size& sz, std::optional<color> fillColor, double dpi);

	// DPI must not change while a clip is in effect.
	// So, for UI drawing, clipping should only be used from a draw function (which is called within WM_PAINT)

	virtual void save_clip()
	{
		COGS_ASSERT(!!m_hDC);
		HRGN savedClip = CreateRectRgn(0, 0, 0, 0);
		int i = GetClipRgn(m_hDC, savedClip);
		if (!i)	// no clipping region right now
		{
			DeleteObject(savedClip);
			savedClip = 0;
		}
		m_savedClips.push(savedClip);
	}

	virtual void restore_clip()
	{
		COGS_ASSERT(!!m_hDC);
		HRGN savedClip = NULL;
		m_savedClips.pop(savedClip);
		SelectClipRgn(m_hDC, savedClip);
		if (!!savedClip)
			DeleteObject(savedClip);
	}

	canvas::bounds get_current_clip()
	{
		COGS_ASSERT(!!m_hDC);
		RECT r;
		GetClipBox(m_hDC, &r);
		return make_bounds(r);
	}

	virtual void clip_out(const canvas::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		ExcludeClipRect(m_hDC, r.left, r.top, r.right, r.bottom);
	}

	virtual void clip_to(const canvas::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		IntersectClipRect(m_hDC, r.left, r.top, r.right, r.bottom);
	}

	virtual bool is_unclipped(const canvas::bounds& b) const
	{
		COGS_ASSERT(!!m_hDC);
		if (!b.get_height() || !b.get_width())
			return false;

		RECT r = make_RECT(b);
		return (RectVisible(m_hDC, &r) == TRUE);
	}
	
	virtual double get_dpi() const
	{
		return m_dpi;
	}

	virtual void set_dpi(double dpi)
	{
		m_dpi = dpi;
		m_scale = dpi / dip_dpi;
	}

	// Only use when position is known to be 0,0
	SIZE make_SIZE(const canvas::size& sz) const
	{
		SIZE sz2;
		sz2.cx = (LONG)std::lround(m_scale * sz.get_width());
		sz2.cy = (LONG)std::lround(m_scale * sz.get_height());
		return sz2;
	}

	POINT make_POINT(const canvas::point& pt) const
	{
		POINT pt2;
		pt2.x = (LONG)std::lround(m_scale * pt.get_x());
		pt2.y = (LONG)std::lround(m_scale * pt.get_y());
		return pt2;
	}

	POINT make_POINT(const canvas::bounds& b) const
	{
		return make_POINT(b.calc_position());
	}

	RECT make_RECT(const canvas::bounds& b) const
	{
		canvas::bounds b1 = b.normalized();
		RECT r2;
		r2.left = (LONG)std::lround(m_scale * b1.get_position().get_x());
		r2.top = (LONG)std::lround(m_scale * b1.get_position().get_y());
		r2.right = (LONG)std::lround(m_scale * (b1.get_position().get_x() + b1.get_size().get_width()));
		r2.bottom = (LONG)std::lround(m_scale * (b1.get_position().get_y() + b1.get_size().get_height()));
		return r2;
	}

	// grows to include all pixels that could potentially overlap, so rounds position down and size up
	RECT make_invalid_RECT(const canvas::bounds& b) const
	{
		canvas::bounds b1 = b.normalized();
		RECT r2;
		r2.left = (LONG)std::floor(m_scale * b1.get_position().get_x());
		r2.top = (LONG)std::floor(m_scale * b1.get_position().get_y());
		r2.right = (LONG)std::ceil(m_scale * (b1.get_position().get_x() + b1.get_size().get_width()));
		r2.bottom = (LONG)std::ceil(m_scale * (b1.get_position().get_y() + b1.get_size().get_height()));
		return r2;
	}

	RECT make_RECT(const canvas::point& pt, const canvas::size& sz) const
	{
		RECT r2;
		r2.left = (LONG)std::lround(m_scale * pt.get_x());
		r2.top = (LONG)std::lround(m_scale * pt.get_y());
		r2.right = (LONG)std::lround(m_scale * (pt.get_x() + sz.get_width()));
		r2.bottom = (LONG)std::lround(m_scale * (pt.get_y() + sz.get_height()));
		return r2;
	}

	BOUNDS make_BOUNDS(const canvas::point& pt, const canvas::size& sz) const
	{
		RECT r = make_RECT(pt, sz);
		BOUNDS b;
		b.pt.x = r.left;
		b.pt.y = r.top;
		b.sz.cx = r.right - r.left;
		b.sz.cy = r.bottom - r.top;
		return b;
	}

	BOUNDS make_BOUNDS(const canvas::bounds& b) const
	{
		canvas::bounds b2 = b.normalized();
		return make_BOUNDS(b2.get_position(), b2.get_size());
	}

	double make_point(LONG i) const
	{
		return i / m_scale;
	}

	canvas::point make_point(const POINT& pt) const
	{
		return canvas::point(pt.x / m_scale, pt.y / m_scale);
	}

	canvas::point make_point(const RECT& r) const
	{
		return canvas::point((r.right - r.left) / m_scale, (r.bottom - r.top) / m_scale);
	}


	double make_size(LONG i) const
	{
		return i / m_scale;
	}

	canvas::size make_size(const SIZE& sz) const
	{
		return canvas::size(sz.cx / m_scale, sz.cy / m_scale);
	}

	canvas::size make_size(const RECT& r) const
	{
		return canvas::size((r.right - r.left) / m_scale, (r.bottom - r.top) / m_scale);
	}

	canvas::size make_size(const Gdiplus::SizeF& sz) const
	{
		return canvas::size(sz.Width / m_scale, sz.Height / m_scale);
	}


	canvas::bounds make_bounds(const POINT& pt, const SIZE& sz) const
	{
		return canvas::bounds(make_point(pt), make_size(sz));
	}

	canvas::bounds make_bounds(const BOUNDS& b) const
	{
		return canvas::bounds(make_point(b.pt), make_size(b.sz));
	}

	canvas::bounds make_bounds(const RECT& r) const
	{
		POINT pt = { r.left, r.right };
		SIZE sz = { r.right - r.left, r.bottom - r.top };
		return make_bounds(pt, sz);
	}


	static BOUNDS make_BOUNDS(const RECT& r)
	{
		BOUNDS b;
		b.pt.x = r.left;
		b.pt.y = r.right;
		b.sz.cx = r.right - r.left;
		b.sz.cy = r.bottom - r.top;
		return b;
	}

	static COLORREF make_COLORREF(const color& c)
	{
		return RGB(c.get_red(), c.get_green(), c.get_blue());
	}
};


}
}
}
}


#include "cogs/os/gfx/bitmap.hpp"


#endif
