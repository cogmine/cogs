//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_FLOW
#define COGS_HEADER_GEOMETRY_FLOW


#include "cogs/geometry/dimension.hpp"


namespace cogs {
namespace geometry {

namespace planar {


enum class flow {
	x_ascending_y_ascending = 0x06,   // 110
	y_ascending_x_ascending = 0x07,   // 111

	x_ascending_y_descending = 0x02,  // 010
	y_ascending_x_descending = 0x05,  // 101

	x_descending_y_ascending = 0x04,  // 100
	y_descending_x_ascending = 0x03,  // 011

	x_descending_y_descending = 0x00, // 000
	y_descending_x_descending = 0x01, // 001

	dimension_mask = 0x01,            // 001

	x_ascending_mask = 0x02,          // 010
	y_ascending_mask = 0x04,          // 100
};


/// @ingroup Planar
/// @brief Display direction and behavior of wrapping elements, such as text
enum class script_flow
{
	/// @brief Script flows from left to right, top to bottom
	left_to_right_top_to_bottom = 0x06, // 110
	/// @brief Script flows from left to right, bottom to top
	left_to_right_bottom_to_top = 0x02, // 010
	/// @brief Script flows from right to left, top to bottom
	right_to_left_top_to_bottom = 0x04, // 100
	/// @brief Script flows from right to left, bottom to top
	right_to_left_bottom_to_top = 0x00, // 000

	/// @brief Script flows from top to bottom, left to right
	top_to_bottom_left_to_right = 0x07, // 111
	/// @brief Script flows from top to bottom, right to left
	top_to_bottom_right_to_left = 0x05, // 101
	/// @brief Script flows from bottom to top, left to right
	bottom_to_top_left_to_right = 0x03, // 011
	/// @brief Script flows bottom to top, right to left
	bottom_to_top_right_to_left = 0x01, // 001

	/// @brief Mask of script_flow that results in a planar::dimension value indicating the primary flow direction
	primary_dimension_mask = 0x01,      // 001
	/// @brief Mask of scipr_flow that results in a non-zero value if the value is ascending horizontally, or false otherwise.
	horizontal_ascending_mask = 0x02,   // 010
	/// @brief Mask of scipr_flow that results in a non-zero value if the value is ascending vertically, or false otherwise.
	vertical_ascending_mask = 0x04,     // 100
};

/// @ingroup Planar
/// @brief Gets the primary flow dimensions from a script_flow
inline dimension get_primary_flow_dimension(const script_flow& f) { return (dimension)((int)f & (int)script_flow::primary_dimension_mask); }

}
namespace spatial {


enum class flow
{
	// masks
	x_ascending_mask = 0x01, // 001
	y_ascending_mask = 0x02, // 010
	z_ascending_mask = 0x04, // 100

	x_primary_mask = 0x0008, // 000 000 001 000
	y_primary_mask = 0x0010, // 000 000 010 000
	z_primary_mask = 0x0020, // 000 000 100 000

	x_secondary_mask = 0x0040, // 000 001 000 000
	y_secondary_mask = 0x0080, // 000 010 000 000
	z_secondary_mask = 0x0100, // 000 100 000 000

	x_tertiary_mask = 0x0200,  // 001 000 000 000
	y_tertiary_mask = 0x0400,  // 010 000 000 000
	z_tertiary_mask = 0x0800,  // 100 000 000 000

	ascending_mask = x_ascending_mask | y_ascending_mask | z_ascending_mask, // 000 000 000 111

	dimension_order_mask =
		x_primary_mask | x_secondary_mask | x_tertiary_mask |
		y_primary_mask | y_secondary_mask | y_tertiary_mask |
		z_primary_mask | z_secondary_mask | z_tertiary_mask, // 111 111 111 000

	// order constants
	dimension_order_x_y_z = x_primary_mask | y_secondary_mask | z_tertiary_mask, // 100 010 001
	dimension_order_y_x_z = y_primary_mask | x_secondary_mask | z_tertiary_mask, // 100 001 010
	dimension_order_y_z_x = y_primary_mask | z_secondary_mask | x_tertiary_mask, // 001 100 010
	dimension_order_x_z_y = x_primary_mask | z_secondary_mask | y_tertiary_mask, // 010 100 001
	dimension_order_z_x_y = z_primary_mask | x_secondary_mask | y_tertiary_mask, // 010 001 100
	dimension_order_z_y_x = z_primary_mask | y_secondary_mask | x_tertiary_mask, // 001 010 100

