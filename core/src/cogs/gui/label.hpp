//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_LABEL
#define COGS_HEADER_GUI_LABEL


#include "cogs/gfx/color.hpp"
#include "cogs/gfx/font.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/dependency_property.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief A pane that displays static text
class label : public pane
{
private:

	// Because all dispatching for properties happens using pane::dispatch(), which serializes,
	// it's sufficient to make each property's data volatile (separately, and not all part of
	// a single transacted state).  This is because sets() will never happen simultaneously,
	// though get() may occur concurrently with set()'s.

	volatile composite_string m_text;
	volatile gfx::font m_font;
	volatile color m_textColor;
	bool m_useLineHeight;

	delegated_dependency_property<composite_string> m_textProperty;
	delegated_dependency_property<gfx::font> m_fontProperty;
	delegated_dependency_property<color> m_colorProperty;

	rcptr<canvas::font> m_cachedFont;
	size m_textExtent;

public:
	explicit label(const composite_string& text,
		const gfx::font& fnt = gfx::font(),
		const color& c = color::constant::black,
		bool useLineHeight = true,
		const std::initializer_list<rcref<frame> >& frames = {})
		: pane(frames),
		m_text(text),
		m_font(fnt),
		m_textColor(c),
		m_useLineHeight(useLineHeight),
		m_textProperty(*this, [this]()
		{
			return m_text;
		}, [this](const composite_string& s)
		{
			m_text = s;
			m_textProperty.set_complete(true);
			if (is_installed())
			{
				invalidate(get_size());
				recompose();
			}
		}),
		m_fontProperty(*this, [this]()
		{
			return m_font;
		}, [this](const gfx::font& f)
		{
			m_font = f;
			m_fontProperty.set_complete(true);
			if (is_installed())
			{
				m_cachedFont = load_font(f);
				invalidate(get_size());
				recompose();
			}
		}),
		m_colorProperty(*this, [this]()
		{
			return m_textColor;
		}, [this](color c)
		{
			m_textColor = c;
			m_colorProperty.set_complete(true);
			invalidate(get_size());
		})
	{ }

	label(const composite_string& text,
		const color& c,
		bool useLineHeight = true,
		const std::initializer_list<rcref<frame> >& frames = {})
		: label(text, gfx::font(), c, useLineHeight, frames)
	{ }

	label(const composite_string& text,
		const gfx::font& fnt,
		bool useLineHeight,
		const std::initializer_list<rcref<frame> >& frames = {})
		: label(text, fnt, color::constant::black, useLineHeight, frames)
	{ }

	label(const composite_string& text,
		const gfx::font& fnt,
		const color& c,
		const std::initializer_list<rcref<frame> >& frames)
		: label(text, fnt, c, true, frames)
	{ }


	label(const composite_string& text,
		bool useLineHeight,
		const std::initializer_list<rcref<frame> >& frames = {})
		: label(text, gfx::font(), color::constant::black, useLineHeight, frames)
	{ }

	label(const composite_string& text,
		const color& c,
		const std::initializer_list<rcref<frame> >& frames)
		: label(text, gfx::font(), c, true, frames)
	{ }

	label(const composite_string& text,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
		: label(text, fnt, color::constant::black, true, frames)
	{ }

	label(const composite_string& text,
		const std::initializer_list<rcref<frame> >& frames)
		: label(text, gfx::font(), color::constant::black, true, frames)
	{ }


	virtual void installing()
	{
		gfx::font f = m_font;
		m_cachedFont = load_font(f);
		pane::installing();
	}

	virtual void uninstalling()
	{
		m_cachedFont.release();
		pane::uninstalling();
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		pane::reshape(b, oldOrigin);
		invalidate(get_size());
	}

	virtual void calculate_range()
	{
		pane::calculate_range();
		composite_string txt = m_text;
		rcptr<canvas::font> f = m_cachedFont;
		m_textExtent = f->calc_text_bounds(txt);
		if (m_useLineHeight)
		{
			font::metrics fm = f->get_metrics();
			double height = m_textExtent.get_height();
			double lineCount = height / fm.m_spacing;
			double lineCount2 = std::floor(lineCount);
			if (lineCount != lineCount2)
				lineCount2++;
			m_textExtent.get_height() = lineCount2 * fm.m_spacing;
		}
	}

	virtual range get_range() const { return range(m_textExtent); }
	virtual size get_default_size() const { return m_textExtent; }

	virtual void drawing()
	{
		composite_string txt(m_text);
		rcptr<canvas::font> f = m_cachedFont;
		color c = m_textColor;
		draw_text(txt, m_textExtent, f.dereference(), c);
	}

	rcref<dependency_property<composite_string> > get_text_property() { return get_self_rcref(&m_textProperty); }
	rcref<dependency_property<gfx::font> > get_font_property() { return get_self_rcref(&m_fontProperty); }
	rcref<dependency_property<color> > get_color_property() { return get_self_rcref(&m_colorProperty); }

	color get_text_color() const
	{
		return m_colorProperty.get();
	}

	void set_text_color(const color& c)
	{
		m_colorProperty.set(c);
	}

};


}
}


#endif
