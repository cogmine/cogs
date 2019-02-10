//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress


#ifndef COGS_EXTENT
#define COGS_EXTENT


#include "cogs/geometry/proportion.hpp"
#include "cogs/geometry/alignment.hpp"


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


namespace cogs {
namespace geometry {
	

namespace planar {

class point;
class margin;
class range;

class size
{
private:
	std::array<double, 2> m_contents;

public:
	size()
	{ }

	size(const size& sz)
		: m_contents(sz.m_contents)
	{ }

	size(double x, double y)
		: m_contents{x, y}
	{ }

	size(const double(&d)[2])
		: m_contents{ d[0], d[1] }
	{ }

	size(const std::array<double, 2>& d)
		: m_contents{ d[0], d[1] }
	{ }

	size& operator=(const size& sz)
	{
		m_contents = sz.m_contents;
		return *this;
	}

	size& operator=(const double(&d)[2])
	{
		m_contents[0] = d[0];
		m_contents[1] = d[1];
		return *this;
	}

	size& operator=(const std::array<double, 2>& d)
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

	double& get_width() { return m_contents[0]; }
	const double get_width() const { return m_contents[0]; }

	double& get_height() { return m_contents[1]; }
	const double get_height() const { return m_contents[1]; }

	point to_point() const;

	void set_width(double d) { m_contents[0] = d; }
	void set_height(double d) { m_contents[1] = d; }

	void clear() { m_contents[0] = 0; m_contents[1] = 0; }

	bool operator!() const { return (m_contents[0] == 0) || (m_contents[1] == 0); }

	bool operator==(const size& sz) const { return (m_contents[0] == sz.m_contents[0]) && (m_contents[1] == sz.m_contents[1]); }

	bool operator!=(const size& sz) const { return !operator==(sz); }

	bool operator<(const size& sz) const
	{
		if (m_contents[0] == sz.m_contents[0])
			return (m_contents[1] < sz.m_contents[1]);
		return (m_contents[0] < sz.m_contents[0]);
	}

	bool operator>=(const size& src)  const { return !operator<(src); }

	bool operator>(const size& src) const { return (src < *this); }

	bool operator<=(const size& src) const { return !operator>(src); }

	int compare(const size& sz) const
	{
		if (m_contents[0] == sz.m_contents[0])
		{
			if (m_contents[1] < sz.m_contents[1])
				return -1;
			return (m_contents[1] > sz.m_contents[1]) ? 1 : 0;
		}
		return (m_contents[0] < sz.m_contents[0]) ? -1 : 1;
	}

	size abs() const { size sz(cogs::abs(m_contents[0]), cogs::abs(m_contents[1])); return sz; }
	void assign_abs() { cogs::assign_abs(m_contents[0]); cogs::assign_abs(m_contents[1]); }
	const size& pre_assign_abs() { assign_abs(); return *this; }
	size post_assign_abs() { size result(*this); assign_abs(); return result; }

	size negative() const { size sz(cogs::negative(m_contents[0]), cogs::negative(m_contents[1])); return sz; }
	void assign_negative() { cogs::assign_negative(m_contents[0]); cogs::assign_negative(m_contents[1]); }
	const size& pre_assign_negative() { assign_negative(); return *this; }
	size post_assign_negative() { size result(*this); assign_negative(); return result; }

	double calc_width() const { return cogs::abs(get_width()); }
	double calc_height() const { return cogs::abs(get_height()); }

	size ceil() const { size sz(cogs::ceil(m_contents[0]), cogs::ceil(m_contents[1])); return sz; }
	void assign_ceil() { cogs::assign_ceil(m_contents[0]); cogs::assign_ceil(m_contents[1]); }
	const size& pre_assign_ceil() { assign_ceil(); return *this; }
	size post_assign_ceil() { size result(*this); assign_ceil(); return result; }

