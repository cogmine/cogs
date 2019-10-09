//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_DEFAULT_ALLOCATOR
#define COGS_HEADER_MEM_DEFAULT_ALLOCATOR


#include "cogs/env.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/mem/allocator.hpp"
#include "cogs/mem/bballoc.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"

namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


/// @ingroup Mem
/// @brief Provides access to the default memory management algorithm
///
/// The default allocator is assumed to only fail in catastrophic conditions.
class default_allocator
{
public:
	static constexpr bool is_static = true;

	typedef ptr<void>	ref_t;

private:
	inline static placement<ptr<allocator> > s_defaultAllocator;

	static allocator* create_default_allocator();
	static void dispose_default_allocator(allocator*);

public:
	static void shutdown();

	static allocator& get()
	{
		// The default allocator will outlive all allocations.
		// It's not shutdown until after all threads have exited, and main is about to return.
		// Note: We impose a rule that globals should not be used to store objects
		// that outlive the allocator.  Globals and static data should either be placed in placement
		// blocks and either be valid when zero-initialized or handled with an atomic lazy-initializer.
		// if cleanup is necessary, the cleanup API should be used.

		volatile ptr<allocator>& defaultAllocator = s_defaultAllocator.get();
		ptr<allocator> al = defaultAllocator;
		if (!al)
		{
			allocator* newAllocator = create_default_allocator();
			if (defaultAllocator.compare_exchange(newAllocator, al, al))
				al = newAllocator;
			else
				dispose_default_allocator(al.get_ptr());
		}
		return *al;
	}

	static ref_t allocate(size_t n, size_t align)										{ return get().allocate(n, align); }
	static void deallocate(const ref_t& p)												{ get().deallocate(p); }


	template <typename type>
	static type* allocate_type(size_t n = 1) { return allocate(sizeof(type) * n, std::alignment_of_v<type>).template reinterpret_cast_to<type>().get_ptr(); }


	template <typename type> static void deallocate(const ptr<type>& p) { get().deallocate(p.template reinterpret_cast_to<void>()); }

	template <typename type>
	static void destruct_deallocate_type(const ptr<type>& p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			get().deallocate(p);
		}
	}

	template <typename type> static bool try_reallocate(const ptr<type>& p, size_t n)
	{
		return get().try_reallocate(p.template reinterpret_cast_to<void>(), n);
	}

	template <typename type> static bool try_reallocate_type(const ptr<type>& p, size_t n)
	{
		return get().try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type));
	}

	template <typename type> static size_t get_allocation_size(const ptr<type>& p, size_t align, size_t knownSize)
	{
		return get().get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);
	}

	template <typename type> static size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize)
	{
		return get().get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of_v<type>, knownSize * sizeof(type)) / sizeof(type);
	}

	// helpers with raw pointers

	static void deallocate(void* p) { get().deallocate(p); }

	template <typename type> static void deallocate(type* p) { get().deallocate(ptr<type>(p).template reinterpret_cast_to<void>()); }

	template <typename type>
	static void destruct_deallocate_type(type* p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			get().deallocate(p);
		}
	}

	template <typename type> static bool try_reallocate(type* p, size_t n)
	{
		return get().try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n);
	}

	template <typename type> static bool try_reallocate_type(type* p, size_t n)
	{
		return get().try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n * sizeof(type));
	}

	template <typename type> static size_t get_allocation_size(type* p, size_t align, size_t knownSize)
	{
		return get().get_allocation_size(ptr<type>(p).template reinterpret_cast_to<void>(), align, knownSize);
	}

	template <typename type> static size_t get_allocation_size_type(type* p, size_t knownSize)
	{
		return get().get_allocation_size(ptr<type>(p).template reinterpret_cast_to<void>(), std::alignment_of_v<type>, knownSize * sizeof(type)) / sizeof(type);
	}

	template <typename header_t, size_t align>
	static header_t* allocate_with_header(size_t n)
	{
		static constexpr size_t commonAlignment = const_lcm_v<std::alignment_of_v<header_t>, align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return allocate(n + headerSize, commonAlignment).template reinterpret_cast_to<header_t>().get_ptr();
	}

	template <typename header_t, typename type>
	static header_t* allocate_type_with_header(size_t n = 1) { return allocate_with_header<header_t, std::alignment_of_v<type> >(n * sizeof(type)); }

	template <typename header_t, size_t align>
	static bool try_reallocate_with_header(const ptr<header_t>& p, size_t n)
	{
		static constexpr size_t commonAlignment = const_lcm_v<std::alignment_of_v<header_t>, align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return try_reallocate(p, n + headerSize);
	}

	template <typename header_t, typename type>
	static bool try_reallocate_type_with_header(const ptr<header_t>& p, size_t n) { return try_reallocate_with_header<header_t, std::alignment_of_v<type> >(p, n * sizeof(type)); }

	template <typename header_t, size_t align>
	static size_t get_allocation_size_without_header(const ptr<header_t>& p, size_t knownSize)
	{
		static constexpr size_t commonAlignment = const_lcm_v<std::alignment_of_v<header_t>, align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return get_allocation_size(p, commonAlignment, knownSize + headerSize) - headerSize;
	}

	template <typename header_t, typename type>
	static size_t get_allocation_size_type_without_header(const ptr<header_t>& p, size_t knownSize)
	{
		return get_allocation_size_without_header<header_t, std::alignment_of_v<type> >(p, knownSize * sizeof(type)) / sizeof(type);
	}

	template <typename header_t, size_t align>
	static void* get_block_from_header(const ptr<const header_t>& p)
	{
		static constexpr size_t commonAlignment = const_lcm_v<std::alignment_of_v<header_t>, align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return (void*)(((unsigned char*)p.get_ptr()) + headerSize);
	}

	template <typename header_t, typename type>
	static type* get_type_block_from_header(const ptr<const header_t>& p) { return (type*)get_block_from_header<header_t, std::alignment_of_v<type> >(p); }


	template <typename header_t, size_t align>
	static header_t* get_header_from_block(const ptr<void>& p)
	{
		static constexpr size_t commonAlignment = const_lcm_v<std::alignment_of_v<header_t>, align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return reinterpret_cast<header_t*>(((unsigned char*)p.get_ptr()) - headerSize);
	}

	template <typename header_t, typename type>
	static header_t* get_header_from_type_block(const ptr<type>& p) { return get_header_from_block<header_t, std::alignment_of_v<type> >(p); }
};


typedef buddy_block_allocator< sizeof(void*), 1024 * 1024 * 4 > default_allocator_t;



}


#pragma warning(pop)


#include "cogs/mem/rc_obj_base.hpp"



#endif

