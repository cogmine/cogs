//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GFX_COLOR
#define COGS_HEADER_GFX_COLOR

#include <type_traits>

#include "cogs/collections/string.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified

struct rgb_t
{
	uint8_t m_red;
	uint8_t m_green;
	uint8_t m_blue;
};

struct rgba_t 
{
	union {
		struct {
			uint8_t m_red;
			uint8_t m_green;
			uint8_t m_blue;
		};
		rgb_t m_rgb;
	};
	uint8_t m_alpha;
};

inline bool operator==(const rgb_t& src1, const rgb_t& src2) { return (src1.m_red == src2.m_red) && (src1.m_green == src2.m_green) && (src1.m_blue == src2.m_blue); }
inline bool operator!=(const rgb_t& src1, const rgb_t& src2) { return !(src1 == src2); }
inline bool operator==(const rgba_t& src1, const rgba_t& src2) { return (src1.m_rgb == src2.m_rgb) && (src1.m_alpha == src2.m_alpha); }
inline bool operator!=(const rgba_t& src1, const rgba_t& src2) { return !(src1 == src2); }


/// @ingroup Graphics
/// @brief A RGBA color value
class color
{
public:
	// constants are 0xAARRGGBB
	enum class constant : size_t
	{
		// -- Start of 16 standard VGA/HTML colors.
		white                  = 0xFFFFFFFF,
		red                    = 0xFFFF0000,
		lime                   = 0xFF00FF00,
		blue                   = 0xFF0000FF,
		black                  = 0xFF000000,

		yellow                 = 0xFFFFFF00,
		aqua                   = 0xFF00FFFF,
		cyan                   = 0xFF00FFFF,
		fuchsia                = 0xFFFF00FF,
		magenta                = 0xFFFF00FF,

		maroon                 = 0xFF800000,
		green                  = 0xFF008000,
		navy                   = 0xFF000080,

		olive                  = 0xFF808000,
		teal                   = 0xFF008080,
		purple                 = 0xFF800080,

		gray                   = 0xFF808080,
		silver                 = 0xFFC0C0C0,
		// -- End of 16 standard VGA/HTML colors

