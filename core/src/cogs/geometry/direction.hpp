//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_DIRECTION
#define COGS_HEADER_GEOMETRY_DIRECTION


namespace cogs {
namespace geometry {


template <size_t n>
class direction;

namespace linear {

/// @ingroup Linear
/// @brief A linear direction
enum class direction
{
	/// @brief Descending direction
	descending = -1,

	/// @brief Stationary / No direction
	stationary = 0,

	/// @brief Descending direction
	ascending = 1
};

/// @ingroup Planar
/// @brief negates a linear dimension
constexpr direction operator-(direction d)
{
	return (direction)(-(int)d);
}

}

namespace planar {


/// @ingroup Planar
/// @brief A planar direction
enum class direction // since we're using top/bottom/left/right for bounds, we use same spatial dimension for planar constants.
{
	/// @brief Up direction
	up = 0x01,              // 0001
	/// @brief Down direction
	down = 0x02,            // 0010
	/// @brief Stationary direction
	stationary = 0x00,      // 0000
	/// @brief Left direction
	left = 0x04,            // 0100
	/// @brief Right direction
	right = 0x08,           // 1000

	/// @brief Left-up direction
	left_up = 0x05,         // 0101
	/// @brief Left-down direction
	left_down = 0x06,       // 0110
	/// @brief Right-up direction
	right_up = 0x09,        // 1001
	/// @brief Right-down direction
	right_down = 0x0A,      // 1010

	/// @brief Vertical direction mask
	vertical_mask = 0x03,   // 0011
	/// @brief Horizontal direction mask
	horizontal_mask = 0x0C, // 1100
};

/// @ingroup Planar
/// @brief negates a planar dimension
constexpr direction operator-(direction d)
{
	return (direction)(
		(((int)d ^ (int)direction::vertical_mask) & (int)direction::vertical_mask)
		| (((int)d ^ (int)direction::horizontal_mask) & (int)direction::horizontal_mask));
}

}

namespace spatial {


enum class direction
{
	stationary = 0x00,          // 000000
	up = 0x01,                  // 000001
	down = 0x02,                // 000010
	left = 0x04,                // 000100
	right = 0x08,               // 001000

	left_up = 0x05,             // 000101
	left_down = 0x06,           // 000110
	right_up = 0x09,            // 001001
	right_down = 0x0A,          // 001010

	forward = 0x10,             // 010000
	up_forward = 0x11,          // 010001
	down_forward = 0x12,        // 010010
	left_forward = 0x14,        // 010100
	right_forward = 0x18,       // 011000

	left_up_forward = 0x15,     // 010101
	left_down_forward = 0x16,   // 010110
	right_up_forward = 0x19,    // 011001
	right_down_forward = 0x1A,  // 011010

	backward = 0x20,            // 100000
	up_backward = 0x21,         // 100001
	down_backward = 0x22,       // 100010
	left_backward = 0x24,       // 100100
	right_backward = 0x28,      // 101000

	left_up_backward = 0x25,    // 100101
	left_down_backward = 0x26,  // 100110
	right_up_backward = 0x29,   // 101001
	right_down_backward = 0x2A, // 101010

	vertical_mask = 0x03,       // 000011
	horizontal_mask = 0x0C,     // 001100
	depth_mask = 0x30,          // 110000
};

constexpr direction operator-(direction d)
{
	return (direction)(
		(((int)d ^ (int)direction::vertical_mask) & (int)direction::vertical_mask)
		| (((int)d ^ (int)direction::horizontal_mask) & (int)direction::horizontal_mask)
		| (((int)d ^ (int)direction::depth_mask) & (int)direction::depth_mask));
}

}


}
}


#endif
