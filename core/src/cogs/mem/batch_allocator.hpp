//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_HEADER_MEM_BATCH_ALLOCATOR
#define COGS_HEADER_MEM_BATCH_ALLOCATOR


#include "cogs/env.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/storage_union.hpp"
#include "cogs/load.hpp"
#include "cogs/mem/allocator_base.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/serial_defer_guard.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


constexpr size_t default_minimum_batch_size = 32;

template <typename T, size_t minimum_batch_size = default_minimum_batch_size, class memory_manager_t = default_memory_manager>
class batch_allocator : public allocator_base<T, batch_allocator<T, minimum_batch_size>, false>
{
public:
	static_assert(minimum_batch_size > 0);

	typedef T type;

private:
	typedef batch_allocator<T, minimum_batch_size> this_t;

	mutable memory_manager_t m_memoryManager;

	class batch;

	typedef placement<T> placement_t;

	struct entry : public placement_t
	{
		batch* m_batch;
	};

	typedef storage_union<entry, entry*> node_t;

	class batch : public slink_t<batch>
	{
	public:
		enum class state : size_t
		{
			current = 0b000,
			floating = 0b001, // In this state, transitioning from 0 to 1 element will cause it to be added to list
			adding = 0b010, // In this state, transitioning to a full batch triggers mid-removal
			in_list = 0b011, // Indicates the batch has been completely added to the list.
			removing = 0b100  // Indicates a mid-removal from the list has started.
		};

		struct freelist_content_t
		{
			node_t* m_head;
			size_t m_countAndState; // (count << 3) | state
		};

		this_t& m_batchAllocator;
		freelist_content_t m_freelist alignas(atomic::get_alignment_v<freelist_content_t>);
		size_t m_curPos alignas(atomic::get_alignment_v<size_t>) = 0;
		const size_t m_batchSize;
		node_t* const m_nodes;

		mutable hazard m_hazard; // Prevents ABA by defering dealloc/enqueue until no longer referenced.

		ptr<batch> m_next; // used by guard only
		ptr<batch> m_listNext;
		versioned_ptr<batch> m_listPrev = (batch*)1;
		bool m_removingFromCurrent = false;

		batch(this_t& batchAllocator, node_t* nodes, size_t n)
			: m_batchAllocator(batchAllocator),
			m_freelist{ nullptr, 0 },
			m_batchSize(n),
			m_nodes(nodes)
		{ }

		batch(batch&&) = delete;
		batch(const batch&) = delete;

		~batch()
		{
			COGS_ASSERT(is_full());
		}

		batch& operator=(const batch&) = delete;
		batch& operator=(batch&&) = delete;

		bool is_full() const { return get_count() == m_curPos; }
		bool is_full() const volatile { return get_count() == const_cast<const batch*>(this)->m_curPos; }

		size_t get_count() const
		{
			return m_freelist.m_countAndState >> 3;
		}

		size_t get_count() const volatile
		{
			return atomic::load(m_freelist.m_countAndState) >> 3;
		}

		int set_floating_state() volatile // 0 == set (done).  -1 == was full (caller releases).  1 == had some (caller adds)
		{
			size_t oldCount = atomic::load(m_freelist.m_countAndState);
			for (;;)
			{
				COGS_ASSERT((state)(oldCount & 0b111) == state::current);
				size_t actualCount = oldCount >> 3;
				if (actualCount == m_batchSize)
					return -1;
				if (actualCount != 0)
					return 1;
				if (atomic::compare_exchange(m_freelist.m_countAndState, (size_t)state::floating, oldCount, oldCount))
					return 0;
			}
		}

		bool set_adding_state() volatile // true == success, false == count was already at max so free it instead
		{
			size_t oldCount = atomic::load(m_freelist.m_countAndState);
			for (;;)
			{
				size_t actualCount = oldCount >> 3;
				if (actualCount == m_batchSize)
					return false;
				COGS_ASSERT(actualCount != 0);
				state oldState = (state)(oldCount & 0b111);
				COGS_ASSERT(oldState == state::current || oldState == state::floating);
				size_t newCount = (actualCount << 3) | (size_t)state::adding;
				if (atomic::compare_exchange(m_freelist.m_countAndState, newCount, oldCount, oldCount))
					return true;
			}
		}

