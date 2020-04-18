//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_MULTIMAP
#define COGS_HEADER_COLLECTION_MULTIMAP

#include <map>
#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/collections/avltree.hpp"
#include "cogs/collections/container_skiplist.hpp"
#include "cogs/collections/rbtree.hpp"


namespace cogs {


// if thread safety is required, a container_skiplist is used.
// if thread safety is not needed, and favor_lookup is false, a rbtree is used.
// if thread safety is not needed, and favor_lookup is true, an avltree is used.


/// @ingroup LockFreeCollections
/// @brief A sorted O(log n) collection mapping keys to values.  Duplicate keys are allowed.
/// @tparam key_t The type used to compare elements.
/// @tparam value_t The value associated with the element.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
template <typename key_t, typename value_t, class comparator_t = default_comparator, class allocator_type = default_allocator>
class multimap
{
public:
	struct node
	{
		const key_t key;
		value_t value;
	};

private:
	typedef multimap<key_t, value_t, comparator_t, allocator_type> this_t;

	struct payload
	{
		delayed_construction<node> m_node;
		const key_t& get_key() const { return m_node->key; }
	};

	typedef container_skiplist<true, key_t, payload, comparator_t, allocator_type> container_skiplist_t;
	container_skiplist_t m_contents;

	multimap(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A multimap element iterator
	class iterator
	{
	private:
		friend class multimap;
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

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		node& operator*() const { return *m_contents->m_node; }
		node* operator->() const { return &*m_contents->m_node; }

		const rcptr<const key_t>& get_key_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node->key, m_contents.get_desc());
			return storage;
		}

		const rcptr<value_t>& get_value_obj(unowned_t<rcptr<value_t> >& storage = unowned_t<rcptr<value_t> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node->value, m_contents.get_desc());
			return storage;
		}