	size floor() const { size sz(cogs::floor(m_contents[0]), cogs::floor(m_contents[1])); return sz; }
	void assign_floor() { cogs::assign_floor(m_contents[0]); cogs::assign_floor(m_contents[1]); }
	const size& pre_assign_floor() { assign_floor(); return *this; }
	size post_assign_floor() { size result(*this); assign_floor(); return result; }

	size round() const { size sz(cogs::round(m_contents[0]), cogs::round(m_contents[1])); return sz; }
	void assign_round() { cogs::assign_round(m_contents[0]); cogs::assign_round(m_contents[1]); }
	const size& pre_assign_round() { assign_round(); return *this; }
	size post_assign_round() { size result(*this); assign_round(); return result; }


	double ceil_width() const { return cogs::ceil(m_contents[0]); }
	void assign_ceil_width() { cogs::assign_ceil(m_contents[0]); }
	const size& pre_assign_ceil_width() { assign_ceil_width(); return *this; }
	size post_assign_ceil_width() { size result(*this); assign_ceil_width(); return result; }

	double floor_width() const { return cogs::floor(m_contents[0]); }
	void assign_floor_width() { cogs::assign_floor(m_contents[0]); }
	const size& pre_assign_floor_width() { assign_floor_width(); return *this; }
	size post_assign_floor_width() { size result(*this); assign_floor_width(); return result; }

	double round_width() const { return cogs::round(m_contents[0]); }
	void assign_round_width() { cogs::assign_round(m_contents[0]); }
	const size& pre_assign_round_width() { assign_round_width(); return *this; }
	size post_assign_round_width() { size result(*this); assign_round_width(); return result; }



	double ceil_height() const { return cogs::ceil(m_contents[1]); }
	void assign_ceil_height() { cogs::assign_ceil(m_contents[1]); }
	const size& pre_assign_ceil_height() { assign_ceil_height(); return *this; }
	size post_assign_ceil_height() { size result(*this); assign_ceil_height(); return result; }

	double floor_height() const { return cogs::floor(m_contents[1]); }
	void assign_floor_height() { cogs::assign_floor(m_contents[1]); }
	const size& pre_assign_floor_height() { assign_floor_height(); return *this; }
	size post_assign_floor_height() { size result(*this); assign_floor_height(); return result; }

	double round_height() const { return cogs::round(m_contents[1]); }
	void assign_round_height() { cogs::assign_round(m_contents[1]); }
	const size& pre_assign_round_height() { assign_round_height(); return *this; }
	size post_assign_round_height() { size result(*this); assign_round_height(); return result; }


	double ceil(dimension d) const { return cogs::ceil(m_contents[(int)d]); }
	void assign_ceil(dimension d) { cogs::assign_ceil(m_contents[(int)d]); }
	const size& pre_assign_ceil(dimension d) { assign_ceil(d); return *this; }
	size post_assign_ceil(dimension d) { size result(*this); assign_ceil(d); return result; }

	double floor(dimension d) const { return cogs::floor(m_contents[(int)d]); }
	void assign_floor(dimension d) { cogs::assign_floor(m_contents[(int)d]); }
	const size& pre_assign_floor(dimension d) { assign_floor(d); return *this; }
	size post_assign_floor(dimension d) { size result(*this); assign_floor(d); return result; }

	double round(dimension d) const { return cogs::round(m_contents[(int)d]); }
	void assign_round(dimension d) { cogs::assign_round(m_contents[(int)d]); }
	const size& pre_assign_round(dimension d) { assign_round(d); return *this; }
	size post_assign_round(dimension d) { size result(*this); assign_round(d); return result; }


	// contains
	bool contains(const point& pt) const;

	// size + size = size
	size operator+(const size& sz) const { size result(m_contents[0] + sz.m_contents[0], m_contents[1] + sz.m_contents[1]); return result; }
	size& operator+=(const size& sz) { m_contents[0] += sz.m_contents[0]; m_contents[1] += sz.m_contents[1]; return *this; }

	// size + point = size
	size operator+(const point& pt) const;
	size& operator+=(const point& pt);

