//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_FREELIST
#define COGS_HEADER_MEM_FREELIST


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {

class default_allocator;


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
template <typename T, class allocator_type = default_allocator, size_t preallocated_count = 0>
class freelist
{
public:
	typedef T type;

private:
	allocator_container<allocator_type> m_allocator;

	class node_t
	{
	public:
		node_t* m_next;
		type* get_contents() const { return placement_with_header<node_t, type>::get_obj_from_header(this); }
		static node_t* from_obj(type& t) { return placement_with_header<node_t, type>::get_header_from_obj(&t); }
	};

	typedef placement_with_header<node_t, type> node_placement_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;
	alignas (atomic::get_alignment_v<size_t>) size_t m_curPos;
	node_placement_t m_preallocated[preallocated_count];

public:
	freelist()
		: m_curPos(0)
	{ }

	explicit freelist(volatile allocator_type& al)
		: m_allocator(al),
		m_curPos(0)
	{ }

	~freelist()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get_contents()->~type();
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
				result = m_allocator.template allocate_type<node_placement_t>()->get_header();
			else
			{
				result = &(m_preallocated[m_curPos].get_type());
				++m_curPos;
			}

			placement_construct(result->get_contents());
		}

		return result->get_contents();
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
				result = m_allocator.template allocate_type<node_placement_t>()->get_header();
			else
			{
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
				{
					m_head.get(oldHead, v);
					continue;
				}
				result = m_preallocated[oldPos].get_header();
			}

			placement_construct(result->get_contents());
			break;
		}

		return result->get_contents();
	}

	void release(type& t)
	{
		node_t* n = node_t::from_obj(t);
		n->m_next = m_head.get_ptr();
		m_head = n;
	}

	void release(type& t) volatile
	{
		node_t* n = node_t::from_obj(t);

		ptr<node_t> oldHead;
		version_t oldVersion;
		m_head.get(oldHead, oldVersion);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.versioned_exchange(n, oldVersion, oldHead));
	}

};

template <typename type, class allocator_type>
class freelist<type, allocator_type, 0>
{
private:
	allocator_container<allocator_type> m_allocator;

	class node_t
	{
	public:
		node_t* m_next;

		type* get_contents() const { return placement_with_header<node_t, type>::get_obj_from_header(this); }
		static node_t* from_obj(type& t) { return placement_with_header<node_t, type>::get_header_from_obj(&t); }
	};

	typedef placement_with_header<node_t, type> node_placement_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;

public:
	freelist() { }

	explicit freelist(volatile allocator_type& al)
		: m_allocator(al)
	{ }

	~freelist()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get_contents()->~type();
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
			result = m_allocator.template allocate_type<node_placement_t>()->get_header();
			placement_construct(result->get_contents());
		}

		return result->get_contents();
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

			result = m_allocator.template allocate_type<node_placement_t>()->get_header();
			placement_construct(result->get_contents());
			break;
		}

		return result->get_contents();
	}

	void release(type& t)
	{
		node_t* n = node_t::from_obj(t);
		n->m_next = m_head.get_ptr();
		m_head = n;
	}

	void release(type& t) volatile
	{
		node_t* n = node_t::from_obj(t);
		ptr<node_t> oldHead;
		m_head.get(oldHead);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.compare_exchange(n, oldHead, oldHead));
	}
};


}


#endif
