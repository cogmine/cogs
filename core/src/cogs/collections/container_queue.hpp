//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	typedef container_deque<type, false, allocator_type> deque_t;
	deque_t m_deque;

	container_queue(const container_queue& src) = delete;
	container_queue(const volatile container_queue& src) = delete;

	this_t& operator=(const container_queue& src) = delete;
	this_t& operator=(const volatile container_queue& src) = delete;

	volatile this_t& operator=(container_queue&& src) volatile = delete;
	volatile this_t& operator=(const container_queue& src) volatile = delete;
	volatile this_t& operator=(const volatile container_queue& src) volatile = delete;

public:
	container_queue() { }

	container_queue(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	explicit container_queue(volatile allocator_type& al)
		: m_deque(al)
	{ }

	container_queue(const std::initializer_list<type>& src)
		: m_deque(src)
	{ }

	container_queue(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_deque(al, src)
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	this_t& operator=(const std::initializer_list<type>& src)
	{
		deque_t tmp(src);
		m_deque = std::move(tmp.m_deque);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.append(std::forward<args_t>(args)), ...);
		return result;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F> bool append_via(F&& f) { return m_deque.append_via(std::forward<F>(f)); }
	template <typename F> bool append_via(F&& f) volatile { return m_deque.append_via(std::forward<F>(f)); }

	bool append(const type& src) { return m_deque.append(src); }
	bool append(type&& src) { return m_deque.append(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) { return m_deque.append(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) { return m_deque.append_emplace(std::forward<args_t>(args)...); }

	bool append(const type& src) volatile { return m_deque.append(src); }
	bool append(type&& src) volatile  { return m_deque.append(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) volatile { return m_deque.append(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) volatile  { return m_deque.append_emplace(std::forward<args_t>(args)...); }

	// Returns true if added, false if not added
	template <typename F> bool append_via_if_not_empty(F&& f) { return m_deque.append_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool append_via_if_not_empty(F&& f) volatile { return m_deque.append_via_if_not_empty(std::forward<F>(f)); }

	bool append_if_not_empty(const type& src) { return m_deque.append_if_not_empty(src); }
	bool append_if_not_empty(type&& src) { return m_deque.append_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) { return m_deque.append_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) { return m_deque.append_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool append_if_not_empty(const type& src) volatile { return m_deque.append_if_not_empty(src); }
	bool append_if_not_empty(type&& src) volatile { return m_deque.append_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) volatile { return m_deque.append_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.append_emplace_if_not_empty(std::forward<args_t>(args)...); }

	// Return true if list was empty, false if it was not
	template <typename F> bool insert_via_if_empty(F&& f) { return m_deque.insert_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool insert_via_if_empty(F&& f) volatile { return m_deque.insert_via_if_empty(std::forward<F>(f)); }

	bool insert_if_empty(const type& src) { return m_deque.insert_if_empty(src); }
	bool insert_if_empty(type&& src) { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

	bool insert_if_empty(const type& src) volatile { return m_deque.insert_if_empty(src); }
	bool insert_if_empty(type&& src) volatile { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) volatile { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

	// It is only valid to peek at a volatile element if you know it wont
	// be removed by another thread.  Doing so is caller error.  Only
	// call from a thread or context that is the only remover of elements.

	bool peek(type& t) const { return m_deque.peek_front(t); }
	bool peek(type& t) const volatile { return m_deque.peek_front(t); }

	bool is_empty() const { return m_deque.is_empty(); }
	bool is_empty() const volatile { return m_deque.is_empty(); }

	bool operator!() const { return !m_deque; }
	bool operator!() const volatile { return !m_deque; }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	bool pop(type& t) { return m_deque.pop_front(t); }
	bool remove() { return m_deque.remove_front(); }

	// first bool indicates whether an element was removed.  If any removed, the second bool indicates if it was the only element
	typedef typename deque_t::volatile_pop_result volatile_pop_result;

	volatile_pop_result pop(type& t) volatile { return m_deque.pop_front(t); }

	typedef typename deque_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove() volatile { return m_deque.remove_front(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }
	this_t exchange(this_t&& src) { return m_deque.exchange(std::move(src.m_deque)); }
	void exchange(this_t&& src, this_t& rtn) { return m_deque.exchange(std::move(src.m_deque), rtn.m_deque); }
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
	typedef container_deque<type, true, allocator_type> deque_t;
	deque_t m_deque;

	container_queue(const container_queue& src) = delete;
	container_queue(const volatile container_queue& src) = delete;

	this_t& operator=(const container_queue& src) = delete;
	this_t& operator=(const volatile container_queue& src) = delete;

	volatile this_t& operator=(container_queue&& src) volatile = delete;
	volatile this_t& operator=(const container_queue& src) volatile = delete;
	volatile this_t& operator=(const volatile container_queue& src) volatile = delete;

public:
	container_queue() { }

	container_queue(this_t&& src)
		: m_deque(std::move(src.m_deque))
	{ }

	explicit container_queue(volatile allocator_type& al)
		: m_deque(al)
	{ }

	container_queue(const std::initializer_list<type>& src)
		: m_deque(src)
	{ }

	container_queue(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_deque(al, src)
	{ }

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	this_t& operator=(const std::initializer_list<type>& src)
	{
		deque_t tmp(src);
		m_deque = std::move(tmp.m_deque);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.append(std::forward<args_t>(args)), ...);
		return result;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F> bool append_via(F&& f) { return m_deque.append_via(std::forward<F>(f)); }
	template <typename F> bool append_via(F&& f) volatile { return m_deque.append_via(std::forward<F>(f)); }

	bool append(const type& src) { return m_deque.append(src); }
	bool append(type&& src) { return m_deque.append(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) { return m_deque.append(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) { return m_deque.append_emplace(std::forward<args_t>(args)...); }

	bool append(const type& src) volatile { return m_deque.append(src); }
	bool append(type&& src) volatile { return m_deque.append(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append(F&& f) volatile { return m_deque.append(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace(args_t&&... args) volatile { return m_deque.append_emplace(std::forward<args_t>(args)...); }

	// Returns true if added, false if not added
	template <typename F> bool append_via_if_not_empty(F&& f) { return m_deque.append_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool append_via_if_not_empty(F&& f) volatile { return m_deque.append_via_if_not_empty(std::forward<F>(f)); }

	bool append_if_not_empty(const type& src) { return m_deque.append_if_not_empty(src); }
	bool append_if_not_empty(type&& src) { return m_deque.append_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) { return m_deque.append_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) { return m_deque.append_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool append_if_not_empty(const type& src) volatile { return m_deque.append_if_not_empty(src); }
	bool append_if_not_empty(type&& src) volatile { return m_deque.append_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_if_not_empty(F&& f) volatile { return m_deque.append_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.append_emplace_if_not_empty(std::forward<args_t>(args)...); }


	// Return true if list was empty, false if it was not
	template <typename F> bool insert_via_if_empty(F&& f) { return m_deque.insert_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool insert_via_if_empty(F&& f) volatile { return m_deque.insert_via_if_empty(std::forward<F>(f)); }

	bool insert_if_empty(const type& src) { return m_deque.insert_if_empty(src); }
	bool insert_if_empty(type&& src) { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

	bool insert_if_empty(const type& src) volatile { return m_deque.insert_if_empty(src); }
	bool insert_if_empty(type&& src) volatile { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_if_empty(F&& f) volatile { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool insert_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }


	// Return true if list was empty, false if it was not
	template <typename F> bool append_multiple_via(size_t n, F&& f) { return m_deque.append_multiple_via(n, std::forward<F>(f)); }
	template <typename F> bool append_multiple_via(size_t n, F&& f) volatile { return m_deque.append_multiple_via(n, std::forward<F>(f)); }

	bool append_multiple(size_t n, const type& src) { return m_deque.append_multiple(n, src); }
	bool append_multiple(size_t n, type&& src) { return m_deque.append_multiple(n, src); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple(size_t n, F&& f) { return m_deque.append_multiple(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple(size_t n, args_t&&... args) { return m_deque.append_emplace_multiple(n, std::forward<args_t>(args)...); }


	bool append_multiple(size_t n, const type& src) volatile { return append_multiple(n, src); }
	bool append_multiple(size_t n, type&& src) volatile { return m_deque.append_multiple(n, src); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple(size_t n, F&& f) volatile { return m_deque.append_multiple(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_emplace_multiple(size_t n, args_t&&... args) volatile { return m_deque.append_emplace_multiple(n, std::forward<args_t>(args)...); }


	// Returns true if added, false if not added
	template <typename F> bool append_multiple_via_if_not_empty(F&& f) { return m_deque.append_multiple_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool append_multiple_via_if_not_empty(F&& f) volatile { return m_deque.append_multiple_via_if_not_empty(std::forward<F>(f)); }

	bool append_multiple_if_not_empty(const type& src) { return m_deque.append_multiple_if_not_empty(src); }
	bool append_multiple_if_not_empty(type&& src) { return m_deque.append_multiple_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple_if_not_empty(size_t n, F&& f) { return m_deque.append_multiple_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_multiple_emplace_if_not_empty(args_t&&... args) { return m_deque.append_multiple_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool append_multiple_if_not_empty(const type& src) volatile { return m_deque.append_multiple_if_not_empty(src); }
	bool append_multiple_if_not_empty(type&& src) volatile { return m_deque.append_multiple_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	append_multiple_if_not_empty(size_t n, F&& f) volatile { return m_deque.append_multiple_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool append_multiple_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.append_multiple_emplace_if_not_empty(std::forward<args_t>(args)...); }

	// Return true if list was empty, false if it was not
	template <typename F> bool insert_multiple_via_if_empty(F&& f) { return m_deque.insert_multiple_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool insert_multiple_via_if_empty(F&& f) volatile { return m_deque.insert_multiple_via_if_empty(std::forward<F>(f)); }

	bool insert_multiple_if_empty(const type& src) { return m_deque.insert_multiple_if_empty(src); }
	bool insert_multiple_if_empty(type&& src) { return m_deque.insert_multiple_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_multiple_if_empty(size_t n, F&& f) { return m_deque.insert_multiple_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool insert_multiple_emplace_if_empty(args_t&&... args) { return m_deque.insert_multiple_emplace_if_empty(std::forward<args_t>(args)...); }

	bool insert_multiple_if_empty(const type& src) volatile { return m_deque.insert_multiple_if_empty(src); }
	bool insert_multiple_if_empty(type&& src) volatile { return m_deque.insert_multiple_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	insert_multiple_if_empty(size_t n, F&& f) volatile { return m_deque.insert_multiple_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool insert_multiple_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_multiple_emplace_if_empty(std::forward<args_t>(args)...); }

	// It is only valid to peek at a volatile element if you know it wont
	// be removed by another thread.  Doing so is caller error.  Only
	// call from a thread or context that is the only remover of elements.

	bool peek(type& t) const { return m_deque.peek(t); }
	bool peek(type& t) const volatile { return m_deque.peek(t); }

	bool is_empty() const { return m_deque.is_empty(); }
	bool is_empty() const volatile { return m_deque.is_empty(); }

	bool operator!() const { return !m_deque; }
	bool operator!() const volatile { return !m_deque; }

	bool contains_one() const { return m_deque.contains_one(); }
	bool contains_one() const volatile { return m_deque.contains_one(); }

	bool pop(type& t) { return m_deque.pop_front(t); }
	bool remove() { return m_deque.remove_front(); }

	// first bool indicates whether an element was removed.  If any removed, the second bool indicates if it was the only element
	typedef typename deque_t::volatile_pop_result volatile_pop_result;

	volatile_pop_result pop(type& t) volatile { return m_deque.pop_front(t); }

	typedef typename deque_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove() volatile { return m_deque.remove_front(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }
	this_t exchange(this_t&& src) { m_deque.exchange(std::move(src.m_deque)); }
	void exchange(this_t&& src, this_t& rtn) { return m_deque.exchange(std::move(src.m_deque), rtn.m_deque); }
};


}


#endif
