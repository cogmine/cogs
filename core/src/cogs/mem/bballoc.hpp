//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_MEM_BBALLOC
#define COGS_HEADER_MEM_BBALLOC

#include <cstdlib> 

#include "cogs/env.hpp"
#include "cogs/env/mem/allocator.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/mem/allocator.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/const_bit_scan.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/serial_defer_guard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {

/// @ingroup Mem
/// @brief A lock-free allocator that uses cascading free-lists.
/// @tparam min_block_size Minimum block size
/// @tparam max_block_size Maximum size of blocks allocated by buddy_block_allocator.  Larger blocks
/// May be allocated directly from the large block allocator
/// @tparam large_block_allocator_type Allocator to use to allocate blocks too large for this allocator to manage.  Default: env::allocator
template <size_t min_block_size, size_t max_block_size, class large_block_allocator_type = env::allocator>
class buddy_block_allocator : public allocator
{
public:
	typedef buddy_block_allocator<min_block_size, max_block_size, large_block_allocator_type> this_t;

private:
	allocator_container<large_block_allocator_type> m_largeBlockAllocator;

	allocator_container<large_block_allocator_type>& get_large_block_allocator() const volatile
	{
		return const_cast<this_t*>(this)->m_largeBlockAllocator;
	}

	enum link_state
	{
		free_link_state = 0,       // State of a link in a freelist
		allocated_link_state = 1,  // State of a link on an allocated block
		coalescing_link_state = 2, // State of a link queued for insertion into a freelist (coalesc attempt)
		promoting_link_state = 3,  // State of a link queued for insertion into the next larger freelist
	};

	class link
	{
	public:
		ptr<link> m_next;
		versioned_ptr<link> m_prev;
		size_type m_selector;

		bool is_left_buddy() const volatile { return !!(m_selector & (size_t)1); }
		bool is_right_buddy() const volatile { return !(m_selector & (size_t)1); }

		size_t get_selector() const volatile { return (m_selector >> 1).get_int(); }

		void set_selector(size_t index, bool isLeftBuddy) volatile
		{
			m_selector = ((size_t)(index << 1) | (isLeftBuddy ? 1 : 0));
		}

		size_t get_block_size() const volatile { return IndexToSize(get_selector()); }

		void* get_block() const volatile
		{
			return (link*)((unsigned char*)this + overhead);
		}

		static link* from_block(void* block)
		{
			return (link*)((unsigned char*)block - overhead);
		}

		link* get_buddy() const volatile
		{
			link* result;
			size_t distance = overhead + get_block_size();
			if (is_left_buddy())
				result = (link*)((unsigned char*)this + distance);
			else
				result = (link*)((unsigned char*)this - distance);
			return result;
		}

		link* get_left_buddy() const volatile
		{
			if (is_left_buddy())
				return (link*)this;
			return (link*)((unsigned char*)this - (overhead + get_block_size()));
		}

		link* get_right_buddy() const volatile
		{
			if (is_right_buddy())
				return (link*)this;
			return (link*)((unsigned char*)this + (overhead + get_block_size()));
		}
	};

	typedef typename versioned_ptr<link>::version_t version_t;

	class link_accessor
	{
	public:
		typedef ptr<link> ref_t;

		static const volatile ref_t& get_next(const volatile link& l) { return l.m_next; }
		static void set_next(volatile link& l, const ref_t& src) { l.m_next = src; }
	};

	// All blocks are prefixed by a link, padded to end at the largest supported alignment.
	static constexpr size_t overhead = least_multiple_of_v<sizeof(link), largest_alignment>;

	// Pad smallestBlockSize to end on largest supported alignment.
	static constexpr size_t smallest_block_size = least_multiple_of_v<min_block_size, largest_alignment>;

	static constexpr size_t last_index_div = ((max_block_size + (overhead * 2)) - 1) / (smallest_block_size + (overhead * 2));
	static constexpr size_t last_index = !last_index_div ? 0 : (const_bit_scan_reverse<last_index_div>::value + 1);
	static constexpr size_t num_indexes = last_index + 1;
	static constexpr size_t largest_block_size = (((size_t)(1 << last_index))*(smallest_block_size + (overhead * 2))) - (overhead * 2);

