//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_FREELIST
#define COGS_HEADER_MEM_FREELIST


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/versioned_ptr.hpp"
#include "cogs/mem/default_allocator.hpp"


namespace cogs {


//template <typename T>
//class default_allocator;


template <typename T>
class freelist_node : public placement<T>
{
public:
	freelist_node<T>* m_next;
};


/// @ingroup Mem
/// @brief A lock-free free-list.
///
/// A freelist is a lock-free stack that avoids the ABA problem without use of hazards,
/// by ensuring the listspans of elements extend beyond the lifespan of the freelist.
///
/// A freelist grows but does not shrink.  Elements are retained until the free-list is destructed.
///
/// @tparam T Type allocated by the freelist
/// @tparam allocator_type Type of allocator to use to allocate from if the freelist is empty.  Default: default_allocator
/// @tparam preallocated_count Number of objects to prepopulate the freelist with.  Default: 0
template <typename T, size_t preallocated_count = 0, class allocator_t = default_allocator<freelist_node<T>>>
class freelist
{
public:
	typedef T type;
	typedef allocator_t allocator_type;

private:
	allocator_type m_allocator;

	typedef freelist_node<type> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;
	alignas(atomic::get_alignment_v<size_t>) size_t m_curPos;
	node_t m_preallocated[preallocated_count];

	freelist(freelist&&) = delete;
	freelist(const freelist&) = delete;
	freelist& operator=(freelist&&) = delete;
	freelist& operator=(const freelist&) = delete;

public:
	freelist()
		: m_curPos(0)
	{ }

	~freelist()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get()->~type();
			n->node_t::~node_t();
			if ((n < &(m_preallocated[0])) || (n >= &(m_preallocated[preallocated_count])))
				m_allocator.deallocate(n);
			n = next;
		}
	}

	type* get()
	{
		ptr<node_t> result;
		if (!!m_head)
		{
			result = m_head.get_ptr();
			m_head.unversioned_set(m_head->m_next);
		}
		else
		{
			if (m_curPos >= preallocated_count)
				result = m_allocator.allocate();
			else
			{
				result = &m_preallocated[m_curPos];
				++m_curPos;
			}

			placement_construct(&result->get());
		}

		return &result->get();
	}

	type* get() volatile
	{
		ptr<node_t> result;
		ptr<node_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		for (;;)
		{
			if (!!oldHead)
			{
				ptr<node_t> newHead = oldHead->m_next;
				if (!m_head.versioned_exchange(newHead, v, oldHead))
					continue;
				result = oldHead;
				break;
			}

			size_t oldPos = atomic::load(m_curPos);
			if (oldPos >= preallocated_count)
				result = m_allocator.allocate();
			else
			{
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
				{
					m_head.get(oldHead, v);
					continue;
				}
				result = const_cast<node_t*>(&m_preallocated[oldPos]);
			}

			placement_construct(&result->get());
			break;
		}

		return &result->get();
	}

	void release(type& t)
	{
		node_t* n = reinterpret_cast<node_t*>(&t);
		n->m_next = m_head.get_ptr();
		m_head = n;
	}

	void release(type& t) volatile
	{
		node_t* n = reinterpret_cast<node_t*>(&t);
		ptr<node_t> oldHead;
		version_t oldVersion;
		m_head.get(oldHead, oldVersion);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.versioned_exchange(n, oldVersion, oldHead));
	}
};


template <typename T, class allocator_t>
class freelist<T, 0, allocator_t>
{
public:
	typedef T type;
	typedef allocator_t allocator_type;

private:
	allocator_type m_allocator;

	typedef freelist_node<type> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;

	freelist(freelist&&) = delete;
	freelist(const freelist&) = delete;
	freelist& operator=(freelist&&) = delete;
	freelist& operator=(const freelist&) = delete;

public:
	freelist() { }

	~freelist()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get().~type();
			m_allocator.destruct_deallocate(n);
			n = next;
		}
	}

	type* get()
	{
		ptr<node_t> result;
		if (!!m_head)
		{
			result = m_head.get_ptr();
			m_head.unversioned_set(m_head->m_next);
		}
		else
		{
			result = m_allocator.allocate();
			placement_construct(&result->get());
		}

		return &result->get();
	}

	type* get() volatile
	{
		ptr<node_t> result;
		ptr<node_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		for (;;)
		{
			if (!!oldHead)
			{
				ptr<node_t> newHead = oldHead->m_next;
				if (!m_head.versioned_exchange(newHead, v, oldHead))
					continue;
				result = oldHead;
				break;
			}

			result = m_allocator.allocate();
			placement_construct(&result->get());
			break;
		}

		return &result->get();
	}

	void release(type& t)
	{
		node_t* n = reinterpret_cast<node_t*>(&t);
		n->m_next = m_head.get_ptr();
		m_head = n;
	}

	void release(type& t) volatile
	{
		node_t* n = reinterpret_cast<node_t*>(&t);
		ptr<node_t> oldHead;
		m_head.get(oldHead);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.compare_exchange(n, oldHead, oldHead));
	}
};


}


#endif
