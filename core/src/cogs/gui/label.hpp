//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_LABEL
#define COGS_HEADER_GUI_LABEL


#include "cogs/gfx/color.hpp"
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
	volatile rcref<gfx::font_parameters_list> m_font;
	volatile transactable<std::optional<color>> m_textColor;
	bool m_useLineHeight;

	delegated_dependency_property<composite_string> m_textProperty;
	delegated_dependency_property<gfx::font_parameters_list> m_fontProperty;
	delegated_dependency_property<std::optional<color>> m_colorProperty;

	rcptr<gfx::font> m_cachedFont;
	size m_textExtent;

public:
	struct options
	{
		composite_string text;
		gfx::font_parameters_list font;
		std::optional<color> textColor;
		bool useLineHeight;
		frame_list frames;
	};

	explicit label(options&& o)
		: pane({
			.frames = std::move(o.frames)
		}),
		m_text(std::move(o.text)),
		m_font(rcnew(gfx::font_parameters_list)(std::move(o.font))),
		m_textColor(transactable<std::optional<color>>::construct_embedded_t(), o.textColor),
		m_useLineHeight(o.useLineHeight),
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
			rcref<gfx::font_parameters_list> tmp = m_font;
			return *tmp;
		}, [this](const gfx::font_parameters_list& f)
		{
			m_font = rcnew(gfx::font_parameters_list)(f);
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
			return *(m_textColor.begin_read());;
		}, [this](const std::optional<color>& c)
		{
			std::optional<color> newColor = c;
			std::optional<color> oldColor = newColor;
			m_textColor.swap_contents(oldColor);
			m_colorProperty.set_complete(newColor != oldColor);
			invalidate(get_size());
		})
	{ }

	virtual void installing()
	{
		rcref<gfx::font_parameters_list> tmp = m_font;
		m_cachedFont = load_font(*tmp);
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

	virtual void calculating_range()
	{
		pane::calculating_range();
		composite_string txt = m_text;
		rcptr<gfx::font> f = m_cachedFont;
		m_textExtent = f->calc_text_bounds(txt);
		if (m_useLineHeight)
		{
			font::metrics fm = f->get_metrics();
			double height = m_textExtent.get_height();
			double lineCount = height / fm.spacing;
			double lineCount2 = std::floor(lineCount);
			if (lineCount != lineCount2)
				lineCount2++;
			m_textExtent.get_height() = lineCount2 * fm.spacing;
		}
	}

	virtual range get_range() const { return range(m_textExtent); }
	virtual std::optional<size> get_default_size() const { return m_textExtent; }

	virtual void drawing()
	{
		composite_string txt(m_text);
		rcptr<gfx::font> f = m_cachedFont;
		std::optional<color> c = *(m_textColor.begin_read());
		color c2 = c.has_value() ? *c : get_default_label_foreground_color();
		draw_text(txt, m_textExtent, f.dereference(), c2);
	}

	rcref<dependency_property<composite_string>> get_text_property() { return get_self_rcref(&m_textProperty); }
	rcref<dependency_property<gfx::font_parameters_list>> get_font_property() { return get_self_rcref(&m_fontProperty); }
	rcref<dependency_property<std::optional<color>>> get_color_property() { return get_self_rcref(&m_colorProperty); }

	std::optional<color> get_text_color() const
	{
		return m_colorProperty.get();
	}

	void set_text_color(const std::optional<color>& c)
	{
		m_colorProperty.set(c);
	}

};


}
}


#endif
