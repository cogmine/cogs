//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_STACK
#define COGS_HEADER_COLLECTION_CONTAINER_STACK


#include "cogs/collections/container_deque.hpp"
#include "cogs/mem/default_allocator.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief A stack container collection
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: false
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = false, class allocator_type = default_allocator>
class container_stack
{
public:
	typedef T type;
	typedef container_stack<type, false, allocator_type> this_t;

private:
	container_deque<type, false, allocator_type> m_deque;

	container_stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	container_stack() { }

	container_stack(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	explicit container_stack(volatile allocator_type& al)
		: m_deque(al)
	{ }

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	bool operator!() const { return !m_deque; }
	bool operator!() const volatile { return !m_deque; }

	bool is_empty() const { return !m_deque; }
	bool is_empty() const volatile { return !m_deque; }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	bool peek(type& t) const { return m_deque.peek_first(t); }
	bool peek(type& t) const volatile { return m_deque.peek_first(t); }

	// Return true if list was empty, false if it was not
	bool push(const type& t) { return m_deque.prepend(t); }
	bool push(const type& t) volatile { return m_deque.prepend(t); }

	// Returns true if added, false if not added
	bool push_if_not_empty(const type& t) { return m_deque.prepend_if_not_empty(t); }
	bool push_if_not_empty(const type& t) volatile { return m_deque.prepend_if_not_empty(t); }
	bool push_if_empty(const type& t) { return m_deque.insert_if_empty(t); }
	bool push_if_empty(const type& t) volatile { return m_deque.insert_if_empty(t); }

	bool pop(type& t) { return m_deque.pop_first(t); }
	bool pop(type& t) volatile { return m_deque.pop_first(t); }

	bool pop(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
};


template <typename T, class allocator_type>
class container_stack<T, true, allocator_type>
{
public:
	typedef T type;
	typedef container_stack<type, true, allocator_type> this_t;

private:
	container_deque<type, true, allocator_type> m_deque;

	container_stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	container_stack() { }

	container_stack(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	explicit container_stack(volatile allocator_type& al)
		: m_deque(al)
	{ }

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	bool operator!() const { return !m_deque; }
	bool operator!() const volatile { return !m_deque; }

	bool is_empty() const { return !m_deque; }
	bool is_empty() const volatile { return !m_deque; }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	bool peek(type& t) const { return m_deque.peek_first(t); }
	bool peek(type& t) const volatile { return m_deque.peek_first(t); }

	// Return true if list was empty, false if it was not
	bool push(const type& t, size_t n = 1) { return m_deque.prepend(t, n); }
	bool push(const type& t, size_t n = 1) volatile { return m_deque.prepend(t, n); }

	// Returns true if added, false if not added
	bool push_if_not_empty(const type& t, size_t n = 1) { return m_deque.prepend_if_not_empty(t, n); }
	bool push_if_not_empty(const type& t, size_t n = 1) volatile { return m_deque.prepend_if_not_empty(t, n); }
	bool push_if_empty(const type& t, size_t n = 1) { return m_deque.insert_if_empty(t, n); }
	bool push_if_empty(const type& t, size_t n = 1) volatile { return m_deque.insert_if_empty(t, n); }

	bool pop(type& t) { return m_deque.pop_first(t); }
	bool pop(type& t) volatile { return m_deque.pop_first(t); }

	bool pop(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
};


}


#endif
