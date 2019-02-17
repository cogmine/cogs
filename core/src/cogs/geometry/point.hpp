//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress


#ifndef COGS_HEADER_GEOMETRY_POINT
#define COGS_HEADER_GEOMETRY_POINT

#include <algorithm>
#include <array>
#include <type_traits>

#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/geometry/range.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


namespace cogs {
namespace geometry {

namespace planar {

class size;

class point
{
private:
	std::array<double, 2> m_contents;

public:
	point()
	{ }

	point(const point& pt)
		: m_contents(pt.m_contents)
	{ }

	point(double x, double y)
		: m_contents{ x, y }
	{ }

	point(const double(&d)[2])
		: m_contents{d[0], d[1]}
	{ }

	point(const std::array<double, 2>& d)
		: m_contents{ d[0], d[1] }
	{ }

	point& operator=(const point& pt)
	{
		m_contents = pt.m_contents;
		return *this;
	}

	point& operator=(const double(&d)[2])
	{
		m_contents[0] = d[0];
		m_contents[1] = d[1];
		return *this;
	}

	point& operator=(const std::array<double, 2>& d)
	{
		m_contents[0] = d[0];
		m_contents[1] = d[1];
		return *this;
	}

	void set(double x, double y)
	{
		m_contents[0] = x;
		m_contents[1] = y;
	}

	double& operator[](dimension d) { return m_contents[(int)d]; }
	double operator[](dimension d) const { return m_contents[(int)d]; }

	double& get_x() { return m_contents[0]; }
	const double get_x() const { return m_contents[0]; }

	double& get_y() { return m_contents[1]; }
	const double get_y() const { return m_contents[1]; }

	size to_size() const { size sz(m_contents); return sz; }

	void set_x(double d) { m_contents[0] = d; }
	void set_y(double d) { m_contents[1] = d; }

	void clear() { m_contents[0] = 0; m_contents[1] = 0; }

	point operator-() const
	{
		point pt(-m_contents[0], -m_contents[1]);
		return pt;
	}

	bool operator!() const { return (m_contents[0] == 0) && (m_contents[1] == 0); }

	bool operator==(const point& pt) const { return (m_contents[0] == pt.m_contents[0]) && (m_contents[1] == pt.m_contents[1]); }

	bool operator!=(const point& pt) const { return !operator==(pt); }

	bool operator<(const point& pt) const
	{
		if (m_contents[0] == pt.m_contents[0])
			return (m_contents[1] < pt.m_contents[1]);
		return (m_contents[0] < pt.m_contents[0]);
	}

	bool operator>=(const point& src)  const { return !operator<(src); }

	bool operator>(const point& src) const { return (src < *this); }

	bool operator<=(const point& src) const { return !operator>(src); }

	int compare(const point& pt) const
	{
		if (m_contents[0] == pt.m_contents[0])
		{
			if (m_contents[1] < pt.m_contents[1])
				return -1;
			return (m_contents[1] > pt.m_contents[1]) ? 1 : 0;
		}
		return (m_contents[0] < pt.m_contents[0]) ? -1 : 1;
	}


	point abs() const { point pt(cogs::abs(m_contents[0]), cogs::abs(m_contents[1])); return pt; }
	void assign_abs() { cogs::assign_abs(m_contents[0]); cogs::assign_abs(m_contents[1]); }
	const point& pre_assign_abs() { assign_abs(); return *this; }
	point post_assign_abs() { point result(*this); assign_abs(); return result; }

	auto negative() const { point pt(cogs::negative(m_contents[0]), cogs::negative(m_contents[1])); return pt; }
	void assign_negative() { cogs::assign_negative(m_contents[0]); cogs::assign_negative(m_contents[1]); }
	const point& pre_assign_negative() { assign_negative(); return *this; }
	point post_assign_negative() { point result(*this); assign_negative(); return result; }

	// point + point = point
	point operator+(const point& pt) const { point result(m_contents[0] + pt.m_contents[0], m_contents[1] + pt.m_contents[1]); return result; }
	point& operator+=(const point& pt) { m_contents[0] += pt.m_contents[0]; m_contents[1] += pt.m_contents[1]; return *this; }

	// point + size = point
	point operator+(const size& sz) const { point result(m_contents[0] + sz[dimension::horizontal], m_contents[1] + sz[dimension::vertical]); return result; }
	point& operator+=(const size& sz) { m_contents[0] += sz[dimension::horizontal]; m_contents[1] += sz[dimension::vertical]; return *this; }

