//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_MARGIN
#define COGS_HEADER_GEOMETRY_MARGIN


#include "cogs/operators.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/geometry/dimension.hpp"
#include "cogs/geometry/size.hpp"


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified


namespace cogs {
namespace geometry {

namespace linear {

class margin;

}

namespace planar {

class margin;

}

namespace linear {

class margin
{
public:
	double m_leading;
	double m_trailing;

	margin()
		: m_leading(0),
		m_trailing(0)
	{ }

	margin(double leading, double trailing)
		: m_leading(leading),
		m_trailing(trailing)
	{ }

	margin(const margin& m)
		: m_leading(m.m_leading),
		m_trailing(m.m_trailing)
	{ }

	margin(double d)
		: m_leading(d),
		m_trailing(0)
	{ }

	margin& operator=(const margin& m)
	{
		m_leading = m.m_leading;
		m_trailing = m.m_trailing;
		return *this;
	}

	margin& operator=(double d)
	{
		m_leading = d;
		m_trailing = 0;
		return *this;
	}

	void set(double l, double t)
	{
		m_leading = l;
		m_trailing = t;
	}

	void clear()
	{
		m_leading = 0;
		m_trailing = 0;
	}

	// set leading
	void set_leading(double d) { m_leading = d; }

	// set trailing
	void set_trailing(double d) { m_trailing = d; }

	// get
	double& get_leading() { return m_leading; }
	const double get_leading() const { return m_leading; }

	double& get_trailing() { return m_trailing; }
	const double get_trailing() const { return m_trailing; }

	const double get_size() const { return m_leading + m_trailing; }

	bool is_empty() const { return !m_leading && !m_trailing; }
	bool operator!() const { return is_empty(); }

	margin operator+(const margin& m) const
	{
		margin result(m_leading + m.m_leading, m_trailing + m.m_trailing);
		return result;
	}

	margin& operator+=(const margin& m)
	{
		m_leading += m.get_leading();
		m_trailing += m.get_trailing();
		return *this;
	}

	margin operator&(const margin& m) const
	{
		margin result(
			cogs::lesser(m_leading, m.get_leading()),
			cogs::lesser(m_trailing, m.get_trailing())
		);
		return result;
	}

	margin& operator&=(const margin& m)
	{
		cogs::assign_lesser(m_leading, m.get_leading());
		cogs::assign_lesser(m_trailing, m.get_trailing());
		return *this;
	}

	margin operator|(const margin& m) const
	{
		margin result(
			cogs::greater(m_leading, m.get_leading()),
			cogs::greater(m_trailing, m.get_trailing())
		);
		return result;
	}

	margin& operator|=(const margin& m)
	{
		cogs::assign_greater(m_leading, m.get_leading());
		cogs::assign_greater(m_trailing, m.get_trailing());
		return *this;
	}
};

}

namespace planar {

/// @ingroup Planar
/// @brief A planar margin consisting of horizontal and vertical linear margins.
class margin
{
private:
	size m_topLeftMargin;
	size m_bottomRightMargin;

public:
	typedef linear::margin linear_t;

	margin()
		: m_topLeftMargin(0, 0),
		m_bottomRightMargin(0, 0)
	{ }

	margin(const margin& src)
		: m_topLeftMargin(src.m_topLeftMargin),
		m_bottomRightMargin(src.m_bottomRightMargin)
	{ }

	margin(const linear_t& x, const linear_t& y)
		: m_topLeftMargin(x.get_leading(), y.get_leading()),
		m_bottomRightMargin(x.get_trailing(), y.get_trailing())
	{ }

	margin(const size& tl)
		: m_topLeftMargin(tl),
		m_bottomRightMargin(0, 0)
	{ }

	margin(const point& tl);

	margin(const size& tl, const size& br)
		: m_topLeftMargin(tl),
		m_bottomRightMargin(br)
	{ }

	margin(const point& tl, const size& br);

	margin(double t, double l, double b, double r)
		: m_topLeftMargin(l, t),
		m_bottomRightMargin(r, b)
	{ }

	margin& operator=(const margin& src)
	{
		m_topLeftMargin = src.m_topLeftMargin;
		m_bottomRightMargin = src.m_bottomRightMargin;
		return *this;
	}

	margin& operator=(const point& src);

	margin& operator=(const size& sz)
	{
		m_topLeftMargin.set(sz.get_width(), sz.get_height());
		m_bottomRightMargin.clear();
		return *this;
	}

	void set(const size& tl, const size& br)
	{
		m_topLeftMargin = tl;
		m_bottomRightMargin = br;
	}

	void set(const point& tl, const size& br);

