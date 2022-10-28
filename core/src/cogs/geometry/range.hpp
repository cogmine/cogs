//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_RANGE
#define COGS_HEADER_GEOMETRY_RANGE

#include <array>
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/is_signed_type.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/geometry/margin.hpp"


namespace cogs {
namespace geometry {

namespace linear {

/// @ingroup Linear
/// @brief A linear range - a minimum value and optional maximum value.
class range;

}

namespace planar {

class range;

}

namespace linear {

/// @ingroup Linear
/// @brief A linear range - a minimum value and optional maximum value.
/// A linear range with a min of 0 and a max of 0 includes the position 0.
/// A linear range is considered 'invalid' if its min is greater than its max.
class range
{
public:
	struct maximum_comparator
	{
		static bool is_less_than(const range& t1, const range& t2) { return t1.m_hasMax && (!t2.m_hasMax || (t1.m_max < t2.m_max)); }
	};

	struct minimum_comparator
	{
		static bool is_less_than(const range& t1, const range& t2) { return t1.m_min < t2.m_min; }
	};

	using default_comparator = cogs::default_comparator;

	// default comparator compares based on side of the range.

private:
	double m_max{ 0 };
	double m_min{ 0 };
	bool m_hasMax = false;

public:
	static range make_unbounded() { range r; return r; }

	static range make_fixed(double d = 0) { range r(d, d, true); return r; }

	static range make_invalid()
	{
		// Set min to an extremely large value, to reduce risk of arithmatical adjustments creating valid ranges.
		// 1000000000000 is within the range of integers that can be represented as doubles, without gaps.
		range r(1000000000000.0, 0, true);
		return r;
	}

	range()
	{ }

	range(const range& src)
		: m_max(src.m_max),
		m_min(src.m_min),
		m_hasMax(src.m_hasMax)
	{ }

	range(double min, double max, bool hasMax = true)
		: m_max(max),
		m_min(min),
		m_hasMax(hasMax)
	{ }

	range& operator=(const range& src)
	{
		m_min = src.m_min;
		m_max = src.m_max;
		m_hasMax = src.m_hasMax;
		return *this;
	}

	void clear_min() { m_min = 0; }
	void clear_max() { m_hasMax = false; }

	void clear() { clear_min(); clear_max(); }
	void set_invalid() { m_min = 1000000000000.0; m_max = 0; m_hasMax = true; }

	void set(double mn, double mx, bool hasMax = true)
	{
		m_min = mn;
		m_max = mx;
		m_hasMax = hasMax;
	}

	void set_fixed(double x = 0)
	{
		m_hasMax = true;
		m_min = x;
		m_max = x;
	}

	void set_min(double x) { m_min = x; }

	void set_max(double x, bool hasMax = true) { m_max = x; m_hasMax = hasMax; }

	// get
	bool& has_max() { return m_hasMax; }
	bool has_max() const { return m_hasMax; }

	double& get_min() { return m_min; }
	double get_min() const { return m_min; }
	double& get_max() { return m_max; }
	double get_max() const { return m_max; }

	// invalid
	bool is_invalid() const { return (m_hasMax && (m_min > m_max)); }

	// A range is empty if its either invalid or has a max of 0.
	bool is_empty() { return m_hasMax && (!m_max || (m_min > m_max)); }

	bool is_fixed() const { return m_hasMax && (m_min == m_max); }

	bool contains(double d) const { return (d >= m_min) && (!m_hasMax || (d <= m_max )); }

	// equality
	bool operator==(const range& cmp) const
	{
		return (&cmp == this) || !(m_hasMax != cmp.has_max() || (m_hasMax && m_max != cmp.m_max) || (m_min != cmp.m_min));
	}

	// inequality
	bool operator!=(const range& cmp) const
	{
		return !operator==(cmp);
	}

	// less than
	bool operator<(const range& cmp) const
	{
		// first compares min, then max. Lack of a max is considered greater than a max.
		if ((&cmp == this) || (m_min > cmp.m_min))
			return false;
		if (m_min < cmp.m_min)
			return true;
		if (!m_hasMax)	// No max means max/infinite max, so we could not possibly be lesser.
			return false;
		return !cmp.m_hasMax || (m_max < cmp.m_max);
	}

