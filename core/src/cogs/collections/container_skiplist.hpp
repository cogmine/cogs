//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_SKIPLIST
#define COGS_HEADER_COLLECTION_CONTAINER_SKIPLIST

#include <initializer_list>
#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_int.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief Default container_skiplist payload type
/// @tparam key_t The type used to compare elements.
template <typename key_t>
class default_container_skiplist_payload
{
private:
	typedef default_container_skiplist_payload<key_t> this_t;

	key_t m_key;

public:
	default_container_skiplist_payload(const this_t& src)
		: m_key(src.m_key)
	{ }

	explicit default_container_skiplist_payload(const key_t& key)
		: m_key(key)
	{ }

	this_t& operator=(const this_t& src)
	{
		m_key = src.m_key;
		return *this;
	}

	const key_t& get_key() const { return m_key; }
};


template <bool allow_multiple, typename key_t, class payload_t = default_container_skiplist_payload<key_t>, class comparator_t = default_comparator, class allocator_type = default_allocator>
class container_skiplist;


/// @ingroup LockFreeCollections
/// @brief A skip-list container collection.
/// @tparam key_t The type used to compare elements.
/// @tparam payload_t Type to contain, if different from key_t.  default_container_skiplist_payload<key_t>
template <typename key_t, class payload_t = default_container_skiplist_payload<key_t>, class comparator_t = default_comparator, class allocator_type = default_allocator>
class container_skiplist_base
{
public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

protected:
	enum class link_mode
	{
		inserting = 0,
		normal = 1,
		removing = 2,
		removing_next = 3
	};

	static constexpr size_t max_height = (sizeof(size_t) * 8) - 1;
	typedef range_to_int_t<0, max_height> height_t;

	typedef container_skiplist_base<key_t, payload_t, comparator_t, allocator_type> this_t;

	class sentinel_link_t;

	class link_t : public object
	{
	public:
		class links_t
		{
		public:
			link_mode m_mode;
			rcptr<link_t> m_next;
			rcptr<link_t> m_prev;
		};

		typedef transactable<links_t> transactable_t;
		typedef typename transactable_t::read_token read_token;
		typedef typename transactable_t::write_token write_token;

	private:
		link_t(link_t&) = delete;
		link_t& operator=(const link_t&) = delete;

	public:
		ptr<transactable_t> m_links;
		const height_t m_height;
		const bool m_isSentinel;

		// Primary mode starts as link_mode::inserting at the start of a volatile insert.
		// As the last insert is completed for this link, the primary mode is changed to link_mode::normal.
		// A remove will change the primary mode to link_mode::removing.
		// If an insert finds the mode has been changed to link_mode::removing, the insert owns the removal.
		alignas (atomic::get_alignment_v<link_mode>) link_mode m_primaryMode;

		explicit link_t(height_t height, link_mode linkMode, bool isSentinel = false)
			: m_height(height),
			m_isSentinel(isSentinel),
			m_primaryMode(linkMode)
		{
			height_t heightPlusOne = height + 1;
			m_links = allocator_container<default_allocator>::template allocate_type<transactable_t>(heightPlusOne);
			for (size_t i = 0; i <= height; i++)
				new (m_links.get_ptr() + i) transactable_t;
			for (height_t i = 0; i <= height; i++)
				m_links[i]->m_mode = linkMode;
		}

		~link_t()
		{
			if (!!m_links)
				default_allocator::destruct_deallocate_type<transactable_t>(m_links, m_height + 1);
		}

		bool is_sentinel() const { return m_isSentinel; }
		bool is_sentinel() const volatile { return const_cast<const link_t*>(this)->m_isSentinel; }

		bool is_removed() { return m_primaryMode == link_mode::removing; }
		bool is_removed() volatile { link_mode primaryMode; atomic::load(m_primaryMode, primaryMode); return primaryMode == link_mode::removing; }

		bool is_removed(height_t height) { return m_links[height]->m_mode == link_mode::removing; }
		bool is_removed(height_t height) volatile { read_token rt; return !begin_read_and_complete(rt, false, height); }

		bool remove()
		{
			bool result = false;
			if (m_primaryMode != link_mode::removing)
			{
				m_primaryMode = link_mode::removing;
				height_t level = m_height;
				for (;;)
				{
					remove(level);
					if (!level)
						break;
					--level;
				}
				result = true;
			}
			return result;
		}

		bool remove(height_t level)
		{
			COGS_ASSERT(level <= m_height);
			if (m_links[level]->m_mode == link_mode::removing)
				return false;

			COGS_ASSERT(!is_sentinel());

			m_links[level]->m_mode = link_mode::removing;
			link_t* nextPtr = m_links[level]->m_next.get_ptr();
			link_t* prevPtr = m_links[level]->m_prev.get_ptr();
			nextPtr->m_links[level]->m_prev = std::move(m_links[level]->m_prev);
			prevPtr->m_links[level]->m_next = std::move(m_links[level]->m_next);
			return true;
		}

		bool remove() volatile
		{
			bool wasLast;
			return remove(wasLast);
		}

		bool remove(bool& wasLast) volatile
		{
			bool result = false;
			link_mode oldLinkMode;
			atomic::load(m_primaryMode, oldLinkMode);
			for (;;)
			{
				if (oldLinkMode == link_mode::removing)
				{
					remove_from_level(wasLast, 0); // Ensure removed at least from bottom level before return.
					break;
				}

				if (!atomic::compare_exchange(m_primaryMode, link_mode::removing, oldLinkMode, oldLinkMode))
					continue;

				if (oldLinkMode == link_mode::normal)
					remove_from_all_levels(wasLast);
				result = true;
				break;
			}

			return result;
		}

		void remove_from_all_levels(bool& wasLast) volatile
		{
			height_t level = const_cast<const link_t*>(this)->m_height;
			for (;;)
			{
				remove_from_level(wasLast, level);
				if (!level)
					break;
				--level;
			}

			remove_cleanup();
		}

		void remove_cleanup() volatile
		{
			// This last fix up avoids the issue where multiple equal keys are removed simultaneously,
			// having been listed in different orders at different level.  Multiple removed nodes
			// are ensured not to have forward links to each other.
			height_t level = const_cast<const link_t*>(this)->m_height;
			for (;;)
			{
				remove_cleanup(level);
				if (!level)
					break;
				--level;
			}
		}

		void remove_cleanup(height_t level) volatile
		{
			COGS_ASSERT(level <= const_cast<const link_t*>(this)->m_height);
			read_token rt;
			for (;;)
			{
				m_links[level].begin_read(rt);
				read_token rt_next;
				rt->m_next->m_links[level].begin_read(rt_next);
				if (rt_next->m_mode == link_mode::removing)
				{
					write_token wt;
					if (!m_links[level].promote_read_token(rt, wt))
						continue;
					wt->m_next = rt_next->m_next;
					COGS_ASSERT(!!wt->m_next.get_desc());

					m_links[level].end_write(wt);
					continue;
				}

				read_token rt_prev;
				rt->m_prev->m_links[level].begin_read(rt_prev);
				if (rt_prev->m_mode == link_mode::removing)
				{
					write_token wt;
					if (!m_links[level].promote_read_token(rt, wt))
						continue;
					wt->m_prev = rt_prev->m_prev;
					m_links[level].end_write(wt);
					continue;
				}
				break;
			}
		}

		bool remove_from_level(bool& wasLast, height_t level) volatile
		{
			COGS_ASSERT(level <= const_cast<const link_t*>(this)->m_height);
			bool result = false;
			for (;;)
			{
				read_token rt;
				if (!begin_read_and_complete(rt, false, level))
					break;
				rcptr<volatile link_t> prev = rt->m_prev;
				read_token rt_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (!m_links[level].is_current(rt))
					continue;
				if (rt_prev->m_next.get_ptr() != this)
				{
					// Could be here because prev->m_next contains a link being inserted.
					// If assisted to complete, m_prev will get updated.
					rcptr<volatile link_t> prevNext = rt_prev->m_next;
					prevNext->complete(level);
					COGS_ASSERT(!m_links[level].is_current(rt));
					continue;
				}
				bool maybeLast = false;
				if (!level)
				{
					ptr<volatile link_t> prev_prev = rt_prev->m_prev.get_ptr();
					maybeLast = (prev_prev.get_ptr() == this);
				}
				write_token wt_prev;
				COGS_ASSERT(level <= prev.template const_cast_to<const link_t>()->m_height);
				if (!prev->m_links[level].promote_read_token(rt_prev, wt_prev))
					continue;
				wt_prev->m_mode = link_mode::removing_next;
				if (!prev->m_links[level].end_write(wt_prev))
					continue;
				if (!level)
					wasLast = maybeLast;
				result = true;
				prev->m_links[level].begin_read(rt_prev);
				if (rt_prev->m_mode == link_mode::removing_next) // If not spontaneously completed by another thread.
					prev->complete_remove_next(level, rt_prev);
				break;
			}
			return result;
		}

		void complete_remove(height_t level, read_token& rt) volatile
		{
			rcptr<volatile link_t> prev = rt->m_prev;
			COGS_ASSERT(!!prev);
			rcptr<volatile link_t> next = rt->m_next;
			COGS_ASSERT(level <= prev.template const_cast_to<const link_t>()->m_height);
			COGS_ASSERT(level <= next.template const_cast_to<const link_t>()->m_height);
			for (;;)
			{
				read_token rt_next;
				next->m_links[level].begin_read(rt_next);
				if (rt_next->m_mode == link_mode::inserting)
				{
					next->complete_insert(level, rt_next);
					next->m_links[level].begin_read(rt_next);
				}
				rcptr<volatile link_t> next_prev = rt_next->m_prev;
				if (next_prev.get_ptr() == this)
				{
					write_token wt_next;
					if (!next->m_links[level].promote_read_token(rt_next, wt_next))
						continue;

					wt_next->m_prev = prev.template const_cast_to<link_t>();
					if (!next->m_links[level].end_write(wt_next))
						continue;
				}
				break;
			}

			for (;;)
			{
				read_token rt_prev;
				prev->m_links[level].begin_read(rt_prev);
				if (rt_prev->m_next.get_ptr() == this) // otherwise, we're done here.
				{
					COGS_ASSERT(rt_prev->m_mode == link_mode::removing_next);
					write_token wt_prev;
					if (!prev->m_links[level].promote_read_token(rt_prev, wt_prev))
						continue;
					wt_prev->m_next = rt->m_next;
					COGS_ASSERT(!!wt_prev->m_next.get_desc());
					wt_prev->m_mode = link_mode::normal;
					if (!prev->m_links[level].end_write(wt_prev))
						continue;
				}
				break;
			}
		}

