//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified


/// @ingroup LockFreeCollections
/// @brief A sorted O(log n) collection mapping unique keys to values.  Unique keys are enforced.
/// @tparam key_t The type used to compare elements.
/// @tparam value_t The value associated with the element.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
template <typename key_t, typename value_t, class comparator_t = default_comparator, class allocator_type = default_allocator>
class map
{
private:
	typedef map<key_t, value_t, comparator_t, allocator_type> this_t;

	class payload
	{
	private:
		placement<key_t> m_key;
		placement<value_t> m_value;

	public:
		payload() { }

		~payload()
		{
			m_key.destruct();
			m_value.destruct();
		}

		void construct(const key_t& key, const value_t& value)
		{
			new (&m_key.get()) key_t(key);
			new (&m_value.get()) value_t(value);
		}

		void construct(const key_t& key)
		{
			new (&m_key.get()) key_t(key);
			new (&m_value.get()) value_t;
		}

		void construct()
		{
			new (&m_key.get()) key_t;
			new (&m_value.get()) value_t;
		}

		key_t& get_key() { return m_key.get(); }
		const key_t& get_key() const { return m_key.get(); }

		value_t& get_value() { return m_value.get(); }
		const value_t& get_value() const { return m_value.get(); }
	};

	typedef container_skiplist<key_t, payload, comparator_t, allocator_type> container_skiplist_t;
	container_skiplist_t m_contents;

	map(const this_t&) = delete;
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

	/// @brief A map element iterator
	class iterator
	{
	private:
		friend class map;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;
		friend class preallocated;

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

