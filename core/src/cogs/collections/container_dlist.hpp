//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_DLIST
#define COGS_HEADER_COLLECTION_CONTAINER_DLIST

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/collections/container_deque.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


/// @ingroup LockFreeCollections
/// @brief A double-link list container collection
/// @tparam T Type to contain.
template <typename T, class allocator_type = default_allocator>
class container_dlist
{
public:
	typedef T type;

private:
	allocator_container<allocator_type> m_allocator;

	enum class link_mode
	{
		normal = 0,
		inserting = 1,
		removing = 2,
		removing_next = 3
	};

	typedef container_dlist<type, allocator_type> this_t;

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

		typedef typename transactable<links_t>::read_token read_token;
		typedef typename transactable<links_t>::write_token write_token;

	private:
		link_t(link_t&) = delete;
		link_t& operator=(const link_t&) = delete;

	public:
		transactable<links_t> m_links;

		virtual type* get()
		{
			return NULL;
		}

		link_t()
		{
			m_links->m_mode = link_mode::normal;
		}

		~link_t()
		{ }

		virtual bool is_sentinel() const			{ return false; }
		virtual bool is_sentinel() const volatile	{ return false; }

		bool is_removed()
		{
			return m_links->m_mode == link_mode::removing;
		}

		bool remove()
		{
			if (m_links->m_mode == link_mode::removing)
				return false;

			COGS_ASSERT(!is_sentinel());

			m_links->m_mode = link_mode::removing;
			m_links->m_next->m_links->m_prev = m_links->m_prev;
			m_links->m_prev->m_links->m_next = m_links->m_next;
			return true;
		}

		bool insert_after(const rcptr<link_t>& afterThis)
		{
			if (afterThis->m_links->m_mode == link_mode::removing)
				return false;

			m_links->m_prev = afterThis;
			m_links->m_next = afterThis->m_links->m_next;
			afterThis->m_links->m_next = m_links->m_next->m_links->m_prev = this_rcref;
			return true;
		}

		bool insert_before(const rcptr<link_t>& beforeThis)
		{
			if (beforeThis->m_links->m_mode == link_mode::removing)
				return false;

			m_links->m_next = beforeThis;
			m_links->m_prev = beforeThis->m_links->m_prev;
			beforeThis->m_links->m_prev = m_links->m_prev->m_links->m_next = this_rcref;
			return true;
		}

		bool remove() volatile
		{
			bool result = false;
			for (;;)
			{
				read_token rt;
				if (!begin_read_and_complete(rt, false))
					break;

				rcptr<volatile link_t> prev = rt->m_prev;
				read_token rt_prev;
				if (!prev->begin_read_and_complete(rt_prev, false))
					continue;
				if (!m_links.is_current(rt))
					continue;

				if (rt_prev->m_next.get_ptr() != this)
				{
					// Could be here because prev->m_next contains a link being inserted.
					// If assisted to complete, m_prev will get updated.

					rcptr<volatile link_t> prevNext = rt_prev->m_next;
					prevNext->complete();

					COGS_ASSERT(!m_links.is_current(rt));
					continue;
				}

				write_token wt_prev;
				if (!prev->m_links.promote_read_token(rt_prev, wt_prev))
					continue;
				wt_prev->m_mode = link_mode::removing_next;
				if (!!prev->m_links.end_write(wt_prev))
				{
					result = true;
					prev->m_links.begin_read(rt_prev);
					if (rt_prev->m_mode == link_mode::removing_next)	// If not spontaneously completed by another thread.
						prev->complete_remove_next(rt_prev);
					break;
				}
				//continue;
			}
			return result;
		}

		// insertMode != insert_mode::normal implies afterThis is the sentinel
		bool insert_after(const rcptr<volatile link_t>& afterThis, insert_mode insertMode = insert_mode::normal)
		{
			bool result = false;
			m_links->m_mode = link_mode::inserting;
			m_links->m_prev = afterThis.template const_cast_to<link_t>();
			for (;;)
			{
				read_token rt_afterThis;
				if (!afterThis->begin_read_and_complete(rt_afterThis, false))
					break;

				rcptr<volatile link_t> afterThisNext = rt_afterThis->m_next;
				read_token rt_afterThisNext;
				if (!afterThisNext->begin_read_and_complete(rt_afterThisNext, false))
					continue;

				if ((insertMode != insert_mode::normal) && (afterThisNext->is_sentinel() == (insertMode == insert_mode::only_if_not_empty)))
					break;

				m_links->m_next = afterThisNext.template const_cast_to<link_t>();
				write_token wt_afterThis;
				if (!afterThis->m_links.promote_read_token(rt_afterThis, wt_afterThis))
					continue;
				wt_afterThis->m_next = this_rcptr;
				if (!afterThis->m_links.end_write(wt_afterThis))
					continue;

				complete();
				result = true;
				break;
			}
			return result;
		}

