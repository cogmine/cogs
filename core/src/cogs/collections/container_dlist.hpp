//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_CONTAINER_DLIST
#define COGS_HEADER_COLLECTION_CONTAINER_DLIST

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/batch_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief A double-link list container collection
/// @tparam T Type to contain.
template <typename T, size_t minimum_batch_size = 32, class memory_manager_t = default_memory_manager>
class container_dlist
{
public:
	typedef T type;
	typedef memory_manager_t memory_manager_type;

	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

private:
	typedef container_dlist<type, minimum_batch_size, memory_manager_t> this_t;

	enum class link_mode
	{
		normal = 0,
		inserting = 1,
		removing = 2,
		removing_next = 3
	};

	enum class insert_mode
	{
		normal = 0,
		only_if_empty = 1,
		only_if_not_empty = 2
	};

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
		volatile link_t* m_sentinel;

		explicit link_t(volatile link_t* sentinel, link_mode linkMode)
			: m_sentinel(sentinel)
		{
			m_links->m_mode = linkMode;
		}

		bool is_sentinel() const { return this == m_sentinel; }
		bool is_sentinel() const volatile { return const_cast<const link_t*>(this)->is_sentinel(); }

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
			link_t* nextPtr = m_links->m_next.get_ptr();
			link_t* prevPtr = m_links->m_prev.get_ptr();
			nextPtr->m_links->m_prev = std::move(m_links->m_prev);
			prevPtr->m_links->m_next = std::move(m_links->m_next);
			return true;
		}

		bool insert_after(const rcref<link_t>& afterThis)
		{
			if (afterThis->m_links->m_mode == link_mode::removing)
				return false;

			m_links->m_prev = afterThis;
			m_links->m_next = afterThis->m_links->m_next;
			afterThis->m_links->m_next = m_links->m_next->m_links->m_prev = this_rcref;
			return true;
		}

		bool insert_before(const rcref<link_t>& beforeThis)
		{
			if (beforeThis->m_links->m_mode == link_mode::removing)
				return false;

			m_links->m_next = beforeThis;
			m_links->m_prev = beforeThis->m_links->m_prev;
			beforeThis->m_links->m_prev = m_links->m_prev->m_links->m_next = this_rcref;
			return true;
		}

		bool remove(bool& wasLast) volatile
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

				ptr<volatile link_t> prev_prev = wt_prev->m_prev.get_ptr();
				wasLast = (prev_prev.get_ptr() == this);

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

		// insertMode != insert_mode::normal implies afterThis is the sentinel,
		// and that true will be returnred.
		template <insert_mode insertMode = insert_mode::normal>
		bool insert_after(const rcref<volatile link_t>& afterThis, bool& wasEmpty)
		{
			bool result = false;
			m_links->m_prev = afterThis.template const_cast_to<link_t>();
			for (;;)
			{
				read_token rt_afterThis;
				bool b = afterThis->begin_read_and_complete(rt_afterThis, false);
				// Shouldn't fail in normal mode, if beforeThis was sentinel.  The sentinel will not get removed.
				COGS_ASSERT(!(insertMode == insert_mode::normal && afterThis->is_sentinel() && !b));
				if (!b)
					break;

				rcptr<volatile link_t> afterThisNext = rt_afterThis->m_next;
				read_token rt_afterThisNext;
				if (!afterThisNext->begin_read_and_complete(rt_afterThisNext, false))
					continue;

				if constexpr (insertMode != insert_mode::normal)
				{
					if (afterThisNext->is_sentinel() == (insertMode == insert_mode::only_if_not_empty))
						break;
				}

				m_links->m_next = afterThisNext.template const_cast_to<link_t>();
				COGS_ASSERT(!!m_links->m_next.get_desc());

				write_token wt_afterThis;
				if (!afterThis->m_links.promote_read_token(rt_afterThis, wt_afterThis))
					continue;
				wt_afterThis->m_next = this_rcptr;
				COGS_ASSERT(!!wt_afterThis->m_next.get_desc());

				if (!afterThis->m_links.end_write(wt_afterThis))
					continue;

				wasEmpty = afterThis.get_ptr() == afterThisNext.get_ptr();
				complete();
				result = true;
				break;
			}
			return result;
		}

		// insertMode != insert_mode::normal implies beforeThis is the sentinel,
		// and that true will be returnred.
		template <insert_mode insertMode = insert_mode::normal>
		bool insert_before(const rcref<volatile link_t>& beforeThis, bool& wasEmpty)
		{
			bool result = false;
			m_links->m_next = beforeThis.template const_cast_to<link_t>();
			COGS_ASSERT(!!m_links->m_next.get_desc());
			for (;;)
			{
				read_token rt_beforeThis;
				bool b = beforeThis->begin_read_and_complete(rt_beforeThis, false);
				// Shouldn't fail in normal mode, if beforeThis was sentinel.  The sentinel will not get removed.
				COGS_ASSERT(!(insertMode == insert_mode::normal && beforeThis->is_sentinel() && !b));
				if (!b)
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

				if constexpr (insertMode != insert_mode::normal)
				{
					if (beforeThisPrev->is_sentinel() == (insertMode == insert_mode::only_if_not_empty))
						break;
				}

				m_links->m_prev = beforeThisPrev.template const_cast_to<link_t>();
				write_token wt_beforeThisPrev;
				if (!beforeThisPrev->m_links.promote_read_token(rt_beforeThisPrev, wt_beforeThisPrev))
					continue;
				wt_beforeThisPrev->m_next = this_rcptr;
				COGS_ASSERT(!!wt_beforeThisPrev->m_next.get_desc());
				if (!beforeThisPrev->m_links.end_write(wt_beforeThisPrev))
					continue;

				wasEmpty = beforeThis.get_ptr() == beforeThisPrev.get_ptr();
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
					COGS_ASSERT(!!wt_prev->m_next.get_desc());
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
		type* get_payload() const { return &(const_cast<payload_link_t*>(this)->m_value.get()); }
		type* get_payload() const volatile { return &(const_cast<payload_link_t*>(this)->m_value.get()); }

		explicit payload_link_t(volatile link_t* sentinel, link_mode linkMode)
			: link_t(sentinel, linkMode)
		{
			sentinel->get_desc()->acquire();
		}

		const rcref<type>& get_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned())
		{
			storage.set(&m_value.get(), object::get_desc());
			return storage.dereference();
		}
	};

	class sentinel_link_t;

	class payload_rc_obj : public rc_obj<payload_link_t>
	{
	public:
		virtual void released();
		virtual void dispose();
	};

	typedef batch_allocator<payload_rc_obj, minimum_batch_size, memory_manager_t> allocator_type;

	class sentinel_link_t : public link_t
	{
	public:
		using link_t::m_links;

		alignas(atomic::get_alignment_v<ptrdiff_t>) ptrdiff_t m_count = 0;

		allocator_type m_allocator;

		explicit sentinel_link_t(allocator_type&& al)
			: link_t(this, link_mode::normal),
			m_allocator(std::move(al))
		{
			m_links->m_next = m_links->m_prev = this_rcptr;
			COGS_ASSERT(!!m_links->m_next.get_desc());
			COGS_ASSERT(!!m_links->m_prev.get_desc());
		}

		void clear()
		{
			m_links->m_next.release();
			m_links->m_prev.release();
		}

		void dec_count()
		{
			--m_count;
		}

		void dec_count() volatile
		{
			cogs::assign_prev(m_count);
		}
	};

	class sentinel_rc_obj : public rc_obj<sentinel_link_t>
	{
	public:
		virtual void released() { }

		virtual void dispose()
		{
			sentinel_link_t* p = rc_obj<sentinel_link_t>::get_object();
			allocator_type al(std::move(p->m_allocator));
			p->sentinel_link_t::~sentinel_link_t();
			al.get_memory_manager().destruct_deallocate_type(this);
		}
	};

	rcptr<sentinel_link_t> m_sentinel;

	void clear_inner()
	{
		// Could allow the links to free themselves, but that would cause a cascade of
		// link releases, that could potential overflow the stack if the list is long enough.
		COGS_ASSERT(!!m_sentinel);
		rcptr<link_t> l = m_sentinel->m_links->m_next;
		while (l.get_ptr() != m_sentinel.get_ptr())
		{
			rcptr<link_t> next = l->m_links->m_next;
			l->m_links->m_next = 0;
			l->m_links->m_prev = 0;
			l = std::move(next);
		}
	}

	static rcref<sentinel_link_t> create_sentinel()
	{
		allocator_type al;
		sentinel_rc_obj* sentinel = al.get_memory_manager().template allocate_type<sentinel_rc_obj>();
		new (sentinel) sentinel_rc_obj;
		return placement_rcnew(sentinel)(std::move(al));
	}

	const rcref<sentinel_link_t>& lazy_init_sentinel()
	{
		if (!m_sentinel)
			m_sentinel = create_sentinel();
		return m_sentinel.dereference();
	}

	rcref<volatile sentinel_link_t> lazy_init_sentinel() volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
		{
			rcptr<sentinel_link_t> newSentinel = create_sentinel();
			if (m_sentinel.compare_exchange(newSentinel, sentinel, sentinel))
				sentinel = newSentinel;
			else
				newSentinel->clear();
		}
		return std::move(sentinel.dereference());
	}

	template <typename F>
	iterator insert_inner(F&& f, const rcref<sentinel_link_t>& sentinel)
	{
		volatile allocator_type& al = sentinel->m_allocator;
		payload_rc_obj* payloadBuffer = new (al.allocate()) payload_rc_obj;
		rcref<link_t> lnk = placement_rcnew(payloadBuffer)(sentinel.get_ptr(), link_mode::normal);
		iterator i(std::move(lnk));
		if (f(i))
			++(sentinel->m_count);
		else
			i.release();
		return i;
	}

	template <typename F>
	iterator insert_inner(F&& f)
	{
		const rcref<sentinel_link_t>& sentinel = lazy_init_sentinel();
		return insert_inner([&](iterator& i)
		{
			return f(i, sentinel);
		}, sentinel);
	}

	template <typename F>
	volatile_iterator insert_inner(F&& f, const rcref<volatile sentinel_link_t>& sentinel) volatile
	{
		payload_rc_obj* payloadBuffer = new (sentinel->m_allocator.allocate()) payload_rc_obj;
		rcref<link_t> lnk = placement_rcnew(payloadBuffer)(sentinel.get_ptr(), link_mode::inserting);
		iterator i(std::move(lnk));
		if (f(i))
			assign_next(sentinel->m_count);
		else
			i.release();
		volatile_iterator i2 = std::move(i);
		return i2;
	}

	template <typename F>
	volatile_iterator insert_inner(F&& f) volatile
	{
		rcref<volatile sentinel_link_t> sentinel = lazy_init_sentinel();
		return insert_inner([&](iterator& i)
		{
			return f(i, sentinel);
		}, sentinel);
	}

	class internal_t {};
