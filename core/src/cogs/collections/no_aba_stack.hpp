//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_ABASTACK
#define COGS_HEADER_COLLECTION_ABASTACK


#include "cogs/collections/slink.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @defgroup Collections Collections
/// @{
/// @brief Collections classes
/// @}

/// @defgroup LockFreeCollections Lock-Free Collections
/// @{
/// @ingroup Collections
/// @ingroup Synchronization
/// @brief Lock-free Collection classes
/// @}


/// @ingroup LockFreeCollections
/// @brief Lock-free intrusive stack that is vulnerable to <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a>.
///
/// For a lock-free instrusive stack that is not vulnerable to <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a>,
/// use cogs::stack.  no_aba_stack is slightly more efficient than cogs::stack when usage patterns gaurantee
/// <a href="https://en.wikipedia.org/wiki/ABA_problem">The ABA Problem</a> will not arise,
/// such as when an element will never be added twice.  Neither no_aba_stack or cogs::stack protect
/// against hazardous (posthumous) access to an element, so elements must remain in
/// scope beyond any potential parallel access.  Managing hazardous access requires
/// managing the scope of the element, so must be done by the caller of any intrusive lock-free collection.
///
/// @tparam link_t Intrusive single-link element type.  Default: slink
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set the next link.  Default: default_slink_accessor\<T, ref_type\>
template <class link_t = slink, template <typename> class ref_type = ptr, class link_accessor = default_slink_accessor<link_t, ref_type> >
class no_aba_stack
{
public:
	/// @brief Aliases to the element reference type.  i.e. ptr\<link_t\>
	typedef ref_type<link_t> ref_t;

	/// @brief Alias to this type.
	typedef no_aba_stack<link_t, ref_type, link_accessor> this_t;

private:
	ref_t m_head;

	static const ref_t& get_next(const link_t& l) { return link_accessor::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l) { return link_accessor::get_next(l); }

	static void set_next(link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }

	no_aba_stack(const ref_t& setTo) { m_head = setTo; }

	no_aba_stack(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	/// @{
	/// @brief Constructor
	no_aba_stack() { }
	/// @}

	/// @{
	/// @brief Move constructor
	/// @param s The no_aba_stack to move
	no_aba_stack(this_t&& s)
		: m_head(std::move(s.m_head))
	{
		s.m_head.clear();
	}
	/// @}

	/// @{
	/// @brief Move assignment operator
	/// @param s The no_aba_stack to move
	/// @return A reference to this
	this_t& operator=(this_t&& s)
	{
		clear();
		m_head = std::move(s.m_head);
		s.m_head.clear();
	}
	/// @}

	/// @{
	/// @brief Counts the elements in the stack.
	/// @return The number of elements in the stack
	size_t count() const
	{
		size_t n = 0;
		ref_t l = m_head;
		while (!!l)
		{
			++n;
			l = get_next(*l);
		}
		return n;
	}
	/// @}

	/// @{
	/// @brief Peek at the element at the head of the stack without removing it.
	/// @return An element reference pointing to the head element.
	ref_t peek() const { return m_head; }
	/// @brief Thread-safe implementation of peek().
	ref_t peek() const volatile { return m_head; }
	/// @}

	/// @{
	/// @brief Checks if the stack is empty.
	/// @return True if the stack is empty.
	bool is_empty() const { return !m_head; }
	/// @brief Thread-safe implementation of is_empty().
	bool is_empty() const volatile { return !m_head; }
	/// @}

	/// @{
	/// @brief Checks if the stack is empty.  Tbhis is alias for is_empty().
	/// @return True if the stack is empty.
	bool operator!() const { return !m_head; }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return !m_head; }
	/// @}

	/// @{
	/// @brief Adds the specified element to the top of the stack.
	/// @param e Element to add to the top of the stack.
	/// @return True if the stack had previously been empty, false if at least 1 other element was also present.
	bool push(const ref_t& e)
	{
		if (!e)
			return is_empty();

		ref_t l = m_head;
		set_next(*e, l);
		m_head = e;
		return !l;
	}

	bool operator+=(const ref_t& e) { return push(e); }

