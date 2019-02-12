//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CONTAINER_SKIPLIST
#define COGS_CONTAINER_SKIPLIST

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


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


/// @ingroup LockFreeCollections
/// @brief Default container_skiplist payload type
/// @tparam key_t The type used to compare elements.
template <typename key_t>
class default_container_skiplist_payload
{
private:
	typedef default_container_skiplist_payload<key_t> this_t;

	key_t	m_key;

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

	const key_t& get_key() const	{ return m_key; }
};


/// @ingroup LockFreeCollections
/// @brief A skip-list container collection.
/// @tparam key_t The type used to compare elements.
/// @tparam payload_t Type to contain, if different from key_t.  default_container_skiplist_payload<key_t>
template <typename key_t, class payload_t = default_container_skiplist_payload<key_t>, class comparator_t = default_comparator, class allocator_type = default_allocator>
class container_skiplist
{
private:
	enum class link_mode
	{
		inserting = 0,
		normal = 1,
		removing = 2,
		removing_next = 3
	};

	static constexpr size_t max_height = (sizeof(size_t) * 8) - 1;
	typedef typename range_to_int<0, max_height>::type height_t;

	typedef container_skiplist<key_t, payload_t, comparator_t, allocator_type> this_t;

	class link_t : public object
	{
	public:
		class links_t
		{
		public:
			link_mode		m_mode;
			rcptr<link_t>	m_next;
			rcptr<link_t>	m_prev;
		};

		typedef transactable<links_t> transactable_t;
		typedef typename transactable_t::read_token read_token;
		typedef typename transactable_t::write_token write_token;

	private:
		link_t(link_t&) = delete;
		link_t& operator=(const link_t&) = delete;

	public:
		ptr<transactable_t>	m_links;
		height_t m_height;

		// Primary mode starts as link_mode::inserting at the start of a volatile insert.
		// As the last insert is completed for this link, the primary mode is changed to link_mode::normal.
		// A remove will change the primary mode to link_mode::removing.
		// If an insert finds the mode has been changed to link_mode::removing, the insert owns the removal.
		alignas (atomic::get_alignment_v<link_mode>) link_mode m_primaryMode;

		virtual       payload_t* get_payload()			{ return NULL; }
		virtual const payload_t* get_payload() const	{ return NULL; }

		link_t(height_t height, link_mode linkMode)
			: m_height(height),
			m_primaryMode(linkMode)
		{
			height_t heightPlusOne = height + 1;
			m_links = allocator_container<default_allocator>::template allocate_type<transactable_t>(heightPlusOne);
			for (size_t i = 0; i <= height; i++)
				new (m_links.get_ptr() + i) transactable_t();
			for (height_t i = 0; i <= height; i++)
				m_links[i]->m_mode = linkMode;
		}

		explicit link_t(height_t height)
			: m_height(height),
			m_primaryMode(link_mode::inserting)
		{
			height_t heightPlusOne = height + 1;
			m_links = allocator_container<default_allocator>::template allocate_type<transactable_t>(heightPlusOne);
			for (size_t i = 0; i <= height; i++)
				new (m_links.get_ptr() + i) transactable_t();
			for (height_t i = 0; i <= height; i++)
				m_links[i]->m_mode = link_mode::inserting;
		}

		explicit link_t(link_mode linkMode)
			: m_primaryMode(linkMode)
		{ }

		explicit link_t()
			: m_primaryMode(link_mode::inserting)
		{ }

		~link_t()
		{
			if (!!m_links)
				default_allocator::destruct_deallocate_type<transactable_t>(m_links, m_height + 1);
		}

		void initialize(height_t height, link_mode linkMode = link_mode::inserting)
		{
			COGS_ASSERT(!m_links);
			m_height = height;
			m_primaryMode = linkMode;
			height_t heightPlusOne = height + 1;
			m_links = allocator_container<default_allocator>::template allocate_type<transactable_t>(heightPlusOne);
			for (size_t i = 0; i <= height; i++)
				new (m_links.get_ptr() + i) transactable_t();
			for (height_t i = 0; i <= height; i++)
			{
				transactable_t* t = &(m_links[i]);
				(*t)->m_mode = linkMode;
			}
		}

		virtual bool is_sentinel() const			{ return false; }
		virtual bool is_sentinel() const volatile	{ return false; }

		bool is_removed()							{ return m_primaryMode == link_mode::removing; }
		bool is_removed() volatile					{ link_mode primaryMode; atomic::load(m_primaryMode, primaryMode); return primaryMode == link_mode::removing; }
		
		bool is_removed(height_t height)			{ return m_links[height]->m_mode == link_mode::removing; }
		bool is_removed(height_t height) volatile	{ read_token rt; return !begin_read_and_complete(height, rt, false); }

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
			m_links[level]->m_next->m_links[level]->m_prev = m_links[level]->m_prev;
			m_links[level]->m_prev->m_links[level]->m_next = m_links[level]->m_next;
			m_links[level]->m_prev.release();
			m_links[level]->m_next.release();
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
					remove_from_level(wasLast, 0);	// Ensure removed at least from bottom level before return.
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
			height_t level = m_height;
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
			height_t level = m_height;
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
			COGS_ASSERT(level <= m_height);
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
			COGS_ASSERT(level <= m_height);
			bool result = false;
			for (;;)
			{
				read_token rt;
				if (!begin_read_and_complete(level, rt, false))
					break;
				rcptr<volatile link_t> prev = rt->m_prev;
				read_token rt_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
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
				COGS_ASSERT(level <= prev->m_height);
				if (!prev->m_links[level].promote_read_token(rt_prev, wt_prev))
					continue;
				wt_prev->m_mode = link_mode::removing_next;
				if (!prev->m_links[level].end_write(wt_prev))
					continue;
				if (!level)
					wasLast = maybeLast;
				result = true;
				prev->m_links[level].begin_read(rt_prev);
				if (rt_prev->m_mode == link_mode::removing_next)	// If not spontaneously completed by another thread.
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
			COGS_ASSERT(level <= prev->m_height);
			COGS_ASSERT(level <= next->m_height);
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
				if (rt_prev->m_next.get_ptr() == this)	// otherwise, we're done here.
				{
					COGS_ASSERT(rt_prev->m_mode == link_mode::removing_next);
					write_token wt_prev;
					if (!prev->m_links[level].promote_read_token(rt_prev, wt_prev))
						continue;
					wt_prev->m_next = rt->m_next;
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
			COGS_ASSERT(level <= next->m_height);
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
			COGS_ASSERT(level <= m_height);
			COGS_ASSERT(rt->m_mode == link_mode::inserting);

			// rt->m_inserting is known to be set.  We know the next time this link is written to, it's
			// going to be to remove the inserting flag, so we don't need a retry loop.
			rcptr<volatile link_t> next = rt->m_next;
			COGS_ASSERT(level <= next->m_height);
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
			COGS_ASSERT(level <= m_height);
			read_token rt;
			m_links[level].begin_read(rt);
			if (rt->m_mode == link_mode::inserting)
				complete_insert(level, rt);
		}

		bool begin_read_and_complete(height_t level, read_token& rt, bool returnTokenEvenIfRemoved) volatile	// returns false this link was removed.
		{
			COGS_ASSERT(level <= m_height);
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
				}
				break;
			}
			COGS_ASSERT(!notRemoved || (rt->m_mode == link_mode::normal));
			return notRemoved;
		}

		void complete(height_t level) volatile
		{
			read_token rt;
			begin_read_and_complete(level, rt, false);
		}