	void set(dimension d, const linear::margin& m)
	{
		if (d == dimension::horizontal)
			set_width(m);
		else
			set_height(m);
	}

	void set(double t, double l, double b, double r)
	{
		m_topLeftMargin.set(l, t);
		m_bottomRightMargin.set(r, b);
	}

	// set_top_left
	void set_top_left(const size& tl)
	{ m_topLeftMargin = tl; }

	// set_bottom_right
	void set_bottom_right(const size& br)
	{ m_bottomRightMargin = br; }

	// set_top
	void set_top(double t) { m_topLeftMargin.set_height(t); }

	// set_left
	void set_left(double l) { m_topLeftMargin.set_width(l); }

	// set_bottom
	void set_bottom(double b) { m_bottomRightMargin.set_height(b); }

	// set_right
	void set_right(double r) { m_bottomRightMargin.set_width(r); }

	// set linear margins
	void set_width(const linear::margin& m)
	{
		m_topLeftMargin.set_width(m.get_leading());
		m_bottomRightMargin.set_width(m.get_trailing());
	}

	void set_height(const linear::margin& m)
	{
		m_topLeftMargin.set_height(m.get_leading());
		m_bottomRightMargin.set_height(m.get_trailing());
	}

	void clear()
	{
		m_topLeftMargin.set(0, 0);
		m_bottomRightMargin.set(0, 0);
	}

	// get
	size& get_top_left() { return m_topLeftMargin; }
	const size& get_top_left() const { return m_topLeftMargin; }
	size& get_bottom_right() { return m_bottomRightMargin; }
	const size& get_bottom_right() const { return m_bottomRightMargin; }

	double& get_top() { return m_topLeftMargin.get_height(); }
	const double get_top() const { return m_topLeftMargin.get_height(); }
	double& get_left() { return m_topLeftMargin.get_width(); }
	const double get_left() const { return m_topLeftMargin.get_width(); }
	double& get_bottom() { return m_bottomRightMargin.get_height(); }
	const double get_bottom() const { return m_bottomRightMargin.get_height(); }
	double& get_right() { return m_bottomRightMargin.get_width(); }
	const double get_right() const { return m_bottomRightMargin.get_width(); }

	const double get_width() const { return get_left() + get_right(); }
	const double get_height() const { return get_top() + get_bottom(); }

	size get_size() const
	{
		size sz(get_width(), get_height());
		return sz;
	}

	double get_size(dimension d) const
	{
		if (d == dimension::horizontal)
			return get_width();
		return get_height();
	}

	linear::margin get_width_margin() const { return linear::margin(get_left(), get_right()); }
	linear::margin get_height_margin() const { return linear::margin(get_top(), get_bottom()); }

	linear::margin operator[](dimension d) const { return (d == dimension::horizontal) ? get_width_margin() : get_height_margin(); }

	// margin + margin = margin
	margin operator+(const margin& src)
	{
		margin m(
			m_topLeftMargin + src.get_top_left(),
			m_bottomRightMargin + src.get_bottom_right());
		return m;
	}

	margin& operator+=(const margin& src)
	{
		m_topLeftMargin += src.get_top_left();
		m_bottomRightMargin += src.get_bottom_right();
		return *this;
	}

	// margin & margin = margin
	margin operator&(const margin& src)
	{
		margin m(
			m_topLeftMargin & src.get_top_left(),
			m_bottomRightMargin & src.get_bottom_right());
		return m;
	}

	margin& operator&=(const margin& src)
	{
		m_topLeftMargin &= src.get_top_left();
		m_bottomRightMargin &= src.get_bottom_right();
		return *this;
	}

	// margin | margin = margin
	margin operator|(const margin& src)
	{
		margin m(
			m_topLeftMargin | src.get_top_left(),
			m_bottomRightMargin | src.get_bottom_right());
		return m;
	}

	margin& operator|=(const margin& src)
	{
		m_topLeftMargin |= src.get_top_left();
		m_bottomRightMargin |= src.get_bottom_right();
		return *this;
	}

};


inline size size::operator+(const margin& m) const { size result(m_contents[0] + m.get_width(), m_contents[1] + m.get_height()); return result; }
inline size& size::operator+=(const margin& m) { m_contents[0] += m.get_width(); m_contents[1] += m.get_height(); return *this; }


inline size size::operator-(const margin& m) const { size result(m_contents[0] - m.get_width(), m_contents[1] - m.get_height()); return result; }
inline size& size::operator-=(const margin& m) { m_contents[0] -= m.get_width(); m_contents[1] -= m.get_height(); return *this; }


}

}
}

#pragma warning(pop)


#endif