public:
	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A container_dlist element iterator
	class iterator
	{
	private:
		rcptr<link_t> m_link;

		friend class container_dlist;
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

		type* get() const { return (!m_link) ? (type*)0 : m_link.template static_cast_to<payload_link_t>()->get_payload(); }
		type& operator*() const { return *(get()); }
		type* operator->() const { return get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		const rcptr<type>& get_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
		{
			if (!!m_link)
				storage.set(m_link.template static_cast_to<payload_link_t>()->get_payload(), m_link.get_desc());
			return storage;
		}

		rcptr<type> get_obj() const volatile
		{
			rcptr<type> result;
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


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_link.swap(wth.m_link); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile  { m_link.swap(wth.m_link); }

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

	/// @brief A volatile container_dlist element iterator
	class volatile_iterator
	{
	protected:
		mutable rcptr<volatile link_t> m_link;

		friend class container_dlist;
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
		bool is_active() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && !lnk->is_removed(); }

		bool is_removed() const { return !!m_link && m_link->is_removed(); }
		bool is_removed() const volatile { rcptr<volatile link_t> lnk(m_link); return !!lnk && lnk->is_removed(); }

		type* get() const { return (!m_link) ? (type*)0 : m_link.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload(); }
		type& operator*() const { return *(get()); }
		type* operator->() const { return get(); }

		rc_obj_base* get_desc() const { return m_link.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_link.get_desc(); }

		const rcptr<type>& get_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
		{
			if (!!m_link)
				storage.set(m_link.template const_cast_to<link_t>().template static_cast_to<payload_link_t>()->get_payload(), m_link.get_desc());
			return storage;
		}

		rcptr<type> get_obj() const volatile
		{
			rcptr<type> result;
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
		volatile_iterator exchange(T2&& src) { return volatile_iterator(m_link.exchange(forward_member<T2>(src.m_link))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) volatile { return volatile_iterator(m_link.exchange(forward_member<T2>(src.m_link))); }


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

	/// @brief A container_dlist element volatile remove token
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

	container_dlist() { }

	template <typename arg1_t>
	container_dlist(arg1_t&& arg1, std::enable_if_t<!std::is_convertible_v<std::remove_cv_t<arg1_t>, this_t>, internal_t> = {})
	{
		append(std::forward<arg1_t>(arg1));
	}

	template <typename arg1_t, typename arg2_t, typename... args_t>
	container_dlist(arg1_t&& arg1, arg2_t&& arg2, args_t&&... args)
	{
		append(std::forward<arg1_t>(arg1));
		append(std::forward<arg2_t>(arg2));
		(append(std::forward<args_t>(args)), ...);
	}

	container_dlist(this_t&& src)
		: m_sentinel(std::move(src.m_sentinel))
	{ }

	container_dlist(this_t& src)
	{
		for (const auto& e : src)
			append(e);
	}

	container_dlist(const this_t& src)
	{
		for (const auto& e : src)
			append(e);
	}

	~container_dlist()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->clear();
		}
	}

	container_dlist(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		clear();
		for (const auto& e : src)
			append(e);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		if (!!m_sentinel)
			clear_inner();
		m_sentinel = std::move(src.m_sentinel);
		src.m_sentinel = nullptr;
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	volatile this_t& operator=(this_t&&) volatile = delete;
	volatile this_t& operator=(const this_t&) volatile = delete;
	volatile this_t& operator=(const volatile this_t&) volatile = delete;

	bool operator==(const this_t& cmp) const
	{
		if (this == &cmp)
			return true;
		if (size() != cmp.size())
			return false;
		iterator i1 = get_first();
		iterator i2 = cmp.get_first();
		for (;;)
		{
			if (!(*i1 == *i2))
				return false;
			if (!++i1)
				break;
			++i2;
		}
		return true;
	}

	bool operator==(const volatile this_t& cmp) const
	{
		this_t tmp(cmp);
		return *this == tmp;
	}

	bool operator==(const this_t& cmp) const volatile
	{
		this_t tmp(*this);
		return cmp == tmp;
	}

	bool operator==(const volatile this_t& cmp) const volatile
	{
		this_t tmp1(*this);
		this_t tmp2(cmp);
		return tmp1 == tmp2;
	}

	bool operator!=(const this_t& cmp) const { return !(*this == cmp); }
	bool operator!=(const volatile this_t& cmp) const { return !(*this == cmp); }
	bool operator!=(const this_t& cmp) const volatile { return !(*this == cmp); }
	bool operator!=(const volatile this_t& cmp) const volatile { return !(*this == cmp); }

	void clear()
	{
		if (!!m_sentinel)
		{
			clear_inner();
			m_sentinel->m_links->m_next = m_sentinel;
			m_sentinel->m_links->m_prev = m_sentinel;
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
		return m_sentinel->m_count;
	}

	size_t size() const volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
			return 0;
		ptrdiff_t sz;
		atomic::load(sentinel->m_count, sz);
		if (sz < 0) // The count can bounce below zero, if an item is added, removed, and dec'ed before inc'ed.
			sz = 0;
		return sz;
	}

	bool is_empty() const
	{
		return !m_sentinel || !m_sentinel->m_count;
	}

	bool is_empty() const volatile
	{
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!sentinel)
			return true;
		ptrdiff_t sz;
		atomic::load(sentinel->m_count, sz);
		return (sz <= 0);
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
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
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
				i.m_link = std::move(lnk);
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
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
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
				i.m_link = std::move(lnk);
				break;
			}
		}
		return i;
	}

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	prepend_via(F&& f)
	{
		return insert_inner([&](iterator& i, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			return l->insert_after(sentinel);
		});
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	prepend_via(F&& f) { return prepend_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	prepend_via(F&& f) { return prepend_via([&](iterator& i) { f(*i.get()); }); }

	iterator prepend(const type& src) { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator prepend(type&& src) { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	prepend(F&& f) { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> iterator prepend_emplace(args_t&&... args) { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }

	struct volatile_insert_result
	{
		volatile_iterator inserted;
		bool wasEmpty;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	prepend_via(F&& f) volatile
	{
		bool wasEmpty = false;
		volatile_iterator inserted = insert_inner([&](iterator& i, const rcref<volatile sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			return l->insert_after(sentinel, wasEmpty);
		});
		return { std::move(inserted), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_result>
	prepend_via(F&& f) volatile { return prepend_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_result>
	prepend_via(F&& f) volatile { return prepend_via([&](iterator& i) { f(*i.get()); }); }

	volatile_insert_result prepend(const type& src) volatile { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result prepend(type&& src) volatile { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	prepend(F&& f) volatile { return prepend_via(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_result prepend_emplace(args_t&&... args) volatile { return prepend_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	prepend_via_if_not_empty(F&& f)
	{
		iterator i2;
		if (!is_empty())
		{
			i2 = insert_inner([&](iterator& i, const rcref<sentinel_link_t>& sentinel)
			{
				link_t* l = i.m_link.get_ptr();
				f(i);
				return l->insert_after(sentinel);
			});
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	prepend_via_if_not_empty(F&& f) { return prepend_via_if_not_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	prepend_via_if_not_empty(F&& f) { return prepend_via_if_not_empty([&](const iterator& i) { f(*i.get()); }); }

	iterator prepend_if_not_empty(const type& src) { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator prepend_if_not_empty(type&& src) { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	prepend_if_not_empty(F&& f) { return prepend_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> iterator prepend_emplace_if_not_empty(args_t&&... args) { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	prepend_via_if_not_empty(F&& f) volatile
	{
		volatile_iterator i2;
		bool wasEmpty = false;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			ptrdiff_t sz;
			atomic::load(sentinel->m_count, sz);
			if (sz > 0)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					return l->template insert_after<insert_mode::only_if_not_empty>(sentinel.dereference(), wasEmpty);
				}, sentinel.dereference());
			}
		}
		return { std::move(i2), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, const iterator>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_result>
	prepend_via_if_not_empty(F&& f) volatile { return prepend_via_if_not_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_result>
	prepend_via_if_not_empty(F&& f) volatile { return prepend_via_if_not_empty([&](const iterator& i) { f(*i.get()); }); }

	volatile_insert_result prepend_if_not_empty(const type& src) volatile { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result prepend_if_not_empty(type&& src) volatile { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	prepend_if_not_empty(F&& f) volatile { return prepend_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_result prepend_emplace_if_not_empty(args_t&&... args) volatile { return prepend_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	append_via(F&& f)
	{
		return insert_inner([&](iterator& i, const rcref<sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			return l->insert_before(sentinel);
		});
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	append_via(F&& f) { return append_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	append_via(F&& f) { return append_via([&](iterator& i) { f(*i.get()); }); }

	iterator append(const type& src) { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator append(type&& src) { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	append(F&& f) { return append_via(std::forward<F>(f)); }


	iterator operator+=(const type& src) { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator operator+=(type&& src) { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	operator+=(F&& f) { return append_via(std::forward<F>(f)); }

	template <typename... args_t> iterator append_emplace(args_t&&... args) { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	append_via(F&& f) volatile
	{
		bool wasEmpty = false;
		volatile_iterator i2 = insert_inner([&](iterator& i, const rcref<volatile sentinel_link_t>& sentinel)
		{
			link_t* l = i.m_link.get_ptr();
			f(i);
			return l->insert_before(sentinel, wasEmpty);
		});
		return { std::move(i2), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_result>
	append_via(F&& f) volatile { return append_via([&](iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_result>
	append_via(F&& f) volatile { return append_via([&](iterator& i) { f(*i.get()); }); }

	volatile_insert_result append(const type& src) volatile { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result append(type&& src) volatile { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	append(F&& f) volatile { return append_via(std::forward<F>(f)); }

	volatile_insert_result operator+=(const type& src) volatile { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result operator+=(type&& src) volatile { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	operator+=(F&& f) volatile { return append_via(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_result append_emplace(args_t&&... args) volatile { return append_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	append_via_if_not_empty(F&& f)
	{
		iterator i2;
		if (!is_empty())
		{
			i2 = insert_inner([&](iterator& i, const rcref<sentinel_link_t>& sentinel)
			{
				link_t* l = i.m_link.get_ptr();
				f(i);
				return l->insert_before(sentinel);
			});
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	append_via_if_not_empty(F&& f) { return append_via_if_not_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	append_via_if_not_empty(F&& f) { return append_via_if_not_empty([&](const iterator& i) { f(*i.get()); }); }

	iterator append_if_not_empty(const type& src) { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator append_if_not_empty(type&& src) { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	append_if_not_empty(F&& f) { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> iterator append_emplace_if_not_empty(args_t&&... args) { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	append_via_if_not_empty(F&& f) volatile
	{
		volatile_iterator i2;
		bool wasEmpty = false;
		rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
		if (!!sentinel)
		{
			ptrdiff_t sz;
			atomic::load(sentinel->m_count, sz);
			if (sz > 0)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					return l->insert_before<insert_mode::only_if_not_empty>(sentinel.dereference(), wasEmpty);
				}, sentinel.dereference());
			}
		}
		return { std::move(i2), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_result>
	append_via_if_not_empty(F&& f) volatile { return append_via_if_not_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_result>
	append_via_if_not_empty(F&& f) volatile { return append_via_if_not_empty([&](const iterator& i) { f(*i.get()); }); }

	volatile_insert_result append_if_not_empty(const type& src) volatile { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result append_if_not_empty(type&& src) volatile { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	append_if_not_empty(F&& f) volatile { return append_via_if_not_empty(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_result append_emplace_if_not_empty(args_t&&... args) volatile { return append_via_if_not_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_via_if_empty(F&& f)
	{
		iterator i2;
		if (is_empty())
		{
			i2 = insert_inner([&](iterator& i, const rcref<sentinel_link_t>& sentinel)
			{
				link_t* l = i.m_link.get_ptr();
				f(i);
				return l->insert_before(sentinel);
			});
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	insert_via_if_empty(F&& f) { return insert_via_if_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	insert_via_if_empty(F&& f) { return insert_via_if_empty([&](const iterator& i) { f(*i.get()); }); }

	iterator insert_if_empty(const type& src) { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator insert_if_empty(type&& src) { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	insert_if_empty(F&& f) { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> iterator insert_emplace_if_empty(args_t&&... args) { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	insert_via_if_empty(F&& f) volatile
	{
		volatile_iterator i2;
		bool wasEmpty = false;
		rcref<volatile sentinel_link_t> sentinel = lazy_init_sentinel();
		ptrdiff_t sz;
		atomic::load(sentinel->m_count, sz);
		if (sz <= 0)
		{
			i2 = insert_inner([&](iterator& i)
			{
				link_t* l = i.m_link.get_ptr();
				f(i);
				return l->insert_before<insert_mode::only_if_empty>(sentinel, wasEmpty);
			}, sentinel);
		}
		return { std::move(i2), wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_result>
	insert_via_if_empty(F&& f) volatile { return insert_via_if_empty([&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_result>
	insert_via_if_empty(F&& f) volatile { return insert_via_if_empty([&](const iterator& i) { f(*i.get()); }); }

	volatile_insert_result insert_if_empty(const type& src) volatile { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_insert_result insert_if_empty(type&& src) volatile { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_insert_result>
	insert_if_empty(F&& f) volatile { return insert_via_if_empty(std::forward<F>(f)); }

	template <typename... args_t> volatile_insert_result insert_emplace_if_empty(args_t&&... args) volatile { return insert_via_if_empty([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_before_via(const iterator& insertBefore, F&& f)
	{
		iterator i2;
		if (!!insertBefore)
		{
			rcptr<sentinel_link_t> sentinel = m_sentinel;
			if (!!sentinel && sentinel.get_ptr() == insertBefore.m_link->m_sentinel)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					return l->insert_before(insertBefore.m_link.dereference());
				}, sentinel.dereference());
			}
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	insert_before_via(const iterator& insertBefore, F&& f) { return insert_before_via(insertBefore, [&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	insert_before_via(const iterator& insertBefore, F&& f) { return insert_before_via(insertBefore, [&](const iterator& i) { f(*i.get()); }); }

	iterator insert_before(const iterator& insertBefore, const type& src) { return insert_before_via(insertBefore, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator insert_before(const iterator& insertBefore, type&& src) { return insert_before_via(insertBefore, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	insert_before(const iterator& insertBefore, F&& f) { return insert_before_via(insertBefore, std::forward<F>(f)); }

	template <typename... args_t> iterator insert_before_emplace(const iterator& insertBefore, args_t&&... args) { return insert_before_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_iterator>
	insert_before_via(const volatile_iterator& insertBefore, F&& f) volatile
	{
		volatile_iterator i2;
		if (!!insertBefore)
		{
			rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
			if (!!sentinel && sentinel.get_ptr() == insertBefore.m_link->m_sentinel)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					bool wasEmpty;
					return l->insert_before(insertBefore.m_link.dereference(), wasEmpty);
				}, sentinel.dereference());
			}
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_iterator>
	insert_before_via(const volatile_iterator& insertBefore, F&& f) volatile { return insert_before_via(insertBefore, [&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_iterator>
	insert_before_via(const volatile_iterator& insertBefore, F&& f) volatile { return insert_before_via(insertBefore, [&](const iterator& i) { f(*i.get()); }); }

	volatile_iterator insert_before(const volatile_iterator& insertBefore, const type& src) volatile { return insert_before_via(insertBefore, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_iterator insert_before(const volatile_iterator& insertBefore, type&& src) volatile { return insert_before_via(insertBefore, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_iterator>
	insert_before(const volatile_iterator& insertBefore, F&& f) volatile { return insert_before_via(insertBefore, std::forward<F>(f)); }

	template <typename... args_t> volatile_iterator insert_before_emplace(const volatile_iterator& insertBefore, args_t&&... args) volatile { return insert_before_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_after_via(const iterator& insertAfter, F&& f)
	{
		iterator i2;
		if (!!insertAfter)
		{
			rcptr<sentinel_link_t> sentinel = m_sentinel;
			if (!!sentinel && sentinel.get_ptr() == insertAfter.m_link->m_sentinel)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					return l->insert_after(insertAfter.m_link.dereference());
				}, sentinel.dereference());
			}
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		iterator>
	insert_after_via(const iterator& insertAfter, F&& f) { return insert_after_via(insertAfter, [&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		iterator>
	insert_after_via(const iterator& insertAfter, F&& f) { return insert_after_via(insertAfter, [&](const iterator& i) { f(*i.get()); }); }

	iterator insert_after(const iterator& insertAfter, const type& src) { return insert_after_via(insertAfter, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	iterator insert_after(const iterator& insertAfter, type&& src) { return insert_after_via(insertAfter, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		iterator>
	insert_after(const iterator& insertAfter, F&& f) { return insert_after_via(insertAfter, std::forward<F>(f)); }

	template <typename... args_t> iterator insert_after_emplace(const iterator& insertAfter, args_t&&... args) { return insert_after_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_iterator>
	insert_after_via(const volatile_iterator& insertAfter, F&& f) volatile
	{
		volatile_iterator i2;
		if (!!insertAfter)
		{
			rcptr<volatile sentinel_link_t> sentinel = m_sentinel;
			if (!!sentinel && sentinel.get_ptr() == insertAfter.m_link->m_sentinel)
			{
				i2 = insert_inner([&](iterator& i)
				{
					link_t* l = i.m_link.get_ptr();
					f(i);
					bool wasEmpty;
					return l->insert_after(insertAfter.m_link.dereference(), wasEmpty);
				}, sentinel.dereference());
			}
		}
		return i2;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_iterator>
	insert_after_via(const volatile_iterator& insertAfter, F&& f) volatile { return insert_after_via(insertAfter, [&](const iterator& i) { f(i.get_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_iterator>
	insert_after_via(const volatile_iterator& insertAfter, F&& f) volatile { return insert_after_via(insertAfter, [&](const iterator& i) { f(*i.get()); }); }

	volatile_iterator insert_after(const volatile_iterator& insertAfter, const type& src) volatile { return insert_after_via(insertAfter, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(src); }); }
	volatile_iterator insert_after(const volatile_iterator& insertAfter, type&& src) volatile { return insert_after_via(insertAfter, [&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::move(src)); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_constructible_v<type, F&&>
		&& !std::is_convertible_v<F, const type&>
		&& !std::is_convertible_v<F, type&&>,
		volatile_iterator>
	insert_after(const volatile_iterator& insertAfter, F&& f) volatile { return insert_after_via(insertAfter, std::forward<F>(f)); }

	template <typename... args_t> volatile_iterator insert_after_emplace(const volatile_iterator& insertAfter, args_t&&... args) volatile { return insert_after_via([&](const rcref<type>& t) { nested_rcnew(t.get_ptr(), *t.get_desc())(std::forward<args_t>(args)...); }); }


	bool remove(const iterator& i)
	{
		bool b = false;
		if (!!i.m_link)
		{
			b = i.m_link->remove();
			if (b && !!m_sentinel)
				--m_sentinel->m_count;
		}
		return b;
	}

	bool remove(const remove_token& rt)
	{
		iterator i(rt);
		return remove(i);
	}

	bool operator-=(const iterator& i) { return remove(i); }
	bool operator-=(const remove_token& rt) { return remove(rt); }

	struct volatile_remove_result
	{
		bool wasRemoved;
		bool wasEmptied;
	};

	volatile_remove_result remove(const volatile_iterator& i) volatile
	{
		bool result = false;
		bool wasLast = false;
		if (!!i.m_link)
		{
			result = i.m_link->remove(wasLast);
			if (result)
			{
				volatile link_t* sentinel = i.m_link.template static_cast_to<volatile payload_link_t>()->m_sentinel;
				((volatile sentinel_link_t*)sentinel)->dec_count();
			}
		}
		return { result, wasLast };
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

	pop_result peek_first() volatile
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
				rcptr<volatile link_t> firstLink = rt_sentinel->m_next;
				if (firstLink.get_ptr() == sentinel.get_ptr())
					break;
				if (firstLink->is_removed()) // also completes other states
					continue;

				write_token wt;
				if (!sentinel->m_links.promote_read_token(rt_sentinel, wt))
					continue;

				COGS_ASSERT(wt->m_mode == link_mode::normal);
				COGS_ASSERT(wt->m_next == firstLink);

				bool mayBeLast = wt->m_next.get_ptr() == wt->m_prev.get_ptr();

				wt->m_mode = link_mode::removing_next;
				if (!sentinel->m_links.end_write(wt))
					continue;

				wasLast = mayBeLast;
				sentinel->begin_read_and_complete(rt_sentinel, false); // completes remove-next
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
		read_token rt_sentinel;
		read_token rt_back;
		read_token rt_prev;
		if (!!sentinel)
		{
			for (;;)
			{
				sentinel->begin_read_and_complete(rt_sentinel, false); // sentinel won't get deleted, no need to check result
				rcptr<volatile link_t> lastLink = rt_sentinel->m_prev;
				if (lastLink.get_ptr() == sentinel.get_ptr())
					break;
				if (lastLink->is_removed()) // also completes other states
					continue;
				if (!lastLink->begin_read_and_complete(rt_back, false))
					continue;
				rcptr<volatile link_t> prev = rt_back->m_prev;
				if (!prev->begin_read_and_complete(rt_prev, false))
					continue;
				if (!lastLink->m_links.is_current(rt_back))
					continue;
				if (!sentinel->m_links.is_current(rt_sentinel))
					continue;

				write_token wt_prev;
				if (!prev->m_links.promote_read_token(rt_prev, wt_prev))
					continue;

				COGS_ASSERT(wt_prev->m_mode == link_mode::normal);
				COGS_ASSERT(wt_prev->m_next == lastLink);

				bool mayBeLast = wt_prev->m_next.get_ptr() == wt_prev->m_prev.get_ptr();

				wt_prev->m_mode = link_mode::removing_next;
				if (!prev->m_links.end_write(wt_prev))
					continue;

				wasLast = mayBeLast;
				prev->begin_read_and_complete(rt_prev, false); // completes remove-next
				sentinel->dec_count();
				itor.m_link = std::move(lastLink);
				break;
			}
		}
		return { std::move(itor), wasLast };
	}

	void swap(this_t& wth)
	{
		m_sentinel.swap(wth.m_sentinel);
	}

	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//void swap(this_t& wth) volatile
	//{
	//	m_sentinel.swap(wth.m_sentinel);
	//}
	//
	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//void swap(volatile this_t& wth)
	//{
	//	m_sentinel.swap(wth.m_sentinel);
	//}

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//this_t exchange(this_t&& src) volatile
	//{
	//	this_t tmp(std::move(src));
	//	swap(tmp);
	//	return tmp;
	//}

	void exchange(this_t&& src, this_t& rtn)
	{
		rtn = std::move(src);
		swap(rtn);
	}

	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//void exchange(this_t&& src, this_t& rtn) volatile
	//{
	//	rtn = std::move(src);
	//	swap(rtn);
	//}

	iterator begin() const { return get_first(); }
	volatile_iterator begin() const volatile { return get_first(); }

	iterator rbegin() const { return get_last (); }
	volatile_iterator rbegin() const volatile { return get_last(); }

	iterator end() const { iterator i; return i; }
	volatile_iterator end() const volatile { volatile_iterator i; return i; }

	iterator rend() const { iterator i; return i; }
	volatile_iterator rend() const volatile { volatile_iterator i; return i; }
};

template <typename T, size_t minimum_batch_size, class memory_manager_t>
inline void container_dlist<T, minimum_batch_size, memory_manager_t>::payload_rc_obj::released()
{
	payload_link_t* p = rc_obj<payload_link_t>::get_object();
	type* t = p->get_payload();
	t->~type();
}

template <typename T, size_t minimum_batch_size, class memory_manager_t>
inline void container_dlist<T, minimum_batch_size, memory_manager_t>::payload_rc_obj::dispose()
{
	payload_link_t* p = rc_obj<payload_link_t>::get_object();
	volatile sentinel_link_t* sentinel = static_cast<volatile sentinel_link_t*>(p->m_sentinel);
	p->payload_link_t::~payload_link_t();
	sentinel->m_allocator.destruct_deallocate(this);
	sentinel->get_desc()->release();
}


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
		released_handlers* newReleasedHandlers = default_allocator<released_handlers>::allocate();
		new (newReleasedHandlers) released_handlers;
		if (atomic::compare_exchange(m_releasedHandlers, newReleasedHandlers, releasedHandlers, releasedHandlers))
			return newReleasedHandlers;
		default_allocator<released_handlers>::destruct_deallocate(newReleasedHandlers);
	}
	return releasedHandlers;
}

template <typename F, typename enable>
inline rc_obj_base::released_handler_remove_token rc_obj_base::on_released(F&& f) const
{
	volatile released_handlers* releasedHandlers = initialize_released_handlers();
	volatile container_dlist<function<void(rc_obj_base&)> >& handler = releasedHandlers->m_onReleasedHandlers;
	released_handler_remove_token result(handler.append(f).inserted);
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
			return handler.remove(removeToken.m_removeToken).wasRemoved;
		}
	}
	return false;
}


inline void rc_obj_base::deallocate_released_handlers()
{
	default_allocator<released_handlers>::destruct_deallocate(m_releasedHandlers);
}


}


#endif