		void complete_remove_next(height_t level, read_token& rt) volatile
		{
			rcptr<volatile link_t> next = rt->m_next;
			COGS_ASSERT(level <= next.template const_cast_to<const link_t>()->m_height);
			for (;;)
			{
				read_token rt_next;
				next->m_links[level].begin_read(rt_next);
				if (rt_next->m_mode == link_mode::removing)
				{
					next->complete_remove(level, rt_next);
					break;
				}
				if (rt_next->m_mode == link_mode::removing_next)
				{
					next->complete_remove_next(level, rt_next);
					continue;
				}
				if (rt_next->m_mode == link_mode::inserting)
				{
					next->complete_insert(level, rt_next);
					continue;
				}
				write_token wt_next;
				if (!next->m_links[level].promote_read_token(rt_next, wt_next))
					continue;
				wt_next->m_mode = link_mode::removing;
				next->m_links[level].end_write(wt_next);
				//continue;
			}
		}

		void complete_insert(height_t level, read_token& rt) volatile
		{
			COGS_ASSERT(level <= const_cast<const link_t*>(this)->m_height);
			COGS_ASSERT(rt->m_mode == link_mode::inserting);

			// rt->m_inserting is known to be set.  We know the next time this link is written to, it's
			// going to be to remove the inserting flag, so we don't need a retry loop.
			rcptr<volatile link_t> next = rt->m_next;
			COGS_ASSERT(level <= next.template const_cast_to<const link_t>()->m_height);
			for (;;)
			{
				read_token rt_next;
				next->m_links[level].begin_read(rt_next);
				rcptr<volatile link_t> nextPrev = rt_next->m_prev;
				if (!m_links[level].is_current(rt))
					break;

				COGS_ASSERT((rt_next->m_mode == link_mode::normal) || (rt_next->m_mode == link_mode::removing_next));

				if (nextPrev.get_ptr() == this)
					break;

				write_token wt_next;
				if (!next->m_links[level].promote_read_token(rt_next, wt_next))
					continue;

				wt_next->m_prev = this_rcptr.template const_cast_to<link_t>();
				if (!next->m_links[level].end_write(wt_next))
					continue;
				break;
			}

			// No loop needed, because the next write will clear insert mode
			write_token wt;
			if (!!m_links[level].promote_read_token(rt, wt))
			{
				wt->m_mode = link_mode::normal;
				m_links[level].end_write(wt);
			}
		}

		void complete_insert(height_t level) volatile
		{
			COGS_ASSERT(level <= const_cast<const link_t*>(this)->m_height);
			read_token rt;
			m_links[level].begin_read(rt);
			if (rt->m_mode == link_mode::inserting)
				complete_insert(level, rt);
		}

		bool begin_read_and_complete(read_token& rt, bool returnTokenEvenIfRemoved, height_t level = 0) volatile // returns false this link was removed.
		{
			COGS_ASSERT(level <= const_cast<const link_t*>(this)->m_height);
			bool notRemoved = true;
			rt.release();
			for (;;)
			{
				m_links[level].begin_read(rt);
				if (rt->m_mode == link_mode::normal)
					break;

				if (rt->m_mode == link_mode::inserting)
				{
					complete_insert(level, rt);
					continue;
				}

				if (rt->m_mode == link_mode::removing_next)
				{
					complete_remove_next(level, rt);
					continue;
				}

				if (rt->m_mode == link_mode::removing)
				{
					notRemoved = false;
					complete_remove(level, rt);
					if (returnTokenEvenIfRemoved)
						m_links[level].begin_read(rt);
				}
				break;
			}
			COGS_ASSERT(!notRemoved || (rt->m_mode == link_mode::normal));
			return notRemoved;
		}

		void complete(height_t level) volatile
		{
			read_token rt;
			begin_read_and_complete(rt, false, level);
		}

		// Scan for the last equal element, and add after it.
		// Ensures elements appear in the list in the order inserted if equal.
		void insert_multi_inner(height_t level, const key_t& criteria, const rcref<link_t>& startFrom, bool& prevWasEqual, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> next;
			rcptr<link_t> prev = startFrom;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!!level)
				{
					rcptr<link_t> newLastAdjacent = next;
					insert_multi_inner(level - 1, criteria, prev, prevWasEqual, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
					COGS_ASSERT(!!prev->m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!next->m_links[level]->m_prev.get_desc());
					m_links[level]->m_mode = link_mode::normal;
					m_links[level]->m_next = std::move(next);
					m_links[level]->m_prev = std::move(prev);
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
				}
				break;
			}
		}

