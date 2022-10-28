//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_GFX_DEVICE_CONTEXT
#define COGS_HEADER_OS_GFX_DEVICE_CONTEXT

#define COGS_GDI 1


#include "cogs/env.hpp"
#include "cogs/collections/container_stack.hpp"
#include "cogs/collections/set.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/mem/auto_handle.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"


namespace cogs {
namespace os {

inline bool operator!(const SIZE& sz) { return !sz.cx || !sz.cy; }
inline bool operator==(const SIZE& sz1, const SIZE& sz2) { return (sz1.cx == sz2.cx) && (sz1.cy == sz2.cy); }
inline bool operator!=(const SIZE& sz1, const SIZE& sz2) { return (sz1.cx != sz2.cx) || (sz1.cy != sz2.cy); }
inline SIZE operator+(const SIZE& sz1, const SIZE& sz2) { return SIZE{ sz1.cx + sz2.cx, sz1.cy + sz2.cy }; }
inline SIZE operator-(const SIZE& sz1, const SIZE& sz2) { return SIZE{ sz1.cx - sz2.cx, sz1.cy - sz2.cy }; }
inline SIZE& operator+=(SIZE& sz1, const SIZE& sz2) { sz1.cx += sz2.cx; sz1.cy += sz2.cy; return sz1; }
inline SIZE& operator-=(SIZE& sz1, const SIZE& sz2) { sz1.cx -= sz2.cx; sz1.cy -= sz2.cy; return sz1; }

inline bool operator!(const POINT& pt) { return !pt.x && !pt.y; }
inline bool operator==(const POINT& pt1, const POINT& pt2) { return (pt1.x == pt2.x) && (pt1.y == pt2.y); }
inline bool operator!=(const POINT& pt1, const POINT& pt2) { return (pt1.x != pt2.x) || (pt1.y != pt2.y); }
inline POINT operator+(const POINT& pt1, const POINT& pt2) { return POINT{ pt1.x + pt2.x, pt1.y + pt2.y }; }
inline POINT operator-(const POINT& pt1, const POINT& pt2) { return POINT{ pt1.x - pt2.x, pt1.y - pt2.y }; }
inline POINT& operator+=(POINT& pt1, const POINT& pt2) { pt1.x += pt2.x; pt1.y += pt2.y; return pt1; }
inline POINT& operator-=(POINT& pt1, const POINT& pt2) { pt1.x -= pt2.x; pt1.y -= pt2.y; return pt1; }

inline POINT make_POINT(const RECT& r) { return POINT{ r.left, r.top }; }
inline SIZE make_SIZE(const RECT& r) { return SIZE{ r.right - r.left, r.bottom - r.top }; }

struct BOUNDS
{
	POINT pt;
	SIZE sz;
};

inline BOUNDS make_BOUNDS(const RECT& r) { return BOUNDS{ make_POINT(r), make_SIZE(r) }; }

inline COLORREF make_COLORREF(const color& c) { return RGB(c.get_red(), c.get_green(), c.get_blue()); }
inline color from_COLORREF(COLORREF c) { return color(GetRValue(c), GetGValue(c), GetBValue(c)); }


namespace gdi {


// GDI device_context canvas.
// Contains a current HDC, but does not own it.
// To avoid multiply deriving from device_context, encapsulate gdi::device_context by value.
class device_context;

class bitmap; // forward declared here, defined in bitmap.hpp

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


#define COGS_RENDER_GDIPLUS_FONTS 0 // If 1, use GDI+ font rendering.  If 0, use GDI font rendering.  GDI fonts are preferred.
// "GDI text rendering typically offers better performance and more accurate text measuring than GDI+."
// - https://docs.microsoft.com/en-us/dotnet/framework/winforms/advanced/how-to-draw-text-with-gdi



// device contexts are assumed to be manipulated only with the same UI thread.  (Does not call GdiFlush())

class device_context : public object, public gfx::canvas
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
	HDC m_hDC = NULL;
	rcref<gdi_plus_scope> m_gdiPlusScope;
	container_stack<HRGN> m_savedClips;
	double m_dpi = dip_dpi;
	double m_scale = 1.0;

protected:
	void draw_bitmap_inner(gdi::bitmap* bmp, const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, bool blendAlpha);
	void draw_bitmask_inner(const gdi::bitmap& src, const BOUNDS& srcBounds, const BOUNDS& dstBounds, const color& fore, const color& back, bool blendForeAlpha, bool blendBackAlpha);

public:
	class font : public gfx::font
	{
	private:
		class list : public nonvolatile_set<composite_string, true, case_insensitive_comparator<composite_string> >
		{
		private:
			static int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC*, DWORD, LPARAM lParam)
			{
				auto fontList = (list*)lParam;
				fontList->insert_unique(lpelfe->lfFaceName);
				return 1;
			}