	// greater than
	bool operator>(const range& cmp) const
	{
		// first compares min, then max. Lack of a max is considered greater than a max.
		if ((&cmp == this) || (m_min < cmp.m_min))
			return false;
		if (m_min > cmp.m_min)
			return true;
		if (!cmp.m_hasMax) // No max means max/infinite max, so we could not possibly be greater.
			return false;
		return !m_hasMax || (m_max > cmp.m_max);
	}

	// range + range = range
	auto operator+(const range& r) const
	{
		range result;
		result.m_min = m_min + r.m_min;
		result.m_hasMax = m_hasMax && r.has_max();
		if (result.m_hasMax)
			result.m_max = m_max + r.m_max;
		return result;
	}

	range& operator+=(const range& r)
	{
		m_min += r.m_min;
		m_hasMax &= r.m_hasMax;
		if (m_hasMax)
			m_max += r.m_max;
		return *this;
	}

	// range + size = range
	auto operator+(double sz) const
	{
		range result;
		result.m_min = m_min + sz;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = m_max + sz;
		return result;
	}

	range& operator+=(double sz)
	{
		m_min += sz;
		if (m_hasMax)
			m_max += sz;
		return *this;
	}

	// range + margin = range
	range operator+(const margin& m) const
	{
		range result;
		double t = m.get_size();
		result.m_min = m_min + t;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = m_max + t;
		return result;
	}

	range& operator+=(const margin& m)
	{
		double t = m.get_size();
		m_min += t;
		if (m_hasMax)
			m_max += t;
		return *this;
	}


	// range - range = range
	auto operator-(const range& r) const
	{
		range result;
		result.m_min = (m_min > r.m_min) ? (m_min - r.m_min) : 0;
		result.m_hasMax = m_hasMax && r.m_hasMax;
		if (result.has_max())
			result.m_max = (m_max > r.m_max) ? (m_max - r.m_max) : 0;
		return result;
	}

	range& operator-=(const range& r)
	{
		m_min = (m_min > r.m_min) ? (m_min - r.m_min) : 0;
		m_hasMax &= r.m_hasMax;
		if (m_hasMax)
			m_max = (m_max > r.m_max) ? (m_max - r.m_max) : 0;
		return *this;
	}

	// range - size = range
	auto operator-(double sz) const
	{
		range result;
		result.m_min = (m_min > sz) ? (m_min - sz) : 0;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = (m_max > sz) ? (m_max - sz) : 0;
		return result;
	}

	range& operator-=(double sz)
	{
		m_min = (m_min > sz) ? (m_min - sz) : 0;
		if (m_hasMax)
			m_max = (m_max > sz) ? (m_max - sz) : 0;
		return *this;
	}

	// range - margin = range
	range operator-(const margin& m) const
	{
		range result;
		double t = m.get_size();
		result.m_min = (m_min > t) ? (m_min - t) : 0;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = (m_max > t) ? (m_max - t) : 0;
		return result;
	}

	range& operator-=(const margin& m)
	{
		double t = m.get_size();
		m_min = (m_min > t) ? (m_min - t) : 0;
		if (m_hasMax)
			m_max = (m_max > t) ? (m_max - t) : 0;
		return *this;
	}


	// range * number = range
	range operator*(double d) const
	{
		range result;
		result.m_min = m_min * d;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = m_max * d;
		return result;
	}

	range& operator*=(double d)
	{
		m_min *= d;
		if (m_hasMax)
			m_max *= d;
		return *this;
	}

	// range / number = range
	range operator/(double d) const
	{
		range result;
		result.m_min = m_min / d;
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = m_max / d;
		return result;
	}

	range& operator/=(double d)
	{
		m_min /= d;
		if (m_hasMax)
			m_max /= d;
		return *this;
	}

	range divide_whole(double d) const
	{
		range result;
		result.m_min = cogs::divide_whole(m_min, d);
		result.m_hasMax = m_hasMax;
		if (m_hasMax)
			result.m_max = cogs::divide_whole(m_max, d);
		return result;
	}

	void assign_divide_whole(double d)
	{
		cogs::assign_divide_whole(m_min, d);
		if (m_hasMax)
			cogs::assign_divide_whole(m_max, d);
	}

