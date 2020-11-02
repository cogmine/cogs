//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_STACK
#define COGS_HEADER_COLLECTION_CONTAINER_STACK


#include "cogs/collections/container_deque.hpp"
#include "cogs/mem/default_allocator.hpp"


namespace cogs {


template <typename T, bool coalesc_equal = false>
using container_stack_node = container_deque_node<T, coalesc_equal>;


/// @ingroup LockFreeCollections
/// @brief A stack container collection
/// @tparam T type to contain
/// @tparam coalesc_equal If true, contiguous equal elements may be coalesced.  Default: false
/// @tparam allocator_type Type of allocator to use to allocate elements.  Default: default_allocator
template <typename T, bool coalesc_equal = false, class allocator_t = batch_allocator<container_queue_node<T, coalesc_equal>>>
class container_stack
{
public:
	typedef T type;
	typedef allocator_t allocator_type;
	typedef container_stack<type, false, allocator_type> this_t;

private:
	typedef container_deque<type, false, allocator_type> deque_t;
	deque_t m_deque;

public:
	template <typename... args_t>
	container_stack(args_t&&... args)
		: m_deque(std::forward<args_t>(args)...)
	{ }

	container_stack(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		m_deque = src.m_deque;
		return *this;
	}

	this_t& operator=(const this_t&) volatile = delete;
	this_t& operator=(const volatile this_t&) = delete;
	volatile this_t& operator=(const volatile this_t&) volatile = delete;

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }

	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F> bool push_via(F&& f) { return m_deque.prepend_via(std::forward<F>(f)); }
	template <typename F> bool push_via(F&& f) volatile { return m_deque.prepend_via(std::forward<F>(f)); }

