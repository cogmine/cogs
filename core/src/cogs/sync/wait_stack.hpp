//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_WAIT_STACK
#define COGS_HEADER_SYNC_WAIT_STACK


#include "cogs/env.hpp"
#include "cogs/collections/container_stack.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/sync/semaphore.hpp"


namespace cogs {

/// @ingroup LockFreeCollections
/// @brief A container_stack that can perform a blocking wait.
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: true
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = true, class allocator_type = default_allocator>
class wait_container_stack
{
public:
	typedef T type;
	typedef wait_container_stack<type, coalesc_equal, allocator_type> this_t;

private:
	semaphore				m_semaphore;
	container_stack<type>	m_stack;

	wait_container_stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	wait_container_stack()
	{ }
	
	explicit wait_container_stack(volatile allocator_type& al)
		:	m_stack(al)
	{ }

	void insert(const type& t, unsigned int n = 1) volatile
	{
		m_stack.append(t, n);
		m_semaphore.release();
	}

	bool get(type& t, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		if (m_semaphore.acquire(1, timeout, spinCount))
		{
			bool b = m_stack.remove(t);
			COGS_ASSERT(b);
			return true;
		}
		return false;
	}
};


template <typename T, class allocator_type>
class wait_container_stack<T, false, allocator_type>
{
public:
	typedef T type;
	typedef wait_container_stack<type, false, allocator_type> this_t;

private:
	semaphore												m_semaphore;
	container_stack<type, false, allocator_type>	m_stack;

	wait_container_stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	wait_container_stack()
	{ }

	explicit wait_container_stack(volatile allocator_type& al)
		: m_stack(al)
	{ }

	void insert(const type& t) volatile
	{
		m_stack.append(t);
		m_semaphore.release();
	}

	bool get(type& t, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		if (m_semaphore.acquire(1, timeout, spinCount))
		{
			bool b = m_stack.pop(t);
			COGS_ASSERT(b);
			return true;
		}
		return false;
	}
};

}


#endif








