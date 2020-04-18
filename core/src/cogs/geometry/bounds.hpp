//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_BOUNDS
#define COGS_HEADER_GEOMETRY_BOUNDS


#include "cogs/operators.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/geometry/point.hpp"


namespace cogs {
namespace geometry {

namespace linear {

class bounds
{
private:
	double m_position;
	double m_size;

public:
	bounds() { }

	bounds(const bounds& b)
		: m_position(b.m_position),
		m_size(b.m_size)
	{ }

	bounds(double sz)
		: m_position(0),
		m_size(sz)
	{ }

	bounds(double pt, double sz)
		: m_position(pt),
		m_size(sz)
	{ }

	bounds& operator=(const bounds& b)
	{
		m_position = b.m_position;
		m_size = b.m_size;
		return *this;
	}

	bounds& operator=(double sz)
	{
		m_position = 0;
		m_size = sz;
		return *this;
	}

	void set(double pt, double sz)
	{
		m_position = pt;
		m_size = sz;
	}

	double& get_position() { return m_position; }
	double get_position() const { return m_position; }

	double& get_size() { return m_size; }
	double get_size() const { return m_size; }

	double calc_size() const { return cogs::abs(m_size); }

	double calc_position() const
	{
		double result = m_position;
		if (m_size < 0)
			result += m_size;
		return result;
	}

	double calc_end() const
	{
		return m_position + m_size;
	}


	void set_position(double pos) { m_position = pos; }
	void set_size(double sz) { m_size = sz; }

	void clear() { cogs::clear(m_position); cogs::clear(m_size); }

	// normalize
	void normalize() { double tmp = calc_position(); m_size = calc_size(); m_position = tmp; }
	bounds normalized() const { return bounds(calc_position(), calc_size()); }


	// inset
	void inset(double i) { inset(i, i); }
	void inset(double start, double end)
	{
		m_position += start;
		m_size -= start;
		m_size -= end;
	}

	void inset(const margin& m)
	{
		inset(m.get_leading(), m.get_trailing());
	}

	// inflate
	void inflate(double i) { inflate(i, i); }
	void inflate(double start, double end)
	{
		m_position -= start;
		m_size += start;
		m_size += end;
	}

	void inflate(const margin& m)
	{
		inflate(m.get_leading(), m.get_trailing());
	}

	bool operator!() const { return !m_size; }

	// equality
	bool operator==(const bounds& b) const
	{
		bounds b1 = normalized();
		bounds b2 = b.normalized();
		return ((b1.get_size() == b2.get_size()) && (b1.get_position() == b2.get_position()));
	}

	// inequality
	bool operator!=(const bounds& b) const
	{ return !operator==(b); }

	// contains
	bool contains(double pt) const
	{
		bounds tmpThis = normalized();
		return (
			(pt >= tmpThis.get_position())
			&& (pt < tmpThis.get_position() + tmpThis.get_size())
			);
	}

	// intersects
	bool intersects(const bounds& b) const
	{
		bounds tmpThis = normalized();
		bounds b2 = b.normalized();
		return (tmpThis.get_position() < b2.get_position() + b2.get_size())
			&& (tmpThis.get_position() + tmpThis.get_size() > b2.get_position());
	}

	// overlaps
	bool overlaps(const bounds& b) const
	{
		bounds tmpThis = normalized();
		bounds b2 = b.normalized();
		return (tmpThis.get_position() <= b2.get_position())
			&& (tmpThis.get_position() + tmpThis.get_size() >= b2.get_position() + b2.get_size());
	}

	// bounds + margin = bounds // extend
	bounds operator+(const margin& m) const
	{
		bounds result(m_position, m_size + m.get_size());
		return result;
	}

	bounds& operator+=(const margin& m)
	{
		m_size += m.get_size();
		return *this;
	}

	// bounds - margin = bounds // reduce
	bounds operator-(const margin& m) const
	{
		bounds result(m_position, m_size - m.get_size());
		return result;
	}

	bounds& operator-=(const margin& m)
	{
		m_size -= m.get_size();
		return *this;
	}