		// insertMode != insert_mode::normal implies beforeThis is the sentinel
		bool insert_before(const rcptr<volatile link_t>& beforeThis, insert_mode insertMode = insert_mode::normal)
		{
			bool result = false;
			m_links->m_mode = link_mode::inserting;
			m_links->m_next = beforeThis.template const_cast_to<link_t>();
			for (;;)
			{
				read_token rt_beforeThis;
				if (!beforeThis->begin_read_and_complete(rt_beforeThis, false))
					break;

				rcptr<volatile link_t> beforeThisPrev = rt_beforeThis->m_prev;
				read_token rt_beforeThisPrev;
				if (!beforeThisPrev->begin_read_and_complete(rt_beforeThisPrev, false))
					continue;

				rcptr<volatile link_t> beforeThisPrevNext = rt_beforeThisPrev->m_next;
				if (beforeThisPrevNext != beforeThis)
				{
					read_token rt_beforeThisPrevNext;
					beforeThisPrevNext->begin_read_and_complete(rt_beforeThisPrevNext, false);
					COGS_ASSERT(!beforeThis->m_links.is_current(rt_beforeThis));
					continue;
				}
				
				if (!beforeThis->m_links.is_current(rt_beforeThis))
					continue;
				
				COGS_ASSERT(rt_beforeThisPrev->m_next == beforeThis);

				if ((insertMode != insert_mode::normal) && (beforeThisPrev->is_sentinel() == (insertMode == insert_mode::only_if_not_empty)))
					break;
				
				m_links->m_prev = beforeThisPrev.template const_cast_to<link_t>();
				write_token wt_beforeThisPrev;
				if (!beforeThisPrev->m_links.promote_read_token(rt_beforeThisPrev, wt_beforeThisPrev))
					continue;
				wt_beforeThisPrev->m_next = this_rcptr;
				if (!beforeThisPrev->m_links.end_write(wt_beforeThisPrev))
					continue;

				complete();
				result = true;
				break;
			}
			return result;
		}

		void complete_insert(read_token& rt) volatile
		{
			COGS_ASSERT(rt->m_mode == link_mode::inserting);

			// rt->m_inserting is known to be set.  We know the next time this link is written to, it's
			// going to be to remove the inserting flag, so we don't need a retry loop.
			rcptr<volatile link_t> next = rt->m_next;
			for (;;)
			{
				read_token rt_next;
				next->m_links.begin_read(rt_next);
				rcptr<volatile link_t> nextPrev = rt_next->m_prev;
				if (!m_links.is_current(rt))
					break;

				COGS_ASSERT((rt_next->m_mode == link_mode::normal) || (rt_next->m_mode == link_mode::removing_next));
					
				if (nextPrev.get_ptr() == this)
					break;

				write_token wt_next;
				if (!next->m_links.promote_read_token(rt_next, wt_next))
					continue;

				wt_next->m_prev = this_rcptr.template const_cast_to<link_t>();
				if (!next->m_links.end_write(wt_next))
					continue;
				break;
			}

			// No loop needed, because the next write will clear insert mode
			write_token wt;
			if (!!m_links.promote_read_token(rt, wt))
			{
				wt->m_mode = link_mode::normal;
				m_links.end_write(wt);
			}
		}

		void complete_remove_next(read_token& rt) volatile
		{
			rcptr<volatile link_t> next = rt->m_next;
			for (;;)
			{
				read_token rt_next;
				next->m_links.begin_read(rt_next);
				if (rt_next->m_mode == link_mode::removing)
				{
					next->complete_remove(rt_next);
					break;
				}
				if (rt_next->m_mode == link_mode::removing_next)
				{
					next->complete_remove_next(rt_next);
					continue;
				}
				COGS_ASSERT(rt_next->m_mode == link_mode::normal);	// Wouldn't be inserting, no need to check link_mode::inserting
				write_token wt_next;
				if (!next->m_links.promote_read_token(rt_next, wt_next))
					continue;
				wt_next->m_mode = link_mode::removing;
				next->m_links.end_write(wt_next);
				//continue;
			}
		}

		void complete_remove(read_token& rt) volatile
		{
			rcptr<volatile link_t> next = rt->m_next;
			for (;;)
			{
				read_token rt_next;
				next->m_links.begin_read(rt_next);
				if (rt_next->m_mode == link_mode::inserting)
				{
					next->complete_insert(rt_next);
					next->m_links.begin_read(rt_next);
				}
				if (rt_next->m_prev.get_ptr() == this)
				{
					write_token wt_next;
					if (!next->m_links.promote_read_token(rt_next, wt_next))
						continue;

					wt_next->m_prev = rt->m_prev;
					if (!next->m_links.end_write(wt_next))
						continue;
				}
				break;
			}

			for (;;)
			{
				rcptr<volatile link_t> prev = rt->m_prev;
				read_token rt_prev;
				prev->m_links.begin_read(rt_prev);
				if (rt_prev->m_next.get_ptr() == this)	// otherwise, we're done here.
				{
					COGS_ASSERT(rt_prev->m_mode == link_mode::removing_next);
					write_token wt_prev;
					if (!prev->m_links.promote_read_token(rt_prev, wt_prev))
						continue;
					wt_prev->m_next = rt->m_next;
					wt_prev->m_mode = link_mode::normal;
					if (!prev->m_links.end_write(wt_prev))
						continue;
				}
				break;
			}
		}