	static size_t SizeToIndex(size_t requestSize)
	{
		// requestSize will be > 0
		// requestSize will be <= largest_block_size
		// index 0 == smallest_block_size
		size_t x = (requestSize + (overhead * 2)) - 1;
		size_t y = smallest_block_size + (overhead * 2);
		size_t z = x / y;
		return !z ? 0 : (bit_scan_reverse(z) + 1);
	}

	static size_t IndexToSize(size_t index)
	{
		return (((size_t)((size_t)1 << index))*(smallest_block_size + (overhead * 2))) - (overhead * 2);
	}

	class free_list
	{
	public:
		ptr<this_t>& get_allocator() const volatile
		{
			// m_bballoc is set on construction.  Slight optimization to cast away volatility.
			return const_cast<free_list*>(this)->m_bballoc;
		}

	private:
		volatile versioned_ptr<link> m_head;
		volatile serial_defer_guard_t<link, link_accessor> m_guard;

		ptr<this_t> m_bballoc;

		bool is_last_free_list() const volatile
		{
			return this == &(get_allocator()->m_freeLists[last_index]);
		}

		free_list* get_next_free_list() const volatile
		{
			if (is_last_free_list())
				return nullptr;

			size_t index = (this - get_allocator()->m_freeLists);
			return &(get_allocator()->m_freeLists[index + 1]);
		}

		void insert_inner(link& l) volatile
		{
			// add to main free list
			ptr<volatile link> volatileLink = &l;
			volatileLink->m_prev.release(); // no prev link, free state
			ptr<link> oldHead;
			version_t oldVersion;
			m_head.get(oldHead, oldVersion);
			for (;;)
			{
				l.m_next = oldHead;
				COGS_ASSERT(oldHead != &l);
				if (!oldHead)
				{
					if (!m_head.versioned_exchange(&l, oldVersion, oldHead))
						continue;
					break;
				}
				ptr<volatile link> volatileOldHead = oldHead;
				ptr<link> oldHeadPrev;
				version_t oldHeadPrevVersion;
				volatileOldHead->m_prev.get(oldHeadPrev, oldHeadPrevVersion);
				bool done = (!oldHeadPrev && (m_head.get_version() == oldVersion) && volatileOldHead->m_prev.versioned_exchange(&l, oldHeadPrevVersion));
				complete_lingering(oldHead, oldVersion);
				if (!done)
					continue;
				break;
			}
		}

