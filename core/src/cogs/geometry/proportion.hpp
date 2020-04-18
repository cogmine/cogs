//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Obsolete


#ifndef COGS_HEADER_GEOMETRY_PROPORTION
#define COGS_HEADER_GEOMETRY_PROPORTION

#include <type_traits>
#include <array>

#include "cogs/geometry/dimension.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace geometry {
namespace planar {

class proportion
{
private:
	std::array<double, 2> m_contents;

public:
	proportion()
		: m_contents{ 1, 1 }
	{ }

	proportion(const proportion& src)
		: m_contents(src.m_contents)
	{ }

	proportion(proportion&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	proportion(double x, double y)
		: m_contents{ x, y }
	{
	}

	proportion& operator=(const proportion& src)
	{
		m_contents = src.m_contents;
		return *this;
	}

	proportion& operator=(proportion&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	void set(double x, double y)
	{
		m_contents[0] = x;
		m_contents[1] = y;
	}

	bool operator==(const proportion& p) const { return ((m_contents[0] == p.m_contents[0]) && (m_contents[1] == p.m_contents[1])); }
	bool operator!=(const proportion& p) const { return ((m_contents[0] != p.m_contents[0]) || (m_contents[1] != p.m_contents[1])); }

	bool operator<(const proportion& p) const
	{
		if (m_contents[0] == p.m_contents[0])
			return (m_contents[1] < p.m_contents[1]);
		return (m_contents[0] < p.m_contents[0]);
	}

	bool operator>=(const proportion& p) const { return !operator<(p); }

	bool operator>(const proportion& p) const { return (p < *this); }

	bool operator<=(const proportion& p) const { return !operator>(p); }

	bool operator!() const { return ((m_contents[0] == 0) && (m_contents[1] == 0)); }

	double& operator[](dimension d) { return m_contents[(int)d]; }
	double operator[](dimension d) const { return m_contents[(int)d]; }

	proportion without_increase()
	{
		proportion rtn(*this);
		rtn.remove_increase();
		return rtn;
	}

	proportion without_decrease()
	{
		proportion rtn(*this);
		rtn.remove_decrease();
		return rtn;
	}

	void remove_increase()
	{
		if (m_contents[0] > 1)
			m_contents[0] = 1;
		if (m_contents[1] > 1)
			m_contents[1] = 1;
	}

	void remove_decrease()
	{
		if (m_contents[0] < 1)
			m_contents[0] = 1;
		if (m_contents[1] < 1)
			m_contents[1] = 1;
	}

	static proportion none() { return proportion(0, 0); }
	static proportion half() { return proportion(0.5, 0.5); }
	static proportion all() { return proportion(1, 1); }
};

}


}
}


#endif