		bool begin_read_and_complete(read_token& rt, bool returnTokenEvenIfRemoved) volatile	// returns false this link was removed.
		{
			bool notRemoved = true;
			rt.release();
			for (;;)
			{
				m_links.begin_read(rt);
				if (rt->m_mode == link_mode::normal)
					break;

				if (rt->m_mode == link_mode::inserting)
				{
					complete_insert(rt);
					continue;
				}

				if (rt->m_mode == link_mode::removing_next)
				{
					complete_remove_next(rt);
					continue;
				}

				if (rt->m_mode == link_mode::removing)
				{
					notRemoved = false;
					complete_remove(rt);
					if (returnTokenEvenIfRemoved)
						m_links.begin_read(rt);
				}

				break;
			}
			return notRemoved;
		}

		bool is_removed() volatile	// returns false this link was removed.
		{
			read_token rt;
			return !begin_read_and_complete(rt, false);
		}

		void complete() volatile
		{
			read_token rt;
			begin_read_and_complete(rt, false);
		}
	};

	typedef typename link_t::read_token read_token;
	typedef typename link_t::write_token write_token;

	class payload_link_t : public link_t
	{
	private:
		typename placement<type> m_value;

	public:
		virtual type* get()						{ return &m_value.get(); }

		payload_link_t()						{ new (get()) type(); }
		explicit payload_link_t(const type& t)	{ new (get()) type(t); }
		~payload_link_t()						{ m_value.destruct(); }

		rcref<type>	get_obj()	{ return get_self_rcref(get()); }
	};

	template <typename T2>
	class aux_payload_link_t : public payload_link_t
	{
	public:
		typedef typename std::remove_cv<T2>::type T3;

		delayed_construction<T3> m_aux;

		explicit aux_payload_link_t()
		{
			placement_rcnew(this_desc, &m_aux.get());
		}

		explicit aux_payload_link_t(const type& t)
			: payload_link_t(t)
		{
			placement_rcnew(this_desc, &m_aux.get());
		}

		const rcref<T2>& get_aux_ref(unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T> >().get_unowned())
		{
			storage.set(&m_aux.get(), this_desc);
			return storage.dereference();
		}
	};

	class sentinel_link_t : public link_t
	{
	public:
		using link_t::m_links;
		
		sentinel_link_t()							{ m_links->m_next = m_links->m_prev = this_rcptr; }

		virtual bool is_sentinel() const			{ return true; }
		virtual bool is_sentinel() const volatile	{ return true; }
	};

	rcptr<link_t> m_sentinel;

	bool pop_first_inner(type* t, bool& wasLast) volatile
	{
		bool result = false;
		rcptr<volatile link_t> sentinel = m_sentinel;
		read_token rt;
		for (;;)
		{
			wasLast = false;
			sentinel->begin_read_and_complete(rt, false);	// sentinel won't get deleted, no need to check result
			rcptr<volatile link_t> firstLink = rt->m_next;
			if (firstLink.get_ptr() == sentinel.get_ptr())
				return false;
			if (firstLink->is_removed())	// also completes other states
				continue;

			write_token wt;
			if (!sentinel->m_links.promote_read_token(rt, wt))
				continue;

			COGS_ASSERT(wt->m_mode == link_mode::normal);
			COGS_ASSERT(wt->m_next == firstLink);

			wasLast = wt->m_next.get_ptr() == wt->m_prev.get_ptr();

			wt->m_mode = link_mode::removing_next;
			if (!sentinel->m_links.end_write(wt))
				continue;

			sentinel->begin_read_and_complete(rt, false);	// completes remove-next
			*t = *(firstLink.template const_cast_to<link_t>()->get());
			result = true;
			break;
		}
		return result;
	}

	bool pop_last_inner(type* t, bool& wasLast) volatile
	{
		bool result = false;
		rcptr<volatile link_t> sentinel = m_sentinel;
		read_token rt_sentinel;
		read_token rt_last;
		read_token rt_prev;
		for (;;)
		{
			wasLast = false;
			sentinel->begin_read_and_complete(rt_sentinel, false);	// sentinel won't get deleted, no need to check result
			rcptr<volatile link_t> lastLink = rt_sentinel->m_prev;
			if (lastLink.get_ptr() == sentinel.get_ptr())
				break;
			if (lastLink->is_removed())	// also completes other states
				continue;
			if (!lastLink->begin_read_and_complete(rt_last, false))
				continue;
			rcptr<volatile link_t> prev = rt_last->m_prev;
			if (!prev->begin_read_and_complete(rt_prev, false))
				continue;
			if (!lastLink->m_links.is_current(rt_last))
				continue;
			if (!sentinel->m_links.is_current(rt_sentinel))
				continue;

			write_token wt_prev;
			if (!prev->m_links.promote_read_token(rt_prev, wt_prev))
				continue;

			COGS_ASSERT(wt_prev->m_mode == link_mode::normal);
			COGS_ASSERT(wt_prev->m_next == lastLink);

			wasLast = wt_prev->m_next.get_ptr() == wt_prev->m_prev.get_ptr();

			wt_prev->m_mode = link_mode::removing_next;
			if (!prev->m_links.end_write(wt_prev))
				continue;

			prev->begin_read_and_complete(rt_prev, false);	// completes remove-next
			if (!!t)
				*t = *(lastLink.template const_cast_to<link_t>()->get());
			result = true;
			break;
		}
		return result;
	}

	void clear_inner()
	{
		// Could allow the links to free themselves, but that would cause a cascade of 
		// link releases, that could potential overflow the stack if the list is long enough.

		rcptr<link_t> l = m_sentinel->m_links->m_next;
		while (l != m_sentinel)
		{
			rcptr<link_t> next = l->m_links->m_next;
			l->m_links->m_next = 0;
			l->m_links->m_prev = 0;
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
		}
		return sentinel;
	}

	container_dlist(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;
	class preallocated_t;

	/// @brief A container_dlist element iterator
	class iterator
	{
	protected:
		rcptr<link_t> m_link;

		friend class container_dlist;
		friend class remove_token;
		friend class preallocated_t;

		iterator(const rcptr<link_t>& l) : m_link(l)	{ }

	public:
		void disown()	{ m_link.disown(); }

		iterator()												{ }
		iterator(const iterator& i) : m_link(i.m_link)	{ }
		iterator(const remove_token& rt) : m_link(rt.m_link)	{ if (is_removed()) release(); }

		void release()				{ m_link.release(); }

		bool is_active() const		{ return !!m_link && !m_link->is_removed(); }
		bool is_removed() const		{ return !!m_link && m_link->is_removed(); }	// implies that it was in the list.  null m_link returns false

		iterator& operator++()
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
			return *this;
		}

		iterator& operator--()
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
			return *this;
		}

		iterator operator++(int)			{ iterator i(*this); ++*this; return i; }
		iterator operator--(int)			{ iterator i(*this); --*this; return i; }

		bool operator!() const								{ return !m_link; }

		bool operator==(const iterator& i) const			{ return m_link == i.m_link; }
		bool operator==(const remove_token& i) const		{ return m_link == i.m_link; }

		bool operator!=(const iterator& i) const			{ return !operator==(i); }
		bool operator!=(const remove_token& i) const		{ return !operator==(i); }

		iterator& operator=(const iterator& i)				{ m_link = i.m_link; return *this; }
		iterator& operator=(const remove_token& i)			{ m_link = i.m_link; if (is_removed()) release(); return *this; }

		type* get() const								{ return (!m_link) ? (type*)0 : m_link->get(); }
		type& operator*() const							{ return *(m_link->get()); }
		type* operator->() const						{ return m_link->get(); }

		rcptr<type> get_obj() const
		{
			rcptr<type> result;
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
				rcptr<link_t>* lnk = &(m_link->m_links->m_next);
				while (!(*lnk)->is_sentinel())
				{	
					if ((*lnk)->m_links->m_mode != link_mode::removing)
					{
						result.m_link = *lnk;
						break;
					}
					lnk = &((*lnk)->m_links->m_next);
				}
			}
			return result;
		}

		iterator prev() const
		{
			iterator result;
			if (!!m_link)
			{
				rcptr<link_t>* lnk = &(m_link->m_links->m_prev);
				while (!(*lnk)->is_sentinel())
				{
					if ((*lnk)->m_links->m_mode != link_mode::removing)
					{
						result.m_link = *lnk;
						break;
					}
					lnk = &((*lnk)->m_links->m_prev);
				}
			}
			return result;
		}
	};

	/// @brief A volatile container_dlist element iterator
	class volatile_iterator
	{
	protected:
		rcptr<volatile link_t> m_link;

		friend class container_dlist;
		friend class volatile_remove_token;

		volatile_iterator(const rcptr<volatile link_t>& l) : m_link(l)	{ }

	public:
		void disown()	{ m_link.disown(); }

		volatile_iterator()																	{ }
		volatile_iterator(const volatile_iterator& i) : m_link(i.m_link)					{ }
		volatile_iterator(const volatile_remove_token& rt) : m_link(rt.m_link)				{ if (is_removed()) release(); }
		volatile_iterator(const volatile volatile_iterator& i) : m_link(i.m_link)			{ }
		volatile_iterator(const volatile volatile_remove_token& rt) : m_link(rt.m_link)		{ if (is_removed()) release(); }

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
		bool is_active() const volatile		{ rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		bool is_removed() const				{ return !!m_link && m_link->is_removed(); }
		bool is_removed() const volatile	{ rcptr<volatile link_t> lnk(m_link); return !!lnk && lnk->is_removed(); }

		volatile_iterator& operator++()
		{
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true);
				rcptr<volatile link_t> nextLink;
				for (;;)
				{
					nextLink = rt->m_next;
					if (nextLink->is_sentinel())
					{
						m_link.release();
						break;
					}
					if (!nextLink->begin_read_and_complete(rt, true))
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
			if (!!oldLink)
			{
				read_token rt;
				for (;;)
				{
					oldLink->begin_read_and_complete(rt, true);
					rcptr<volatile link_t> newLink = rt->m_next;
					for (;;)
					{
						if (newLink->is_sentinel())
							newLink.release();
						else if (!newLink->begin_read_and_complete(rt, true))
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
		}

		volatile_iterator& operator--()
		{
			if (!!m_link)
			{
				read_token rt;
				m_link->begin_read_and_complete(rt, true);
				rcptr<volatile link_t> prevLink;
				for (;;)
				{
					prevLink = rt->m_prev;
					if (prevLink->is_sentinel())
					{
						m_link.release();
						break;
					}
					if (!prevLink->begin_read_and_complete(rt, true))
						continue;
					m_link = prevLink;
					break;
				}
			}
			return *this;
		}

		void operator--() volatile
		{
			rcptr<volatile link_t> oldLink = m_link;
			if (!!oldLink)
			{
				read_token rt;
				for (;;)
				{
					oldLink->begin_read_and_complete(rt, true);
					rcptr<volatile link_t> newLink = rt->m_prev;
					for (;;)
					{
						if (newLink->is_sentinel())
							newLink.release();
						else if (!newLink->begin_read_and_complete(rt, true))
						{
							newLink = rt->m_prev;
							continue;
						}
						break;
					}
					if (!m_link.compare_exchange(newLink, oldLink, oldLink))
						continue;
					break;
				}
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
		bool operator==(const volatile_remove_token& i) const			{ return m_link == i.m_link; }
		bool operator==(const volatile volatile_remove_token& i) const	{ return m_link == i.m_link; }
		bool operator==(const volatile_remove_token& i) const volatile	{ return m_link == i.m_link; }

		bool operator!=(const volatile_iterator& i) const				{ return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const		{ return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile		{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& i) const			{ return !operator==(i); }
		bool operator!=(const volatile volatile_remove_token& i) const	{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& i) const volatile	{ return !operator==(i); }

		type* get() const												{ return (!m_link) ? (type*)0 : m_link.template const_cast_to<link_t>()->get(); }
		type& operator*() const											{ return *(get()); }
		type* operator->() const										{ return get(); }

		void release()													{ m_link.release(); }
		void release() volatile											{ m_link.release(); }

		rcptr<type>	get_obj() const
		{
			rcptr<type> result;
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

	/// @brief A preallocated container_dlist element
	class preallocated_t
	{
	protected:
		rcptr<link_t> m_link;

		friend class container_dlist;

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
		preallocated_t& operator=(const preallocated_t& i)		{ m_link = i.m_link; return *this; }

		type* get() const								{ return (!m_link) ? (type*)0 : m_link->get(); }
		type& operator*() const							{ return *(m_link->get()); }
		type* operator->() const						{ return m_link->get(); }

		rcptr<type> get_obj() const
		{
			rcptr<type> result;
			if (!!m_link)
				result = m_link.template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}
		
		rc_obj_base* get_desc() const		{ return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_link.get_desc(); }
	};

	/// @brief A container_dlist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		weak_rcptr<link_t> m_link;

		friend class container_dlist;
	public:
		remove_token()												{ }
		remove_token(const preallocated_t& i)	: m_link(i.m_link)	{ }
		remove_token(const iterator& i)			: m_link(i.m_link)	{ }
		remove_token(const remove_token& rt)	: m_link(rt.m_link)	{ }

		remove_token& operator=(const preallocated_t& i)	{ m_link = i.m_link; return *this; }
		remove_token& operator=(const iterator& i)			{ m_link = i.m_link; return *this; }
		remove_token& operator=(const remove_token& i)		{ m_link = i.m_link; return *this; }

		bool is_active() const							{ rcptr<link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		void release()									{ m_link.release(); }

		bool operator!() const							{ return !m_link; }

		bool operator==(const iterator& i) const		{ return m_link == i.m_link; }
		bool operator==(const remove_token& i) const	{ return m_link == i.m_link; }

		bool operator!=(const iterator& i) const		{ return !operator==(i); }
		bool operator!=(const remove_token& i) const	{ return !operator==(i); }
	};

	/// @brief A volatile container_dlist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class volatile_remove_token
	{
	protected:
		weak_rcptr<volatile link_t> m_link;

		friend class container_dlist;
	public:
		volatile_remove_token()																{ }
		volatile_remove_token(const          preallocated_t& i) : m_link(i.m_link)			{ }
		volatile_remove_token(const          volatile_iterator& i) : m_link(i.m_link)		{ }
		volatile_remove_token(const volatile volatile_iterator& i) : m_link(i.m_link)		{ }
		volatile_remove_token(const          volatile_remove_token& rt) : m_link(rt.m_link)	{ }
		volatile_remove_token(const volatile volatile_remove_token& rt) : m_link(rt.m_link)	{ }

		volatile_remove_token& operator=(const preallocated_t& i)					{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile_iterator& i)				{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile volatile_iterator& i)		{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile_remove_token& i)			{ m_link = i.m_link; return *this; }
		volatile_remove_token& operator=(const volatile volatile_remove_token& i)	{ m_link = i.m_link; return *this; }

		void operator=(const volatile_iterator& i) volatile							{ m_link = i.m_link; }
		void operator=(const volatile_remove_token& i) volatile						{ m_link = i.m_link; }

		bool is_active() const				{ rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }
		bool is_active() const volatile		{ rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		void release()						{ m_link.release(); }
		void release() volatile				{ m_link.release(); }

		bool operator!() const				{ return !m_link; }
		bool operator!() const volatile		{ return !m_link; }

		bool operator==(const volatile_iterator& i) const				{ return m_link == i.m_link; }
		bool operator==(const volatile volatile_iterator& i) const		{ return m_link == i.m_link; }
		bool operator==(const volatile_iterator& i) const volatile		{ return m_link == i.m_link; }
		bool operator==(const volatile_remove_token& i) const			{ return m_link == i.m_link; }
		bool operator==(const volatile volatile_remove_token& i) const	{ return m_link == i.m_link; }
		bool operator==(const volatile_remove_token& i) const volatile	{ return m_link == i.m_link; }

		bool operator!=(const volatile_iterator& i) const				{ return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const		{ return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile		{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& i) const			{ return !operator==(i); }
		bool operator!=(const volatile volatile_remove_token& i) const	{ return !operator==(i); }
		bool operator!=(const volatile_remove_token& i) const volatile	{ return !operator==(i); }
	};

	container_dlist()
		: m_sentinel(rcnew(sentinel_link_t))
	{
	}

	container_dlist(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_sentinel(std::move(m_sentinel))
	{ }

	this_t& operator=(this_t&& src)
	{
		if (!!m_sentinel)
			clear_inner();
		m_allocator = std::move(src.m_allocator);
		m_sentinel = std::move(src.m_sentinel);
		return *this;
	}


	container_dlist(volatile allocator_type& al)
		: m_sentinel(instance_rcnew(al, sentinel_link_t))
	{
	}

	~container_dlist()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->m_links->m_next.release();
			m_sentinel->m_links->m_prev.release();
			COGS_ASSERT(m_sentinel.get_desc()->is_owned());	// Should only be referenced by container_dlist itself
		}
	}

	void clear()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->m_links->m_next = m_sentinel;
			m_sentinel->m_links->m_prev = m_sentinel;
			COGS_ASSERT(m_sentinel.get_desc()->get_strong_count() == 3);	// Should only be referenced by container_dlist itself
		}
	}

	void drain() volatile
	{
		while (!!remove_last())
			;
	}

	bool is_empty() const			{ return !m_sentinel || ((m_sentinel->m_links->m_next.get_ptr() == m_sentinel.get_ptr())); }
	bool is_empty() const volatile
	{
		volatile link_t* sentinel = m_sentinel.get_ptr();
		return !sentinel || (sentinel->m_links.begin_read()->m_next.get_ptr() == sentinel);
	}

	bool operator!() const			{ return is_empty(); }
	bool operator!() const volatile	{ return is_empty(); }

	iterator get_first() const
	{
		iterator i;
		if (!!m_sentinel && (m_sentinel->m_links->m_next.get_ptr() != m_sentinel.get_ptr()))
			i.m_link = m_sentinel->m_links->m_next;
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
				lnk = sentinel->m_links.begin_read()->m_next;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (lnk->is_removed())
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
		if (!!m_sentinel && (m_sentinel->m_links->m_prev.get_ptr() != m_sentinel.get_ptr()))
			i.m_link = m_sentinel->m_links->m_prev;
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
				lnk = sentinel->m_links.begin_read()->m_prev;
				if (lnk.get_ptr() == sentinel.get_ptr())
					break;
				if (lnk->is_removed())
					continue;
				i.m_link = lnk;
				break;
			}
		}
		return i;
	}

	preallocated_t preallocate() const volatile					{ preallocated_t i; i.m_link = container_rcnew(m_allocator, payload_link_t); return i; }
	preallocated_t preallocate(const type& t) const volatile	{ preallocated_t i; i.m_link = container_rcnew(m_allocator, payload_link_t, t); return i; }

	template <typename T2>
	const rcref<T2>& preallocate_with_aux(preallocated_t& i, unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T2> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T2> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	template <typename T2>
	const rcref<T2>& preallocate_with_aux(const type& t, preallocated_t& i, unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T2> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T2> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t, t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	iterator prepend_preallocated(const preallocated_t& i, insert_mode insertMode = insert_mode::normal) const volatile
	{
		iterator result;
		if (!!i.m_link->insert_after(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	iterator append_preallocated(const preallocated_t& i, insert_mode insertMode = insert_mode::normal) const volatile
	{
		iterator result;
		if (!!i.m_link->insert_before(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator prepend_preallocated(const preallocated_t& i, insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator result;
		if (!!i.m_link->insert_after(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator append_preallocated(const preallocated_t& i, insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator result;
		if (!i.m_link->insert_before(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator prepend(insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t);
		if (!i.m_link.template const_cast_to<link_t>()->insert_after(lazy_init_sentinel(), insertMode))
			i.release();
		return i;
	}

	volatile_iterator append(insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t);
		i.m_link.template const_cast_to<link_t>()->insert_before(lazy_init_sentinel(), insertMode);
		return i;
	}

	volatile_iterator prepend(const type& t, insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t);
		if (!i.m_link.template const_cast_to<link_t>()->insert_after(lazy_init_sentinel(), insertMode))
			i.release();
		return i;
	}

	volatile_iterator append(const type& t, insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t);
		if (!i.m_link.template const_cast_to<link_t>()->insert_before(lazy_init_sentinel(), insertMode))
			i.release();
		return i;
	}

	iterator prepend()
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t);
		if (!i.m_link->insert_after(lazy_init_sentinel()))
			i.release();
		return i;
	}

	iterator append()
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t);
		if (!i.m_link->insert_before(lazy_init_sentinel()))
			i.release();
		return i;
	}

	iterator prepend(const type& t)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t);
		if (!i.m_link->insert_after(lazy_init_sentinel()))
			i.release();
		return i;
	}

	iterator append(const type& t)
	{
		iterator i;
		i.m_link = container_rcnew(m_allocator, payload_link_t, t);
		if (!i.m_link->insert_before(lazy_init_sentinel()))
			i.release();
		return i;
	}

	// insert_after/insert_before return an empty iterator if insertAfter/insertBefore node was already removed

	iterator insert_preallocated_before(const preallocated_t& i, const iterator& insertBefore)
	{ 
		iterator result;
		if (!!insertBefore && i.m_link->insert_before(insertBefore.m_link))
			result.m_link = i.m_link;
		return result;
	}

	iterator insert_preallocated_after(const preallocated_t& i, const iterator& insertAfter)
	{
		iterator result;
		if (!!insertAfter && i.m_link->insert_after(insertAfter.m_link))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator insert_preallocated_before(const preallocated_t& i, const volatile_iterator& insertBefore) volatile
	{
		volatile_iterator result;
		if (!!insertBefore && i.m_link->insert_before(insertBefore.m_link))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator insert_preallocated_after(const preallocated_t& i, const volatile_iterator& insertAfter) volatile
	{
		volatile_iterator result;
		if (!!insertAfter && i.m_link->insert_after(insertAfter.m_link))
			result.m_link = i.m_link;
		return result;
	}

	iterator insert_before(const iterator& insertBefore)
	{
		iterator i;
		if (!!insertBefore)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t);
			if (!i.m_link->insert_before(insertBefore.m_link))
				i.release();
		}
		return i;
	}

	iterator insert_after(const iterator& insertAfter)
	{
		iterator i;
		if (!!insertAfter)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t);
			if (!i.m_link->insert_after(insertAfter.m_link))
				i.release();
		}
		return i;
	}

	iterator insert_before(const type& t, const iterator& insertBefore)
	{ 
		iterator i;
		if (!!insertBefore)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t, t);
			if (!i.m_link->insert_before(insertBefore.m_link))
				i.release();
		}
		return i;
	}

	iterator insert_after(const type& t, const iterator& insertAfter)
	{ 
		iterator i;
		if (!!insertAfter)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t, t);
			if (!i.m_link->insert_after(insertAfter.m_link))
				i.release();
		}
		return i;
	}

	volatile_iterator insert_before(const volatile_iterator& insertBefore) volatile
	{ 
		volatile_iterator i;
		if (!!insertBefore)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t);
			if (!i.m_link.template const_cast_to<link_t>()->insert_before(insertBefore.m_link))
				i.release();
		}
		return i;
	}

	volatile_iterator insert_after(const volatile_iterator& insertAfter) volatile
	{ 
		volatile_iterator i;
		if (!!insertAfter)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t);
			if (!i.m_link.template const_cast_to<link_t>()->insert_after(insertAfter.m_link))
				i.release();
		}
		return i;
	}

	volatile_iterator insert_before(const type& t, const volatile_iterator& insertBefore) volatile
	{ 
		volatile_iterator i;
		if (!!insertBefore)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t, t);
			if (!i.m_link.template const_cast_to<link_t>()->insert_before(insertBefore.m_link))
				i.release();
		}
		return i;
	}

	volatile_iterator insert_after(const type& t, const volatile_iterator& insertAfter) volatile
	{ 
		volatile_iterator i;
		if (!!insertAfter)
		{
			i.m_link = container_rcnew(m_allocator, payload_link_t, t);
			if (!i.m_link.template const_cast_to<link_t>()->insert_after(insertAfter.m_link))
				i.release();
		}
		return i;
	}

	bool remove(const iterator& i)
	{
		return !!i.m_link && i.m_link->remove();
	}

	bool remove(const remove_token& rt)
	{
		iterator i(rt);
		return remove(i);
	}

	bool remove(const volatile_iterator& i) volatile
	{
		rcptr<volatile link_t> lnk = i.m_link;
		return !!lnk && lnk->remove();
	}

	bool remove(const volatile volatile_iterator& i) volatile
	{
		volatile_iterator i2(i);
		return remove(i2);
	}

	bool remove(const volatile_remove_token& rt) volatile
	{
		volatile_iterator i(rt);
		return remove(i);
	}

	bool peek_first(type& t)
	{
		iterator i = get_first();
		if (!i)
			return false;
		t = *i;
		return true;
	}

	bool peek_first(type& t) volatile
	{
		volatile_iterator i = get_first();
		if (!i)
			return false;
		t = *i;
		return true;
	}

	bool peek_last(type& t)
	{
		iterator i = get_last();
		if (!i)
			return false;
		t = *i;
		return true;
	}

	bool peek_last(type& t) volatile
	{
		volatile_iterator i = get_last();
		if (!i)
			return false;
		t = *i;
		return true;
	}

	bool pop_first(type& t, bool& wasLast)
	{
		iterator i = get_first();
		if (!i)
			return false;
		t = *i;
		remove(i);
		wasLast = is_empty();
		return true;
	}

	bool pop_first(type& t, bool& wasLast) volatile
	{
		return pop_first_inner(&t, wasLast);
	}

	bool pop_first(type& t)
	{
		iterator i = get_first();
		if (!i)
			return false;
		t = *i;
		remove(i);
		return true;
	}

	bool pop_first(type& t) volatile
	{
		bool wasLast;
		return pop_first(t, wasLast);
	}

	bool pop_last(type& t, bool& wasLast)
	{
		iterator i = get_last();
		if (!i)
			return false;
		t = *i;
		remove(i);
		if (is_empty())
			wasLast = true;
		return true;
	}

	bool pop_last(type& t, bool& wasLast) volatile
	{
		return pop_last_inner(&t, wasLast);
	}

	bool pop_last(type& t)
	{
		iterator i = get_last();
		if (!i)
			return false;
		t = *i;
		remove(i);
		return true;
	}

	bool pop_last(type& t) volatile
	{
		bool wasLast;
		return pop_last(t, wasLast);
	}

	bool remove_first(bool& wasLast)
	{
		iterator i = get_first();
		if (!i)
			return false;
		remove(i);
		wasLast = is_empty();
		return true;
	}

	bool remove_first(bool& wasLast) volatile
	{
		return pop_first_inner(NULL, wasLast);
	}

	bool remove_first()
	{
		iterator i = get_first();
		if (!i)
			return false;
		remove(i);
		return true;
	}

	bool remove_first() volatile
	{
		bool wasLast;
		return remove_first(wasLast);
	}

	bool remove_last(bool& wasLast)
	{
		iterator i = get_last();
		if (!i)
			return false;
		remove(i);
		wasLast = is_empty();
		return true;
	}

	bool remove_last(bool& wasLast) volatile
	{
		return pop_last_inner(NULL, wasLast);
	}

	bool remove_last()
	{
		iterator i = get_last();
		if (!i)
			return false;
		remove(i);
		return true;
	}

	bool remove_last() volatile
	{
		bool wasLast;
		return remove_last(wasLast);
	}
};


