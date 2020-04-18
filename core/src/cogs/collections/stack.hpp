//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_STACK
#define COGS_HEADER_COLLECTION_STACK


#include "cogs/collections/slink.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief Lock-free intrusive stack that is not vulnerable to <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a>.
///
/// For a lock-free instrusive stack that is vulnerable to <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a>,
/// use no_aba_stack.  no_aba_stack is slightly more efficient than cogs::stack when usage patterns gaurantee
/// <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a> will not arise,
/// such as when an element will never be added twice.  Neither no_aba_stack or cogs::stack protect
/// against hazardous (posthumous) access to an element, so elements must remain in
/// scope beyond any potential parallel access.  Managing hazardous access requires
/// managing the scope of the element, so must be done by the caller of any intrusive lock-free collection.
///
/// Unlike no_aba_stack, the ABA solution employed by cogs::stack requires elements are referred to only by ptr (not rcptr, etc.).
///
/// @tparam T Intrusive single-link element type.  Default: slink
/// @tparam link_accessor Helper type providing functions to get and set the next link.  Default: default_slink_accessor\<T\>
template <class T = slink, class link_accessor = default_slink_accessor<T> >
class stack
{
public:
	/// @brief Alias to this type.
	typedef stack<T, link_accessor> this_t;

	/// @brief Alias to the link type.
	typedef T link_t;

private:
	typedef versioned_ptr<link_t> head_ref_t;
	typedef typename head_ref_t::version_t version_t;

	head_ref_t m_head;

	static const ptr<link_t>& get_next(const link_t& l) { return link_accessor::get_next(l); }

	static void set_next(link_t& l, ptr<link_t> src) { link_accessor::set_next(l, src); }

	stack(ptr<link_t> setTo)
		: m_head(setTo)
	{ }

	stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	stack()
		: m_head(0)
	{ }

	stack(this_t&& src)
		: m_head(std::move(src.m_head))
	{
	}

	this_t& operator=(this_t&& src)
	{
		m_head = std::move(src.m_head);
		return *this;
	}

	size_t count() const
	{
		size_t n = 0;
		link_t* l = m_head.get();
		while (!!l)
		{
			++n;
			l = get_next(*l);
		}
		return n;
	}

	/// @{
	/// @brief Peek at the element at the head of the stack, without removing it.
	/// @return An element reference pointing to the head element.
	link_t* peek() const { return m_head.get_ptr(); }
	/// @brief Thread-safe implementation of peek().
	link_t* peek() const volatile { return m_head.get_ptr(); }
	/// @}

	/// @{
	/// @brief Checks if the stack is empty.
	/// @return A value indicating whether the stack is empty.
	bool is_empty() const { return !m_head; }
	/// @brief Thread-safe implementation of is_empty().
	bool is_empty() const volatile { return !m_head; }
	/// @}