	// bounds - bounds = bounds[] // portion of this that does not overlap with arg.  Will be 0-2 segments.
	std::pair<std::array<bounds, 2>, size_t> operator-(const bounds& b) const
	{
		std::pair<std::array<bounds, 2>, size_t> result;
		bounds* segments = result.first.data();
		size_t& numSegments = result.second;
		numSegments = 0;

		bounds b1 = normalized();
		double start1 = b1.get_position();
		double end1 = b1.calc_end();

		bounds b2 = b.normalized();
		double start2 = b2.get_position();
		double end2 = b2.calc_end();

		if ((end1 <= start2) || (end2 <= start1))
		{
			if (b1.get_size() != 0)
				segments[numSegments++] = *this; // this ends before arg starts, or this starts as/after the arg ends.  No overlap
		}
		else
		{
			if (start1 < start2) // this starts before arg starts
				segments[numSegments++].set(start1, start2 - start1);
			if (end2 < end1) // arg ends before this ends
				segments[numSegments++].set(end2, end1 - end2);
		}

		return result;
	}

	// bounds ^ bounds = bounds // Only non-overlapping areas
	std::pair<std::array<bounds, 2>, size_t> operator^(const bounds& b) const
	{
		std::pair<std::array<bounds, 2>, size_t> result;
		bounds* segments = result.first.data();
		size_t& numSegments = result.second;
		numSegments = 0;

		bounds b1 = normalized();
		double start1 = b1.get_position();
		double end1 = b1.calc_end();

		bounds b2 = b.normalized();
		double start2 = b2.get_position();
		double end2 = b2.calc_end();

		if ((end1 <= start2) || (end2 <= start1))
		{
			if (get_size() != 0)
				segments[numSegments++] = *this;
			if (b.get_size() != 0)
				segments[numSegments++] = b;
		}
		else
		{
			if (start1 < start2)
				segments[numSegments++].set(start1, start2 - start1);
			else if (start2 < start1)
				segments[numSegments++].set(start2, start1 - start2);

			if (end1 < end2)
				segments[numSegments++].set(end1, end2 - end1);
			else if (end2 < end1)
				segments[numSegments++].set(end2, end1 - end2);
		}

		return result;
	}

	// bounds | bounds = bounds // union
	bounds operator|(const bounds& b) const
	{
		bounds result;
		bounds b1 = normalized();
		double end1 = b1.calc_end();
		bounds b2 = b.normalized();
		double end2 = b2.calc_end();

		if (b1.get_position() < b2.get_position())
			result.get_position() = b1.get_position();
		else
			result.get_position() = b2.get_position();

		if (end1 > end2)
			result.get_size() = end1 - result.get_position();
		else
			result.get_size() = end2 - result.get_position();

		return result;
	}

	bounds& operator|=(const bounds& b)
	{
		normalize();
		double end1 = calc_end();
		bounds b2 = b.normalized();
		double end2 = b2.calc_end();

		if (get_position() > b2.get_position())
			get_position() = b2.get_position();

		if (end1 < end2)
			m_size = end2 - get_position();

		return *this;
	}

	// bounds & bounds = bounds // intesection
	bounds operator&(const bounds& b) const
	{
		bounds result;
		bounds b1 = normalized();
		double end1 = b1.calc_end();
		bounds b2 = b.normalized();
		double end2 = b2.calc_end();

		if ((end1 < b2.get_position())
			|| (b1.get_position() > end2)
			)
			result.clear();
		else
		{
			if (b1.get_position() > b2.get_position())
				result.get_position() = b1.get_position();
			else
				result.get_position() = b2.get_position();

			if (end1 > end2)
				result.get_size() = end2;
			else
				result.get_size() = end1;
			result.get_size() -= result.get_position();

		}
		return result;
	}

	bounds& operator&=(const bounds& b)
	{
		normalize();
		double end1 = calc_end();
		bounds b2 = b.normalized();
		double end2 = b2.calc_end();

		if ((end1 < b2.get_position())
			|| (get_position() > end2)
			)
			clear();
		else
		{
			if (get_position() < b2.get_position())
				get_position() = b2.get_position();

			if (end1 > end2)
				get_size() = end2;
			else
				get_size() = end1;
			get_size() -= get_position();
		}

		return *this;
	}

	// to_string
	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;

		static constexpr char_t part1 = (char_t)'{';
		static constexpr char_t part2 = (char_t)',';
		static constexpr char_t part3 = (char_t)'}';

		result = string_t<char_t>::contain(&part1, 1);
		result += cogs::to_string_t<char_t>(m_position);
		result += string_t<char_t>::contain(&part2, 1);
		result += cogs::to_string_t<char_t>(m_size);
		result += string_t<char_t>::contain(&part3, 1);
		return result;
	}