// Making these available so storage size can be used without knowing the key type yet, since the sizes will be the same regardless of key type
static constexpr size_t container_dlist_iterator_size = sizeof(container_dlist<int>::iterator);
static constexpr size_t container_dlist_volatile_iterator_size = sizeof(container_dlist<int>::volatile_iterator);
static constexpr size_t container_dlist_remove_token_size = sizeof(container_dlist<int>::remove_token);
static constexpr size_t container_dlist_volatile_remove_token_size = sizeof(container_dlist<int>::volatile_remove_token);
static constexpr size_t container_dlist_preallocated_t_size = sizeof(container_dlist<int>::preallocated_t);

static constexpr size_t container_dlist_iterator_alignment = std::alignment_of<typename container_dlist<int>::iterator>::value;
static constexpr size_t container_dlist_volatile_iterator_alignment = std::alignment_of<typename container_dlist<int>::volatile_iterator>::value;
static constexpr size_t container_dlist_remove_token_alignment = std::alignment_of<typename container_dlist<int>::remove_token>::value;
static constexpr size_t container_dlist_volatile_remove_token_alignment = std::alignment_of<typename container_dlist<int>::volatile_remove_token>::value;
static constexpr size_t container_dlist_preallocated_t_alignment = std::alignment_of<typename container_dlist<int>::preallocated_t>::value;



#pragma warning(pop)


}


#endif