		bool is_set_adding_state() volatile
		{
			size_t oldCount = atomic::load(m_freelist.m_countAndState);
			state oldState = (state)(oldCount & 0b111);
			COGS_ASSERT(oldState == state::adding || oldState == state::removing);
			return oldState == state::adding;
		}

		bool set_added_state() volatile // true == success (do nothing), false == count was already at max so start mid-removal
		{
			size_t oldCount = atomic::load(m_freelist.m_countAndState);
			for (;;)
			{
				state oldState = (state)(oldCount & 0b111);
				if (oldState == state::removing) // If removed before we could update the state.
					return true;
				COGS_ASSERT(oldState == state::adding);
				size_t actualCount = oldCount >> 3;
				if (actualCount == m_batchSize)
					return false;
				size_t newCount = (actualCount << 3) | (size_t)state::in_list;
				if (atomic::compare_exchange(m_freelist.m_countAndState, newCount, oldCount, oldCount))
					return true;
			}
		}

		bool set_current_state() volatile // true == success, false == already mid-removing this batch
		{
			size_t oldCount = atomic::load(m_freelist.m_countAndState);
			for (;;)
			{
				size_t oldState = oldCount & 0b111;
				COGS_ASSERT(oldState == (size_t)state::adding || oldState == (size_t)state::in_list);
				size_t actualCount = oldCount >> 3;
				if (actualCount == m_batchSize)
					return false;
				size_t newCount = (actualCount << 3) | (size_t)state::in_list;
				if (atomic::compare_exchange(m_freelist.m_countAndState, newCount, oldCount, oldCount))
					return true;
			}
		}

		void deallocate_inner(node_t& n) volatile
		{
			freelist_content_t oldContent = atomic::load(m_freelist);
			for (;;)
			{
				COGS_ASSERT(oldContent.m_countAndState >> 3 != m_batchSize);
				freelist_content_t newContent;
				n.get_second() = &oldContent.m_head->get_first();
				newContent.m_head = &n;
				if (oldContent.m_countAndState == (size_t)state::floating) // auto-add
				{
					newContent.m_countAndState = (1 << 3) | (size_t)state::adding;
					if (!atomic::compare_exchange(m_freelist, newContent, oldContent, oldContent))
						continue;
				}
				else if (oldContent.m_countAndState == (((m_batchSize - 1) << 3) | (size_t)state::in_list)) // auto-remove
				{
					// Becoming full, and in the list.  Mid-remove it.
					newContent.m_countAndState = (m_batchSize << 3) | (size_t)state::removing;
					if (!atomic::compare_exchange(m_freelist, newContent, oldContent, oldContent))
						continue;
					volatile this_t& batchAllocatorVolatile = m_batchAllocator;
					batchAllocatorVolatile.mid_remove(*const_cast<batch*>(this));
					break;
				}
				else
				{
					newContent.m_countAndState = oldContent.m_countAndState + (1 << 3);
					if (!atomic::compare_exchange(m_freelist, newContent, oldContent, oldContent))
						continue;
					break;
				}
				volatile this_t& batchAllocatorVolatile = m_batchAllocator;
				batchAllocatorVolatile.insert(*this);
				break;
			}
		}

		entry* allocate()
		{
			entry* result = 0;
			if (m_curPos < m_batchSize)
			{
				result = &m_nodes[m_curPos].get_first();
				result->m_batch = this;
				++m_curPos;
			}
			else if (!!m_freelist.m_head)
			{
				result = &(m_freelist.m_head->get_first());
				entry* e = m_freelist.m_head->get_second();
				m_freelist.m_head = reinterpret_cast<node_t*>(static_cast<placement_t*>(e));
				COGS_ASSERT(m_freelist.m_countAndState >= 0b1000);
				m_freelist.m_countAndState -= 0b1000;
				result->m_batch = this;
			}
			return result;
		}