	// range | range = range (union)
	range operator|(const range& r) const
	{
		range result;
		if (is_invalid() || r.is_invalid())
			result.set_invalid();
		else
		{
			result.m_min = (m_min < r.m_min) ? m_min : r.m_min;
			result.m_hasMax = m_hasMax && r.m_hasMax;
			if (result.m_hasMax)
				result.m_max = (m_max < r.m_max) ? r.m_max : m_max;
		}
		return result;
	}

	range& operator|=(const range& r)
	{
		if (!r.is_invalid())
		{
			if (is_invalid())
				*this = r;
			else
			{
				if (m_min > r.m_min)
					m_min = r.m_min;
				if (!r.m_hasMax)
					m_hasMax = false;
				else if (m_hasMax && (m_max < r.m_max))
					m_max = r.m_max;
			}
		}
		return *this;
	}


	// range & range = range (intersection)
	range operator&(const range& r) const
	{
		range result;
		if (r.is_invalid() || is_invalid())
			result.set_invalid();
		else
		{
			result.m_min = (m_min < r.m_min) ? r.m_min : m_min;
			if (r.has_max())
			{
				result.m_max = (!m_hasMax || (m_max > r.m_max)) ? r.m_max : m_max;
				result.m_hasMax = true;
			}
			else
			{
				result.m_hasMax = m_hasMax;
				if (m_hasMax)
					result.m_max = m_max;
			}
		}
		return result;
	}


	range& operator&=(const range& r)
	{
		if (r.is_invalid())
			set_invalid();
		else if (!is_invalid())
		{
			if (m_min < r.m_min)
				m_min = r.m_min;
			if (r.m_hasMax)
			{
				if (!m_hasMax || (m_max > r.m_max))
					m_max = r.m_max;
				m_hasMax = true;
			}
			if (m_hasMax && (m_max < m_min))
				m_max = m_min;
		}
		return *this;
	}

	// range overlap range = range (overlap)
	range operator^(const range& r) const
	{
		range result;
		if (r.is_invalid() || is_invalid())
			result.set_invalid();
		else
		{
			result.m_min = (m_min < r.m_min) ? r.m_min : m_min;
			result.m_hasMax = (r.has_max() && m_hasMax);
			if (result.m_hasMax)
				result.m_max = (m_max > r.m_max) ? r.m_max : m_max;
		}
		return result;
	}

	range& operator^=(const range& r)
	{
		if (!is_invalid())
		{
			if (r.is_invalid())
				set_invalid();
			else
			{
				if (m_min < r.m_min)
					m_min = r.m_min;
				m_hasMax &= r.m_hasMax;
				if (m_hasMax && (m_max < r.m_max))
					m_max = r.m_max;
			}
		}
		return *this;
	}

	double get_limit_min(double d) const { return cogs::greater(d, m_min); }
	void limit_min(double& d) const
	{
		if (d < m_min)
			d = m_min;
	}

	double get_limit_max(double d) const { return m_hasMax ? cogs::lesser(d, m_max) : d; }
	void limit_max(double& d) const
	{
		if (m_hasMax && d > m_max)
			d = m_max;
	}

	double get_limit(double d) const { return (m_hasMax && d > m_max) ? m_max : cogs::greater(d, m_min); }
	void limit(double& d) const
	{
		if (d < m_min)
			d = m_min;
		else if (m_hasMax && d > m_max)
			d = m_max;
	}

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;

		static constexpr char_t part1 = (char_t)'{';
		static constexpr char_t part2 = (char_t)'-';
		static constexpr char_t part3 = (char_t)'}';

		static constexpr char_t part2b[] = { (char_t)'-', (char_t)'n', (char_t)'}' };

		result = string_t<char_t>::contain(&part1, 1);
		result += cogs::to_string_t<char_t>(m_min);
		if (!m_hasMax)
			result += string_t<char_t>::contain(part2b, 3);
		else
		{
			result += string_t<char_t>::contain(&part2, 1);
			result += cogs::to_string_t<char_t>(m_max);
			result += string_t<char_t>::contain(&part3, 1);
		}

		return result;
	}

