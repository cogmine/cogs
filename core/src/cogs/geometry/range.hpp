//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_RANGE
#define COGS_HEADER_GEOMETRY_RANGE


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
class range
{
public:
	class maximum_comparator
	{
	public:
		static bool is_less_than(const range& t1, const range& t2)
		{
			bool result;
			if (!t1)
				result = true;
			else if (!t2)
				result = false;
			else if (!t1.has_max()) // If invalid/empty range, its min
				result = false; // If we don't have a max, put us last (or equal to others that are last)
			else if (!t2.has_max())
				result = true; // If we have a max, and they don't, we are definitely lesser.
			else
				result = t1.get_max() < t2.get_max(); // Both have max.  Compare their maxes.
			return result;
		}
	};

	class minimum_comparator
	{
	public:
		static bool is_less_than(const range& t1, const range& t2)
		{
			// Put invalid/empty ranges first, with a min/max of 0/0
			// If mins are equal, put one with smaller max first
			bool result;
			if (!t1.has_max())
				result = t1.get_min() < t2.get_min();
			else if (!t2.has_max())
				result = t1.get_min() <= t2.get_min();
			else if (t1.get_min() > t1.get_max()) // t1 is invalid/empty, put it first.
				result = true;
			else if (t2.get_min() > t2.get_max()) // t2 is invalid/empty, put it first.
				result = false;
			else if (t1.get_min() == t2.get_min())
				result = t1.get_max() < t2.get_max();
			else
				result = t1.get_min() < t2.get_min();
			return result;
		}
	};

	// default comparator compares based on side of the range.

private:
	double m_max{ 0 };
	double m_min{ 0 };
	bool m_hasMax = false;

public:
	static range make_unbounded() { range r; return r; }

	static range make_fixed(double d) { range r(d, d, true); return r; }

	static range make_empty()
	{
		range r(1, 0, true);
		return r;
	}

	range()
	{ }

	range(const range& src)
		: m_max(src.m_max),
		m_min(src.m_min),
		m_hasMax(src.m_hasMax)
	{ }

	range(double min, double max, bool hasMax)
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
	void set_empty() { m_min = 1; m_max = 0; m_hasMax = true; }

	void set(double mn, double mx)
	{
		m_min = mn;
		m_max = mx;
		m_hasMax = true;
	}

	void set_fixed(double x)
	{
		m_hasMax = true;
		m_min = x;
		m_max = x;
	}

	void set_min(double x) { m_min = x; }

	void set_max(double x) { m_max = x; m_hasMax = true; }

	// get
	bool& has_max() { return m_hasMax; }
	bool has_max() const { return m_hasMax; }

	double& get_min() { return m_min; }
	double get_min() const { return m_min; }
	double& get_max() { return m_max; }
	double get_max() const { return m_max; }

	// empty/not
	bool is_empty() const { return (m_hasMax && (m_min > m_max)); }
	bool operator!() const { return is_empty(); }

	bool is_fixed() const { return m_hasMax && (m_min == m_max); }

	bool contains(double d) const { return (d >= m_min) && (!m_hasMax || (d <= m_max )); }

	// equality
	bool operator==(const range& cmp) const
	{
		bool result = true;
		if (&cmp != this)
		{
			if (m_hasMax != cmp.has_max())
				result = false;
			else if (!m_hasMax)
				result = m_min == cmp.m_min;
			else
			{
				bool isEmpty = (m_min > m_max);
				bool cmpIsEmpty = (cmp.get_min() > cmp.get_max());
				if (isEmpty)
					result = cmpIsEmpty;
				else if (cmpIsEmpty)
					result = false;
				else
					result = (m_max == cmp.m_max) && (m_min == cmp.m_min);
			}
		}
		return result;
	}

	// inequality
	bool operator!=(const range& cmp) const
	{
		return !operator==(cmp);
	}

	// less than
	bool operator<(const range& cmp) const
	{
		// lesser is the one with the least difference between min and max.
		// If neither have a max, the lesser is the one with the highest min.
		// Invalid/empty ranges are considered equal and least.
		bool result = false;
		if (&cmp != this)
		{
			if (!m_hasMax)
			{
				if (cmp.has_max())
					result = false;
				else
					result = m_min > cmp.get_min();
			}
			else if (!cmp.has_max())
				result = true;
			else if (cmp.get_max() < cmp.get_min())
				result = false;
			else if (m_max < m_min)
				result = true;
			else
				result = (m_max - m_min) < (cmp.get_max() - cmp.get_min());
		}
		return result;
	}

	// greater than
	bool operator>(const range& cmp) const
	{
		bool result = false;
		if (&cmp != this)
		{
			if (!m_hasMax)
			{
				if (cmp.has_max())
					result = true;
				else
					result = m_min < cmp.get_min();
			}
			else if (!cmp.has_max())
				result = false;
			else if (cmp.get_max() < cmp.get_min())
				result = true;
			else if (m_max < m_min)
				result = false;
			else
				result = (m_max - m_min) >(cmp.get_max() - cmp.get_min());
		}
		return result;
	}

	// range + range = range
	auto operator+(const range& r) const
	{
		range result;
		if (is_empty() || r.is_empty())
			result.set_empty();
		else
		{
			result.get_min() = m_min + r.get_min();
			result.has_max() = m_hasMax && r.has_max();
			if (result.has_max())
				result.get_max() = m_max + r.get_max();
		}

		return result;
	}