	/// @brief Thread-safe implementation of push().
	bool push(const ref_t& e) volatile
	{
		if (!e)
			return is_empty();

		bool wasEmpty;
		ref_t oldHead = m_head;
		do {
			wasEmpty = !oldHead;
			set_next(*e, oldHead);
		} while (!m_head.compare_exchange(e, oldHead, oldHead));
		return wasEmpty;
	}

	bool operator+=(const ref_t& e) volatile { return push(e); }
	/// @}

	/// @{
	/// @brief Removes the element from the top of the stack.
	/// @param[out] wasLast If specified, receives a value indicating whether the removed element
	///                     was the last remaining element in the stack at the time.
	/// @return An element reference, or null if the stack was empty.
	ref_t pop(bool* wasLast = 0)
	{
		bool b = false;
		ref_t l = m_head;
		if (!!l)
		{
			ref_t next = get_next(*l);
			m_head = next;
			b = (!next);
		}
		if (!!wasLast)
			*wasLast = b;
		return l;
	}

	/// @brief Thread-safe implementation of pop().
	ref_t pop(bool* wasLast = 0) volatile
	{
		ref_t newHead;
		ref_t oldHead = m_head;
		do {
			if (!oldHead)
				break;
			newHead = get_next(*oldHead);
		} while (!m_head.compare_exchange(newHead, oldHead, oldHead));
		if (!!wasLast)
			*wasLast = (!!oldHead) && (!get_next(*oldHead));
		return oldHead;
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
	ref_t pop_if_equals(ref_t& e, ref_t& oldHead, bool* wasLast = 0)
	{
		oldHead = m_head.get();
		return pop_if_equals(e, wasLast);
	}

	/// @brief Thread-safe implementation of pop_if_equals().
	ref_t pop_if_equals(ref_t& e, ref_t& oldHead, bool* wasLast = 0) volatile
	{
		if (!e)
			oldHead.clear();
		else
		{
			ref_t newHead;
			oldHead = m_head.get();
			do {
				if (oldHead != e)
				{
					oldHead.clear();
					break;
				}
				newHead = get_next(*oldHead);
			} while (!m_head.compare_exchange(newHead, oldHead, oldHead));
		}

		if (!!wasLast)
			*wasLast = (!!oldHead) && (!get_next(*oldHead));
		return oldHead;
	}

	/// @brief Removes the specified element if it is currently at the head of the stack.
	/// @param e Element to remove, if at the head of the stack.
	/// @param[out] wasLast If specified, receives a value indicating whether the removed element
	///                     was the last remaining elements in the stack at the time.
	/// @return An element reference, or null if the specified element was not at the head of the stack.
	ref_t pop_if_equals(ref_t& e, bool* wasLast = 0)
	{
		ref_t sl;
		sl.clear();
		if (!!e)
		{
			if (m_head == e)
			{
				sl = m_head;
				if (!!sl)
					m_head = get_next(*sl);
			}
		}
		if (!!wasLast)
			*wasLast = (!!sl) && (!get_next(*sl));
		return sl;
	}

	/// @brief Thread-safe implementation of pop_if_equals().
	ref_t pop_if_equals(ref_t& e, bool* wasLast = 0) volatile
	{
		ref_t oldHead;
		return pop_if_equals(e, oldHead, wasLast);
	}
	/// @}

	/// @{
	/// @brief Exchanges this contents of this stack with the specified stack.
	/// @param[in,out] s Stack to exchange the contents of this stack with.
	///
	/// Note that the operation can only be considered atomic for one of the stacks involved.
	void swap(this_t& s)
	{
		cogs::swap(m_head, s.m_head);
	}
	/// @brief Thread-safe implementation of exchange().
	void swap(this_t& s) volatile
	{
		cogs::swap(m_head, s.m_head);
	}
	/// @brief Thread-safe implementation of exchange().
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
		ref_t oldHead = m_head;
		m_head.clear();
		return this_t(oldHead);
	}

	/// @brief Thread-safe implementation of clear().
	this_t clear() volatile
	{
		ref_t tmp;
		tmp.clear();
		m_head.exchange(tmp, tmp);
		return this_t(tmp);
	}
	/// @}
};


}


#endif
