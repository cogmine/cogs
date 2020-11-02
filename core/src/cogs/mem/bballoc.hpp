//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_MEM_BBMM
#define COGS_HEADER_MEM_BBMM

#include <cstdlib>
#include <memory>

#include "cogs/env.hpp"
#include "cogs/env/mem/memory_manager.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/mem/const_bit_scan.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/serial_defer_guard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {

/// @ingroup Mem
/// @brief A lock-free memory manager that uses cascading free-lists.
/// @tparam min_block_size Minimum block size
/// @tparam max_block_size Maximum size of blocks allocated by buddy_block_memory_manager.  Larger blocks
/// May be allocated directly from the large block memory manager
/// @tparam large_block_memory_manager_type Memory manager to use to allocate blocks too large to manage.  Default: env::memory manager
template <size_t min_block_size, size_t max_block_size, class large_block_memory_manager_type = env::memory_manager>
class buddy_block_memory_manager : public memory_manager_base<buddy_block_memory_manager<min_block_size, max_block_size, large_block_memory_manager_type>, false>
{
public:
	typedef buddy_block_memory_manager<min_block_size, max_block_size, large_block_memory_manager_type> this_t;

private:
	large_block_memory_manager_type m_largeBlockMemoryManager;

	large_block_memory_manager_type& get_large_block_memory_manager() const volatile
	{
		return const_cast<this_t*>(this)->m_largeBlockMemoryManager;
	}

	enum class link_state
	{
		free = 0,       // State of a link in a freelist
		allocated = 1,  // State of a link on an allocated block
		coalescing = 2, // State of a link queued for insertion into a freelist (coalesc attempt)
		promoting = 3,  // State of a link queued for insertion into the next larger freelist
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

		static const volatile ref_t& get_next(const volatile link& lnk) { return lnk.m_next; }
		static void set_next(volatile link& lnk, const ref_t& src) { lnk.m_next = src; }
	};

	// All blocks are prefixed by a link, padded to end at the largest supported alignment.
	static constexpr size_t overhead = least_multiple_of_v<sizeof(link), largest_alignment>;

	// Pad smallestBlockSize to end on largest supported alignment.
	static constexpr size_t smallest_block_size = least_multiple_of_v<min_block_size, largest_alignment>;

	static constexpr size_t last_index_div = ((max_block_size + (overhead * 2)) - 1) / (smallest_block_size + (overhead * 2));
	static constexpr size_t last_index = !last_index_div ? 0 : (const_bit_scan_reverse<last_index_div>::value + 1);
	static constexpr size_t index_count = last_index + 1;
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
		ptr<this_t>& get_memory_manager() const volatile
		{
			// m_bbmm is set on construction.  Slight optimization to cast away volatility.
			return const_cast<free_list*>(this)->m_bbmm;
		}

	private:
		volatile versioned_ptr<link> m_head;
		volatile serial_defer_guard_t<link, link_accessor> m_guard;

		ptr<this_t> m_bbmm;

		bool is_last_free_list() const volatile
		{
			return this == &(get_memory_manager()->m_freeLists[last_index]);
		}

		free_list* get_next_free_list() const volatile
		{
			if (is_last_free_list())
				return nullptr;

			size_t index = (this - get_memory_manager()->m_freeLists);
			return &(get_memory_manager()->m_freeLists[index + 1]);
		}

		void insert_inner(link& lnk) volatile
		{
			// add to main free list

			ptr<link> oldHead;
			version_t oldHeadVersion;
			version_t oldHeadPrevVersion;
			bool done = false;
			for (;;)
			{
				complete_lingering(oldHead, oldHeadVersion, oldHeadPrevVersion);
				if (done)
					break;
				ptr<volatile link> lnkVolatile = &lnk;
				lnkVolatile->m_prev.release();
				lnk.m_next = oldHead;
				COGS_ASSERT(oldHead != &lnk);
				if (!oldHead)
				{
					if (m_head.versioned_exchange(&lnk, oldHeadVersion))
						break;
					continue;
				}
				ptr<volatile link> oldHeadVolatile = oldHead;
				done = oldHeadVolatile->m_prev.versioned_exchange(&lnk, oldHeadPrevVersion);
				continue;
			}
		}