		// Scan for the last equal element, and add after it.
		// Ensures elements appear in the list in the order inserted if equal.
		void insert_multi_inner(height_t level, const key_t& criteria, const rcptr<link_t>& startFrom, bool& prevWasEqual, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> next;
			rcptr<link_t> prev = startFrom;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
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
					m_links[level]->m_mode = link_mode::normal;
					m_links[level]->m_next = next;
					m_links[level]->m_prev = prev;
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
				}
				break;
			}
		}

		// insert_multi_from_front_inner() is just like insert_multi_from_end_inner(), and should have the same performance characteristics,
		// however insert_multi_from_front_inner() is used if the criteria is < the first element, to optimize for prepend.
		void insert_multi_from_front_inner(bool& wasEmpty, height_t level, const key_t& criteria, const rcptr<volatile link_t>& startFrom, bool& prevWasEqual, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			write_token wt;
			rcptr<volatile link_t> next;
			rcptr<volatile link_t> prev = startFrom;
			bool insertedLowerLevels = false;
			for (;;)
			{
				bool emptyList = false;
				if (!prev->begin_read_and_complete(level, rt, true))
				{
					prevWasEqual = false;
					prev = rt->m_prev;
					COGS_ASSERT(!!prev);
					continue;
				}
				next = rt->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (!emptyList && (next.get_ptr() != sentinelPtr))	// if next is not sentinel
					{
						const key_t& nextCriteria = next.template const_cast_to<link_t>()->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
							continue;
						}
					}
				}
				if (!!level && !insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = next;
					insert_multi_from_front_inner(wasEmpty, level - 1, criteria, prev, prevWasEqual, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev->m_height);
					if (!prev->m_links[level].promote_read_token(rt, wt))
						continue;
					next->complete_insert(level);
					m_links[level]->m_next = next.template const_cast_to<link_t>();
					m_links[level]->m_prev = prev.template const_cast_to<link_t>();
					wt->m_next = this_rcptr;
					bool maybeEmpty = next.get_ptr() == prev.get_ptr();
					if (!prev->m_links[level].end_write(wt))
						continue;
					if (!level)
						wasEmpty = maybeEmpty;
					complete(level);
				}
				break;
			}
		}

		void insert_multi_from_end_inner(bool& wasEmpty, height_t level, const key_t& criteria, const rcptr<volatile link_t>& startFrom, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> next = startFrom;
			bool insertedLowerLevels = false;
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& prevCriteria = prev.template const_cast_to<link_t>()->get_payload()->get_key();
						if (comparator_t::is_less_than(criteria, prevCriteria))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!!level && !insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = prev;
					insert_multi_from_end_inner(wasEmpty, level - 1, criteria, next, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev->m_height);
					if (!prev->m_links[level].promote_read_token(rt_prev, wt))
						continue;
					next->complete_insert(level);
					m_links[level]->m_next = next.template const_cast_to<link_t>();
					m_links[level]->m_prev = prev.template const_cast_to<link_t>();
					wt->m_next = this_rcptr;
					bool maybeEmpty = prev.get_ptr() == next.get_ptr();
					if (!prev->m_links[level].end_write(wt))
						continue;
					if (!level)
						wasEmpty = maybeEmpty;
					complete(level);
				}
				break;
			}
		}

		// Scan for first equal element, removes all equal elements, inserts.
		void insert_replace_inner(height_t level, const key_t& criteria, const rcptr<link_t>& startFrom, bool& collision, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (next != lastAdjacent)
						{
							if (comparator_t::is_less_than(next->get_payload()->get_key(), criteria))
							{
								prev = next;
								continue;
							}
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
					{
						// If at the bottom level, remove matching elements before inserting this.
						if (!comparator_t::is_less_than(criteria, next->get_payload()->get_key()))
						{
							collision |= next->remove();
							continue;
						}
					}
				}
				else //if (!!level)
				{
					rcptr<link_t> newLastAdjacent = next;
					insert_replace_inner(level - 1, criteria, prev, collision, newLastAdjacent, sentinelPtr);
				}
				if (level <= m_height)
				{
					m_links[level]->m_mode = link_mode::normal;
					m_links[level]->m_next = next;
					m_links[level]->m_prev = prev;
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
				}
				break;
			}
		}

		// Scan for any equal element, and abort if already present.
		// Returns null if no matching element, otherwise returns matching element.
		rcptr<link_t> insert_unique_inner(height_t level, const key_t& criteria, const rcptr<link_t>& startFrom, rcptr<link_t>& lastAdjacent, link_t* sentinelPtr)
		{
			rcptr<link_t> result;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			for (;;)
			{
				COGS_ASSERT(level <= prev->m_height);
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = next;
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							result = next;
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
					m_links[level]->m_mode = link_mode::normal;
					m_links[level]->m_next = next;
					m_links[level]->m_prev = prev;
					COGS_ASSERT(level <= next->m_height);
					prev->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
				}
				break;
			}
			return result;
		}

		// If a volatile non-dupe insert, we can't assume the first match isn't in the process of being removed.
		// Scan all the way down to lowest level before declaring a duplicate.  Will be efficient due to lastAdjacent var.
		rcptr<volatile link_t> insert_unique_inner(height_t level, const key_t& criteria, const rcptr<volatile link_t>& startFrom, rcptr<volatile link_t>& lastAdjacent, volatile link_t* sentinelPtr)
		{
			read_token rt;
			read_token rt_prev;
			read_token rt_prev_next;
			write_token wt;
			rcptr<volatile link_t> result;
			rcptr<volatile link_t> prev;
			rcptr<volatile link_t> next = sentinelPtr;
			bool insertedLowerLevels = false;
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template const_cast_to<link_t>()->get_payload()->get_key()))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
					{
						// If at the bottom level, abort if there is a match
						if (!comparator_t::is_less_than(prev.template const_cast_to<link_t>()->get_payload()->get_key(), criteria))	// if prev and arg are equal
						{
							result = prev;
							break;
						}
					}
				}
				else if (!insertedLowerLevels)
				{
					insertedLowerLevels = true;
					rcptr<volatile link_t> newLastAdjacent = prev;
					result = insert_unique_inner(level - 1, criteria, next, newLastAdjacent, sentinelPtr);
					if (!!result)
						break;
				}
				if (level <= m_height)
				{
					COGS_ASSERT(level <= prev->m_height);
					if (!prev->m_links[level].promote_read_token(rt_prev, wt))
						continue;
					next->complete_insert(level);
					m_links[level]->m_next = next.template const_cast_to<link_t>();
					m_links[level]->m_prev = prev.template const_cast_to<link_t>();
					wt->m_next = this_rcptr;
					if (!prev->m_links[level].end_write(wt))
						continue;
					complete(level);
				}
				break;
			}
			return result;
		}

		void insert_multi(height_t height, const rcptr<link_t>& sentinel)
		{
			m_primaryMode = link_mode::normal;
			link_t* sentinelPtr = sentinel.get_ptr();
			const key_t& criteria = get_payload()->get_key();

			// Append/Prepend optimization
			height_t level = 0;
			rcptr<link_t> prev = sentinelPtr->m_links[0]->m_prev;
			if ((prev.get_ptr() == sentinelPtr) || !comparator_t::is_less_than(prev->get_payload()->get_key(), criteria))
			{
				for (;;)
				{
					m_links[level]->m_mode = link_mode::normal;
					m_links[level]->m_next = sentinel;
					m_links[level]->m_prev = prev;
					prev->m_links[level]->m_next = sentinel->m_links[level]->m_prev = this_rcptr;
					if (level == m_height)
						break;
					++level;
					prev = sentinelPtr->m_links[level]->m_prev;
				}
			}
			else
			{
				rcptr<link_t> next = sentinelPtr->m_links[0]->m_next;
				if ((next.get_ptr() == sentinelPtr) || comparator_t::is_less_than(criteria, next->get_payload()->get_key()))
				{
					for (;;)
					{
						m_links[level]->m_mode = link_mode::normal;
						m_links[level]->m_next = next;
						m_links[level]->m_prev = sentinel;
						COGS_ASSERT(level <= next->m_height);
						sentinel->m_links[level]->m_next = next->m_links[level]->m_prev = this_rcptr;
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

		void insert_multi(bool& wasEmpty, height_t height, const rcptr<volatile link_t>& sentinel)
		{
			// Append/Prepend optimization.
			// Whether or not we scan forward or in reverse, the performance characteristics should be equivalent.
			// Though, on the assumption that new elements may be larger then existin ones, we default to doing
			// a reverse scan.  The reverse scan is optimized for append.
			// If new node is < the first node, we scan forward, which is optimized for prepend.
			rcptr<volatile link_t> lastAdjacent;
			volatile link_t* sentinelPtr = sentinel.get_ptr();
			const key_t& criteria = get_payload()->get_key();
			rcptr<link_t> next;
			for (;;)
			{
				read_token rt;
				sentinelPtr->begin_read_and_complete(0, rt, false);
				next = rt->m_next;
				if (!!next)
					break;
				COGS_ASSERT(!sentinelPtr->m_links[0].is_current(rt));
			}

			if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next->get_payload()->get_key()))
				insert_multi_from_end_inner(wasEmpty, height, criteria, sentinel, lastAdjacent, sentinelPtr);
			else
			{
				bool prevWasEqual = false;
				insert_multi_from_front_inner(wasEmpty, height, criteria, sentinel, prevWasEqual, lastAdjacent, sentinelPtr);
			}
			if (!atomic::compare_exchange(m_primaryMode, link_mode::normal, link_mode::inserting))
			{
				bool wasLast;
				remove_from_all_levels(wasLast);	// could only fail because a remove was initiated.  Needs to be owned by this thread.
			}
		}

		bool insert_replace(height_t height, const rcptr<link_t>& sentinel)
		{
			bool collision = false;
			rcptr<link_t> lastAdjacent;
			m_primaryMode = link_mode::normal;
			insert_replace_inner(height, get_payload()->get_key(), sentinel, collision, lastAdjacent, sentinel.get_ptr());
			return collision;
		}

		rcptr<link_t> insert_unique(height_t height, const rcptr<link_t>& sentinel)
		{
			rcptr<link_t> lastAdjacent;
			m_primaryMode = link_mode::normal;
			return insert_unique_inner(height, get_payload()->get_key(), sentinel, lastAdjacent, sentinel.get_ptr());
		}

		rcptr<volatile link_t> insert_unique(height_t height, const rcptr<volatile link_t>& sentinel)
		{
			rcptr<volatile link_t> lastAdjacent;
			rcptr<volatile link_t> result = insert_unique_inner(height, get_payload()->get_key(), sentinel, lastAdjacent, sentinel.get_ptr());
			if (!result)	// If no collision, and was inesrted.
			{
				if (!atomic::compare_exchange(m_primaryMode, link_mode::normal, link_mode::inserting))
				{
					bool wasLast;
					remove_from_all_levels(wasLast);	// could only fail because a remove was initiated.  Needs to be owned by this thread.
				}
			}
			return result;
		}
	};

	typedef typename link_t::transactable_t transactable_t;
	typedef typename link_t::read_token read_token;
	typedef typename link_t::write_token write_token;

	class payload_link_t : public link_t
	{
	private:
		typename placement<payload_t>	m_value;

	public:
		virtual       payload_t* get_payload()			{ return &m_value.get(); }
		virtual const payload_t* get_payload() const	{ return &m_value.get(); }

		payload_link_t()
		{
			new (get_payload()) payload_t();
		}

		explicit payload_link_t(const payload_t& t)
		{
			new (get_payload()) payload_t(t);
		}

		explicit payload_link_t(height_t height)
			:	link_t(height)
		{
			new (get_payload()) payload_t();
		}

		payload_link_t(const payload_t& t, height_t height)
			: link_t(height)
		{
			new (get_payload()) payload_t(t);
		}

		~payload_link_t()				{ m_value.destruct(); }

		rcref<payload_t>	get_obj()	{ return get_self_rcref(get_payload()); }
	};

	template <typename T>
	class aux_payload_link_t : public payload_link_t
	{
	public:
		typedef typename std::remove_cv<T>::type T2;

		delayed_construction<T2> m_aux;

		aux_payload_link_t()
		{
			placement_rcnew(this_desc, &m_aux.get());
		}

		explicit aux_payload_link_t(const payload_t& t)
			: payload_link_t(t)
		{
			placement_rcnew(this_desc, &m_aux.get());
		}

		const rcref<T>& get_aux_ref(unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned())
		{
			storage.set(&m_aux.get(), this_desc);
			return storage.dereference();
		}
	};

	class sentinel_link_t : public link_t
	{
	public:
		sentinel_link_t()
			: link_t(max_height, link_mode::normal)
		{
			link_t::m_links[0]->m_prev = link_t::m_links[0]->m_next = this_rcptr;
		}

		void clear()
		{
			link_t::m_links[0]->m_prev.release();
			link_t::m_links[0]->m_next.release();
		}

		virtual bool is_sentinel() const			{ return true; }
		virtual bool is_sentinel() const volatile	{ return true; }
	};

	class height_and_count_t
	{
	public:
		alignas (atomic::get_alignment_v<height_t>) height_t m_currentHeight;
		alignas (atomic::get_alignment_v<ptrdiff_t>) ptrdiff_t m_count;
	};

	rcptr<link_t> m_sentinel;
	alignas (atomic::get_alignment_v<height_and_count_t>) height_and_count_t m_heightAndCount;
	allocator_container<allocator_type> m_allocator;

	void dec_count()
	{
		if (--(m_heightAndCount.m_count) < m_heightAndCount.m_currentHeight)
			m_heightAndCount.m_currentHeight = m_heightAndCount.m_count;
	}

	void dec_count() volatile
	{
		height_and_count_t oldHeightAndCount;
		height_and_count_t newHeightAndCount;
		atomic::load(m_heightAndCount, oldHeightAndCount);
		do {				// might bounce below zero if something is removed in the middle of being added
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

	// 1) 1/2 chance the end bit is 1
	// 2) 1/4 chance the end bits are 10		(not 00, 01, or 11)
	// 3) 1/8 chance the end bits are 100		(not 000, 001, 010, 011, 101, 110, 111)
	// 4) 1/16 chance the end bits are 1000		(etc.)
	static height_t generate_height(height_t currentHeight)
	{
		// Normalized to 1-based, then extend by 1	// 0 will not be returned, so normalizing back to 0-based
		height_t h = (height_t)random_coin_flips<height_t>(currentHeight + 2);
		COGS_ASSERT(!!h);
		if (!!h)
			--h;
		return h;
	}

	height_t generate_height() const	{ return generate_height(m_heightAndCount.m_currentHeight); }

	height_t generate_height() const volatile
	{
		height_t currentHeight;
		atomic::load(m_heightAndCount.m_currentHeight, currentHeight);
		return generate_height(currentHeight);
	}

	height_t accommodate_height(height_t newHeight)
	{
		if (newHeight <= m_heightAndCount.m_currentHeight)
			return m_heightAndCount.m_currentHeight;
		
		COGS_ASSERT(newHeight <= max_height);

		rcptr<link_t>& sentinel = m_sentinel;
		transactable_t* links = sentinel->m_links.get_ptr();
		transactable_t* pos = links + newHeight;
		transactable_t* end = links + m_heightAndCount.m_currentHeight;
		for (;;)
		{
			if (!!(*pos)->m_next)
				break;
			(*pos)->m_prev = (*pos)->m_next = m_sentinel;
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

		rcptr<volatile link_t> sentinel = m_sentinel;
		transactable_t* links = sentinel->m_links.get_ptr();
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
			
			wt->m_prev = wt->m_next = sentinel.template const_cast_to<link_t>();
			pos->end_write(wt);	// no need to verify, the first write is always to set these.
			if (--pos == end)
				break;
		}

		atomic::compare_exchange(m_heightAndCount.m_currentHeight, newHeight, oldHeight);
		// ignore failure to set height
		// it doesn't matter if another thread increased it, and it's not really 
		// detrimental if it to zero - implying the list is extremely simple right now.

		return newHeight;
	}

	void clear_inner()
	{
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
			l = next;
		}
	}

	// To support use of zero-initialized buffer placement, lazy-initialize the sentinel
	rcptr<link_t>& lazy_init_sentinel()
	{
		if (!m_sentinel)
			m_sentinel = container_rcnew(m_allocator, sentinel_link_t);
		return m_sentinel;
	}

	rcptr<link_t> lazy_init_sentinel() volatile
	{
		rcptr<link_t> sentinel = m_sentinel;
		if (!sentinel)
		{
			rcptr<link_t> newSentinel = container_rcnew(m_allocator, sentinel_link_t);
			if (m_sentinel.compare_exchange(newSentinel, sentinel, sentinel))
				sentinel = newSentinel;
			else
			{
				sentinel_link_t* unusedSentinel = (sentinel_link_t*)newSentinel.get_ptr();
				unusedSentinel->clear();
			}
		}
		return sentinel;
	}

	container_skiplist(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;
	class preallocated_t;

	/// @brief A container_skiplist element iterator
	class iterator
	{
	protected:
		rcptr<link_t> m_link;

		friend class container_skiplist;
		friend class remove_token;

		iterator(const rcptr<link_t>& l) : m_link(l)	{ }

	public:
		void disown()	{ m_link.disown(); }

		iterator()												{ }
		iterator(const iterator& i)			: m_link(i.m_link)	{ }
		iterator(const remove_token& rt) : m_link(rt.m_link)	{ if (is_removed()) release(); }

		void release()				{ m_link.release(); }

		bool is_active() const		{ return !!m_link && !m_link->is_removed(); }
		bool is_removed() const		{ return !!m_link && m_link->is_removed(); }	// implies that it was in the list.  null m_link returns false

		iterator& operator++()
		{
			if (!!m_link)
			{
				do {
					m_link = m_link->m_links[0]->m_next;
					if (!!m_link->is_sentinel())
					{
						m_link.release();
						break;
					}
				} while (m_link->m_links[0]->m_mode == link_mode::removing);
			}
			return *this;
		}

		iterator& operator--()
		{
			if (!!m_link)
			{
				do {
					m_link = m_link->m_links[0]->m_prev;
					if (!!m_link->is_sentinel())
					{
						m_link.release();
						break;
					}
				} while (m_link->m_links[0]->m_mode == link_mode::removing);
			}
			return *this;
		}

		iterator operator++(int)						{ iterator i(*this); ++*this; return i; }
		iterator operator--(int)						{ iterator i(*this); --*this; return i; }

		bool operator!() const							{ return !m_link; }

		bool operator==(const iterator& i) const		{ return m_link == i.m_link; }
		bool operator==(const remove_token& rt) const	{ return m_link == rt.m_link; }

		bool operator!=(const iterator& i) const		{ return !operator==(i); }
		bool operator!=(const remove_token& rt) const	{ return !operator==(rt); }

		iterator& operator=(const iterator& i)			{ m_link = i.m_link; return *this; }
		iterator& operator=(const remove_token& rt)		{ m_link = rt.m_link; if (is_removed()) release(); return *this; }

		payload_t* get() const							{ return (!m_link) ? (payload_t*)0 : m_link->get_payload(); }
		payload_t& operator*() const					{ return *(m_link->get_payload()); }
		payload_t* operator->() const					{ return m_link->get_payload(); }

		rcptr<payload_t> get_obj() const
		{
			rcptr<payload_t> result;
			if (!!m_link)
				result = m_link.template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}
		
		rc_obj_base* get_desc() const		{ return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_link.get_desc(); }

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
	};

	/// @brief A volatile container_skiplist element iterator
	class volatile_iterator
	{
	protected:
		rcptr<volatile link_t> m_link;

		friend class container_skiplist;
		friend class volatile_remove_token;

		volatile_iterator(const rcptr<volatile link_t>& l) : m_link(l)	{ }

	public:
		void disown()	{ m_link.disown(); }

		volatile_iterator()																{ }
		volatile_iterator(const volatile_iterator& i)				: m_link(i.m_link)	{ }
		volatile_iterator(const volatile_remove_token& rt)			: m_link(rt.m_link)	{ if (is_removed()) release(); }
		volatile_iterator(const volatile volatile_iterator& i)		: m_link(i.m_link)	{ }
		volatile_iterator(const volatile volatile_remove_token& rt) : m_link(rt.m_link)	{ if (is_removed()) release(); }

		volatile_iterator& operator=(const volatile_iterator& i)					{ m_link = i.m_link; return *this; }
		volatile_iterator& operator=(const volatile_remove_token& rt)				{ m_link = rt.m_link; if (is_removed()) release(); return *this; }
		volatile_iterator& operator=(const volatile volatile_iterator& i)			{ m_link = i.m_link; return *this; }
		volatile_iterator& operator=(const volatile volatile_remove_token& rt)		{ m_link = rt.m_link; if (is_removed()) release(); return *this; }
		void operator=(const volatile_iterator& i) volatile							{ m_link = i.m_link; }
		void operator=(const volatile_remove_token& rt) volatile
		{
			rcptr<volatile link_t> lnk = rt.m_link; 
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = lnk;
		}

		bool is_active() const				{ return !!m_link && !m_link->is_removed(); }
		bool is_active() const volatile		{ rcptr<volatile link_t> lnk = m_link; return !!lnk && !!lnk->is_removed(); }

		bool is_removed() const				{ return !!m_link && m_link->is_removed(); }
		bool is_removed() const volatile	{ rcptr<volatile link_t> lnk = m_link; return !!lnk && lnk->is_removed(); }

		volatile_iterator& operator++()
		{
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(0, rt, true);
				rcptr<volatile link_t> nextLink;
				for (;;)
				{
					nextLink = rt->m_next;
					if (nextLink->is_sentinel())
					{
						m_link.release();
						break;
					}
					if (!nextLink->begin_read_and_complete(0, rt, true))
						continue;

					m_link = nextLink;
					break;
				}
			}
			return *this;
		}

		void operator++() volatile
		{
			rcptr<volatile link_t> oldLink = m_link;
			read_token rt;
			for (;;)
			{
				if (!oldLink)
					break;
				oldLink->begin_read_and_complete(0, rt, true);
				rcptr<volatile link_t> newLink = rt->m_next;
				for (;;)
				{
					if (newLink->is_sentinel())
						newLink.release();
					else if (!newLink->begin_read_and_complete(0, rt, true))
					{
						newLink = rt->m_next;
						continue;
					}
					break;
				}
				if (!m_link.compare_exchange(newLink, oldLink, oldLink))
					continue;
				break;
			}
		}

		volatile_iterator& operator--()
		{
			if (!!m_link)
			{
				read_token rt;
				rcptr<volatile link_t> newLink;
				for (;;)
				{
					m_link->begin_read_and_complete(0, rt, true);
					newLink = rt->m_prev;
					if (!newLink)
					{
						COGS_ASSERT(!m_link->m_links[0].is_current(rt));
						continue;
					}
					break;
				}
				for (;;)
				{
					if (newLink->is_sentinel())
					{
						m_link.release();
						break;
					}

					bool done = false;
					for (;;)
					{
						if (!!newLink->begin_read_and_complete(0, rt, true))
						{
							m_link = newLink;
							done = true;
							break;
						}
						rcptr<volatile link_t> tmp = rt->m_prev;
						if (!tmp)
						{
							COGS_ASSERT(!newLink->m_links[0].is_current(rt));
							continue;
						}
						newLink = tmp;
						break;	// but not done.
					}
					if (done)
						break;
					//continue;
				}
			}
			return *this;
		}

		void operator--() volatile
		{
			rcptr<volatile link_t> oldLink = m_link;
			read_token rt;
			for (;;)
			{
				if (!oldLink)
					break;
				oldLink->begin_read_and_complete(0, rt, true);
				rcptr<volatile link_t> newLink = rt->m_prev;
				if (!newLink)
				{
					COGS_ASSERT(!oldLink->m_links[0].is_current(rt));
					continue;
				}
				for (;;)
				{
					if (newLink->is_sentinel())
					{
						m_link.release();
						break;
					}

					bool done = false;
					for (;;)
					{
						if (!!newLink->begin_read_and_complete(0, rt, true))
						{
							done = true;
							break;
						}
						rcptr<volatile link_t> tmp = rt->m_prev;
						if (!tmp)
						{
							COGS_ASSERT(!newLink->m_links[0].is_current(rt));
							continue;
						}
						newLink = tmp;
						break;	// but not done.
					}
					if (done)
						break;
					//continue;
				}
				if (!m_link.compare_exchange(newLink, oldLink, oldLink))
					continue;
				break;
			}
		}

		volatile_iterator operator++(int)			{ volatile_iterator i(*this); ++*this; return i; }

		volatile_iterator operator++(int) volatile
		{
			volatile_iterator original(*this);
			for (;;)
			{
				volatile_iterator next = original;
				++next;
				if (!m_link.compare_exchange(next.m_link, original.m_link, original.m_link))
					continue;
				break;
			}
			return original;
		}

		volatile_iterator operator--(int)			{ volatile_iterator i(*this); --*this; return i; }

		volatile_iterator operator--(int) volatile
		{
			volatile_iterator original(*this);
			for (;;)
			{
				volatile_iterator prev = original;
				++prev;
				if (!m_link.compare_exchange(prev.m_link, original.m_link, original.m_link))
					continue;
				break;
			}
			return original;
		}

		bool operator!() const											{ return !m_link; }
		bool operator!() const volatile									{ return !m_link; }

		bool operator==(const volatile_iterator& i) const				{ return m_link == i.m_link; }
		bool operator==(const volatile volatile_iterator& i) const		{ return m_link == i.m_link; }
		bool operator==(const volatile_iterator& i) const volatile		{ return m_link == i.m_link; }
		bool operator==(const volatile_remove_token& rt) const			{ return m_link == rt.m_link; }
		bool operator==(const volatile volatile_remove_token& rt) const	{ return m_link == rt.m_link; }
		bool operator==(const volatile_remove_token& rt) const volatile	{ return m_link == rt.m_link; }

		bool operator!=(const volatile_iterator& i) const				{ return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const		{ return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile		{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& rt) const			{ return !operator==(rt); }
		bool operator!=(const volatile volatile_remove_token& rt) const	{ return !operator==(rt); }
		bool operator!=(const volatile_remove_token& rt) const volatile	{ return !operator==(rt); }

		payload_t* get() const											{ return (!m_link) ? (payload_t*)0 : m_link.template const_cast_to<link_t>()->get_payload(); }
		payload_t& operator*() const									{ return *(get()); }
		payload_t* operator->() const									{ return get(); }

		void release()													{ m_link.release(); }
		void release() volatile											{ m_link.release(); }

		rcptr<payload_t> get_obj() const
		{
			rcptr<payload_t> result;
			if (!!m_link)
				result = m_link.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}

		rc_obj_base* get_desc() const		{ return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_link.get_desc(); }

		volatile_iterator next() const	{ volatile_iterator result(this); ++result; return result; }
		volatile_iterator prev() const	{ volatile_iterator result(this); --result; return result; }

		bool compare_exchange(const volatile_iterator& src, const volatile_iterator& cmp) volatile
		{
			return m_link.compare_exchange(src.m_link, cmp.m_link);
		}

		bool compare_exchange(const volatile_iterator& src, const volatile_iterator& cmp, volatile_iterator& rtn) volatile
		{
			return m_link.compare_exchange(src.m_link, cmp.m_link, rtn.m_link);
		}
	};

	/// @brief A preallocated container_skiplist element
	class preallocated_t
	{
	protected:
		rcptr<link_t> m_link;

		friend class container_skiplist;

		preallocated_t(const rcptr<link_t>& l)
			: m_link(l)
		{ }

	public:
		void disown()	{ m_link.disown(); }

		preallocated_t()	{ }

		preallocated_t(const preallocated_t& src)
			: m_link(src.m_link)
		{ }

		void release()										{ m_link.release(); }

		bool operator!() const								{ return !m_link; }
		bool operator==(const preallocated_t& i) const		{ return m_link == i.m_link; }
		bool operator!=(const preallocated_t& i) const		{ return !operator==(i); }
		preallocated_t& operator=(const preallocated_t& i)	{ m_link = i.m_link; return *this; }

		payload_t* get() const							{ return (!m_link) ? (payload_t*)0 : m_link->get_payload(); }
		payload_t& operator*() const					{ return *(m_link->get_payload()); }
		payload_t* operator->() const					{ return m_link->get_payload(); }

		rcptr<payload_t> get_obj() const
		{
			rcptr<payload_t> result;
			if (!!m_link)
				result = m_link.template static_cast_to<payload_link_t>()->get_obj(); 
			return result;
		}

		rc_obj_base* get_desc() const		{ return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_link.get_desc(); }
	};

	/// @brief A container_skiplist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		weak_rcptr<link_t> m_link;

		friend class container_skiplist;
	public:
		remove_token()												{ }
		remove_token(const preallocated_t& i)	: m_link(i.m_link)	{ }
		remove_token(const iterator& i)			: m_link(i.m_link)	{ }
		remove_token(const remove_token& rt)	: m_link(rt.m_link)	{ }

		remove_token& operator=(const preallocated_t& i)		{ m_link = i.m_link; return *this; }
		remove_token& operator=(const iterator& i)				{ m_link = i.m_link; return *this; }
		remove_token& operator=(const remove_token& rt)			{ m_link = rt.m_link; return *this; }

		bool is_active() const									{ rcptr<link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		void release()											{ m_link.release(); }

		bool operator!() const									{ return !m_link; }

		bool operator==(const iterator& i) const				{ return m_link == i.m_link; }
		bool operator==(const remove_token& rt) const			{ return m_link == rt.m_link; }

		bool operator!=(const iterator& i) const				{ return !operator==(i); }
		bool operator!=(const remove_token& rt) const			{ return !operator==(rt); }
	};

	/// @brief A volatile container_skiplist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class volatile_remove_token
	{
	protected:
		weak_rcptr<volatile link_t> m_link;

		friend class container_skiplist;
	public:
		volatile_remove_token()																{ }
		volatile_remove_token(const             preallocated_t& i) : m_link(i.m_link)		{ }
		volatile_remove_token(const          volatile_iterator& i) : m_link(i.m_link)		{ }
		volatile_remove_token(const volatile volatile_iterator& i) : m_link(i.m_link)		{ }
		volatile_remove_token(const          volatile_remove_token& rt) : m_link(rt.m_link)	{ }
		volatile_remove_token(const volatile volatile_remove_token& rt) : m_link(rt.m_link)	{ }

		volatile_remove_token& operator=(const preallocated_t& i)					{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile_iterator& i)				{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile volatile_iterator& i)		{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile_remove_token& rt)			{ m_link = rt.m_link; return *this; }
		volatile_remove_token& operator=(const volatile volatile_remove_token& rt)	{ m_link = rt.m_link; return *this; }

		void operator=(const volatile_iterator& i) volatile							{ m_link = i.m_link; }
		void operator=(const volatile_remove_token& rt) volatile						{ m_link = rt.m_link; }

		bool is_active() const			{ rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }
		bool is_active() const volatile	{ rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		void release()									{ m_link.release(); }
		void release() volatile							{ m_link.release(); }

		bool operator!() const							{ return !m_link; }
		bool operator!() const volatile					{ return !m_link; }

		bool operator==(const volatile_iterator& i) const				{ return m_link == i.m_link; }
		bool operator==(const volatile volatile_iterator& i) const		{ return m_link == i.m_link; }
		bool operator==(const volatile_iterator& i) const volatile		{ return m_link == i.m_link; }
		bool operator==(const volatile_remove_token& rt) const			{ return m_link == rt.m_link; }
		bool operator==(const volatile volatile_remove_token& rt) const	{ return m_link == rt.m_link; }
		bool operator==(const volatile_remove_token& rt) const volatile	{ return m_link == rt.m_link; }

		bool operator!=(const volatile_iterator& i) const				{ return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const		{ return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile		{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& rt) const			{ return !operator==(rt); }
		bool operator!=(const volatile volatile_remove_token& rt) const	{ return !operator==(rt); }
		bool operator!=(const volatile_remove_token& rt) const volatile	{ return !operator==(rt); }
	};


	container_skiplist(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_sentinel(std::move(m_sentinel)),
		m_heightAndCount(m_heightAndCount)
	{ }

	this_t& operator=(this_t&& src)
	{
		clear_inner();
		m_allocator = std::move(src.m_allocator);
		m_sentinel = std::move(src.m_sentinel);
		m_heightAndCount = src.m_heightAndCount;
		return *this;
	}


	container_skiplist()
	{
		m_heightAndCount.m_currentHeight = 0;
		m_heightAndCount.m_count = 0;
	}

	~container_skiplist()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			height_t level = 0;
			for (;;)
			{
				if (!m_sentinel->m_links[level]->m_next)
					break;
				m_sentinel->m_links[level]->m_next.release();
				m_sentinel->m_links[level]->m_prev.release();
				if (level == max_height)
					break;
				++level;
			}
			m_sentinel.release();
		}
	}

	void clear()
	{
		if (!!m_heightAndCount.m_count)
		{
			clear_inner();
			height_t level = 0;
			for (;;)
			{
				if (m_sentinel->m_links[level]->m_next.get_ptr() == m_sentinel.get_ptr())
					break;
				m_sentinel->m_links[level]->m_next = m_sentinel;
				m_sentinel->m_links[level]->m_prev = m_sentinel;
				if (level == max_height)
					break;
				++level;
			}
			m_heightAndCount.m_count = 0;
			m_heightAndCount.m_currentHeight = 0;
		}
	}

	void drain() volatile
	{
		while (!!pop_last())
			;
	}

	size_t size() const				{ return m_heightAndCount.m_count; }
	size_t size() const volatile
	{
		ptrdiff_t sz;
		atomic::load(m_heightAndCount.m_count, sz);
		if (sz < 0)	// The count can bounce below zero, if an item is added, removed, and dec'ed before inc'ed.
			sz = 0;
		return sz;
	}

	bool is_empty() const			{ return !m_heightAndCount.m_count; }
	bool is_empty() const volatile
	{
		ptrdiff_t sz;
		atomic::load(m_heightAndCount.m_count, sz);
		return (sz <= 0);
	}

	bool operator!() const			{ return is_empty(); }
	bool operator!() const volatile	{ return is_empty(); }

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
		rcptr<volatile link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			rcptr<volatile link_t> lnk;
			for (;;)
			{
				read_token rt;
				bool b = sentinel->begin_read_and_complete(0, rt, false);
				COGS_ASSERT(b);	// sentinel will not be removed
				lnk = rt->m_next;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (!lnk->begin_read_and_complete(0, rt, false))	// just in case it needs help removing...
					continue;
				i.m_link = lnk;
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
		rcptr<volatile link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			rcptr<volatile link_t> lnk;
			for (;;)
			{
				read_token rt;
				bool b = sentinel->begin_read_and_complete(0, rt, false);
				COGS_ASSERT(b);	// sentinel will not be removed
				lnk = rt->m_prev;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (!lnk->begin_read_read_and_complete(0, rt, false))	// just in case it needs help removing...
					continue;
				i.m_link = lnk;
				break;
			}
		}
		return i;
	}

	preallocated_t preallocate() const volatile { preallocated_t i; i.m_link = container_rcnew(m_allocator, payload_link_t); return i; }
	preallocated_t preallocate(const payload_t& t) const volatile { preallocated_t i; i.m_link = container_rcnew(m_allocator, payload_link_t, t); return i; }

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(const payload_t& t, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t, t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	iterator insert_multi_preallocated(const preallocated_t& i)
	{
		iterator result;
		i.m_link->initialize(generate_height());
		result.m_link = i.m_link;
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		i.m_link->insert_multi(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return result;
	}

	volatile_iterator insert_multi_preallocated(const preallocated_t& i) volatile
	{
		bool wasEmpty;
		return insert_multi_preallocated(i, wasEmpty);
	}

	// wasEmpty is of limited usefulness, as parallel inserts and removes could
	// occur and invalidate the value before it's evaluated.  It's only useful if the access
	// pattern is known not to invalidate it.
	volatile_iterator insert_multi_preallocated(const preallocated_t& i, bool& wasEmpty) volatile
	{
		volatile_iterator result;
		i.m_link->initialize(generate_height());
		result.m_link = i.m_link;
		wasEmpty = false;
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link.template const_cast_to<link_t>()->m_height);
		i.m_link.template const_cast_to<link_t>()->insert_multi(wasEmpty, height, sentinel);
		assign_next(m_heightAndCount.m_count);
		return result;
	}

	iterator insert_multi(const payload_t& t)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		i.m_link->insert_multi(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return i;
	}

	volatile_iterator insert_multi(const payload_t& t) volatile
	{
		bool wasEmpty;
		return insert_multi(t, wasEmpty);
	}

	// wasEmpty is of limited usefulness, as parallel inserts and removes could
	// occur and invalidate the value before it's evaluated.  It's only useful is access
	// pattern is known not to invalidate it.
	volatile_iterator insert_multi(const payload_t& t, bool& wasEmpty) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link.template const_cast_to<link_t>()->m_height);
		i.m_link.template const_cast_to<link_t>()->insert_multi(wasEmpty, height, sentinel);
		assign_next(m_heightAndCount.m_count);
		return i;
	}

	iterator insert_replace_preallocated(const preallocated_t& i)
	{
		iterator result;
		i.m_link->initialize(generate_height());
		result.m_link = i.m_link;
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		i.m_link->insert_replace(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return result;
	}

	iterator insert_replace_preallocated(const preallocated_t& i, bool& collision)
	{
		iterator result;
		i.m_link->initialize(generate_height()); 
		result.m_link = i.m_link;
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		collision = i.m_link->insert_replace(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return result;
	}

	iterator insert_replace(const payload_t& t)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		i.m_link->insert_replace(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return i;
	}

	iterator insert_replace(const payload_t& t, bool& collision)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		collision = i.m_link->insert_replace(m_heightAndCount.m_currentHeight, sentinel);
		++(m_heightAndCount.m_count);
		return i;
	}

	// returns null iterator if collision
	iterator insert_unique_preallocated(const preallocated_t& i)
	{
		iterator result;
		i.m_link->initialize(generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		if (!i.m_link->insert_unique(m_heightAndCount.m_currentHeight, sentinel))
		{
			result.m_link = i.m_link;
			++(m_heightAndCount.m_count);
		}
		return result;
	}

	// If collision, sets collision to true and returns collided result.
	iterator insert_unique_preallocated(const preallocated_t& i, bool& collision)
	{
		iterator result;
		i.m_link->initialize(generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		rcptr<link_t> collidedWith = i.m_link->insert_unique(m_heightAndCount.m_currentHeight, sentinel);
		collision = !!collidedWith;
		if (collision)
			result.m_link = collidedWith;
		else
		{
			result.m_link = i.m_link;
			++(m_heightAndCount.m_count);
		}
		return result;
	}

	// returns null iterator if collision
	volatile_iterator insert_unique_preallocated(const preallocated_t& i) volatile
	{
		volatile_iterator result;
		i.m_link->initialize(generate_height());
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link->m_height);
		if (!i.m_link->insert_unique(height, sentinel))
		{
			result.m_link = i.m_link;
			assign_next(m_heightAndCount.m_count);
		}
		return result;
	}

	// If collision, sets collision to true and returns collided result.
	volatile_iterator insert_unique_preallocated(const preallocated_t& i, bool& collision) volatile
	{
		volatile_iterator result;
		i.m_link->initialize(generate_height());
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link->m_height);
		rcptr<volatile link_t> collidedWith = i.m_link->insert_unique(height, sentinel);
		collision = !!collidedWith;
		if (collision)
			result.m_link = collidedWith;
		else
		{
			result.m_link = i.m_link;
			assign_next(m_heightAndCount.m_count);
		}
		return result;
	}

	// returns null iterator if collision
	iterator insert_unique(const payload_t& t)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		if (!!i.m_link->insert_unique(m_heightAndCount.m_currentHeight, sentinel))
			i.m_link.release();
		else
			++(m_heightAndCount.m_count);
		return i;
	}

	// If collision, sets collision to true and returns collided result.
	iterator insert_unique(const payload_t& t, bool& collision)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<link_t>& sentinel = lazy_init_sentinel();
		accommodate_height(i.m_link->m_height);
		rcptr<link_t> collidedWith = i.m_link->insert_unique(m_heightAndCount.m_currentHeight, sentinel);
		collision = !!collidedWith;
		if (collision)
			i.m_link = collidedWith;
		else
			++(m_heightAndCount.m_count);
		return i;
	}

	// returns null iterator if collision
	volatile_iterator insert_unique(const payload_t& t) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link.template const_cast_to<link_t>()->m_height);
		if (!!i.m_link.template const_cast_to<link_t>()->insert_unique(height, sentinel))
			i.m_link.release();	
		else
			assign_next(m_heightAndCount.m_count);
		return i;
	}

	// If collision, sets collision to true and returns collided result.
	volatile_iterator insert_unique(const payload_t& t, bool& collision) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t, generate_height());
		rcptr<volatile link_t> sentinel = lazy_init_sentinel();
		height_t height = accommodate_height(i.m_link.template const_cast_to<link_t>()->m_height);
		rcptr<link_t> collidedWith = i.m_link.template const_cast_to<link_t>()->insert_unique(height, sentinel);
		collision = !!collidedWith;
		if (collision)
			i.m_link = collidedWith;
		else
			assign_next(m_heightAndCount.m_count);
		return i;
	}

	bool remove(const iterator& i)
	{
		bool b = false;
		if (!!i.m_link)
		{
			b = i.m_link->remove();
			if (b)
				dec_count();
		}
		return b;
	}

	bool remove(const remove_token& rt)
	{
		iterator i(rt);
		return remove(i);
	}

	bool remove(const volatile_iterator& i, bool& wasLast) volatile
	{
		wasLast = false;
		rcptr<volatile link_t> lnk = i.m_link;
		bool b = false;
		if (!!lnk)
		{
			b = lnk->remove(wasLast);
			if (b)
				dec_count();
		}
		return b;
	}

	bool remove(const volatile_remove_token& rt, bool& wasLast) volatile
	{
		volatile_iterator i(rt);
		return remove(i, wasLast);
	}

	bool remove(const volatile_iterator& i) volatile
	{
		bool wasEmpty;
		return remove(i, wasEmpty);
	}

	bool remove(const volatile_remove_token& rt) volatile
	{
		volatile_iterator i(rt);
		bool wasEmpty; 
		return remove(i, wasEmpty);
	}

	iterator pop_first()
	{
		iterator i = get_first();
		remove(i);
		return i;
	}

	iterator pop_first(bool& wasLast)
	{
		iterator i = get_first();
		remove(i);
		wasLast = !m_heightAndCount.m_count;
		return i;
	}

	iterator pop_last()
	{
		iterator i = get_last();
		remove(i);
		return i;
	}

	iterator pop_last(bool& wasLast)
	{
		iterator i = get_last();
		remove(i);
		wasLast = !m_heightAndCount.m_count;
		return i;
	}

	volatile_iterator pop_first() volatile
	{
		bool wasLast;
		return pop_first(wasLast);
	}

	volatile_iterator pop_last() volatile
	{
		bool wasLast;
		return pop_last(wasLast);
	}

	volatile_iterator pop_first(bool& wasLast) volatile
	{
		volatile_iterator itor;
		rcptr<volatile link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			read_token rt_sentinel;
			for (;;)
			{
				wasLast = false;
				bool b = sentinel->begin_read_and_complete(0, rt_sentinel, false);	// sentinel won't get deleted, no need to check result
				COGS_ASSERT(b);
				rcptr<volatile link_t> prev = rt_sentinel->m_prev;

				rcptr<volatile link_t> firstLink = rt_sentinel->m_next;
				if (firstLink.get_ptr() == sentinel.get_ptr())
					break;
				if (firstLink->is_removed(0))	// also completes other states
					continue;

				write_token wt;
				if (!sentinel->m_links[0].promote_read_token(rt_sentinel, wt))
					continue;

				COGS_ASSERT(wt->m_mode == link_mode::normal);
				COGS_ASSERT(wt->m_next == firstLink);

				wasLast = wt->m_next.get_ptr() == prev.get_ptr();

				wt->m_mode = link_mode::removing_next;
				if (!sentinel->m_links[0].end_write(wt))
					continue;

				sentinel->begin_read_and_complete(0, rt_sentinel, false);	// completes remove-next
				if (!firstLink->remove())
					continue;	// someone else marked it first.
				dec_count();
				itor.m_link = firstLink;
				break;
			}
		}
		
		return itor;
	}

	volatile_iterator pop_last(bool& wasLast) volatile
	{
		rcptr<volatile link_t> sentinel = m_sentinel;
		read_token rt_last;
		read_token rt_prev;
		read_token rt_sentinel;
		volatile_iterator itor;
		if (!!sentinel)
		{
			for (;;)
			{
				wasLast = false;
				sentinel->begin_read_and_complete(0, rt_sentinel, false);	// sentinel won't get deleted, no need to check result
				rcptr<volatile link_t> lastLink = rt_sentinel->m_prev;
				if (lastLink.get_ptr() == sentinel.get_ptr())
					break;

				rcptr<volatile link_t> prev;
				rcptr<volatile link_t> prev_prev;
				bool outerContinue = false;
				for (;;)
				{
					if (!lastLink->begin_read_and_complete(0, rt_last, false))
					{
						outerContinue = true;
						break;
					}
					prev = rt_last->m_prev;
					bool outerContinue2 = false;
					for (;;)
					{
						if (!prev->begin_read_and_complete(0, rt_prev, false))
						{
							outerContinue2 = true;
							break;
						}
						prev_prev = rt_prev->m_prev;
						break;
					}
					if (outerContinue2)
						continue;
					break;	// and !outerContinue
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

				wasLast = wt_prev->m_next.get_ptr() == prev_prev.get_ptr();

				wt_prev->m_mode = link_mode::removing_next;
				if (!prev->m_links[0].end_write(wt_prev))
					continue;

				prev->begin_read_and_complete(0, rt_prev, false);	// completes remove-next
				if (!lastLink->remove())
					continue;
				dec_count();
				itor.m_link = lastLink;
				break;
			}
		}
		return itor;
	}

	iterator find_any_equal(const key_t& criteria) const	
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = next;
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							i.m_link = next;
							break;
						}
					}
				}
				if (!level)
					break;
				lastAdjacent = next;
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
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(next->get_payload()->get_key(), criteria))
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next->get_payload()->get_key()))// if next and arg are equal
						i.m_link = next;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_next;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if prev is not sentinel
					{
						const key_t& prevCriteria = prev->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance)
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria))	// if prev and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual)
						i.m_link = next;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual)
						i.m_link = prev;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_last_equal(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if prev is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev.template const_cast_to<link_t>()->get_payload()->get_key()))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if ((prev.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(prev.template const_cast_to<link_t>()->get_payload()->get_key(), criteria))	// if next and arg are equal
						i.m_link = prev;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(next->get_payload()->get_key(), criteria))
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& prevCriteria = prev->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria))	// if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual && (prev.get_ptr() != sentinelPtr))
						i.m_link = prev;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual && (next.get_ptr() != sentinelPtr))
						i.m_link = next;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}				
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if prev is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev->get_payload()->get_key()))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = next;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = next;
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))
						{
							i.m_link = next;
							break;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = next;
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
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						if (comparator_t::is_less_than(nextCriteria, criteria))
						{
							prev = next;
							continue;
						}
						if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
						{
							i.m_link = next;
							break;
						}
					}
				}
				if (!level)
					break;
				lastAdjacent = next;
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
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(next->get_payload()->get_key(), criteria))
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if ((next.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(criteria, next->get_payload()->get_key()))
						i.m_link = next;						
					else if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					nextWasEqual = false;
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& prevCriteria = prev->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria))	// if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (nextWasEqual)
						i.m_link = next;
					else if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(next->get_payload()->get_key(), criteria))
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = next;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			bool nextWasEqual = false;
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					nextWasEqual = false;
					next = rt->m_prev;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& prevCriteria = prev->get_payload()->get_key();
						bool advance = false;
						if (!nextWasEqual)
							advance = comparator_t::is_less_than(criteria, prevCriteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(prevCriteria, criteria))	// if next and arg are equal
							{
								advance = true;
								nextWasEqual = true;
							}
						}
						if (advance)
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (next.get_ptr() != sentinelPtr)
						i.m_link = next;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i; 
	}
	
	volatile_iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_prev;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev->get_payload()->get_key()))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prev.get_ptr() != sentinelPtr)
						i.m_link = prev;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

	iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		iterator i;
		link_t* sentinelPtr = m_sentinel.get_ptr();
		if (!!sentinelPtr)
		{
			height_t level = m_heightAndCount.m_currentHeight;
			rcptr<link_t> next;
			rcptr<link_t> prev = sentinelPtr;
			rcptr<link_t> lastAdjacent;
			bool prevWasEqual = false;
			for (;;)
			{
				next = prev->m_links[level]->m_next;
				if (next != lastAdjacent)
				{
					lastAdjacent.release();
					if (next.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						const key_t& nextCriteria = next->get_payload()->get_key();
						bool advance = false;
						if (!prevWasEqual)
							advance = comparator_t::is_less_than(nextCriteria, criteria);
						if (!advance)	// Will skip this block if next is less than arg.
						{
							if (!comparator_t::is_less_than(criteria, nextCriteria))	// if next and arg are equal
							{
								advance = true;
								prevWasEqual = true;
							}
						}
						if (advance)
						{
							prev = next;
							continue;
						}
					}
				}
				if (!level)
				{
					if (prevWasEqual)
						i.m_link = prev;
					else if (next.get_ptr() != sentinelPtr)
						i.m_link = next;
					break;
				}
				lastAdjacent = next;
				--level;
				continue;
			}
		}
		return i;
	}

	volatile_iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const volatile
	{
		volatile_iterator i;
		volatile link_t* sentinelPtr = m_sentinel.get_ptr();
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
			atomic::load(m_heightAndCount.m_currentHeight, level);
			for (;;)
			{
				if (!next->begin_read_and_complete(level, rt, true))
				{
					next = rt->m_next;
					continue;
				}
				prev = rt->m_prev;
				if (!prev->begin_read_and_complete(level, rt_prev, false))
					continue;
				if (rt_prev->m_next.get_ptr() != next.get_ptr())
				{
					ptr<volatile link_t> prevNextPtr = rt_prev->m_next.get_ptr();
					prevNextPtr->begin_read_and_complete(level, rt_prev_next, false);
					COGS_ASSERT(!next->m_links[level].is_current(rt));
					continue;
				}
				if (prev != lastAdjacent)
				{
					lastAdjacent.release();
					if (prev.get_ptr() != sentinelPtr)	// if next is not sentinel
					{
						if (comparator_t::is_less_than(criteria, prev->get_payload()->get_key()))
						{
							next = prev;
							continue;
						}
					}
				}
				if (!level)
				{
					if ((prev.get_ptr() != sentinelPtr) && !comparator_t::is_less_than(prev->get_payload()->get_key(), criteria))	// if prev and arg are equal
						i.m_link = prev;
					else if (next.get_ptr() != sentinelPtr)
						i.m_link = next;
					break;
				}
				lastAdjacent = prev;
				--level;
				continue;
			}
		}
		return i;
	}

};


#pragma warning(pop)


}


#endif