	composite_string to_string() const { return to_string_t<wchar_t>(); }
	composite_cstring to_cstring() const { return to_string_t<char>(); }
};


}


namespace planar {

class bounds
{
private:
	point m_position;
	size m_size;

public:
	bounds() { }

	bounds(const bounds& b)
		: m_position(b.m_position),
		m_size(b.m_size)
	{ }

	bounds(const size& sz)
		: m_position(0, 0),
		m_size(sz)
	{ }

	bounds(const point& pt, const size& sz)
		: m_position(pt),
		m_size(sz)
	{ }

	bounds(double x, double y, double w, double h)
		: m_position(x, y),
		m_size(w, h)
	{ }

	bounds& operator=(const bounds& b)
	{
		m_position = b.m_position;
		m_size = b.m_size;
		return *this;
	}

	bounds& operator=(const size& sz)
	{
		m_position.set(0, 0);
		m_size = sz;
		return *this;
	}

	void set(const point& pt, const size& sz)
	{
		m_position = pt;
		m_size = sz;
	}

	point& get_position() { return m_position; }
	const point& get_position() const { return m_position; }

	double& get_position(dimension d) { return m_position[d]; }
	double get_position(dimension d) const { return m_position[d]; }

	double& get_x() { return m_position.get_x(); }
	double get_x() const { return m_position.get_x(); }
	double& get_y() { return m_position.get_y(); }
	double get_y() const { return m_position.get_y(); }

	double& get_left() { return m_position.get_x(); }
	double get_left() const { return m_position.get_x(); }
	double& get_top() { return m_position.get_y(); }
	double get_top() const { return m_position.get_y(); }

	size& get_size() { return m_size; }
	const size& get_size() const { return m_size; }

	double& get_size(dimension d) { return m_size[d]; }
	double get_size(dimension d) const { return m_size[d]; }

	double& get_height() { return m_size.get_height(); }
	double get_height() const { return m_size.get_height(); }
	double& get_width() { return m_size.get_width(); }
	double get_width() const { return m_size.get_width(); }

	double calc_width() const { return cogs::abs(get_width()); }
	double calc_height() const { return cogs::abs(get_height()); }

	size calc_size() const { return m_size.abs(); }
	point calc_position() const { return calc_top_left(); }

	double calc_top() const
	{
		double result = m_position.get_y();
		if (m_size.get_height() < 0)
			result += m_size.get_height();
		return result;
	}

	double calc_left() const
	{
		double result = m_position.get_x();
		if (m_size.get_width() < 0)
			result += m_size.get_width();
		return result;
	}

	double calc_bottom() const
	{
		double result = m_position.get_y();
		if (m_size.get_height() > 0)
			result += m_size.get_height();
		return result;
	}

	double calc_right() const
	{
		double result = m_position.get_x();
		if (m_size.get_width() > 0)
			result += m_size.get_width();
		return result;
	}

	point calc_top_left() const { return point(calc_left(), calc_top()); }
	point calc_top_right() const { return point(calc_right(), calc_top()); }
	point calc_bottom_left() const { return point(calc_left(), calc_bottom()); }
	point calc_bottom_right() const { return point(calc_right(), calc_bottom()); }

	void set_position(const point& pos) { m_position = pos; }
	void set_position(double x, double y) { m_position.set_x(x); m_position.set_y(y); }
	void set_position(dimension d, double pos) { m_position[d] = pos; }

	void set_size(const size& sz) { m_size = sz; }
	void set_size(dimension d, double sz) { m_size[d] = sz; }

	void set_height(double d) { m_size.set_height(d); }
	void set_width(double d) { m_size.set_width(d); }

	void clear() { m_position.clear(); m_size.clear(); }

	// normalize
	void normalize() { point pt = calc_position(); m_size = calc_size(); m_position = pt; }
	bounds normalized() const { return bounds(calc_position(), calc_size()); }


	// inset
	void inset(double i) { inset(i, i, i, i); }
	void inset(double h, double v) { inset(v, h, v, h); }
	void inset(double t, double l, double b, double r)
	{
		m_position.get_x() += l;
		m_position.get_y() += t;
		m_size.get_width() -= l;
		m_size.get_width() -= r;
		m_size.get_height() -= t;
		m_size.get_height() -= b;
	}

	void inset(const margin& m)
	{
		inset(m.get_top(), m.get_left(), m.get_bottom(), m.get_right());
	}