	composite_string to_string() const { return to_string_t<wchar_t>(); }
	composite_cstring to_cstring() const { return to_string_t<char>(); }
};

}

namespace planar {

// A planar range with a min of 0 and a max of 0 in either dimension, includes the position 0 in that dimension.
//
// A dimension of linear range is considered invalid if its min is greater than its max.
// A planar range is considered invalid if invalid in either dimension.
class range
{
private:
	std::array<linear::range, 2> m_contents;

public:
	typedef linear::range linear_t;

	static range make_unbounded() { range r; return r; }

	static range make_fixed(const size& sz) { range r(sz, sz, true, true); return r; }

	static range make_invalid()
	{
		// Set min to an extremely large value, to reduce risk of arithmatical adjustments creating valid ranges.
		// 1000000000000 is within the range of integers that can be represented as doubles, without gaps.
		range r(linear_t(1000000000000.0, 0, true), linear_t(1000000000000.0, 0, true));
		return r;
	}

	range()
	{ }

	range(const range& src)
		: m_contents(src.m_contents)
	{ }

	range(const linear::range& horizontalRange, const linear::range& verticalRange)
		: m_contents{ horizontalRange, verticalRange }
	{ }

	range(const size& minSize, const size& maxSize, bool hasMaxWidth = true, bool hasMaxHeight = true)
		: m_contents{
			linear::range(minSize.get_width(), maxSize.get_width(), hasMaxWidth),
			linear::range(minSize.get_height(), maxSize.get_height(), hasMaxHeight)
		}
	{ }

	range(const size& fixedSize)
		: m_contents{
			linear::range(fixedSize.get_width(), fixedSize.get_width(), true),
			linear::range(fixedSize.get_height(), fixedSize.get_height(), true)
		}
	{ }

	range& operator=(const range& src)
	{
		m_contents = src.m_contents;
		return *this;
	}

	range& operator=(const size& fixedSize)
	{
		set_fixed_width(fixedSize.get_width());
		set_fixed_height(fixedSize.get_height());
		return *this;
	}

	void set(const linear::range& horizontalRange, const linear::range& verticalRange)
	{
		set_width(horizontalRange);
		set_height(verticalRange);
	}

	void set(const size& minSize, const size& maxSize, bool hasMaxWidth = true, bool hasMaxHeight = true)
	{
		set_width(minSize.get_width(), maxSize.get_width(), hasMaxWidth);
		set_height(minSize.get_height(), maxSize.get_height(), hasMaxHeight);
	}

	void set(dimension d, double mn, double mx, bool hasMax = true) { m_contents[(int)d].set(mn, mx, hasMax); }

	void set(dimension d, const linear::range& r) { m_contents[(int)d] = r; }

	void set_min_width(double w) { m_contents[0].set_min(w); }
	void set_min_height(double h) { m_contents[1].set_min(h); }

	void set_min(double x, double y) { set_min_width(x); set_min_height(y); }

	void set_min(const size& minSize) { set_min(minSize.get_width(), minSize.get_height()); }

	void set_min(dimension d, double n) { m_contents[(int)d].set_min(n); }

	void set_max_width(double w, bool hasMax = true) { m_contents[0].set_max(w, hasMax); }
	void set_max_height(double h, bool hasMax = true) { m_contents[1].set_max(h, hasMax); }

	void set_max(double x, double y) { set_max_width(x); set_max_height(y); }

	void set_max(const size& maxSize) { set_max(maxSize.get_width(), maxSize.get_height()); }

	void set_max(dimension d, double n, bool hasMax = true) { m_contents[(int)d].set_max(n, hasMax); }

	void clear_max_width() { m_contents[0].clear_max(); }
	void clear_max_height() { m_contents[1].clear_max(); }
	void clear_max() { clear_max_width(); clear_max_height(); }

	void clear_max(dimension d) { m_contents[(int)d].clear_max(); }

	void clear_min_width() { m_contents[0].clear_min(); }
	void clear_min_height() { m_contents[1].clear_min(); }
	void clear_min() { clear_min_width(); clear_min_height(); }

	void clear_min(dimension d) { m_contents[(int)d].clear_min(); }

	// Sets to an unlimited range
	void clear() { clear_min(); clear_max(); }

	void clear_width() { clear_min_width(); clear_max_width(); }
	void clear_height() { clear_min_height(); clear_max_height(); }

