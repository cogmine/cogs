//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_DIMENSION
#define COGS_HEADER_GEOMETRY_DIMENSION


namespace cogs {
namespace geometry {

namespace planar {



/// @ingroup Planar
/// @brief A planar dimensions
enum class dimension
{
	/// @brief The horizontal dimension
	horizontal = 0,

	/// @brief The vertical dimension
	vertical = 1
};


/// @ingroup Planar
/// @brief Inverts a planar dimension
inline dimension operator!(dimension d) { return (dimension)!(int)d; }


}


namespace spatial {

/// @ingroup Planar
/// @brief A planar dimensions
enum dimension
{
	x = 0,
	y = 1,
	z = 2,
};


}

}
}


#endif
