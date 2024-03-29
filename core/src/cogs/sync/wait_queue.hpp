//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_WAIT_QUEUE
#define COGS_HEADER_SYNC_WAIT_QUEUE


#include "cogs/env.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/sync/semaphore.hpp"


namespace cogs {

template <typename T, bool coalesc_equal = true>
using wait_container_queue_node = container_queue_node<T, coalesc_equal>;


/// @ingroup LockFreeCollections
/// @brief A container_queue that can perform a blocking wait.
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: true
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = true, class allocator_t = batch_allocator<wait_container_queue_node<T, coalesc_equal>>>

class wait_container_queue
{
public:
	typedef T type;
	typedef allocator_t allocator_type;
	typedef wait_container_queue<type, coalesc_equal, allocator_type> this_t;

private:
	semaphore m_semaphore;
	container_queue<type, coalesc_equal, allocator_type> m_queue;

	wait_container_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	wait_container_queue()
	{ }

	void insert(const type& t, size_t n = 1) volatile
	{
		m_queue.append(t, n);
		m_semaphore.release(n);
	}

	bool get(type& t, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		if (m_semaphore.acquire(1, timeout, spinCount))
		{
			bool b = m_queue.pop(t);
			COGS_ASSERT(b);
			return true;
		}
		return false;
	}
};

template <typename T, class allocator_type>
class wait_container_queue<T, false, allocator_type>
{
public:
	typedef T type;
	typedef wait_container_queue<type, false, allocator_type> this_t;

private:
	semaphore m_semaphore;
	container_queue<type, false, allocator_type> m_queue;

	wait_container_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	wait_container_queue()
	{ }

	void insert(const type& t) volatile
	{
		m_queue.append(t);
		m_semaphore.release();
	}

	bool get(type& t, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		if (m_semaphore.acquire(1, timeout, spinCount))
		{
			bool b = m_queue.pop(t);
			COGS_ASSERT(b);
			return true;
		}
		return false;
	}
};


}


#endif
