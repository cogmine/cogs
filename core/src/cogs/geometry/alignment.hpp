//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress


#ifndef COGS_HEADER_GEOMETRY_ALIGNMENT
#define COGS_HEADER_GEOMETRY_ALIGNMENT

#include <array>

#include "cogs/geometry/dimension.hpp"


namespace cogs {

/// @defgroup Geometry
/// @{
/// @ingroup Math
/// @brief Geometry
/// @}


/// @ingroup Geometry
/// @brief Namespace for Geometry
namespace geometry {

/// @ingroup Linear
/// @brief Namespace for Linear Geometry
namespace linear {

}

namespace planar {


class alignment
{
private:
	std::array<double, 2> m_contents;

public:
	alignment()
		: m_contents{ 0.0, 0.0 }
	{ }

	alignment(const alignment& src)
		: m_contents(src.m_contents)
	{ }

	alignment(alignment&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	alignment(double x, double y)
		: m_contents{ x, y }
	{
	}

	alignment& operator=(const alignment& src)
	{
		m_contents = src.m_contents;
		return *this;
	}

	alignment& operator=(alignment&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	void set(double x, double y)
	{
		m_contents[0] = x;
		m_contents[1] = y;
	}

	bool operator==(const alignment& p) const { return ((m_contents[0] == p.m_contents[0]) && (m_contents[1] == p.m_contents[1])); }
	bool operator!=(const alignment& p) const { return ((m_contents[0] != p.m_contents[0]) || (m_contents[1] != p.m_contents[1])); }

	bool operator<(const alignment& p) const
	{
		if (m_contents[0] == p.m_contents[0])
			return (m_contents[1] < p.m_contents[1]);
		return (m_contents[0] < p.m_contents[0]);
	}

	bool operator>=(const alignment& p) const { return !operator<(p); }

	bool operator>(const alignment& p) const { return (p < *this); }

	bool operator<=(const alignment& p) const { return !operator>(p); }

	double& operator[](dimension d) { return m_contents[(int)d]; }
	double operator[](dimension d) const { return m_contents[(int)d]; }


	static alignment top_left() { return alignment(0, 0); }
	static alignment top_center() { return alignment(0.5, 0); }
	static alignment top_right() { return alignment(1, 0); }
	static alignment center_left() { return alignment(1, 0.5); }
	static alignment center() { return alignment(0.5, 0.5); }
	static alignment center_right() { return alignment(0, 0.5); }
	static alignment bottom_left() { return alignment(0, 1); }
	static alignment bottom_center() { return alignment(0.5, 1); }
	static alignment bottom_right() { return alignment(1, 1); }
};

}

}
}

#endif
