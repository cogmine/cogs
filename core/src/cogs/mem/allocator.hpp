//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_ALLOCATOR
#define COGS_HEADER_MEM_ALLOCATOR


#include "cogs/env.hpp"
#include "cogs/mem/ptr.hpp"


/// @brief Root namespace for cogs library
namespace cogs {


/// @defgroup Mem Memory Management
/// @{
/// @brief Memory Management algorithms
/// @}



class default_allocator;



/// @brief A base class for an allocator (or example layout for a static allocator)
/// 
/// @tparam ref_type The type of reference used to refer to an allocation.  Default: ptr
template <template <typename> class ref_type = ptr>
class allocator_t
{
public:
	/// @brief The reference type returned by this allocator
	typedef ref_type<void> ref_t;

	/// @brief True if this allocator type is static.  False if instance-based.
	static constexpr bool is_static = false;

	/// @brief Allocate data
	/// @param n Size of allocation in bytes
	/// @param align Alignment of allocation
	/// @return A reference to the allocation
	virtual ref_t allocate(size_t n, size_t align) volatile = 0;

	/// @brief Deallocate data
	/// @param p Reference to data to deallocate
	virtual void deallocate(const ref_t& p) volatile = 0;

	/// @brief Attempts to reallocate a block of data in place
	/// @param p A reference to the data to reallocate
	/// @param n The requested new size of the allocation
	/// @return True if the same block can be used
	virtual bool try_reallocate(const ref_t& p, size_t n) volatile = 0;

	/// @brief Gets the size of an allocation
	/// Allocators may vary in support for get_allocation_size().
	/// If the caller needs to manage the size of an allocation, it should store the requested
	/// size (knownSize).  get_allocation_size() may be used to determine if more than the requested
	/// amount was allocated.  Allocators that do not support retrieval of a block's size may
	/// return knownSize from get_allocation_size().
	/// @param p A reference to the data to get the size of
	/// @param align Alignment of the allocation.  (Really only required because _aligned_msize() requires it for some reason).
	/// @param knownSize A fallback value to use if the allocator cannot determine the size of the block.
	virtual size_t get_allocation_size(const ref_t& p, size_t align, size_t knownSize) const volatile { return knownSize; }
};



template <>
class allocator_t<ptr>
{
public:
	typedef ptr<void> ref_t;

	static constexpr bool is_static = false;

	virtual ref_t allocate(size_t n, size_t align) volatile = 0;
	virtual void deallocate(const ref_t& p) volatile = 0;
	virtual bool try_reallocate(const ref_t& p, size_t n) volatile = 0;	// returns true if same block can be used.
	virtual size_t get_allocation_size(const ref_t& p, size_t align, size_t knownSize) const volatile { return knownSize; }
};


/// @brief An alias to allocator_t<ptr>
typedef allocator_t<> allocator;


}


#pragma warning(push)
#pragma warning (disable: 4290)


// placement operator new/delete for allocator

inline void* operator new(size_t n, cogs::allocator& al)
{
	cogs::ptr<void> p = al.allocate(n, cogs::largest_alignment);
	return p.get_ptr();
}

inline void operator delete(void* p, cogs::allocator& al)
{
	if (!!p)
		al.deallocate(p);
}




#pragma warning(pop)


#endif