	// point + double[2] = point
	point operator+(const double(&d)[2]) const { point result(m_contents[0] + d[0], m_contents[1] + d[1]); return result; }
	point& operator+=(const double(&d)[2]) { m_contents[0] += d[0]; m_contents[1] += d[1]; return *this; }

	point operator+(const std::array<double, 2>& d) const { point result(m_contents[0] + d[0], m_contents[1] + d[1]); return result; }
	point& operator+=(const std::array<double, 2>& d) { m_contents[0] += d[0]; m_contents[1] += d[1]; return *this; }


	// point - point = size
	size operator-(const point& pt) const { size result(m_contents[0] - pt.m_contents[0], m_contents[1] - pt.m_contents[1]); return result; }
	//point& operator-=(const point& pt) { m_contents[0] -= pt.m_contents[0]; m_contents[1] -= pt.m_contents[1]; return *this; }

	// point - size = point
	point operator-(const size& sz) const { point result(m_contents[0] - sz[dimension::horizontal], m_contents[1] - sz[dimension::vertical]); return result; }
	point& operator-=(const size& sz) { m_contents[0] -= sz[dimension::horizontal]; m_contents[1] -= sz[dimension::vertical]; return *this; }

	// point - double[2] = point
	point operator-(const double(&d)[2]) const { point result(m_contents[0] - d[0], m_contents[1] - d[1]); return result; }
	point& operator-=(const double(&d)[2]) { m_contents[0] -= d[0]; m_contents[1] -= d[1]; return *this; }

	point operator-(const std::array<double, 2>& d) const { point result(m_contents[0] - d[0], m_contents[1] - d[1]); return result; }
	point& operator-=(const std::array<double, 2>& d) { m_contents[0] -= d[0]; m_contents[1] -= d[1]; return *this; }


	// point * double = point
	point operator*(double d) const { point result(m_contents[0] * d, m_contents[1] * d); return result; }
	point& operator*=(double d) { m_contents[0] *= d; m_contents[1] *= d; return *this; }

	// point * proportion = point
	point operator*(const proportion& p) const { point result(m_contents[0] * p[dimension::horizontal], m_contents[1] * p[dimension::vertical]); return result; }
	point& operator*=(const proportion& p) { m_contents[0] *= p[dimension::horizontal]; m_contents[1] *= p[dimension::vertical]; return *this; }
	
	// point * double[2] = point
	point operator*(const double(&d)[2]) const { point result(m_contents[0] * d[0], m_contents[1] * d[1]); return result; }
	point& operator*=(const double(&d)[2]) { m_contents[0] *= d[0]; m_contents[1] *= d[1]; return *this; }

	point operator*(const std::array<double, 2>& d) const { point result(m_contents[0] * d[0], m_contents[1] * d[1]); return result; }
	point& operator*=(const std::array<double, 2>& d) { m_contents[0] *= d[0]; m_contents[1] *= d[1]; return *this; }



	// point / double = point
	point operator/(double d) const { point result(m_contents[0] / d, m_contents[1] / d); return result; }
	point& operator/=(double d) { m_contents[0] /= d; m_contents[1] /= d; return *this; }

	// point / proportion = point
	point operator/(const proportion& p) const { point result(m_contents[0] / p[dimension::horizontal], m_contents[1] / p[dimension::vertical]); return result; }
	point& operator/=(const proportion& p) { m_contents[0] /= p[dimension::horizontal]; m_contents[1] /= p[dimension::vertical]; return *this; }