	bool push(const type& src) { return m_deque.prepend(src); }
	bool push(type&& src) { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push(F&& f) { return m_deque.prepend(std::forward<F>(f)); }

	bool operator+=(const type& src) { return m_deque.prepend(src); }
	bool operator+=(type&& src) { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	operator+=(F&& f) { return m_deque.prepend(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace(args_t&&... args) { return m_deque.prepend_emplace(std::forward<args_t>(args)...); }

	bool push(const type& src) volatile { return m_deque.prepend(src); }
	bool push(type&& src) volatile { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push(F&& f) volatile { return m_deque.prepend(std::forward<F>(f)); }

	bool operator+=(const type& src) volatile { return m_deque.prepend(src); }
	bool operator+=(type&& src) volatile { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	operator+=(F&& f) volatile { return m_deque.prepend(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace(args_t&&... args) volatile { return m_deque.prepend_emplace(std::forward<args_t>(args)...); }

	// Returns true if added, false if not added
	template <typename F> bool push_via_if_not_empty(F&& f) { return m_deque.prepend_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool push_via_if_not_empty(F&& f) volatile { return m_deque.prepend_via_if_not_empty(std::forward<F>(f)); }

	bool push_if_not_empty(const type& src) { return m_deque.prepend_if_not_empty(src); }
	bool push_if_not_empty(type&& src) { return m_deque.prepend_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_not_empty(F&& f) { return m_deque.prepend_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_not_empty(args_t&&... args) { return m_deque.prepend_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool push_if_not_empty(const type& src) volatile { return m_deque.prepend_if_not_empty(src); }
	bool push_if_not_empty(type&& src) volatile { return m_deque.prepend_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_not_empty(F&& f) volatile { return m_deque.prepend_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.prepend_emplace_if_not_empty(std::forward<args_t>(args)...); }

	// Return true if list was empty, false if it was not
	template <typename F> bool push_via_if_empty(F&& f) { return m_deque.insert_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool push_via_if_empty(F&& f) volatile { return m_deque.insert_via_if_empty(std::forward<F>(f)); }

	bool push_if_empty(const type& src) { return m_deque.insert_if_empty(src); }
	bool push_if_empty(type&& src) { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_empty(F&& f) { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_empty(args_t&&... args) { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

	bool push_if_empty(const type& src) volatile { return m_deque.insert_if_empty(src); }
	bool push_if_empty(type&& src) volatile { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_empty(F&& f) volatile { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

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
	bool remove() { return m_deque.remove_first(); }

	// first bool indicates whether an element was removed.  If any removed, the second bool indicates if it was the only element
	typedef typename deque_t::volatile_pop_result volatile_pop_result;

	volatile_pop_result pop(type& t) volatile { return m_deque.pop_front(t); }

	typedef typename deque_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove() volatile { return m_deque.remove_first(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }
	this_t exchange(this_t&& src) { return m_deque.exchange(std::move(src.m_deque)); }
	void exchange(this_t&& src, this_t& rtn) { return m_deque.exchange(std::move(src.m_deque), rtn.m_deque); }
};


template <typename T, class allocator_type>
class container_stack<T, true, allocator_type>
{
public:
	typedef T type;
	typedef container_stack<type, true, allocator_type> this_t;

private:
	typedef container_deque<type, true, allocator_type> deque_t;
	deque_t m_deque;

public:
	template <typename... args_t>
	container_stack(args_t&&... args)
		: m_deque(std::forward<args_t>(args)...)
	{ }

	container_stack(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		m_deque = src.m_deque;
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	volatile this_t& operator=(const this_t& src) volatile = delete;
	volatile this_t& operator=(const volatile this_t&) volatile = delete;

	this_t& operator=(this_t&& src)
	{
		m_deque = std::move(src.m_deque);
		return *this;
	}

	void clear() { m_deque.clear(); }
	void clear() volatile { m_deque.clear(); }


	// lamba is used passed referenced to unconstructed object,
	// and is required to construct it.

	// Return true if list was empty, false if it was not
	template <typename F> bool push_via(F&& f) { return m_deque.prepend_via(std::forward<F>(f)); }
	template <typename F> bool push_via(F&& f) volatile { return m_deque.prepend_via(std::forward<F>(f)); }

	bool push(const type& src) { return m_deque.prepend(src); }
	bool push(type&& src) { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push(F&& f) { return m_deque.prepend(std::forward<F>(f)); }

	bool operator+=(const type& src) { return m_deque.prepend(src); }
	bool operator+=(type&& src) { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	operator+=(F&& f) { return m_deque.prepend(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace(args_t&&... args) { return m_deque.prepend_emplace(std::forward<args_t>(args)...); }

	bool push(const type& src) volatile { return m_deque.prepend(src); }
	bool push(type&& src) volatile { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push(F&& f) volatile { return m_deque.prepend(std::forward<F>(f)); }

	bool operator+=(const type& src) volatile { return m_deque.prepend(src); }
	bool operator+=(type&& src) volatile { return m_deque.prepend(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	operator+=(F&& f) volatile { return m_deque.prepend(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace(args_t&&... args) volatile { return m_deque.prepend_emplace(std::forward<args_t>(args)...); }

	// Returns true if added, false if not added
	template <typename F> bool push_via_if_not_empty(F&& f) { return m_deque.prepend_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool push_via_if_not_empty(F&& f) volatile { return m_deque.prepend_via_if_not_empty(std::forward<F>(f)); }

	bool push_if_not_empty(const type& src) { return m_deque.prepend_if_not_empty(src); }
	bool push_if_not_empty(type&& src) { return m_deque.prepend_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_not_empty(F&& f) { return m_deque.prepend_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_not_empty(args_t&&... args) { return m_deque.prepend_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool push_if_not_empty(const type& src) volatile { return m_deque.prepend_if_not_empty(src); }
	bool push_if_not_empty(type&& src) volatile { return m_deque.prepend_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_not_empty(F&& f) volatile { return m_deque.prepend_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.prepend_emplace_if_not_empty(std::forward<args_t>(args)...); }


	// Return true if list was empty, false if it was not
	template <typename F> bool push_via_if_empty(F&& f) { return m_deque.insert_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool push_via_if_empty(F&& f) volatile { return m_deque.insert_via_if_empty(std::forward<F>(f)); }

	bool push_if_empty(const type& src) { return m_deque.insert_if_empty(src); }
	bool push_if_empty(type&& src) { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_empty(F&& f) { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_empty(args_t&&... args) { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }

	bool push_if_empty(const type& src) volatile { return m_deque.insert_if_empty(src); }
	bool push_if_empty(type&& src) volatile { return m_deque.insert_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_if_empty(F&& f) volatile { return m_deque.insert_if_empty(std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_emplace_if_empty(std::forward<args_t>(args)...); }


	// Return true if list was empty, false if it was not
	template <typename F> bool push_multiple_via(size_t n, F&& f) { return m_deque.prepend_multiple_via(n, std::forward<F>(f)); }
	template <typename F> bool push_multiple_via(size_t n, F&& f) volatile { return m_deque.prepend_multiple_via(n, std::forward<F>(f)); }

	bool push_multiple(size_t n, const type& src) { return m_deque.prepend_multiple(n, src); }
	bool push_multiple(size_t n, type&& src) { return m_deque.prepend_multiple(n, src); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple(size_t n, F&& f) { return m_deque.prepend_multiple(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_multiple(size_t n, args_t&&... args) { return m_deque.prepend_emplace_multiple(n, std::forward<args_t>(args)...); }


	bool push_multiple(size_t n, const type& src) volatile { return prepend_multiple(n, src); }
	bool push_multiple(size_t n, type&& src) volatile { return m_deque.prepend_multiple(n, src); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple(size_t n, F&& f) volatile { return m_deque.prepend_multiple(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_emplace_multiple(size_t n, args_t&&... args) volatile { return m_deque.prepend_emplace_multiple(n, std::forward<args_t>(args)...); }


	// Returns true if added, false if not added
	template <typename F> bool push_multiple_via_if_not_empty(F&& f) { return m_deque.prepend_multiple_via_if_not_empty(std::forward<F>(f)); }
	template <typename F> bool push_multiple_via_if_not_empty(F&& f) volatile { return m_deque.prepend_multiple_via_if_not_empty(std::forward<F>(f)); }

	bool push_multiple_if_not_empty(const type& src) { return m_deque.prepend_multiple_if_not_empty(src); }
	bool push_multiple_if_not_empty(type&& src) { return m_deque.prepend_multiple_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple_if_not_empty(size_t n, F&& f) { return m_deque.prepend_multiple_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_multiple_emplace_if_not_empty(args_t&&... args) { return m_deque.prepend_multiple_emplace_if_not_empty(std::forward<args_t>(args)...); }

	bool push_multiple_if_not_empty(const type& src) volatile { return m_deque.prepend_multiple_if_not_empty(src); }
	bool push_multiple_if_not_empty(type&& src) volatile { return m_deque.prepend_multiple_if_not_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple_if_not_empty(size_t n, F&& f) volatile { return m_deque.prepend_multiple_if_not_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_multiple_emplace_if_not_empty(args_t&&... args) volatile { return m_deque.prepend_multiple_emplace_if_not_empty(std::forward<args_t>(args)...); }

	// Return true if list was empty, false if it was not
	template <typename F> bool push_multiple_via_if_empty(F&& f) { return m_deque.insert_multiple_via_if_empty(std::forward<F>(f)); }
	template <typename F> bool push_multiple_via_if_empty(F&& f) volatile { return m_deque.insert_multiple_via_if_empty(std::forward<F>(f)); }

	bool push_multiple_if_empty(const type& src) { return m_deque.insert_multiple_if_empty(src); }
	bool push_multiple_if_empty(type&& src) { return m_deque.insert_multiple_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple_if_empty(size_t n, F&& f) { return m_deque.insert_multiple_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_multiple_emplace_if_empty(args_t&&... args) { return m_deque.insert_multiple_emplace_if_empty(std::forward<args_t>(args)...); }

	bool push_multiple_if_empty(const type& src) volatile { return m_deque.insert_multiple_if_empty(src); }
	bool push_multiple_if_empty(type&& src) volatile { return m_deque.insert_multiple_if_empty(std::move(src)); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		bool>
	push_multiple_if_empty(size_t n, F&& f) volatile { return m_deque.insert_multiple_if_empty(n, std::forward<F>(f)); }

	template <typename... args_t> bool push_multiple_emplace_if_empty(args_t&&... args) volatile { return m_deque.insert_multiple_emplace_if_empty(std::forward<args_t>(args)...); }

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
	bool remove() { return m_deque.remove_first(); }

	// first bool indicates whether an element was removed.  If any removed, the second bool indicates if it was the only element
	typedef typename deque_t::volatile_pop_result volatile_pop_result;

	volatile_pop_result pop(type& t) volatile { return m_deque.pop_front(t); }

	typedef typename deque_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove() volatile { return m_deque.remove_first(); }

	void swap(this_t& wth) { m_deque.swap(wth.m_deque); }
	this_t exchange(this_t&& src) { m_deque.exchange(std::move(src.m_deque)); }
	void exchange(this_t&& src, this_t& rtn) { return m_deque.exchange(std::move(src.m_deque), rtn.m_deque); }
};


}


#endif