		void release_guard() volatile
		{
			for (;;)
			{
				ptr<link> l = m_guard.release();
				if (!l)
					break;

				ptr<volatile link> volatileLink = l;
				size_t mark = volatileLink->m_prev.get_mark();
				COGS_ASSERT((mark == coalescing_link_state) || (mark == promoting_link_state));
				if (mark == coalescing_link_state)
				{
					ptr<link> otherBuddy = l->get_buddy();
					ptr<volatile link> volatileOtherBuddy = otherBuddy;
					COGS_ASSERT(volatileOtherBuddy->get_buddy() == l.get_ptr());
					COGS_ASSERT(volatileOtherBuddy->get_selector() == l->get_selector());
					ptr<link> otherBuddyPrev;
					version_t otherBuddyPrevVersion;
					size_t otherBuddyPrevMark;
					volatileOtherBuddy->m_prev.get(otherBuddyPrev, otherBuddyPrevVersion);
					for (;;)
					{
						otherBuddyPrevMark = otherBuddyPrev.get_mark();
						COGS_ASSERT(otherBuddyPrevMark != promoting_link_state);
						if (otherBuddyPrevMark == allocated_link_state) // other buddy is allocated.
						{
							insert_inner(*l);
							break;
						}
						ptr<link> newOtherBuddyPrev;
						if (otherBuddyPrevMark == coalescing_link_state) // Other buddy queued to coalesc as well.
							newOtherBuddyPrev.set_to_mark(promoting_link_state);
						else //if (otherBuddyPrevMark == free_link_state)
						{ // attempt mid-removal
							// Try to change otherBuddyPrevMark to allocated_link_state
							newOtherBuddyPrev = otherBuddyPrev;
							newOtherBuddyPrev.set_mark(allocated_link_state);
						}
						if (!volatileOtherBuddy->m_prev.versioned_exchange(newOtherBuddyPrev, otherBuddyPrevVersion, otherBuddyPrev))
							continue;
						break;
					}

					if (otherBuddyPrevMark != free_link_state) // else, attempt mid-removal
						continue;

					if (!!otherBuddyPrev) // otherwise, it was at the head of the list and complete_lingering should suffice
					{
						ptr<link> otherBuddyNext = volatileOtherBuddy->m_next;
						ptr<volatile link> volatileOtherBuddyPrev = otherBuddyPrev;
						volatileOtherBuddyPrev->m_next.compare_exchange(otherBuddyNext, otherBuddy);
						if (!!otherBuddyNext)
						{
							ptr<volatile link> volatileOtherBuddyNext = otherBuddyNext;
							volatileOtherBuddyNext->m_prev.compare_exchange(otherBuddyPrev, otherBuddy);
						}
					}
					complete_lingering();
				}
				else if (mark != promoting_link_state) // else, was in coalesc queue when buddy was free'd, now represents both.
					continue;

				ptr<link> leftBuddy = l->get_left_buddy();
				COGS_ASSERT(leftBuddy->get_selector() == l->get_selector());
				ptr<link> promotedBuddy = link::from_block(leftBuddy.get_ptr());
				COGS_ASSERT(promotedBuddy->get_selector() == l->get_selector() + 1);
				COGS_ASSERT((promotedBuddy->get_selector() == last_index) || (promotedBuddy->m_prev.get_mark() == allocated_link_state));
				free_list* nextFreeList = get_next_free_list();
				if (!nextFreeList)
					get_allocator()->get_large_block_allocator().deallocate(promotedBuddy);
				else
					nextFreeList->insert(*promotedBuddy);
			}
		}

		void complete_lingering() volatile
		{
			ptr<link> oldHead;
			version_t oldVersion;
			complete_lingering(oldHead, oldVersion);
		}

		void complete_lingering(ptr<link>& oldHead, version_t& oldVersion) volatile
		{
			for (;;)
			{
				m_head.get(oldHead, oldVersion);
				if (!oldHead)
					break;

				ptr<volatile link> oldHeadVolatile = oldHead;
				ptr<link> oldHeadPrev = oldHeadVolatile->m_prev.get_ptr();
				if (!oldHeadPrev)
					break;

				COGS_ASSERT(oldHead != oldHeadPrev);

				if (oldHeadPrev.get_mark() == free_link_state) // no mark - it must include a ptr since it's not null.
				{ // Insertion
					m_head.versioned_exchange(oldHeadPrev, oldVersion);
					continue;
				}

				// Removal

				ptr<link> oldHeadNext = oldHead->m_next; // ok to read non-volatile, would not change if head version hasn't.
				if (m_head.get_version() != oldVersion)
					continue;

				if (!oldHeadNext)
				{
					if (!m_head.versioned_exchange(0, oldVersion))
						continue;

					oldHead.release();
					break;
				}

				// Before we can put m_head->m_next at the head of the list, we need to clear it's prev ptr.
				ptr<volatile link> oldHeadNextVolatile = oldHeadNext;
				ptr<link> oldHeadNextPrev;
				version_t oldHeadNextPrevVersion; 
				oldHeadNextVolatile->m_prev.get(oldHeadNextPrev, oldHeadNextPrevVersion);
				if (m_head.get_version() != oldVersion)
					continue;

				size_t oldHeadNextPrevMark = oldHeadNextPrev.get_mark();
				oldHeadNextVolatile->m_prev.versioned_exchange((link*)oldHeadNextPrevMark, oldHeadNextPrevVersion, oldHeadNextPrev);
				m_head.versioned_exchange(oldHeadNext, oldVersion);

				// The previous link pointer is used by mid-removal to identify the link to fixup the next link of.
				// The guard prevents more than 1 mid-removal from interrupting complete_lingering().
				// Therefore, if colision were to leave a prev link incorrect, it would be resolved by complete_lingering(),
				// before the next mid-removal attempt.
			}
		}

