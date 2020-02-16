//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SERIAL_DEFER_GUARD
#define COGS_HEADER_SYNC_SERIAL_DEFER_GUARD


#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/collections/slink.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/defer_guard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief Provides a mechanism for serializing work that must not be done in parallel.
///
/// A serial_defer_guard_t is used to queue and dequeue work, such that only a single caller
/// is allowed to process work at a time.
///
/// In order to add work, the caller must acquire the guard.  As the guard is released, the caller
/// is returned zero or one element to process.  Work will never be given to multiple threads in parallel.
///
/// Elements may also dequeue from the guard while it is acquired, to remove and repurpose
/// the next queued element.
///
/// A serial_defer_guard_t is vulnerable to livelock if the guard is constantly under contention,
/// and should not be used in situations in which this has the potential to occur.
///
/// Like stack or no_aba_stack, a serial_defer_guard_t is intrusive and does not protect
/// against hazardous (posthumous) access to an element, so elements must remain in
/// scope beyond any potential parallel access.
template <class link_t, class link_accessor = default_slink_accessor<link_t> >
class serial_defer_guard_t
{
private:
	versioned_ptr<link_t> m_head;
	alignas (atomic::get_alignment_v<size_t>) size_t m_count;

	typedef typename versioned_ptr<link_t>::version_t version_t;

public:
	serial_defer_guard_t()
		: m_count(0)
	{ }

	~serial_defer_guard_t()
	{
		// A guard should not be destructed while active
		COGS_ASSERT(is_free());
	}

	/// @{
	/// @brief Acquires the guard.  release() must be called to release the guard.
	/// @return True if the guard was previously unacquired by any thread.
	bool begin_guard() { return (!(m_count++)); }
	/// @brief Thread-safe implementation of begin_guard().
	bool begin_guard() volatile { return !post_assign_next(m_count); }
	/// @}

	/// @{
	/// @brief Gets the number of threads that currently hold the guard.
	/// @return The number of threads currently holding the guard.
	size_t get_count() const { return m_count; }
	/// @brief Thread-safe implementation of get_count().
	size_t get_count() const volatile { return atomic::load(m_count); }
	/// @}

	/// @{
	/// @brief Test if the guard is currently unacquired by any thread.
	/// @return True if the guard is currently unacquired by any thread.
	bool is_free() const { return !get_count(); }
	/// @brief Thread-safe implementation of is_free().
	bool is_free() const volatile { return !get_count(); }
	/// @}

	/// @{
	/// @brief Adds an item to the guard.  The guard must be in the acquired state when add() is called.
	void add(link_t& l)
	{
		link_accessor::set_next(l, m_head);
		m_head = &l;
	}
	/// @brief Thread-safe implementation of add().
	void add(link_t& l) volatile
	{
		version_t oldVersion;
		ptr<link_t> oldHead;
		m_head.get(oldHead, oldVersion);
		do {
			COGS_ASSERT(oldHead != &l);
			link_accessor::set_next(l, oldHead);
		} while (!m_head.versioned_exchange(&l, oldVersion, oldHead));
	}
	/// @}

	/// @{
	/// @brief Peeks at the next element in the guard.  The guard must be in the acquired state when peek() is called.
	/// @return The next element in the guard, or NULL.
	link_t* peek() { return m_head.get_ptr(); }
	/// @brief Thread-safe implementation of peek().
	link_t* peek() volatile { return m_head.get_ptr(); }
	/// @}

	/// @{
	/// @brief Removes the next element in the guard.  The guard must be in the acquired state when remove() is called.
	/// @param[in] wasLast If specified, receives a value indicating whether the element removed was the last element.
	/// @return An element
	link_t* remove(bool* wasLast = 0) // only safe within a guard
	{
		ptr<link_t> l = m_head;
		if (!l)
			return 0;
		ptr<link_t> next = link_accessor::get_next(*l);
		COGS_ASSERT(l != next);
		if (!!wasLast)
			*wasLast = !next;
		m_head = next;
		return l;
	}
	/// @brief Thread-safe implementation of remove().
	link_t* remove(bool* wasLast = 0) volatile // only safe within a guard
	{
		version_t oldVersion;
		ptr<link_t> oldHead;
		m_head.get(oldHead, oldVersion);
		for (;;)
		{
			if (!oldHead)
				break;
			ptr<link_t> next = link_accessor::get_next(*oldHead);
			COGS_ASSERT(oldHead != next);
			if (!m_head.versioned_exchange(next, oldVersion, oldHead))
				continue;
			if (!!wasLast)
				*wasLast = !next;
			break;
		}
		return oldHead.get_ptr();
	}
	/// @}

	/// @{
	/// @brief Attempts to releases the guard.
	///
	/// If an element is returned, the caller must process it, and must call release() again until no element is returned.
	/// @param[in] wasReleased If specified, receives a value indicating whether the guard became unacquired (by any thread).
	/// @return An element the caller must process, or null.  If an element is returned, the caller must process it,
	/// and must call release() again until no element is returned.
	link_t* release(bool* wasReleased = 0)
	{
		ptr<link_t> l;
		bool b = false;
		if (m_count > 1)
			--m_count;
		else
		{
			l = remove();
			if (!l)
			{
				m_count = 0;
				b = true;
			}
		}
		if (!!wasReleased)
			*wasReleased = b;
		return l;
	}

	/// @brief Thread-safe implementation of release().
	link_t* release(bool* wasReleased = 0) volatile
	{
		bool b = false;
		ptr<link_t> oldHead;
		size_t oldCount = atomic::load(m_count);
		for (;;)
		{
			COGS_ASSERT(!!oldCount);
			if (oldCount > 1)
			{
				if (!atomic::compare_exchange(m_count, oldCount - 1, oldCount, oldCount))
					continue;
				oldHead.release();
				break;
			}

			// oldCount == 1
			size_t oldCount2;
			version_t oldVersion;
			m_head.get(oldHead, oldVersion);
			if (!!oldHead)
				oldCount2 = atomic::load(m_count);
			else
			{
				if (!atomic::compare_exchange(m_count, (size_t)0, oldCount, oldCount))
					continue;

				m_head.get(oldHead, oldVersion);
				if (!oldHead)
				{
					b = true;
					break;
				}

				// Having successfully decremented the count, someone else slipped in a link and released.
				oldCount2 = pre_assign_next(m_count); // Reclaim the guard.
			}
			if (oldCount2 != 1)
			{
				oldCount = oldCount2;
				continue;
			}
			if (m_head.get_version() != oldVersion)
				continue;

			ptr<link_t> next = link_accessor::get_next(*oldHead);
			COGS_ASSERT(oldHead != next);
			if (!m_head.versioned_exchange(next, oldVersion))
				continue;
			break;
		}

		if (!!wasReleased)
			*wasReleased = b;

		return oldHead.get_ptr();
	}
	/// @}

	/// @{
	/// @brief Releases the guard only if there are other guard holders.
	///
	/// May allow the caller to release the guard without having to process elements.
	/// @return True if the guard was succesfully released.
	bool try_release() // Only released if NOT the last reference
	{
		if (m_count == 1)
			return false;
		--m_count;
		return true;
	}
	/// @brief Thread-safe implementation of try_release().
	bool try_release() volatile
	{
		size_t oldCount = atomic::load(m_count);
		for (;;)
		{
			if (oldCount <= 1)
				return false;

			if (atomic::compare_exchange(m_count, oldCount - 1, oldCount, oldCount))
				return true;
		}
	}
	/// @}
};


}


#endif
