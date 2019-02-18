//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_PLACEMENT_HEADER
#define COGS_HEADER_MEM_PLACEMENT_HEADER


#include "cogs/env.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {

// Header and payload need the same (least common multiple) alignment.
// Header needs to be padded to the alignment needed for the payload, to ensure the payload starts at the correct alignment.
// Payload needs to be padded to the alignment needed for header, in case in an array.

/// @ingroup Mem
/// @brief Gets the common memory alignment of two types
/// @tparam T1 Type to inspect
/// @tparam T2 Type to inspect
template <typename T1, typename T2>
class get_common_alignment
{
public:
	static constexpr size_t value = const_lcm_v<std::alignment_of_v<T1>, std::alignment_of_v<T2> >;
};
template <class T1, class T2>
inline constexpr size_t get_common_alignment_v = get_common_alignment<T1, T2>::value;


/// @ingroup Mem
/// @brief Gets the size of a block containing a header and a data type 
///
/// Ensures alignment of both types are satisfied in the new block.
/// @tparam header_t Header type
/// @tparam T Data type
template <typename header_t, typename T>
class get_type_and_header_size
{
public:
	static constexpr size_t value = least_multiple_of_v<sizeof(header_t), get_common_alignment_v<header_t, T> > +
		least_multiple_of_v<sizeof(T), get_common_alignment_v<header_t, T> >;
};

/// @ingroup Mem
/// @brief A helper class that facilitates placement storage access to a header and a data type 
///
/// @tparam header_t Header type
/// @tparam T Data type
template <typename header_t, typename T>
class placement_type_header_storage : public placement_storage<get_type_and_header_size<header_t, T>::value, get_common_alignment_v<header_t, T> >
{
public:
	header_t& get_header() { return get<header_t>(); }
	const header_t& get_header() const { return get<header_t>(); }
	volatile header_t& get_header() volatile { return get<header_t>(); }
	const volatile header_t& get_header() const volatile { return get<header_t>(); }
};



template <typename header_t, typename T>
inline T* get_type_block_from_header(const ptr<const header_t>& p)
{
	return (T*)(((unsigned char*)p.get_ptr()) + least_multiple_of_v<sizeof(header_t), get_common_alignment_v<header_t, T> >);
}


template <typename header_t, typename T>
inline header_t* get_header_from_type_block(const ptr<T>& p)
{
	return reinterpret_cast<header_t*>(((unsigned char*)p.get_ptr()) - least_multiple_of_v<sizeof(header_t), get_common_alignment_v<header_t, T> >);
}




}


#endif

