//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_QUEUE
#define COGS_HEADER_COLLECTION_CONTAINER_QUEUE


#include "cogs/collections/container_deque.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief A queue container collection
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: false
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = false, class allocator_type = default_allocator>
class container_queue
{
public:
	typedef T type;
	typedef container_queue<type, false, allocator_type> this_t;

private:
	// A container_deque<>, really.  Retricted to the capabilities of a queue
	// only for the purpose of type safety.
	container_deque<type, false, allocator_type> m_deque;

	container_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	container_queue() { }

	container_queue(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	explicit container_queue(volatile allocator_type& al)
		: m_deque(al)
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	// Return true if list was empty, false if it was not
	bool append(const type& t) { return m_deque.append(t); }
	bool append(const type& t) volatile { return m_deque.append(t); }

	// Returns true if added, false if not added
	bool append_if_not_empty(const type& t) { return m_deque.append_if_not_empty(t); }
	bool append_if_not_empty(const type& t) volatile { return m_deque.append_if_not_empty(t); }
	bool append_if_empty(const type& t) { return m_deque.insert_if_empty(t); }
	bool append_if_empty(const type& t) volatile { return m_deque.insert_if_empty(t); }

	bool peek(type& t) const { return m_deque.peek_first(t); }

	// It is only valid to peek at a volatile element if you know it wont
	// be removed by another thread.  Doing so is caller error.  Only
	// call from a thread or context that is the only remover of elements.
	bool peek(type& t) const volatile { return m_deque.peek_first(t); }

	bool peek_first(type& t) const { return m_deque.peek_first(t); }
	bool peek_first(type& t) const volatile { return m_deque.peek_first(t); }

	bool pop(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t) { return m_deque.pop_first(t); }
	bool pop(type& t) volatile { return m_deque.pop_first(t); }

	bool pop_first(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop_first(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
	bool pop_first(type& t) { return m_deque.pop_first(t); }
	bool pop_first(type& t) volatile { return m_deque.pop_first(t); }

	bool remove(bool& wasLast) { return m_deque.remove_first(wasLast); }
	bool remove(bool& wasLast) volatile { return m_deque.remove_first(wasLast); }
	bool remove() { return m_deque.remove_first(); }
	bool remove() volatile { return m_deque.remove_first(); }

	bool remove_first(bool& wasLast) { return m_deque.remove_first(wasLast); }
	bool remove_first(bool& wasLast) volatile { return m_deque.remove_first(wasLast); }
	bool remove_first() { return m_deque.remove_first(); }
	bool remove_first() volatile { return m_deque.remove_first(); }

	bool is_empty() const { return m_deque.is_empty(); }
	bool is_empty() const volatile { return m_deque.is_empty(); }

	bool operator!() const { return m_deque.is_empty(); }
	bool operator!() const volatile { return m_deque.is_empty(); }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile { m_deque.swap(wth.m_deque); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth) { m_deque.swap(wth.m_deque); }

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
class container_queue<T, true, allocator_type>
{
public:
	typedef T type;
	typedef container_queue<type, true, allocator_type> this_t;

private:
	// A container_deque<>, really.  Retricted to the capabilities of a queue
	// only for the purpose of type safety.
	container_deque<type, true, allocator_type> m_deque;

	container_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	container_queue() { }

	container_queue(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	explicit container_queue(volatile allocator_type& al)
		: m_deque(al)
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	// Return true if list was empty, false if it was not
	bool append(const type& t, size_t n = 1) { return m_deque.append(t, n); }
	bool append(const type& t, size_t n = 1) volatile { return m_deque.append(t, n); }

	// Returns true if added, false if not added
	bool append_if_not_empty(const type& t, size_t n = 1) { return m_deque.append_if_not_empty(t, n); }
	bool append_if_not_empty(const type& t, size_t n = 1) volatile { return m_deque.append_if_not_empty(t, n); }
	bool append_if_empty(const type& t, size_t n = 1) { return m_deque.insert_if_empty(t, n); }
	bool append_if_empty(const type& t, size_t n = 1) volatile { return m_deque.insert_if_empty(t, n); }


	bool peek(type& t) const { return m_deque.peek_first(t); }

	// It is only valid to peek at a volatile element if you know it wont
	// be removed by another thread.  Doing so is caller error.  Only
	// call from a thread or context that is the only remover of elements.
	bool peek(type& t) const volatile { return m_deque.peek_first(t); }

	bool peek_first(type& t) const { return m_deque.peek_first(t); }
	bool peek_first(type& t) const volatile { return m_deque.peek_first(t); }

	bool pop(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
	bool pop(type& t) { return m_deque.pop_first(t); }
	bool pop(type& t) volatile { return m_deque.pop_first(t); }

	bool pop_first(type& t, bool& wasLast) { return m_deque.pop_first(t, wasLast); }
	bool pop_first(type& t, bool& wasLast) volatile { return m_deque.pop_first(t, wasLast); }
	bool pop_first(type& t) { return m_deque.pop_first(t); }
	bool pop_first(type& t) volatile { return m_deque.pop_first(t); }

	bool remove(bool& wasLast) { return m_deque.remove_first(wasLast); }
	bool remove(bool& wasLast) volatile { return m_deque.remove_first(wasLast); }
	bool remove() { return m_deque.remove_first(); }
	bool remove() volatile { return m_deque.remove_first(); }

	bool remove_first(bool& wasLast) { return m_deque.remove_first(wasLast); }
	bool remove_first(bool& wasLast) volatile { return m_deque.remove_first(wasLast); }
	bool remove_first() { return m_deque.remove_first(); }
	bool remove_first() volatile { return m_deque.remove_first(); }

	bool is_empty() const { return m_deque.is_empty(); }
	bool is_empty() const volatile { return m_deque.is_empty(); }

	bool operator!() const { return m_deque.is_empty(); }
	bool operator!() const volatile { return m_deque.is_empty(); }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile { m_deque.swap(wth.m_deque); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth) { m_deque.swap(wth.m_deque); }

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


#endif
