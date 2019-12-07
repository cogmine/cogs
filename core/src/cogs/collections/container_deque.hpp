//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_DEQUE
#define COGS_HEADER_COLLECTION_CONTAINER_DEQUE


#include "cogs/collections/dlink.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


enum class insert_mode
{
	normal = 0,
	only_if_empty = 1,
	only_if_not_empty = 2
};


// A lock-free deque implementation based loosely on a paper by Maged M. Michael


/// @ingroup LockFreeCollections
/// @brief A double-ended queue container collection
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: false
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = false, class allocator_type = default_allocator>
class container_deque
{
public:
	typedef T type;
	typedef container_deque<type, false, allocator_type> this_t;

private:
	class link_t : public dlink_t<link_t, versioned_ptr, default_dlink_accessor<link_t, versioned_ptr> >
	{
	public:
		typedef typename versioned_ptr<link_t>::version_t version_t;

		type m_contents;

		link_t(const type& t)
			: m_contents(t)
		{ }
	};

	mutable hazard m_hazard;

	struct content_t
	{
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_head;
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_tail;
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

	void stabilize(content_t& oldContents) volatile
	{
		typename hazard::pointer hazardPointer1;
		typename hazard::pointer hazardPointer2;
		typename link_t::version_t v;
		content_t newContents;
		for (;;)
		{
			atomic::load(m_contents, oldContents);
			ptr<link_t> headRaw = oldContents.m_head;
			if (!!headRaw.get_mark()) // prepend in progress.  Fix up head->next->prev to point to head
			{
				link_t* head = headRaw.get_unmarked();
				hazardPointer1.bind(m_hazard, head);
				atomic::load(m_contents, oldContents);
				if ((headRaw == oldContents.m_head) && (hazardPointer1.validate()))
				{
					link_t* next = head->get_next_link().get_ptr();
					hazardPointer2.bind(m_hazard, next);
					atomic::load(m_contents, oldContents);
					if ((headRaw == oldContents.m_head) && (hazardPointer2.validate()))
					{
						v = ((volatile link_t*)next)->get_prev_link().get_version();
						atomic::load(m_contents, oldContents);
						if (headRaw == oldContents.m_head)
						{
							((volatile link_t*)next)->get_prev_link().versioned_exchange(head, v);
							newContents.m_head = head;
							newContents.m_tail = oldContents.m_tail;
							atomic::compare_exchange(m_contents, newContents, oldContents);
						}
						if (hazardPointer2.release())
							m_allocator.template destruct_deallocate_type<link_t>(next);
					}
					if (hazardPointer1.release())
						m_allocator.template destruct_deallocate_type<link_t>(head);
				}
			}
			else
			{
				ptr<link_t> tailRaw = oldContents.m_tail;
				if (!tailRaw.get_mark())
					break;

				link_t* tail = tailRaw.get_unmarked();
				hazardPointer1.bind(m_hazard, tail);
				atomic::load(m_contents, oldContents);
				if ((tailRaw == oldContents.m_tail) && (hazardPointer1.validate()))
				{
					link_t* prev = tail->get_prev_link().get_ptr();
					hazardPointer2.bind(m_hazard, prev);
					atomic::load(m_contents, oldContents);
					if ((tailRaw == oldContents.m_tail) && (hazardPointer2.validate()))
					{
						v = ((volatile link_t*)prev)->get_next_link().get_version();
						atomic::load(m_contents, oldContents);
						if (tailRaw == oldContents.m_tail)
						{
							((volatile link_t*)prev)->get_next_link().versioned_exchange(tail, v);
							newContents.m_head = oldContents.m_head;
							newContents.m_tail = tail;
							atomic::compare_exchange(m_contents, newContents, oldContents);
						}
						if (hazardPointer2.release())
							m_allocator.template destruct_deallocate_type<link_t>(prev);
					}
					if (hazardPointer1.release())
						m_allocator.template destruct_deallocate_type<link_t>(tail);
				}

			}
		}
	}

