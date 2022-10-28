//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_COLLECTION_MAP
#define COGS_HEADER_COLLECTION_MAP

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/collections/avltree.hpp"
#include "cogs/collections/container_skiplist.hpp"
#include "cogs/collections/rbtree.hpp"


namespace cogs {


/// @ingroup LockFreeCollections
/// @brief A sorted O(log n) collection mapping unique keys to values.  Unique keys are enforced.
/// @tparam key_t The type used to compare elements.
/// @tparam value_t The value associated with the element.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
template <typename key_t, typename value_t, class comparator_t = default_comparator, class memory_manager_t = default_memory_manager>
class map
{
public:
	typedef key_t key_type;
	typedef value_t value_type;
	typedef memory_manager_t memory_manager_type;

	struct node
	{
		const key_t key;
		value_t value;
	};

private:
	typedef map<key_t, value_t, comparator_t, memory_manager_type> this_t;

	struct payload
	{
		placement<node> m_node;
		~payload() { m_node.destruct(); }
		const key_t& get_key() const { return m_node->key; }
	};

	typedef container_skiplist<false, key_t, payload, comparator_t, memory_manager_type> container_skiplist_t;
	container_skiplist_t m_contents;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

	template <typename T2> static constexpr bool is_iterator_type_v = std::is_same_v<iterator, std::remove_cv_t<T2> > || std::is_same_v<volatile_iterator, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_remove_token_type_v = std::is_same_v<remove_token, std::remove_cv_t<T2> > || std::is_same_v<volatile_remove_token, std::remove_cv_t<T2> >;
	template <typename T2> static constexpr bool is_element_reference_type_v = is_iterator_type_v<T2> || is_remove_token_type_v<T2>;

	/// @brief A map element iterator
	class iterator
	{
	private:
		friend class map;
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
		void operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); }

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