	// point / double[2] = point
	point operator/(const double(&d)[2]) const { point result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	point& operator/=(const double(&d)[2]) { m_contents[0] /= d[0]; m_contents[1] /= d[1]; return *this; }

	point operator/(const std::array<double, 2>& d) const { point result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	point& operator/=(const std::array<double, 2>& d) { m_contents[0] /= d[0]; m_contents[1] /= d[1]; return *this; }


	// point divide_whole double = point
	point divide_whole(double d) const { point result(cogs::divide_whole(m_contents[0], d), cogs::divide_whole(m_contents[1], d)); return result; }
	void assign_divide_whole(double d) { cogs::assign_divide_whole(m_contents[0], d); cogs::assign_divide_whole(m_contents[1], d); }

	// point divide_whole proportion = point
	point divide_whole(const proportion& p) const { point result(m_contents[0] / p[dimension::horizontal], m_contents[1] / p[dimension::vertical]); return result; }
	void assign_divide_whole(const proportion& p) { cogs::assign_divide_whole(m_contents[0], p[dimension::horizontal]); cogs::assign_divide_whole(m_contents[1], p[dimension::vertical]); }

	// point divide_whole double[2] = point
	point divide_whole(const double(&d)[2]) const { point result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	void assign_divide_whole(const double(&d)[2]) { cogs::assign_divide_whole(m_contents[0], d[0]); cogs::assign_divide_whole(m_contents[1], d[1]); }

	point divide_whole(const std::array<double, 2>& d) const { point result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	void assign_divide_whole(const std::array<double, 2>& d) { cogs::assign_divide_whole(m_contents[0], d[0]); cogs::assign_divide_whole(m_contents[1], d[1]); }

	// point | point = point		(leftmost x topmost)
	// point lesser point = point

	// point & point = point		(rightmost x bottommost)
	// point greater point = point	

	// to_string
	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;

		static constexpr char_t part1 = (char_t)'{';
		static constexpr char_t part2 = (char_t)',';
		static constexpr char_t part3 = (char_t)'}';

		result = string_t<char_t>::contain(&part1, 1);
		result += cogs::to_string_t<char_t>(m_contents[0]);
		result += string_t<char_t>::contain(&part2, 1);
		result += cogs::to_string_t<char_t>(m_contents[1]);
		result += string_t<char_t>::contain(&part3, 1);
		return result;
	}

	composite_string to_string() const { return to_string_t<wchar_t>(); }
	composite_cstring to_cstring() const { return to_string_t<char>(); }
};


inline size size::operator+(const point& pt) const { size result(m_contents[0] + pt[dimension::horizontal], m_contents[1] + pt[dimension::vertical]); return result; }
inline size& size::operator+=(const point& pt) { m_contents[0] += pt[dimension::horizontal]; m_contents[1] += pt[dimension::vertical]; return *this; }

inline size size::operator-(const point& pt) const { size result(m_contents[0] - pt[dimension::horizontal], m_contents[1] - pt[dimension::vertical]); return result; }
inline size& size::operator-=(const point& pt) { m_contents[0] -= pt[dimension::horizontal]; m_contents[1] -= pt[dimension::vertical]; return *this; }



inline margin::margin(const point& tl)
	: m_topLeftMargin(tl.get_x(), tl.get_y()),
	m_bottomRightMargin(0, 0)
{ }


inline margin::margin(const point& tl, const size& br)
	: m_topLeftMargin(tl.get_x(), tl.get_y()),
	m_bottomRightMargin(br)
{ }



inline margin& margin::operator=(const point& src)
{
	m_topLeftMargin.set(src.get_x(), src.get_y());
	m_bottomRightMargin.clear();
	return *this;
}


inline void margin::set(const point& tl, const size& br)
{
	m_topLeftMargin.set(tl.get_x(), tl.get_y());
	m_bottomRightMargin = br;
}


inline bool size::contains(const point& pt) const
{
	if (!m_contents[0] || !m_contents[1])
		return false;
	if (m_contents[0] < 0)
	{
		if ((pt.get_x() > 0) || (pt.get_x() <= m_contents[0]))
			return false;
	}
	else if ((pt.get_x() < 0) || (pt.get_x() >= m_contents[0]))
		return false;
	if (m_contents[1] < 0)
	{
		if ((pt.get_y() > 0) || (pt.get_y() <= m_contents[1]))
			return false;
	}
	else if ((pt.get_y() < 0) || (pt.get_y() >= m_contents[1]))
		return false;
	return true;
}

inline point size::to_point() const { point pt(m_contents); return pt; }

inline point range::limit(const point& pt) const
{
	point rtn(get_width().limit(pt.get_x()), get_height().limit(pt.get_y()));
	return rtn;
}

inline point range::limit_min(const point& pt) const
{
	point rtn(get_width().limit_min(pt.get_x()), get_height().limit_min(pt.get_y()));
	return rtn;
}

inline point range::limit_max(const point& pt) const
{
	point rtn(get_width().limit_max(pt.get_x()), get_height().limit_max(pt.get_y()));
	return rtn;
}

}

}


}


#pragma warning(pop)


#endif