	public:
		link* remove() volatile
		{
			link* result = 0;
			if (is_last_free_list())
			{
				ptr<link> newHead;
				ptr<link> oldHead;
				version_t v;
				m_head.get(oldHead, v);
				do {
					if (!oldHead)
						break;
					newHead = oldHead->m_next;
				} while (!m_head.versioned_exchange(newHead, v, oldHead));
				if (!!oldHead)
					result = oldHead.get_ptr();
			}
			else
			{
				m_guard.begin_guard(); // The guard ensures the block can be interrupted by at most 1 insert operation
				bool done = false;
				for (;;)
				{
					ptr<link> l = m_guard.remove(); // prefer to snag a block in the process of deallocating
					if (!l)
					{
						ptr<link> oldHead;
						version_t oldVersion;
						m_head.get(oldHead, oldVersion);
						if (!!oldHead)
						{
							ptr<volatile link> oldHeadVolatile = oldHead;
							ptr<link> oldHeadPrev;
							version_t oldHeadPrevVersion;
							oldHeadVolatile->m_prev.get(oldHeadPrev, oldHeadPrevVersion);
							if (!oldHeadPrev)
							{
								done = oldHeadVolatile->m_prev.versioned_exchange((link*)allocated_link_state, oldHeadPrevVersion); // Try to put a mark in oldHead->get_prev_link()
								if (done)
									result = oldHead.get_ptr();
							}

							complete_lingering(oldHead, oldVersion);
							if (!done)
								continue;
							break;
						}

						l = m_guard.remove(); // goink
						if (!l)
							break;
					}

					// Block now belongs to us.  However, if another thread attempts to
					// release a block who's buddy is already in the guard, it may change
					// the link state from coalescing to promoting.  We need to atomically
					// set it to allocated, only if coalescing.  Otherwise, we need to free
					// it's buddy block.

					ptr<volatile link> volatileLink = l;
					ptr<link> oldPrev;
					version_t oldPrevVersion;
					volatileLink->m_prev.get(oldPrev, oldPrevVersion);
					size_t prevMark;
					for (;;)
					{
						prevMark = oldPrev.get_mark();
						COGS_ASSERT(prevMark != free_link_state);
						if (prevMark == allocated_link_state) // Must have just been claimed in a coalesc
							break;
						if (!volatileLink->m_prev.versioned_exchange((link*)allocated_link_state, oldPrevVersion, oldPrev))
							continue;
						done = true;
						break;
					}
					if (!done)
						continue;

					if (prevMark == promoting_link_state)
					{ // actually 2 blocks here, free the other one.
						ptr<link> otherBuddy = l->get_buddy();
						COGS_ASSERT(otherBuddy->get_buddy() == l.get_ptr());
						ptr<volatile link> volatileOtherBuddy = otherBuddy;
						volatileOtherBuddy->m_prev.set_to_mark(coalescing_link_state); // doesn't need to be volatile here because no other thread will change it.
						m_guard.add(*otherBuddy);
					}

					result = l.get_ptr();
					break;
				}
				release_guard();
			}
			return result;
		}

		// attempts to coalesc
		void insert(link& l) volatile
		{
			if (is_last_free_list())
			{
				ptr<link> oldHead;
				version_t v;
				m_head.get(oldHead, v);
				do {
					l.m_next = oldHead;
					COGS_ASSERT(oldHead != &l);
				} while (!m_head.versioned_exchange(&l, v, oldHead));
			}
			else
			{
				m_guard.begin_guard();
				((volatile link*)&l)->m_prev.set_to_mark(coalescing_link_state);
				m_guard.add(l);
				release_guard();
			}

		}

