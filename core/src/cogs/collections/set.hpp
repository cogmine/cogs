//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_SET
#define COGS_HEADER_COLLECTION_SET

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/collections/avltree.hpp"
#include "cogs/collections/container_skiplist.hpp"
#include "cogs/collections/rbtree.hpp"


namespace cogs {


#include <type_traits>


/// @ingroup LockFreeCollections
/// @brief A sorted O(log n) collection.  Unique values are enforced.
/// @tparam T The type to contain
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
template <typename T, class comparator_t = default_comparator, class allocator_type = default_allocator>
class set
{
public:
	typedef T type;

private:
	typedef set<type, comparator_t, allocator_type> this_t;

	class payload
	{
	private:
		delayed_construction<type> m_value;

	public:
		type& get_key() { return m_value.get(); }
		const type& get_key() const { return m_value.get(); }
	};

	typedef container_skiplist<false, type, payload, comparator_t, allocator_type> container_skiplist_t;
	container_skiplist_t m_contents;

	set(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A set element iterator
	class iterator
	{
	private:
		friend class set;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		typename container_skiplist_t::iterator m_contents;

		iterator(const typename container_skiplist_t::iterator& i) : m_contents(i) { }
		iterator(const volatile typename container_skiplist_t::iterator& i) : m_contents(i) { }
		iterator(typename container_skiplist_t::iterator&& i) : m_contents(std::move(i)) { }

		iterator(const typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		iterator(const volatile typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		iterator(typename container_skiplist_t::remove_token&& i) : m_contents(std::move(i)) { }

		iterator(const typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		iterator(const volatile typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		iterator(typename container_skiplist_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		iterator(const typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		iterator(const volatile typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		iterator(typename container_skiplist_t::volatile_remove_token&& i) : m_contents(std::move(i)) { }

	public:
		iterator() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator(T2&& i) : iterator(forward_member<T2>(i.m_contents)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator& operator=(T2&& i) { m_contents = forward_member<T2>(i.m_contents); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile iterator& operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool is_removed() const { return m_contents.is_removed(); }
		bool is_removed() const volatile { return m_contents.is_removed(); }

		iterator& operator++() { ++m_contents; return *this; }
		iterator operator++() volatile { return iterator(++m_contents); }
		iterator operator++(int) { return iterator(m_contents++); }
		iterator operator++(int) volatile { return iterator(m_contents++); }

		iterator& operator--() { --m_contents; return *this; }
		iterator operator--() volatile { return iterator(--m_contents); }
		iterator operator--(int) { return iterator(m_contents--); }
		iterator operator--(int) volatile { return iterator(m_contents--); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_contents != i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_contents != i.m_contents; }

		type& operator*() const { return m_contents.get()->get_key(); }
		type* operator->() const { return &(m_contents.get()->get_key()); }

		const rcptr<const type>& get_obj(unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->get_key(), m_contents.get_desc());
			return storage;
		}

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		iterator next() const { return iterator(m_contents.next()); }
		iterator next() const volatile { return iterator(m_contents.next()); }

		void assign_next() { m_contents.assign_next(); }
		void assign_next() volatile { m_contents.assign_next(); }

		iterator prev() const { return iterator(m_contents.prev()); }
		iterator prev() const volatile { return iterator(m_contents.prev()); }

		void assign_prev() { m_contents.assign_prev(); }
		void assign_prev() volatile { m_contents.assign_prev(); }

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


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	/// @brief A volatile set element iterator
	class volatile_iterator
	{
	private:
		friend class set;
		friend class iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		typename container_skiplist_t::volatile_iterator m_contents;

		volatile_iterator(const typename container_skiplist_t::iterator& i) : m_contents(i) { }
		volatile_iterator(const volatile typename container_skiplist_t::iterator& i) : m_contents(i) { }
		volatile_iterator(typename container_skiplist_t::iterator&& i) : m_contents(std::move(i)) { }

		volatile_iterator(const typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		volatile_iterator(const volatile typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		volatile_iterator(typename container_skiplist_t::remove_token&& i) : m_contents(std::move(i)) { }

		volatile_iterator(const typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		volatile_iterator(const volatile typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		volatile_iterator(typename container_skiplist_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		volatile_iterator(const typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		volatile_iterator(const volatile typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		volatile_iterator(typename container_skiplist_t::volatile_remove_token&& i) : m_contents(std::move(i)) { }

	public:
		volatile_iterator() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator(T2&& i) : volatile_iterator(forward_member<T2>(i.m_contents)) { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator& operator=(T2&& i) { m_contents = forward_member<T2>(i.m_contents); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile volatile_iterator& operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool is_removed() const { return m_contents.is_removed(); }
		bool is_removed() const volatile { return m_contents.is_removed(); }

		volatile_iterator& operator++() { ++m_contents; return *this; }
		volatile_iterator operator++() volatile { return volatile_iterator(++m_contents); }
		volatile_iterator operator++(int) { return volatile_iterator(m_contents++); }
		volatile_iterator operator++(int) volatile { return volatile_iterator(m_contents++); }

		volatile_iterator& operator--() { --m_contents; return *this; }
		volatile_iterator operator--() volatile { return volatile_iterator(--m_contents); }
		volatile_iterator operator--(int) { return volatile_iterator(m_contents--); }
		volatile_iterator operator--(int) volatile { return volatile_iterator(m_contents--); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_contents != i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_contents != i.m_contents; }

		type& operator*() const { return m_contents->get_key(); }
		type* operator->() const { return &(m_contents->get_key()); }

		const rcptr<const type>& get_obj(unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->get_key(), m_contents.get_desc());
			return storage;
		}

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		volatile_iterator next() const { return volatile_iterator(m_contents.next()); }
		volatile_iterator next() const volatile { return volatile_iterator(m_contents.next()); }

		void assign_next() { m_contents.assign_next(); }
		void assign_next() volatile { m_contents.assign_next(); }

		volatile_iterator prev() const { return volatile_iterator(m_contents.prev()); }
		volatile_iterator prev() const volatile { return volatile_iterator(m_contents.prev()); }

		void assign_prev() { m_contents.assign_prev(); }
		void assign_prev() volatile { m_contents.assign_prev(); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) { return volatile_iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) volatile { return volatile_iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	/// @brief A set element remove token
	class remove_token
	{
	private:
		friend class set;
		friend class iterator;
		friend class volatile_iterator;
		friend class volatile_remove_token;

		typename container_skiplist_t::remove_token m_contents;

		remove_token(const typename container_skiplist_t::iterator& i) : m_contents(i) { }
		remove_token(const volatile typename container_skiplist_t::iterator& i) : m_contents(i) { }
		remove_token(typename container_skiplist_t::iterator&& i) : m_contents(std::move(i)) { }

		remove_token(const typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		remove_token(const volatile typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		remove_token(typename container_skiplist_t::remove_token&& i) : m_contents(std::move(i)) { }

		remove_token(const typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(const volatile typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(typename container_skiplist_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		remove_token(const typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		remove_token(const volatile typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		remove_token(typename container_skiplist_t::volatile_remove_token&& i) : m_contents(std::move(i)) { }

	public:
		remove_token() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token(T2&& i) : remove_token(forward_member<T2>(i.m_contents)) { }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token& operator=(T2&& i) { m_contents = forward_member<T2>(i.m_contents); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile remove_token& operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_contents != i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_contents != i.m_contents; }


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


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	/// @brief A set element volatile remove token
	class volatile_remove_token
	{
	private:
		friend class set;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;

		typename container_skiplist_t::volatile_remove_token m_contents;

		volatile_remove_token(const typename container_skiplist_t::iterator& i) : m_contents(i) { }
		volatile_remove_token(const volatile typename container_skiplist_t::iterator& i) : m_contents(i) { }
		volatile_remove_token(typename container_skiplist_t::iterator&& i) : m_contents(std::move(i)) { }

		volatile_remove_token(const typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		volatile_remove_token(const volatile typename container_skiplist_t::remove_token& i) : m_contents(i) { }
		volatile_remove_token(typename container_skiplist_t::remove_token&& i) : m_contents(std::move(i)) { }

		volatile_remove_token(const typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		volatile_remove_token(const volatile typename container_skiplist_t::volatile_iterator& i) : m_contents(i) { }
		volatile_remove_token(typename container_skiplist_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		volatile_remove_token(const typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		volatile_remove_token(const volatile typename container_skiplist_t::volatile_remove_token& i) : m_contents(i) { }
		volatile_remove_token(typename container_skiplist_t::volatile_remove_token&& i) : m_contents(std::move(i)) { }

	public:
		volatile_remove_token() { }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token(T2&& i) : volatile_remove_token(forward_member<T2>(i.m_contents)) { }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token& operator=(T2&& i) { m_contents = forward_member<T2>(i.m_contents); return *this; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile volatile_remove_token& operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); return *this; }


		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_contents == i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_contents != i.m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_contents != i.m_contents; }


		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_remove_token_type_v<T2> && !std::is_const_v<T2> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) { return volatile_remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) volatile { return volatile_remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	set() { }

	set(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	explicit set(volatile allocator_type& al)
		: m_contents(al)
	{ }

	set(const std::initializer_list<type>& src)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	set(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_contents(al)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	volatile this_t& operator=(this_t&& src) volatile
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	this_t& operator=(const std::initializer_list<type>& src)
	{
		this_t tmp(src);
		*this = std::move(tmp);
		return *this;
	}

	volatile this_t& operator=(const std::initializer_list<type>& src) volatile
	{
		this_t tmp(src);
		*this = std::move(tmp);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.insert_replace(std::forward<args_t>(args)), ...);
		return result;
	}

	/// @{
	/// @brief Removes all elements.
	void clear() { m_contents.clear(); }
	/// @}

	/// @{
	/// @brief Drains all elements.  Unlike clear(), drain() does not imply the operation can be done atomically with respect to all elements.
	bool drain() { bool foundAny = !m_contents.is_empty(); m_contents.clear(); return foundAny; }
	/// @brief Thread-safe implementation of drain()
	bool drain() volatile { return m_contents.drain(); }
	/// @}

	/// @{
	/// @brief Gets the number of elements in the collection
	/// @return The number of elements in the collection
	size_t size() const { return m_contents.size(); }
	/// @brief Thread-safe implementation of size()
	size_t size() const volatile { return m_contents.size(); }
	/// @}

	/// @{
	/// @brief Tests if the collection is empty
	/// @return True if the collection is empty
	bool is_empty() const { return m_contents.is_empty(); }
	/// @brief Thread-safe implementation of size()
	bool is_empty() const volatile { return m_contents.is_empty(); }
	/// @}

	/// @{
	/// @brief Tests if the collection is empty.  An alias for is_empty().
	/// @return True if the collection is empty
	bool operator!() const { return is_empty(); }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return is_empty(); }
	/// @}

	/// @{
	/// @brief Gets an iterator to the first element.
	/// @return An iterator to the first element
	iterator get_first() const { return iterator(m_contents.get_first()); }
	/// @brief Thread-safe implementation of get_first()
	volatile_iterator get_first() const volatile { return volatile_iterator(m_contents.get_first()); }
	/// @}

	/// @{
	/// @brief Gets an iterator to the last element.
	/// @return An iterator to the last element
	iterator get_last() const { return iterator(m_contents.get_last()); }
	/// @brief Thread-safe implementation of get_last()
	volatile_iterator get_last() const volatile { return volatile_iterator(m_contents.get_last()); }
	/// @}

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
		iterator i;
		auto p = m_contents.insert_replace_via([&](typename container_skiplist_t::iterator& i2)
		{
			new (i2.get()) payload;	// should be no-op, but for completeness.
			i = std::move(i2);
			f(i);
		});
		return { std::move(p.replaced), std::move(i) };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(i.get_obj().dereference().const_cast_to<type>()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(*const_cast<type*>(&i.get_key())); }); }


	insert_replace_result insert_replace(const type& k)
	{
		return insert_replace_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(k);
		});
	}

	insert_replace_result insert_replace(type&& k)
	{
		return insert_replace_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::move(k));
		});
	}


	template <typename... args_t>
	insert_replace_result insert_replace_emplace(args_t&&... args)
	{
		return insert_replace_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	struct insert_unique_result
	{
		iterator iterator;
		bool hadCollision;
	};

	// The returned iterator is the new element, or the element collided with.
	// The returned bool indicates whether or not a collision occured.
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_unique_result>
	insert_unique_via(F&& f)
	{
		iterator i;
		auto p = m_contents.insert_unique_via([&](typename container_skiplist_t::iterator& i2)
		{
			new (i2.get()) payload;	// should be no-op, but for completeness.
			i = std::move(i2);
			f(i);
		});
		return { std::move(i), p.hadCollision };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(i.get_obj().dereference().const_cast_to<type>()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(*const_cast<type*>(&*i)); }); }


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
		iterator i;
		auto p = m_contents.insert_unique_via([&](typename container_skiplist_t::iterator& i2)
		{
			new (i2.get()) payload;	// should be no-op, but for completeness.
			i = std::move(i2);
			f(i);
		});
		return { std::move(i), p.hadCollision, p.wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<type>&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(i.get_obj().dereference().const_cast_to<type>()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<type>&>
		&& std::is_invocable_v<F, type&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(*const_cast<type*>(&*i)); }); }


	insert_unique_result insert_unique(const type& k)
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(k);
		});
	}

	insert_unique_result insert_unique(type&& k)
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::move(k));
		});
	}


	volatile_insert_unique_result insert_unique(const type& k) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(k);
		});
	}

	volatile_insert_unique_result insert_unique(type&& k) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::move(k));
		});
	}


	template <typename... args_t>
	insert_unique_result insert_unique_emplace(args_t&&... args)
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	volatile_insert_unique_result insert_unique_emplace(args_t&&... args) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			placement_rcnew(&i.m_contents->get_key(), *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	/// @{
	/// @brief Removes an element
	/// @param e Element to remove
	/// @return True if the remove was successful, false if the element has already been removed
	bool remove(const remove_token& e) { return m_contents.remove(e.m_contents); }
	bool remove(const iterator& e) { return m_contents.remove(e.m_contents); }
	/// @}

	typedef typename container_skiplist_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove(const volatile_remove_token& e) volatile { return m_contents.remove(e.m_contents); }
	volatile_remove_result remove(const volatile_iterator& e) volatile { return m_contents.remove(e.m_contents); }

	iterator pop_first() { return iterator(m_contents.pop_first()); }
	iterator pop_last() { return iterator(m_contents.pop_last()); }

	struct pop_result
	{
		volatile_iterator iterator;
		bool wasEmptied;
	};

	pop_result pop_first() volatile
	{
		auto p = m_contents.pop_first();
		volatile_iterator i(std::move(p.iterator));
		return { std::move(i), p.wasEmptied };
	}

	pop_result pop_last() volatile
	{
		auto p = m_contents.pop_last();
		volatile_iterator i(std::move(p.iterator));
		return { std::move(i), p.wasEmptied };
	}

	/// @{
	/// @brief Find an element with the matching value
	/// @param criteria The value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find(const type& criteria) const { return iterator(m_contents.find_any_equal(criteria)); }
	/// @brief Thread-safe implementation of find()
	volatile_iterator find(const type& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal(criteria)); }
	/// @}

	/// @{
	/// @brief Find element with the nearest value lesser than specified.
	/// @param criteria The value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_nearest_less_than(const type& criteria) const { return iterator(m_contents.find_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_nearest_less_than()
	volatile_iterator find_nearest_less_than(const type& criteria) const volatile { return volatile_iterator(m_contents.find_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find element with the nearest value greater than specified.
	/// @param criteria The value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_nearest_greater_than(const type& criteria) const { return iterator(m_contents.find_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_nearest_greater_than()
	volatile_iterator find_nearest_greater_than(const type& criteria) const volatile { return volatile_iterator(m_contents.find_nearest_greater_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find an element equal to or the nearest element with a value lesser than specified
	/// @param criteria The value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_equal_or_nearest_less_than(const type& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_equal_or_nearest_less_than()
	volatile_iterator find_equal_or_nearest_less_than(const type& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find an element equal to or the nearest element with a value greater than specified
	/// @param criteria The value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_equal_or_nearest_greater_than(const type& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_equal_or_nearest_greater_than()
	volatile_iterator find_equal_or_nearest_greater_than(const type& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @}

	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }

	template <typename enable = std::enable_if_t<allocator_type::is_static> >
	void swap(volatile this_t& wth) { m_contents.swap(wth.m_contents); }

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


template <typename T, bool favor_lookup = false, class comparator_t = default_comparator, class allocator_type = default_allocator>
class nonvolatile_set
{
public:
	typedef T type;

private:
	typedef nonvolatile_set<type, favor_lookup, comparator_t, allocator_type> this_t;

	class node : public std::conditional_t<favor_lookup, avltree_node_t<node>, rbtree_node_t<node> >
	{
	private:
		delayed_construction<type> m_value;

	public:
		type& get_key() { return m_value.get(); }
		const type& get_key() const { return m_value.get(); }
	};

	typedef std::conditional_t<favor_lookup, avltree<type, node, comparator_t>, rbtree<type, node, comparator_t> > tree_t;

	typedef typename tree_t::ref_t ref_t;

	nonvolatile_set(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	tree_t m_tree;
	size_t m_count;
	allocator_container<allocator_type> m_allocator;

	void clear_inner()
	{
		ref_t n = m_tree.get_first_postorder();
		while (!!n)
		{
			ref_t n2 = m_tree.get_next_postorder(n);
			m_allocator.template destruct_deallocate_type<node>(n);
			n = n2;
		}
	}

public:
	class iterator
	{
	private:
		friend class nonvolatile_set;

		const tree_t* m_tree;
		ref_t m_node;

		iterator(const ref_t& n, const tree_t& t)
			: m_tree(&t),
			m_node(n)
		{ }

	public:
		iterator() { }

		iterator(const iterator& i)
			: m_tree(i.m_tree),
			m_node(i.m_node)
		{ }

		iterator(iterator&& i)
			: m_tree(i.m_tree),
			m_node(std::move(i.m_node))
		{ }

		void release() { m_node = 0; }

		iterator& operator++() { if (!!m_node) m_node = m_tree->get_next(m_node); return *this; }
		iterator& operator--() { if (!!m_node) m_node = m_tree->get_prev(m_node); return *this; }
		iterator& operator++(int) { iterator tmp(*this); ++*this; return tmp; }
		iterator& operator--(int) { iterator tmp(*this); --*this; return tmp; }

		bool operator!() const { return !m_node; }

		bool operator==(const iterator& i) const { return m_node == i.m_node; }
		bool operator!=(const iterator& i) const { return m_node != i.m_node; }

		iterator& operator=(const iterator& i) { m_node = i.m_node; m_tree = i.m_tree; return *this; }
		iterator& operator=(iterator&& i) { m_node = std::move(i.m_node); m_tree = i.m_tree; return *this; }

		const type& operator*() const { return m_node->get_key(); }
		const type* operator->() const { return &(m_node->get_key()); }

		iterator next() const
		{
			iterator result;
			if (!!m_node)
			{
				result.m_node = m_tree->get_next(m_node);
				result.m_tree = m_tree;
			}
			return result;
		}

		iterator prev() const
		{
			iterator result;
			if (!!m_node)
			{
				result.m_node = m_tree->get_prev(m_node);
				result.m_tree = m_tree;
			}
			return result;
		}

		void assign_next()
		{
			if (!!m_node)
				m_node = m_tree->get_next(m_node);
		}

		void assign_prev()
		{
			if (!!m_node)
				m_node = m_tree->get_prev(m_node);
		}
	};

	nonvolatile_set()
		: m_count(0)
	{ }

	nonvolatile_set(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_tree(std::move(src.m_tree)),
		m_count(src.m_count)
	{ }

	explicit nonvolatile_set(volatile allocator_type& al)
		: m_count(0),
		m_allocator(al)
	{ }

	nonvolatile_set(const std::initializer_list<type>& src)
		: m_count(0)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	nonvolatile_set(volatile allocator_type& al, const std::initializer_list<type>& src)
		: m_count(0),
		m_allocator(al)
	{
		for (auto& entry : src)
			insert_replace(entry);
	}

	~nonvolatile_set()
	{
		clear_inner();
	}

	this_t& operator=(this_t&& src)
	{
		m_allocator = std::move(src.m_allocator);
		m_tree = std::move(src.m_tree);
		m_count = src.m_count;
		return *this;
	}

	this_t& operator=(const std::initializer_list<type>& src)
	{
		this_t tmp(src);
		*this = std::move(tmp);
		return *this;
	}

	template <typename... args_t>
	static this_t create(args_t&&... args)
	{
		this_t result;
		(result.insert_replace(std::forward<args_t>(args)), ...);
		return result;
	}

	void clear()
	{
		clear_inner();
		m_tree.clear();
		m_count = 0;
	}

	bool drain() { bool foundAny = !!m_count; clear(); return foundAny; }

	size_t size() const { return m_count; }
	bool is_empty() const { return !m_count; }
	bool operator!() const { return is_empty(); }

	iterator get_first() const { return iterator(m_tree.get_first(), m_tree); }
	iterator get_last() const { return iterator(m_tree.get_last(), m_tree); }

	struct insert_replace_result
	{
		iterator iterator;
		bool wasReplacement;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_replace_result>
	insert_replace_via(F&& f)
	{
		insert_replace_result result;
		ref_t r = m_allocator.template allocate_type<node>();
		new (r.get_ptr()) node();
		result.iterator = iterator(r, m_tree);
		f(result.iterator);
		ref_t existing = m_tree.insert_replace(r);
		result.wasReplacement = !!existing;
		if (result.wasReplacement)
			m_allocator.template destruct_deallocate_type<node>(existing);
		else
			m_count++;
		return result;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, type&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(i.m_node->get_key()); }); }

	insert_replace_result insert_replace(const type& t) { return insert_replace_via([&](iterator& i) { new (&i.m_node->get_key()) type(t); }); }

	insert_replace_result insert_replace(type&& t) { return insert_replace_via([&](iterator& i) { new (&i.m_node->get_key()) type(std::move(t)); }); }

	template <typename... args_t>
	insert_replace_result insert_replace_emplace(args_t&&... args)
	{
		return insert_replace_via([&](iterator& i) { new (&i.m_node->get_key()) type(std::forward<args_t>(args)...); });
	}

	struct insert_unique_result
	{
		iterator iterator;
		bool hadCollision;
	};

	// The returned iterator is the new element, or the element collided with.
	// The returned bool indicates whether or not a collision occured.
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_unique_result>
	insert_unique_via(F&& f)
	{
		insert_unique_result result;
		ref_t r = m_allocator.template allocate_type<node>();
		new (r.get_ptr()) node();
		result.iterator = iterator(r, m_tree);
		f(result.iterator);
		ref_t existing = m_tree.insert_unique(r);
		result.hadCollision = !!existing;
		if (!result.hadCollision)
			m_count++;
		else
		{
			m_allocator.template destruct_deallocate_type<node>(r);
			result.iterator.m_node = std::move(existing);
		}
		return result;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, type&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(i.m_node->get_key()); }); }

	insert_unique_result insert_unique(const type& t) { return insert_unique_via([&](iterator& i) { new (&i.m_node->get_key()) type(t); }); }

	insert_unique_result insert_unique(type&& t) { return insert_unique_via([&](iterator& i) { new (&i.m_node->get_key()) type(std::move(t)); }); }

	template <typename... args_t>
	insert_unique_result insert_unique_emplace(args_t&&... args)
	{
		return insert_unique_via([&](iterator& i) { new (&i.m_node->get_key()) type(std::forward<args_t>(args)...); });
	}

	void remove(const iterator& i)
	{
		m_tree.remove(i.m_node);
		m_allocator.template destruct_deallocate_type<node>(i.m_node);
		m_count--;
	}

	iterator find_equal(const type& criteria) const { return iterator(m_tree.find_any_equal(criteria), m_tree); }
	iterator find_nearest_less_than(const type& criteria) const { return iterator(m_tree.find_nearest_less_than(criteria), m_tree); }
	iterator find_nearest_greater_than(const type& criteria) const { return iterator(m_tree.find_nearest_greater_than(criteria), m_tree); }
	iterator find_equal_or_nearest_less_than(const type& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_less_than(criteria), m_tree); }
	iterator find_equal_or_nearest_greater_than(const type& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_greater_than(criteria), m_tree); }

	void swap(this_t& wth)
	{
		m_tree.swap(wth.m_tree);
		cogs::swap(m_count, wth.m_count);
		m_allocator.swap(wth.m_allocator);
	}

	this_t exchange(this_t&& src)
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

	iterator begin() const { return get_first(); }
	iterator rbegin() const { return get_last(); }
	iterator end() const { iterator i; return i; }
	iterator rend() const { iterator i; return i; }
};


}


#endif