	/// @brief A volatile map element iterator
	class volatile_iterator
	{
	private:
		friend class map;
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
		void operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); }

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

	/// @brief A map element remove token
	class remove_token
	{
	private:
		friend class map;
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
		void operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); }


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

	/// @brief A map element volatile remove token
	class volatile_remove_token
	{
	private:
		friend class map;
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
		void operator=(T2&& i) volatile { m_contents = forward_member<T2>(i.m_contents); }

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

	map() { }

	map(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	map(const this_t& src)
		: m_contents(src.m_contents)
	{ }

	map(const volatile this_t&) = delete;

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	this_t& operator=(const this_t& src)
	{
		m_contents = src.m_contents;
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	void operator=(this_t&&) volatile = delete;
	void operator=(const this_t& src) volatile = delete;
	void operator=(const volatile this_t&) volatile = delete;

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
		iterator inserted;
		iterator replaced;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_replace_result>
	insert_replace_via(F&& f)
	{
		iterator inserted;
		auto p = m_contents.insert_replace_via([&](typename container_skiplist_t::iterator& i)
		{
			new (i.get()) payload;	// should be no-op, but for completeness.
			inserted = std::move(i);
			f(inserted);
		});
		return { std::move(inserted), std::move(p.replaced) };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& std::is_invocable_v<F, key_t&, const rcref<value_t>&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& std::is_invocable_v<F, const rcref<key_t>&, value_t&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i->value); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, value_t&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }


	insert_replace_result insert_replace(const key_t& k, const value_t& v)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	insert_replace_result insert_replace(key_t&& k, const value_t& v)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	insert_replace_result insert_replace(const key_t& k, value_t&& v)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}

	insert_replace_result insert_replace(key_t&& k, value_t&& v)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}


	template <typename... args_t>
	insert_replace_result insert_replace_emplace(const key_t& k, args_t&&... args)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	insert_replace_result insert_replace_emplace(key_t&& k, args_t&&... args)
	{
		return insert_replace_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}


	struct insert_unique_result
	{
		iterator inserted;
		iterator existing;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_unique_result>
	insert_unique_via(F&& f)
	{
		iterator inserted;
		auto p = m_contents.insert_unique_via([&](typename container_skiplist_t::iterator& i)
		{
			new (i.get()) payload;	// should be no-op, but for completeness.
			inserted = std::move(i);
			f(inserted);
		});
		return { std::move(inserted), std::move(p.existing) };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& std::is_invocable_v<F, key_t&, const rcref<value_t>&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& std::is_invocable_v<F, const rcref<key_t>&, value_t&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i->value); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, value_t&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }


	struct volatile_insert_unique_result
	{
		volatile_iterator inserted;
		volatile_iterator existing;
		bool wasEmpty;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile
	{
		iterator inserted;
		auto p = m_contents.insert_unique_via([&](typename container_skiplist_t::iterator& i)
		{
			new (i.get()) payload;	// should be no-op, but for completeness.
			inserted = std::move(i);
			f(inserted);
		});
		return { std::move(inserted), std::move(p.existing), p.wasEmpty };
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& std::is_invocable_v<F, key_t&, const rcref<value_t>&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i.get_value_obj().dereference()); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& std::is_invocable_v<F, const rcref<key_t>&, value_t&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(i.get_key_obj().dereference().template const_cast_to<key_t>(), i->value); }); }

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, key_t&, const rcref<value_t>&>
		&& !std::is_invocable_v<F, const rcref<key_t>&, value_t&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		volatile_insert_unique_result>
	insert_unique_via(F&& f) volatile { return insert_unique_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }


	insert_unique_result insert_unique(const key_t& k, const value_t& v)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	insert_unique_result insert_unique(key_t&& k, const value_t& v)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	insert_unique_result insert_unique(const key_t& k, value_t&& v)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}

	insert_unique_result insert_unique(key_t&& k, value_t&& v)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}


	volatile_insert_unique_result insert_unique(const key_t& k, const value_t& v) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	volatile_insert_unique_result insert_unique(key_t&& k, const value_t& v) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(v);
		});
	}

	volatile_insert_unique_result insert_unique(const key_t& k, value_t&& v) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}

	volatile_insert_unique_result insert_unique(key_t&& k, value_t&& v) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::move(v));
		});
	}


	template <typename... args_t>
	insert_unique_result insert_unique_emplace(const key_t& k, args_t&&... args)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	insert_unique_result insert_unique_emplace(key_t&& k, args_t&&... args)
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}


	template <typename... args_t>
	volatile_insert_unique_result insert_unique_emplace(const key_t& k, args_t&&... args) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(k);
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	template <typename... args_t>
	volatile_insert_unique_result insert_unique_emplace(key_t&& k, args_t&&... args) volatile
	{
		return insert_unique_via([&](iterator& i)
		{
			nested_rcnew(const_cast<key_t*>(&i->key), *i.get_desc())(std::move(k));
			nested_rcnew(&i->value, *i.get_desc())(std::forward<args_t>(args)...);
		});
	}

	/// @{
	/// @brief Removes an element
	/// @param e Element to remove
	/// @return True if the remove was successful, false if the element has already been removed
	bool remove(const remove_token& e) { return m_contents.remove(e.m_contents); }
	bool remove(const iterator& e) { return m_contents.remove(e.m_contents); }
	/// @}

	bool operator-=(const iterator& i) { return remove(i); }
	bool operator-=(const remove_token& rt) { return remove(rt); }

	typedef typename container_skiplist_t::volatile_remove_result volatile_remove_result;

	volatile_remove_result remove(const volatile_remove_token& e) volatile { return m_contents.remove(e.m_contents); }
	volatile_remove_result remove(const volatile_iterator& e) volatile { return m_contents.remove(e.m_contents); }

	volatile_remove_result operator-=(const volatile_iterator& i) volatile { return remove(i); }
	volatile_remove_result operator-=(const volatile_remove_token& rt) volatile { return remove(rt); }

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
	/// @brief Find an element with the matching key value
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find(const key_t& criteria) const { return iterator(m_contents.find_any_equal(criteria)); }
	/// @brief Thread-safe implementation of find()
	volatile_iterator find(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal(criteria)); }
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
	/// @brief Find an element equal to or the nearest element with a value lesser than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @brief Thread-safe implementation of find_equal_or_nearest_less_than()
	volatile_iterator find_equal_or_nearest_less_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_less_than(criteria)); }
	/// @}

	/// @{
	/// @brief Find an element equal to or the nearest element with a value greater than specified
	/// @param criteria The key value to search for
	/// @return An iterator to the found element, or an empty iterator if not found.
	iterator find_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @brief Thread-safe implementation of find_equal_or_nearest_greater_than()
	volatile_iterator find_equal_or_nearest_greater_than(const key_t& criteria) const volatile { return volatile_iterator(m_contents.find_any_equal_or_nearest_greater_than(criteria)); }
	/// @}

	/// @{
	/// @brief An index operato.  An alias to find()
	iterator operator[](const key_t& criteria) const { return find(criteria); }
	/// @brief Thread-safe implementation of operator[]()
	volatile_iterator operator[](const key_t& criteria) const volatile { return find(criteria); }
	/// @}

	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }

	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }

	// Volatile swap/exchange/move are only thread safe with regard to other volatile swap/exchange/move operations.
	//void swap(volatile this_t& wth) { m_contents.swap(wth.m_contents); }

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

	iterator rbegin() const { return get_last(); }
	volatile_iterator rbegin() const volatile { return get_last(); }

	iterator end() const { iterator i; return i; }
	volatile_iterator end() const volatile { volatile_iterator i; return i; }

	iterator rend() const { iterator i; return i; }
	volatile_iterator rend() const volatile { volatile_iterator i; return i; }
};