		void release_guard() volatile
		{
			for (;;)
			{
				ptr<link> lnk = m_guard.release();
				if (!lnk)
					break;

				ptr<volatile link> linkVolatile = lnk;
				size_t mark = linkVolatile->m_prev.get_mark();
				COGS_ASSERT((mark == (int)link_state::coalescing) || (mark == (int)link_state::promoting));
				if (mark == (int)link_state::coalescing)
				{
					ptr<link> otherBuddy = lnk->get_buddy();
					ptr<volatile link> otherBuddyVolatile = otherBuddy;
					COGS_ASSERT(otherBuddyVolatile->get_buddy() == lnk.get_ptr());
					COGS_ASSERT(otherBuddyVolatile->get_selector() == lnk->get_selector());
					ptr<link> otherBuddyPrev;
					version_t otherBuddyPrevVersion;
					size_t otherBuddyPrevMark;
					otherBuddyVolatile->m_prev.get(otherBuddyPrev, otherBuddyPrevVersion);
					for (;;)
					{
						otherBuddyPrevMark = otherBuddyPrev.get_mark();
						COGS_ASSERT(otherBuddyPrevMark != (int)link_state::promoting);
						if (otherBuddyPrevMark == (int)link_state::allocated) // other buddy is allocated.
						{
							insert_inner(*lnk);
							break;
						}
						ptr<link> newOtherBuddyPrev;
						if (otherBuddyPrevMark == (int)link_state::coalescing) // Other buddy queued to coalesc as well.
							newOtherBuddyPrev.set_to_mark((int)link_state::promoting);
						else //if (otherBuddyPrevMark == link_state::free)
						{ // attempt mid-removal
							// Try to change otherBuddyPrevMark to link_state::allocated
							newOtherBuddyPrev = otherBuddyPrev;
							newOtherBuddyPrev.set_mark((int)link_state::allocated);
						}
						if (!otherBuddyVolatile->m_prev.versioned_exchange(newOtherBuddyPrev, otherBuddyPrevVersion, otherBuddyPrev))
							continue;
						break;
					}

					if (otherBuddyPrevMark != (int)link_state::free) // else, attempt mid-removal
						continue;

					if (!!otherBuddyPrev) // otherwise, it was at the head of the list and complete_lingering should suffice
					{
						ptr<link> otherBuddyNext = otherBuddyVolatile->m_next;
						ptr<volatile link> otherBuddyPrevVolatile = otherBuddyPrev;
						otherBuddyPrevVolatile->m_next.compare_exchange(otherBuddyNext, otherBuddy);
						if (!!otherBuddyNext)
						{
							ptr<volatile link> otherBuddyNextVolatile = otherBuddyNext;
							otherBuddyNextVolatile->m_prev.compare_exchange(otherBuddyPrev, otherBuddy);
						}
					}
					complete_lingering();
				}
				else if (mark != (int)link_state::promoting) // else, was in coalesc queue when buddy was free'd, now represents both.
					continue;

				ptr<link> leftBuddy = lnk->get_left_buddy();
				COGS_ASSERT(leftBuddy->get_selector() == lnk->get_selector());
				ptr<link> promotedBuddy = link::from_block(leftBuddy.get_ptr());
				COGS_ASSERT(promotedBuddy->get_selector() == lnk->get_selector() + 1);
				COGS_ASSERT((promotedBuddy->get_selector() == last_index) || (promotedBuddy->m_prev.get_mark() == (int)link_state::allocated));
				free_list* nextFreeList = get_next_free_list();
				if (!nextFreeList)
					get_memory_manager()->get_large_block_memory_manager().deallocate(promotedBuddy.get_ptr());
				else
					nextFreeList->insert(*promotedBuddy);
			}
		}

		void complete_lingering() volatile
		{
			ptr<link> oldHead;
			version_t oldHeadVersion;
			version_t oldHeadPrevVersion;
			complete_lingering(oldHead, oldHeadVersion, oldHeadPrevVersion);
		}