	void clear(dimension d) { m_contents[(int)d].clear(); }

	// Sets to an invalid range, which can contain nothing.
	void set_invalid() { set_invalid_height(); set_invalid_width(); }

	void set_invalid_width() { m_contents[0].set_invalid(); }
	void set_invalid_height() { m_contents[1].set_invalid(); }

	void set_width(double mn, double mx, bool hasMax = true) { m_contents[0].set(mn, mx, hasMax); }
	void set_height(double mn, double mx, bool hasMax = true) { m_contents[1].set(mn, mx, hasMax); }

	void set_width(const linear::range& r) { m_contents[0] = r; }
	void set_height(const linear::range& r) { m_contents[1] = r; }

	void set_fixed_width(double w) { m_contents[0].set_fixed(w); }
	void set_fixed_height(double h) { m_contents[1].set_fixed(h); }

	void set_fixed(dimension d, double n) { m_contents[(int)d].set_fixed(n); }

	void set_fixed(double w, double h) { set_fixed_width(w); set_fixed_height(h); }
	void set_fixed(const size& sz) { set_fixed(sz.get_width(), sz.get_height()); }

	bool& has_max_width() { return m_contents[0].has_max(); }
	bool has_max_width() const { return m_contents[0].has_max(); }

	bool& has_max_height() { return m_contents[1].has_max(); }
	bool has_max_height() const { return m_contents[1].has_max(); }

	bool& has_max(dimension d) { return (d == dimension::horizontal) ? has_max_width() : has_max_height(); }
	bool has_max(dimension d) const { return (d == dimension::horizontal) ? has_max_width() : has_max_height(); }

	size get_min() const
	{
		size sz(m_contents[0].get_min(), m_contents[1].get_min());
		return sz;
	}

	size get_max() const
	{
		size sz(m_contents[0].get_max(), m_contents[1].get_max());
		return sz;
	}

	double& get_min(dimension d) { return m_contents[(int)d].get_min(); }
	double get_min(dimension d) const { return m_contents[(int)d].get_min(); }

	double& get_max(dimension d) { return m_contents[(int)d].get_max(); }
	double get_max(dimension d) const { return m_contents[(int)d].get_max(); }

	double& get_min_width() { return m_contents[0].get_min(); }
	double get_min_width() const { return m_contents[0].get_min(); }

	double& get_max_width() { return m_contents[0].get_max(); }
	double get_max_width() const { return m_contents[0].get_max(); }

	double& get_min_height() { return m_contents[1].get_min(); }
	double get_min_height() const { return m_contents[1].get_min(); }

	double& get_max_height() { return m_contents[1].get_max(); }
	double get_max_height() const { return m_contents[1].get_max(); }

	linear::range& get_width() { return m_contents[0]; }
	const linear::range& get_width() const { return m_contents[0]; }

	linear::range& get_height() { return m_contents[1]; }
	const linear::range& get_height() const { return m_contents[1]; }

	linear::range& operator[](dimension d) { return m_contents[(int)d]; }
	const linear::range& operator[](dimension d) const { return m_contents[(int)d]; }

	// invalid
	bool is_width_invalid() const { return m_contents[0].is_invalid(); }
	bool is_height_invalid() const { return m_contents[1].is_invalid(); }

	bool is_invalid(dimension d) const { return m_contents[(int)d].is_invalid(); }

	bool is_invalid() const { return is_height_invalid() || is_width_invalid(); }

	// A range is empty if its either invalid or has a maximum height or width of 0.
	bool is_empty() const
	{
		if (!has_max_width() || !has_max_height())
			return false;
		return (!get_max_width() || !get_max_height() || (get_min_width() > get_max_width()) || (get_min_height() > get_max_height()));
	}

	bool is_fixed() const
	{
		return get_max_width() && get_max_height() && (get_min_width() == get_max_width() && (get_min_height() == get_max_height()));
	}

	bool is_width_fixed() const { return m_contents[0].is_fixed(); }
	bool is_height_fixed() const { return m_contents[1].is_fixed(); }

	bool is_fixed(dimension d) const { return m_contents[(int)d].is_fixed(); }

	bool contains(dimension d, double x) const { return m_contents[(int)d].contains(x); }