		protected:
			list()
			{
				// Load fonts.  (Fonts added or removed will not be observed until relaunch)
				LOGFONT logFont = {};
				logFont.lfCharSet = DEFAULT_CHARSET;
				HDC hdc = GetDC(NULL);
				EnumFontFamiliesEx(hdc, &logFont, &EnumFontFamExProc, (LPARAM)this, 0);
				ReleaseDC(NULL, hdc);
			}
		};

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
		gfx::font_parameters_list m_fontParmsLists;
		mutable std::unique_ptr<gdiplus_font> m_GdiplusFont;
		mutable auto_HFONT m_HFONT;
		mutable double m_savedDpi;

		friend class device_context;

		void update_font(double dpi) const
		{
			COGS_ASSERT(!!m_deviceContext->m_hDC);
			gfx::font_parameters default_font_params = get_default_font();

			auto update = [&](const gfx::font_parameters& fp)
			{
				int fontStyle = (int)Gdiplus::FontStyleRegular;
				if (fp.isItalic)
					fontStyle |= (int)Gdiplus::FontStyleItalic;
				if (fp.isUnderlined)
					fontStyle |= (int)Gdiplus::FontStyleUnderline;
				if (fp.isBold)
					fontStyle |= (int)Gdiplus::FontStyleBold;
				if (fp.isStrikeOut)
					fontStyle |= (int)Gdiplus::FontStyleStrikeout;

				double pt = fp.pointSize;
				if (pt == 0)
					pt = default_font_params.pointSize;

				// convert pt to pixels
				if (!!dpi)
					m_savedDpi = dpi;
				else
					m_savedDpi = dip_dpi;
				double pixels = pixels = (dpi * pt) / 72;

				m_GdiplusFont = std::make_unique<gdiplus_font>(
					fp.fontName.composite().cstr(),
					(Gdiplus::REAL)pixels,
					fontStyle,
					Gdiplus::UnitPixel);

				Gdiplus::Graphics gfx(m_deviceContext->m_hDC);
				LOGFONT logFont = {};
				Gdiplus::Status status = m_GdiplusFont->m_font.GetLogFontW(&gfx, &logFont);
				COGS_ASSERT(status == Gdiplus::Status::Ok);
				(void)status;
				m_HFONT = CreateFontIndirect(&logFont);
			};

			for (const auto& font_params : m_fontParmsLists)
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

				auto itor = singleton<list>::get()->find_equal(font_params.fontName);
				if (!itor)
					continue;
				update(font_params);
				return;
			}
			if (!m_fontParmsLists.size())
				update(default_font_params);
			else
			{
				// If failing to find the font, use the first entry, but with the default font name.
				gfx::font_parameters params = *m_fontParmsLists.get_first();
				params.fontName = default_font_params.fontName;
				update(params);
			}
		}

		void handle_dpi_change() const
		{
			double newDpi = m_deviceContext->get_dpi();
			if (newDpi != m_savedDpi)
				update_font(newDpi);
		}

	public:
		static gfx::font_parameters get_default_font()
		{
			gfx::font_parameters fp;

			// Load default font
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dip_dpi);