		void complete_lingering(ptr<link>& oldHead, version_t& oldHeadVersion, version_t& oldHeadPrevVersion) volatile
		{
			ptr<link> oldHeadPrev;
			bool keepHeadPrev = false;
			oldHead = m_head.get_ptr(oldHeadVersion);
			for (;;)
			{
				if (!oldHead)
					return;

				ptr<volatile link> oldHeadVolatile = oldHead;
				if (keepHeadPrev)
					keepHeadPrev = false;
				else
					oldHeadPrev = oldHeadVolatile->m_prev.get_ptr(oldHeadPrevVersion);

				// Make sure this represents a value that was present while still head element.
				version_t oldHeadVersion2;
				ptr<link> oldHead2 = m_head.get_ptr(oldHeadVersion2);
				if (oldHeadVersion != oldHeadVersion2)
				{
					oldHead = oldHead2;
					oldHeadVersion = oldHeadVersion2;
					continue;
				}

				if (!oldHeadPrev)
					return;

				COGS_ASSERT(oldHead != oldHeadPrev);

				if (oldHeadPrev.get_mark() == (int)link_state::free) // no mark - it must include a ptr since it's not null.
				{
					// Insert
					if (m_head.versioned_exchange(oldHeadPrev, oldHeadVersion, oldHead))
						oldHead = oldHeadPrev;
					continue;
				}

				// Remove
				// There is a race here with mid-removal of the next element that could
				// results in that removed element being set as the head by us.
				// Since both are claimed by the thread to set the mark, and both threads will call
				// complete_lingering again before returning, the race seems to be addressed.
				ptr<link> oldHeadNext = oldHeadVolatile->m_next; // Might race with change of m_next by mid-removal

				// Before we can put m_head->m_next at the head of the list, we need to clear it's prev ptr.
				ptr<link> oldHeadNextPrev;
				version_t oldHeadNextPrevVersion;
				ptr<volatile link> oldHeadNextVolatile;
				bool retry = false;
				for (;;)
				{
					oldHead2 = m_head.get_ptr(oldHeadVersion2);
					if (oldHeadVersion2 != oldHeadVersion)
					{
						oldHead = oldHead2;
						oldHeadVersion = oldHeadVersion2;
						retry = true;
						break;
					}

					if (!oldHeadNext)
					{
						if (!m_head.versioned_exchange(0, oldHeadVersion, oldHead))
						{
							retry = true;
							break;
						}
						oldHead.release();
						return;
					}

					oldHeadNextVolatile = oldHeadNext;
					oldHeadNextVolatile->m_prev.get(oldHeadNextPrev, oldHeadNextPrevVersion);

					// Make sure oldHeadNext hasn't changed.
					ptr<link> oldHeadNext2 = oldHead->m_next;
					if (oldHeadNext == oldHeadNext2)
						break;
					oldHeadNext = oldHeadNext2;
				}
				if (retry)
					continue;

				// Make sure oldHead hasn't changed
				oldHead2 = m_head.get_ptr(oldHeadVersion2);
				if (oldHeadVersion != oldHeadVersion2)
				{
					oldHead = oldHead2;
					oldHeadVersion = oldHeadVersion2;
					continue;
				}
				// Clear previous pointer, but leave the mark if present.
				size_t oldHeadNextPrevMark = oldHeadNextPrev.get_mark();
				oldHeadNextVolatile->m_prev.versioned_exchange((link*)oldHeadNextPrevMark, oldHeadNextPrevVersion, oldHeadNextPrev);
				if (m_head.versioned_exchange(oldHeadNext, oldHeadVersion, oldHead))
				{
					oldHeadPrev.set_to_mark(oldHeadNextPrevMark);
					oldHeadPrevVersion = oldHeadNextPrevVersion;
					keepHeadPrev = true;
					oldHead = oldHeadNext;
				}

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
					ptr<link> lnk = m_guard.remove(); // prefer to snag a block in the process of deallocating
					if (!lnk)
					{
						ptr<link> oldHead;
						version_t oldHeadVersion;
						m_head.get(oldHead, oldHeadVersion);
						if (!!oldHead)
						{
							ptr<volatile link> oldHeadVolatile = oldHead;
							ptr<link> oldHeadPrev;
							version_t oldHeadPrevVersion;
							oldHeadVolatile->m_prev.get(oldHeadPrev, oldHeadPrevVersion);
							if (!oldHeadPrev)
							{
								done = oldHeadVolatile->m_prev.versioned_exchange((link*)link_state::allocated, oldHeadPrevVersion); // Try to put a mark in oldHead->get_prev_link()
								if (done)
									result = oldHead.get_ptr();
							}

							complete_lingering(oldHead, oldHeadVersion, oldHeadPrevVersion);
							if (!done)
								continue;
							break;
						}

						lnk = m_guard.remove(); // goink
						if (!lnk)
							break;
					}

					// Block now belongs to us.  However, if another thread attempts to
					// release a block who's buddy is already in the guard, it may change
					// the link state from coalescing to promoting.  We need to atomically
					// set it to allocated, only if coalescing.  Otherwise, we need to free
					// it's buddy block.

					ptr<volatile link> linkVolatile = lnk;
					ptr<link> oldPrev;
					version_t oldPrevVersion;
					linkVolatile->m_prev.get(oldPrev, oldPrevVersion);
					size_t prevMark;
					for (;;)
					{
						prevMark = oldPrev.get_mark();
						COGS_ASSERT(prevMark != (int)link_state::free);
						if (prevMark == (int)link_state::allocated) // Must have just been claimed in a coalesc
							break;
						if (!linkVolatile->m_prev.versioned_exchange((link*)link_state::allocated, oldPrevVersion, oldPrev))
							continue;
						done = true;
						break;
					}
					if (!done)
						continue;

					if (prevMark == (int)link_state::promoting)
					{ // actually 2 blocks here, free the other one.
						ptr<link> otherBuddy = lnk->get_buddy();
						COGS_ASSERT(otherBuddy->get_buddy() == lnk.get_ptr());
						ptr<volatile link> otherBuddyVolatile = otherBuddy;
						otherBuddyVolatile->m_prev.set_to_mark((int)link_state::coalescing); // doesn't need to be volatile here because no other thread will change it.
						m_guard.add(*otherBuddy);
					}

					result = lnk.get_ptr();
					break;
				}
				release_guard();
			}
			return result;
		}

		// attempts to coalesc
		void insert(link& lnk) volatile
		{
			if (is_last_free_list())
			{
				ptr<link> oldHead;
				version_t v;
				m_head.get(oldHead, v);
				do {
					lnk.m_next = oldHead;
					COGS_ASSERT(oldHead != &lnk);
				} while (!m_head.versioned_exchange(&lnk, v, oldHead));
			}
			else
			{
				m_guard.begin_guard();
				((volatile link*)&lnk)->m_prev.set_to_mark((int)link_state::coalescing);
				m_guard.add(lnk);
				release_guard();
			}

		}

		bool is_empty() const volatile { return !m_head; }
		link* peek() const volatile { return m_head.get_ptr(); }
	};

	free_list m_freeLists[index_count];

	void init()
	{
		for (size_t i = 0; i < index_count; i++)
			m_freeLists[i].get_memory_manager() = this;
	}