		// -- Start of standard X11/W3C colors (additional)
		indian_red             = 0xFFCD5C5C,
		light_coral            = 0xFFF08080,
		salmon                 = 0xFFFA8072,
		dark_salmon            = 0xFFE9967A,
		light_salmon           = 0xFFFFA07A,
		crimson                = 0xFFDC143C,
		fire_brick             = 0xFFB22222,
		dark_red               = 0xFF8B0000,
		pink                   = 0xFFFFC0CB,
		light_pink             = 0xFFFFB6C1,
		hot_pink               = 0xFFFF69B4,
		deep_pink              = 0xFFFF1493,
		medium_violet_red      = 0xFFC71585,
		pale_violet_red        = 0xFFDB7093,
		coral                  = 0xFFFF7F50,
		tomato                 = 0xFFFF6347,
		orange_red             = 0xFFFF4500,
		dark_orange            = 0xFFFF8C00,
		orange                 = 0xFFFFA500,
		gold                   = 0xFFFFD700,
		light_yellow           = 0xFFFFFFE0,
		lemon_chiffon          = 0xFFFFFACD,
		light_goldenrod_yellow = 0xFFFAFAD2,
		papaya_whip            = 0xFFFFEFD5,
		moccasin               = 0xFFFFE4B5,
		peach_puff             = 0xFFFFDAB9,
		pale_goldenrod         = 0xFFEEE8AA,
		khaki                  = 0xFFF0E68C,
		dark_khaki             = 0xFFBDB76B,
		lavender               = 0xFFE6E6FA,
		thistle                = 0xFFD8BFD8,
		plum                   = 0xFFDDA0DD,
		violet                 = 0xFFEE82EE,
		orchid                 = 0xFFDA70D6,
		medium_orchid          = 0xFFBA55D3,
		medium_purple          = 0xFF9370DB,
		blue_violet            = 0xFF8A2BE2,
		dark_violet            = 0xFF9400D3,
		dark_orchid            = 0xFF9932CC,
		dark_magenta           = 0xFF8B008B,
		indigo                 = 0xFF4B0082,
		dark_slate_blue        = 0xFF483D8B,
		slate_blue             = 0xFF6A5ACD,
		medium_slate_blue      = 0xFF7B68EE,
		green_yellow           = 0xFFADFF2F,
		chartreuse             = 0xFF7FFF00,
		lawn_green             = 0xFF7CFC00,
		lime_green             = 0xFF32CD32,
		pale_green             = 0xFF98FB98,
		light_green            = 0xFF90EE90,
		medium_spring_green    = 0xFF00FA9A,
		spring_green           = 0xFF00FF7F,
		medium_sea_green       = 0xFF3CB371,
		sea_green              = 0xFF2E8B57,
		forest_green           = 0xFF228B22,
		dark_green             = 0xFF006400,
		yellow_green           = 0xFF9ACD32,
		olive_drab             = 0xFF6B8E23,
		dark_olive_green       = 0xFF556B2F,
		medium_aquamarine      = 0xFF66CDAA,
		dark_sea_green         = 0xFF8FBC8F,
		light_sea_green        = 0xFF20B2AA,
		dark_cyan              = 0xFF008B8B,
		light_cyan             = 0xFFE0FFFF,
		pale_turquoise         = 0xFFAFEEEE,
		aquamarine             = 0xFF7FFFD4,
		turquoise              = 0xFF40E0D0,
		medium_turquoise       = 0xFF48D1CC,
		dark_turquoise         = 0xFF00CED1,
		cadet_blue             = 0xFF5F9EA0,
		steel_blue             = 0xFF4682B4,
		light_steel_blue       = 0xFFB0C4DE,
		powder_blue            = 0xFFB0E0E6,
		light_blue             = 0xFFADD8E6,
		sky_blue               = 0xFF87CEEB,
		light_sky_blue         = 0xFF87CEFA,
		deep_sky_blue          = 0xFF00BFFF,
		dodger_blue            = 0xFF1E90FF,
		cornflower_blue        = 0xFF6495ED,
		royal_blue             = 0xFF4169E1,
		medium_blue            = 0xFF0000CD,
		dark_blue              = 0xFF00008B,
		midnight_blue          = 0xFF191970,
		cornsilk               = 0xFFFFF8DC,
		blanched_almond        = 0xFFFFEBCD,
		bisque                 = 0xFFFFE4C4,
		navajo_white           = 0xFFFFDEAD,
		wheat                  = 0xFFF5DEB3,
		burly_wood             = 0xFFDEB887,
		tan                    = 0xFFD2B48C,
		rosy_brown             = 0xFFBC8F8F,
		sandy_brown            = 0xFFF4A460,
		goldenrod              = 0xFFDAA520,
		dark_goldenrod         = 0xFFB8860B,
		peru                   = 0xFFCD853F,
		chocolate              = 0xFFD2691E,
		saddle_brown           = 0xFF8B4513,
		sienna                 = 0xFFA0522D,
		brown                  = 0xFFA52A2A,
		snow                   = 0xFFFFFAFA,
		honeydew               = 0xFFF0FFF0,
		mint_cream             = 0xFFF5FFFA,
		azure                  = 0xFFF0FFFF,
		alice_blue             = 0xFFF0F8FF,
		ghost_white            = 0xFFF8F8FF,
		white_smoke            = 0xFFF5F5F5,
		seashell               = 0xFFFFF5EE,
		beige                  = 0xFFF5F5DC,
		old_lace               = 0xFFFDF5E6,
		floral_white           = 0xFFFFFAF0,
		ivory                  = 0xFFFFFFF0,
		antique_white          = 0xFFFAEBD7,
		linen                  = 0xFFFAF0E6,
		lavender_blush         = 0xFFFFF0F5,
		misty_rose             = 0xFFFFE4E1,
		gainsboro              = 0xFFDCDCDC,
		light_grey             = 0xFFD3D3D3,
		dark_gray              = 0xFFA9A9A9,
		dim_gray               = 0xFF696969,
		light_slate_gray       = 0xFF778899,
		slate_gray             = 0xFF708090,
		dark_slate_gray        = 0xFF2F4F4F,
		// -- End of standard X11/W3C colors (additional)