template <typename key_t, typename value_t, bool favor_lookup = false>
class nonvolatile_map_node
{
public:
	const key_t key;
	value_t value;

	class storage;
};

template <typename key_t, typename value_t, bool favor_lookup>
class nonvolatile_map_node<key_t, value_t, favor_lookup>::storage : public std::conditional_t<favor_lookup, avltree_node_t<storage>, rbtree_node_t<storage> >
{
public:
	placement<nonvolatile_map_node<key_t, value_t, favor_lookup>> m_node;
	~storage() { m_node.destruct(); }
	const key_t& get_key() const { return m_node->key; }
};

template <typename key_t, typename value_t, bool favor_lookup = false, class comparator_t = default_comparator, class allocator_t = batch_allocator<typename nonvolatile_map_node<key_t, value_t, favor_lookup>::storage>>
class nonvolatile_map
{
public:
	typedef key_t key_type;
	typedef value_t value_type;
	typedef allocator_t allocator_type;

	typedef nonvolatile_map_node<key_t, value_t, favor_lookup> node;

private:
	typedef nonvolatile_map<key_t, value_t, favor_lookup, comparator_t, allocator_type> this_t;

	typedef typename nonvolatile_map_node<key_t, value_t, favor_lookup>::storage payload;

	typedef std::conditional_t<favor_lookup, avltree<key_t, payload, comparator_t>, rbtree<key_t, payload, comparator_t> > tree_t;

	tree_t m_tree;
	size_t m_count;
	allocator_type m_allocator;

	void clear_inner()
	{
		ptr<payload> n = m_tree.get_first_postorder();
		while (!!n)
		{
			ptr<payload> n2 = m_tree.get_next_postorder(n);
			m_allocator.destruct_deallocate(n.get_ptr());
			n = n2;
		}
	}

public:
	class iterator
	{
	private:
		friend class nonvolatile_map;

		const tree_t* m_tree;
		ptr<payload> m_payload;

		iterator(const ptr<payload>& n, const tree_t& t)
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
		iterator& operator++(int) { iterator tmp(*this); ++*this; return tmp; }
		iterator& operator--(int) { iterator tmp(*this); --*this; return tmp; }

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

	typedef iterator remove_token;

	nonvolatile_map()
		: m_count(0)
	{ }