	void clear_inner()
	{
		ptr<link_t> l = m_contents.m_head;
		ptr<link_t> oldTail = m_contents.m_tail;
		if (!!l)
		{
			for (;;)
			{
				ptr<link_t> next = l->get_next_link().get_ptr();
				m_allocator.template destruct_deallocate_type<link_t>(l);
				if (l == oldTail)
					break;
				l = next;
			}
		}
	}

	template <bool fromEnd>
	bool remove_inner(type* t, bool& wasLast)
	{
		wasLast = false;

		ptr<link_t> oldLink = fromEnd ? m_contents.m_tail : m_contents.m_head;
		if (!oldLink)
			return false;

		if (!!t)
			*t = oldLink->m_contents;

		if (m_contents.m_tail == m_contents.m_head)
		{
			m_contents.m_tail = m_contents.m_head = 0;
			wasLast = true;
		}
		else if (fromEnd)
			m_contents.m_tail = oldLink->get_prev_link().get_ptr();
		else
			m_contents.m_head = oldLink->get_next_link().get_ptr();

		m_allocator.template destruct_deallocate_type<link_t>(oldLink);
		return true;
	}

	template <bool fromEnd>
	bool remove_inner(type* t, bool& wasLast) volatile
	{
		bool result = false;

		typename hazard::pointer hazardPointer;
		content_t newContents;
		content_t oldContents;
		for (;;)
		{
			wasLast = false;
			stabilize(oldContents);

			if (!oldContents.m_head)
				break;

			ptr<link_t> oldLink = fromEnd ? oldContents.m_tail : oldContents.m_head;
			hazardPointer.bind(m_hazard, oldLink.get_ptr());
			atomic::load(m_contents, newContents);
			if ((newContents.m_head != oldContents.m_head) || (newContents.m_tail != oldContents.m_tail) || (!hazardPointer.validate()))
				continue;

			if (oldContents.m_head == oldContents.m_tail)
			{
				newContents.m_head = newContents.m_tail = 0;
				wasLast = true;
			}
			else if (fromEnd)
			{
				newContents.m_head = oldContents.m_head;
				newContents.m_tail = oldLink->get_prev_link().get_ptr();
			}
			else
			{
				newContents.m_head = oldLink->get_next_link().get_ptr();
				newContents.m_tail = oldContents.m_tail;
			}

			bool done = atomic::compare_exchange(m_contents, newContents, oldContents);
			if (done)
			{
				result = true;
				if (!!t)
					*t = oldLink->m_contents;
				bool b = m_hazard.release(oldLink.get_ptr()); // We have a hazard, so this will not return ownership
				COGS_ASSERT(!b);
			}

			if (hazardPointer.release())
				m_allocator.template destruct_deallocate_type<link_t>(oldLink);

			if (done)
				break;
			// continue;
		}
		return result;
	}

