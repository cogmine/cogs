//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_WEAK_RCPTR_LIST
#define COGS_HEADER_COLLECTION_WEAK_RCPTR_LIST

#include <type_traits>


#include "cogs/collections/container_dlist.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {


template <typename T>
class weak_rcptr_list : public object
{
private:
	typedef weak_rcptr_list<T> this_t;

	class node
	{
	public:
		weak_rcptr<T> m_obj;
		rc_obj_base::released_handler_remove_token m_removeToken;

		node(const rcref<T>& t, rc_obj_base::released_handler_remove_token&& rt)
			: m_obj(t),
			m_removeToken(std::move(rt))
		{ }
	};

	typedef container_dlist<node> list_t;
	list_t m_list;

	weak_rcptr_list(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;


public:
	class iterator;
	class remove_token;

	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	class iterator
	{
	private:
		friend class weak_rcptr_list;
		friend class remove_token;

		typename list_t::volatile_iterator m_contents;

		iterator(const typename list_t::iterator& i) : m_contents(i) { }
		iterator(typename list_t::iterator&& i) : m_contents(std::move(i)) { }

		iterator(const typename list_t::volatile_iterator& i) : m_contents(i) { }
		iterator(typename list_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		iterator(const typename list_t::remove_token& rt) : m_contents(rt) { }
		iterator(const typename list_t::volatile_remove_token& rt) : m_contents(rt) { }

		iterator& operator=(const typename list_t::iterator& i) { m_contents = i; return *this; }
		iterator& operator=(typename list_t::iterator&& i) { m_contents = std::move(i); return *this; }

		iterator& operator=(const typename list_t::volatile_iterator& i) { m_contents = i; return *this; }
		iterator& operator=(typename list_t::volatile_iterator&& i) { m_contents = std::move(i); return *this; }

		iterator& operator=(const typename list_t::remove_token& rt) { m_contents = rt; return *this; }
		iterator& operator=(const typename list_t::volatile_remove_token& rt) { m_contents = rt; return *this; }

	public:
		iterator() { }
		iterator(const iterator& i) : m_contents(i.m_contents) { }
		iterator(const remove_token& rt) : m_contents(rt.m_contents) { }
		iterator(const volatile iterator& i) : m_contents(i.m_contents) { }
		iterator(const volatile remove_token& rt) : m_contents(rt.m_contents) { }

		iterator(iterator&& i) : m_contents(std::move(i.m_contents)) { }

		iterator& operator=(const iterator& i) { m_contents = i.m_contents; return *this; }
		iterator& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		iterator& operator=(const volatile iterator& i) { m_contents = i.m_contents; return *this; }
		iterator& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		iterator& operator=(iterator&& i) { m_contents = std::move(i.m_contents); return *this; }

		volatile iterator& operator=(const iterator& i) volatile { m_contents = i.m_contents; return *this; }
		volatile iterator& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }
		volatile iterator& operator=(iterator&& i) volatile { m_contents = std::move(i.m_contents); return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const
		{
			if (!m_contents)
				return false;
			if (!m_contents.is_active())
				return false;
			if (!m_contents->m_removeToken) // non rc obj
				return true;
			rc_obj_base* desc = m_contents->m_obj.get_desc();
			if (!desc)
				return true;
			return !desc->is_released();
		}

		bool is_active() const volatile
		{
			iterator itor(*this);
			return itor.is_active();
		}

		bool is_removed() const
		{
			if (!m_contents)
				return false;
			if (m_contents.is_removed())
				return true;
			if (!m_contents->m_removeToken) // non rc obj
				return false;
			rc_obj_base* desc = m_contents->m_obj.get_desc();
			if (!desc)
				return false;
			return desc->is_released();
		}

		bool is_removed() const volatile
		{
			iterator itor(*this);
			return itor.is_removed();
		}

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const iterator& i) const { return m_contents == i.m_contents; }
		bool operator==(const volatile iterator& i) const { return m_contents == i.m_contents; }
		bool operator==(const iterator& i) const volatile { return m_contents == i.m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const volatile { return !operator==(i); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }

		iterator& operator++()
		{
			if (!!*this)
			{
				for (;;)
				{
					++m_contents;
					if (!is_removed())
						break;
				}
			}
			return *this;
		}

		iterator operator++() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				newValue = oldValue;
				++newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return newValue;
		}

		iterator& operator--()
		{
			if (!!*this)
			{
				for (;;)
				{
					--m_contents;
					if (!is_removed())
						break;
				}
			}
			return *this;
		}

		iterator operator--() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				newValue = oldValue;
				--newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return newValue;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		iterator operator++(int) volatile
		{
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				iterator newValue(oldValue);
				++newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return oldValue;
		}

		iterator operator--(int) volatile
		{
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				iterator newValue(oldValue);
				--newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return oldValue;
		}

		rcptr<T> get() const
		{
			rcptr<T> result;
			if (!!m_contents)
				result = m_contents->m_obj;
			return result;
		}

		T& operator*() const { return *get(); }
		rcptr<T> operator->() const { return get(); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) { return iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) volatile { return iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3>&& is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3>&& is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	class remove_token
	{
	private:
		friend class weak_rcptr_list;
		friend class iterator;

		typename list_t::volatile_remove_token m_contents;

		remove_token(const typename list_t::iterator& i) : m_contents(i) { }
		remove_token(typename list_t::iterator&& i) : m_contents(std::move(i)) { }

		remove_token(const typename list_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(typename list_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		remove_token(const typename list_t::remove_token& rt) : m_contents(rt) { }
		remove_token(typename list_t::remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token(const typename list_t::volatile_remove_token& rt) : m_contents(rt) { }
		remove_token(typename list_t::volatile_remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token& operator=(const typename list_t::iterator& i) { m_contents = i; return *this; }

		remove_token& operator=(const typename list_t::volatile_iterator& i) { m_contents = i; return *this; }

		remove_token& operator=(const typename list_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename list_t::remove_token&& rt) { m_contents = std::move(rt); return *this; }

		remove_token& operator=(const typename list_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename list_t::volatile_remove_token&& rt) { m_contents = std::move(rt); return *this; }

	public:
		remove_token() { }
		remove_token(const iterator& i) : m_contents(i.m_contents) { }
		remove_token(const remove_token& rt) : m_contents(rt.m_contents) { }
		remove_token(const volatile iterator& i) : m_contents(i.m_contents) { }
		remove_token(const volatile remove_token& rt) : m_contents(rt.m_contents) { }

		remove_token(remove_token&& rt) : m_contents(std::move(rt.m_contents)) { }

		remove_token& operator=(const iterator& i) { m_contents = i.m_contents; return *this; }
		remove_token& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(const volatile iterator& i) { m_contents = i.m_contents; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(remove_token&& rt) { m_contents = std::move(rt.m_contents); return *this; }

		volatile remove_token& operator=(const iterator& i) volatile { m_contents = i.m_contents; return *this; }
		volatile remove_token& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }
		volatile remove_token& operator=(remove_token&& rt) volatile { m_contents = std::move(rt.m_contents); return *this; }

		bool is_active() const { iterator itor(*this); return itor.is_active(); }
		bool is_active() const volatile { iterator itor(*this); return itor.is_active(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const iterator& i) const { return m_contents == i.m_contents; }
		bool operator==(const volatile iterator& i) const { return m_contents == i.m_contents; }
		bool operator==(const iterator& i) const volatile { return m_contents == i.m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const volatile { return !operator==(i); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) { return remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) volatile { return remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3>&& is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> >&& is_element_reference_type_v<T3>&& is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	weak_rcptr_list()
	{ }

	explicit weak_rcptr_list(this_t&& src)
		: m_list(std::move(src.m_list))
	{ }

	bool drain() volatile
	{
		bool foundAny = false;
		for (;;)
		{
			auto itor = m_list.remove_last();
			if (!itor)
				break;
			if (!itor->m_removeToken) // non rc obj
				foundAny = true;
			else
			{
				if (itor->m_obj.get_desc()->uninstall_released_handler(itor->m_removeToken))
					foundAny |= !itor->m_obj.is_released();
			}
		}

		return foundAny;
	}

	bool is_empty() const volatile { return m_list.is_empty(); }
	bool operator!() const volatile { return m_list.is_empty(); }

	iterator get_first() const volatile
	{
		iterator result;
		for (;;)
		{
			result = m_list.get_first();
			if (!result.is_removed())
				break;
			const_cast<this_t*>(this)->remove(result);
		}
		return result;
	}

	iterator get_last() const volatile
	{
		iterator result;
		for (;;)
		{
			result = m_list.get_last();
			if (!result.is_removed())
				break;
			const_cast<this_t*>(this)->remove(result);
		}
		return result;
	}

	struct insert_result
	{
		iterator inserted;
		bool wasEmpty;
	};

	insert_result prepend(const rcref<T>& t) volatile
	{
		iterator inserted;
		auto p = m_list.prepend_via([&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			inserted = std::move(i);
		});
		return { std::move(inserted), p.wasEmpty };
	}

	insert_result prepend_if_not_empty(const rcref<T>& t) volatile
	{
		iterator i2;
		auto p = m_list.prepend_via_if_not_empty([&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			i2 = std::move(i);
		});
		return { std::move(i2), p.wasEmpty };
	}

	insert_result append(const rcref<T>& t) volatile
	{
		iterator i2;
		auto p = m_list.append_via([&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			i2 = std::move(i);
		});
		return { std::move(i2), p.wasEmpty };
	}

	insert_result append_if_not_empty(const rcref<T>& t) volatile
	{
		iterator i2;
		auto p = m_list.append_via_if_not_empty([&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			i2 = std::move(i);
		});
		return { std::move(i2), p.wasEmpty };
	}

	insert_result insert_if_empty(const rcref<T>& t) volatile
	{
		iterator i2;
		auto p = m_list.insert_via_if_empty([&](typename list_t::iterator& i)
			{
				rc_obj_base* desc = t.get_desc();
				rc_obj_base::released_handler_remove_token rt;
				if (!!desc)
				{
					rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
					{
						rcptr<volatile this_t> r2 = r;
						if (!!r2)
							r2->m_list.remove(rt2);
					});
				}
				new (i.get()) node(t, std::move(rt));
				i2 = std::move(i);
			});
		return { std::move(i2), p.wasEmpty };
	}

	iterator insert_before(const iterator& insertBefore, const rcref<T>& t) volatile
	{
		iterator i2;
		m_list.insert_before_via(insertBefore.m_contents, [&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			i2 = std::move(i);
		});
		return std::move(i2);
	}

	iterator insert_after(const iterator& insertAfter, const rcref<T>& t) volatile
	{
		iterator i2;
		m_list.insert_after_via(insertAfter.m_contents, [&](typename list_t::iterator& i)
		{
			rc_obj_base* desc = t.get_desc();
			rc_obj_base::released_handler_remove_token rt;
			if (!!desc)
			{
				rt = desc->on_released([r{ this_weak_rcptr }, rt2{ typename list_t::volatile_remove_token(i) }](rc_obj_base&)
				{
					rcptr<volatile this_t> r2 = r;
					if (!!r2)
						r2->m_list.remove(rt2);
				});
			}
			new (i.get()) node(t, std::move(rt));
			i2 = std::move(i);
		});
		return std::move(i2);
	}

	typedef typename list_t::volatile_remove_result remove_result;

	remove_result remove(const iterator& i) volatile
	{
		remove_result result{ false, false };
		if (!!i)
		{
			result = m_list.remove(i.m_contents);
			if (result.wasRemoved)
			{
				rc_obj_base* desc = i.m_contents->m_obj.get_desc();
				if (!!desc)
					desc->uninstall_released_handler(i.m_contents->m_removeToken);
			}
		}
		return result;
	}

	remove_result remove(const remove_token& rt) volatile { return remove(iterator(rt)); }

	remove_result operator-=(const iterator& i) volatile { return remove(i); }
	remove_result operator-=(const remove_token& rt) volatile { return remove(rt); }

	iterator begin() const volatile { return get_first(); }
	iterator rbegin() const volatile { return get_last(); }
	iterator end() const volatile { iterator i; return i; }
	iterator rend() const volatile { iterator i; return i; }
};


}


#endif