		// insert_multi_from_front_inner() is just like insert_multi_from_end_inner(), and should have the same performance characteristics,
		// however insert_multi_from_front_inner() is used if the criteria is < the first element, to optimize for prepend.
		void insert_multi_from_front_inner(bool& wasEmpty, height_t level, const key_t& criteria, const rcref<volatile link_t>& startFrom, bool& prevWasEqual, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			write_token wt;
			rcptr<volatile link_t> next;
			rcptr<volatile link_t> prev = startFrom;
			COGS_ASSERT(!!prev.get_desc());
			bool insertedLowerLevels = false;
			for (;;)
			{
				COGS_ASSERT(!!prev.get_desc());
				bool emptyList = false;
				if (!prev->begin_read_and_complete(rt, true, level))
				{
					prevWasEqual = false;
					COGS_ASSERT(!!rt->m_prev.get_desc());

					prev = rt->m_prev;
					COGS_ASSERT(!!prev);
					COGS_ASSERT(!!prev.get_desc());
					continue;
				}
				next = rt->m_next;
				COGS_ASSERT(!!next.get_desc());
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (!emptyList && (next.get_ptr() != sentinelPtr)) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							COGS_ASSERT(!!next.get_desc());
							prev = std::move(next);
							COGS_ASSERT(!!prev.get_desc());
							continue;
						}
					}
				}
				if (!!level && !insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = next;
					insert_multi_from_front_inner(wasEmpty, level - 1, criteria, prev.dereference(), prevWasEqual, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev.template const_cast_to<const link_t>()->m_height);
					if (!prev->m_links[level].promote_read_token(rt, wt))
						continue;
					volatile link_t* prevPtr = prev.get_ptr();
					bool mayBeEmpty = next.get_ptr() == prevPtr;
					next->complete_insert(level);
					m_links[level]->m_next = std::move(next).template const_cast_to<link_t>();
					m_links[level]->m_prev = prev.template const_cast_to<link_t>();
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
					wt->m_next = this_rcptr;
					COGS_ASSERT(!!wt->m_next.get_desc());
					COGS_ASSERT(!!prev.get_desc());
					if (!prevPtr->m_links[level].end_write(wt))
					{
						COGS_ASSERT(!!prev.get_desc());
						continue;
					}
					if (!level)
						wasEmpty = mayBeEmpty;
					complete(level);
				}
				break;
			}
		}

		void insert_multi_from_end_inner(bool& wasEmpty, height_t level, const key_t& criteria, const rcref<volatile link_t>& startFrom, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> next = startFrom;
			COGS_ASSERT(!!next.get_desc());
			bool insertedLowerLevels = false;
			for (;;)
			{
				COGS_ASSERT(!!next.get_desc());
				if (!next->begin_read_and_complete(rt, true, level))
				{
					COGS_ASSERT(!!rt->m_next.get_desc());
					next = rt->m_next;
					COGS_ASSERT(!!next.get_desc());
					continue;
				}
				COGS_ASSERT(!!next.get_desc());
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& prevCriteria = prev.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(criteria, prevCriteria))
						{
							COGS_ASSERT(!!prev.get_desc());
							next = std::move(prev);
							COGS_ASSERT(!!next.get_desc());
							continue;
						}
					}
				}
				if (!!level && !insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = prev;
					insert_multi_from_end_inner(wasEmpty, level - 1, criteria, next.dereference(), newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev.template const_cast_to<const link_t>()->m_height);
					if (!prev->m_links[level].promote_read_token(rt_prev, wt))
						continue;
					next->complete_insert(level);
					volatile link_t* prevPtr = prev.get_ptr();
					bool mayBeEmpty = prevPtr == next.get_ptr();
					m_links[level]->m_next = next.template const_cast_to<link_t>();
					m_links[level]->m_prev = std::move(prev).template const_cast_to<link_t>();
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
					wt->m_next = this_rcptr;
					COGS_ASSERT(!!wt->m_next.get_desc());
					if (!prevPtr->m_links[level].end_write(wt))
						continue;
					if (!level)
						wasEmpty = mayBeEmpty;
					complete(level);
				}
				break;
			}
		}

		// Scan for first equal element, removes all equal elements, inserts.
		rcptr<link_t> insert_replace_inner(height_t level, const key_t& criteria, const rcref<link_t>& startFrom, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> result;
			rcptr<link_t> next;
			rcptr<link_t> prev = startFrom;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(next.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
					{
						// If at the bottom level, remove matching element before inserting this.
						if (!comparator_t::is_less_than(criteria, next.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next->remove();
							result = next;
							continue;
						}
					}
				}
				else //if (!!level)
				{
					rcptr<link_t> newLastAdjacent = next;
					result = insert_replace_inner(level - 1, criteria, prev, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
					COGS_ASSERT(!!prev->m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!next->m_links[level]->m_prev.get_desc());
					m_links[level]->m_next = std::move(next);
					m_links[level]->m_prev = std::move(prev);
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
				}
				break;
			}
			return result;
		}

		// Scan for any equal element, and abort if already present.
		// Returns null if no matching element, otherwise returns matching element.
		rcptr<link_t> insert_unique_inner(height_t level, const key_t& criteria, const rcref<link_t>& startFrom, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> result;
			rcptr<link_t> next;
			rcptr<link_t> prev = startFrom;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = std::move(next);
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							result = std::move(next);
							break;
						}
					}
				}
				if (!!level)
				{
					rcptr<link_t> newLastAdjacent = next;
					result = insert_unique_inner(level - 1, criteria, prev, newLastAdjacent, sentinelPtr);
					if (!!result)
						break;
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
					COGS_ASSERT(!!prev->m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!next->m_links[level]->m_prev.get_desc());

					m_links[level]->m_next = std::move(next);
					m_links[level]->m_prev = std::move(prev);
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
				}
				break;
			}
			return result;
		}

		// If a volatile non-dupe insert, we can't assume the first match isn't in the process of being removed.
		// Scan all the way down to lowest level before declaring a duplicate.  Will be efficient due to lastAdjacent var.
		rcptr<volatile link_t> insert_unique_inner(bool& wasEmpty, height_t level, const key_t& criteria, const rcref<volatile link_t>& startFrom, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> result;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> next = startFrom;
			bool insertedLowerLevels = false;
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
					{
						// If at the bottom level, abort if there is a match
						if (!comparator_t::is_less_than(prev.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria)) // if prev and arg are equal
						{
							result = std::move(prev);
							break;
						}
					}
				}
				else if (!insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = prev;
					result = insert_unique_inner(wasEmpty, level - 1, criteria, next.dereference(), newLastAdjacent, sentinelPtr);
					if (!!result)
						break;
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev.template const_cast_to<const link_t>()->m_height);
					if (!prev->m_links[level].promote_read_token(rt_prev, wt))
						continue;
					volatile link_t* prevPtr = prev.get_ptr();
					bool mayBeEmpty = next.get_ptr() == prevPtr;
					next->complete_insert(level);
					m_links[level]->m_next = next.template const_cast_to<link_t>();
					m_links[level]->m_prev = std::move(prev).template const_cast_to<link_t>();
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
					wt->m_next = this_rcptr;
					COGS_ASSERT(!!wt->m_next.get_desc());
					if (!prevPtr->m_links[level].end_write(wt))
						continue;
					if (!level)
						wasEmpty = mayBeEmpty;
					complete(level);
				}
				break;
			}
			return result;
		}

		void insert_multi(height_t height, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* sentinelPtr = sentinel.get_ptr();
			const key_t& criteria = static_cast<payload_link_t*>(this)->get_payload()->get_key();

			// Append/Prepend optimization
			height_t level = 0;
			rcptr<link_t> prev = sentinelPtr->m_links[0]->m_prev;
			if ((prev.get_ptr() == sentinelPtr) || !comparator_t::is_less_than(prev.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
			{
				for (;;)
				{
					prev->m_links[level]->m_next = sentinel->m_links[level]->m_prev = this_rcptr;
					COGS_ASSERT(!!prev->m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!sentinel->m_links[level]->m_prev.get_desc());
					m_links[level]->m_next = sentinel;
					m_links[level]->m_prev = std::move(prev);
					COGS_ASSERT(!!m_links[level]->m_next.get_desc());
					COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
					if (level == m_height)
						break;
					++level;
					prev = sentinelPtr->m_links[level]->m_prev;
				}
			}
			else
			{
				rcptr<link_t> next = sentinelPtr->m_links[0]->m_next;
				if ((next.get_ptr() == sentinelPtr) || comparator_t::is_less_than(criteria, next.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
				{
					for (;;)
					{
						COGS_ASSERT(level <= next->m_height);
						sentinel->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
						COGS_ASSERT(!!sentinel->m_links[level]->m_next.get_desc());
						COGS_ASSERT(!!next->m_links[level]->m_prev.get_desc());

						m_links[level]->m_next = std::move(next);
						m_links[level]->m_prev = sentinel;
						COGS_ASSERT(!!m_links[level]->m_next.get_desc());
						COGS_ASSERT(!!m_links[level]->m_prev.get_desc());
						if (level == m_height)
							break;
						++level;
						next = sentinelPtr->m_links[level]->m_next;
					}
				}
				else
				{
					bool prevWasEqual = false;
					rcptr<link_t> lastAdjacent;
					insert_multi_inner(height, criteria, sentinel, prevWasEqual, lastAdjacent, sentinelPtr);
				}
			}
		}

		void insert_multi(bool& wasEmpty, height_t height, const rcref<volatile sentinel_link_t>& sentinel)
		{
			// Append/Prepend optimization.
			// Whether or not we scan forward or in reverse, the performance characteristics should be equivalent.
			// Though, on the assumption that new elements may be larger then existin ones, we default to doing
			// a reverse scan.  The reverse scan is optimized for append.
			// If new node is < the first node, we scan forward, which is optimized for prepend.
			rcptr<volatile link_t> lastAdjacent;
			volatile link_t* sentinelPtr = sentinel.get_ptr();
			const key_t& criteria = (static_cast<payload_link_t*>(this))->get_payload()->get_key();
			rcptr<link_t> next;
			for (;;)
			{
				read_token rt;
				sentinelPtr->begin_read_and_complete(rt, false);
				next = rt->m_next;
				if (!!next)
					break;
				COGS_ASSERT(!sentinelPtr->m_links[0].is_current(rt));
			}

			if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
				insert_multi_from_end_inner(wasEmpty, height, criteria, sentinel, lastAdjacent, sentinelPtr);
			else
			{
				bool prevWasEqual = false;
				insert_multi_from_front_inner(wasEmpty, height, criteria, sentinel, prevWasEqual, lastAdjacent, sentinelPtr);
			}
			if (!atomic::compare_exchange(m_primaryMode, link_mode::normal, link_mode::inserting))
			{
				bool wasLast;
				remove_from_all_levels(wasLast); // could only fail because a remove was initiated.  Needs to be owned by this thread.
			}
		}

		rcptr<link_t> insert_replace(height_t height, const rcref<sentinel_link_t>& sentinel)
		{
			rcptr<link_t> lastAdjacent;
			return insert_replace_inner(height, (static_cast<payload_link_t*>(this))->get_payload()->get_key(), sentinel, lastAdjacent, sentinel.get_ptr());
		}

		rcptr<link_t> insert_unique(height_t height, const rcref<sentinel_link_t>& sentinel)
		{
			rcptr<link_t> lastAdjacent;
			return insert_unique_inner(height, (static_cast<payload_link_t*>(this))->get_payload()->get_key(), sentinel, lastAdjacent, sentinel.get_ptr());
		}

		rcptr<volatile link_t> insert_unique(bool& wasEmpty, height_t height, const rcref<volatile sentinel_link_t>& sentinel)
		{
			rcptr<volatile link_t> lastAdjacent;
			rcptr<volatile link_t> result = insert_unique_inner(wasEmpty, height, (static_cast<payload_link_t*>(this))->get_payload()->get_key(), sentinel, lastAdjacent, sentinel.get_ptr());
			if (!result) // If no collision, and was inesrted.
			{
				if (!atomic::compare_exchange(m_primaryMode, link_mode::normal, link_mode::inserting))
				{
					bool wasLast;
					remove_from_all_levels(wasLast); // could only fail because a remove was initiated.  Needs to be owned by this thread.
				}
			}
			return result;
		}
	};

	typedef typename link_t::transactable_t transactable_t;
	typedef typename link_t::read_token read_token;
	typedef typename link_t::write_token write_token;

	class height_and_count_t
	{
	public:
		alignas (atomic::get_alignment_v<height_t>) height_t m_currentHeight;
		alignas (atomic::get_alignment_v<ptrdiff_t>) ptrdiff_t m_count;
	};

	class sentinel_link_t : public link_t
	{
	public:
		using link_t::m_links;

		alignas (atomic::get_alignment_v<height_and_count_t>) height_and_count_t m_heightAndCount;

		sentinel_link_t()
			: link_t(max_height, link_mode::normal, true)
		{
			link_t::m_links[0]->m_prev = link_t::m_links[0]->m_next = this_rcptr;
			COGS_ASSERT(!!link_t::m_links[0]->m_prev.get_desc());
			COGS_ASSERT(!!link_t::m_links[0]->m_next.get_desc());
			m_heightAndCount.m_currentHeight = 0;
			m_heightAndCount.m_count = 0;
		}

		void clear()
		{
			height_t level = 0;
			for (;;)
			{
				if (!m_links[level]->m_next)
					break;
				m_links[level]->m_next.release();
				m_links[level]->m_prev.release();
				if (level == max_height)
					break;
				++level;
			}
		}

		// 1) 1/2 chance the end bit is 1
		// 2) 1/4 chance the end bits are 10    (not 00, 01, or 11)
		// 3) 1/8 chance the end bits are 100   (not 000, 001, 010, 011, 101, 110, 111)
		// 4) 1/16 chance the end bits are 1000 (etc.)
		static height_t generate_height(height_t currentHeight)
		{
			// Normalized to 1-based, then extend by 1
			// 0 will not be returned, so normalizing back to 0-based
			height_t h = (height_t)random_coin_flips<height_t>(currentHeight + 2);
			COGS_ASSERT(!!h);
			if (!!h)
				--h;
			return h;
		}

		height_t generate_height()
		{
			height_t h = generate_height(m_heightAndCount.m_currentHeight);
			accommodate_height(h);
			return h;
		}

		height_t generate_height() volatile
		{
			height_t currentHeight;
			atomic::load(m_heightAndCount.m_currentHeight, currentHeight);
			height_t h = generate_height(currentHeight);
			accommodate_height(h);
			return h;
		}

		height_t accommodate_height(height_t newHeight)
		{
			if (newHeight <= m_heightAndCount.m_currentHeight)
				return m_heightAndCount.m_currentHeight;

			COGS_ASSERT(newHeight <= max_height);

			transactable_t* links = link_t::m_links.get_ptr();
			transactable_t* pos = links + newHeight;
			transactable_t* end = links + m_heightAndCount.m_currentHeight;
			for (;;)
			{
				if (!!(*pos)->m_next)
					break;
				(*pos)->m_prev = (*pos)->m_next = m_sentinel;
				COGS_ASSERT(!!(*pos)->m_prev.get_desc());
				COGS_ASSERT(!!(*pos)->m_next.get_desc());
				if (--pos == end)
					break;
			}

			m_heightAndCount.m_currentHeight = newHeight;
			return newHeight;
		}

		height_t accommodate_height(height_t newHeight) volatile
		{
			height_t oldHeight;
			atomic::load(m_heightAndCount.m_currentHeight, oldHeight);
			if (newHeight <= oldHeight)
				return oldHeight;

			COGS_ASSERT(newHeight <= max_height);

			transactable_t* links = link_t::m_links.get_ptr();
			transactable_t* pos = links + newHeight;
			transactable_t* end = links + oldHeight;
			for (;;)
			{
				read_token rt;
				pos->begin_read(rt);
				if (!!rt->m_next)
					break;

				write_token wt;
				if (!pos->promote_read_token(rt, wt))
					break;

				wt->m_prev = wt->m_next = this_rcref.template static_cast_to<volatile link_t>().template const_cast_to<link_t>();
				COGS_ASSERT(!!wt->m_prev.get_desc());
				COGS_ASSERT(!!wt->m_next.get_desc());
				pos->end_write(wt); // no need to verify, the first write is always to set these.
				if (--pos == end)
					break;
			}

			atomic::compare_exchange(m_heightAndCount.m_currentHeight, newHeight, oldHeight);
			// ignore failure to set height
			// it doesn't matter if another thread increased it, and it's not really
			// detrimental if it to zero - implying the list is extremely simple right now.

			return newHeight;
		}

		void dec_count()
		{
			if (--m_heightAndCount.m_count < m_heightAndCount.m_currentHeight)
				m_heightAndCount.m_currentHeight = m_heightAndCount.m_count;
		}

		void dec_count() volatile
		{
			height_and_count_t oldHeightAndCount;
			height_and_count_t newHeightAndCount;
			atomic::load(m_heightAndCount, oldHeightAndCount);
			do { // might bounce below zero if something is removed in the middle of being added
				newHeightAndCount.m_count = oldHeightAndCount.m_count - 1;
				ptrdiff_t minHeight = (ptrdiff_t)oldHeightAndCount.m_count;
				if (minHeight < 0)
					minHeight = 0;
				if (minHeight < oldHeightAndCount.m_currentHeight)
					newHeightAndCount.m_currentHeight = (height_t)minHeight;
				else
					newHeightAndCount.m_currentHeight = oldHeightAndCount.m_currentHeight;
			} while (!atomic::compare_exchange(m_heightAndCount, newHeightAndCount, oldHeightAndCount, oldHeightAndCount));
		}
	};

	class payload_link_t : public link_t
	{
	private:
		delayed_construction<payload_t> m_value;
		weak_rcptr<sentinel_link_t> m_sentinel; // Set once before inserted, so OK to read without atomicity

	public:
		payload_t* get_payload() const { return &(const_cast<payload_link_t*>(this)->m_value.get()); }
		payload_t* get_payload() const volatile { return &(const_cast<payload_link_t*>(this)->m_value.get()); }

		payload_link_t(const rcref<sentinel_link_t>& sentinel, height_t height)
			: link_t(height, link_mode::normal),
			m_sentinel(sentinel)
		{ }

		payload_link_t(const rcref<volatile sentinel_link_t>& sentinel, height_t height)
			: link_t(height, link_mode::inserting),
			m_sentinel(sentinel.template const_cast_to<sentinel_link_t>())
		{ }

		const rcref<payload_t>& get_obj(unowned_t<rcptr<payload_t> >& storage = unowned_t<rcptr<payload_t> >().get_unowned())
		{
			storage.set(&m_value.get(), &this_desc);
			return storage.dereference();
		}

		void set_sentinel(const rcref<sentinel_link_t>& sentinel) { m_sentinel = sentinel; }
		void set_sentinel(const rcref<volatile sentinel_link_t>& sentinel) volatile { m_sentinel = sentinel.template const_cast_to<sentinel_link_t>(); }

		const weak_rcptr<sentinel_link_t>& get_sentinel() const { return m_sentinel; }
		rcptr<volatile sentinel_link_t> get_sentinel() const volatile { return const_cast<const payload_link_t*>(this)->m_sentinel; }
	};

	allocator_container<allocator_type> m_allocator;
	rcptr<sentinel_link_t> m_sentinel;

	void clear_inner()
	{
		COGS_ASSERT(!!m_sentinel);
		link_t* sentinelPtr = m_sentinel.get_ptr();
		// Could allow the links to free themselves, but that would cause a cascade of
		// link releases, that could potential overflow the stack if the list is long enough.
		rcptr<link_t> l = sentinelPtr->m_links[0]->m_next;
		while (l.get_ptr() != sentinelPtr)
		{
			rcptr<link_t> next = l->m_links[0]->m_next;
			height_t level = l->m_height;
			for (;;)
			{
				l->m_links[level]->m_next.release();
				l->m_links[level]->m_prev.release();
				if (!level)
					break;
				--level;
			}
			l = std::move(next);
		}
	}

	// To support use of zero-initialized buffer placement, lazy-initialize the sentinel
	const rcref<sentinel_link_t>& lazy_init_sentinel()
	{
		if (!m_sentinel)
			m_sentinel = container_rcnew(m_allocator, sentinel_link_t);
		return m_sentinel.dereference();
	}

	rcref<volatile sentinel_link_t> lazy_init_sentinel() volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
		{
			rcptr<sentinel_link_t> newSentinel = container_rcnew(m_allocator, sentinel_link_t);
			if (m_sentinel.compare_exchange(newSentinel, sentinel, sentinel))
				sentinel = newSentinel;
			else
				newSentinel->clear();
		}
		return std::move(sentinel.dereference());
	}

	container_skiplist_base(const this_t&) = delete;
	container_skiplist_base(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) = delete;
	this_t& operator=(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) volatile = delete;
	this_t& operator=(const volatile this_t&) volatile = delete;

	template <typename F>
	iterator insert_inner(F&& f)
	{
		iterator i;
		const rcref<sentinel_link_t>& sentinel = lazy_init_sentinel();
		height_t height = sentinel->generate_height();
		i.m_link = container_rcnew(m_allocator, payload_link_t)(sentinel, height);
		height_t currentHeight = sentinel->accommodate_height(height);
		if (f(i, currentHeight, sentinel))
			++(sentinel->m_heightAndCount.m_count);
		return i;
	}

	template <typename F>
	volatile_iterator insert_inner(F&& f) volatile
	{
		iterator i;
		rcref<volatile sentinel_link_t> sentinel = lazy_init_sentinel();
		height_t height = sentinel->generate_height();
		i.m_link = container_rcnew(m_allocator, payload_link_t)(sentinel, height);
		height_t currentHeight = sentinel->accommodate_height(height);
		if (f(i, currentHeight, sentinel))
			assign_next(sentinel->m_heightAndCount.m_count);
		volatile_iterator i2 = std::move(i);
		return i2;
	}

public:
	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A container_skiplist element iterator
	class iterator
	{
	protected:
		rcptr<link_t> m_link;

		friend class container_skiplist_base;
		template <bool, typename, class, class, class> friend class container_skiplist;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		iterator(const rcptr<link_t>& l) : m_link(l) { }
		iterator(const volatile rcptr<link_t>& l) : m_link(l) { }
		iterator(rcptr<link_t>&& l) : m_link(std::move(l)) { }

		iterator(const rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(const volatile rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(rcptr<volatile link_t>&& l) : m_link(std::move(l).template const_cast_to<link_t>()) { }

		iterator(const rcref<link_t>& l) : m_link(l) { }
		iterator(const volatile rcref<link_t>& l) : m_link(l) { }
		iterator(rcref<link_t>&& l) : m_link(std::move(l)) { }

		iterator(const rcref<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(const volatile rcref<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(rcref<volatile link_t>&& l) : m_link(std::move(l).template const_cast_to<link_t>()) { }

		iterator(const weak_rcptr<link_t>& l) : m_link(l) { if (is_removed()) release(); }
		iterator(const volatile weak_rcptr<link_t>& l) : m_link(l) { if (is_removed()) release(); }

		iterator(const weak_rcptr<volatile link_t>& l)
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

		iterator(const volatile weak_rcptr<volatile link_t>& l)
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

		void assign(const rcptr<link_t>& l) { m_link = l; }
		void assign(const rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) volatile { m_link = l; }
		void assign(rcptr<link_t>&& l) { m_link = std::move(l); }
		void assign(rcptr<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(rcptr<volatile link_t>&& l) { m_link = std::move(l).template const_cast_to<link_t>(); }
		void assign(rcptr<volatile link_t>&& l) volatile { m_link = std::move(l).template const_cast_to<link_t>(); }

		void assign(const rcref<link_t>& l) { m_link = l; }
		void assign(const rcref<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<link_t>& l) { m_link = l; }
		void assign(const volatile rcref<link_t>& l) volatile { m_link = l; }
		void assign(rcref<link_t>&& l) { m_link = std::move(l); }
		void assign(rcref<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcref<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const rcref<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcref<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcref<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(rcref<volatile link_t>&& l) { m_link = std::move(l).template const_cast_to<link_t>(); }
		void assign(rcref<volatile link_t>&& l) volatile { m_link = std::move(l).template const_cast_to<link_t>(); }

		void assign(const weak_rcptr<link_t>& l)
		{
			m_link = l;
			if (is_removed())
				release();
		}

		void assign(const weak_rcptr<link_t>& l) volatile
		{
			rcptr<link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const volatile weak_rcptr<link_t>& l)
		{
			m_link = l;
			if (is_removed())
				release();
		}

		void assign(const volatile weak_rcptr<link_t>& l) volatile
		{
			rcptr<link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const weak_rcptr<volatile link_t>& l)
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

		void assign(const weak_rcptr<volatile link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

		void assign(const volatile weak_rcptr<volatile link_t>& l)
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

		void assign(const volatile weak_rcptr<volatile link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk).template const_cast_to<link_t>();
		}

	public:
		iterator() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator(T2&& i) : iterator(forward_member<T2>(i.m_link)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator& operator=(T2&& i) { assign(forward_member<T2>(i.m_link)); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile iterator& operator=(T2&& i) volatile { assign(forward_member<T2>(i.m_link)); return *this; }

		void disown() { m_link.disown(); }
		void disown() volatile { m_link.disown(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

		bool is_active() const { return !!m_link && !m_link->is_removed(); }
		bool is_active() const volatile { iterator i(*this); return i.is_active(); }

		bool is_removed() const { return !!m_link && m_link->is_removed(); } // implies that it was in the list.  null m_link returns false
		bool is_removed() const volatile { iterator i(*this); return i.is_removed(); }

		payload_t* get() const { return (!m_link) ? (payload_t*)0 : m_link.template static_cast_to<payload_link_t>()->get_payload(); }
		payload_t& operator*() const { return *(get()); }
		payload_t* operator->() const { return get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		const rcptr<payload_t>& get_obj(unowned_t<rcptr<payload_t> >& storage = unowned_t<rcptr<payload_t> >().get_unowned()) const
		{
			if (!!m_link)
				storage.set(m_link.template static_cast_to<payload_link_t>()->get_payload(), m_link.get_desc());
			return storage;
		}

		rcptr<payload_t> get_obj() const volatile
		{
			rcptr<payload_t> result;
			rcptr<volatile link_t> lnk = m_link;
			if (!!lnk)
			{
				result.set(lnk.template static_cast_to<payload_link_t>()->get_payload(), lnk.get_desc());
				lnk.disown();
			}
			return result;
		}

		bool operator!() const { return !m_link; }
		bool operator!() const volatile { return !m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_link != i.m_link; }

		iterator next() const
		{
			iterator result;
			if (!!m_link)
			{
				rcptr<link_t>* lnk = &(m_link->m_links[0]->m_next);
				while (!(*lnk)->is_sentinel())
				{
					if ((*lnk)->m_links[0]->m_mode != link_mode::removing)
					{
						result.m_link = *lnk;
						break;
					}
					lnk = &((*lnk)->m_links[0]->m_next);
				}
			}
			return result;
		}

		iterator next() const volatile { iterator tmp(*this); return tmp.next(); }

		void assign_next()
		{
			if (!!m_link)
			{
				do {
					m_link = m_link->m_links->m_next;
					if (!!m_link->is_sentinel())
					{
						m_link.release();
						break;
					}
				} while (m_link->m_links->m_mode == link_mode::removing);
			}
		}

		void assign_next() volatile
		{
			iterator i(*this);
			link_t* oldValue;
			do {
				oldValue = i.m_link.get_ptr();
				if (!oldValue)
					break;
				i.assign_next();
			} while (!m_link.compare_exchange(i.m_link, oldValue, i.m_link));
		}

		iterator prev() const
		{
			iterator result;
			if (!!m_link)
			{
				rcptr<link_t>* lnk = &(m_link->m_links[0]->m_prev);
				while (!(*lnk)->is_sentinel())
				{
					if ((*lnk)->m_links[0]->m_mode != link_mode::removing)
					{
						result.m_link = *lnk;
						break;
					}
					lnk = &((*lnk)->m_links[0]->m_prev);
				}
			}
			return result;
		}

		iterator prev() const volatile { iterator tmp(*this); return tmp.prev(); }

		void assign_prev()
		{
			if (!!m_link)
			{
				do {
					m_link = m_link->m_links->m_prev;
					if (!!m_link->is_sentinel())
					{
						m_link.release();
						break;
					}
				} while (m_link->m_links->m_mode == link_mode::removing);
			}
		}

		void assign_prev() volatile
		{
			iterator i(*this);
			link_t* oldValue;
			do {
				oldValue = i.m_link.get_ptr();
				if (!oldValue)
					break;
				i.assign_prev();
			} while (!m_link.compare_exchange(i.m_link, oldValue, i.m_link));
		}

		iterator& operator++()
		{
			assign_next();
			return *this;
		}

		iterator operator++() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;) {
				if (!oldValue)
					break;
				newValue = oldValue.next();
				if (m_link.compare_exchange(newValue.m_link, oldValue.m_link, oldValue.m_link))
					break;
				newValue.release();
			}
			return newValue;
		}

		iterator operator++(int) { iterator i(*this); assign_next(); return i; }

		iterator operator++(int) volatile
		{
			iterator oldValue(*this);
			while (!!oldValue)
			{
				iterator newValue = oldValue.next();
				if (m_link.compare_exchange(newValue.m_link, oldValue.m_link, oldValue.m_link))
					break;
			}
			return oldValue;
		}

		iterator& operator--()
		{
			assign_prev();
			return *this;
		}

		iterator operator--() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;) {
				if (!oldValue)
					break;
				newValue = oldValue.prev();
				if (m_link.compare_exchange(newValue.m_link, oldValue.m_link, oldValue.m_link))
					break;
				newValue.release();
			}
			return newValue;
		}

		iterator operator--(int) { iterator i(*this); assign_prev(); return i; }

		iterator operator--(int) volatile
		{
			iterator oldValue(*this);
			while (!!oldValue)
			{
				iterator newValue = oldValue.prev();
				if (m_link.compare_exchange(newValue.m_link, oldValue.m_link, oldValue.m_link))
					break;
			}
			return oldValue;
		}


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) volatile { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

	};

	/// @brief A volatile container_skiplist element iterator
	class volatile_iterator
	{
	protected:
		rcptr<volatile link_t> m_link;

		friend class container_skiplist_base;
		template <bool, typename, class, class, class> friend class container_skiplist;
		friend class iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		volatile_iterator(const rcptr<link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcptr<link_t>& l) : m_link(l) { }
		volatile_iterator(rcptr<link_t>&& l) : m_link(std::move(l)) { }

		volatile_iterator(const rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(rcptr<volatile link_t>&& l) : m_link(std::move(l)) { }

		volatile_iterator(const rcref<link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcref<link_t>& l) : m_link(l) { }
		volatile_iterator(rcref<link_t>&& l) : m_link(std::move(l)) { }

		volatile_iterator(const rcref<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcref<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(rcref<volatile link_t>&& l) : m_link(std::move(l)) { }

		volatile_iterator(const weak_rcptr<link_t>& l) : m_link(l) { if (is_removed()) release(); }
		volatile_iterator(const volatile weak_rcptr<link_t>& l) : m_link(l) { if (is_removed()) release(); }

		volatile_iterator(const weak_rcptr<volatile link_t>& l) : m_link(l) { if (is_removed()) release(); }
		volatile_iterator(const volatile weak_rcptr<volatile link_t>& l) : m_link(l) { if (is_removed()) release(); }


		void assign(const rcptr<link_t>& l) { m_link = l; }
		void assign(const rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) volatile { m_link = l; }
		void assign(rcptr<link_t>&& l) { m_link = std::move(l); }
		void assign(rcptr<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const rcptr<volatile link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<volatile link_t>& l) volatile { m_link = l; }
		void assign(rcptr<volatile link_t>&& l) { m_link = std::move(l); }
		void assign(rcptr<volatile link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcref<link_t>& l) { m_link = l; }
		void assign(const rcref<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<link_t>& l) { m_link = l; }
		void assign(const volatile rcref<link_t>& l) volatile { m_link = l; }
		void assign(rcref<link_t>&& l) { m_link = std::move(l); }
		void assign(rcref<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcref<volatile link_t>& l) { m_link = l; }
		void assign(const rcref<volatile link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<volatile link_t>& l) { m_link = l; }
		void assign(const volatile rcref<volatile link_t>& l) volatile { m_link = l; }
		void assign(rcref<volatile link_t>&& l) { m_link = std::move(l); }
		void assign(rcref<volatile link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const weak_rcptr<link_t>& l) { m_link = l; if (is_removed()) release(); }

		void assign(const weak_rcptr<link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const volatile weak_rcptr<link_t>& l) { m_link = l; if (is_removed()) release(); }

		void assign(const volatile weak_rcptr<link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const weak_rcptr<volatile link_t>& l) { m_link = l; if (is_removed()) release(); }

		void assign(const weak_rcptr<volatile link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const volatile weak_rcptr<volatile link_t>& l) { m_link = l; if (is_removed()) release(); }

		void assign(const volatile weak_rcptr<volatile link_t>& l) volatile
		{
			rcptr<volatile link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

	public:
		volatile_iterator() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator(T2&& i) : volatile_iterator(forward_member<T2>(i.m_link)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator& operator=(T2&& i) { assign(forward_member<T2>(i.m_link)); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile volatile_iterator& operator=(T2&& i) volatile { assign(forward_member<T2>(i.m_link)); return *this; }

		void disown() { m_link.disown(); }
		void disown() volatile { m_link.disown(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

		bool is_active() const { return !!m_link && !m_link->is_removed(); }
		bool is_active() const volatile { rcptr<volatile link_t> lnk = m_link; return !!lnk && !!lnk->is_removed(); }

		bool is_removed() const { return !!m_link && m_link->is_removed(); }
		bool is_removed() const volatile { rcptr<volatile link_t> lnk = m_link; return !!lnk && lnk->is_removed(); }

		payload_t* get() const { return (!m_link) ? (payload_t*)0 : m_link.template static_cast_to<volatile payload_link_t>()->get_payload(); }
		payload_t& operator*() const { return *(get()); }
		payload_t* operator->() const { return get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		const rcptr<payload_t>& get_obj(unowned_t<rcptr<payload_t> >& storage = unowned_t<rcptr<payload_t> >().get_unowned()) const
		{
			if (!!m_link)
				storage.set(m_link.template static_cast_to<volatile payload_link_t>()->get_payload(), m_link.get_desc());
			return storage;
		}

		rcptr<payload_t> get_obj() const volatile
		{
			rcptr<payload_t> result;
			rcptr<volatile link_t> lnk = m_link;
			if (!!lnk)
			{
				result.set(lnk.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload(), lnk.get_desc());
				lnk.disown();
			}
			return result;
		}

		bool operator!() const { return !m_link; }
		bool operator!() const volatile { return !m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_link != i.m_link; }

		volatile_iterator next() const
		{
			volatile_iterator result;
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true); // OK if already removed
				rcptr<volatile link_t> newLink;
				for (;;)
				{
					newLink = rt->m_next;
					if (newLink->is_sentinel())
						break;
					if (!newLink->begin_read_and_complete(rt, true))
						continue;
					result.m_link = std::move(newLink);
					break;
				}
			}
			return result;
		}

		volatile_iterator next() const volatile
		{
			volatile_iterator result;
			rcptr<volatile link_t> oldValue;
			bool done = false;
			for (;;)
			{
				oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_next;
					if (!newLink->is_sentinel())
					{
						if (!newLink->begin_read_and_complete(rt, true))
						{
							if (oldValue != m_link)
								break;
							continue;
						}
						result.m_link = std::move(newLink);
					}
					done = true;
					break;
				}
				if (done)
					break;
			}
			return result;
		}

		void assign_next()
		{
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true); // OK if already removed
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_next;
					if (newLink->is_sentinel())
					{
						m_link.release();
						break;
					}
					if (!newLink->begin_read_and_complete(rt, true))
						continue;
					m_link = std::move(newLink);
					break;
				}
			}
		}

		void assign_next() volatile
		{
			bool done = false;
			for (;;)
			{
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_next;
					if (newLink->is_sentinel())
						newLink.release();
					else if (!newLink->begin_read_and_complete(rt, true))
					{
						if (oldValue != m_link)
							break;
						continue;
					}
					done = m_link.compare_exchange(newLink, oldValue, oldValue);
					break;
				}
				if (done)
					break;
			}
		}

		volatile_iterator prev() const
		{
			volatile_iterator result;
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true); // OK if already removed
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_prev;
					if (newLink->is_sentinel())
						break;
					if (!newLink->begin_read_and_complete(rt, true))
						continue;
					result.m_link = std::move(newLink);
					break;
				}
			}
			return result;
		}

		volatile_iterator prev() const volatile
		{
			volatile_iterator result;
			bool done = false;
			for (;;)
			{
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_prev;
					if (!newLink->is_sentinel())
					{
						if (!newLink->begin_read_and_complete(rt, true))
						{
							if (oldValue != m_link)
								break;
							continue;
						}
						result.m_link = std::move(newLink);
					}
					done = true;
					break;
				}
				if (done)
					break;
			}
			return result;
		}

		void assign_prev()
		{
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true); // OK if already removed
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_prev;
					if (newLink->is_sentinel())
					{
						m_link.release();
						break;
					}
					if (!newLink->begin_read_and_complete(rt, true))
						continue;
					m_link = std::move(newLink);
					break;
				}
			}
		}

		void assign_prev() volatile
		{
			bool done = false;
			for (;;)
			{
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_prev;
					if (newLink->is_sentinel())
						newLink.release();
					else if (!newLink->begin_read_and_complete(rt, true))
					{
						if (oldValue != m_link)
							break;
						continue;
					}
					done = m_link.compare_exchange(newLink, oldValue, oldValue);
					break;
				}
				if (done)
					break;
			}
		}

		volatile_iterator& operator++()
		{
			assign_next();
			return *this;
		}

		volatile_iterator operator++() volatile
		{
			volatile_iterator result;
			rcptr<volatile link_t> newLink;
			bool done = false;
			for (;;)
			{
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					result.m_link = rt->m_next;
					if (result.m_link->is_sentinel())
						result.m_link.release();
					else if (!result.m_link->begin_read_and_complete(rt, true))
					{
						if (oldValue != m_link)
							break;
						continue;
					}
					done = m_link.compare_exchange(result.m_link, oldValue, oldValue);
					break;
				}
				if (done)
					break;
			}
			return result;
		}

		volatile_iterator operator++(int) { volatile_iterator i(*this); assign_next(); return i; }

		volatile_iterator operator++(int) volatile
		{
			volatile_iterator oldValue(*this);
			while (!!oldValue)
			{
				if (compare_exchange(oldValue.next(), oldValue, oldValue))
					break;
			}
			return oldValue;
		}

		volatile_iterator& operator--()
		{
			assign_prev();
			return *this;
		}

		volatile_iterator operator--() volatile
		{
			volatile_iterator result;
			rcptr<volatile link_t> newLink;
			bool done = false;
			for (;;)
			{
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
				for (;;)
				{
					result.m_link = rt->m_prev;
					if (result.m_link->is_sentinel())
						result.m_link.release();
					else if (!result.m_link->begin_read_and_complete(rt, true))
					{
						if (oldValue != m_link)
							break;
						continue;
					}
					done = m_link.compare_exchange(result.m_link, oldValue, oldValue);
					break;
				}
				if (done)
					break;
			}
			return result;
		}

		volatile_iterator operator--(int) { volatile_iterator i(*this); assign_prev(); return i; }

		volatile_iterator operator--(int) volatile
		{
			volatile_iterator oldValue(*this);
			while (!!oldValue)
			{
				if (compare_exchange(oldValue.prev(), oldValue, oldValue))
					break;
			}
			return oldValue;
		}


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) volatile { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	/// @brief A container_skiplist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		weak_rcptr<link_t> m_link;

		friend class container_skiplist_base;
		template <bool, typename, class, class, class> friend class container_skiplist;
		friend class iterator;
		friend class volatile_iterator;
		friend class volatile_remove_token;

		remove_token(const weak_rcptr<link_t>& l) : m_link(l) { }
		remove_token(const volatile weak_rcptr<link_t>& l) : m_link(l) { }
		remove_token(weak_rcptr<link_t>&& l) : m_link(std::move(l)) { }

		remove_token(const weak_rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		remove_token(const volatile weak_rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		remove_token(weak_rcptr<volatile link_t>&& l) : m_link(std::move(l).template const_cast_to<link_t>()) { }

		remove_token(const rcptr<link_t>& l) : m_link(l) { }
		remove_token(const volatile rcptr<link_t>& l) : m_link(l) { }

		remove_token(const rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		remove_token(const volatile rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }

		void assign(const weak_rcptr<link_t>& l) { m_link = l; }
		void assign(const weak_rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile weak_rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile weak_rcptr<link_t>& l) volatile { m_link = l; }
		void assign(weak_rcptr<link_t>&& l) { m_link = std::move(l); }
		void assign(weak_rcptr<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const weak_rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const weak_rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile weak_rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile weak_rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(weak_rcptr<volatile link_t>&& l) { m_link = std::move(l).template const_cast_to<link_t>(); }
		void assign(weak_rcptr<volatile link_t>&& l) volatile { m_link = std::move(l).template const_cast_to<link_t>(); }

		void assign(const rcptr<link_t>& l) { m_link = l; }
		void assign(const rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) volatile { m_link = l; }

		void assign(const rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcptr<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcptr<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }

		void assign(const rcref<link_t>& l) { m_link = l; }
		void assign(const rcref<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<link_t>& l) { m_link = l; }
		void assign(const volatile rcref<link_t>& l) volatile { m_link = l; }

		void assign(const rcref<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const rcref<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcref<volatile link_t>& l) { m_link = l.template const_cast_to<link_t>(); }
		void assign(const volatile rcref<volatile link_t>& l) volatile { m_link = l.template const_cast_to<link_t>(); }

	public:
		remove_token() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token(T2&& i) : remove_token(forward_member<T2>(i.m_link)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token& operator=(T2&& i) { assign(forward_member<T2>(i.m_link)); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile remove_token& operator=(T2&& i) volatile { assign(forward_member<T2>(i.m_link)); return *this; }

		void disown() { m_link.disown(); }
		void disown() volatile { m_link.disown(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

		bool is_active() const { rcptr<link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }
		bool is_active() const volatile { rcptr<link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		bool operator!() const { return !m_link; }
		bool operator!() const volatile { return !m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) { return remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) volatile { return remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	/// @brief A container_skiplist element volatile remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class volatile_remove_token
	{
	protected:
		weak_rcptr<volatile link_t> m_link;

		friend class container_skiplist_base;
		template <bool, typename, class, class, class> friend class container_skiplist;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;

		volatile_remove_token(const weak_rcptr<link_t>& l) : m_link(l) { }
		volatile_remove_token(const volatile weak_rcptr<link_t>& l) : m_link(l) { }
		volatile_remove_token(weak_rcptr<link_t>&& l) : m_link(std::move(l)) { }

		volatile_remove_token(const weak_rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_remove_token(const volatile weak_rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_remove_token(weak_rcptr<volatile link_t>&& l) : m_link(std::move(l)) { }

		volatile_remove_token(const rcptr<link_t>& l) : m_link(l) { }
		volatile_remove_token(const volatile rcptr<link_t>& l) : m_link(l) { }

		volatile_remove_token(const rcptr<volatile link_t>& l) { m_link = l; }
		volatile_remove_token(const volatile rcptr<volatile link_t>& l) { m_link = l; }

		void assign(const weak_rcptr<link_t>& l) { m_link = l; }
		void assign(const weak_rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile weak_rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile weak_rcptr<link_t>& l) volatile { m_link = l; }
		void assign(weak_rcptr<link_t>&& l) { m_link = std::move(l); }
		void assign(weak_rcptr<link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const weak_rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const weak_rcptr<volatile link_t>& l) volatile { m_link = l; }
		void assign(const volatile weak_rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const volatile weak_rcptr<volatile link_t>& l) volatile { m_link = l; }
		void assign(weak_rcptr<volatile link_t>&& l) { m_link = std::move(l); }
		void assign(weak_rcptr<volatile link_t>&& l) volatile { m_link = std::move(l); }

		void assign(const rcptr<link_t>& l) { m_link = l; }
		void assign(const rcptr<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<link_t>& l) volatile { m_link = l; }

		void assign(const rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const rcptr<volatile link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcptr<volatile link_t>& l) { m_link = l; }
		void assign(const volatile rcptr<volatile link_t>& l) volatile { m_link = l; }

		void assign(const rcref<link_t>& l) { m_link = l; }
		void assign(const rcref<link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<link_t>& l) { m_link = l; }
		void assign(const volatile rcref<link_t>& l) volatile { m_link = l; }

		void assign(const rcref<volatile link_t>& l) { m_link = l; }
		void assign(const rcref<volatile link_t>& l) volatile { m_link = l; }
		void assign(const volatile rcref<volatile link_t>& l) { m_link = l; }
		void assign(const volatile rcref<volatile link_t>& l) volatile { m_link = l; }

	public:
		volatile_remove_token() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token(T2&& i) : volatile_remove_token(forward_member<T2>(i.m_link)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token& operator=(T2&& i) { assign(forward_member<T2>(i.m_link)); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile volatile_remove_token& operator=(T2&& i) volatile { assign(forward_member<T2>(i.m_link)); return *this; }

		void disown() { m_link.disown(); }
		void disown() volatile { m_link.disown(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

		bool is_active() const { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }
		bool is_active() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		bool operator!() const { return !m_link; }
		bool operator!() const volatile { return !m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) { return volatile_remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) volatile { return volatile_remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	container_skiplist_base() { }

	container_skiplist_base(this_t&& src)
		: m_sentinel(std::move(m_sentinel))
	{ }

	explicit container_skiplist_base(volatile allocator_type& al)
		: m_allocator(al)
	{ }

	~container_skiplist_base()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->clear();
			COGS_ASSERT(m_sentinel.get_desc()->is_owned());
		}
	}

	this_t& operator=(this_t&& src)
	{
		if (!!m_sentinel)
			clear_inner();
		m_allocator = std::move(src.m_allocator);
		m_sentinel = std::move(src.m_sentinel);
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		m_sentinel.exchange(src.m_sentinel);
		src.m_sentinel.release();
		return *this;
	}

	void clear()
	{
		if (!!m_sentinel && !!m_sentinel->m_heightAndCount.m_count)
		{
			clear_inner();
			height_t level = 0;
			for (;;)
			{
				if (m_sentinel->m_links[level]->m_next.get_ptr() == m_sentinel.get_ptr())
					break;
				m_sentinel->m_links[level]->m_next = m_sentinel;
				m_sentinel->m_links[level]->m_prev = m_sentinel;
				COGS_ASSERT(!!m_sentinel->m_links[level]->m_next.get_desc());
				COGS_ASSERT(!!m_sentinel->m_links[level]->m_prev.get_desc());
				if (level == max_height)
					break;
				++level;
			}
			m_sentinel->m_heightAndCount.m_count = 0;
			m_sentinel->m_heightAndCount.m_currentHeight = 0;
		}
	}

	bool drain() volatile
	{
		bool foundAny = false;
		while (!pop_last().wasEmptied)
			foundAny = true;
		return foundAny;
	}

	size_t size() const
	{
		if (!m_sentinel)
			return 0;
		return m_sentinel->m_heightAndCount.m_count;
	}

	size_t size() const volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
			return 0;
		ptrdiff_t sz;
		atomic::load(sentinel->m_heightAndCount.m_count, sz);
		if (sz < 0) // The count can bounce below zero, if an item is added, removed, and dec'ed before inc'ed.
			sz = 0;
		return sz;
	}

	bool is_empty() const
	{
		return !m_sentinel || !m_sentinel->m_heightAndCount.m_count;
	}

	bool is_empty() const volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
			return true;
		ptrdiff_t sz;
		atomic::load(sentinel->m_heightAndCount.m_count, sz);
		return (sz <= 0);
	}

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	iterator get_first() const
	{
		iterator i;
		if (!!m_sentinel && (m_sentinel->m_links[0]->m_next.get_ptr() != m_sentinel.get_ptr()))
			i.m_link = m_sentinel->m_links[0]->m_next;
		return i;
	}

	volatile_iterator get_first() const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			rcptr<volatile link_t> lnk;
			for (;;)
			{
				read_token rt;
				bool b = sentinel->begin_read_and_complete(rt, false);
				COGS_ASSERT(b); // sentinel will not be removed
				lnk = rt->m_next;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (!lnk->begin_read_and_complete(rt, false)) // just in case it needs help removing...
					continue;
				i.m_link = std::move(lnk);
				break;
			}
		}
		return i;
	}

	iterator get_last() const
	{
		iterator i;
		if (!!m_sentinel && (m_sentinel->m_links[0]->m_prev.get_ptr() != m_sentinel.get_ptr()))
			i.m_link = m_sentinel->m_links[0]->m_prev;
		return i;
	}

	volatile_iterator get_last() const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			rcptr<volatile link_t> lnk;
			for (;;)
			{
				read_token rt;
				bool b = sentinel->begin_read_and_complete(rt, false);
				COGS_ASSERT(b); // sentinel will not be removed
				lnk = rt->m_prev;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (!lnk->begin_read_read_and_complete(0, rt, false)) // just in case it needs help removing...
					continue;
				i.m_link = std::move(lnk);
				break;
			}
		}
		return i;
	}

	bool remove(const iterator& i)
	{
		bool b = false;
		if (!!i.m_link)
		{
			b = i.m_link->remove();
			if (b && !!m_sentinel)
				m_sentinel->dec_count();
		}
		return b;
	}

	bool remove(const remove_token& rt)
	{
		iterator i(rt);
		return remove(i);
	}

	struct volatile_remove_result
	{
		bool wasRemoved;
		bool wasEmptied;
	};

	volatile_remove_result remove(const volatile_iterator& i) volatile
	{
		bool wasLast = false;
		rcptr<volatile link_t> lnk = i.m_link;
		bool b = false;
		if (!!lnk)
		{
			b = lnk->remove(wasLast);
			if (b)
			{
				rcptr<volatile sentinel_link_t> sentinel = i.m_link.template static_cast_to<volatile payload_link_t>()->get_sentinel();
				if (!!sentinel)
					sentinel->dec_count();
			}
		}
		return { b, wasLast };
	}

	volatile_remove_result remove(const volatile_remove_token& rt) volatile
	{
		volatile_iterator i(rt);
		return remove(i);
	}

	iterator pop_first()
	{
		iterator i = get_first();
		remove(i);
		return i;
	}

	iterator pop_last()
	{
		iterator i = get_last();
		remove(i);
		return i;
	}

	struct pop_result
	{
		volatile_iterator iterator;
		bool wasEmptied;
	};

	pop_result pop_first() volatile
	{
		volatile_iterator itor;
		bool wasLast = false;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			read_token rt_sentinel;
			for (;;)
			{
				bool b = sentinel->begin_read_and_complete(rt_sentinel, false); // sentinel won't get deleted, no need to check result
				COGS_ASSERT(b);
				rcptr<volatile link_t> prev = rt_sentinel->m_prev;

				rcptr<volatile link_t> firstLink = rt_sentinel->m_next;
				if (firstLink.get_ptr() == sentinel.get_ptr())
					break;
				if (firstLink->is_removed(0)) // also completes other states
					continue;

				write_token wt;
				if (!sentinel->m_links[0].promote_read_token(rt_sentinel, wt))
					continue;

				COGS_ASSERT(wt->m_mode == link_mode::normal);
				COGS_ASSERT(wt->m_next == firstLink);

				bool mayBeLast = wt->m_next.get_ptr() == wt->m_prev.get_ptr();

				wt->m_mode = link_mode::removing_next;
				if (!sentinel->m_links[0].end_write(wt))
					continue;

				sentinel->begin_read_and_complete(rt_sentinel, false); // completes remove-next
				if (!firstLink->remove())
					continue; // someone else marked it first.
				wasLast = mayBeLast;
				sentinel->dec_count();
				itor.m_link = std::move(firstLink);
				break;
			}
		}
		return { std::move(itor), wasLast };
	}

	pop_result pop_last() volatile
	{
		volatile_iterator itor;
		bool wasLast = false;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		read_token rt_last;
		read_token rt_prev;
		read_token rt_sentinel;
		if (!!sentinel)
		{
			for (;;)
			{
				sentinel->begin_read_and_complete(rt_sentinel, false); // sentinel won't get deleted, no need to check result
				rcptr<volatile link_t> lastLink = rt_sentinel->m_prev;
				if (lastLink.get_ptr() == sentinel.get_ptr())
					break;
				rcptr<volatile link_t> prev;
				rcptr<volatile link_t> prev_prev;
				bool outerContinue = false;
				for (;;)
				{
					if (!lastLink->begin_read_and_complete(rt_last, false))
					{
						outerContinue = true;
						break;
					}
					prev = rt_last->m_prev;
					bool outerContinue2 = false;
					for (;;)
					{
						if (!prev->begin_read_and_complete(rt_prev, false))
						{
							outerContinue2 = true;
							break;
						}
						prev_prev = rt_prev->m_prev;
						break;
					}
					if (outerContinue2)
						continue;
					break; // and !outerContinue
				}
				if (outerContinue)
					continue;

				if (!lastLink->m_links[0].is_current(rt_last))
					continue;
				if (!sentinel->m_links[0].is_current(rt_sentinel))
					continue;

				write_token wt_prev;
				if (!prev->m_links[0].promote_read_token(rt_prev, wt_prev))
					continue;

				COGS_ASSERT(wt_prev->m_mode == link_mode::normal);
				COGS_ASSERT(wt_prev->m_next == lastLink);

				bool mayBeLast = wt_prev->m_next.get_ptr() == prev_prev.get_ptr();

				wt_prev->m_mode = link_mode::removing_next;
				if (!prev->m_links[0].end_write(wt_prev))
					continue;

				prev->begin_read_and_complete(rt_prev, false); // completes remove-next
				if (!lastLink->remove())
					continue;
				wasLast = mayBeLast;
				sentinel->dec_count();
				itor.m_link = std::move(lastLink);
				break;
			}
		}
		return { std::move(itor), wasLast };
	}


	iterator find_any_equal(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = std::move(next);
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							i.m_link = std::move(next);
							break;
						}
					}
				}
				if (!level)
					break;
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_any_equal(const key_t& criteria) const volatile
	{
		// Due to the possibility of lingering removals, we need to scan all the way to the bottom
		// of the list in order to ensure a match is still valid.
		return find_last_equal(criteria);
	}

	iterator find_first_equal(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(next.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next.template static_cast_to<payload_link_t>()->get_payload()->get_key()))// if next and arg are equal
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			bool nextWasEqual = false;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_next;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if prev is not sentinel
					{
						const key_t& prevCriteria = prev.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance)
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria)) // if prev and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_last_equal(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if prev is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if ((prev.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(prev.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria)) // if next and arg are equal
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(next.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			bool nextWasEqual = false;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& prevCriteria = prev.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria)) // if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual && (prev.get_ptr() != sentinelPtr))
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual && (next.get_ptr() != sentinelPtr))
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if prev is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = std::move(next);
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							i.m_link = std::move(next);
							break;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const volatile
	{
		// Due to the possibility of lingering removals, we need to scan all the way to the bottom
		// of the list in order to ensure a match is still valid.
		return find_last_equal_or_nearest_less_than(criteria);
	}

	iterator find_any_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = std::move(next);
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
						{
							i.m_link = std::move(next);
							break;
						}
					}
				}
				if (!level)
					break;
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_any_equal_or_nearest_greater_than(const key_t& criteria) const volatile
	{
		// Due to the possibility of lingering removals, we need to scan all the way to the bottom
		// of the list in order to ensure a match is still valid.
		return find_last_equal_or_nearest_greater_than(criteria);
	}

	iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(next.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						i.m_link = std::move(next);
					else if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			bool nextWasEqual = false;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& prevCriteria = prev.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria)) // if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual)
						i.m_link = std::move(next);
					else if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(next.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria))
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			bool nextWasEqual = false;
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					nextWasEqual = false;
					next = rt->m_prev;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& prevCriteria = prev.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria)) // if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next;
			rcptr<volatile link_t> prev = sentinelPtr;
			rcptr<volatile link_t> lastAdjacent;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					next = rt->m_prev;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = std::move(prev);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		sentinel_link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = sentinelPtr->m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						const key_t& nextCriteria = next.template static_cast_to<payload_link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance) // Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria)) // if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = std::move(next);
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual)
						i.m_link = std::move(prev);
					else if (next.get_ptr() != sentinelPtr)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(next);
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		volatile sentinel_link_t* sentinelPtr = sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> next = sentinelPtr;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> lastAdjacent;
			height_t level;
			atomic::load(sentinelPtr->m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(rt, true, level))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false, level))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(rt_prev_next, false, level);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev.get_ptr() != lastAdjacent.get_ptr())
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr) // if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template static_cast_to<payload_link_t>()->get_payload()->get_key()))
						{
							next = std::move(prev);
							continue;
						}
					}
				}
				if (!level)
				{
					if ((prev.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(prev.template static_cast_to<payload_link_t>()->get_payload()->get_key(), criteria)) // if prev and arg are equal
						i.m_link = std::move(prev);
					else if (next.get_ptr() != sentinelPtr)
						i.m_link = std::move(next);
					break;
				}
				lastAdjacent = std::move(prev);
				--level;
				continue;
			}
		}
		return i;
	}


	void swap(this_t& wth)
	{
		m_sentinel.swap(wth.m_sentinel);
		m_allocator.swap(wth.m_allocator);
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile
	{
		m_sentinel.swap(wth.m_sentinel);
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth)
	{
		m_sentinel.swap(wth.m_sentinel);
	}

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

	void exchange(this_t&& src, this_t& rtn)
	{
		rtn = std::move(src);
		swap(rtn);
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void exchange(this_t&& src, this_t& rtn) volatile
	{
		rtn = std::move(src);
		swap(rtn);
	}

	iterator begin() const { return get_first(); }
	volatile_iterator begin() const volatile { return get_first(); }

	iterator rbegin() const { return get_last(); }
	volatile_iterator rbegin() const volatile { return get_last(); }

	iterator end() const { iterator i; return i; }
	volatile_iterator end() const volatile { volatile_iterator i; return i; }

	iterator rend() const { iterator i; return i; }
	volatile_iterator rend() const volatile { volatile_iterator i; return i; }
};


template <typename key_t, class payload_t, class comparator_t, class allocator_type>
class container_skiplist<false, key_t, payload_t, comparator_t, allocator_type> : public container_skiplist_base<key_t, payload_t, comparator_t, allocator_type>
{
private:
	typedef container_skiplist<false, key_t, payload_t, comparator_t, allocator_type> this_t;
	typedef container_skiplist_base<key_t, payload_t, comparator_t, allocator_type> base_t;

	container_skiplist(const this_t&) = delete;
	container_skiplist(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) = delete;
	this_t& operator=(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) volatile = delete;
	this_t& operator=(const volatile this_t&) volatile = delete;

	using typename base_t::link_t;
	using typename base_t::sentinel_link_t;
	using typename base_t::height_t;

	using base_t::insert_inner;

public:
	using typename base_t::iterator;
	using typename base_t::volatile_iterator;

	container_skiplist() { }

	container_skiplist(this_t&& src)
		: base_t(std::move(src))
	{ }

	explicit container_skiplist(volatile allocator_type& al)
		: base_t(al)
	{ }

	container_skiplist(const std::initializer_list<payload_t>& src)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	container_skiplist(volatile allocator_type& al, const std::initializer_list<payload_t>& src)
		: base_t(al)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	this_t& operator=(this_t&& src)
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	this_t& operator=(const std::initializer_list<payload_t>& src)
	{
		this_t tmp(src);
		base_t::operator=(std::move(tmp));
		return *this;
	}

	volatile this_t& operator=(const std::initializer_list<payload_t>& src) volatile
	{
		this_t tmp(src);
		base_t::operator=(std::move(tmp));
		return *this;
	}


	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.insert_replace(std::forward<args_t>(args)), ...);
		return result;
	}

	struct insert_replace_result
	{
		iterator replaced;
		iterator inserted;
	};

	// The first iterator is the newly created element.  The second is the element that was replaced, if any.
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_replace_result>
	insert_replace_via(F&& f)
	{
		iterator collidedWith;
		iterator i = insert_inner([&](iterator& i, height_t currentHeight, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			collidedWith.m_link = l->insert_replace(currentHeight, sentinel);
			return true;
		});
		return { std::move(collidedWith), std::move(i) };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<payload_t>&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<payload_t>&>
		&& std::is_invocable_v<F, payload_t&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(*i.get()); }); }

	insert_replace_result insert_replace(const payload_t& src) { return insert_replace_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(src); }); }
	insert_replace_result insert_replace(payload_t&& src) { return insert_replace_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<payload_t, F&&>
		&& !std::is_convertible_v<F, const payload_t&>
		&& !std::is_convertible_v<F, payload_t&&>,
		insert_replace_result>
	insert_replace(F&& f) { return insert_replace_via(std::forward<F>(f)); }

	template <typename... args_t> insert_replace_result insert_replace_emplace(args_t&&... args) { return insert_replace_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::forward<args_t>(args)...); }); }


	struct insert_unique_result
	{
		iterator iterator;
		bool hadCollision;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_unique_result>
	insert_unique_via(F&& f)
	{
		bool collided;
		iterator i2 = insert_inner([&](iterator& i, height_t currentHeight, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			rcptr<link_t> collidedWith = l->insert_unique(currentHeight, sentinel);
			collided = !!collidedWith;
			if (collided)
			{
				i.m_link = std::move(collidedWith);
				return false;
			}
			return true;
		});
		return { std::move(i2), collided };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<payload_t>&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<payload_t>&>
		&& std::is_invocable_v<F, payload_t&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(*i.get()); }); }

	insert_unique_result insert_unique(const payload_t& src) { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(src); }); }
	insert_unique_result insert_unique(payload_t&& src) { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<payload_t, F&&>
		&& !std::is_convertible_v<F, const payload_t&>
		&& !std::is_convertible_v<F, payload_t&&>,
		insert_unique_result>
	insert_unique(F&& f) { return insert_unique_via(std::forward<F>(f)); }

	template <typename... args_t> insert_unique_result insert_unique_emplace(args_t&&... args) { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::forward<args_t>(args)...); }); }

	struct volatile_insert_unique_result
	{
		volatile_iterator iterator;
		bool hadCollision;
		bool wasEmpty;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile
	{
		bool wasEmpty = false;
		bool collided;
		volatile_iterator i2 = insert_inner([&](iterator& i, height_t currentHeight, const rcref<volatile sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.template const_cast_to<link_t>().get_ptr();
			f(i);
			rcptr<volatile link_t> collidedWith = l->insert_unique(wasEmpty, currentHeight, sentinel);
			collided = !!collidedWith;
			if (collided)
			{
				i.m_link = std::move(collidedWith).template const_cast_to<link_t>();
				return false;
			}
			return true;
		});
		return { std::move(i2), collided, wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<payload_t>&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<payload_t>&>
		&& std::is_invocable_v<F, payload_t&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(*i.get()); }); }

	volatile_insert_unique_result insert_unique(const payload_t& src) volatile { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(src); }); }
	volatile_insert_unique_result insert_unique(payload_t&& src) volatile { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<payload_t, F&&>
		&& !std::is_convertible_v<F, const payload_t&>
		&& !std::is_convertible_v<F, payload_t&&>,
		volatile_insert_unique_result>
	insert_unique(F&& f) volatile { return insert_unique_via(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_unique_result insert_unique_emplace(args_t&&... args) volatile { return insert_unique_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::forward<args_t>(args)...); }); }
};