	template <bool fromEnd>
	bool peek_inner(type& t) const volatile
	{
		bool result = false;
		typename hazard::pointer hazardPointer;
		content_t newContents;
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		for (;;)
		{
			if (!!oldContents.m_head)
			{
				ptr<link_t> oldLink = (fromEnd ? oldContents.m_tail : oldContents.m_head);
				oldLink.clear_mark();
				hazardPointer.bind(m_hazard, oldLink.get_ptr());
				atomic::load(m_contents, newContents);
				if ((oldLink != (fromEnd ? newContents.m_tail : newContents.m_head)) || (!hazardPointer.validate()))
					continue;
				t = oldLink->m_contents;
				if (hazardPointer.release())
					m_allocator.template destruct_deallocate_type<link_t>(oldLink);
				result = true;
			}
			break;
		}
		return result;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(const type& t)
	{
		ptr<link_t> oldLink = atEnd ? m_contents.m_tail : m_contents.m_head;
		bool wasEmpty = !oldLink;

		ptr<link_t> l = m_allocator.template allocate_type<link_t>();
		new (l.get_ptr()) link_t(t);

		if (wasEmpty)
		{
			if (insertMode == insert_mode::only_if_not_empty)
				return true;

			m_contents.m_tail = m_contents.m_head = l.get_ptr();
		}
		else
		{
			if (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				l->set_prev_link(oldLink);
				oldLink->set_next_link(l);
				m_contents.m_tail = l.get_ptr();
			}
			else
			{
				l->set_next_link(oldLink);
				oldLink->set_prev_link(l);
				m_contents.m_head = l.get_ptr();
			}
		}

		return wasEmpty;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(const type& t) volatile
	{
		bool wasEmpty = false;

		ptr<link_t> myLink = m_allocator.template allocate_type<link_t>();
		new (myLink.get_ptr()) link_t(t);

		content_t newContents;
		content_t oldContents;
		bool done = false;
		for (;;)
		{
			stabilize(oldContents);
			if (done)
				break;

			if (!oldContents.m_head) // The list was empty.
			{
				if (insertMode == insert_mode::only_if_not_empty)
					return true;

				newContents.m_head = newContents.m_tail = myLink.get_ptr();
				if (atomic::compare_exchange(m_contents, newContents, oldContents))
				{ // success
					wasEmpty = true;
					break;
				}
				continue;
			}

			if (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				myLink->set_prev_link(oldContents.m_tail);
				newContents.m_tail = myLink.get_marked(1); // indicate an append is in progress.
				newContents.m_head = oldContents.m_head;
			}
			else
			{
				myLink->set_next_link(oldContents.m_head);
				newContents.m_head = myLink.get_marked(1); // indicate a prepend is in progress.
				newContents.m_tail = oldContents.m_tail;
			}
			done = atomic::compare_exchange(m_contents, newContents, oldContents);
			// continue;
		}
		return wasEmpty;
	}

	container_deque(const container_deque& src) = delete;
	this_t& operator=(const container_deque& src) = delete;

	allocator_container<allocator_type> m_allocator;

public:
	container_deque()
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
	}

	container_deque(this_t&& src)
		: m_allocator(std::move(src.m_allocator))
	{
		m_contents.m_head = src.m_contents.m_head;
		m_contents.m_tail = src.m_contents.m_tail;
		src.m_contents.m_head = 0;
		src.m_contents.m_tail = 0;
	}

	this_t& operator=(this_t&& src)
	{
		clear_inner();
		m_allocator = std::move(src.m_allocator);
		m_contents.m_head = src.m_contents.m_head;
		m_contents.m_tail = src.m_contents.m_tail;
		src.m_contents.m_head = 0;
		src.m_contents.m_tail = 0;
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		swap(src);
		src.clear();
		return *this;
	}

	explicit container_deque(volatile allocator_type& al)
		: m_allocator(al)
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
	}

	~container_deque() { clear_inner(); }

	void clear() { clear_inner(); m_contents.m_head = 0; m_contents.m_tail = 0; }
	void clear() volatile
	{
		content_t newContents;
		newContents.m_head = 0;
		newContents.m_tail = 0;
		content_t oldContents;
		do {
			stabilize(oldContents);
		} while (!atomic::compare_exchange(m_contents, newContents, oldContents));

		ptr<link_t> l = oldContents.m_head;
		ptr<link_t> oldTail = oldContents.m_tail;
		if (!!l)
		{
			for (;;)
			{
				ptr<link_t> next = l->get_next_link().get_ptr();
				if (m_hazard.release(l.get_ptr()))
					m_allocator.template destruct_deallocate_type<link_t>(l);
				if (l == oldTail)
					break;
				l = next;
			}
		}
	}

	// Return true if list was empty, false if it was not
	bool prepend(const type& t) { return insert_inner<false, insert_mode::normal>(t); }
	bool prepend(const type& t) volatile { return insert_inner<false, insert_mode::normal>(t); }
	bool append(const type& t) { return insert_inner<true, insert_mode::normal>(t); }
	bool append(const type& t) volatile { return insert_inner<true, insert_mode::normal>(t); }