		entry* allocate() volatile
		{
			entry* result = 0;
			size_t oldPos = atomic::load(m_curPos);
			while (oldPos < m_batchSize)
			{
				size_t pos = oldPos;
				if (!atomic::compare_exchange(m_curPos, oldPos + 1, oldPos, oldPos))
					continue;
				result = &const_cast<node_t*>(&(m_nodes[pos]))->get_first();
				result->m_batch = const_cast<batch*>(this);
				return result;
			}
			freelist_content_t oldContent;
			auto acquire_hazard = [&]
			{
				oldContent = atomic::load(m_freelist);
				return oldContent.m_head;
			};
			hazard::pointer hazardPtr(m_hazard, acquire_hazard);
			ptr<node_t> oldHead = oldContent.m_head;
			while (!!oldHead)
			{
				state oldState = (state)(oldContent.m_countAndState & 0b111);
				COGS_ASSERT(oldState != state::floating);
				COGS_ASSERT(oldState != state::adding);
				COGS_ASSERT(oldState != state::in_list);
				if (oldState == state::removing)
				{
					// This indicates mid-removal has started right before this batch was
					// removed from the list to be restored to current.  If so, let it go.
					if (hazardPtr.release())
						deallocate_inner(*oldHead);
					break;
				}
				COGS_ASSERT(oldContent.m_countAndState >= 0b1000);
				freelist_content_t newContent{ reinterpret_cast<node_t*>(&oldHead->get_second()), oldContent.m_countAndState - 0b1000 };
				if (!atomic::compare_exchange(m_freelist, newContent, oldContent, oldContent))
				{
					if (oldHead == oldContent.m_head)
						continue;
					if (!hazardPtr.release())
					{
						deallocate_inner(*oldHead);
						oldHead = hazardPtr.acquire(m_hazard, oldContent.m_head, acquire_hazard);
						continue;
					}
					// It was allocated then deallocated.  We can have it.
				}
				else
				{
					bool b = hazardPtr.release();
					COGS_ASSERT(!b);
				}
				result = &oldHead->get_first();
				result->m_batch = const_cast<batch*>(this);
				break;
			}
			return result;
		}

		bool deallocate(entry& e)
		{
			bool result = false;
			node_t* n = reinterpret_cast<node_t*>(static_cast<placement_t*>(&e));
			n->get_second() = &m_freelist.m_head->get_first();
			m_freelist.m_head = n;
			size_t oldCount = m_freelist.m_countAndState;
			state oldState = (state)(oldCount & 0b111);
			oldCount >>= 3;
			state newState;
			bool adding = oldState == state::floating;// && !oldCount
			size_t newCount = oldCount + 1;
			if (adding)
			{
				newState = state::in_list;
				m_batchAllocator.insert(*this);
			}
			else if (oldState == state::in_list)
			{
				if (newCount != m_batchSize)
					newState = oldState;
				else
				{
					newState = state::removing;
					result = true;
				}
			}
			else
			{
				newState = state::current;
				COGS_ASSERT(oldState == state::current);
			}
			m_freelist.m_countAndState = (newCount << 3) | (size_t)newState;
			return result;
		}

		void deallocate(entry& e) volatile
		{
			node_t* n = reinterpret_cast<node_t*>(&e);
			if (m_hazard.release(n))
				deallocate_inner(*n);
		}
	};

	batch* allocate_batch()
	{
		batch* result;
		size_t usableSize;
		size_t allocSize = minimum_batch_size * sizeof(node_t);
		result = m_memoryManager.template allocate_with_header<batch, alignof(node_t)>(allocSize, &usableSize);
		node_t* nodes = placement_with_header<batch, node_t>::from_header(result);
		size_t usableNodeCount = usableSize / sizeof(node_t);
		return new (result) batch(*this, nodes, usableNodeCount);
	}

	batch* allocate_batch() volatile
	{
		batch* result;
		size_t usableSize;
		size_t allocSize = minimum_batch_size * sizeof(node_t);
		result = m_memoryManager.template allocate_with_header<batch, alignof(node_t)>(allocSize, &usableSize);
		node_t* nodes = placement_with_header<batch, node_t>::from_header(result);
		size_t usableNodeCount = usableSize / sizeof(node_t);
		return new (result) batch(*const_cast<this_t*>(this), nodes, usableNodeCount);
	}

	typedef typename versioned_ptr<batch>::version_t version_t;

	versioned_ptr<batch> m_head;
	batch* m_currentBatch = nullptr;
	mutable hazard m_hazard; // A primary hazard to prevent release of a link or removal from current
	serial_defer_guard_t<batch> m_guard;