			string s = ncm.lfMessageFont.lfFaceName;
			fp.fontName = s;
			fp.pointSize = -ncm.lfMessageFont.lfHeight;
			fp.isItalic = ncm.lfMessageFont.lfItalic != 0;
			fp.isBold = ncm.lfMessageFont.lfWeight >= 700;
			fp.isUnderlined = ncm.lfMessageFont.lfUnderline != 0;
			fp.isStrikeOut = ncm.lfMessageFont.lfStrikeOut != 0;
			return fp;
		}

		static string get_default_font_name()
		{
			// Load default font
			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, dip_dpi);

			return ncm.lfMessageFont.lfFaceName;
		}


		font(const gfx::font_parameters_list& fontParmsLists, const rcref<device_context>& dc)
			: m_deviceContext(dc),
			m_fontParmsLists(fontParmsLists)
		{
			update_font(m_deviceContext->get_dpi());
		}

		HFONT get_HFONT() const { return *m_HFONT; }

		virtual gfx::font::metrics get_metrics() const
		{
			handle_dpi_change();
			return m_deviceContext->get_font_metrics(*this);
		}

		virtual gfx::size calc_text_bounds(const composite_string& s) const
		{
			handle_dpi_change();
			return m_deviceContext->calc_text_bounds(s, *this);
		}
	};

private:
	gfx::font::metrics get_font_metrics(const font& f) const
	{
		COGS_ASSERT(!!m_hDC);
		gfx::font::metrics result = {};

#if COGS_RENDER_GDIPLUS_FONTS
		Gdiplus::FontFamily ff;
		Gdiplus::Status status = f.m_GdiplusFont->m_font.GetFamily(&ff);
		COGS_ASSERT(status == Gdiplus::Status::Ok);

		Gdiplus::REAL size = f.m_GdiplusFont->m_font.GetSize();
		int emHeight = ff.GetEmHeight(Gdiplus::FontStyleRegular);
		int ascent = ff.GetCellAscent(Gdiplus::FontStyleRegular);
		int descent = ff.GetCellDescent(Gdiplus::FontStyleRegular);
		int lineSpacing = ff.GetLineSpacing(Gdiplus::FontStyleRegular);

		result.ascent = (((double)size * ascent) / emHeight) / m_scale;
		result.descent = (((double)size * descent) / emHeight) / m_scale;
		result.spacing = (((double)size * lineSpacing) / emHeight) / m_scale;
		result.height = result.ascent + result.descent;
		result.leading = result.spacing - result.height;
#else // GDI
		HFONT savedFont = SelectFont(m_hDC, f.get_HFONT());
		TEXTMETRIC tm = {};
		GetTextMetrics(m_hDC, &tm);
		SelectFont(m_hDC, savedFont);
		result.ascent = tm.tmAscent / m_scale;
		result.descent = tm.tmDescent / m_scale;
		result.leading = tm.tmExternalLeading / m_scale;
		result.height = result.ascent + result.descent;
		result.spacing = result.height + result.leading;
#endif
		return result;
	}

	gfx::size calc_text_bounds(const composite_string& s, const font& f) const
	{
		COGS_ASSERT(!!m_hDC);
		gfx::size result(0, 0);
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
			DrawText(m_hDC, &(s2[0]), (int)s2.get_length(), &r, DT_CALCRECT);
			SelectFont(m_hDC, savedFont);
			result = make_size(r);
#endif
		}

		return result;
	}