		bool is_empty() const volatile { return !m_head; }
		link* peek() const volatile { return m_head.get_ptr(); }
	};

	free_list m_freeLists[num_indexes];

	void init()
	{
		for (size_t i = 0; i < num_indexes; i++)
			m_freeLists[i].get_allocator() = this;
	}

public:
	explicit buddy_block_allocator()
	{
		init();
	}

	explicit buddy_block_allocator(large_block_allocator_type& al)
		: m_largeBlockAllocator(&al)
	{
		init();
	}

	~buddy_block_allocator()
	{
		for (;;)
		{
			link* l = m_freeLists[last_index].remove();
			if (!l)
				break;
			m_largeBlockAllocator.deallocate(l);
		}
	}

	virtual ptr<void> allocate(size_t n, size_t align) volatile
	{
		ptr<void> block;

		if (n > largest_block_size)
		{
			block = get_large_block_allocator().allocate(n + overhead, largest_alignment);
			if (!block)
			{
				COGS_ASSERT(false); // For now, let's consider out of memory to be a fatal error
				return nullptr; // TBD ?
			}
			link* linkPtr = (link*)(block.get_ptr());
			linkPtr->set_selector(num_indexes, false); // Selector of m_numIndexes means a large block
			return linkPtr->get_block();
		}

		if (!n)
			n = 1;

		size_t idealIndex = SizeToIndex(n);
		size_t i = idealIndex;
		link* l;
		for (;;)
		{
			l = m_freeLists[i].remove();
			if (!!l)
				break;

			if (i == last_index) // If at the last freelist
			{
				l = (link*)(get_large_block_allocator().allocate(largest_block_size + overhead, largest_alignment).get_ptr());
				if (!l)
				{
					COGS_ASSERT(false); // For now, let's consider out of memory to be a fatal error
					return nullptr; // TBD ?
				}
				l->set_selector(last_index, false);
				break;
			}
			++i;
		}
		block = l->get_block();
		while (i > idealIndex)
		{ // Split up allocated block into two smaller blocks, and put one into i-1
			i--;
			link* leftBuddy = (link*)(block.get_ptr());
			leftBuddy->set_selector(i, true);
			leftBuddy->m_prev.set_to_mark(allocated_link_state); // mark as allocated

			link* rightBuddy = leftBuddy->get_right_buddy();
			rightBuddy->set_selector(i, false);
			rightBuddy->m_prev.set_to_mark(allocated_link_state);
			COGS_ASSERT(rightBuddy->get_buddy() == leftBuddy);

			m_freeLists[i].insert(*rightBuddy);

			block = leftBuddy->get_block();
		}

		return block;
	}

	virtual void deallocate(const ptr<void>& p) volatile
	{
		if (!p)
			return;

		link* l = link::from_block(p.get_ptr());
		size_t i = l->get_selector();

		COGS_ASSERT(i <= num_indexes);
		COGS_ASSERT(l->m_prev.get_mark() == allocated_link_state);

		if (i == num_indexes)
			get_large_block_allocator().deallocate((void*)l);
		else
			m_freeLists[i].insert(*l);
	}

	virtual bool try_reallocate(const ptr<void>& p, size_t newSize) volatile
	{
		if (!p)
			return false;

		link* l = link::from_block(p.get_ptr());
		size_t i = l->get_selector();

		if (i == num_indexes)
			return get_large_block_allocator().try_reallocate(l, newSize + overhead);
		return (newSize <= IndexToSize(i));
	}

	virtual size_t get_allocation_size(const ptr<void>& p, size_t align, size_t knownSize) const volatile
	{
		link* l = link::from_block(p.get_ptr());
		size_t i = l->get_selector();
		if (i == num_indexes)
			return get_large_block_allocator().get_allocation_size(l, align, knownSize + overhead) - overhead;
		return IndexToSize(i);
	}

};


}


#endif