	void release_inner(volatile batch& lnk) volatile
	{
		if (lnk.m_removingFromCurrent)
		{
			lnk.m_removingFromCurrent = false;
			// At this point, the only references to this batch will be due to deallocations.
			int i = lnk.set_floating_state();
			if (!i)
				return;
			if (i != -1)
			{
				// Add to list.  If, after finishing adding to the list, the batch is full, we need to turn around and mid-remove it.
				insert(lnk);
				return;
			}
		}
		m_memoryManager.destruct_deallocate_type(&lnk);
	}

	void complete_lingering(ptr<batch>& oldHead, version_t& oldHeadVersion, hazard::pointer& hazardPtr, version_t& oldHeadPrevVersion) volatile
	{
		ptr<volatile batch> currentHazard = oldHead;
		ptr<batch> oldHeadPrevSaved;
		ptr<batch> oldHeadPrev;
		bool keepHeadPrev = false;
		oldHead = m_head.get_ptr(oldHeadVersion);
		while (!!oldHead)
		{
			if (currentHazard != oldHead)
			{
				if (hazardPtr.release())
					release_inner(*currentHazard);
				for (;;)
				{
					version_t oldHeadVersion2;
					hazardPtr.bind_unacquired(m_hazard, oldHead.get_ptr());
					ptr<batch> oldHead2 = m_head.get_ptr(oldHeadVersion2);
					if (oldHeadVersion2 == oldHeadVersion && hazardPtr.validate())
						break;
					oldHead = oldHead2;
					oldHeadVersion = oldHeadVersion2;
					//continue;
				}
				if (!oldHead)
					break;
				currentHazard = oldHead;
			}
			ptr<volatile batch> oldHeadVolatile = oldHead;
			if (keepHeadPrev)
				keepHeadPrev = false;
			else
				oldHeadPrev = oldHeadVolatile->m_listPrev.get_ptr(oldHeadPrevVersion);
			// Make sure this represents a value that was present while still head element.
			version_t oldHeadVersion2;
			ptr<batch> oldHead2 = m_head.get_ptr(oldHeadVersion2);
			if (oldHeadVersion != oldHeadVersion2)
			{
				oldHead = oldHead2;
				oldHeadVersion = oldHeadVersion2;
				continue;
			}
			if (!oldHeadPrev)
				return; // Returns acquired hazardPtr
			COGS_ASSERT(oldHead != oldHeadPrev);
			bool isRemoving = (bool)oldHeadPrev.get_mark();
			if (!isRemoving)
			{
				// Insert
				if (m_head.versioned_exchange(oldHeadPrev, oldHeadVersion, oldHead))
					oldHead = oldHeadPrev;
			}
			else
			{
				// Remove
				// There is a race here with mid-removal of the next element that could
				// results in that removed element being set as the head by us.
				// Since both are claimed by the thread to set the mark, and both threads will call
				// complete_lingering again before returning, the race seems to be addressed.
				ptr<batch> oldHeadNext = oldHeadVolatile->m_listNext; // Might race with change of m_listNext by mid-removal

				// Before we can put m_head->m_listNext at the head of the list, we need to clear it's prev ptr.
				// First, we need to acquire a hazard for oldHeadNext.
				ptr<batch> oldHeadNextPrev;
				version_t oldHeadNextPrevVersion;
				ptr<volatile batch> oldHeadNextVolatile;
				hazard::pointer hazardPtr2;
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
						if (hazardPtr.release())
							release_inner(*currentHazard);
						return;
					}

					hazardPtr2.bind_unacquired(m_hazard, oldHeadNext.get_ptr());
					ptr<batch> oldHeadNext2 = oldHeadVolatile->m_listNext;
					if (oldHeadNext != oldHeadNext2 || !hazardPtr2.validate())
					{
						oldHeadNext = oldHeadNext2;
						continue;
					}
					oldHeadNextVolatile = oldHeadNext;
					oldHeadNextVolatile->m_listPrev.get(oldHeadNextPrev, oldHeadNextPrevVersion);

					// Make sure oldHeadNext hasn't changed.
					oldHeadNext2 = oldHead->m_listNext;
					if (oldHeadNext == oldHeadNext2)
						break;
					if (hazardPtr2.release())
						release_inner(*oldHeadNext);
					oldHeadNext = oldHeadNext2;
					//continue;
				}
				if (retry)
					continue;

				// Make sure oldHead hasn't changed
				oldHead2 = m_head.get_ptr(oldHeadVersion2);
				if (oldHeadVersion2 != oldHeadVersion)
				{
					oldHead = oldHead2;
					oldHeadVersion = oldHeadVersion2;
					if (hazardPtr2.release())
						release_inner(*oldHeadNext);
					continue;
				}
				// Clear previous pointer, but leave the mark if present.
				size_t oldHeadNextPrevMark = oldHeadNextPrev.get_mark();
				oldHeadNextVolatile->m_listPrev.versioned_exchange((batch*)oldHeadNextPrevMark, oldHeadNextPrevVersion, oldHeadNextPrev);
				if (m_head.versioned_exchange(oldHeadNext, oldHeadVersion, oldHead))
				{
					if (hazardPtr.release())
						release_inner(*currentHazard);
					oldHeadPrev.set_to_mark(oldHeadNextPrevMark);
					oldHeadPrevVersion = oldHeadNextPrevVersion;
					keepHeadPrev = true;
					hazardPtr = std::move(hazardPtr2);
					currentHazard = oldHeadNext;
					oldHead = oldHeadNext;
					continue;
				}
				if (hazardPtr2.release())
					release_inner(*oldHeadNext);
			}
			//continue;
		}
		if (!oldHead && hazardPtr.release())
			release_inner(*currentHazard);
	}

	void mid_remove(batch& lnk) volatile
	{
		// Won't get here unless auto-destruct mode was set, in which case there will not be an add pending...
		volatile batch& lnkVolatile = lnk;
		version_t lnkPrevVersion;
		ptr<batch> lnkPrev = lnkVolatile.m_listPrev.get_ptr(lnkPrevVersion);
		bool isRemoving = (bool)lnkPrev.get_mark();
		if (!isRemoving && lnkVolatile.m_listPrev.versioned_exchange(lnkPrev.get_marked(1), lnkPrevVersion))
		{
			if (!!lnkPrev)
			{
				ptr<batch> lnkNext = lnkVolatile.m_listNext;
				ptr<volatile batch> lnkPrevVolatile = lnkPrev;
				lnkPrevVolatile->m_listNext.compare_exchange(lnkNext, &lnk);
				if (!!lnkNext)
				{
					ptr<volatile batch> lnkNextVolatile = lnkNext;
					lnkNextVolatile->m_listPrev.compare_exchange(lnkPrev, &lnk);
				}
			}
			ptr<batch> oldHead;
			version_t oldHeadVersion;
			version_t oldHeadPrevVersion;
			hazard::pointer hazardPtr;
			complete_lingering(oldHead, oldHeadVersion, hazardPtr, oldHeadPrevVersion);
			if (hazardPtr.release())
				release_inner(*oldHead);
			if (m_hazard.release(&lnk))
				release_inner(lnk);
		}
	}

	void release_insert_guard() volatile
	{
		ptr<batch> oldHead;
		version_t oldHeadVersion;
		hazard::pointer hazardPtr;
		hazard::pointer hazardPtr2;
		version_t oldHeadPrevVersion;
		for (;;)
		{
			ptr<batch> lnk = m_guard.release();
			if (!lnk)
				break;
			if (!lnk->is_set_adding_state())
			{
				release_inner(*lnk);
				continue;
			}
			hazardPtr2.bind_acquired(m_hazard, lnk.get_ptr());
			bool done = false;
			for (;;)
			{
				complete_lingering(oldHead, oldHeadVersion, hazardPtr, oldHeadPrevVersion);
				if (done)
				{
					if (hazardPtr.release())
						release_inner(*oldHead);
					break;
				}
				ptr<volatile batch> lnkVolatile = lnk;
				lnkVolatile->m_listPrev.release();
				lnk->m_listNext = oldHead;
				if (!oldHead)
				{
					if (m_head.versioned_exchange(lnk, oldHeadVersion))
						break;
					continue;
				}
				// Set prev link of current head to this link.  Then complete lingering.
				ptr<volatile batch> oldHeadVolatile = oldHead;
				done = oldHeadVolatile->m_listPrev.versioned_exchange(lnk, oldHeadPrevVersion);
				continue;
			}
			if (!lnk->set_added_state())
				mid_remove(*lnk);
			if (hazardPtr2.release())
				release_inner(*lnk);
		}
	}

	void insert(batch& lnk)
	{
		lnk.m_listPrev.unversioned_release();
		lnk.m_listNext = m_head.get_ptr();
		if (m_head.get_ptr())
			m_head->m_listPrev = &lnk;
		m_head = &lnk;
	}

	void insert(volatile batch& lnk) volatile
	{
		m_guard.begin_guard();
		m_guard.add(*const_cast<batch*>(&lnk));
		release_insert_guard();
	}

	batch* remove_inner()
	{
		batch* lnk = m_head.get_ptr();
		if (!!lnk)
		{
			lnk->m_listPrev.unversioned_set_to_mark(1);
			m_head.unversioned_set(lnk->m_listNext.get_ptr());
			if (!!m_head)
				m_head->m_listPrev.unversioned_release();
		}
		return lnk;
	}

	batch* remove()
	{
		batch* lnk = remove_inner();
		if (!!lnk)
			lnk->m_freelist.m_countAndState = (lnk->m_freelist.m_countAndState & ~0b111) | (size_t)batch::state::current;
		return lnk;
	}

	batch* remove(hazard::pointer& hazardPtr) volatile
	{
		m_guard.begin_guard();
		// Can be interrupted by other head removals, a single insert, or a single mid-removal.
		bool done = false;
		batch* result = m_guard.remove(); // prefer to snag an element in the process of being inserted.
		if (!!result)
		{
			// Should not fail to set to current state.  It would not be removable until it was added.
			bool b = result->set_current_state();
			COGS_ASSERT(b);
		}
		else
		{
			ptr<batch> oldHead;
			version_t oldHeadVersion;
			version_t oldHeadPrevVersion;
			complete_lingering(oldHead, oldHeadVersion, hazardPtr, oldHeadPrevVersion);
			while (!!oldHead)
			{
				ptr<volatile batch> oldHeadVolatile = oldHead;
				done = oldHeadVolatile->m_listPrev.versioned_exchange((batch*)1, oldHeadPrevVersion); // Try to put a mark in oldHead->get_prev_link()
				if (done)
					result = oldHead.get_ptr();
				complete_lingering(oldHead, oldHeadVersion, hazardPtr, oldHeadPrevVersion);
				if (!done)
					continue;
				if (!!result && !result->set_current_state()) // If released, let it go.
					continue;
				if (hazardPtr.release())
					release_inner(*oldHead);
				break;
			}
		}
		release_insert_guard();
		return result;
	}

	void remove(batch& lnk)
	{
		if (!!lnk.m_listPrev)
			lnk.m_listPrev->m_listNext = lnk.m_listNext;
		if (!!lnk.m_listNext)
			lnk.m_listNext->m_listPrev = lnk.m_listPrev;
		if (m_head == &lnk)
			m_head = lnk.m_listNext;
	}

	batch_allocator(const this_t&) = delete;
	batch_allocator& operator=(const this_t&) = delete;
	batch_allocator& operator=(this_t& src) = delete;

