//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_TEXTEDITOR
#define COGS_HEADER_GUI_TEXTEDITOR


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI text editor.
class text_editor_interface
{
public:
	virtual composite_string get_text() const = 0;

	virtual void set_text(const composite_string&) = 0;
	virtual void set_max_length(size_t) = 0;
	virtual void set_enabled(bool) = 0;
	virtual void set_font(const gfx::font&) = 0;

	virtual void set_text_color(const color& c) = 0;
};


/// @ingroup GUI
/// @brief A GUI text editor
class text_editor : public pane_bridge
{
private:
	composite_string m_text;
	size_t m_maxLength;
	bool m_isEnabled;
	bool m_isMultiLine;
	gfx::font m_font;
	color m_textColor;
	rcptr<text_editor_interface> m_nativeTextEditor;

public:
	text_editor(const composite_string& text,
		const color& textColor = color::constant::black,
		bool isMultiLine = false,
		bool isEnabled = true,
		const gfx::font& fnt = gfx::font(),
		size_t maxLength = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: pane_bridge(frames),
		m_text(text),
		m_maxLength(maxLength),
		m_isEnabled(isEnabled),
		m_isMultiLine(isMultiLine),
		m_font(fnt),
		m_textColor(textColor)
	{ }


	text_editor(const composite_string& text,
		bool isMultiLine,
		bool isEnabled = true,
		const gfx::font& fnt = gfx::font(),
		size_t maxLength = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, color::constant::black, isMultiLine, isEnabled, fnt, maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		const gfx::font& fnt,
		size_t maxLength = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, color::constant::black, isMultiLine, true, fnt, maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		const gfx::font& fnt,
		size_t maxLength = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, textColor, false, true, fnt, maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		bool isEnabled,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, textColor, isMultiLine, isEnabled, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, textColor, isMultiLine, true, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		bool isEnabled,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, textColor, isMultiLine, isEnabled, fnt, 0, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, textColor, isMultiLine, true, fnt, 0, frames)
	{ }


	text_editor(const composite_string& text,
		const gfx::font& fnt,
		size_t maxLength = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, color::constant::black, false, true, fnt, maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		bool isEnabled,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
	: text_editor(text, color::constant::black, isMultiLine, isEnabled, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, color::constant::black, isMultiLine, true, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		bool isEnabled,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
	: text_editor(text, color::constant::black, isMultiLine, isEnabled, fnt, 0, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, color::constant::black, isMultiLine, true, fnt, 0, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, textColor, false, true, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string & text,
		const color & textColor,
		const gfx::font & fnt,
		const std::initializer_list<rcref<frame> > & frames)
		: text_editor(text, textColor, false, true, fnt, 0, frames)
	{ }


	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		bool isEnabled,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, textColor, isMultiLine, isEnabled, gfx::font(), 0, frames)
	{ }

	text_editor(const composite_string& text,
		const color& textColor,
		bool isMultiLine,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, textColor, isMultiLine, true, gfx::font(), 0, frames)
	{ }

	text_editor(const composite_string& text,
		size_t maxLength,
		const std::initializer_list<rcref<frame> >& frames = {})
		: text_editor(text, color::constant::black, false, true, gfx::font(), maxLength, frames)
	{ }

	text_editor(const composite_string& text,
		const gfx::font& fnt,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, color::constant::black, false, true, fnt, 0, frames)
	{ }

	text_editor(const composite_string& text,
		bool isMultiLine,
		bool isEnabled,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, color::constant::black, isMultiLine, isEnabled, gfx::font(), 0, frames)
	{ }

	text_editor(const composite_string & text,
		bool isMultiLine,
		const std::initializer_list<rcref<frame> > & frames)
		: text_editor(text, color::constant::black, isMultiLine, true, gfx::font(), 0, frames)
	{ }

	text_editor(const composite_string& text,
		const std::initializer_list<rcref<frame> >& frames)
		: text_editor(text, color::constant::black, false, true, gfx::font(), 0, frames)
	{ }

	virtual void installing()
	{
		auto nativeTextEditor = get_subsystem()->create_text_editor();
		m_nativeTextEditor = std::move(nativeTextEditor.second);
		pane_bridge::install_bridged(std::move(nativeTextEditor.first));
	}

	virtual void uninstalling()
	{
		if (!!m_nativeTextEditor)
			m_text = m_nativeTextEditor->get_text();
		pane_bridge::uninstalling();
		m_nativeTextEditor.release();
	}

	composite_string get_text() const
	{
		return m_text;
	}

	void set_text(const composite_string& text)
	{
		m_text = text;
		if (!!m_nativeTextEditor)
			m_nativeTextEditor->set_text(text);
	}

	size_t get_max_length() const { return m_maxLength; }
	void set_max_length(size_t maxLength)
	{
		m_maxLength = maxLength;
		if (!!m_nativeTextEditor)
			m_nativeTextEditor->set_max_length(maxLength);
	}

	bool is_enabled() const { return m_isEnabled; }
	void set_enabled(bool b)
	{
		m_isEnabled = b;
		if (!!m_nativeTextEditor)
			m_nativeTextEditor->set_enabled(b);
	}

	bool is_multi_line() const { return m_isMultiLine; }

	const gfx::font& get_font() const { return m_font; }
	void set_font(const gfx::font& fnt)
	{
		m_font = fnt;
		if (!!m_nativeTextEditor)
		{
			m_nativeTextEditor->set_font(fnt);
			recompose();
		}
	}

	color get_text_color() const
	{
		return m_textColor;
	}

	void set_text_color(const color& c)
	{
		m_textColor = c;
		if (!!m_nativeTextEditor)
		{
			m_nativeTextEditor->set_text_color(c);
			invalidate(get_size());
		}
	}
};


}
}


#endif