		transparent            = 0x00000000
	};

private:
	transactable<rgba_t> m_rgba;

	color get() const volatile { return *(m_rgba.begin_read()); }

public:
	color() { }
	color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) { set(r, g, b, a); }
	color(const color& c) { set(c); }
	color(const volatile color& c) { set(c); }
	color(const rgba_t& c) { set(c); }
	color(const rgb_t& c) { set(c); }
	color(const rgb_t& c, uint8_t a) { set(c, a); }
	color(constant c) { set(c); }
	color(constant c, uint8_t a) { set(c, a); }

	color& operator=(const color& c) { set(c); return *this; }
	color& operator=(const volatile color& c) { set(c); return *this; }
	color& operator=(const rgba_t& c) { set(c); return *this; }
	color& operator=(const rgb_t& c) { set(c); return *this; }
	color& operator=(constant c) { set(c); return *this; }

	volatile color& operator=(const color& c) volatile { set(c); return *this; }
	volatile color& operator=(const rgba_t& c) volatile { set(c); return *this; }
	volatile color& operator=(const rgb_t& c) volatile { set(c); return *this; }
	volatile color& operator=(constant c) volatile { set(c); return *this; }

	bool operator==(constant c) const { return get_constant() == c; }
	bool operator==(constant c) const volatile { return get().get_constant() == c; }
	bool operator==(const color& c) const { return *m_rgba == c.get_rgba(); }
	bool operator==(const color& c) const volatile { return get() == c; }
	bool operator==(const volatile color& c) const { return c == *this; }

	bool operator!=(constant c) const { return !operator==(c); }
	bool operator!=(constant c) const volatile { return !operator==(c); }
	bool operator!=(const color& c) const { return !operator==(c); }
	bool operator!=(const color& c) const volatile { return !operator==(c); }
	bool operator!=(const volatile color& c) const { return !operator==(c); }

	rgba_t get_rgba() const { return *m_rgba; }
	rgba_t get_rgba() const volatile { return *(m_rgba.begin_read()); }

	rgb_t get_rgb() const { return m_rgba->m_rgb; }
	rgb_t get_rgb() const volatile { return m_rgba.begin_read()->m_rgb; }

	uint8_t get_red() const { return m_rgba->m_red; }
	uint8_t get_green() const { return m_rgba->m_green; }
	uint8_t get_blue() const { return m_rgba->m_blue; }
	uint8_t get_alpha() const { return m_rgba->m_alpha; }

	uint8_t get_red() const volatile { return m_rgba.begin_read()->m_red; }
	uint8_t get_green() const volatile { return m_rgba.begin_read()->m_green; }
	uint8_t get_blue() const volatile { return m_rgba.begin_read()->m_blue; }
	uint8_t get_alpha() const volatile { return m_rgba.begin_read()->m_alpha; }

	constant get_constant() const
	{
		rgba_t c = get_rgba();
		return (constant)((c.m_red << 16) | (c.m_green << 8) | c.m_blue | (c.m_alpha << 24));
	}

	constant get_constant() const volatile { return get().get_constant(); }

	void set(const color& c) { set(c.get_red(), c.get_green(), c.get_blue(), c.get_alpha()); }
	void set(const color& c) volatile { m_rgba.set(c.get_rgba()); }
	void set(const volatile color& c) { set(c.get()); }

	void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		m_rgba->m_red = r;
		m_rgba->m_green = g;
		m_rgba->m_blue = b;
		m_rgba->m_alpha = a;
	}

	void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a) volatile
	{
		rgba_t c;
		c.m_red = r;
		c.m_green = g;
		c.m_blue = b;
		c.m_alpha = a;
		m_rgba.set(c);
	}

	void set(uint8_t r, uint8_t g, uint8_t b) { set(r, g, b, 0xFF); }
	void set(uint8_t r, uint8_t g, uint8_t b) volatile { set(r, g, b, 0xFF); }

	void set(const rgba_t& c) { set(c.m_red, c.m_green, c.m_blue, c.m_alpha); }
	void set(const rgba_t& c) volatile { set(c.m_red, c.m_green, c.m_blue, c.m_alpha); }
	void set(const rgb_t& c) { set(c.m_red, c.m_green, c.m_blue); }
	void set(const rgb_t& c) volatile { set(c.m_red, c.m_green, c.m_blue); }
	void set(const rgb_t& c, uint8_t a) { set(c.m_red, c.m_green, c.m_blue, a); }
	void set(const rgb_t& c, uint8_t a) volatile { set(c.m_red, c.m_green, c.m_blue, a); }

	void set(constant c) { set((uint8_t)((size_t)c >> 16), (uint8_t)((size_t)c >> 8), (uint8_t)c, (uint8_t)((size_t)c >> 24)); }
	void set(constant c) volatile { set(color(c)); }

	void set(constant c, uint8_t a) { set((uint8_t)((size_t)c >> 16), (uint8_t)((size_t)c >> 8), (uint8_t)c, a); }
	void set(constant c, uint8_t a) volatile { set(color(c, a)); }

	void set_red(uint8_t r) { m_rgba->m_red = r; }
	void set_green(uint8_t g) { m_rgba->m_green = g; }
	void set_blue(uint8_t b) { m_rgba->m_blue = b; }
	void set_alpha(uint8_t a) { m_rgba->m_alpha = a; }

	void set_red(uint8_t r) volatile
	{
		transactable<rgba_t>::write_token wt;
		do {
			m_rgba.begin_write(wt);
			wt->m_red = r;
		} while (!m_rgba.end_write(wt));
	}

	void set_green(uint8_t g) volatile
	{
		transactable<rgba_t>::write_token wt;
		do {
			m_rgba.begin_write(wt);
			wt->m_green = g;
		} while (!m_rgba.end_write(wt));
	}

	void set_blue(uint8_t b) volatile
	{
		transactable<rgba_t>::write_token wt;
		do {
			m_rgba.begin_write(wt);
			wt->m_blue = b;
		} while (!m_rgba.end_write(wt));
	}

	void set_alpha(uint8_t a) volatile
	{
		transactable<rgba_t>::write_token wt;
		do {
			m_rgba.begin_write(wt);
			wt->m_alpha = a;
		} while (!m_rgba.end_write(wt));
	}

	bool is_opaque() const { return m_rgba->m_alpha == 0xFF; }
	bool is_opaque() const volatile { return get().is_opaque(); }

	bool is_fully_transparent() const { return m_rgba->m_alpha == 0x00; }
	bool is_fully_transparent() const volatile { return get().is_fully_transparent(); }

	bool is_semi_transparent() const { return !is_opaque() && !is_fully_transparent(); }
	bool is_semi_transparent() const volatile { return get().is_semi_transparent(); }

	template <typename char_t>
	composite_string_t<char_t> to_string_t(bool trimZeroAlpha = true) const
	{
		composite_string_t<char_t> result;
		int_to_fixed_integer_t<uint8_t> r = get_red();
		int_to_fixed_integer_t<uint8_t> g = get_green();
		int_to_fixed_integer_t<uint8_t> b = get_blue();
		int_to_fixed_integer_t<uint8_t> a = get_alpha();

		static constexpr char_t part1 = (char_t)'#';
		result = string_t<char_t>::contain(&part1, 1);
		if (!trimZeroAlpha || !!a)
			result += a.template to_string_t<char_t>(16, 2);
		result += r.template to_string_t<char_t>(16, 2);
		result += g.template to_string_t<char_t>(16, 2);
		result += b.template to_string_t<char_t>(16, 2);
		return result;
	}

	composite_string to_string(bool trimZeroAlpha = true) const { return to_string_t<wchar_t>(trimZeroAlpha); }
	composite_cstring to_cstring(bool trimZeroAlpha = true) const { return to_string_t<char>(trimZeroAlpha); }

	template <typename char_t>
	composite_string_t<char_t> to_string_t(bool trimZeroAlpha = true) const volatile
	{
		return get().template to_string_t<char_t>(trimZeroAlpha);
	}

	composite_string to_string(bool trimZeroAlpha = true) const volatile { return get().template to_string_t<wchar_t>(trimZeroAlpha); }
	composite_cstring to_cstring(bool trimZeroAlpha = true) const volatile { return get().template to_string_t<char>(trimZeroAlpha); }
};


#pragma warning(pop)

}


#endif