	// Returns true if added, false if not added
	bool prepend_if_not_empty(const type& t) { return !insert_inner<false, insert_mode::only_if_not_empty>(t); }
	bool prepend_if_not_empty(const type& t) volatile { return !insert_inner<false, insert_mode::only_if_not_empty>(t); }
	bool append_if_not_empty(const type& t) { return !insert_inner<true, insert_mode::only_if_not_empty>(t); }
	bool append_if_not_empty(const type& t) volatile { return !insert_inner<true, insert_mode::only_if_not_empty>(t); }
	bool insert_if_empty(const type& t) { return insert_inner<true, insert_mode::only_if_empty>(t); }
	bool insert_if_empty(const type& t) volatile { return insert_inner<true, insert_mode::only_if_empty>(t); }

	bool peek_first(type& t) const { ptr<link_t> l = m_contents.m_head; if (!l) return false; t = l->m_contents; return true; }
	bool peek_first(type& t) const volatile { return peek_inner<false>(t); }

	bool peek_last(type& t) const { ptr<link_t> l = m_contents.m_tail; if (!l) return false; t = l->m_contents; return true; }
	bool peek_last(type& t) const volatile { return peek_inner<true>(t); }

	bool is_empty() const { return !m_contents.m_head; }
	bool is_empty() const volatile { link_t* l; atomic::load(m_contents.m_head, l); return !l; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	bool contains_one() const { return !!m_contents.m_head && m_contents.m_head == m_contents.m_tail; }
	bool contains_one() const volatile { content_t c; atomic::load(m_contents, c); return !!c.m_head && c.m_head == c.m_tail; }

	bool pop_first(type& t, bool& wasLast) { return remove_inner<false>(&t, wasLast); }
	bool pop_first(type& t, bool& wasLast) volatile { return remove_inner<false>(&t, wasLast); }
	bool pop_last(type& t, bool& wasLast) { return remove_inner<true>(&t, wasLast); }
	bool pop_last(type& t, bool& wasLast) volatile { return remove_inner<true>(&t, wasLast); }

	bool pop_first(type& t) { bool wasLast; return pop_first(t, wasLast); }
	bool pop_first(type& t) volatile { bool wasLast; return pop_first(t, wasLast); }
	bool pop_last(type& t) { bool wasLast; return pop_last(t, wasLast); }
	bool pop_last(type& t) volatile { bool wasLast; return pop_last(t, wasLast); }


	bool remove_first(bool& wasLast) { return remove_inner<false>(0, wasLast); }
	bool remove_first(bool& wasLast) volatile { return remove_inner<false>(0, wasLast); }
	bool remove_last(bool& wasLast) { return remove_inner<true>(0, wasLast); }
	bool remove_last(bool& wasLast) volatile { return remove_inner<true>(0, wasLast); }

	bool remove_first() { bool wasLast; return remove_first(wasLast); }
	bool remove_first() volatile { bool wasLast; return remove_first(wasLast); }
	bool remove_last() { bool wasLast; return remove_last(wasLast); }
	bool remove_last() volatile { bool wasLast; return remove_last(wasLast); }

	void swap(this_t& wth)
	{
		content_t tmp = m_contents;
		m_contents = wth.m_contents;
		wth.m_contents = tmp;
		allocator_container<allocator_type> tmp2 = m_allocator;
		m_allocator = wth.m_allocator;
		wth.m_allocator = tmp2;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile
	{
		content_t oldContents;
		do {
			stabilize(oldContents);
		} while (!atomic::compare_exchange(m_contents, wth.m_contents, oldContents));
		wth.m_contents = oldContents;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth) { wth.swap(*this); }

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	this_t exchange(this_t&& src) volatile
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}
};


template <typename T, class allocator_type>
class container_deque<T, true, allocator_type>
{
public:
	typedef T type;
	typedef container_deque<type, true, allocator_type> this_t;

private:
	class link_t : public dlink_t<link_t, versioned_ptr, default_dlink_accessor<link_t, versioned_ptr> >
	{
	public:
		typedef typename versioned_ptr<link_t>::version_t version_t;

		type m_contents;

		alignas (atomic::get_alignment_v<size_t>) size_t m_remainingCount;