	nonvolatile_map(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_tree(std::move(src.m_tree)),
		m_count(src.m_count)
	{ }

	nonvolatile_map(const this_t& src)
	{
		for (const auto& entry : src)
			insert_replace(entry.m_key, entry.m_value);
	}

	~nonvolatile_map()
	{
		clear_inner();
	}

	nonvolatile_map(const volatile this_t&) = delete;

	this_t& operator=(this_t&& src)
	{
		m_allocator = std::move(src.m_allocator);
		m_tree = std::move(src.m_tree);
		m_count = src.m_count;
		return *this;
	}

	this_t& operator=(const this_t& src)
	{
		clear();
		for (const auto& entry : src)
			insert_replace(entry.m_key, entry.m_value);
	}

	this_t& operator=(const volatile this_t&) = delete;
	void operator=(this_t&&) volatile = delete;
	void operator=(const this_t& src) volatile = delete;

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
		iterator inserted;
		bool wasReplacement;
	};

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, iterator&>,
		insert_replace_result>
	insert_replace_via(F&& f)
	{
		insert_replace_result result;
		ptr<payload> r = m_allocator.allocate();
		new (r.get_ptr()) payload;
		result.inserted = iterator(std::move(r), m_tree);
		f(result.inserted);
		ptr<payload> existing = m_tree.insert_replace(result.inserted.m_payload);
		result.wasReplacement = !!existing;
		if (result.wasReplacement)
			m_allocator.destruct_deallocate(existing.get_ptr());
		else
			m_count++;
		return result;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		insert_replace_result>
	insert_replace_via(F&& f) { return insert_replace_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }

	insert_replace_result insert_replace(const key_t& k, const value_t& v) { return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(v); }); }
	insert_replace_result insert_replace(key_t&& k, const value_t& v) { return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(v); }); }
	insert_replace_result insert_replace(const key_t& k, value_t&& v) { return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::move(v)); }); }
	insert_replace_result insert_replace(key_t&& k, value_t&& v) { return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::move(v)); }); }

	template <typename... args_t>
	insert_replace_result insert_replace_emplace(const key_t& k, args_t&&... args)
	{
		return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}

	template <typename... args_t>
	insert_replace_result insert_replace_emplace(key_t&& k, args_t&&... args)
	{
		return insert_replace_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}

	struct insert_unique_result
	{
		iterator inserted;
		iterator existing;
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
		ptr<payload> r = m_allocator.allocate();
		new (r.get_ptr()) payload;
		result.inserted = iterator(std::move(r), m_tree);
		f(result.inserted);
		ptr<payload> existing = m_tree.insert_unique(result.inserted.m_payload);
		if (!existing)
			m_count++;
		else
		{
			result.inserted.release();
			m_allocator.destruct_deallocate(r.get_ptr());
			result.existing = iterator(std::move(existing), m_tree);
		}
		return result;
	}

	template <typename F>
	std::enable_if_t<
		!std::is_invocable_v<F, iterator&>
		&& std::is_invocable_v<F, key_t&, value_t&>,
		insert_unique_result>
	insert_unique_via(F&& f) { return insert_unique_via([&](iterator& i) { f(*const_cast<key_t*>(&i->key), i->value); }); }

	insert_unique_result insert_unique(const key_t& k, const value_t& v) { return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(v); }); }
	insert_unique_result insert_unique(key_t&& k, const value_t& v) { return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(v); }); }
	insert_unique_result insert_unique(const key_t& k, value_t&& v) { return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::move(v)); }); }
	insert_unique_result insert_unique(key_t&& k, value_t&& v) { return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::move(v)); }); }

	template <typename... args_t>
	insert_unique_result insert_unique_emplace(const key_t& k, args_t&&... args)
	{
		return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(k); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}

	template <typename... args_t>
	insert_unique_result insert_unique_emplace(key_t&& k, args_t&&... args)
	{
		return insert_unique_via([&](iterator& i) { new (const_cast<key_t*>(&i->key)) key_t(std::move(k)); new (&i->value) value_t(std::forward<args_t>(args)...); });
	}


	bool remove(const iterator& i)
	{
		m_tree.remove(i.m_payload);
		m_allocator.destruct_deallocate(i.m_payload.get_ptr());
		m_count--;
		return true;
	}

	bool remove(const iterator& i, bool& wasLast)
	{
		m_tree.remove(i.m_payload);
		m_allocator.destruct_deallocate(i.m_payload.get_ptr());
		m_count--;
		wasLast = m_count == 0;
		return true;
	}

	iterator find(const key_t& criteria) const { return iterator(m_tree.find_any_equal(criteria), m_tree); }
	iterator find_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_nearest_less_than(criteria), m_tree); }
	iterator find_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_nearest_greater_than(criteria), m_tree); }
	iterator find_equal_or_nearest_less_than(const key_t& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_less_than(criteria), m_tree); }
	iterator find_equal_or_nearest_greater_than(const key_t& criteria) const { return iterator(m_tree.find_any_equal_or_nearest_greater_than(criteria), m_tree); }

	iterator operator[](const key_t& criteria) const { return find(criteria); }

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
