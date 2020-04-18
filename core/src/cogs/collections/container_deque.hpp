//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_DEQUE
#define COGS_HEADER_COLLECTION_CONTAINER_DEQUE


#include <initializer_list>

#include "cogs/collections/dlink.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/allocator.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {

// Based loosely on a paper by Maged M. Michael titled, "CAS-Based Lock-Free Algorithm for Shared Deques".

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

	struct volatile_pop_result
	{
		bool wasRemoved;
		bool wasEmptied;
	};

private:
	class link_t : public delayed_construction<type>, public dlink_t<link_t, versioned_ptr, default_dlink_accessor<link_t, versioned_ptr> >
	{
	};

	typedef typename versioned_ptr<link_t>::version_t version_t;

	mutable hazard m_hazard;

	struct content_t
	{
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_head;
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_tail;
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

	void stabilize(content_t& oldContents) volatile
	{
		hazard::pointer hazardPointer1;
		hazard::pointer hazardPointer2;
		version_t v;
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
	bool remove_inner(type* t)
	{
		ptr<link_t> oldLink = fromEnd ? m_contents.m_tail : m_contents.m_head;
		if (!oldLink)
			return false;
		if (!!t)
			*t = std::move(oldLink->get());
		if (m_contents.m_tail == m_contents.m_head)
			m_contents.m_tail = m_contents.m_head = 0;
		else if (fromEnd)
			m_contents.m_tail = oldLink->get_prev_link().get_ptr();
		else
			m_contents.m_head = oldLink->get_next_link().get_ptr();
		m_allocator.template destruct_deallocate_type<link_t>(oldLink);
		return true;
	}

	template <bool fromEnd>
	volatile_pop_result remove_inner(type* t) volatile
	{
		bool result = false;
		bool wasLast = false;
		typename hazard::pointer hazardPointer;
		content_t newContents;
		content_t oldContents;
		for (;;)
		{
			stabilize(oldContents);

			if (!oldContents.m_head)
				break;

			ptr<link_t> oldLink = fromEnd ? oldContents.m_tail : oldContents.m_head;
			hazardPointer.bind(m_hazard, oldLink.get_ptr());
			atomic::load(m_contents, newContents);
			if ((newContents.m_head != oldContents.m_head) || (newContents.m_tail != oldContents.m_tail) || (!hazardPointer.validate()))
				continue;

			wasLast = oldContents.m_head == oldContents.m_tail;
			if (wasLast)
				newContents.m_head = newContents.m_tail = 0;
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
					*t = std::move(oldLink->get());
				bool b = m_hazard.release(oldLink.get_ptr()); // We have a hazard, so this will not return ownership
				COGS_ASSERT(!b);
			}

			if (hazardPointer.release())
				m_allocator.template destruct_deallocate_type<link_t>(oldLink);

			if (done)
				break;
			// continue;
		}
		return { result, wasLast };
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
				t = oldLink->get();
				if (hazardPointer.release())
					m_allocator.template destruct_deallocate_type<link_t>(oldLink);
				result = true;
			}
			break;
		}
		return result;
	}

	enum class insert_mode
	{
		normal = 0,
		only_if_empty = 1,
		only_if_not_empty = 2
	};

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(link_t* l)
	{
		ptr<link_t> oldLink = atEnd ? m_contents.m_tail : m_contents.m_head;
		bool wasEmpty = !oldLink;
		if (wasEmpty)
		{
			if constexpr (insertMode == insert_mode::only_if_not_empty)
				return true;

			m_contents.m_tail = m_contents.m_head = l;
		}
		else
		{
			if constexpr (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				l->set_prev_link(oldLink);
				oldLink->set_next_link(l);
				m_contents.m_tail = l;
			}
			else
			{
				l->set_next_link(oldLink);
				oldLink->set_prev_link(l);
				m_contents.m_head = l;
			}
		}

		return wasEmpty;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(link_t* lnk) volatile
	{
		bool wasEmpty = false;
		ptr<link_t> l = lnk;
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
				if constexpr (insertMode == insert_mode::only_if_not_empty)
					return true;

				newContents.m_head = newContents.m_tail = l.get_ptr();
				if (atomic::compare_exchange(m_contents, newContents, oldContents))
				{ // success
					wasEmpty = true;
					break;
				}
				continue;
			}

			if constexpr (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				l->set_prev_link(oldContents.m_tail);
				newContents.m_tail = l.get_marked(1); // indicate an append is in progress.
				newContents.m_head = oldContents.m_head;
			}
			else
			{
				l->set_next_link(oldContents.m_head);
				newContents.m_head = l.get_marked(1); // indicate a prepend is in progress.
				newContents.m_tail = oldContents.m_tail;
			}
			done = atomic::compare_exchange(m_contents, newContents, oldContents);
			// continue;
		}
		return wasEmpty;
	}

	container_deque(const container_deque& src) = delete;
	container_deque(const volatile container_deque& src) = delete;

	this_t& operator=(const container_deque& src) = delete;
	this_t& operator=(const volatile container_deque& src) = delete;

	volatile this_t& operator=(container_deque&& src) volatile = delete;
	volatile this_t& operator=(const container_deque& src) volatile = delete;
	volatile this_t& operator=(const volatile container_deque& src) volatile = delete;

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

	container_deque(const std::initializer_list<type>& src)
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
		for (auto& entry : src)
			append(entry);
	}

	container_deque(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_allocator(std::move(src.m_allocator))
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
		for (auto& entry : src)
			append(entry);
	}

	~container_deque() { clear_inner(); }

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

	this_t& operator=(const std::initializer_list<type>& src)
	{
		this_t tmp(src);
		*this = std::move(tmp);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.append(std::forward<args_t>(args)), ...);
		return result;
	}

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

	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F>
	bool prepend_via(F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l);
	}

	template <typename F>
	bool prepend_via(F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l);
	}

	bool prepend(const type& src) { return prepend_via([&](type& t) { new (&t) type(src); }); }
	bool prepend(type&& src) { return prepend_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend(F&& f) { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace(args_t&&... args) { return prepend_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool prepend(const type& src) volatile { return prepend_via([&](type& t) { new (&t) type(src); }); }
	bool prepend(type&& src) volatile { return prepend_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend(F&& f) volatile { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace(args_t&&... args) volatile { return prepend_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// Return true if list was empty, false if it was not
	template <typename F>
	bool append_via(F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l);
	}

	template <typename F>
	bool append_via(F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l);
	}

	bool append(const type& src) { return append_via([&](type& t) { new (&t) type(src); }); }
	bool append(type&& src) { return append_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) { return append_via(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) { return append_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append(const type& src) volatile { return append_via([&](type& t) { new (&t) type(src); }); }
	bool append(type&& src) volatile { return append_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) volatile { return append_via(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) volatile { return append_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// Returns true if added, false if not added
	template <typename F>
	bool prepend_via_if_not_empty(F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<false, insert_mode::only_if_not_empty>(l);
		return true;
	}

	template <typename F>
	bool prepend_via_if_not_empty(F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<false, insert_mode::only_if_not_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}


	bool prepend_if_not_empty(const type& src) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool prepend_if_not_empty(type&& src) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_if_not_empty(F&& f) { return prepend_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_if_not_empty(args_t&&... args) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool prepend_if_not_empty(const type& src) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool prepend_if_not_empty(type&& src) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_if_not_empty(F&& f) volatile { return prepend_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_if_not_empty(args_t&&... args) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Returns true if added, false if not added
	template <typename F>
	bool append_via_if_not_empty(F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_not_empty>(l);
		return true;
	}

	template <typename F>
	bool append_via_if_not_empty(F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<true, insert_mode::only_if_not_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}

	bool append_if_not_empty(const type& src) { return append_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool append_if_not_empty(type&& src) { return append_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) { return append_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append_if_not_empty(const type& src) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool append_if_not_empty(type&& src) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) volatile { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Return true if list was empty, false if it was not
	template <typename F>
	bool insert_via_if_empty(F&& f)
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_empty>(l);
		return true;
	}

	template <typename F>
	bool insert_via_if_empty(F&& f) volatile
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = insert_inner<true, insert_mode::only_if_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}

	bool insert_if_empty(const type& src) { return insert_via_if_empty([&](type& t) { new (&t) type(src); }); }
	bool insert_if_empty(type&& src) { return insert_via_if_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) { return insert_via_if_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool insert_if_empty(const type& src) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(src); }); }
	bool insert_if_empty(type&& src) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) volatile { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// It is only valid to peek at a volatile element if you know it wont
	// be removed by another thread.  Doing so is caller error.  Only
	// call from a thread or context that is the only remover of elements.

	bool peek_front(type& t) const { ptr<link_t> l = m_contents.m_head; if (!l) return false; t = l->get(); return true; }
	bool peek_front(type& t) const volatile { return peek_inner<false>(t); }

	bool peek_back(type& t) const { ptr<link_t> l = m_contents.m_tail; if (!l) return false; t = l->get(); return true; }
	bool peek_back(type& t) const volatile { return peek_inner<true>(t); }

	bool is_empty() const { return !m_contents.m_head; }
	bool is_empty() const volatile { link_t* l; atomic::load(m_contents.m_head, l); return !l; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	bool contains_one() const { return !!m_contents.m_head && m_contents.m_head == m_contents.m_tail; }
	bool contains_one() const volatile { content_t c; atomic::load(m_contents, c); return !!c.m_head && c.m_head == c.m_tail; }

	bool pop_front(type& t) { return remove_inner<false>(&t); }
	bool pop_back(type& t) { return remove_inner<true>(&t); }

	bool remove_front() { return remove_inner<false>(0); }
	bool remove_last() { return remove_inner<true>(0); }

	volatile_pop_result pop_front(type& t) volatile { return remove_inner<false>(&t); }
	volatile_pop_result pop_back(type& t) volatile { return remove_inner<true>(&t); }

	typedef volatile_pop_result volatile_remove_result;

	volatile_remove_result remove_front() volatile { return remove_inner<false>(0); }
	volatile_remove_result remove_last() volatile { return remove_inner<true>(0); }

	void swap(this_t& wth)
	{
		content_t tmp = m_contents;
		m_contents = wth.m_contents;
		wth.m_contents = tmp;
		m_allocator.swap(wth.m_allocator);
	}

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	void exchange(this_t&& src, this_t& rtn)
	{
		rtn = std::move(src);
		swap(rtn);
	}
};


template <typename T, class allocator_type>
class container_deque<T, true, allocator_type>
{
public:
	typedef T type;
	typedef container_deque<type, true, allocator_type> this_t;

	struct volatile_pop_result
	{
		bool wasRemoved;
		bool wasEmptied;
	};

private:
	class link_t : public delayed_construction<type>, public dlink_t<link_t, versioned_ptr, default_dlink_accessor<link_t, versioned_ptr> >
	{
	public:
		alignas (atomic::get_alignment_v<size_t>) size_t m_remainingCount;

		volatile boolean m_isRemoved;
	};

	typedef typename versioned_ptr<link_t>::version_t version_t;

	mutable hazard m_hazard;

	struct content_t
	{
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_head;
		alignas (atomic::get_alignment_v<link_t*>) link_t* m_tail;
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

	void stabilize(content_t& oldContents) volatile
	{
		hazard::pointer hazardPointer1;
		hazard::pointer hazardPointer2;
		version_t v;
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
	bool remove_inner(type* t)
	{
		ptr<link_t> oldLink = fromEnd ? m_contents.m_tail : m_contents.m_head;
		if (!oldLink)
			return false;
		if (!--(oldLink->m_remainingCount))
		{
			if (!!t)
				*t = std::move(oldLink->get());
			if (m_contents.m_tail == m_contents.m_head)
				m_contents.m_tail = m_contents.m_head = 0;
			else if (fromEnd)
				m_contents.m_tail = oldLink->get_prev_link().get_ptr();
			else
				m_contents.m_head = oldLink->get_next_link().get_ptr();
			m_allocator.template destruct_deallocate_type<link_t>(oldLink);
		}
		else
		{
			if (!!t)
				*t = oldLink->get();
		}
		return true;
	}

	template <bool fromEnd>
	volatile_pop_result remove_inner(type* t) volatile
	{
		bool result = false;
		bool wasLast = false;
		bool ownedRemoval = false;
		typename hazard::pointer hazardPointer;
		content_t newContents;
		content_t oldContents;
		for (;;)
		{
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
					*t = oldLink->get();
				if (ownedRemoval)
				{
					oldLink->m_isRemoved = true;
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

		return { result, wasLast };
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
				t = oldLink->get();
				if (hazardPointer.release())
					m_allocator.template destruct_deallocate_type<link_t>(oldLink);
				result = true;
			}
			break;
		}
		return result;
	}

	enum class insert_mode
	{
		normal = 0,
		only_if_empty = 1,
		only_if_not_empty = 2
	};

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(link_t* l, size_t n = 1)
	{
		l->m_remainingCount = n;
		ptr<link_t> oldLink = atEnd ? m_contents.m_tail : m_contents.m_head;
		bool wasEmpty = !oldLink;
		if (wasEmpty)
		{
			if constexpr (insertMode == insert_mode::only_if_not_empty)
				return true;

			m_contents.m_tail = m_contents.m_head = l;
		}
		else
		{
			if constexpr (insertMode == insert_mode::only_if_empty)
				return false;

			if (atEnd)
			{
				l->set_prev_link(oldLink);
				oldLink->set_next_link(l);
				m_contents.m_tail = l;
			}
			else
			{
				l->set_next_link(oldLink);
				oldLink->set_prev_link(l);
				m_contents.m_head = l;
			}
		}

		return wasEmpty;
	}

	template <bool atEnd, insert_mode insertMode>
	bool insert_inner(link_t* lnk, size_t n = 1) volatile
	{
		bool wasEmpty = false;
		ptr<link_t> l = lnk;
		l->m_remainingCount = n;
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

				newContents.m_head = newContents.m_tail = l.get_ptr();
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
				l->set_prev_link(oldContents.m_tail);
				newContents.m_tail = l.get_marked(1); // indicate an append is in progress.
				newContents.m_head = oldContents.m_head;
			}
			else
			{
				l->set_next_link(oldContents.m_head);
				newContents.m_head = l.get_marked(1); // indicate a prepend is in progress.
				newContents.m_tail = oldContents.m_tail;
			}
			done = atomic::compare_exchange(m_contents, newContents, oldContents);
			// continue
		}
		return wasEmpty;
	}

	container_deque(const container_deque& src) = delete;
	container_deque(const volatile container_deque& src) = delete;

	this_t& operator=(const container_deque& src) = delete;
	this_t& operator=(const volatile container_deque& src) = delete;

	volatile this_t& operator=(container_deque&& src) volatile = delete;
	volatile this_t& operator=(const container_deque& src) volatile = delete;
	volatile this_t& operator=(const volatile container_deque& src) volatile = delete;

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

	container_deque(const std::initializer_list<type>& src)
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
		for (auto& entry : src)
			append(entry);
	}

	container_deque(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_allocator(std::move(src.m_allocator))
	{
		m_contents.m_head = 0;
		m_contents.m_tail = 0;
		for (auto& entry : src)
			append(entry);
	}

	~container_deque() { clear_inner(); }

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

	this_t& operator=(const std::initializer_list<type>& src)
	{
		this_t tmp(src);
		*this = std::move(tmp);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.append(std::forward<args_t>(args)), ...);
		return result;
	}

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

	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F>
	bool prepend_via(F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l);
	}

	template <typename F>
	bool prepend_via(F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l);
	}

	bool prepend(const type& src) { return prepend_via([&](type& t) { new (&t) type(src); }); }
	bool prepend(type&& src) { return prepend_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend(F&& f) { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace(args_t&&... args) { return prepend_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool prepend(const type& src) volatile { return prepend_via([&](type& t) { new (&t) type(src); }); }
	bool prepend(type&& src) volatile { return prepend_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend(F&& f) volatile { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace(args_t&&... args) volatile { return prepend_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// Return true if list was empty, false if it was not
	template <typename F>
	bool append_via(F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l);
	}

	template <typename F>
	bool append_via(F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l);
	}

	bool append(const type& src) { return append_via([&](type& t) { new (&t) type(src); }); }
	bool append(type&& src) { return append_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) { return append_via(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) { return append_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append(const type& src) volatile { return append_via([&](type& t) { new (&t) type(src); }); }
	bool append(type&& src) volatile { return append_via([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) volatile { return append_via(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) volatile { return append_via([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// Returns true if added, false if not added
	template <typename F>
	bool prepend_via_if_not_empty(F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<false, insert_mode::only_if_not_empty>(l);
		return true;
	}

	template <typename F>
	bool prepend_via_if_not_empty(F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<false, insert_mode::only_if_not_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}


	bool prepend_if_not_empty(const type& src) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool prepend_if_not_empty(type&& src) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }
	template <typename... args_t> bool prepend_emplace_if_not_empty(args_t&&... args) { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_if_not_empty(F&& f) { return prepend_via_if_not_empty(std::forward<F>(f)); }

	bool prepend_if_not_empty(const type& src) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool prepend_if_not_empty(type&& src) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_if_not_empty(F&& f) volatile { return prepend_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_if_not_empty(args_t&&... args) volatile { return prepend_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Returns true if added, false if not added
	template <typename F>
	bool append_via_if_not_empty(F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_not_empty>(l);
		return true;
	}

	template <typename F>
	bool append_via_if_not_empty(F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<true, insert_mode::only_if_not_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}

	bool append_if_not_empty(const type& src) { return append_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool append_if_not_empty(type&& src) { return append_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) { return append_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append_if_not_empty(const type& src) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(src); }); }
	bool append_if_not_empty(type&& src) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) volatile { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) volatile { return append_via_if_not_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Return true if list was empty, false if it was not
	template <typename F>
	bool insert_via_if_empty(F&& f)
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_empty>(l);
		return true;
	}

	template <typename F>
	bool insert_via_if_empty(F&& f) volatile
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = insert_inner<true, insert_mode::only_if_empty>(l);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}

	bool insert_if_empty(const type& src) { return insert_via_if_empty([&](type& t) { new (&t) type(src); }); }
	bool insert_if_empty(type&& src) { return insert_via_if_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) { return insert_via_if_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool insert_if_empty(const type& src) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(src); }); }
	bool insert_if_empty(type&& src) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) volatile { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) volatile { return insert_via_if_empty([&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Return true if list was empty, false if it was not
	template <typename F>
	bool prepend_multiple_via(size_t n, F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l, n);
	}

	template <typename F>
	bool prepend_multiple_via(size_t n, F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<false, insert_mode::normal>(l, n);
	}

	bool prepend_multiple(size_t n, const type& src) { return prepend_multiple_via(n, [&](type& t) { new (&t) type(src); }); }
	bool prepend_multiple(size_t n, type&& src) { return prepend_multiple_via(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_multiple(size_t n, F&& f) { return prepend_multiple_via(n, std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_multiple(size_t n, args_t&&... args) { return prepend_multiple_via(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool prepend_multiple(size_t n, const type& src) volatile { return prepend_multiple_via(n, [&](type& t) { new (&t) type(src); }); }
	bool prepend_multiple(size_t n, type&& src) volatile { return prepend_multiple_via(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_multiple(size_t n, F&& f) volatile { return prepend_multiple_via(n, std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_multiple(size_t n, args_t&&... args) volatile { return prepend_multiple_via(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	template <typename F>
	bool append_multiple_via(size_t n, F&& f)
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l, n);
	}

	template <typename F>
	bool append_multiple_via(size_t n, F&& f) volatile
	{
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		return insert_inner<true, insert_mode::normal>(l, n);
	}

	bool append_multiple(size_t n, const type& src) { return append_multiple_via(n, [&](type& t) { new (&t) type(src); }); }
	bool append_multiple(size_t n, type&& src) { return append_multiple_via(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple(size_t n, F&& f) { return append_multiple_via(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple(size_t n, args_t&&... args) { return append_multiple_via(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append_multiple(size_t n, const type& src) volatile { return append_multiple_via(n, [&](type& t) { new (&t) type(src); }); }
	bool append_multiple(size_t n, type&& src) volatile { return append_multiple_via(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple(size_t n, F&& f) volatile { return append_multiple_via(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple(size_t n, args_t&&... args) volatile { return append_multiple_via(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	// Returns true if added, false if not added
	template <typename F>
	bool prepend_multiple_via_if_not_empty(size_t n, F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<false, insert_mode::only_if_not_empty>(l, n);
		return true;
	}

	template <typename F>
	bool prepend_multiple_via_if_not_empty(size_t n, F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<false, insert_mode::only_if_not_empty>(l, n);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}


	bool prepend_multiple_if_not_empty(size_t n, const type& src) { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool prepend_multiple_if_not_empty(size_t n, type&& src) { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_multiple_if_not_empty(size_t n, F&& f) { return prepend_multiple_via_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_multiple_if_not_empty(size_t n, args_t&&... args) { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool prepend_multiple_if_not_empty(size_t n, const type& src) volatile { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool prepend_multiple_if_not_empty(size_t n, type&& src) volatile { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	prepend_multiple_if_not_empty(size_t n, F&& f) volatile { return prepend_multiple_via_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool prepend_emplace_multiple_if_not_empty(size_t n, args_t&&... args) volatile { return prepend_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	template <typename F>
	bool append_multiple_via_if_not_empty(size_t n, F&& f)
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_not_empty>(l, n);
		return true;
	}

	template <typename F>
	bool append_multiple_via_if_not_empty(size_t n, F&& f) volatile
	{
		if (is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = !insert_inner<true, insert_mode::only_if_not_empty>(l, n);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}


	bool append_multiple_if_not_empty(size_t n, const type& src) { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool append_multiple_if_not_empty(size_t n, type&& src) { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple_if_not_empty(size_t n, F&& f) { return append_multiple_via_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple_if_not_empty(size_t n, args_t&&... args) { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool append_multiple_if_not_empty(size_t n, const type& src) volatile { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool append_multiple_if_not_empty(size_t n, type&& src) volatile { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple_if_not_empty(size_t n, F&& f) volatile { return append_multiple_via_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple_if_not_empty(size_t n, args_t&&... args) volatile { return append_multiple_via_if_not_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	// Return true if list was empty, false if it was not
	template <typename F>
	bool insert_multiple_via_if_empty(size_t n, F&& f)
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		insert_inner<true, insert_mode::only_if_empty>(l, n);
		return true;
	}

	template <typename F>
	bool insert_multiple_via_if_empty(size_t n, F&& f) volatile
	{
		if (!is_empty())
			return false;
		link_t* l = new (m_allocator.template allocate_type<link_t>()) link_t;
		f(l->get());
		bool b = insert_inner<true, insert_mode::only_if_empty>(l, n);
		if (!b)
			m_allocator.template destruct_deallocate_type<link_t>(l);
		return b;
	}

	bool insert_multiple_if_empty(size_t n, const type& src) { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool insert_multiple_if_empty(size_t n, type&& src) { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_multiple_if_empty(size_t n, F&& f) { return insert_multiple_via_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_multiple_if_empty(size_t n, args_t&&... args) { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }

	bool insert_multiple_if_empty(size_t n, const type& src) volatile { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(src); }); }
	bool insert_multiple_if_empty(size_t n, type&& src) volatile { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_multiple_if_empty(size_t n, F&& f) volatile { return insert_multiple_via_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_multiple_if_empty(size_t n, args_t&&... args) volatile { return insert_multiple_via_if_empty(n, [&](type& t) { new (&t) type(std::forward<args_t>(args)...); }); }


	bool peek_front(type& t) const { ptr<link_t> l = m_contents.m_head; if (!l) return false; t = l->get(); return true; }
	bool peek_front(type& t) const volatile { return peek_inner<false>(t); }

	bool peek_back(type& t) const { ptr<link_t> l = m_contents.m_tail; if (!l) return false; t = l->get(); return true; }
	bool peek_back(type& t) const volatile { return peek_inner<true>(t); }

	bool is_empty() const { return !m_contents.m_head; }
	bool is_empty() const volatile { link_t* l; atomic::load(m_contents.m_head, l); return !l; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	bool contains_one() const { return !!m_contents.m_head && m_contents.m_head == m_contents.m_tail; }
	bool contains_one() const volatile { content_t c; atomic::load(m_contents, c); return !!c.m_head && c.m_head == c.m_tail; }

	bool pop_front(type& t) { return remove_inner<false>(&t); }
	bool pop_back(type& t) { return remove_inner<true>(&t); }

	bool remove_front() { return remove_inner<false>(0); }
	bool remove_last() { return remove_inner<true>(0); }

	volatile_pop_result pop_front(type& t) volatile { return remove_inner<false>(&t); }
	volatile_pop_result pop_back(type& t) volatile { return remove_inner<true>(&t); }

	typedef volatile_pop_result volatile_remove_result;

	volatile_remove_result remove_front() volatile { return remove_inner<false>(0); }
	volatile_remove_result remove_last() volatile { return remove_inner<true>(0); }

	void swap(this_t& wth)
	{
		content_t tmp = m_contents;
		m_contents = wth.m_contents;
		wth.m_contents = tmp;
		m_allocator.swap(wth.m_allocator);
	}

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	void exchange(this_t&& src, this_t& rtn)
	{
		rtn = std::move(src);
		swap(rtn);
	}
};


}


#endif