	// inflate
	void inflate(double i) { inflate(i, i, i, i); }
	void inflate(double h, double v) { inflate(v, h, v, h); }
	void inflate(double t, double l, double b, double r)
	{
		m_position.get_x() -= l;
		m_position.get_y() -= t;
		m_size.get_width() += l;
		m_size.get_width() += r;
		m_size.get_height() += t;
		m_size.get_height() += b;
	}

	void inflate(const margin& m)
	{
		inflate(m.get_top(), m.get_left(), m.get_bottom(), m.get_right());
	}

	bool operator!() const { return !m_size; }

	// equality
	bool operator==(const bounds& b) const
	{
		bounds b1 = normalized();
		bounds b2 = b.normalized();
		return ((b1.get_size() == b2.get_size()) && (b1.get_position() == b2.get_position()));
	}

	// inequality
	bool operator!=(const bounds& b) const
	{ return !operator==(b); }

	// contains
	bool contains(const point& pt) const
	{
		bounds tmpThis = normalized();
		return (pt.get_y() >= tmpThis.get_position().get_y())
			&& (pt.get_y() < tmpThis.get_position().get_y() + tmpThis.get_size().get_height())
			&& (pt.get_x() >= tmpThis.get_position().get_x())
			&& (pt.get_x() < tmpThis.get_position().get_x() + tmpThis.get_size().get_width());
	}

	// intersects
	bool intersects(const bounds& b) const
	{
		bounds tmpThis = normalized();
		bounds b2 = b.normalized();
		return (tmpThis.get_position().get_x() < b2.get_position().get_x() + b2.get_size().get_width())
			&& (tmpThis.get_position().get_x() + tmpThis.get_size().get_width() > b2.get_position().get_x())
			&& (tmpThis.get_position().get_y() < b2.get_position().get_y() + b2.get_size().get_height())
			&& (tmpThis.get_position().get_y() + tmpThis.get_size().get_height() > b2.get_position().get_y());
	}

	// overlaps
	bool overlaps(const bounds& b) const
	{
		bounds tmpThis = normalized();
		bounds b2 = b.normalized();
		return (tmpThis.get_position().get_x() <= b2.get_position().get_x())
			&& (tmpThis.get_position().get_x() + tmpThis.get_size().get_width() >= b2.get_position().get_x() + b2.get_size().get_width())
			&& (tmpThis.get_position().get_y() <= b2.get_position().get_y())
			&& (tmpThis.get_position().get_y() + tmpThis.get_size().get_height() >= b2.get_position().get_y() + b2.get_size().get_height());
	}


	// bounds + point = bounds // offset
	bounds operator+(const point& p) const
	{
		bounds result(m_position + p, m_size);
		return result;
	}

	bounds& operator+=(const point& p)
	{
		m_position += p;
		return *this;
	}

	// bounds - point = bounds // offset
	bounds operator-(const point& p) const
	{
		bounds result(m_position - p.to_size(), m_size);
		return result;
	}

	bounds& operator-=(const point& p)
	{
		m_position -= p.to_size();
		return *this;
	}

	// bounds + size = bounds // extend
	bounds operator+(const size& sz) const
	{
		bounds result(m_position, m_size + sz);
		return result;
	}

	bounds& operator+=(const size& sz)
	{
		m_size += sz;
		return *this;
	}

	// bounds + margin = bounds // extend
	bounds operator+(const margin& m) const
	{
		bounds result(m_position, m_size + m.get_size());
		return result;
	}

	bounds& operator+=(const margin& m)
	{
		m_size += m.get_size();
		return *this;
	}

	// bounds - size = bounds // reduce
	bounds operator-(const size& sz) const
	{
		bounds result(m_position, m_size - sz);
		return result;
	}

	bounds& operator-=(const size& sz)
	{
		m_size -= sz;
		return *this;
	}

	// bounds - margin = bounds // reduce
	bounds operator-(const margin& m) const
	{
		bounds result(m_position, m_size - m.get_size());
		return result;
	}

	bounds& operator-=(const margin& m)
	{
		m_size -= m.get_size();
		return *this;
	}