		const rcptr<node>& get_obj(unowned_t<rcptr<node> >& storage = unowned_t<rcptr<node> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node, m_contents.get_desc());
			return storage;
		}

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

	/// @brief A volatile multimap element iterator
	class volatile_iterator
	{
	private:
		friend class multimap;
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

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		node& operator*() const { return *m_contents->m_node; }
		node* operator->() const { return &*m_contents->m_node; }

		const rcptr<const key_t>& get_key_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node->key, m_contents.get_desc());
			return storage;
		}

		const rcptr<value_t>& get_value_obj(unowned_t<rcptr<value_t> >& storage = unowned_t<rcptr<value_t> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node->value, m_contents.get_desc());
			return storage;
		}

		const rcptr<node>& get_obj(unowned_t<rcptr<node> >& storage = unowned_t<rcptr<node> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->m_node, m_contents.get_desc());
			return storage;
		}

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

	/// @brief A multimap element remove token
	class remove_token
	{
	private:
		friend class multimap;
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

	/// @brief A multimap element volatile remove token
	class volatile_remove_token
	{
	private:
		friend class multimap;
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


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> && !std::is_const_v<T2> > >
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

	multimap() { }

	multimap(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	explicit multimap(volatile allocator_type& al) : m_contents(al) { }

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

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_via(F&& f)
	{
		iterator i;
		m_contents.insert_multi_via([&](typename container_skiplist_t::iterator& i2)
		{
			new (i2.get()) payload;	// should be no-op, but for completeness.
			i = std::move(i2);
			f(i);
		});
		return i;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>,
		iterator>
	insert_via(F&& f) { return insert_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& std::is_invocable_v<F, key_t&, const rcref<value_t>&>,
		iterator>
	insert_via(F&& f) { return insert_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& std::is_invocable_v<F, const rcref<key_t>&, value_t&>,
		iterator>
	insert_via(F&& f) { return insert_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i->value); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, value_t&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		iterator>
	insert_via(F&& f) { return insert_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }

	struct volatile_insert_result
	{
		volatile_iterator inserted;
		bool wasEmpty;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_result>
	insert_via(F&& f) volatile
	{
		iterator inserted;
		auto p = m_contents.insert_multi_via([&](typename container_skiplist_t::iterator& i)
		{
			new (i.get()) payload;	// should be no-op, but for completeness.
			inserted = std::move(i);
			f(inserted);
		});
		return { std::move(inserted), p.wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>,
		volatile_insert_result>
	insert_via(F&& f) volatile { return insert_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& std::is_invocable_v<F, key_t&, const rcref<value_t>&>,
		volatile_insert_result>
	insert_via(F&& f) volatile { return insert_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& std::is_invocable_v<F, const rcref<key_t>&, value_t&>,
		volatile_insert_result>
	insert_via(F&& f) volatile { return insert_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i->value); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, value_t&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		volatile_insert_result>
	insert_via(F&& f) volatile { return insert_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }


	/// @{
	/// @brief Inserts an element
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @return An iterator to the newly inserted element
	iterator insert(const key_t& k, const value_t& v)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	iterator insert(key_t&& k, const value_t& v)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	iterator insert(const key_t& k, value_t&& v)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}

	iterator insert(key_t&& k, value_t&& v)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}


	volatile_insert_result insert(const key_t& k, const value_t& v) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	volatile_insert_result insert(key_t&& k, const value_t& v) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	volatile_insert_result insert(const key_t& k, value_t&& v) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}

	volatile_insert_result insert(key_t&& k, value_t&& v) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}


	template <typename... args_t>
	iterator insert_emplace(const key_t& k, args_t&&... args)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	volatile_insert_result insert_emplace(const key_t& k, args_t&&... args) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			placement_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	iterator insert_emplace(key_t&& k, args_t&&... args)
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	volatile_insert_result insert_emplace(key_t&& k, args_t&&... args) volatile
	{
		return insert_via([&](iterator& i)
		{
			placement_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			placement_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	/// @{
	/// @brief Removes an element
	/// @param e Element to remove
	/// @return True if the remove was successful, false if the element was not removed in parallel by another thread.
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
	/// @brief Find any element with a matching key value
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_any_equal(const key_t& criteria) const { return iterator(m_contents.find_any_equal(criteria)); }
	/// @brief Thread-safe implementation of find_any_equal()
	volatile_iterator find_any_equal(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal(criteria)); }
	/// @}

	/// @{
	/// @brief Find the first element with a matching key value
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_first_equal(const key_t& criteria) const { return iterator(m_contents.find_first_equal(criteria)); }
	/// @brief Thread-safe implementation of find_first_equal()
	volatile_iterator find_first_equal(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_first_equal(criteria)); }
	/// @}

	/// @{
	/// @brief Find the last element with a matching key value
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_last_equal(const key_t& criteria) const { return iterator(m_contents.find_last_equal(criteria)); }
	/// @brief Thread-safe implementation of find_last_equal()
	volatile_iterator find_last_equal(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_last_equal(criteria)); }
	/// @}

	/// @{
	/// @brief Find element with the nearest key value lesser than specified.
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_nearest_less_than(const key_t& criteria) const { return iterator(m_contents.find_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_nearest_less_than()
	volatile_iterator find_nearest_less_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find element with the nearest key value greater than specified.
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_nearest_greater_than(const key_t& criteria) const { return iterator(m_contents.find_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_nearest_greater_than()
	volatile_iterator find_nearest_greater_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_nearest_greater_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find any element equal to or the nearest element with a value lesser than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_any_equal_or_nearest_less_than()
	volatile_iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find any element equal to or the nearest element with a value greater than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_any_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_any_equal_or_nearest_greater_than()
	volatile_iterator find_any_equal_or_nearest_greater_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find the first element equal to or the nearest element with a value lesser than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_contents.find_first_equal_or_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_first_equal_or_nearest_less_than()
	volatile_iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_first_equal_or_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find the first element equal to or the nearest element with a value greater than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_contents.find_first_equal_or_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_first_equal_or_nearest_greater_than()
	volatile_iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_first_equal_or_nearest_greater_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find the last element equal to or the nearest element with a value lesser than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_contents.find_last_equal_or_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_last_equal_or_nearest_less_than()
	volatile_iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_last_equal_or_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find the last element equal to or the nearest element with a value greater than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_contents.find_last_equal_or_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_last_equal_or_nearest_greater_than()
	volatile_iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_last_equal_or_nearest_greater_than(criteria)); }
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


template <typename key_t, typename value_t, bool favor_lookup = false, class comparator_t = default_comparator, class allocator_type = default_allocator>
class nonvolatile_multimap
{
public:
	struct node
	{
		const key_t key;
		value_t value;
	};

private:
	typedef nonvolatile_multimap<key_t, value_t, favor_lookup, comparator_t, allocator_type> this_t;

	class payload : public std::conditional_t<favor_lookup, avltree_node_t<payload>, rbtree_node_t<payload> >
	{
	public:
		delayed_construction<node> m_node;
		const key_t& get_key() const { return m_node->key; }
	};

	typedef std::conditional_t<favor_lookup, avltree<key_t, payload, comparator_t>, rbtree<key_t, payload, comparator_t> > tree_t;

	typedef typename tree_t::ref_t ref_t;

	nonvolatile_multimap(const this_t&) = delete;
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
			m_allocator.template destruct_deallocate_type<payload>(n);
			n = n2;
		}
	}

public:
	class iterator
	{
	private:
		friend class nonvolatile_multimap;

		const tree_t* m_tree;
		ref_t m_payload;

		iterator(const ref_t& n, const tree_t& t)
			: m_tree(&t),
			m_payload(n)
		{ }

	public:
		iterator() { }

		iterator(const iterator& i)
			: m_tree(i.m_tree),
			m_payload(i.m_payload)
		{ }

		iterator(iterator&& i)
			: m_tree(i.m_tree),
			m_payload(std::move(i.m_payload))
		{ }

		void release() { m_payload = 0; }

		iterator& operator++() { if (!!m_payload) m_payload = m_tree->get_next(m_payload); return *this; }
		iterator& operator--() { if (!!m_payload) m_payload = m_tree->get_prev(m_payload); return *this; }
		iterator& operator++(int) { iterator tmp(*this); ++* this; return tmp; }
		iterator& operator--(int) { iterator tmp(*this); --* this; return tmp; }

		bool operator!() const { return !m_payload; }

		bool operator==(const iterator& i) const { return m_payload == i.m_payload; }
		bool operator!=(const iterator& i) const { return m_payload != i.m_payload; }

		iterator& operator=(const iterator& i) { m_payload = i.m_payload; m_tree = i.m_tree; return *this; }

		const key_t& get_key() const { return m_payload->m_node->key; }
		value_t& get_value() const { return m_payload->m_node->value; }

		node& operator*() const { return *m_payload->m_node; }
		node* operator->() const { return &*m_payload->m_node; }

		iterator next() const
		{
			iterator result;
			if (!!m_payload)
			{
				result.m_payload = m_tree->get_next(m_payload);
				result.m_tree = m_tree;
			}
			return result;
		}

		iterator prev() const
		{
			iterator result;
			if (!!m_payload)
			{
				result.m_payload = m_tree->get_prev(m_payload);
				result.m_tree = m_tree;
			}
			return result;
		}

		void assign_next()
		{
			if (!!m_payload)
				m_payload = m_tree->get_next(m_payload);
		}

		void assign_prev()
		{
			if (!!m_payload)
				m_payload = m_tree->get_prev(m_payload);
		}
	};

	nonvolatile_multimap()
		: m_count(0)
	{ }

	nonvolatile_multimap(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_tree(std::move(src.m_tree)),
		m_count(src.m_count)
	{ }

	explicit nonvolatile_multimap(volatile allocator_type& al)
		: m_count(0),
		m_allocator(al)
	{ }

	~nonvolatile_multimap()
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


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		iterator>
	insert_via(F&& f)
	{
		ref_t r = m_allocator.template allocate_type<payload>();
		new (r.get_ptr()) payload;
		iterator i(r, m_tree);
		f(i);
		m_tree.insert_multi(r);
		m_count++;
		return i;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		iterator>
	insert_via(F&& f) { return insert_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }

	iterator insert(const key_t& k, const value_t& v) { return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(v); }); }
	iterator insert(key_t&& k, const value_t& v) { return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(v); }); }
	iterator insert(const key_t& k, value_t&& v) { return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::move(v)); }); }
	iterator insert(key_t&& k, value_t&& v) { return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::move(v)); }); }

	template <typename... args_t>
	iterator insert_emplace(const key_t& k, args_t&&... args)
	{
		return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}

	template <typename... args_t>
	iterator insert_emplace(key_t&& k, args_t&&... args)
	{
		return insert_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}


	void remove(const iterator& i)
	{
		m_tree.remove(i.m_payload);
		m_allocator.template destruct_deallocate_type<payload>(i.m_payload);
		m_count--;
	}

	iterator find_any_equal(const key_t& criteria) const { return iterator(m_tree.find_any_equal(criteria), m_tree); }
	iterator find_first_equal(const key_t& criteria) const { return iterator(m_tree.find_first_equal(criteria), m_tree); }
	iterator find_last_equal(const key_t& criteria) const { return iterator(m_tree.find_last_equal(criteria), m_tree); }
	iterator find_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_nearest_less_than(criteria), m_tree); }
	iterator find_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_nearest_greater_than(criteria), m_tree); }
	iterator find_any_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_less_than(criteria), m_tree); }
	iterator find_any_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_greater_than(criteria), m_tree); }
	iterator find_first_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_first_equal_or_nearest_less_than(criteria), m_tree); }
	iterator find_first_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_first_equal_or_nearest_greater_than(criteria), m_tree); }
	iterator find_last_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_last_equal_or_nearest_less_than(criteria), m_tree); }
	iterator find_last_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_last_equal_or_nearest_greater_than(criteria), m_tree); }

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