	// size + margin = size
	size operator+(const margin& m) const;
	size& operator+=(const margin& m);

	// size + double[2] = size
	size operator+(const double(&d)[2]) const { size result(m_contents[0] + d[0], m_contents[1] + d[1]); return result; }
	size& operator+=(const double(&d)[2]) { m_contents[0] += d[0]; m_contents[1] += d[1]; return *this; }

	size operator+(const std::array<double, 2>& d) const { size result(m_contents[0] + d[0], m_contents[1] + d[1]); return result; }
	size& operator+=(const std::array<double, 2>& d) { m_contents[0] += d[0]; m_contents[1] += d[1]; return *this; }


	// size - size = size
	size operator-(const size& sz) const { size result(m_contents[0] - sz.m_contents[0], m_contents[1] - sz.m_contents[1]); return result; }
	size& operator-=(const size& sz) { m_contents[0] -= sz.m_contents[0]; m_contents[1] -= sz.m_contents[1]; return *this; }
	
	// size - point = size
	size operator-(const point& pt) const;
	size& operator-=(const point& pt);

	// size - margin = size
	size operator-(const margin& m) const;
	size& operator-=(const margin& m);

	// size - double[2] = size
	size operator-(const double(&d)[2]) const { size result(m_contents[0] - d[0], m_contents[1] - d[1]); return result; }
	size& operator-=(const double(&d)[2]) { m_contents[0] -= d[0]; m_contents[1] -= d[1]; return *this; }

	size operator-(const std::array<double, 2>& d) const { size result(m_contents[0] - d[0], m_contents[1] - d[1]); return result; }
	size& operator-=(const std::array<double, 2>& d) { m_contents[0] -= d[0]; m_contents[1] -= d[1]; return *this; }


	// size * double = size
	size operator*(double d) const { size result(m_contents[0] * d, m_contents[1] * d); return result; }
	size& operator*=(double d) { m_contents[0] *= d; m_contents[1] *= d; return *this; }

	// size * proportion = size
	size operator*(const proportion& p) const { size result(m_contents[0] * p[dimension::horizontal], m_contents[1] * p[dimension::vertical]); return result; }
	size& operator*=(const proportion& p) { m_contents[0] *= p[dimension::horizontal]; m_contents[1] *= p[dimension::vertical]; return *this; }

	// size * double[2] = size
	size operator*(const double(&d)[2]) const { size result(m_contents[0] * d[0], m_contents[1] * d[1]); return result; }
	size& operator*=(const double(&d)[2]) { m_contents[0] *= d[0]; m_contents[1] *= d[1]; return *this; }

	size operator*(const std::array<double, 2>& d) const { size result(m_contents[0] * d[0], m_contents[1] * d[1]); return result; }
	size& operator*=(const std::array<double, 2>& d) { m_contents[0] *= d[0]; m_contents[1] *= d[1]; return *this; }

	// size * alignment = size
	size operator*(const alignment& a) const { size result(m_contents[0] * a[dimension::horizontal], m_contents[1] * a[dimension::vertical]); return result; }
	size& operator*=(const alignment& a) { m_contents[0] *= a[dimension::horizontal]; m_contents[1] *= a[dimension::vertical]; return *this; }


	// size / double = size
	size operator/(double d) const { size result(m_contents[0] / d, m_contents[1] / d); return result; }
	size& operator/=(double d) { m_contents[0] /= d; m_contents[1] /= d; return *this; }

	// size / proportion = size
	size operator/(const proportion& p) const { size result(m_contents[0] / p[dimension::horizontal], m_contents[1] / p[dimension::vertical]); return result; }
	size& operator/=(const proportion& p) { m_contents[0] /= p[dimension::horizontal]; m_contents[1] /= p[dimension::vertical]; return *this; }

