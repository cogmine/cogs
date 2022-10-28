//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_ALLOCATOR_BASE
#define COGS_HEADER_MEM_ALLOCATOR_BASE


#include "cogs/mem/memory_manager_base.hpp"


namespace cogs {


template <typename T, typename allocator_t, bool is_static = true>
class allocator_base
{
};


template <typename T, typename allocator_t>
class allocator_base<T, allocator_t, true>
{
public:
	static constexpr bool is_static = true;
	typedef T type;

	static void destruct_deallocate(type* p)
	{
		if (!!p)
		{
			placement_destruct(p);
			allocator_t::deallocate(p);
		}
	}
};


template <typename T, typename allocator_t>
class allocator_base<T, allocator_t, false>
{
public:
	static constexpr bool is_static = false;
	typedef T type;

	void destruct_deallocate(type* p)
	{
		if (!!p)
		{
			placement_destruct(p);
			static_cast<allocator_t*>(this)->deallocate(p);
		}
	}

	void destruct_deallocate(type* p) const
	{
		if (!!p)
		{
			placement_destruct(p);
			static_cast<const allocator_t*>(this)->deallocate(p);
		}
	}

	void destruct_deallocate(type* p) volatile
	{
		if (!!p)
		{
			placement_destruct(p);
			static_cast<volatile allocator_t*>(this)->deallocate(p);
		}
	}

	void destruct_deallocate(type* p) const volatile
	{
		if (!!p)
		{
			placement_destruct(p);
			static_cast<const volatile allocator_t*>(this)->deallocate(p);
		}
	}
};


}


#endif
