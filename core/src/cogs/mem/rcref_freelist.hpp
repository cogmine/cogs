//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCREF_FREELIST
#define COGS_HEADER_MEM_RCREF_FREELIST


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


template <typename T>
class rcref_freelist_node : public placement<T>, public rc_obj_base
{
public:
	rcref_freelist_node<T>* m_next;
	volatile versioned_ptr<rcref_freelist_node<T>>* m_freelist;

	rcref_freelist_node()
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		set_type_name(typeid(T).name());
		set_obj_ptr(&placement<T>::get());
#endif
	}

	virtual void released()
	{
		// We don't destruct
	}

	virtual bool contains(void* obj) const
	{
		const T* start = &placement<T>::get();
		const unsigned char* p = (const unsigned char*)obj;
		return (p >= (const unsigned char*)start) && (p < (const unsigned char*)(start + 1));
	}

	virtual void dispose()
	{
		// Return to freelist
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		set_debug_str("rcref_freelist-free");
#endif
		ptr<rcref_freelist_node<T>> oldHead;
		typename versioned_ptr<rcref_freelist_node<T>>::version_t oldVersion;
		m_freelist->get(oldHead, oldVersion);
		do {
			m_next = oldHead.get_ptr();
		} while (!m_freelist->versioned_exchange(this, oldVersion, oldHead));

	}
};


/// @brief A freelist that returns rcref's.
///
/// As an rcref<> goes out of scope, the object is returned to the freelist.
/// A free-list grows but does not shrink.  Elements are retained until the free-list is destructed.
/// @tparam T Type to contain
/// @tparam allocator_type Type of allocator to use to allocate from if the free-list is empty.  Default: default_allocator
/// @tparam preallocated_count Number of objects to prepopulate the free-list with.  Default: 0
template <typename T, size_t preallocated_count = 0, class allocator_t = default_allocator<rcref_freelist_node<T>>>
class rcref_freelist
{
public:
	typedef T type;
	typedef allocator_t allocator_type;

private:
	typedef rcref_freelist<T, preallocated_count, allocator_type> this_t;

	allocator_type m_allocator;

	typedef rcref_freelist_node<T> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;
	node_t m_preallocated[preallocated_count];
	alignas(atomic::get_alignment_v<size_t>) size_t m_curPos;

public:
	rcref_freelist()
		: m_curPos(0)
	{
		// Preallocated means preconstructed
		for (size_t i = 0; i < preallocated_count; i++)
			new (&m_preallocated[i].get()) type;
	}

	~rcref_freelist()
	{
		size_t i = 0;
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get().type::~type();
			n->node_t::~node_t();
			if ((n < (node_t*)&(m_preallocated[0])) || (n >= (node_t*)&(m_preallocated[preallocated_count])))
				m_allocator.deallocate(n.get_ptr());
			else
				++i;
			n = next;
		}
		COGS_ASSERT(i == m_curPos);
		while (m_curPos < preallocated_count)
		{
			m_preallocated[m_curPos].get().type::~type();
			m_preallocated[m_curPos++].node_t::~node_t();
		}
	}

	rcref<type> get() volatile
	{
		ptr<node_t> desc;
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
				desc = oldHead;
				desc->reset_counts();
				break;
			}

			size_t oldPos = atomic::load(m_curPos);
			if (oldPos >= preallocated_count)
			{
				desc = m_allocator.allocate();
				new (desc) node_t;
			}
			else
			{
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
				{
					m_head.get(oldHead, v);
					continue;
				}
				desc = const_cast<node_t*>(&m_preallocated[oldPos]);
			}

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
			desc->set_debug_str("rcref_freelist");
#endif

			desc->m_freelist = &m_head;
			nested_rcnew(&desc->get(), *desc);
			break;
		}

		type* obj = &desc->get();
		rcref<type> result(obj, desc);
		return result;
	}
};


template <typename T, class allocator_type>
class rcref_freelist<T, 0, allocator_type>
{
private:
	typedef rcref_freelist<T, 0, allocator_type> this_t;

	allocator_type m_allocator;

	typedef rcref_freelist_node<T> node_t;

	typedef typename versioned_ptr<node_t>::version_t version_t;

	versioned_ptr<node_t> m_head;

public:
	typedef T type;

	rcref_freelist()
	{ }

	~rcref_freelist()
	{
		ptr<node_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<node_t> next = n->m_next;
			n->get().~type();
			m_allocator.destruct_deallocate(n.get_ptr());
			n = next;
		}
	}

	rcref<type> get() volatile
	{
		ptr<node_t> desc;
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
				desc = oldHead;
				desc->reset_counts();
				break;
			}

			desc = m_allocator.allocate();
			new (desc) node_t;

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
			desc->set_debug_str("rcref_freelist");
#endif

			desc->m_freelist = &m_head;
			nested_rcnew(&desc->get(), *desc);
			break;
		}

		type* obj = &desc->get();
		rcref<type> result(obj, desc);
		return result;
	}
};


}


#endif
