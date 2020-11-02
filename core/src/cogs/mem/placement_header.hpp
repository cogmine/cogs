//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	static constexpr size_t value = const_lcm_v<alignof(T1), alignof(T2) >;
};
template <class T1, class T2>
inline constexpr size_t get_common_alignment_v = get_common_alignment<T1, T2>::value;


/// @ingroup Mem
/// @brief Facilitates placement storage access to a header and a data type
///
/// @tparam header_t Header type
/// @tparam type_alignment Alignment of stored object
/// @tparam type_size Size of stored object
template <typename header_t, size_t type_alignment, size_t type_size>
class placement_storage_with_header
{
public:
	static constexpr size_t alignment = const_lcm_v<alignof(header_t), type_alignment>;
	static constexpr size_t distance = least_multiple_of_v<sizeof(header_t), alignment>;
	static constexpr size_t size = distance + least_multiple_of_v<type_size, alignment>;

private:
	placement_storage<size, alignment> m_storage;

public:
	static header_t* from_obj(void* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(const void* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(volatile void* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(const volatile void* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }

	static void* from_header(header_t* hdr) { return reinterpret_cast<void*>((unsigned char*)hdr + distance); }
	static void* from_header(const header_t* hdr) { return reinterpret_cast<void*>((unsigned char*)hdr + distance); }
	static void* from_header(volatile header_t* hdr) { return reinterpret_cast<void*>((unsigned char*)hdr + distance); }
	static void* from_header(const volatile header_t* hdr) { return reinterpret_cast<void*>((unsigned char*)hdr + distance); }

	header_t* get_header() { return &m_storage.template get<header_t>(); }
	header_t* get_header() const { return const_cast<header_t*>(&m_storage.template get<header_t>()); }
	header_t* get_header() volatile { return const_cast<header_t*>(&m_storage.template get<header_t>()); }
	header_t* get_header() const volatile { return const_cast<header_t*>(&m_storage.template get<header_t>()); }

	void* get_obj() { return from_header(&get_header()); }
	void* get_obj() const { return from_header(&get_header()); }
	void* get_obj() volatile { return from_header(&get_header()); }
	void* get_obj() const volatile { return from_header(&get_header()); }
};


/// @ingroup Mem
/// @brief Facilitates placement storage access to a header and a data type
///
/// @tparam header_t Header type
/// @tparam T Data type
/// @tparam n Count of objects of type T to store
template <typename header_t, typename T, size_t n = 1>
class placement_with_header
{
public:
	static_assert(n > 0);

	static constexpr size_t alignment = get_common_alignment_v<header_t, T>;
	static constexpr size_t distance = least_multiple_of_v<sizeof(header_t), alignment>;
	static constexpr size_t size = (least_multiple_of_v<sizeof(T), alignment> * n) + distance;

private:
	placement_storage<size, alignment> m_storage;

public:
	// For an allocation at runtime, storage does not need to be extended to an alignment boundary.
	static size_t get_runtime_size(size_t length) { return distance + (sizeof(T) * length); }

	static header_t* from_obj(T* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(const T* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(volatile T* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }
	static header_t* from_obj(const volatile T* t) { return reinterpret_cast<header_t*>((unsigned char*)t - distance); }

	static T* from_header(header_t* hdr) { return reinterpret_cast<T*>((unsigned char*)hdr + distance); }
	static T* from_header(const header_t* hdr) { return reinterpret_cast<T*>((unsigned char*)hdr + distance); }
	static T* from_header(volatile header_t* hdr) { return reinterpret_cast<T*>((unsigned char*)hdr + distance); }
	static T* from_header(const volatile header_t* hdr) { return reinterpret_cast<T*>((unsigned char*)hdr + distance); }

	header_t* get_header() { return &m_storage.template get<header_t>(); }
	header_t* get_header() const { return const_cast<header_t*>(&m_storage.template get<header_t>()); }
	header_t* get_header() volatile { return const_cast<header_t*>(&m_storage.template get<header_t>()); }
	header_t* get_header() const volatile { return const_cast<header_t*>(&m_storage.template get<header_t>()); }

	T* get_obj() { return from_header(&get_header()); }
	T* get_obj() const { return from_header(&get_header()); }
	T* get_obj() volatile { return from_header(&get_header()); }
	T* get_obj() const volatile { return from_header(&get_header()); }
};

}


#endif
