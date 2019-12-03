//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_DLIST
#define COGS_HEADER_COLLECTION_CONTAINER_DLIST

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/collections/container_deque.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified

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

		explicit link_t(rc_obj_base& desc)
			: object(desc)
		{
			m_links->m_mode = link_mode::normal;
		}

		~link_t()
		{ }

		virtual bool is_sentinel() const { return false; }
		virtual bool is_sentinel() const volatile { return false; }

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
					if (rt_prev->m_mode == link_mode::removing_next) // If not spontaneously completed by another thread.
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
				COGS_ASSERT(rt_next->m_mode == link_mode::normal); // Wouldn't be inserting, no need to check link_mode::inserting
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
				if (rt_prev->m_next.get_ptr() == this) // otherwise, we're done here.
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

		bool begin_read_and_complete(read_token& rt, bool returnTokenEvenIfRemoved) volatile // returns false this link was removed.
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

		bool is_removed() volatile // returns false this link was removed.
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
		placement<type> m_value;

	public:
		virtual type* get() { return &m_value.get(); }

		explicit payload_link_t(rc_obj_base& desc)
			: link_t(desc)
		{
			new (get()) type;
		}

		payload_link_t(rc_obj_base& desc, const type& t)
			: link_t(desc)
		{
			new (get()) type(t);
		}

		~payload_link_t()
		{
			m_value.destruct();
		}

		rcref<type> get_obj() { return get_self_rcref(get()); }
	};

	template <typename T2>
	class aux_payload_link_t : public payload_link_t
	{
	public:
		typedef std::remove_cv_t<T2> T3;

		delayed_construction<T3> m_aux;

		explicit aux_payload_link_t(rc_obj_base& desc)
			: payload_link_t(desc)
		{
			placement_rcnew(&m_aux.get(), this_desc);
		}

		explicit aux_payload_link_t(rc_obj_base& desc, const type& t)
			: payload_link_t(desc, t)
		{
			placement_rcnew(&m_aux.get(), this_desc);
		}

		const rcref<T2>& get_aux_ref(unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T> >().get_unowned())
		{
			storage.set(&m_aux.get(), &this_desc);
			return storage.dereference();
		}
	};

	class sentinel_link_t : public link_t
	{
	public:
		using link_t::m_links;

		explicit sentinel_link_t(rc_obj_base& desc)
			: link_t(desc)
		{
			m_links->m_next = m_links->m_prev = this_rcptr;
		}

		virtual bool is_sentinel() const { return true; }
		virtual bool is_sentinel() const volatile { return true; }
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
			sentinel->begin_read_and_complete(rt, false); // sentinel won't get deleted, no need to check result
			rcptr<volatile link_t> firstLink = rt->m_next;
			if (firstLink.get_ptr() == sentinel.get_ptr())
				return false;
			if (firstLink->is_removed()) // also completes other states
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

			sentinel->begin_read_and_complete(rt, false); // completes remove-next
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
			sentinel->begin_read_and_complete(rt_sentinel, false); // sentinel won't get deleted, no need to check result
			rcptr<volatile link_t> lastLink = rt_sentinel->m_prev;
			if (lastLink.get_ptr() == sentinel.get_ptr())
				break;
			if (lastLink->is_removed()) // also completes other states
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

			prev->begin_read_and_complete(rt_prev, false); // completes remove-next
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
	class preallocated;

	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = std::is_same_v<preallocated, std::remove_cv_t<T2> > || is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A container_dlist element iterator
	class iterator
	{
	private:
		rcptr<link_t> m_link;

		friend class container_dlist;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;
		friend class preallocated;

		iterator(const rcptr<link_t>& l) : m_link(l) { }
		iterator(const volatile rcptr<link_t>& l) : m_link(l) { }
		iterator(rcptr<link_t>&& l) : m_link(std::move(l)) { }

		iterator(const rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(const volatile rcptr<volatile link_t>& l) : m_link(l.template const_cast_to<link_t>()) { }
		iterator(rcptr<volatile link_t>&& l) : m_link(std::move(l).template const_cast_to<link_t>()) { }

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

		void assign(const weak_rcptr<link_t>& l) { m_link = l; if (is_removed()) release(); }

		void assign(const weak_rcptr<link_t>& l) volatile
		{
			rcptr<link_t> lnk = l;
			if (!!lnk && lnk->is_removed())
				lnk.release();
			m_link = std::move(lnk);
		}

		void assign(const volatile weak_rcptr<link_t>& l) { m_link = l; if (is_removed()) release(); }

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

		type* get() const { return (!m_link) ? (type*)0 : m_link->get(); }
		type& operator*() const { return *(m_link->get()); }
		type* operator->() const { return m_link->get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		rcptr<type> get_obj() const
		{
			rcptr<type> result;
			if (!!m_link)
				result = m_link.template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}

		rcptr<type> get_obj() const volatile
		{
			iterator tmp = *this;
			return tmp.get_obj();
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
				if (m_link.compare_exchange(oldValue.next(), oldValue.m_link, oldValue.m_link))
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
				if (m_link.compare_exchange(oldValue.prev(), oldValue.m_link, oldValue.m_link))
					break;
			}
			return oldValue;
		}


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile  { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) volatile { return iterator(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

	};

	/// @brief A volatile container_dlist element iterator
	class volatile_iterator
	{
	protected:
		mutable rcptr<volatile link_t> m_link;

		friend class container_dlist;
		friend class iterator;
		friend class remove_token;
		friend class volatile_remove_token;
		friend class preallocated;

		volatile_iterator(const rcptr<link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcptr<link_t>& l) : m_link(l) { }
		volatile_iterator(rcptr<link_t>&& l) : m_link(std::move(l)) { }

		volatile_iterator(const rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(const volatile rcptr<volatile link_t>& l) : m_link(l) { }
		volatile_iterator(rcptr<volatile link_t>&& l) : m_link(std::move(l)) { }

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
		bool is_active() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		bool is_removed() const { return !!m_link && m_link->is_removed(); }
		bool is_removed() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && lnk->is_removed(); }

		type* get() const { return (!m_link) ? (type*)0 : m_link.template const_cast_to<link_t>()->get(); }
		type& operator*() const { return *(get()); }
		type* operator->() const { return get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		rcptr<type> get_obj() const
		{
			rcptr<type> result;
			if (!!m_link)
				result = m_link.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}

		rcptr<type> get_obj() const volatile
		{
			volatile_iterator tmp = *this;
			return tmp.get_obj();
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
				for (;;)
				{
					rcptr<volatile link_t> newLink = rt->m_next;
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
			bool done = false;
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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
			do {
				rcptr<volatile link_t> oldValue = m_link;
				if (!oldValue)
					break;
				read_token rt;
				oldValue->begin_read_and_complete(rt, true); // OK if already removed
				if (oldValue != m_link)
					continue;
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
			} while (!done);
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


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) { return volatile_iterator(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) volatile { return volatile_iterator(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	/// @brief A preallocated container_dlist element
	class preallocated
	{
	private:
		rcptr<link_t> m_link;

		friend class container_dlist;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		preallocated(const rcptr<link_t>& l) : m_link(l) { }
		preallocated(const volatile rcptr<link_t>& l) : m_link(l) { }
		preallocated(rcptr<link_t>&& l) : m_link(std::move(l)) { }

	public:
		preallocated() { }
		preallocated(const preallocated& src) : m_link(src.m_link) { }
		preallocated(const volatile preallocated& src) : m_link(src.m_link) { }

		preallocated(preallocated&& src) : m_link(std::move(src.m_link)) { }

		preallocated& operator=(const preallocated& i) { m_link = i.m_link; return *this; }
		preallocated& operator=(const volatile preallocated& i) { m_link = i.m_link; return *this; }
		volatile preallocated& operator=(const preallocated& i) volatile { m_link = i.m_link; return *this; }
		volatile preallocated& operator=(const volatile preallocated& i) volatile { m_link = i.m_link; return *this; }

		preallocated& operator=(preallocated&& i) { m_link = std::move(i.m_link); return *this; }
		preallocated& operator=(preallocated&& i) volatile { m_link = std::move(i.m_link); return *this; }

		void disown() { m_link.disown(); }
		void disown() volatile { m_link.disown(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

		type* get() const { return (!m_link) ? (type*)0 : m_link->get(); }
		type& operator*() const { return *(m_link->get()); }
		type* operator->() const { return m_link->get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		rcptr<type> get_obj() const
		{
			rcptr<type> result;
			if (!!m_link)
				result = m_link.template static_cast_to<payload_link_t>()->get_obj();
			return result;
		}

		rcptr<type> get_obj() const volatile
		{
			preallocated tmp = *this;
			return tmp.get_obj();
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


		void swap(preallocated& wth) { m_link.swap(wth.m_link); }
		void swap(preallocated& wth) volatile { m_link.swap(wth.m_link); }
		void swap(volatile preallocated& wth) { m_link.swap(wth.m_link); }


		preallocated exchange(const preallocated& src) { return preallocated(m_link.exchange(src.m_link)); }
		preallocated exchange(const volatile preallocated& src) { return preallocated(m_link.exchange(src.m_link)); }
		preallocated exchange(preallocated&& src) { return preallocated(m_link.exchange(std::move(src.m_link))); }

		preallocated exchange(const preallocated& src) volatile { return preallocated(m_link.exchange(src.m_link)); }
		preallocated exchange(const volatile preallocated& src) volatile { return preallocated(m_link.exchange(src.m_link)); }
		preallocated exchange(preallocated&& src) volatile { return preallocated(m_link.exchange(std::move(src.m_link))); }


		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(const preallocated& src, T3& rtn) { m_link.exchange(src.m_link, rtn.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(const volatile preallocated& src, T3& rtn) { m_link.exchange(src.m_link, rtn.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(preallocated&& src, T3& rtn) { m_link.exchange(std::move(src.m_link), rtn.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(const preallocated& src, T3& rtn) volatile { m_link.exchange(src.m_link, rtn.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(const volatile preallocated& src, T3& rtn) volatile { m_link.exchange(src.m_link, rtn.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(preallocated&& src, T3& rtn) volatile { m_link.exchange(std::move(src.m_link), rtn.m_link); }


		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(const preallocated& src, const T3& cmp) { return m_link.compare_exchange(src.m_link, cmp.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(const volatile preallocated& src, const T3& cmp) { return m_link.compare_exchange(src.m_link, cmp.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(preallocated&& src, const T3& cmp) { return m_link.compare_exchange(std::move(src.m_link), cmp.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(const preallocated& src, const T3& cmp) volatile { return m_link.compare_exchange(src.m_link, cmp.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(const volatile preallocated& src, const T3& cmp) volatile { return m_link.compare_exchange(src.m_link, cmp.m_link); }

		template <typename T3, typename = std::enable_if_t<is_element_reference_type_v<T3> > >
		bool compare_exchange(preallocated&& src, const T3& cmp) volatile { return m_link.compare_exchange(std::move(src.m_link), cmp.m_link); }


		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(const preallocated& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(src.m_link, cmp.m_link, rtn.m_link); }

		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(const volatile preallocated& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(src.m_link, cmp.m_link, rtn.m_link); }

		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(preallocated&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(std::move(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(const preallocated& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(src.m_link, cmp.m_link, rtn.m_link); }

		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(const volatile preallocated& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(src.m_link, cmp.m_link, rtn.m_link); }

		template <typename T3, typename T4, typename = std::enable_if_t<is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(preallocated&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(std::move(src.m_link), cmp.m_link, rtn.m_link); }



	};

	/// @brief A container_dlist element remove token
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class remove_token
	{
	private:
		weak_rcptr<link_t> m_link;

		friend class container_dlist;
		friend class iterator;
		friend class volatile_iterator;
		friend class volatile_remove_token;
		friend class preallocated;

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

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) { return remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) volatile { return remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	/// @brief A volatile container_dlist element remove token
	///
	/// A remove token is like an iterator, but keeps a weak reference to the content.
	class volatile_remove_token
	{
	private:
		weak_rcptr<volatile link_t> m_link;

		friend class container_dlist;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;
		friend class preallocated;

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


	public:
		volatile_remove_token() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token(T2&& i) : volatile_remove_token(forward_member<T2>(i.m_link)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token& operator=(T2&& i) { assign(forward_member<T2>(i.m_link)); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile volatile_remove_token& operator=(T2&& i) volatile { assign(forward_member<T2>(i.m_link)); return *this; }

		bool is_active() const { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }
		bool is_active() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		void release() { m_link.release(); }
		void release() volatile { m_link.release(); }

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

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_link.swap(wth.m_link); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) { return volatile_remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) volatile { return volatile_remove_token(m_link.exchange(forward_member<T2>(src.m_link))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_link.exchange(forward_member<T2>(src.m_link), rtn.m_link); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_link.compare_exchange(forward_member<T2>(src.m_link), cmp.m_link, rtn.m_link); }
	};

	container_dlist()
		: m_sentinel(rcnew(sentinel_link_t))
	{
	}

	container_dlist(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_sentinel(std::move(m_sentinel))
	{ }

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
			COGS_ASSERT(m_sentinel.get_desc()->is_owned()); // Should only be referenced by container_dlist itself
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
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->m_links->m_next = m_sentinel;
			m_sentinel->m_links->m_prev = m_sentinel;
			COGS_ASSERT(m_sentinel.get_desc()->get_strong_count() == 3); // Should only be referenced by container_dlist itself
		}
	}

	bool drain() volatile
	{
		bool foundAny = false;
		while (!!remove_last())
			foundAny = true;
		return foundAny;
	}

	bool is_empty() const { return !m_sentinel || ((m_sentinel->m_links->m_next.get_ptr() == m_sentinel.get_ptr())); }
	bool is_empty() const volatile
	{
		volatile link_t* sentinel = m_sentinel.get_ptr();
		return !sentinel || (sentinel->m_links.begin_read()->m_next.get_ptr() == sentinel);
	}

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

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

	preallocated preallocate() const volatile { preallocated i; i.m_link = container_rcnew(m_allocator, payload_link_t); return i; }
	preallocated preallocate(const type& t) const volatile { preallocated i; i.m_link = container_rcnew(m_allocator, payload_link_t, t); return i; }

	template <typename T2>
	const rcref<T2>& preallocate_with_aux(preallocated& i, unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T2> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T2> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	template <typename T2>
	const rcref<T2>& preallocate_with_aux(const type& t, preallocated& i, unowned_t<rcptr<T2> >& storage = unowned_t<rcptr<T2> >().get_unowned()) const volatile
	{
		typedef aux_payload_link_t<T2> aux_payload_t;
		rcref<aux_payload_t> p = container_rcnew(m_allocator, aux_payload_t, t);
		i.m_link = p;
		return p->get_aux_ref(storage);
	}

	iterator prepend_preallocated(const preallocated& i, insert_mode insertMode = insert_mode::normal) const volatile
	{
		iterator result;
		if (!!i.m_link->insert_after(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	iterator append_preallocated(const preallocated& i, insert_mode insertMode = insert_mode::normal) const volatile
	{
		iterator result;
		if (!!i.m_link->insert_before(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator prepend_preallocated(const preallocated& i, insert_mode insertMode = insert_mode::normal) volatile
	{
		volatile_iterator result;
		if (!!i.m_link->insert_after(lazy_init_sentinel(), insertMode))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator append_preallocated(const preallocated& i, insert_mode insertMode = insert_mode::normal) volatile
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

	iterator insert_preallocated_before(const preallocated& i, const iterator& insertBefore)
	{ 
		iterator result;
		if (!!insertBefore && i.m_link->insert_before(insertBefore.m_link))
			result.m_link = i.m_link;
		return result;
	}

	iterator insert_preallocated_after(const preallocated& i, const iterator& insertAfter)
	{
		iterator result;
		if (!!insertAfter && i.m_link->insert_after(insertAfter.m_link))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator insert_preallocated_before(const preallocated& i, const volatile_iterator& insertBefore) volatile
	{
		volatile_iterator result;
		if (!!insertBefore && i.m_link->insert_before(insertBefore.m_link))
			result.m_link = i.m_link;
		return result;
	}

	volatile_iterator insert_preallocated_after(const preallocated& i, const volatile_iterator& insertAfter) volatile
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
static constexpr size_t container_dlist_preallocated_size = sizeof(container_dlist<int>::preallocated);

static constexpr size_t container_dlist_iterator_alignment = std::alignment_of_v<typename container_dlist<int>::iterator>;
static constexpr size_t container_dlist_volatile_iterator_alignment = std::alignment_of_v<typename container_dlist<int>::volatile_iterator>;
static constexpr size_t container_dlist_remove_token_alignment = std::alignment_of_v<typename container_dlist<int>::remove_token>;
static constexpr size_t container_dlist_volatile_remove_token_alignment = std::alignment_of_v<typename container_dlist<int>::volatile_remove_token>;
static constexpr size_t container_dlist_preallocated_alignment = std::alignment_of_v<typename container_dlist<int>::preallocated>;


#pragma warning(pop)



class rc_obj_base::released_handlers
{
private:
	friend class rc_obj_base;

	container_dlist<function<void(rc_obj_base&)> > m_onReleasedHandlers;
};

class rc_obj_base::released_handler_remove_token
{
private:
	friend class rc_obj_base;

	typedef function<void(rc_obj_base&)> func_t;
	typedef container_dlist<func_t> collection_t;

	collection_t::volatile_remove_token m_removeToken;

	released_handler_remove_token(const collection_t::volatile_iterator& i) : m_removeToken(i) { }
	released_handler_remove_token(const collection_t::volatile_remove_token& rt) : m_removeToken(rt) { }

	released_handler_remove_token& operator=(const collection_t::volatile_iterator& i) { m_removeToken = i; return *this; }
	released_handler_remove_token& operator=(const collection_t::volatile_remove_token& rt) { m_removeToken = rt; return *this; }

public:
	released_handler_remove_token() { }
	released_handler_remove_token(const released_handler_remove_token& rt) : m_removeToken(rt.m_removeToken) { }

	released_handler_remove_token& operator=(const released_handler_remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }

	bool is_active() const { return m_removeToken.is_active(); }

	void release() { m_removeToken.release(); }

	bool operator!() const { return !m_removeToken; }

	bool operator==(const released_handler_remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
	bool operator!=(const released_handler_remove_token& rt) const { return !operator==(rt); }
};

inline rc_obj_base::released_handlers* rc_obj_base::initialize_released_handlers() const
{
	released_handlers* releasedHandlers = atomic::load(m_releasedHandlers);
	if (!releasedHandlers)
	{
		released_handlers* newReleasedHandlers = new (default_allocator::get()) released_handlers;
		if (atomic::compare_exchange(m_releasedHandlers, newReleasedHandlers, releasedHandlers, releasedHandlers))
			return newReleasedHandlers;
		default_allocator::destruct_deallocate_type(newReleasedHandlers);
	}
	return releasedHandlers;
}

template <typename F, typename enable>
inline rc_obj_base::released_handler_remove_token rc_obj_base::on_released(F&& f) const
{
	volatile released_handlers* releasedHandlers = initialize_released_handlers();
	volatile container_dlist<function<void(rc_obj_base&)> >& handler = releasedHandlers->m_onReleasedHandlers;
	released_handler_remove_token result(handler.append(f));
	return result;
}

inline void rc_obj_base::run_released_handlers()
{
	released_handlers* releasedHandlers = atomic::load(m_releasedHandlers);
	if (!!releasedHandlers)
	{
		volatile container_dlist<function<void(rc_obj_base&)> >& handler = releasedHandlers->m_onReleasedHandlers;
		container_dlist<function<void(rc_obj_base&)> >::volatile_iterator itor = handler.get_first();
		while (!!itor)
		{
			(*itor)(*this);
			++itor;
		}
	}
}

inline bool rc_obj_base::uninstall_released_handler(const released_handler_remove_token& removeToken) const
{
	if (!!removeToken)
	{
		released_handlers* releasedHandlers = atomic::load(m_releasedHandlers);
		if (!!releasedHandlers)
		{
			volatile container_dlist<function<void(rc_obj_base&)> >& handler = releasedHandlers->m_onReleasedHandlers;
			return handler.remove(removeToken.m_removeToken);
		}
	}
	return false;
}


inline void rc_obj_base::deallocate_released_handlers()
{
	default_allocator::destruct_deallocate_type(m_releasedHandlers);
}

}


#endif
