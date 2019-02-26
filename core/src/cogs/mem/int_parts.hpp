//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_INT_PARTS
#define COGS_HEADER_MEM_INT_PARTS


#include <type_traits> 


namespace cogs {


/// @ingroup Mem
/// @brief Helper template to get the mask of high and low part of the specified integer type
/// @tparam int_t Int type
template <typename int_t>
class get_part_mask
{
public:
	static constexpr int_t high_part_mask = (int_t)((int_t)~(int_t)0 << sizeof(int_t)*4);
	static constexpr int_t low_part_mask  = (int_t)~high_part_mask;
};


/// @ingroup Mem
/// @brief Helper template to get the low part of a constant integer
/// @tparam int_t Int type
/// @tparam v Int value
template <typename int_t, int_t v>
class get_const_low_part
{
public:
	static constexpr int_t value  = v & get_part_mask<int_t>::low_part_mask;
};
template <typename int_t, int_t v>
inline constexpr int_t get_const_low_part_v = get_const_low_part<int_t, v>::value;

/// @ingroup Mem
/// @brief Helper template to get the high part of a constant integer
/// @tparam int_t Int type
/// @tparam v Int value
template <typename int_t, int_t v>
class get_const_high_part
{
private:
	typedef std::make_unsigned_t<int_t> uint_t;

public:
	static constexpr int_t value = ((uint_t)v >> (sizeof(int_t)*4)) & get_part_mask<int_t>::low_part_mask;
};
template <typename int_t, int_t v>
inline constexpr int_t get_const_high_part_v = get_const_high_part<int_t, v>::value;


/// @ingroup Mem
/// @brief Helper template to create the high part of a constant integer
/// @tparam int_t Int type
/// @tparam high_part High part value
template <typename int_t, int_t high_part>
class make_const_high_part
{
public:
	static constexpr int_t value = high_part << sizeof(int_t)*4;
};
template <typename int_t, int_t high_part>
inline constexpr int_t make_const_high_part_v = make_const_high_part<int_t, high_part>::value;

	
/// @ingroup Mem
/// @brief Helper template to create a constant integer from high and low parts
/// @tparam int_t Int type
/// @tparam high_part High part value
/// @tparam low_part Low part value
template <typename int_t, int_t high_part, int_t low_part>
class make_const_int_from_parts
{
public:
	static constexpr int_t value = (make_const_high_part_v<int_t, high_part>) | (get_const_low_part_v<int_t, low_part>);
};
template <typename int_t, int_t high_part, int_t low_part>
inline constexpr int_t make_const_int_from_parts_v = make_const_int_from_parts<int_t, high_part, low_part>::value;

/// @ingroup Mem
/// @brief Helper function to get the high part of an integer
/// @tparam int_t Int type
/// @param i Value to get high part from
/// @return High part of the value
template <typename int_t>
inline int_t get_high_part(const int_t& i)
{
	typedef std::make_unsigned_t<int_t> uint_t;
	return (int_t)((uint_t)i >> (sizeof(int_t)*4));
}


/// @ingroup Mem
/// @brief Helper function to get the low part of an integer
/// @tparam int_t Int type
/// @param i Value to get low part from
/// @return Low part of the value
template <typename int_t>
inline int_t get_low_part(const int_t& i)
{
	typedef std::make_unsigned_t<int_t> uint_t;
	return (int_t)((uint_t)i & (~(uint_t)0 >> sizeof(uint_t)*4));
}


/// @ingroup Mem
/// @brief Helper function to create an integer with the specified high part
/// @tparam int_t Int type
/// @param i Value to shift high
/// @return New value with original value shifted to the high part
template <typename int_t>
inline int_t make_high_part(const int_t& i)
{
	return i << sizeof(int_t)*4;
}

/// @ingroup Mem
/// @brief Helper function to create an integer from high and low parts
/// @tparam int_t Int type
/// @param highPart High part of new value
/// @param lowPart Low part of new value
/// @return New value
template <typename int_t>
inline int_t make_int_from_parts(const int_t& highPart, const int_t& lowPart)
{
	return make_high_part(highPart) | get_low_part(lowPart);
}


}


#endif