		iterator(const typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		iterator(const volatile typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		iterator(typename container_skiplist_t::preallocated&& i) : m_contents(std::move(i)) { }

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

		const key_t& get_key() const { return m_contents->get_key(); }
		value_t& get_value() const { return m_contents->get_value(); }
		value_t& operator*() const { return m_contents->get_value(); }
		value_t* operator->() const { return &(m_contents->get_value()); }

		rcptr<const key_t> get_key_obj() const
		{
			rcptr<const key_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_key()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_value_obj() const
		{
			rcptr<value_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

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

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) { return iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		iterator exchange(T2&& src) volatile { return iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
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
		friend class preallocated;

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

		volatile_iterator(const typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		volatile_iterator(const volatile typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		volatile_iterator(typename container_skiplist_t::preallocated&& i) : m_contents(std::move(i)) { }

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

		const key_t& get_key() const { return m_contents->get_key(); }
		value_t& get_value() const { return m_contents->get_value(); }
		value_t& operator*() const { return m_contents->get_value(); }
		value_t* operator->() const { return &(m_contents->get_value()); }

		rcptr<const key_t> get_key_obj() const
		{
			rcptr<const key_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_key()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_value_obj() const
		{
			rcptr<value_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

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

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) { return volatile_iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_iterator exchange(T2&& src) volatile { return volatile_iterator(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

	};

	class preallocated
	{
	private:
		friend class map;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;
		friend class volatile_remove_token;

		typename container_skiplist_t::preallocated m_contents;

		payload& get_payload() const { return *m_contents; }

		preallocated(const typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		preallocated(const volatile typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		preallocated(typename container_skiplist_t::preallocated&& i) : m_contents(std::move(i)) { }

	public:
		preallocated() { }

		preallocated(const preallocated& src) : m_contents(src.m_contents) { }
		preallocated(const volatile preallocated& src) : m_contents(src.m_contents) { }
		preallocated(preallocated&& src) : m_contents(std::move(src.m_contents)) { }

		preallocated& operator=(const preallocated& i) { m_contents = i.m_contents; return *this; }
		preallocated& operator=(const volatile preallocated& i) { m_contents = i.m_contents; return *this; }
		preallocated& operator=(preallocated&& i) { m_contents = std::move(i.m_contents); return *this; }
		volatile preallocated& operator=(const preallocated& i) volatile { m_contents = i.m_contents; return *this; }
		volatile preallocated& operator=(const volatile preallocated& i) volatile { m_contents = i.m_contents; return *this; }
		volatile preallocated& operator=(preallocated&& i) volatile { m_contents = std::move(i.m_contents); return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator==(const T2& i) const volatile { return m_link == i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const { return m_link != i.m_link; }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<T2> > >
		bool operator!=(const T2& i) const volatile { return m_link != i.m_link; }

		key_t& get_key() const { return m_contents->get_key(); }
		value_t& get_value() const { return m_contents->get_value(); }
		value_t& operator*() const { return m_contents->get_value(); }
		value_t* operator->() const { return &(m_contents->get_value()); }

		rcptr<key_t> get_key_obj() const
		{
			rcptr<key_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_key()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_value_obj() const
		{
			rcptr<value_t> result;
			rcptr<payload> obj = m_contents.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

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

	/// @brief A map element remove token
	class remove_token
	{
	private:
		friend class map;
		friend class iterator;
		friend class volatile_iterator;
		friend class volatile_remove_token;
		friend class preallocated;

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

		remove_token(const typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		remove_token(const volatile typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		remove_token(typename container_skiplist_t::preallocated&& i) : m_contents(std::move(i)) { }

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


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) { return remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		remove_token exchange(T2&& src) volatile { return remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

	};

	/// @brief A volatile map element remove token
	class volatile_remove_token
	{
	private:
		friend class map;
		friend class iterator;
		friend class volatile_iterator;
		friend class remove_token;
		friend class preallocated;

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

		volatile_remove_token(const typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		volatile_remove_token(const volatile typename container_skiplist_t::preallocated& i) : m_contents(i) { }
		volatile_remove_token(typename container_skiplist_t::preallocated&& i) : m_contents(std::move(i)) { }

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


		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(T2& wth) { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> && !std::is_volatile_v<T2> > >
		void swap(T2& wth) volatile { m_contents.swap(wth.m_contents); }

		template <typename T2, typename = std::enable_if_t<is_iterator_type_v<T2> && !std::is_const_v<T2> && !std::is_same_v<std::remove_cv_t<T2>, preallocated> > >
		void swap(volatile T2& wth) { m_contents.swap(wth.m_contents); }


		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) { return volatile_remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }

		template <typename T2, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > > >
		volatile_remove_token exchange(T2&& src) volatile { return volatile_remove_token(m_contents.exchange(forward_member<T2>(src.m_contents))); }


		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t<is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && !std::is_const_v<T3> && !std::is_same_v<std::remove_cv_t<T3>, preallocated> > >
		void exchange(T2&& src, T3& rtn) volatile { m_contents.exchange(forward_member<T2>(src.m_contents), rtn.m_contents); }


		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }

		template <typename T2, typename T3, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> > >
		bool compare_exchange(T2&& src, const T3& cmp) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents); }


		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }

		template <typename T2, typename T3, typename T4, typename = std::enable_if_t< is_element_reference_type_v<std::remove_reference_t<T2> > && is_element_reference_type_v<T3> && is_element_reference_type_v<T4> && !std::is_const_v<T4> && !std::is_same_v<std::remove_cv_t<T4>, preallocated> > >
		bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile { return m_contents.compare_exchange(forward_member<T2>(src.m_contents), cmp.m_contents, rtn.m_contents); }
	};

	map(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	map() { }
	map(volatile allocator_type& al) : m_contents(al) { }

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

	/// @{
	/// @brief Inserts an element, replacing any existing element with an equal key value
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @return An iterator to the newly inserted element 
	iterator insert_replace(const key_t& k, const value_t& v)
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_replace_preallocated(i));
	}

	/// @brief Inserts an element, replacing any existing element with an equal key value
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @param[out] collision Receives a value indicating whether the insert encountered a collision and removed another element
	/// @return An iterator to the newly inserted element 
	iterator insert_replace(const key_t& k, const value_t& v, bool& collision)
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_replace_preallocated(i, collision));
	}
	/// @}

	/// @{
	/// @brief Inserts an element only if there is no existing element with the same key value.
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @return An iterator to the newly inserted element, or an empty iterator if there was already an element with the same key.
	iterator try_insert(const key_t& k, const value_t& v)
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_unique_preallocated(i));
	}

	/// @brief Inserts an element only if there is no existing element with the same key value.
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @param[out] collision Receives a value indicating whether the insert encountered a collision and removed another element
	/// @return An iterator to the newly inserted element, or an empty iterator if there was already an element with the same key.
	iterator try_insert(const key_t& k, const value_t& v, bool& collision)
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_unique_preallocated(i, collision));
	}

	/// @brief Thread-safe implementation of try_insert()
	volatile_iterator try_insert(const key_t& k, const value_t& v) volatile
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return volatile_iterator(m_contents.insert_unique_preallocated(i));
	}

	/// @brief Thread-safe implementation of try_insert()
	volatile_iterator try_insert(const key_t& k, const value_t& v, bool& collision) volatile
	{
		typename container_skiplist_t::preallocated i = m_contents.preallocate();
		i.get()->construct(k, v);
		return volatile_iterator(m_contents.insert_unique_preallocated(i, collision));
	}
	/// @}

	preallocated preallocate() const volatile
	{
		preallocated i = m_contents.preallocate();
		i.get_payload().construct();
		return i;
	}

	preallocated preallocate(const key_t& k, const value_t& v) const volatile
	{
		preallocated i = m_contents.preallocate();
		i.get_payload().construct(k, v);
		return i;
	}

	preallocated preallocate_key(const key_t& k) const volatile
	{
		preallocated i = m_contents.preallocate();
		i.get_payload().construct(k);
		return i;
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_contents, storage);
		i.get_payload().construct();
		return storage.dereference();
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(const key_t& k, const value_t& v, preallocated& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_contents, storage);
		i.get_payload().construct(k, v);
		return storage.dereference();
	}

	template <typename T>
	const rcref<T>& preallocate_key_with_aux(const key_t& k, preallocated& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_contents, storage);
		i.get_payload().construct(k);
		return storage.dereference();
	}

	iterator insert_replace_preallocated(const preallocated& i) { return iterator(m_contents.insert_replace_preallocated(i.m_contents)); }
	iterator insert_replace_preallocated(const preallocated& i, bool& collision) { return iterator(m_contents.insert_replace_preallocated(i.m_contents, collision)); }

	iterator try_insert_preallocated(const preallocated& i) { return iterator(m_contents.insert_unique_preallocated(i.m_contents)); }
	iterator try_insert_preallocated(const preallocated& i, bool& collision) { return iterator(m_contents.insert_unique_preallocated(i.m_contents, collision)); }

	volatile_iterator try_insert_preallocated(const preallocated& i) volatile { return volatile_iterator(m_contents.insert_unique_preallocated(i.m_contents)); }
	volatile_iterator try_insert_preallocated(const preallocated& i, bool& collision) volatile { return volatile_iterator(m_contents.insert_unique_preallocated(i.m_contents, collision)); }

	/// @{
	/// @brief Removes an element
	/// @param e Element to remove
	/// @return True if the remove was successful, false if the element has already been removed
	bool remove(const remove_token& e) { return m_contents.remove(e.m_contents); }
	bool remove(const iterator& e) { return m_contents.remove(e.m_contents); }
	/// @brief Removes an element
	/// @param e Element to remove
	/// @param[out] wasLast Receives a value indicating whether this was the last element in the list
	/// @return True if the remove was successful, false if the element has already been removed
	bool remove(const remove_token& e, bool& wasLast) { bool b = m_contents.remove(e.m_contents); wasLast = b && is_empty(); return b; }
	/// @brief Removes an element
	/// @param e Element to remove
	/// @param[out] wasLast Receives a value indicating whether this was the last element in the list
	/// @return True if the remove was successful, false if the element has already been removed
	bool remove(const iterator& e, bool& wasLast) { bool b = m_contents.remove(e.m_contents); wasLast = b && is_empty(); return b; }
	/// @brief Thread-safe implementation of remove()
	bool remove(const volatile_remove_token& e) volatile { return m_contents.remove(e.m_contents); }
	/// @brief Thread-safe implementation of remove()
	bool remove(const volatile_iterator& e) volatile { return m_contents.remove(e.m_contents); }
	/// @}

	iterator pop_first() { return iterator(m_contents.pop_first()); }
	iterator pop_first(bool& wasLast) { return iterator(m_contents.pop_first(wasLast)); }
	iterator pop_last() { return iterator(m_contents.pop_last()); }
	iterator pop_last(bool& wasLast) { return iterator(m_contents.pop_last(wasLast)); }
	volatile_iterator pop_first() volatile { return volatile_iterator(m_contents.pop_first()); }
	volatile_iterator pop_last() volatile { return volatile_iterator(m_contents.pop_last()); }
	volatile_iterator pop_first(bool& wasLast) volatile { return volatile_iterator(m_contents.pop_first(wasLast)); }
	volatile_iterator pop_last(bool& wasLast) volatile { return volatile_iterator(m_contents.pop_last(wasLast)); }

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
};


template <typename key_t, typename value_t, bool favor_lookup = false, class comparator_t = default_comparator, class allocator_type = default_allocator>
class nonvolatile_map
{
private:
	typedef nonvolatile_map<key_t, value_t, favor_lookup, comparator_t, allocator_type> this_t;

	class node : public std::conditional_t<favor_lookup, avltree_node_t<node>, rbtree_node_t<node> >
	{
	public:
		key_t m_key;
		value_t m_contents;

		node(const key_t& key, const value_t& contents)
			: m_key(key),
			m_contents(contents)
		{ }

		node(const key_t& key)
			: m_key(key)
		{ }

		const key_t& get_key() const
		{
			return m_key;
		}
	};

	typedef std::conditional_t<favor_lookup, avltree<key_t, node, comparator_t>, rbtree<key_t, node, comparator_t> > tree_t;

	typedef typename tree_t::ref_t ref_t;

	nonvolatile_map(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	tree_t m_tree;
	size_t m_count;
	allocator_container<default_allocator> m_allocator;

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
		friend class nonvolatile_map;

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

		const key_t& get_key() const { return m_node->m_key; }
		value_t& get_value() const { return m_node->m_contents; }

		value_t& operator*() const { return m_node->m_contents; }
		value_t* operator->() const { return &(m_node->m_contents); }

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

	nonvolatile_map(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_tree(std::move(src.m_tree)),
		m_count(src.m_count)
	{ }

	this_t& operator=(this_t&& src)
	{
		m_allocator = std::move(src.m_allocator);
		m_tree = std::move(src.m_tree);
		m_count = src.m_count;
		return *this;
	}

	nonvolatile_map()
		: m_count(0)
	{ }

	nonvolatile_map(volatile allocator_type& al)
		: m_count(0),
		m_allocator(al)
	{ }

	~nonvolatile_map()
	{
		clear_inner();
	}

	void swap(this_t& srcDst)
	{
		m_tree.swap(srcDst.m_tree);
		size_t tmpCount = m_count;
		m_count = srcDst.m_count;
		srcDst.m_count = tmpCount;
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

	iterator insert_replace(const key_t& key, const value_t& t)
	{
		ref_t r = m_allocator.template allocate_type<node>();
		typename ref_t::locked_t n = r;
		new (n) node(key, t);
		m_tree.insert_replace(r);
		m_count++;
		return iterator(r, m_tree);
	}

	iterator try_insert(const key_t& key, const value_t& t)
	{
		ref_t r = m_allocator.template allocate_type<node>();
		typename ref_t::locked_t n = r;
		new (n) node(key, t);
		ref_t existing = m_tree.insert_unique(r);
		if (!existing)
		{
			m_count++;
			return iterator(r, m_tree);
		}
		m_allocator.template destruct_deallocate_type<node>(n);
		return iterator();
	}

	iterator try_insert(const key_t& key, const value_t& t, bool& collision)
	{
		ref_t r = m_allocator.template allocate_type<node>();
		typename ref_t::locked_t n = r;
		new (n) node(key, t);
		ref_t existing = m_tree.insert_unique(r);
		if (!existing)
		{
			collision = false;
			m_count++;
			return iterator(r, m_tree);
		}
		collision = true;
		m_allocator.template destruct_deallocate_type<node>(n);
		return iterator(existing, m_tree);
	}

	bool remove(const iterator& i)
	{
		m_tree.remove(i.m_node);
		m_allocator.template destruct_deallocate_type<node>(i.m_node);
		m_count--;
		return true;
	}

	bool remove(const iterator& i, bool& wasLast)
	{
		m_tree.remove(i.m_node);
		m_allocator.template destruct_deallocate_type<node>(i.m_node);
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
};


#pragma warning(pop)


}


#endif