	// bounds - bounds = bounds[] // portion of this that does not overlap with arg.  Might be 0-4 rects.
	std::pair<std::array<bounds, 4>, size_t> operator-(const bounds& b) const
	{
		std::pair<std::array<bounds, 4>, size_t> result;
		bounds* segments = result.first.data();
		size_t& numSegments = result.second;
		numSegments = 0;

		bounds b1 = normalized();
		point br1 = b1.calc_bottom_right();

		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		double start1X = b1.get_position().get_x();
		double end1X = br1.get_x();
		double start1Y = b1.get_position().get_y();
		double end1Y = br1.get_y();

		double start2X = b2.get_position().get_x();
		double end2X = br2.get_x();
		double start2Y = b2.get_position().get_y();
		double end2Y = br2.get_y();

		if ((end1X <= start2X) || (start1X >= end2X) || (end1Y <= start2Y) || (start1Y >= end2Y))
			segments[numSegments++] = *this;
		else
		{
			double size1X = b1.get_size().get_width();
			double size1Y = b1.get_size().get_height();

			if (start1X < start2X) // this starts before arg starts
				segments[numSegments++].set(point(start1X, start1Y), size(start2X - start1X, size1Y));
			if (end2X < end1X) // arg ends before this ends
				segments[numSegments++].set(point(end2X, start1Y), size(end1X - end2X, size1Y));

			if (start1Y < start2Y) // this starts before arg starts
				segments[numSegments++].set(point(start1X, start1Y), size(size1X, start2Y - start1Y));
			if (end2Y < end1Y) // arg ends before this ends
				segments[numSegments++].set(point(start1X, end2Y), size(size1X, end1Y - end2Y));
		}

		return result;
	}

	// bounds ^ bounds = bounds // Only non-overlapping areas
	std::pair<std::array<bounds, 4>, size_t> operator^(const bounds& b) const
	{
		std::pair<std::array<bounds, 4>, size_t> result;
		bounds* segments = result.first.data();
		size_t& numSegments = result.second;
		numSegments = 0;

		bounds b1 = normalized();
		point br1 = b1.calc_bottom_right();

		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		double start1X = b1.get_position().get_x();
		double end1X = br1.get_x();
		double start1Y = b1.get_position().get_y();
		double end1Y = br1.get_y();

		double start2X = b2.get_position().get_x();
		double end2X = br2.get_x();
		double start2Y = b2.get_position().get_y();
		double end2Y = br2.get_y();

		if ((end1X <= start2X) || (start1X >= end2X) || (end1Y <= start2Y) || (start1Y >= end2Y))
		{
			if (!!*this)
				segments[numSegments++] = *this;
			if (!!b)
				segments[numSegments++] = b;
		}
		else
		{
			double size1X = b1.get_size().get_width();
			double size1Y = b1.get_size().get_height();
			double size2X = b2.get_size().get_width();
			double size2Y = b2.get_size().get_height();

			if (start1X < start2X)
				segments[numSegments++].set(point(start1X, start1Y), size(start2X - start1X, size1Y));
			else if (start2X < start1X)
				segments[numSegments++].set(point(start2X, start2Y), size(start1X - start2X, size2Y));

			if (end2X < end1X)
				segments[numSegments++].set(point(end2X, start1Y), size(end1X - end2X, size1Y));
			else if (end1X < end2X)
				segments[numSegments++].set(point(end1X, start2Y), size(end2X - end1X, size2Y));

			if (start1Y < start2Y)
				segments[numSegments++].set(point(start1X, start1Y), size(size1X, start2Y - start1Y));
			else if (start2Y < start1Y)
				segments[numSegments++].set(point(start2X, start2Y), size(size2X, start1Y - start2Y));

			if (end2Y < end1Y)
				segments[numSegments++].set(point(start1X, end2Y), size(size1X, end1Y - end2Y));
			else if (end1Y < end2Y)
				segments[numSegments++].set(point(start2X, end1Y), size(size2X, end2Y - end1Y));
		}

		return result;
	}

	// bounds | bounds = bounds // union
	bounds operator|(const bounds& b) const
	{
		bounds result;
		bounds b1 = normalized();
		point br1 = b1.calc_bottom_right();
		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		if (b1.get_position().get_y() < b2.get_position().get_y())
			result.get_position().get_y() = b1.get_position().get_y();
		else
			result.get_position().get_y() = b2.get_position().get_y();

		if (b1.get_position().get_x() < b2.get_position().get_x())
			result.get_position().get_x() = b1.get_position().get_x();
		else
			result.get_position().get_x() = b2.get_position().get_x();

		if (br1.get_y() > br2.get_y())
			result.get_height() = br1.get_y() - result.get_position().get_y();
		else
			result.get_height() = br2.get_y() - result.get_position().get_y();

		if (br1.get_x() > br2.get_x())
			result.get_width() = br1.get_x() - result.get_position().get_x();
		else
			result.get_width() = br2.get_x() - result.get_position().get_x();

		return result;
	}

