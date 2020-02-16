//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GFX_FONT
#define COGS_HEADER_GFX_FONT


#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace gfx {


// cogs::gfx::canvas::font is a base class for platform-specific fonts (size is embedded).

#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified

// cogs::gui:::font is platform-independent, and contains a priority-sorted list of fonts.
// When applied to a particular platform, the first font that can be matched is loaded.

/// @ingroup Graphics
/// @brief A font
class font // copyable by value
{
private:
	class description
	{
	public:
		vector<composite_string> m_fontNames;
		double m_pointSize = 0.0; // 0 point size implies default point size
		bool m_isItalic = false;
		bool m_isBold = false;
		bool m_isUnderlined = false;
		bool m_isStrikeOut = false;

		description() // no font name(s) implies default font
		{ }

		description(const description& src)
			: m_fontNames(src.m_fontNames),
			m_pointSize(src.m_pointSize),
			m_isItalic(src.m_isItalic),
			m_isBold(src.m_isBold),
			m_isUnderlined(src.m_isUnderlined),
			m_isStrikeOut(src.m_isStrikeOut)
		{ }

		description(double pointSize) // no font name(s) implies default font
			: m_pointSize(pointSize)
		{ }

		description(const vector<composite_string>& fontNames, double pointSize)
			: m_fontNames(fontNames),
			m_pointSize(pointSize),
			m_isItalic(false),
			m_isBold(false),
			m_isUnderlined(false),
			m_isStrikeOut(false)
		{ }

		description(const composite_string& fontName, double pointSize)
			: m_pointSize(pointSize)
		{
			m_fontNames.prepend(1, fontName);
		}

		bool operator==(const description& cmp) const
		{
			return m_pointSize == cmp.m_pointSize &&
				m_isItalic == cmp.m_isItalic &&
				m_isBold == cmp.m_isBold &&
				m_isUnderlined == cmp.m_isUnderlined &&
				m_isStrikeOut == cmp.m_isStrikeOut &&
				m_fontNames == cmp.m_fontNames; // Does not account for font names in a different order
		}

		bool operator!=(const description& cmp) const { return !operator==(cmp); }
	};

	typedef transactable<description> transactable_t;
	typedef transactable_t::read_token read_token;
	typedef transactable_t::write_token write_token;

	transactable_t m_contents;

public:
	explicit font()
	{ }

	font(const font& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{ }

	font(const volatile font& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{ }

	explicit font(double pointSize)
		: m_contents(typename transactable_t::construct_embedded_t(), description(pointSize))
	{ }

	font(const vector<composite_string>& fontNames, double pointSize = 0)
		: m_contents(typename transactable_t::construct_embedded_t(), description(fontNames, pointSize))
	{ }

	font(const composite_string& fontName, double pointSize = 0)
		: m_contents(typename transactable_t::construct_embedded_t(), description(fontName, pointSize))
	{ }

	font& operator=(const font& src)
	{
		*m_contents = *(src.m_contents);
		return *this;
	}

	font& operator=(const volatile font& src)
	{
		*m_contents = *(src.m_contents.begin_read());
		return *this;
	}

	volatile font& operator=(const font& src) volatile
	{
		m_contents.set(*(src.m_contents));
		return *this;
	}

	bool operator==(const font& cmp) const { return *m_contents == *(cmp.m_contents); }
	bool operator==(const font& cmp) const volatile { return *(m_contents.begin_read()) == *(cmp.m_contents); }
	bool operator==(const volatile font& cmp) const { return cmp == *this; }
	bool operator!=(const font& cmp) const { return !operator==(cmp); }
	bool operator!=(const font& cmp) const volatile { return !operator==(cmp); }
	bool operator!=(const volatile font& cmp) const { return !operator==(cmp); }

	//description& get_description() { return *m_contents; }
	//description get_description() const { return *m_contents; }
	//description get_description() const volatile { return *(m_contents.begin_read()); }

	void prepend_font_name(const composite_string& fontName) { m_contents->m_fontNames.prepend(1, fontName); }

	void prepend_font_name(const composite_string& fontName) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_fontNames.prepend(1, fontName);
		} while (!m_contents.end_write(wt));
	}

	void append_font_name(const composite_string& fontName) { m_contents->m_fontNames.append(1, fontName); }

	void append_font_name(const composite_string& fontName) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_fontNames.append(1, fontName);
		} while (!m_contents.end_write(wt));
	}

	vector<composite_string>& get_font_names() { return m_contents->m_fontNames; }
	const vector<composite_string>& get_font_names() const { return m_contents->m_fontNames; }
	vector<composite_string> get_font_names() const volatile { return m_contents.begin_read()->m_fontNames; }

	size_t get_font_name_count() const { return m_contents->m_fontNames.get_length(); }
	size_t get_font_name_count() const volatile { return m_contents.begin_read()->m_fontNames.get_length(); }

	bool is_underlined() const { return m_contents->m_isUnderlined; }
	bool is_bold() const { return m_contents->m_isBold; }
	bool is_italic() const { return m_contents->m_isItalic; }
	bool is_strike_out() const { return m_contents->m_isStrikeOut; }

	bool is_underlined() const volatile { return m_contents.begin_read()->m_isUnderlined; }
	bool is_bold() const volatile { return m_contents.begin_read()->m_isBold; }
	bool is_italic() const volatile { return m_contents.begin_read()->m_isItalic; }
	bool is_strike_out() const volatile { return m_contents.begin_read()->m_isStrikeOut; }

	void set_underlined(bool b = true) { m_contents->m_isUnderlined = b; }
	void set_bold(bool b = true) { m_contents->m_isBold = b; }
	void set_italic(bool b = true) { m_contents->m_isItalic = b; }
	void set_strike_out(bool b = true) { m_contents->m_isStrikeOut = b; }

	void set_underlined(bool b = true) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_isUnderlined = b;
		} while (!m_contents.end_write(wt));
	}

	void set_bold(bool b = true) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_isBold = b;
		} while (!m_contents.end_write(wt));
	}

	void set_italic(bool b = true) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_isItalic = b;
		} while (!m_contents.end_write(wt));
	}


	void set_strike_out(bool b = true) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_isStrikeOut = b;
		} while (!m_contents.end_write(wt));
	}

	double& get_point_size() { return m_contents->m_pointSize; }
	double get_point_size() const { return m_contents->m_pointSize; }
	double get_point_size() const volatile { return m_contents.begin_read()->m_pointSize; }

	void set_point_size(double ptSize) { m_contents->m_pointSize = ptSize; }

	void set_point_size(double ptSize) volatile
	{
		write_token wt;
		do
		{
			m_contents.begin_write(wt);
			wt->m_pointSize = ptSize;
		} while (!m_contents.end_write(wt));
	}
};

#pragma warning(pop)


}
}


#endif