public:
	buddy_block_memory_manager()
	{
		init();
	}

	explicit buddy_block_memory_manager(large_block_memory_manager_type& al)
		: m_largeBlockMemoryManager(&al)
	{
		init();
	}

	~buddy_block_memory_manager()
	{
		for (size_t i = 0; i < last_index; i++)
			COGS_ASSERT(m_freeLists[i].is_empty());
		for (;;)
		{
			link* lnk = m_freeLists[last_index].remove();
			if (!lnk)
				break;
			m_largeBlockMemoryManager.deallocate(lnk);
		}
	}

	void* allocate(size_t n, size_t = cogs::largest_alignment, size_t* usableSize = nullptr) volatile
	{
		void* block;

		if (n > largest_block_size)
		{
			block = get_large_block_memory_manager().allocate(n + overhead, largest_alignment, usableSize);
			if (!block)
			{
				COGS_ASSERT(false); // For now, let's consider out of memory to be a fatal error
				return nullptr; // TBD ?
			}
			if (usableSize)
				*usableSize -= overhead;
			link* linkPtr = (link*)block;
			linkPtr->set_selector(index_count, false); // Selector of index_count means a large block
			return linkPtr->get_block();
		}

		if (!n)
			n = 1;

		size_t idealIndex = SizeToIndex(n);
		size_t i = idealIndex;
		link* lnk;
		for (;;)
		{
			lnk = m_freeLists[i].remove();
			if (!!lnk)
				break;

			if (i == last_index) // If at the last freelist
			{
				lnk = (link*)(get_large_block_memory_manager().allocate(largest_block_size + overhead, largest_alignment));
				if (!lnk)
				{
					COGS_ASSERT(false); // For now, let's consider out of memory to be a fatal error
					return nullptr; // TBD ?
				}
				lnk->set_selector(last_index, false);
				break;
			}
			++i;
		}
		block = lnk->get_block();
		while (i > idealIndex)
		{ // Split up allocated block into two smaller blocks, and put one into i-1
			i--;
			link* leftBuddy = (link*)block;
			leftBuddy->set_selector(i, true);
			leftBuddy->m_prev.set_to_mark((int)link_state::allocated); // mark as allocated

			link* rightBuddy = leftBuddy->get_right_buddy();
			rightBuddy->set_selector(i, false);
			rightBuddy->m_prev.set_to_mark((int)link_state::allocated);
			COGS_ASSERT(rightBuddy->get_buddy() == leftBuddy);

			m_freeLists[i].insert(*rightBuddy);

			block = leftBuddy->get_block();
		}
		if (usableSize)
			*usableSize = IndexToSize(idealIndex);

		return block;
	}

	void deallocate(void* p) volatile
	{
		if (!p)
			return;

		link* lnk = link::from_block(p);
		size_t i = lnk->get_selector();

		COGS_ASSERT(i <= index_count);
		COGS_ASSERT(lnk->m_prev.get_mark() == (int)link_state::allocated);

		if (i == index_count)
			get_large_block_memory_manager().deallocate((void*)lnk);
		else
			m_freeLists[i].insert(*lnk);
	}

	bool try_reallocate(void* p, size_t newSize, size_t = cogs::largest_alignment, size_t* usableSize = nullptr) volatile
	{
		if (!p)
			return false;

		link* lnk = link::from_block(p);
		size_t i = lnk->get_selector();

		if (i == index_count)
		{
			bool b = get_large_block_memory_manager().try_reallocate(lnk, newSize + overhead, cogs::largest_alignment, usableSize);
			if (usableSize)
				*usableSize -= overhead;
			return b;
		}
		return (newSize <= IndexToSize(i));
	}
};


}


#endif