	range& operator+=(const range& r)
	{
		if (!is_empty())
		{
			if (r.is_empty())
				set_empty();
			else
			{
				m_min += r.get_min();
				m_hasMax &= r.has_max();
				if (m_hasMax)
					m_max += r.get_max();
			}
		}
		return *this;
	}

	// range + size = range
	auto operator+(double sz) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			result.get_min() = m_min + sz;
			result.has_max() = m_hasMax;
			if (m_hasMax)
				result.get_max() = m_max + sz;
		}
		return result;
	}

	range& operator+=(double sz)
	{
		if (!is_empty())
		{
			m_min += sz;
			if (m_hasMax)
				m_max += sz;
		}
		return *this;
	}

	// range + margin = range
	range operator+(const margin& m) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			double t = m.get_size();
			result.get_min() = m_min + t;
			result.m_hasMax = m_hasMax;
			if (m_hasMax)
				result.get_max() = m_max + t;
		}
		return result;
	}

	range& operator+=(const margin& m)
	{
		if (!is_empty())
		{
			double t = m.get_size();
			m_min += t;
			if (m_hasMax)
				m_max += t;
		}
		return *this;
	}


	// range - range = range
	auto operator-(const range& r) const
	{
		range result;
		if (is_empty() || r.is_empty())
			result.set_empty();
		else
		{
			result.get_min() = (m_min > r.get_min()) ? (m_min - r.get_min()) : 0;
			result.has_max() = m_hasMax && r.has_max();
			if (result.has_max())
				result.get_max() = (m_max > r.get_max()) ? (m_max - r.get_max()) : 0;
		}

		return result;
	}

	range& operator-=(const range& r)
	{
		if (!is_empty())
		{
			if (r.is_empty())
				set_empty();
			else
			{
				m_min = (m_min > r.get_min()) ? (m_min - r.get_min()) : 0;
				m_hasMax &= r.has_max();
				if (m_hasMax)
					m_max = (m_max > r.get_max()) ? (m_max - r.get_max()) : 0;
			}
		}
		return *this;
	}

	// range - size = range
	auto operator-(double sz) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			result.get_min() = (m_min > sz) ? (m_min - sz) : 0;
			result.has_max() = m_hasMax;
			if (m_hasMax)
				result.get_max() = (m_max > sz) ? (m_max - sz) : 0;
		}
		return result;
	}

	range& operator-=(double sz)
	{
		if (!is_empty())
		{
			m_min = (m_min > sz) ? (m_min - sz) : 0;
			if (m_hasMax)
				m_max = (m_max > sz) ? (m_max - sz) : 0;
		}
		return *this;
	}

	// range - margin = range
	range operator-(const margin& m) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			double t = m.get_size();
			result.get_min() = (m_min > t) ? (m_min - t) : 0;
			result.m_hasMax = m_hasMax;
			if (m_hasMax)
				result.get_max() = (m_max > t) ? (m_max - t) : 0;
		}
		return result;
	}

	range& operator-=(const margin& m)
	{
		if (!is_empty())
		{
			double t = m.get_size();
			m_min = (m_min > t) ? (m_min - t) : 0;
			if (m_hasMax)
				m_max = (m_max > t) ? (m_max - t) : 0;
		}
		return *this;
	}


	// range * number = range
	range operator*(double d) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			result.get_min() = m_min * d;
			result.has_max() = m_hasMax;
			if (m_hasMax)
				result.get_max() = m_max * d;
		}
		return result;
	}

	range& operator*=(double d)
	{
		if (!is_empty())
		{
			if (m_hasMax)
				m_max *= d;
			m_min *= d;
		}
		return *this;
	}

	// range / number = range
	range operator/(double d) const
	{
		range result;
		if (is_empty())
			result.set_empty();
		else
		{
			result.get_min() = m_min / d;
			result.has_max() = m_hasMax;
			if (m_hasMax)
				result.get_max() = m_max / d;
		}
		return result;
	}

	range& operator/=(double d)
	{
		if (!is_empty())
		{
			if (m_hasMax)
				m_max /= d;
			m_min /= d;
		}
		return *this;
	}

	// range | range = range (union)
	range operator|(const range& r) const
	{
		range result;
		if (is_empty())
			result = r;
		else if (r.is_empty())
			result = *this;
		else
		{
			result.get_min() = (m_min > r.get_min()) ? r.get_min() : m_min;
			result.has_max() = m_hasMax && r.has_max() && m_hasMax;
			if (result.has_max())
				result.m_max = (m_max < r.get_max()) ? r.get_max() : m_max;
		}
		return result;
	}

	range& operator|=(const range& r)
	{
		if (!r.is_empty())
		{
			if (is_empty())
				*this = r;
			else
			{
				if (m_min > r.get_min())
					m_min = r.get_min();
				if (!r.has_max())
					m_hasMax = false;
				else if (m_hasMax && (m_max < r.get_max()))
					m_max = r.get_max();
			}
		}
		return *this;
	}


	// range & range = range (intersection)
	range operator&(const range& r) const
	{
		range result;
		if (r.is_empty() || is_empty())
			result.set_empty();
		else
		{
			result.m_min = (m_min < r.get_min()) ? r.get_min() : m_min;
			if (r.has_max())
			{
				result.m_max = (!m_hasMax || (m_max > r.get_max())) ? r.get_max() : m_max;
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
		if (r.is_empty())
			set_empty();
		else if (!is_empty())
		{
			if (m_min < r.get_min())
				m_min = r.get_min();
			if (r.has_max())
			{
				if (!m_hasMax || (m_max > r.get_max()))
					m_max = r.get_max();
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
		if (r.is_empty() || is_empty())
			result.set_empty();
		else
		{
			result.m_min = (m_min < r.get_min()) ? r.get_min() : m_min;
			result.m_hasMax = (r.has_max() && m_hasMax);
			if (result.m_hasMax)
				result.m_max = (m_max > r.get_max()) ? r.get_max() : m_max;
		}
		return result;
	}

	range& operator^=(const range& r)
	{
		if (!is_empty())
		{
			if (r.is_empty())
				set_empty();
			else
			{
				if (m_min < r.get_min())
					m_min = r.get_min();
				m_hasMax &= r.has_max();
				if (m_hasMax && (m_max < r.get_max()))
					m_max = r.get_max();
			}
		}
		return *this;
	}

	double limit(double d) const
	{
		return (m_hasMax && d > m_max) ? m_max : cogs::greater(d, m_min);
	}

	double limit_min(double d) const
	{
		return cogs::greater(d, m_min);
	}

	double limit_max(double d) const
	{
		if (m_hasMax)
			return cogs::lesser(d, m_max);
		return d;
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

class range
{
private:
	size m_minSize{ 0, 0 };
	size m_maxSize{ 0, 0 };
	bool m_hasMaxWidth = false;
	bool m_hasMaxHeight = false;

public:
	typedef linear::range linear_t;

	static range make_unbounded() { range r; return r; }

	static range make_fixed(const size& sz) { range r(sz, sz, true, true); return r; }

	static range make_empty()
	{
		range r(size(1, 1), size(0, 0), true, true);
		return r;
	}

	range()
	{ }

	range(const range& src)
		: m_minSize(src.m_minSize),
		m_maxSize(src.m_maxSize),
		m_hasMaxWidth(src.m_hasMaxWidth),
		m_hasMaxHeight(src.m_hasMaxHeight)
	{ }

	range(const size& fixedSize)
		: m_minSize(fixedSize),
		m_maxSize(fixedSize),
		m_hasMaxWidth(true),
		m_hasMaxHeight(true)
	{ }

	range(const linear::range& horizontalRange, const linear::range& verticalRange)
		: m_minSize(horizontalRange.get_min(), verticalRange.get_min()),
		m_maxSize(horizontalRange.get_max(), verticalRange.get_max()),
		m_hasMaxWidth(horizontalRange.has_max()),
		m_hasMaxHeight(verticalRange.has_max())
	{ }

	range(const size& minSize, const size& maxSize, bool hasMaxWidth, bool hasMaxHeight)
		: m_minSize(minSize),
		m_maxSize(maxSize),
		m_hasMaxWidth(hasMaxWidth),
		m_hasMaxHeight(hasMaxHeight)
	{ }

	range(dimension d, double mn, double mx)
	{
		if (d == dimension::horizontal)
			set_width(mn, mx);
		else
			set_height(mn, mx);
	}

	range(dimension d, const linear::range& r)
	{
		if (d == dimension::horizontal)
			set_width(r);
		else
			set_height(r);
	}

	range& operator=(const range& src)
	{
		m_minSize = src.m_minSize;
		m_maxSize = src.m_maxSize;
		m_hasMaxWidth = src.m_hasMaxWidth;
		m_hasMaxHeight = src.m_hasMaxHeight;
		return *this;
	}

	range& operator=(const size& fixedSize)
	{
		m_minSize = fixedSize;
		m_maxSize = fixedSize;
		m_hasMaxWidth = true;
		m_hasMaxHeight = true;
		return *this;
	}

	void set(const linear::range& horizontalRange, const linear::range& verticalRange)
	{
		m_minSize.set(horizontalRange.get_min(), verticalRange.get_min());
		m_maxSize.set(horizontalRange.get_max(), verticalRange.get_max());
		m_hasMaxWidth = horizontalRange.has_max();
		m_hasMaxHeight = verticalRange.has_max();
	}

	void set(const size& minSize, const size& maxSize, bool hasMaxWidth, bool hasMaxHeight)
	{
		m_minSize = minSize;
		m_maxSize = maxSize;
		m_hasMaxWidth = hasMaxWidth;
		m_hasMaxHeight = hasMaxHeight;
	}

	void set(dimension d, double mn, double mx)
	{
		if (d == dimension::horizontal)
			set_width(mn, mx);
		else
			set_height(mn, mx);
	}

	void set(dimension d, const linear::range& r)
	{
		if (d == dimension::horizontal)
			set_width(r);
		else
			set_height(r);
	}

	void set_min_width(double w) { m_minSize.set_width(w); }
	void set_min_height(double h) { m_minSize.set_height(h); }

	void set_min(const size& minSize) { m_minSize = minSize; }

	void set_min(double x, double y) { m_minSize.set(x, y); }

	void set_min(dimension d, double n)
	{
		if (d == dimension::horizontal)
			set_min_width(n);
		else
			set_min_height(n);
	}

	void set_max_width(double w) { m_hasMaxWidth = true; m_maxSize.set_width(w); }
	void set_max_height(double h) { m_hasMaxHeight = true; m_maxSize.set_height(h); }

	void set_max(const size& maxSize) { m_hasMaxWidth = true; m_hasMaxHeight = true; m_maxSize = maxSize; }

	void set_max(double x, double y) { m_hasMaxWidth = true; m_hasMaxHeight = true; m_maxSize.set(x, y); }

	void set_max(dimension d, double n)
	{
		if (d == dimension::horizontal)
			set_max_width(n);
		else
			set_max_height(n);
	}

	void clear_max_width() { m_hasMaxWidth = false; }
	void clear_max_height() { m_hasMaxHeight = false; }
	void clear_max() { m_hasMaxHeight = false; m_hasMaxWidth = false; }

	void clear_max(dimension d)
	{
		if (d == dimension::horizontal)
			clear_max_width();
		else
			clear_max_height();
	}

	void clear_min_width() { m_minSize.set_width(0); }
	void clear_min_height() { m_minSize.set_height(0); }
	void clear_min() { m_minSize.set(0, 0); }

	void clear_min(dimension d)
	{
		if (d == dimension::horizontal)
			clear_min_width();
		else
			clear_min_height();
	}

	// Sets to an unlimited range
	void clear() { clear_min(); clear_max(); }

	void clear_width() { clear_min_width(); clear_max_width(); }
	void clear_height() { clear_min_height(); clear_max_height(); }

	void clear(dimension d)
	{
		if (d == dimension::horizontal)
			clear_width();
		else
			clear_height();
	}

	// Sets to an invalid/empty range, which can contain nothing.
	void set_empty() { set_empty_height(); set_empty_width(); }

	void set_empty_width() { set_min_width(1); set_max_width(0); m_hasMaxWidth = true; }
	void set_empty_height() { set_min_height(1); set_max_height(0); m_hasMaxHeight = true; }

	void set_width(double mn, double mx)
	{
		m_hasMaxWidth = true;
		m_minSize.set_width(mn);
		m_maxSize.set_width(mx);
	}

	void set_height(double mn, double mx)
	{
		m_hasMaxHeight = true;
		m_minSize.set_height(mn);
		m_maxSize.set_height(mx);
	}

	void set_width(const linear::range& r)
	{
		m_hasMaxWidth = r.has_max();
		m_minSize.set_width(r.get_min());
		m_maxSize.set_width(r.get_max());
	}

	void set_height(const linear::range& r)
	{
		m_hasMaxHeight = r.has_max();
		m_minSize.set_height(r.get_min());
		m_maxSize.set_height(r.get_max());
	}

	void set_fixed_width(double w)
	{
		m_hasMaxWidth = true;
		m_minSize.set_width(w);
		m_maxSize.set_width(w);
	}

	void set_fixed_height(double h)
	{
		m_hasMaxHeight = true;
		m_minSize.set_height(h);
		m_maxSize.set_height(h);
	}

	void set_fixed(dimension d, double n)
	{
		if (d == dimension::horizontal)
			set_fixed_width(n);
		else
			set_fixed_height(n);
	}

	void set_fixed(const size& sz)
	{
		m_hasMaxWidth = true;
		m_hasMaxHeight = true;
		m_minSize = sz;
		m_maxSize = sz;
	}

	bool& has_max_width() { return m_hasMaxWidth; }
	bool has_max_width() const { return m_hasMaxWidth; }

	bool& has_max_height() { return m_hasMaxHeight; }
	bool has_max_height() const { return m_hasMaxHeight; }

	bool& has_max(dimension d) { return (d == dimension::horizontal) ? has_max_width() : has_max_height(); }
	bool has_max(dimension d) const { return (d == dimension::horizontal) ? has_max_width() : has_max_height(); }

	size& get_min() { return m_minSize; }
	const size& get_min() const { return m_minSize; }

	double& get_min(dimension d) { return m_minSize[d]; }
	double get_min(dimension d) const { return m_minSize[d]; }

	size& get_max() { return m_maxSize; }
	const size& get_max() const { return m_maxSize; }

	double& get_max(dimension d) { return m_maxSize[d]; }
	double get_max(dimension d) const { return m_maxSize[d]; }

	double& get_min_height() { return m_minSize.get_height(); }
	double get_min_height() const { return m_minSize.get_height(); }

	double& get_max_height() { return m_maxSize.get_height(); }
	double get_max_height() const { return m_maxSize.get_height(); }

	double& get_min_width() { return m_minSize.get_width(); }
	double get_min_width() const { return m_minSize.get_width(); }

	double& get_max_width() { return m_maxSize.get_width(); }
	double get_max_width() const { return m_maxSize.get_width(); }

	const linear::range get_width() const { return linear::range(m_minSize.get_width(), m_maxSize.get_width(), m_hasMaxWidth); }
	const linear::range get_height() const { return linear::range(m_minSize.get_height(), m_maxSize.get_height(), m_hasMaxHeight); }

	const linear::range operator[](dimension d) const { return (d == dimension::horizontal) ? get_width() : get_height(); }

	// empty/not
	bool is_height_empty() const { return m_hasMaxHeight && (get_min_height() > get_max_height()); }
	bool is_width_empty() const { return m_hasMaxWidth && (get_min_width() > get_max_width()); }
	bool is_empty(dimension d) const { return (d == dimension::horizontal) ? is_width_empty() : is_height_empty(); }

	bool is_empty() const { return is_height_empty() || is_width_empty(); }
	bool operator!() const { return is_empty(); }

	bool is_fixed() const
	{
		return m_hasMaxWidth && m_hasMaxHeight && (get_min() == get_max());
	}

	bool is_width_fixed() const
	{
		return m_hasMaxWidth && (get_min_width() == get_max_height());
	}

	bool is_height_fixed() const
	{
		return m_hasMaxHeight && (get_min_height() == get_max_height());
	}

	bool is_fixed(dimension d) const
	{
		if (d == dimension::horizontal)
			return is_width_fixed();
		return is_height_fixed();
	}

	bool contains(dimension d, double x) const
	{
		return ((x >= get_min(d))
			&& (!has_max(d) || (x <= get_max(d))));
	}

	bool contains(const size& sz) const
	{
		return ((sz.get_width() >= get_min_width())
			&& (sz.get_height() >= get_min_height())
			&& (!m_hasMaxWidth || (sz.get_width() <= get_max_width()))
			&& (!m_hasMaxHeight || (sz.get_height() <= get_max_height())));
	}

	// equality
	bool operator==(const range& r) const
	{
		if ((m_minSize != r.get_min())
			|| (m_hasMaxWidth != r.has_max_width())
			|| (m_hasMaxHeight != r.has_max_height())
			|| (m_hasMaxWidth && (get_max_width() != r.get_max_width()))
			|| (m_hasMaxHeight && (get_max_height() != r.get_max_height())))
			return false;

		return true;
	}

	// inequality
	bool operator!=(const range& r) const
	{ return !operator==(r); }


	// range + range = range
	range operator+(const range& r) const
	{
		range result;
		if (is_width_empty() || r.is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = get_min_width() + r.get_min_width();
			result.has_max_width() = m_hasMaxWidth && r.has_max_width();
			if (result.has_max_width())
				result.get_max_width() = get_max_width() + r.get_max_width();
		}
		if (is_height_empty() || r.is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = get_min_height() + r.get_min_height();
			result.has_max_height() = m_hasMaxWidth && r.has_max_height();
			if (result.has_max_height())
				result.get_max_height() = get_max_height() + r.get_max_height();
		}
		return result;
	}

	range& operator+=(const range& r)
	{
		if (!is_width_empty())
		{
			get_min_width() += r.get_min_width();
			has_max_width() &= r.has_max_width();
			if (has_max_width())
				get_max_width() += r.get_max_width();
		}

		if (!is_height_empty())
		{
			get_min_height() += r.get_min_height();
			has_max_height() &= r.has_max_height();
			if (has_max_height())
				get_max_height() += r.get_max_height();
		}
		return *this;
	}


	// range + size = range
	range operator+(const size& sz) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = get_min_width() + sz.get_width();
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() + sz.get_width();
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = get_min_height() + sz.get_height();
			result.has_max_height() = m_hasMaxWidth;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() + sz.get_height();
		}
		return result;
	}

	range& operator+=(const size& sz)
	{
		if (!is_width_empty())
		{
			double d = sz.get_width();
			get_min_width() += d;
			if (has_max_width())
				get_max_width() += d;
		}
		if (!is_height_empty())
		{
			double d = sz.get_height();
			get_min_height() += d;
			if (has_max_height())
				get_max_height() += d;
		}
		return *this;
	}


	// range + margin = range
	range operator+(const margin& m) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double t = m.get_width();
			result.get_min_width() = get_min_width() + t;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() + t;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double t = m.get_height();
			result.get_min_height() = get_min_height() + t;
			result.has_max_height() = m_hasMaxWidth;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() + t;
		}
		return result;
	}

	range& operator+=(const margin& m)
	{
		if (!is_width_empty())
		{
			double d = m.get_width();
			get_min_width() += d;
			if (has_max_width())
				get_max_width() += d;
		}
		if (!is_height_empty())
		{
			double d = m.get_height();
			get_min_height() += d;
			if (has_max_height())
				get_max_height() += d;
		}
		return *this;
	}


	// range - range = range
	range operator-(const range& r) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = (get_min_width() > r.get_min_width()) ? (get_min_width() - r.get_min_width()) : 0;
			result.has_max_width() = m_hasMaxWidth && r.has_max_width();
			if (m_hasMaxWidth)
				result.get_max_width() = (get_max_width() > r.get_max_width()) ? (get_max_width() - r.get_max_width()) : 0;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = (get_min_height() > r.get_min_height()) ? (get_min_height() - r.get_min_height()) : 0;
			result.has_max_height() = m_hasMaxHeight && r.has_max_height();
			if (m_hasMaxHeight)
				result.get_max_height() = (get_max_height() > r.get_max_height()) ? (get_max_height() - r.get_max_height()) : 0;
		}
		return result;
	}

	range& operator-=(const range& r)
	{
		if (!is_width_empty())
		{
			get_min_width() = (get_min_width() > r.get_min_width()) ? (get_min_width() - r.get_min_width()) : 0;
			has_max_width() &= r.has_max_width();
			if (has_max_width())
				get_max_width() = (get_max_width() > r.get_max_width()) ? (get_max_width() - r.get_max_width()) : 0;
		}

		if (!is_height_empty())
		{
			get_min_height() = (get_min_height() > r.get_min_height()) ? (get_min_height() - r.get_min_height()) : 0;
			has_max_height() &= r.has_max_height();
			if (has_max_height())
				get_max_height() = (get_max_height() > r.get_max_height()) ? (get_max_height() - r.get_max_height()) : 0;
		}
		return *this;
	}

	// range - size = range
	range operator-(const size& sz) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = (get_min_width() > sz.get_width()) ? (get_min_width() - sz.get_width()) : 0;
			result.has_max_width() = m_hasMaxWidth;
			if (m_hasMaxWidth)
				result.get_max_width() = (get_max_width() > sz.get_width()) ? (get_max_width() - sz.get_width()) : 0;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = (get_min_height() > sz.get_height()) ? (get_min_height() - sz.get_height()) : 0;
			result.has_max_height() = m_hasMaxHeight;
			if (m_hasMaxHeight)
				result.get_max_height() = (get_max_height() > sz.get_height()) ? (get_max_height() - sz.get_height()) : 0;
		}
		return result;
	}

	range& operator-=(const size& sz)
	{
		if (!is_width_empty())
		{
			get_min_width() = (get_min_width() > sz.get_width()) ? (get_min_width() - sz.get_width()) : 0;
			if (has_max_width())
				get_max_width() = (get_max_width() > sz.get_width()) ? (get_max_width() - sz.get_width()) : 0;
		}
		if (!is_height_empty())
		{
			get_min_height() = (get_min_height() > sz.get_height()) ? (get_min_height() - sz.get_height()) : 0;
			if (has_max_height())
				get_max_height() = (get_max_height() > sz.get_height()) ? (get_max_height() - sz.get_height()) : 0;
		}
		return *this;
	}

	// range - margin = range
	range operator-(const margin& m) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double t = m.get_width();
			result.get_min_width() = (get_min_width() > t) ? (get_min_width() - t) : 0;
			result.m_hasMaxWidth = m_hasMaxWidth;
			if (m_hasMaxWidth)
				result.get_max_width() = (get_max_width() > t) ? (get_max_width() - t) : 0;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double t = m.get_height();
			result.get_min_height() = (get_min_height() > t) ? (get_min_height() - t) : 0;
			result.m_hasMaxHeight = m_hasMaxHeight;
			if (m_hasMaxHeight)
				result.get_max_height() = (get_max_height() > t) ? (get_max_height() - t) : 0;
		}
		return result;
	}

	range& operator-=(const margin& m)
	{
		if (!is_width_empty())
		{
			double t = m.get_width();
			get_min_width() = (get_min_width() > t) ? (get_min_width() - t) : 0;
			if (has_max_width())
				get_max_width() = (get_max_width() > t) ? (get_max_width() - t) : 0;
		}
		if (!is_height_empty())
		{
			double t = m.get_height();
			get_min_height() = (get_min_height() > t) ? (get_min_height() - t) : 0;
			if (has_max_height())
				get_max_height() = (get_max_height() > t) ? (get_max_height() - t) : 0;
		}
		return *this;
	}


	// range * number = range
	range operator*(double d) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = get_min_width() * d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() * d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = get_min_height() * d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() * d;
		}
		return result;
	}

	range& operator*=(double d)
	{
		if (!is_width_empty())
		{
			get_min_width() *= d;
			if (has_max_width())
				get_max_width() *= d;
		}
		if (!is_height_empty())
		{
			get_min_height() *= d;
			if (has_max_height())
				get_max_height() *= d;
		}
		return *this;
	}

	// range * proportion = range
	range operator*(const proportion& p) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[dimension::horizontal];
			result.get_min_width() = get_min_width() * d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() * d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[dimension::vertical];
			result.get_min_height() = get_min_height() * d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() * d;
		}
		return result;
	}

	range& operator*=(const proportion& p)
	{
		if (!is_width_empty())
		{
			double d = p[dimension::horizontal];
			get_min_width() *= d;
			if (has_max_width())
				get_max_width() *= d;
		}
		if (!is_height_empty())
		{
			double d = p[dimension::vertical];
			get_min_height() *= d;
			if (has_max_height())
				get_max_height() *= d;
		}
		return *this;
	}


	// range * double[2] = range
	range operator*(const double(&p)[2]) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[0];
			result.get_min_width() = get_min_width() * d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() * d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[1];
			result.get_min_height() = get_min_height() * d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() * d;
		}
		return result;
	}

	range& operator*=(const double(&p)[2])
	{
		if (!is_width_empty())
		{
			double d = p[0];
			get_min_width() *= d;
			if (has_max_width())
				get_max_width() *= d;
		}
		if (!is_height_empty())
		{
			double d = p[1];
			get_min_height() *= d;
			if (has_max_height())
				get_max_height() *= d;
		}
		return *this;
	}


	// range / number = range
	range operator/(double d) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = get_min_width() / d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() / d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = get_min_height() / d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() / d;
		}
		return result;
	}

	range& operator/=(double d)
	{
		if (!is_width_empty())
		{
			get_min_width() /= d;
			if (has_max_width())
				get_max_width() /= d;
		}
		if (!is_height_empty())
		{
			get_min_height() /= d;
			if (has_max_height())
				get_max_height() /= d;
		}
		return *this;
	}

	// range / proportion = range
	range operator/(const proportion& p) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[dimension::horizontal];
			result.get_min_width() = get_min_width() / d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() / d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[dimension::vertical];
			result.get_min_height() = get_min_height() / d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() / d;
		}
		return result;
	}

	range& operator/=(const proportion& p)
	{
		if (!is_width_empty())
		{
			double d = p[dimension::horizontal];
			get_min_width() /= d;
			if (has_max_width())
				get_max_width() /= d;
		}
		if (!is_height_empty())
		{
			double d = p[dimension::vertical];
			get_min_height() /= d;
			if (has_max_height())
				get_max_height() /= d;
		}
		return *this;
	}

	// range / proportion = range
	range operator/(const double(&p)[2]) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[0];
			result.get_min_width() = get_min_width() / d;
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = get_max_width() / d;
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[1];
			result.get_min_height() = get_min_height() / d;
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = get_max_height() / d;
		}
		return result;
	}

	range& operator/=(const double(&p)[2])
	{
		if (!is_width_empty())
		{
			double d = p[0];
			get_min_width() /= d;
			if (has_max_width())
				get_max_width() /= d;
		}
		if (!is_height_empty())
		{
			double d = p[1];
			get_min_height() /= d;
			if (has_max_height())
				get_max_height() /= d;
		}
		return *this;
	}


	// range divide_whole number = range
	range divide_whole(double d) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = cogs::divide_whole(get_min_width(), d);
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = cogs::divide_whole(get_max_width(), d);
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = cogs::divide_whole(get_min_height(), d);
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = cogs::divide_whole(get_max_height(), d);
		}
		return result;
	}

	void assign_divide_whole(double d)
	{
		if (!is_width_empty())
		{
			cogs::assign_divide_whole(get_min_width(), d);
			if (has_max_width())
				cogs::assign_divide_whole(get_max_width(), d);
		}
		if (!is_height_empty())
		{
			cogs::assign_divide_whole(get_min_height(), d);
			if (has_max_height())
				cogs::assign_divide_whole(get_max_height(), d);
		}
	}

	// range divide_whole proportion = range
	range divide_whole(const proportion& p) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[dimension::horizontal];
			result.get_min_width() = cogs::divide_whole(get_min_width(), d);
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = cogs::divide_whole(get_max_width(), d);
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[dimension::vertical];
			result.get_min_height() = cogs::divide_whole(get_min_height(), d);
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = cogs::divide_whole(get_max_height(), d);
		}
		return result;
	}

	void assign_divide_whole(const proportion& p)
	{
		if (!is_width_empty())
		{
			double d = p[dimension::horizontal];
			cogs::assign_divide_whole(get_min_width(), d);
			if (has_max_width())
				cogs::assign_divide_whole(get_max_width(), d);
		}
		if (!is_height_empty())
		{
			double d = p[dimension::vertical];
			cogs::assign_divide_whole(get_min_height(), d);
			if (has_max_height())
				cogs::assign_divide_whole(get_max_height(), d);
		}
	}

	// range divide_whole proportion = range
	range divide_whole(const double(&p)[2]) const
	{
		range result;
		if (is_width_empty())
			result.set_empty_width();
		else
		{
			double d = p[0];
			result.get_min_width() = cogs::divide_whole(get_min_width(), d);
			result.has_max_width() = m_hasMaxWidth;
			if (result.has_max_width())
				result.get_max_width() = cogs::divide_whole(get_max_width(), d);
		}
		if (is_height_empty())
			result.set_empty_height();
		else
		{
			double d = p[1];
			result.get_min_height() = cogs::divide_whole(get_min_height(), d);
			result.has_max_height() = m_hasMaxHeight;
			if (result.has_max_height())
				result.get_max_height() = cogs::divide_whole(get_max_height(), d);
		}
		return result;
	}

	void assign_divide_whole(const double(&p)[2])
	{
		if (!is_width_empty())
		{
			double d = p[0];
			cogs::assign_divide_whole(get_min_width(), d);
			if (has_max_width())
				cogs::assign_divide_whole(get_max_width(), d);
		}
		if (!is_height_empty())
		{
			double d = p[1];
			cogs::assign_divide_whole(get_min_height(), d);
			if (has_max_height())
				cogs::assign_divide_whole(get_max_height(), d);
		}
	}


	// range | range = range (union)
	range operator|(const range& r) const
	{
		range result;
		if (is_width_empty())
		{
			result.get_min_width() = r.get_min_width();
			result.has_max_width() = r.has_max_width();
			if (result.has_max_width())
				result.get_max_width() = r.get_max_width();
		}
		else if (r.is_width_empty())
		{
			result.get_min_width() = get_min_width();
			result.has_max_width() = has_max_width();
			if (result.has_max_width())
				result.get_max_width() = get_max_width();
		}
		else
		{
			result.get_min_width() = (get_min_width() > r.get_min_width()) ? r.get_min_width() : get_min_width();
			result.has_max_width() = (r.has_max_width() && has_max_width());
			if (result.has_max_width())
				result.get_max_width() = (get_max_width() < r.get_max_width()) ? r.has_max_width() : get_max_width();
		}

		if (is_height_empty())
		{
			result.get_min_height() = r.get_min_height();
			result.has_max_height() = r.has_max_height();
			if (result.has_max_height())
				result.get_max_height() = r.get_max_height();
		}
		else if (r.is_height_empty())
		{
			result.get_min_height() = get_min_height();
			result.has_max_height() = has_max_height();
			if (result.has_max_height())
				result.get_max_height() = get_max_height();
		}
		else
		{
			result.get_min_height() = (get_min_height() > r.get_min_height()) ? r.get_min_height() : get_min_height();
			result.has_max_height() = (r.has_max_height() && has_max_height());
			if (result.has_max_height())
				result.get_max_height() = (get_max_height() < r.get_max_height()) ? r.get_max_height() : get_max_height();
		}
		return result;
	}

	range& operator|=(const range& r)
	{
		if (r.is_width_empty())
		{
			if (is_width_empty())
			{
				get_min_width() = r.get_min_width();
				has_max_width() = r.has_max_width();
				if (has_max_width())
					get_max_width() = r.get_max_width();
			}
			else
			{
				if (get_min_width() > r.get_min_width())
					get_min_width() = r.get_min_width();

				has_max_width() &= r.has_max_width();
				if (has_max_width() && (get_max_width() < r.get_max_width()))
					get_max_width() = r.get_max_width();
			}
		}

		if (r.is_height_empty())
		{
			if (is_height_empty())
			{
				get_min_height() = r.get_min_height();
				has_max_height() = r.has_max_height();
				if (has_max_height())
					get_max_height() = r.get_max_height();
			}
			else
			{
				if (get_min_height() > r.get_min_height())
					get_min_height() = r.get_min_height();

				has_max_height() &= r.has_max_height();
				if (has_max_height() && (get_max_height() < r.get_max_height()))
					get_max_height() = r.get_max_height();
			}
		}
		return *this;
	}


	// range & range = range (intersection)
	range operator&(const range& r) const
	{
		range result;
		if (r.is_width_empty() || is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = (get_min_width() < r.get_min_width()) ? r.get_min_width() : get_min_width();
			if (r.has_max_width())
			{
				result.has_max_width() = true;
				result.get_max_width() = (!has_max_width() || (get_max_width() > r.get_max_width())) ? r.get_max_width() : get_max_width();
			}
			else
			{
				result.has_max_width() = has_max_width();
				if (has_max_width())
					result.get_max_width() = get_max_width();
			}
		}
		if (r.is_height_empty() || is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = (get_min_height() < r.get_min_height()) ? r.get_min_height() : get_min_height();
			if (r.has_max_height())
			{
				result.has_max_height() = true;
				result.get_max_height() = (!has_max_height() || (get_max_height() > r.get_max_height())) ? r.get_max_height() : get_max_height();
			}
			else
			{
				result.has_max_height() = has_max_height();
				if (has_max_height())
					result.get_max_height() = get_max_height();
			}
		}

		return result;
	}

	range& operator&=(const range& r)
	{
		if (r.is_width_empty())
			set_empty_width();
		else if (!is_width_empty())
		{
			if (get_min_width() < r.get_min_width())
				get_min_width() = r.get_min_width();

			if (r.has_max_width())
			{
				if (!has_max_width() || (get_max_width() > r.get_max_width()))
					get_max_width() = r.get_max_width();
				has_max_width() = true;
			}
			if (has_max_width() && (get_max_width() < get_min_width()))
				get_max_width() = get_min_width();
		}

		if (r.is_height_empty())
			set_empty_height();
		else if (!is_height_empty())
		{
			if (get_min_height() < r.get_min_height())
				get_min_height() = r.get_min_height();

			if (r.has_max_height())
			{
				if (!has_max_height() || (get_max_height() > r.get_max_height()))
					get_max_height() = r.get_max_height();
				has_max_height() = true;
			}
			if (has_max_height() && (get_max_height() < get_min_height()))
				get_max_height() = get_min_height();
		}

		return *this;
	}


	// range ^ range = range (overlap)
	range operator^(const range& r) const
	{
		range result;
		if (r.is_width_empty() || is_width_empty())
			result.set_empty_width();
		else
		{
			result.get_min_width() = (get_min_width() < r.get_min_width()) ? r.get_min_width() : get_min_width();
			result.has_max_width() = (r.has_max_width() && has_max_width());
			if (result.has_max_width())
				result.get_max_width() = (get_max_width() > r.get_max_width()) ? r.get_max_width() : get_max_width();
		}
		if (r.is_height_empty() || is_height_empty())
			result.set_empty_height();
		else
		{
			result.get_min_height() = (get_min_height() < r.get_min_height()) ? r.get_min_height() : get_min_height();
			result.has_max_height() = (r.has_max_height() && has_max_height());
			if (result.has_max_height())
				result.get_max_height() = (get_max_height() > r.get_max_height()) ? r.get_max_height() : get_max_height();
		}
		return result;
	}

	range& operator^=(const range& r)
	{
		if (!is_width_empty())
		{
			if (r.is_width_empty())
				set_empty_width();
			else
			{
				if (get_min_width() < r.get_min_width())
					get_min_width() = r.get_min_width();

				has_max_width() &= r.has_max_width();
				if (has_max_width() && (get_max_width() < r.get_max_width()))
					get_max_width() = r.get_max_width();
			}
		}
		if (!is_height_empty())
		{
			if (r.is_height_empty())
				set_empty_height();
			else
			{
				if (get_min_height() < r.get_min_height())
					get_min_height() = r.get_min_height();

				has_max_height() &= r.has_max_height();
				if (has_max_height() && (get_max_height() < r.get_max_height()))
					get_max_height() = r.get_max_height();
			}
		}

		return *this;
	}

	point limit(const point& pt) const;
	point limit_min(const point& pt) const;
	point limit_max(const point& pt) const;

	size limit(const size& sz) const
	{
		size rtn;
		rtn.get_width() = (has_max_width() && sz.get_width() > get_max_width()) ? get_max_width() : cogs::greater(sz.get_width(), get_min_width());
		rtn.get_height() = (has_max_height() && sz.get_height() > get_max_height()) ? get_max_height() : cogs::greater(sz.get_height(), get_min_height());
		return rtn;
	}

	double limit(dimension d, double d2) const
	{
		return (has_max(d) && d2 > get_max(d)) ? get_max(d) : cogs::greater(d2, get_max(d));
	}

	size limit_min(const size& sz) const
	{
		size rtn;
		rtn.get_width() = cogs::greater(sz.get_width(), get_min_width());
		rtn.get_height() = cogs::greater(sz.get_height(), get_min_height());
		return rtn;
	}

	size limit_max(const size& sz) const
	{
		size rtn;
		rtn.get_width() = (has_max_width()) ? cogs::lesser(sz.get_width(), get_max_width()) : sz.get_width();
		rtn.get_height() = (has_max_height()) ? cogs::lesser(sz.get_height(), get_max_height()) : sz.get_height();
		return rtn;
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