		volatile boolean m_removed;

		link_t(const type& t, size_t n)
			: m_contents(t)
		{
			m_remainingCount = n;
		}
	};

	mutable hazard m_hazard;

	struct content_t
	{
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_head;
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_tail;
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

	void stabilize(content_t& oldContents) volatile
	{
		typename hazard::pointer hazardPointer1;
		typename hazard::pointer hazardPointer2;
		typename link_t::version_t v;
		content_t newContents;
		for (;;)
		{
			atomic::load(m_contents, oldContents);
			ptr<link_t> headRaw = oldContents.m_head;
			if (!!headRaw.get_mark()) // prepend in progress.  Fix up head->next->prev to point to head
			{
				link_t* head = headRaw.get_unmarked();
				hazardPointer1.bind(m_hazard, head);
				atomic::load(m_contents, oldContents);
				if ((headRaw == oldContents.m_head) && (hazardPointer1.validate()))
				{
					link_t* next = head->get_next_link().get_ptr();
					hazardPointer2.bind(m_hazard, next);
					atomic::load(m_contents, oldContents);
					if ((headRaw == oldContents.m_head) && (hazardPointer2.validate()))
					{
						v = ((volatile link_t*)next)->get_prev_link().get_version();
						atomic::load(m_contents, oldContents);
						if (headRaw == oldContents.m_head)
						{
							((volatile link_t*)next)->get_prev_link().versioned_exchange(head, v);
							newContents.m_head = head;
							newContents.m_tail = oldContents.m_tail;
							atomic::compare_exchange(m_contents, newContents, oldContents);
						}
						if (hazardPointer2.release())
							m_allocator.template destruct_deallocate_type<link_t>(next);
					}
					if (hazardPointer1.release())
						m_allocator.template destruct_deallocate_type<link_t>(head);
				}
			}
			else
			{
				ptr<link_t> tailRaw = oldContents.m_tail;
				if (!tailRaw.get_mark())
					break;

				link_t* tail = tailRaw.get_unmarked();
				hazardPointer1.bind(m_hazard, tail);
				atomic::load(m_contents, oldContents);
				if ((tailRaw == oldContents.m_tail) && (hazardPointer1.validate()))
				{
					link_t* prev = tail->get_prev_link().get_ptr();
					hazardPointer2.bind(m_hazard, prev);
					atomic::load(m_contents, oldContents);
					if ((tailRaw == oldContents.m_tail) && (hazardPointer2.validate()))
					{
						v = ((volatile link_t*)prev)->get_next_link().get_version();
						atomic::load(m_contents, oldContents);
						if (tailRaw == oldContents.m_tail)
						{
							((volatile link_t*)prev)->get_next_link().versioned_exchange(tail, v);
							newContents.m_head = oldContents.m_head;
							newContents.m_tail = tail;
							atomic::compare_exchange(m_contents, newContents, oldContents);
						}
						if (hazardPointer2.release())
							m_allocator.template destruct_deallocate_type<link_t>(prev);
					}
					if (hazardPointer1.release())
						m_allocator.template destruct_deallocate_type<link_t>(tail);
				}

			}
		}
	}

	void clear_inner()
	{
		ptr<link_t> l = m_contents.m_head;
		ptr<link_t> oldTail = m_contents.m_tail;
		if (!!l)
		{
			for (;;)
			{
				ptr<link_t> next = l->get_next_link().get_ptr();
				m_allocator.template destruct_deallocate_type<link_t>(l);
				if (l == oldTail)
					break;
				l = next;
			}
		}
	}

	template <bool fromEnd>
	bool remove_inner(type* t, bool& wasLast)
	{
		wasLast = false;
		ptr<link_t> oldLink = fromEnd ? m_contents.m_tail : m_contents.m_head;
		if (!oldLink)
			return false;

		if (!!t)
			*t = oldLink->get();

		if (!--(oldLink->m_remainingCount))
		{
			if (m_contents.m_tail == m_contents.m_head)
			{
				m_contents.m_tail = m_contents.m_head = 0;
				wasLast = true;
			}
			else if (fromEnd)
				m_contents.m_tail = oldLink->get_prev_link().get_ptr();
			else
				m_contents.m_head = oldLink->get_next_link().get_ptr();
			m_allocator.template destruct_deallocate_type<link_t>(oldLink);
		}
		return true;
	}