public:
	explicit device_context(const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: m_gdiPlusScope(gdiPlusScope)
	{ }

	explicit device_context(HDC hDC, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: m_hDC(hDC),
		m_gdiPlusScope(gdiPlusScope)
	{ }

	device_context(HDC hDC, double dpi, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: m_hDC(hDC),
		m_gdiPlusScope(gdiPlusScope)
	{
		set_dpi(dpi);
	}

	explicit device_context(double dpi, const rcref<gdi_plus_scope>& gdiPlusScope = get_default_gdi_plus_scope())
		: m_gdiPlusScope(gdiPlusScope)
	{
		set_dpi(dpi);
	}

	~device_context() // Does not free the DC here!
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

	virtual void fill(const gfx::bounds& b, const color& c, bool blendAlpha = true)
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
			gfx.FillRectangle(&solidBrush, (INT)b2.pt.x, (INT)b2.pt.y, (INT)b2.sz.cx, (INT)b2.sz.cy);
		}
	}

	virtual void invert(const gfx::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		InvertRect(m_hDC, &r);
	}

	virtual void draw_line(const gfx::point& startPt, const gfx::point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true)
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
	virtual rcref<gfx::font> load_font(const gfx::font_parameters_list& f)
	{
		COGS_ASSERT(!!m_hDC);
		return rcnew(font)(f, this_rcref);
	}

	virtual string get_default_font_name() const
	{
		return font::get_default_font_name();
	}

	static gfx::font_parameters get_default_font()
	{
		return font::get_default_font();
	}

	virtual void draw_text(const composite_string& s, const gfx::bounds& b, const rcptr<gfx::font>& f, const color& c = color::constant::black)
	{
		COGS_ASSERT(!!m_hDC);
		if (!!s)
		{
			auto inner = [&](font& f2)
			{
				string s2 = s.composite();

#if COGS_RENDER_GDIPLUS_FONTS
				BYTE alph = c.get_alpha();
				BYTE rd = c.get_red();
				BYTE grn = c.get_green();
				BYTE bl = c.get_blue();

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
				inner(*load_font(gfx::font_parameters_list()).template static_cast_to<font>());
			else
			{
				font* f4 = static_cast<font*>(f.get_ptr());
				f4->handle_dpi_change();
				inner(*f4);
			}
		}
	}

	virtual void draw_bitmap(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bounds& dstBounds, bool blendAlpha = true);
	virtual void draw_bitmask(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true);
	virtual void mask_out(const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool inverted = false);
	virtual void draw_bitmap_with_bitmask(const gfx::bitmap& src, const gfx::bounds& srcBounds, const gfx::bitmask& msk, const gfx::bounds& mskBounds, const gfx::bounds& dstBounds, bool blendAlpha = true, bool inverted = false);

	virtual rcref<gfx::bitmap> create_bitmap(const gfx::size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return create_bitmap(sz, fillColor, dip_dpi);
	}

	virtual rcref<gfx::bitmap> load_bitmap(const composite_string& location);

	virtual rcref<gfx::bitmask> create_bitmask(const gfx::size& sz, std::optional<bool> value = std::nullopt);

	virtual rcref<gfx::bitmask> load_bitmask(const composite_string& location);

	rcref<gfx::bitmap> create_bitmap(const gfx::size& sz, std::optional<color> fillColor, double dpi);

	// DPI must not change while a clip is in effect.
	// So, for UI drawing, clipping should only be used from a draw function (which is called within WM_PAINT)

	virtual void save_clip()
	{
		COGS_ASSERT(!!m_hDC);
		HRGN savedClip = CreateRectRgn(0, 0, 0, 0);
		int i = GetClipRgn(m_hDC, savedClip);
		if (!i) // no clipping region right now
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

	gfx::bounds get_current_clip()
	{
		COGS_ASSERT(!!m_hDC);
		RECT r;
		GetClipBox(m_hDC, &r);
		return make_bounds(r);
	}

	virtual void clip_out(const gfx::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		ExcludeClipRect(m_hDC, r.left, r.top, r.right, r.bottom);
	}

	virtual void clip_to(const gfx::bounds& b)
	{
		COGS_ASSERT(!!m_hDC);
		RECT r = make_RECT(b);
		IntersectClipRect(m_hDC, r.left, r.top, r.right, r.bottom);
	}

	virtual bool is_unclipped(const gfx::bounds& b) const
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

	LONG make_SIZE(double i) const
	{
		return (LONG)std::lround(m_scale * i);
	}

	// Only use when position is known to be 0,0
	SIZE make_SIZE(const gfx::size& sz) const
	{
		return SIZE{ (LONG)std::lround(m_scale * sz.get_width()), (LONG)std::lround(m_scale * sz.get_height()) };
	}

	LONG make_POINT(double i) const
	{
		return (LONG)std::lround(m_scale * i);
	}

	POINT make_POINT(const gfx::point& pt) const
	{
		return POINT{ (LONG)std::lround(m_scale * pt.get_x()), (LONG)std::lround(m_scale * pt.get_y()) };
	}

	POINT make_POINT(const gfx::bounds& b) const
	{
		return make_POINT(b.calc_position());
	}

	RECT make_RECT(const gfx::bounds& b) const
	{
		gfx::bounds b2 = b.normalized();
		return make_RECT(b2.get_position(), b2.get_size());
	}

	RECT make_RECT(const gfx::point& pt, const gfx::size& sz) const
	{
		RECT r2;
		r2.left = (LONG)std::lround(m_scale * pt.get_x());
		r2.top = (LONG)std::lround(m_scale * pt.get_y());
		r2.right = (LONG)std::lround(m_scale * (pt.get_x() + sz.get_width()));
		r2.bottom = (LONG)std::lround(m_scale * (pt.get_y() + sz.get_height()));
		return r2;
	}

	// grows to include all pixels that could potentially overlap, so rounds position down and size up
	RECT make_invalid_RECT(const gfx::bounds& b) const
	{
		gfx::bounds b2 = b.normalized();
		return make_invalid_RECT(b2.get_position(), b2.get_size());
	}

	RECT make_invalid_RECT(const gfx::point& pt, const gfx::size& sz) const
	{
		RECT r2;
		r2.left = (LONG)std::floor(m_scale * pt.get_x());
		r2.top = (LONG)std::floor(m_scale * pt.get_y());
		r2.right = (LONG)std::ceil(m_scale * (pt.get_x() + sz.get_width()));
		r2.bottom = (LONG)std::ceil(m_scale * (pt.get_y() + sz.get_height()));
		return r2;
	}

	BOUNDS make_BOUNDS(const gfx::point& pt, const gfx::size& sz) const
	{
		return os::make_BOUNDS(make_RECT(pt, sz));
	}

	BOUNDS make_BOUNDS(const gfx::bounds& b) const
	{
		return os::make_BOUNDS(make_RECT(b));
	}

	double make_point(LONG i) const
	{
		return i / m_scale;
	}

	gfx::point make_point(const POINT& pt) const
	{
		return gfx::point(pt.x / m_scale, pt.y / m_scale);
	}

	gfx::point make_point(const RECT& r) const
	{
		return gfx::point(r.left / m_scale, r.top / m_scale);
	}

	double make_size(LONG i) const
	{
		return i / m_scale;
	}

	gfx::size make_size(const SIZE& sz) const
	{
		return gfx::size(sz.cx / m_scale, sz.cy / m_scale);
	}

	gfx::size make_size(const RECT& r) const
	{
		SIZE sz = { r.right - r.left, r.bottom - r.top };
		return make_size(sz);
	}

	gfx::size make_size(const Gdiplus::SizeF& sz) const
	{
		return gfx::size(sz.Width / m_scale, sz.Height / m_scale);
	}


	gfx::bounds make_bounds(const POINT& pt, const SIZE& sz) const
	{
		return gfx::bounds(make_point(pt), make_size(sz));
	}

	gfx::bounds make_bounds(const BOUNDS& b) const
	{
		return gfx::bounds(make_point(b.pt), make_size(b.sz));
	}

	gfx::bounds make_bounds(const RECT& r) const
	{
		return make_bounds(os::make_POINT(r), os::make_SIZE(r));
	}
};


}
}
}


#include "cogs/os/gfx/bitmap.hpp"


#endif