public:
	batch_allocator()
	{ }

	batch_allocator(this_t&& src)
		: m_memoryManager(std::move(src.m_memoryManager)),
		m_head(src.m_head),
		m_currentBatch(src.m_currentBatch)
	{
		COGS_ASSERT(src.m_guard.is_free());
		COGS_ASSERT(src.m_hazard.is_empty());

		m_currentBatch = src.m_currentBatch;
		src.m_currentBatch = nullptr;
	}

	~batch_allocator()
	{
		COGS_ASSERT(m_guard.is_free());
		COGS_ASSERT(m_hazard.is_empty());
		batch* b = m_currentBatch;
		if (!b)
			b = remove_inner();
		while (!!b)
		{
			COGS_ASSERT(b->m_curPos == (b->m_freelist.m_countAndState >> 3));
			m_memoryManager.destruct_deallocate_type(b);
			b = remove_inner();
		}
	}

	T* allocate()
	{
		entry* result = nullptr;
		for (;;)
		{
			if (!m_currentBatch)
			{
				m_currentBatch = remove();
				if (!m_currentBatch)
					m_currentBatch = allocate_batch();
			}
			result = m_currentBatch->allocate();
			if (!!result)
				break;
			COGS_ASSERT(m_currentBatch->m_freelist.m_countAndState == 0);
			m_currentBatch->m_freelist.m_countAndState = (size_t)batch::state::floating;
			m_currentBatch = nullptr;
			//continue;
		}
		return reinterpret_cast<T*>(static_cast<placement_t*>(result));
	}

	T* allocate() volatile
	{
		entry* result = nullptr;
		hazard::pointer hazardPtr;
		hazard::pointer hazardPtr2;
		ptr<batch> nextCandidateBatch;
		ptr<batch> currentBatch = atomic::load(m_currentBatch);
		ptr<batch> currentHazard;
		for (;;)
		{
			if (currentBatch != currentHazard)
			{
				if (hazardPtr.release())
					release_inner(*currentHazard);
				currentBatch = hazardPtr.acquire(m_hazard, currentBatch.get_ptr(), m_currentBatch);
				currentHazard = currentBatch;
			}
			if (!currentBatch)
			{
				if (!nextCandidateBatch)
				{
					nextCandidateBatch = remove(hazardPtr2);
					if (!nextCandidateBatch)
					{
						nextCandidateBatch = allocate_batch();
						hazardPtr2.bind_acquired(m_hazard, nextCandidateBatch.get_ptr());
					}
				}
				if (!atomic::compare_exchange(m_currentBatch, nextCandidateBatch.get_ptr(), (batch*)nullptr, currentBatch.get_ptr_ref()))
					continue;
				currentBatch = nextCandidateBatch;
				nextCandidateBatch.release();
				hazardPtr = std::move(hazardPtr2);
			}

			ptr<volatile batch> currentBatchVolatile = currentBatch;
			result = currentBatchVolatile->allocate();
			if (!!result)
				break;
			if (!atomic::compare_exchange(m_currentBatch, (batch*)nullptr, currentBatch.get_ptr(), currentBatch.get_ptr_ref()))
				continue;
			currentBatch->m_removingFromCurrent = true;
			// If remove succeeded, it must have been the value we had in currentBatch (presently in currentHazard).
			// But, we have a hazard that refers to currentHazard.  So, when we try to release it, it should not succeedd yet.
			COGS_ASSERT(currentBatch == currentHazard);
			bool b = m_hazard.release(currentBatch.get_ptr());
			COGS_ASSERT(!b);
			if (hazardPtr.release())
				release_inner(*currentHazard);
			currentBatch = nullptr;// hazardPtr.acquire(m_hazard, m_currentBatch);
			continue;
		}
		if (hazardPtr.release())
			release_inner(*currentHazard);
		if (hazardPtr2.release())
			release_inner(*nextCandidateBatch);
		return reinterpret_cast<T*>(static_cast<placement_t*>(result));
	}

	void deallocate(T* p)
	{
		if (!!p)
		{
			entry* e = reinterpret_cast<entry*>(p);
			batch* b = e->m_batch;
			if (b->deallocate(*e))
			{
				remove(*b);
				m_memoryManager.destruct_deallocate_type(b);
			}
		}
	}

	void deallocate(T* p) volatile
	{
		if (!!p)
		{
			entry* e = reinterpret_cast<entry*>(p);
			volatile batch* b = e->m_batch;
			// Wrap with a hazard to address a race in which mid-removal could be interupted by normal removal.
			hazard::pointer hazardPtr;
			hazardPtr.bind_acquired(m_hazard, b);
			b->deallocate(*e);
			if (hazardPtr.release())
				release_inner(*b);
		}
	}

	memory_manager_t& get_memory_manager() const { return m_memoryManager; }
	volatile memory_manager_t& get_memory_manager() const volatile { return m_memoryManager; }
};


template <typename T, class memory_manager_t>
class batch_allocator<T, 1, memory_manager_t> : public default_allocator<T, memory_manager_t>
{
};



}


#endif