	template <bool fromEnd>
	bool remove_inner(type* t, bool& wasLast) volatile
	{
		bool result = false;
		typename hazard::pointer hazardPointer;
		bool ownedRemoval = false;
		content_t newContents;
		content_t oldContents;
		for (;;)
		{
			wasLast = false;
			stabilize(oldContents);

			if (!oldContents.m_head)
				break;

			ptr<link_t> oldLink = fromEnd ? oldContents.m_tail : oldContents.m_head;
			hazardPointer.bind(m_hazard, oldLink.get_ptr());
			atomic::load(m_contents, newContents);
			if ((newContents.m_head != oldContents.m_head) || (newContents.m_tail != oldContents.m_tail) || (!hazardPointer.validate()))
				continue;

			// If reference count is >1, decrement it.
			// If reference count == 1, try to remove.
			size_t oldCount;
			atomic::load(oldLink->m_remainingCount, oldCount);
			for (;;)
			{
				ownedRemoval = (oldCount == 1); // if 1, leave it at one, and only claim removal if we successfully remove it.
				if (!ownedRemoval)
				{
					if (!atomic::compare_exchange(oldLink->m_remainingCount, oldCount - 1, oldCount, oldCount))
						continue;
					result = true;
				}
				break;
			}
			if (ownedRemoval)
			{
				if (oldContents.m_head == oldContents.m_tail)
				{
					newContents.m_head = newContents.m_tail = 0;
					wasLast = true;
				}
				else if (fromEnd)
				{
					newContents.m_head = oldContents.m_head;
					newContents.m_tail = oldLink->get_prev_link().get_ptr();
				}
				else
				{
					newContents.m_head = oldLink->get_next_link().get_ptr();
					newContents.m_tail = oldContents.m_tail;
				}
				result = atomic::compare_exchange(m_contents, newContents, oldContents);
			}
			if (result)
			{
				if (!!t)
					*t = oldLink->m_contents;
				if (ownedRemoval)
				{
					oldLink->m_removed = true;
					bool b = m_hazard.release(oldLink.get_ptr()); // We have a hazard, so this will not return ownership
					COGS_ASSERT(!b);
				}
			}

			if (hazardPointer.release())
				m_allocator.template destruct_deallocate_type<link_t>(oldLink);

			if (!result)
				continue;
			break;
		}

		return result;
	}