	bool contains(double x, double y) const  { return m_contents[0].contains(x) && m_contents[1].contains(y); }

	bool contains(const size& sz) const  { return contains(sz.get_width(), sz.get_height()); }

	// equality
	bool operator==(const range& r) const { return (m_contents[0] == r.m_contents[0]) && (m_contents[1] == r.m_contents[1]); }

	// inequality
	bool operator!=(const range& r) const { return !operator==(r); }

	// range + range = range
	range operator+(const range& r) const
	{
		range result(m_contents[0] + r.m_contents[0], m_contents[1] + r.m_contents[1]);
		return result;
	}

	range& operator+=(const range& r)
	{
		m_contents[0] += r.m_contents[0];
		m_contents[1] += r.m_contents[1];
		return *this;
	}


	// range + size = range
	range operator+(const size& sz) const
	{
		range result(m_contents[0] + sz.get_width(), m_contents[1] + sz.get_height());
		return result;
	}

	range& operator+=(const size& sz)
	{
		m_contents[0] += sz.get_width();
		m_contents[1] += sz.get_height();
		return *this;
	}


	// range + margin = range
	range operator+(const margin& m) const
	{
		range result(m_contents[0] + m.get_width(), m_contents[1] + m.get_height());
		return result;
	}

	range& operator+=(const margin& m)
	{
		m_contents[0] += m.get_width();
		m_contents[1] += m.get_height();
		return *this;
	}


	// range - range = range
	range operator-(const range& r) const
	{
		range result(m_contents[0] - r.m_contents[0], m_contents[1] - r.m_contents[1]);
		return result;
	}

	range& operator-=(const range& r)
	{
		m_contents[0] -= r.m_contents[0];
		m_contents[1] -= r.m_contents[1];
		return *this;
	}

	// range - size = range
	range operator-(const size& sz) const
	{
		range result(m_contents[0] - sz.get_width(), m_contents[1] - sz.get_height());
		return result;
	}

	range& operator-=(const size& sz)
	{
		m_contents[0] -= sz.get_width();
		m_contents[1] -= sz.get_height();
		return *this;
	}

	// range - margin = range
	range operator-(const margin& m) const
	{
		range result(m_contents[0] - m.get_width(), m_contents[1] - m.get_height());
		return result;
	}

	range& operator-=(const margin& m)
	{
		m_contents[0] -= m.get_width();
		m_contents[1] -= m.get_height();
		return *this;
	}


	// range * number = range
	range operator*(double d) const
	{
		range result(m_contents[0] * d, m_contents[1] * d);
		return result;
	}

	range& operator*=(double d)
	{
		m_contents[0] *= d;
		m_contents[1] *= d;
		return *this;
	}

	// range * proportion = range
	range operator*(const proportion& p) const
	{
		range result(m_contents[0] * p[dimension::horizontal], m_contents[1] * p[dimension::vertical]);
		return result;
	}

	range& operator*=(const proportion& p)
	{
		m_contents[0] *= p[dimension::horizontal];
		m_contents[1] *= p[dimension::vertical];
		return *this;
	}


	// range * double[2] = range
	range operator*(const double(&p)[2]) const
	{
		range result(m_contents[0] * p[0], m_contents[1] * p[1]);
		return result;
	}

	range& operator*=(const double(&p)[2])
	{
		m_contents[0] *= p[0];
		m_contents[1] *= p[1];
		return *this;
	}


	// range / number = range
	range operator/(double d) const
	{
		range result(m_contents[0] / d, m_contents[1] / d);
		return result;

	}

	range& operator/=(double d)
	{
		m_contents[0] /= d;
		m_contents[1] /= d;
		return *this;
	}

	// range / proportion = range
	range operator/(const proportion& p) const
	{
		range result(m_contents[0] / p[dimension::horizontal], m_contents[1] / p[dimension::vertical]);
		return result;
	}

	range& operator/=(const proportion& p)
	{
		m_contents[0] /= p[dimension::horizontal];
		m_contents[1] /= p[dimension::vertical];
		return *this;
	}

	// range / proportion = range
	range operator/(const double(&p)[2]) const
	{
		range result(m_contents[0] / p[0], m_contents[1] / p[1]);
		return result;
	}