	/// @{
	/// @brief Checks if the stack is empty.  An alias for is_empty().
	/// @return A value indicating whether the stack is empty.
	bool operator!() const { return is_empty(); }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return is_empty(); }
	/// @}

	/// @{
	/// @brief Adds the specified element to the top of the stack.
	/// @param e Element to add to the top of the stack.
	/// @return True if the stack had previously been empty, false if at least 1 other element was also present.
	bool push(link_t& e)
	{
		link_t* l = m_head.get();
		set_next(e, l);
		m_head = &e;
		return !l;
	}

	/// @brief Thread-safe implementation of push().
	bool push(link_t& e) volatile
	{
		bool wasEmpty;
		link_t* oldHead;
		version_t v;
		m_head.get(oldHead, v);
		do {
			wasEmpty = !oldHead;
			set_next(e, oldHead);
		} while (!m_head.versioned_exchange(&e, v, oldHead));
		return wasEmpty;
	}
	/// @}

	/// @{
	/// @brief Removes the element from the top of the stack.
	/// @param[out] wasLast If specified, receives a value indicating whether the removed element
	///                     was the last remaining element in the stack at the time.
	/// @return An element reference, or null if the stack was empty.
	link_t* pop(bool* wasLast = 0)
	{
		bool b = false;
		link_t* sl = m_head.get();
		if (!!sl)
		{
			link_t* next = get_next(*sl);
			m_head = next;
			b = (!next);
		}
		if (!!wasLast)
			*wasLast = b;
		return sl;
	}

	/// @brief Thread-safe implementation of pop().
	link_t* pop(bool* wasLast = 0) volatile
	{
		ptr<link_t> newHead;
		ptr<link_t> oldHead;
		version_t v;
		m_head.get(oldHead, v);
		do {
			if (!oldHead)
				break;
			newHead = get_next(*oldHead);
		} while (!m_head.versioned_exchange(newHead, v, oldHead));
		if (!!wasLast)
			*wasLast = (!!oldHead) && (!get_next(*oldHead));
		return oldHead.get_ptr();
	}
	/// @}

	/// @{
	/// @brief Removes the specified element if it is currently at the head of the stack.
	/// @param e Element to remove, if at the head of the stack.
	/// @param[out] oldHead If specified, receives a reference to the element at the head of the stack,
	///                     regardless of whether or not removal is successful.
	/// @param[out] wasLast If specified, receives a value indicating whether the removed element
	///                     was the last remaining elements in the stack at the time.
	/// @return An element reference, or null if the specified element was not at the head of the stack.
	link_t* pop_if_equals(link_t& e, link_t*& oldHead, bool* wasLast = 0)
	{
		oldHead = m_head.get();
		return pop_if_equals(e, wasLast);
	}

	/// @brief Removes the specified element if it is currently at the head of the stack.
	/// @param e Element to remove, if at the head of the stack.
	/// @param[out] wasLast If specified, receives a value indicating whether the removed element
	///                     was the last remaining elements in the stack at the time.
	/// @return An element reference, or null if the specified element was not at the head of the stack.
	link_t* pop_if_equals(link_t& e, bool* wasLast = 0)
	{
		link_t* sl = 0;
		if (m_head == &e)
		{
			sl = m_head.get();
			if (!!sl)
				m_head = get_next(*sl);
		}
		if (!!wasLast)
			*wasLast = (!!sl) && (!get_next(*sl));
		return sl;
	}

	/// @brief Thread-safe implementation of pop_if_equals().
	link_t* pop_if_equals(link_t& e, link_t*& oldHead, bool* wasLast = 0) volatile
	{
		link_t* newHead;
		oldHead = m_head.get();
		do {
			if (oldHead != &e)
			{
				oldHead = 0;
				break;
			}
			newHead = get_next(*oldHead);
		} while (!m_head.compare_exchange(newHead, oldHead, oldHead));
		if (!!wasLast)
			*wasLast = (!!oldHead) && (!get_next(*oldHead));
		return oldHead;
	}

	/// @brief Thread-safe implementation of pop_if_equals().
	link_t* pop_if_equals(link_t& e, bool* wasLast = 0) volatile
	{
		link_t* oldHead;
		return pop_if_equals(e, oldHead, wasLast);
	}
	/// @}

	/// @{
	/// @brief Swaps this contents of this stack with the specified stack.
	/// @param[in,out] s Stack to swap the contents of this stack with.
	///
	/// Note that the operation can only be considered atomic for one of the stacks involved.
	void swap(this_t& s)
	{
		cogs::swap(m_head, s.m_head);
	}

	/// @brief Thread-safe implementation of swap().
	void swap(this_t& s) volatile
	{
		cogs::swap(m_head, s.m_head);
	}

	/// @brief Thread-safe implementation of swap().
	void swap(volatile this_t& s)
	{
		s.swap(*this);
	}
	/// @}

	/// @{
	/// @brief Removes all elements from the stack.
	/// @return An no_aba_stack containing all of the elements that had been in this stack.
	///
	/// Since the no_aba_stack does not manage the scope of its elements, the elements are
	/// returned to allow cleanup.  Calling clear() is equivalent to calling exchange() and passing an empty stack.
	this_t clear()
	{
		link_t* oldHead;
		m_head.exchange(0, oldHead);
		return this_t(oldHead);
	}

	/// @brief Thread-safe implementation of clear().
	this_t clear() volatile
	{
		link_t* oldHead;
		m_head.exchange(0, oldHead);
		return this_t(oldHead);
	}
	/// @}
};


}


#endif