	template <bool fromEnd>
	bool peek_inner(type& t) const volatile
	{
		bool result = false;
		typename hazard::pointer hazardPointer;
		content_t newContents;
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		for (;;)
		{
			if (!!oldContents.m_head)
			{
				ptr<link_t> oldLink = (fromEnd ? oldContents.m_tail : oldContents.m_head);
				oldLink.clear_mark();
				hazardPointer.bind(m_hazard, oldLink.get_ptr());
				atomic::load(m_contents, newContents);
				if ((oldLink != (fromEnd ? newContents.m_tail : newContents.m_head)) || (!hazardPointer.validate()))
					continue;
				t = oldLink->m_contents;
				if (hazardPointer.release())
					m_allocator.template destruct_deallocate_type<link_t>(oldLink);
				result = true;
			}
			break;
		}
		return result;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(const type& t, size_t n)
	{
		ptr<link_t> oldLink = atEnd ? m_contents.m_tail : m_contents.m_head;
		bool wasEmpty = !oldLink;
		if (wasEmpty)
		{
			if (insertMode == insert_mode::only_if_not_empty)
				return true;
		}
		else if (insertMode == insert_mode::only_if_empty)
			return false;

		if ((!wasEmpty) && (oldLink->get() == t))
			oldLink->m_remainingCount += n;
		else
		{
			ptr<link_t> l = m_allocator.template allocate_type<link_t>();
			new (l.get_ptr()) link_t(t, n);
			if (wasEmpty)
				m_contents.m_tail = m_contents.m_head = l.get_ptr();
			else if (atEnd)
			{
				l->set_prev_link(oldLink);
				oldLink->set_next_link(l);
				m_contents.m_tail = l.get_ptr();
			}
			else
			{
				l->set_next_link(oldLink);
				oldLink->set_prev_link(l);
				m_contents.m_head = l.get_ptr();
			}
		}
		return wasEmpty;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(const type& t, size_t n) volatile
	{
		bool wasEmpty = false;

		ptr<link_t> myLink = m_allocator.template allocate_type<link_t>();
		new (myLink.get_ptr()) link_t(t, n);

		content_t newContents;
		content_t oldContents;
		bool done = false;
		for (;;)
		{
			stabilize(oldContents);
			if (done)
				break;

			if (!oldContents.m_head) // The list was empty.
			{
				if (insertMode == insert_mode::only_if_not_empty)
					return true;

				newContents.m_head = newContents.m_tail = myLink.get_ptr();
				if (atomic::compare_exchange(m_contents, newContents, oldContents))
				{ // success
					wasEmpty = true;
					break;
				}
				continue;
			}

			if (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				myLink->set_prev_link(oldContents.m_tail);
				newContents.m_tail = myLink.get_marked(1); // indicate an append is in progress.
				newContents.m_head = oldContents.m_head;
			}
			else
			{
				myLink->set_next_link(oldContents.m_head);
				newContents.m_head = myLink.get_marked(1); // indicate a prepend is in progress.
				newContents.m_tail = oldContents.m_tail;
			}
			done = atomic::compare_exchange(m_contents, newContents, oldContents);
			// continue
		}
		return wasEmpty;
	}

	container_deque(const container_deque& src) = delete;
	this_t& operator=(const container_deque& src) = delete;

	allocator_container<allocator_type> m_allocator;

public:
	container_deque()
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
	}

	container_deque(this_t&& src)
		: m_allocator(std::move(src.m_allocator))
	{
		m_contents.m_head = src.m_contents.m_head;
		m_contents.m_tail = src.m_contents.m_tail;
		src.m_contents.m_head = 0;
		src.m_contents.m_tail = 0;
	}

	explicit container_deque(volatile allocator_type& al)
		: m_allocator(al)
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
	}

	this_t& operator=(this_t&& src)
	{
		clear_inner();
		m_allocator = std::move(src.m_allocator);
		m_contents.m_head = src.m_contents.m_head;
		m_contents.m_tail = src.m_contents.m_tail;
		src.m_contents.m_head = 0;
		src.m_contents.m_tail = 0;
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		swap(src);
		src.clear();
		return *this;
	}

	~container_deque() { clear_inner(); }

	void clear() { clear_inner(); m_contents.m_head = 0; m_contents.m_tail = 0; }
	void clear() volatile
	{
		content_t newContents;
		newContents.m_head = 0;
		newContents.m_tail = 0;
		content_t oldContents;
		do {
			stabilize(oldContents);
		} while (!atomic::compare_exchange(m_contents, newContents, oldContents));

		ptr<link_t> l = oldContents.m_head.get_ptr();
		ptr<link_t> oldTail = oldContents.m_tail.get_ptr();
		if (!!l)
		{
			for (;;)
			{
				ptr<link_t> next = l->get_next_link().get_ptr();
				if (m_hazard.release(l.get_ptr()))
					m_allocator.template destruct_deallocate_type<link_t>(l);
				if (l == oldTail)
					break;
				l = next;
			}
		}
	}

	// Return true if list was empty, false if it was not
	bool prepend(const type& t, size_t n = 1) { return insert_inner<false, insert_mode::normal>(t, n); }
	bool prepend(const type& t, size_t n = 1) volatile { return insert_inner<false, insert_mode::normal>(t, n); }
	bool append(const type& t, size_t n = 1) { return insert_inner<true, insert_mode::normal>(t, n); }
	bool append(const type& t, size_t n = 1) volatile { return insert_inner<true, insert_mode::normal>(t, n); }