template <typename key_t, class payload_t, class comparator_t, class allocator_type>
class container_skiplist<true, key_t, payload_t, comparator_t, allocator_type> : public container_skiplist_base<key_t, payload_t, comparator_t, allocator_type>
{
private:
	typedef container_skiplist<true, key_t, payload_t, comparator_t, allocator_type> this_t;
	typedef container_skiplist_base<key_t, payload_t, comparator_t, allocator_type> base_t;

	container_skiplist(const this_t&) = delete;
	container_skiplist(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) = delete;
	this_t& operator=(const volatile this_t&) = delete;

	this_t& operator=(const this_t&) volatile = delete;
	this_t& operator=(const volatile this_t&) volatile = delete;

	using typename base_t::link_t;
	using typename base_t::sentinel_link_t;
	using typename base_t::height_t;

	using base_t::insert_inner;

public:
	using typename base_t::iterator;
	using typename base_t::volatile_iterator;

	container_skiplist() { }

	container_skiplist(this_t&& src)
		: base_t(std::move(src))
	{ }

	explicit container_skiplist(volatile allocator_type& al)
		: base_t(al)
	{ }

	container_skiplist(const std::initializer_list<payload_t>& src)
	{
		for (auto& entry : src)
			insert_multi(entry);
	}

	container_skiplist(volatile allocator_type& al, const std::initializer_list<payload_t>& src)
		: base_t(al)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	this_t& operator=(this_t&& src)
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	volatile this_t& operator=(this_t&& src) volatile
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	this_t& operator=(const std::initializer_list<payload_t>& src)
	{
		this_t tmp(src);
		base_t::operator=(std::move(tmp));
		return *this;
	}