	bounds& operator|=(const bounds& b)
	{
		normalize();
		point br1 = calc_bottom_right();
		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		if (get_position().get_y() > b2.get_position().get_y())
			get_position().get_y() = b2.get_position().get_y();

		if (get_position().get_x() > b2.get_position().get_x())
			get_position().get_x() = b2.get_position().get_x();

		if (br1.get_y() < br2.get_y())
			get_height() = br2.get_y() - get_position().get_y();

		if (br1.get_x() < br2.get_x())
			get_width() = br2.get_x() - get_position().get_x();

		return *this;
	}

	// bounds & bounds = bounds // intesection
	bounds operator&(const bounds& b) const
	{
		bounds result;
		bounds b1 = normalized();
		point br1 = b1.calc_bottom_right();
		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		if ((br1.get_y() < b2.get_position().get_y())
			|| (b1.get_position().get_y() > br2.get_y())
			|| (br1.get_x() < b2.get_position().get_x())
			|| (b1.get_position().get_x() > br2.get_x()))
			result.clear();
		else
		{
			if (b1.get_position().get_y() > b2.get_position().get_y())
				result.get_position().get_y() = b1.get_position().get_y();
			else
				result.get_position().get_y() = b2.get_position().get_y();

			if (b1.get_position().get_x() > b2.get_position().get_x())
				result.get_position().get_x() = b1.get_position().get_x();
			else
				result.get_position().get_x() = b2.get_position().get_x();

			if (br1.get_y() > br2.get_y())
				result.get_height() = br2.get_y();
			else
				result.get_height() = br1.get_y();
			result.get_height() -= result.get_position().get_y();

			if (br1.get_x() > br2.get_x())
				result.get_width() = br2.get_x();
			else
				result.get_width() = br1.get_x();
			result.get_width() -= result.get_position().get_x();
		}
		return result;
	}

	bounds& operator&=(const bounds& b)
	{
		normalize();
		point br1 = calc_bottom_right();
		bounds b2 = b.normalized();
		point br2 = b2.calc_bottom_right();

		if ((br1.get_y() < b2.get_position().get_y())
			|| (get_position().get_y() > br2.get_y())
			|| (br1.get_x() < b2.get_position().get_x())
			|| (get_position().get_x() > br2.get_x()))
			clear();
		else
		{
			if (get_position().get_y() < b2.get_position().get_y())
				get_position().get_y() = b2.get_position().get_y();

			if (get_position().get_x() < b2.get_position().get_x())
				get_position().get_x() = b2.get_position().get_x();

			if (br1.get_y() > br2.get_y())
				get_height() = br2.get_y();
			else
				get_height() = br1.get_y();
			get_height() -= get_position().get_y();

			if (br1.get_x() > br2.get_x())
				get_width() = br2.get_x();
			else
				get_width() = br1.get_x();
			get_width() -= get_position().get_x();
		}

		return *this;
	}


	// bounds | size = bounds // union
	bounds operator|(const size& sz) const
	{
		bounds result = normalized();
		result.m_size |= sz;
		return result;
	}

	bounds& operator|=(const size& sz)
	{
		normalize();
		m_size |= sz;
		return *this;
	}


	// bounds & size = bounds // intesection
	bounds operator&(const size& sz) const
	{
		bounds result = normalized();
		result.m_size &= sz;
		return result;
	}

	bounds& operator&=(const size& sz)
	{
		normalize();
		m_size &= sz;
		return *this;
	}

	// to_string
	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;

		static constexpr char_t part1 = (char_t)'{';
		static constexpr char_t part2 = (char_t)',';
		static constexpr char_t part3 = (char_t)'}';

		result = string_t<char_t>::contain(&part1, 1);
		result += m_position.template to_string_t<char_t>();
		result += string_t<char_t>::contain(&part2, 1);
		result += m_size.template to_string_t<char_t>();
		result += string_t<char_t>::contain(&part3, 1);
		return result;
	}

	composite_string to_string() const { return to_string_t<wchar_t>(); }
	composite_cstring to_cstring() const { return to_string_t<char>(); }
};

}
}
}

#endif
