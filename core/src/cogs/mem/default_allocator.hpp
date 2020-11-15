//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_DEFAULT_ALLOCATOR
#define COGS_HEADER_MEM_DEFAULT_ALLOCATOR


#include "cogs/collections/no_aba_stack.hpp"
#include "cogs/env.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/mem/allocator_base.hpp"
#include "cogs/mem/bballoc.hpp"
#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


template <typename T, class memory_manager_t = default_memory_manager, bool is_static = memory_manager_t::is_static>
class default_allocator
{
};

template <typename T, class memory_manager_t>
class default_allocator<T, memory_manager_t, true> : public allocator_base<T, default_allocator<T, memory_manager_t, true>, true>
{
public:
	static T* allocate() { return memory_manager_t::template allocate_type<T>(); }
	static void deallocate(T* p) { return memory_manager_t::deallocate(p); }

	memory_manager_t get_memory_manager() const volatile { memory_manager_t result; return result; }
};

template <typename T, class memory_manager_t>
class default_allocator<T, memory_manager_t, false> : public allocator_base<T, default_allocator<T, memory_manager_t, false>, false>
{
private:
	memory_manager_t m_memoryManager;

public:
	T* allocate() { return m_memoryManager.template allocate_type<T>(); }
	T* allocate() const { return m_memoryManager.template allocate_type<T>(); }
	T* allocate() volatile { return m_memoryManager.template allocate_type<T>(); }
	T* allocate() const volatile { return m_memoryManager.template allocate_type<T>(); }

	void deallocate(T* p) { return m_memoryManager.deallocate(p); }
	void deallocate(T* p) const { return m_memoryManager.deallocate(p); }
	void deallocate(T* p) volatile { return m_memoryManager.deallocate(p); }
	void deallocate(T* p)const volatile { return m_memoryManager.deallocate(p); }

	memory_manager_t& get_memory_manager() const { return m_memoryManager; }
	volatile memory_manager_t& get_memory_manager() const volatile { return m_memoryManager; }
};


}


#endif