	// size / double[2] = size
	size operator/(const double(&d)[2]) const { size result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	size& operator/=(const double(&d)[2]) { m_contents[0] /= d[0]; m_contents[1] /= d[1]; return *this; }

	size operator/(const std::array<double, 2>& d) const { size result(m_contents[0] / d[0], m_contents[1] / d[1]); return result; }
	size& operator/=(const std::array<double, 2>& d) { m_contents[0] /= d[0]; m_contents[1] /= d[1]; return *this; }

	// size / alignment = size
	size operator/(const alignment& a) const { size result(m_contents[0] / a[dimension::horizontal], m_contents[1] / a[dimension::vertical]); return result; }
	size& operator/=(const alignment& a) { m_contents[0] /= a[dimension::horizontal]; m_contents[1] /= a[dimension::vertical]; return *this; }


	// size divide_whole double = size
	size divide_whole(double d) const { size result(cogs::divide_whole(m_contents[0], d), cogs::divide_whole(m_contents[1], d)); return result; }
	void assign_divide_whole(double d) { cogs::assign_divide_whole(m_contents[0], d); cogs::assign_divide_whole(m_contents[1], d); }

	// size divide_whole proportion = size
	size divide_whole(const proportion& p) const { size result(cogs::divide_whole(m_contents[0], p[dimension::horizontal]), cogs::divide_whole(m_contents[1], p[dimension::vertical])); return result; }
	void assign_divide_whole(const proportion& p) { cogs::assign_divide_whole(m_contents[0], p[dimension::horizontal]); cogs::assign_divide_whole(m_contents[1], p[dimension::vertical]); }

	// size divide_whole double[2] = size
	size divide_whole(const double(&d)[2]) const { size result(cogs::divide_whole(m_contents[0], d[0]), cogs::divide_whole(m_contents[1], d[1])); return result; }
	void assign_divide_whole(const double(&d)[2]) { cogs::assign_divide_whole(m_contents[0], d[0]); cogs::assign_divide_whole(m_contents[1], d[1]); }

	size divide_whole(const std::array<double, 2>& d) const { size result(cogs::divide_whole(m_contents[0], d[0]), cogs::divide_whole(m_contents[1], d[1])); return result; }
	void assign_divide_whole(const std::array<double, 2>& d) { cogs::assign_divide_whole(m_contents[0], d[0]); cogs::assign_divide_whole(m_contents[1], d[1]); }


	// size | size = size	// longer
	size operator|(const size& sz) const { size result(cogs::greater(m_contents[0], sz.m_contents[0]), cogs::greater(m_contents[1], sz.m_contents[1])); return result; }
	size& operator|=(const size& sz) { cogs::assign_greater(m_contents[0], sz.m_contents[0]); cogs::assign_greater(m_contents[1], sz.m_contents[1]); return *this; }

	// size greater size = size	// longer
	size greater(const size& sz) const { size result(cogs::greater(m_contents[0], sz.m_contents[0]), cogs::greater(m_contents[1], sz.m_contents[1])); return result; }
	void assign_greater(const size& sz) { cogs::assign_greater(m_contents[0], sz.m_contents[0]); cogs::assign_greater(m_contents[1], sz.m_contents[1]); }

	// size & size = size	// shorter
	size operator&(const size& sz) const { size result(cogs::lesser(m_contents[0], sz.m_contents[0]), cogs::lesser(m_contents[1], sz.m_contents[1])); return result; }
	size& operator&=(const size& sz) { cogs::assign_lesser(m_contents[0], sz.m_contents[0]); cogs::assign_lesser(m_contents[1], sz.m_contents[1]); return *this; }

	// size lesser size = size	// shorter
	size lesser(const size& sz) const { size result(cogs::lesser(m_contents[0], sz.m_contents[0]), cogs::lesser(m_contents[1], sz.m_contents[1])); return result; }
	void assign_lesser(const size& sz) { cogs::assign_lesser(m_contents[0], sz.m_contents[0]); cogs::assign_lesser(m_contents[1], sz.m_contents[1]); }

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

}


}
}


#pragma warning(pop)


#endif

