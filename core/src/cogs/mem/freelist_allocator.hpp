//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_FREELIST_ALLOCATOR
#define COGS_HEADER_MEM_FREELIST_ALLOCATOR


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/storage_union.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


/// @ingroup Mem
/// @brief An allocator that uses a lock-free free-list
///
/// A freelist_allocator is an allocator based on freelist.  The list contains raw blocks.
/// Objects are constructed when retrieved, and destructed when restored to the list.
///
/// @tparam block_size The block size of allocations in the free-list
/// @tparam alignment The alignment of allocations in the free-list
/// @tparam allocator_type Type of allocator to use to allocate from if the freelist is empty.  Default: default_allocator
/// @tparam num_preallocated Number of objects to prepopulate the free-list with.  Default: 0
template <size_t block_size, size_t alignment, class allocator_type = default_allocator, size_t num_preallocated = 0>
class freelist_allocator 
{
private:
	allocator_container<allocator_type> m_allocator;

	typedef placement_storage<block_size, alignment> placement_t;

	typedef storage_union<placement_t, placement_t*> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	volatile versioned_ptr<node_t> m_head;
	alignas (atomic::get_alignment_v<size_t>) volatile size_t m_curPos;
	node_t m_preallocated[num_preallocated];

public:
	typedef ptr<void> ref_t;

	static constexpr bool is_static = true;

	freelist_allocator()
		: m_curPos(0)
	{ }

	freelist_allocator(volatile allocator_type& al)
		: m_allocator(al),
		m_curPos(0)
	{ }

	~freelist_allocator()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->get_second();
			if ((n < &(m_preallocated[0])) || (n >= &(m_preallocated[num_preallocated])))
				m_allocator.deallocate(n);
			n = next;
		}
	}

	virtual size_t get_allocation_size(const ref_t& p, size_t align, size_t knownSize) const volatile
	{
		return block_size;
	}

	virtual bool try_reallocate(const ref_t& p, size_t n) volatile
	{
		return n <= block_size;
	}

	virtual ref_t allocate(size_t n, size_t align) volatile
	{
		COGS_ASSERT(align <= alignment);

		ptr<node_t> result;
		size_t oldPos = atomic::load(m_curPos);
		for (;;)
		{
			if (oldPos < num_preallocated)
			{
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
					continue;
				result = const_cast<node_t*>(&(m_preallocated[oldPos]));
				break;
			}

			ptr<node_t> oldHead;
			version_t v;
			m_head.get(oldHead, v);
			for (;;)
			{
				if (!!oldHead)
				{
					ptr<node_t> newHead = (node_t*)(oldHead->get_second());
					if (!m_head.versioned_exchange(newHead, v, oldHead))
						continue;
					result = oldHead;
					break;
				}

				result = m_allocator.template allocate_type<node_t>();
				break;
			}
			break;
		}

		return result->get_first();
	}


	virtual void deallocate(const ref_t& p) volatile
	{
		node_t* n = p.template reinterpret_cast_to<node_t>();
		ptr<node_t> oldHead;
		version_t oldVersion;
		m_head.get(oldHead, oldVersion);
		do {
			n->get_second() = oldHead.get_ptr();
		} while (!m_head.versioned_exchange(n, oldVersion, oldHead));
	}
};


template <size_t block_size, size_t alignment, class allocator_type>
class freelist_allocator<block_size, alignment, allocator_type, 0> 
{
private:
	allocator_container<allocator_type> m_allocator;

	typedef placement_storage<block_size, alignment> placement_t;

	typedef storage_union<placement_t, placement_t*> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	volatile versioned_ptr<node_t> m_head;

public:
	typedef ptr<void> ref_t;

	static constexpr bool is_static = true;

	freelist_allocator()	{ }

	freelist_allocator(volatile allocator_type& al)
		: m_allocator(al)
	{ }

	~freelist_allocator()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->get_second();
			m_allocator.deallocate(n);
			n = next;
		}
	}

	virtual size_t get_allocation_size(const ref_t& p, size_t align, size_t knownSize) const volatile
	{
		return block_size;
	}

	virtual bool try_reallocate(const ref_t& p, size_t n) volatile
	{
		return n <= block_size;
	}

	virtual ref_t allocate(size_t n, size_t align) volatile
	{
		COGS_ASSERT(align <= alignment);

		ptr<node_t> result;
		ptr<node_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		for (;;)
		{
			if (!!oldHead)
			{
				ptr<node_t> newHead = (node_t*)(oldHead->get_second());
				if (!m_head.versioned_exchange(newHead, v, oldHead))
					continue;
				result = oldHead;
				break;
			}

			result = m_allocator.template allocate_type<node_t>();
			break;
		}

		return result->get_first();
	}

	virtual void deallocate(const ref_t& p) volatile
	{
		node_t* n = p.template reinterpret_cast_to<node_t>();
		ptr<node_t> oldHead;
		m_head.get(oldHead);
		do {
			n->get_second() = oldHead.get_ptr();
		} while (!m_head.compare_exchange(n, oldHead, oldHead));
	}
};


}


#endif