	// Returns true if added, false if not added
	bool prepend_if_not_empty(const type& t, size_t n = 1) { return !insert_inner<false, insert_mode::only_if_not_empty>(t, n); }
	bool prepend_if_not_empty(const type& t, size_t n = 1) volatile { return !insert_inner<false, insert_mode::only_if_not_empty>(t, n); }
	bool append_if_not_empty(const type& t, size_t n = 1) { return !insert_inner<true, insert_mode::only_if_not_empty>(t, n); }
	bool append_if_not_empty(const type& t, size_t n = 1) volatile { return !insert_inner<true, insert_mode::only_if_not_empty>(t, n); }
	bool insert_if_empty(const type& t, size_t n = 1) { return insert_inner<true, insert_mode::only_if_empty>(t, n); }
	bool insert_if_empty(const type& t, size_t n = 1) volatile { return insert_inner<true, insert_mode::only_if_empty>(t, n); }

	bool peek_first(type& t) const { ptr<link_t> l = m_contents.m_head; if (!l) return false; t = l->m_contents; return true; }
	bool peek_first(type& t) const volatile { return peek_inner<false>(t); }

	bool peek_last(type& t) const { ptr<link_t> l = m_contents.m_tail; if (!l) return false; t = l->m_contents; return true; }
	bool peek_last(type& t) const volatile { return peek_inner<true>(t); }

	bool is_empty() const { return !m_contents.m_head; }
	bool is_empty() const volatile { link_t* l; atomic::load(m_contents.m_head, l); return !l; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	bool contains_one() const { return !!m_contents.m_head && m_contents.m_head == m_contents.m_tail; }
	bool contains_one() const volatile { content_t c; atomic::load(m_contents, c); return !!c.m_head && c.m_head == c.m_tail; }

	bool pop_first(type& t, bool& wasLast) { return remove_inner<false>(&t, wasLast); }
	bool pop_first(type& t, bool& wasLast) volatile { return remove_inner<false>(&t, wasLast); }
	bool pop_last(type& t, bool& wasLast) { return remove_inner<true>(&t, wasLast); }
	bool pop_last(type& t, bool& wasLast) volatile { return remove_inner<true>(&t, wasLast); }

	bool pop_first(type& t) { bool wasLast; return pop_first(t, wasLast); }
	bool pop_first(type& t) volatile { bool wasLast; return pop_first(t, wasLast); }
	bool pop_last(type& t) { bool wasLast; return pop_last(t, wasLast); }
	bool pop_last(type& t) volatile { bool wasLast; return pop_last(t, wasLast); }


	bool remove_first(bool& wasLast) { return remove_inner<false>(0, wasLast); }
	bool remove_first(bool& wasLast) volatile { return remove_inner<false>(0, wasLast); }
	bool remove_last(bool& wasLast) { return remove_inner<true>(0, wasLast); }
	bool remove_last(bool& wasLast) volatile { return remove_inner<true>(0, wasLast); }

	bool remove_first() { bool wasLast; return remove_first(wasLast); }
	bool remove_first() volatile { bool wasLast; return remove_first(wasLast); }
	bool remove_last() { bool wasLast; return remove_last(wasLast); }
	bool remove_last() volatile { bool wasLast; return remove_last(wasLast); }

	void swap(this_t& wth)
	{
		content_t tmp = m_contents;
		m_contents = wth.m_contents;
		wth.m_contents = tmp;
		allocator_container<allocator_type> tmp2 = m_allocator;
		m_allocator = wth.m_allocator;
		wth.m_allocator = tmp2;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile
	{
		content_t oldContents;
		do {
			stabilize(oldContents);
		} while (!atomic::compare_exchange(m_contents, wth.m_contents, oldContents));
		wth.m_contents = oldContents;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth) { wth.swap(*this); }

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	this_t exchange(this_t&& src) volatile
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}
};


}


#include "cogs/collections/container_dlist.hpp"


#endif