	// values
	x_ascending_y_ascending_z_ascending = dimension_order_x_y_z | x_ascending_mask | y_ascending_mask | z_ascending_mask,
	y_ascending_x_ascending_z_ascending = dimension_order_y_x_z | x_ascending_mask | y_ascending_mask | z_ascending_mask,
	y_ascending_z_ascending_x_ascending = dimension_order_y_z_x | x_ascending_mask | y_ascending_mask | z_ascending_mask,
	x_ascending_z_ascending_y_ascending = dimension_order_x_z_y | x_ascending_mask | y_ascending_mask | z_ascending_mask,
	z_ascending_x_ascending_y_ascending = dimension_order_z_x_y | x_ascending_mask | y_ascending_mask | z_ascending_mask,
	z_ascending_y_ascending_x_ascending = dimension_order_z_y_x | x_ascending_mask | y_ascending_mask | z_ascending_mask,

	x_ascending_y_ascending_z_descending = dimension_order_x_y_z | x_ascending_mask | y_ascending_mask,
	y_ascending_x_ascending_z_descending = dimension_order_y_x_z | x_ascending_mask | y_ascending_mask,
	y_ascending_z_ascending_x_descending = dimension_order_y_z_x | y_ascending_mask | z_ascending_mask,
	x_ascending_z_ascending_y_descending = dimension_order_x_z_y | x_ascending_mask | z_ascending_mask,
	z_ascending_x_ascending_y_descending = dimension_order_z_x_y | x_ascending_mask | z_ascending_mask,
	z_ascending_y_ascending_x_descending = dimension_order_z_y_x | y_ascending_mask | z_ascending_mask,

	x_ascending_y_descending_z_ascending = dimension_order_x_y_z | x_ascending_mask | z_ascending_mask,
	y_ascending_x_descending_z_ascending = dimension_order_y_x_z | y_ascending_mask | z_ascending_mask,
	y_ascending_z_descending_x_ascending = dimension_order_y_z_x | x_ascending_mask | y_ascending_mask,
	x_ascending_z_descending_y_ascending = dimension_order_x_z_y | x_ascending_mask | y_ascending_mask,
	z_ascending_x_descending_y_ascending = dimension_order_z_x_y | y_ascending_mask | z_ascending_mask,
	z_ascending_y_descending_x_ascending = dimension_order_z_y_x | x_ascending_mask | z_ascending_mask,

	x_descending_y_ascending_z_ascending = dimension_order_x_y_z | y_ascending_mask | z_ascending_mask,
	y_descending_x_ascending_z_ascending = dimension_order_y_x_z | x_ascending_mask | z_ascending_mask,
	y_descending_z_ascending_x_ascending = dimension_order_y_z_x | x_ascending_mask | z_ascending_mask,
	x_descending_z_ascending_y_ascending = dimension_order_x_z_y | y_ascending_mask | z_ascending_mask,
	z_descending_x_ascending_y_ascending = dimension_order_z_x_y | x_ascending_mask | y_ascending_mask,
	z_descending_y_ascending_x_ascending = dimension_order_z_y_x | x_ascending_mask | y_ascending_mask,

	x_ascending_y_descending_z_descending = dimension_order_x_y_z | x_ascending_mask,
	y_ascending_x_descending_z_descending = dimension_order_y_x_z | y_ascending_mask,
	y_ascending_z_descending_x_descending = dimension_order_y_z_x | y_ascending_mask,
	x_ascending_z_descending_y_descending = dimension_order_x_z_y | x_ascending_mask,
	z_ascending_x_descending_y_descending = dimension_order_z_x_y | z_ascending_mask,
	z_ascending_y_descending_x_descending = dimension_order_z_y_x | z_ascending_mask,

	x_descending_y_ascending_z_descending = dimension_order_x_y_z | y_ascending_mask,
	y_descending_x_ascending_z_descending = dimension_order_y_x_z | x_ascending_mask,
	y_descending_z_ascending_x_descending = dimension_order_y_z_x | z_ascending_mask,
	x_descending_z_ascending_y_descending = dimension_order_x_z_y | z_ascending_mask,
	z_descending_x_ascending_y_descending = dimension_order_z_x_y | x_ascending_mask,
	z_descending_y_ascending_x_descending = dimension_order_z_y_x | y_ascending_mask,

	x_descending_y_descending_z_ascending = dimension_order_x_y_z | z_ascending_mask,
	y_descending_x_descending_z_ascending = dimension_order_y_x_z | z_ascending_mask,
	y_descending_z_descending_x_ascending = dimension_order_y_z_x | x_ascending_mask,
	x_descending_z_descending_y_ascending = dimension_order_x_z_y | y_ascending_mask,
	z_descending_x_descending_y_ascending = dimension_order_z_x_y | y_ascending_mask,
	z_descending_y_descending_x_ascending = dimension_order_z_y_x | x_ascending_mask,

	x_descending_y_descending_z_descending = dimension_order_x_y_z,
	y_descending_x_descending_z_descending = dimension_order_y_x_z,
	y_descending_z_descending_x_descending = dimension_order_y_z_x,
	x_descending_z_descending_y_descending = dimension_order_x_z_y,
	z_descending_x_descending_y_descending = dimension_order_z_x_y,
	z_descending_y_descending_x_descending = dimension_order_z_y_x,
};

}

}
}

#endif