	range& operator/=(const double(&p)[2])
	{
		m_contents[0] /= p[0];
		m_contents[1] /= p[1];
		return *this;
	}

	// range divide_whole number = range
	range divide_whole(double d) const
	{
		range result(m_contents[0].divide_whole(d), m_contents[1].divide_whole(d));
		return result;
	}

	void assign_divide_whole(double d)
	{
		m_contents[0].assign_divide_whole(d);
		m_contents[1].assign_divide_whole(d);
	}

	// range divide_whole proportion = range
	range divide_whole(const proportion& p) const
	{
		range result(m_contents[0].divide_whole(p[dimension::horizontal]), m_contents[1].divide_whole(p[dimension::vertical]));
		return result;
	}

	void assign_divide_whole(const proportion& p)
	{
		m_contents[0].assign_divide_whole(p[dimension::horizontal]);
		m_contents[1].assign_divide_whole(p[dimension::vertical]);
	}

	// range divide_whole proportion = range
	range divide_whole(const double(&p)[2]) const
	{
		range result(m_contents[0].divide_whole(p[0]), m_contents[1].divide_whole(p[1]));
		return result;
	}

	void assign_divide_whole(const double(&p)[2])
	{
		m_contents[0].assign_divide_whole(p[0]);
		m_contents[1].assign_divide_whole(p[1]);
	}

	// range | range = range (union)
	range operator|(const range& r) const
	{
		range result(m_contents[0] | r.m_contents[0], m_contents[1] | r.m_contents[1]);
		return result;
	}

	range& operator|=(const range& r)
	{
		m_contents[0] |= r.m_contents[0];
		m_contents[1] |= r.m_contents[1];
		return *this;
	}

	// range & range = range (intersection)
	range operator&(const range& r) const
	{
		range result(m_contents[0] & r.m_contents[0], m_contents[1] & r.m_contents[1]);
		return result;
	}

	range& operator&=(const range& r)
	{
		m_contents[0] &= r.m_contents[0];
		m_contents[1] &= r.m_contents[1];
		return *this;
	}


	// range ^ range = range (overlap)
	range operator^(const range& r) const
	{
		range result(m_contents[0] ^ r.m_contents[0], m_contents[1] ^ r.m_contents[1]);
		return result;
	}

	range& operator^=(const range& r)
	{
		m_contents[0] ^= r.m_contents[0];
		m_contents[1] ^= r.m_contents[1];
		return *this;
	}

	double get_limit(dimension d, double d2) const { return m_contents[(int)d].get_limit(d2); }
	void limit(dimension d, double d2) const { m_contents[(int)d].limit(d2); }

	point get_limit(const point& pt) const;
	point get_limit_min(const point& pt) const;
	point get_limit_max(const point& pt) const;

	void limit(point& pt) const;
	void limit_min(point& pt) const;
	void limit_max(point& pt) const;

	size get_limit(const size& sz) const
	{
		size result(m_contents[0].get_limit(sz.get_width()), m_contents[1].get_limit(sz.get_height()));
		return result;
	}
	void limit(size& sz) const
	{
		m_contents[0].limit(sz.get_width());
		m_contents[1].limit(sz.get_height());
	}

	size get_limit_min(const size& sz) const
	{
		size result(m_contents[0].get_limit_min(sz.get_width()), m_contents[1].get_limit_min(sz.get_height()));
		return result;
	}

	void limit_min(size& sz) const
	{
		m_contents[0].limit_min(sz.get_width());
		m_contents[1].limit_min(sz.get_height());
	}

	size get_limit_max(const size& sz) const
	{
		size result(m_contents[0].get_limit_max(sz.get_width()), m_contents[1].get_limit_max(sz.get_height()));
		return result;
	}

	void limit_max(size& sz) const
	{
		m_contents[0].limit_max(sz.get_width());
		m_contents[1].limit_max(sz.get_height());
	}

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;

		static constexpr char_t part1 = (char_t)'{';
		static constexpr char_t part2 = (char_t)',';
		static constexpr char_t part3 = (char_t)'}';

		result = string_t<char_t>::contain(&part1, 1);
		result += get_width().template to_string_t<char_t>();
		result += string_t<char_t>::contain(&part2, 1);
		result += get_height().template to_string_t<char_t>();
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
