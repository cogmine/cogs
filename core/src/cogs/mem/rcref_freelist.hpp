//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCREF_FREELIST
#define COGS_HEADER_MEM_RCREF_FREELIST


#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


/// @brief A freelist that returns rcref's.
/// 
/// As an rcref<> goes out of scope, the object is returned to the freelist.
/// A free-list grows but does not shrink.  Elements are retained until the free-list is destructed.
/// @tparam T Type to contain
/// @tparam allocator_type Type of allocator to use to allocate from if the free-list is empty.  Default: default_allocator
/// @tparam num_preallocated Number of objects to prepopulate the free-list with.  Default: 0
template <typename T, class allocator_type = default_allocator, size_t num_preallocated = 0>
class rcref_freelist
{
private:
	typedef rcref_freelist<T, allocator_type, num_preallocated> this_t;

	allocator_container<allocator_type> m_allocator;

	class descriptor_t : public rc_obj_base
	{
	public:
		volatile this_t* m_freelist;
		descriptor_t* m_next;

		virtual void released()
		{
			// We don't destruct
		}

		virtual void dispose()
		{
			// Return to freelist
			m_freelist->release(this);
		}

		T* get_obj() const { return placement_with_header<descriptor_t, T>::get_obj_from_header(this); }

		static descriptor_t* from_obj(T* obj) { return placement_with_header<descriptor_t, T>::get_header_from_obj(obj); }
	};

	typedef placement_with_header<descriptor_t, T>  placement_t;

	typedef typename versioned_ptr<descriptor_t>::version_t version_t;

	volatile versioned_ptr<descriptor_t> m_head;
	placement_t m_preallocated[num_preallocated];
	alignas (atomic::get_alignment_v<size_t>) volatile size_t m_curPos;

	void release(descriptor_t* n) volatile
	{
		ptr<descriptor_t> oldHead;
		version_t oldVersion;
		m_head.get(oldHead, oldVersion);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.versioned_exchange(n, oldVersion, oldHead));
	}

public:
	typedef T type;

	rcref_freelist()
		: m_curPos(0)
	{ }

	rcref_freelist(volatile allocator_type& al)
		: m_allocator(al),
		m_curPos(0)
	{ }

	~rcref_freelist()
	{
		ptr<descriptor_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<descriptor_t> next = n->m_next;
			n->get_obj()->~type();
			n->descriptor_t::~descriptor_t();
			if ((n < (descriptor_t*)&(m_preallocated[0])) || (n >= (descriptor_t*)&(m_preallocated[num_preallocated])))
				m_allocator.deallocate(n);
			n = next;
		}
	}

	rcref<type> get() volatile
	{
		ptr<descriptor_t> desc;
		ptr<descriptor_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		for (;;)
		{
			if (!!oldHead)
			{
				ptr<descriptor_t> newHead = oldHead->m_next;
				if (!m_head.versioned_exchange(newHead, v, oldHead))
					continue;
				desc = oldHead;
				desc->reset_counts();
				break;
			}

			size_t oldPos = atomic::load(m_curPos);
			if (oldPos >= num_preallocated)
				desc = m_allocator.template allocate_type<placement_t>()->get_header();
			else
			{
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
				{
					m_head.get(oldHead, v);
					continue;
				}
				desc = m_preallocated[oldPos].get_header();
			}

			new (desc) descriptor_t;
			desc->m_freelist = this;
			placement_rcnew(desc->get_obj(), *desc);
			break;
		}

		type* obj = desc->get_obj();
		rcref<type> result(obj, desc);
		return result;
	}
};


template <typename T, class allocator_type>
class rcref_freelist<T, allocator_type, 0>
{
private:
	typedef rcref_freelist<T, allocator_type, 0> this_t;

	allocator_container<allocator_type> m_allocator;

	class descriptor_t : public rc_obj_base
	{
	public:
		volatile this_t* m_freelist;
		descriptor_t* m_next;

		virtual void released()
		{
			// We don't destruct
		}

		virtual void dispose()
		{
			// Return to freelist
			m_freelist->release(this);
		}

		T* get_obj() const { return placement_with_header<descriptor_t, T>::get_obj_from_header(this); }

		static descriptor_t* from_obj(T* obj) { return placement_with_header<descriptor_t, T>::get_header_from_obj(obj); }
	};

	typedef placement_with_header<descriptor_t, T>  placement_t;

	typedef typename versioned_ptr<descriptor_t>::version_t version_t;

	volatile versioned_ptr<descriptor_t> m_head;

	void release(descriptor_t* n) volatile
	{
		ptr<descriptor_t> oldHead;
		version_t oldVersion;
		m_head.get(oldHead, oldVersion);
		do {
			n->m_next = oldHead.get_ptr();
		} while (!m_head.versioned_exchange(n, oldVersion, oldHead));
	}

public:
	typedef T type;

	rcref_freelist()
	{ }

	rcref_freelist(volatile allocator_type& al)
		: m_allocator(al)
	{ }

	~rcref_freelist()
	{
		ptr<descriptor_t> n = m_head.get_ptr();
		while (!!n)
		{
			ptr<descriptor_t> next = n->m_next;
			n->get_obj()->~type();
			n->descriptor_t::~descriptor_t();
			m_allocator.deallocate(n);
			n = next;
		}
	}

	rcref<type> get() volatile
	{
		ptr<descriptor_t> desc;
		ptr<descriptor_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		for (;;)
		{
			if (!!oldHead)
			{
				ptr<descriptor_t> newHead = oldHead->m_next;
				if (!m_head.versioned_exchange(newHead, v, oldHead))
					continue;
				desc = oldHead;
				desc->reset_counts();
				break;
			}

			desc = m_allocator.template allocate_type<placement_t>();
			new (desc) descriptor_t;
			desc->m_freelist = this;
			placement_rcnew(desc->get_obj(), *desc);
			break;
		}

		type* obj = desc->get_obj();
		rcref<type> result(obj, desc);
		return result;
	}
};


}


#endif