	volatile this_t& operator=(const std::initializer_list<payload_t>& src) volatile
	{
		this_t tmp(src);
		base_t::operator=(std::move(tmp));
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.insert_multi(std::forward<args_t>(args)), ...);
		return result;
	}

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_multi_via(F&& f)
	{
		return insert_inner([&](iterator& i, height_t currentHeight, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.template const_cast_to<link_t>().get_ptr();
			f(i);
			l->insert_multi(currentHeight, sentinel);
			return true;
		});
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<payload_t>&>,
		iterator>
	insert_multi_via(F&& f) { return insert_multi_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, payload_t&>,
		iterator>
	insert_multi_via(F&& f) { return insert_multi_via([&](iterator& i) { f(*i.get()); }); }

	iterator insert_multi(const payload_t& src) { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(src); }); }
	iterator insert_multi(payload_t&& src) { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<payload_t, F&&>
		&& !std::is_convertible_v<F, const payload_t&>
		&& !std::is_convertible_v<F, payload_t&&>,
		iterator>
	insert_multi(F&& f) { return insert_multi_via(std::forward<F>(f)); }

	template <typename... args_t> iterator insert_multi_emplace(args_t&&... args) { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::forward<args_t>(args)...); }); }


	struct volatile_insert_multi_result
	{
		volatile_iterator iterator;
		bool wasEmpty;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_multi_result>
	insert_multi_via(F&& f) volatile
	{
		bool wasEmpty = false;
		volatile_iterator i2 = insert_inner([&](iterator& i, height_t currentHeight, const rcref<volatile sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.template const_cast_to<link_t>().get_ptr();
			f(i);
			l->insert_multi(wasEmpty, currentHeight, sentinel);
			return true;
		});
		return { std::move(i2), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<payload_t>&>,
		volatile_insert_multi_result>
	insert_multi_via(F&& f) volatile { return insert_multi_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<payload_t>&>
		&& std::is_invocable_v<F, payload_t&>,
		volatile_insert_multi_result>
	insert_multi_via(F&& f) volatile { return insert_multi_via([&](iterator& i) { f(*i.get()); }); }

	volatile_insert_multi_result insert_multi(const payload_t& src) volatile { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(src); }); }
	volatile_insert_multi_result insert_multi(payload_t&& src) volatile { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<payload_t, F&&>
		&& !std::is_convertible_v<F, const payload_t&>
		&& !std::is_convertible_v<F, payload_t&&>,
		volatile_insert_multi_result>
	insert_multi(F&& f) volatile { return insert_multi_via(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_multi_result insert_multi_emplace(args_t&&... args) volatile { return insert_multi_via([&](const rcref<payload_t>& p) { placement_rcnew(p.get_ptr(), *p.get_desc())(std::forward<args_t>(args)...); }); }
};


}


#endif
