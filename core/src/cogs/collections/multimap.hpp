//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_MULTIMAP
#define COGS_HEADER_COLLECTION_MULTIMAP

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
/// @brief A sorted O(log n) collection mapping keys to values.  Duplicate keys are allowed.
/// @tparam key_t The type used to compare elements.
/// @tparam value_t The value associated with the element.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
template <typename key_t, typename value_t, class comparator_t = default_comparator, class allocator_type = default_allocator>
class multimap
{
private:
	typedef multimap<key_t, value_t, comparator_t, allocator_type> this_t;

	class payload
	{
	public:
		placement<key_t> m_key;
		placement<value_t> m_value;

		payload()
		{ }

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

	multimap(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class iterator;
	class volatile_iterator;
	class remove_token;
	class volatile_remove_token;

	/// @brief A multimap element iterator
	class iterator
	{
	private:
		typename container_skiplist_t::iterator m_iterator;

	protected:
		iterator(const typename container_skiplist_t::iterator& itor) : m_iterator(itor) { }

		friend class multimap;

	public:
		void disown() { m_iterator.disown(); }

		iterator() { }
		iterator(const iterator& itor) : m_iterator(itor.m_iterator) { }
		iterator(const remove_token& rt) : m_iterator(rt.m_removeToken) { }

		iterator& operator=(const iterator& i) { m_iterator = i.m_iterator; return *this; }
		iterator& operator=(const remove_token& rt) { m_iterator = rt.m_removeToken; return *this; }

		void release() { m_iterator.release(); }

		bool is_removed() { return m_iterator.is_removed(); }

		iterator& operator++() { ++m_iterator; return *this; }
		iterator& operator--() { --m_iterator; return *this; }

		iterator operator++(int) { iterator i(*this); ++* this; return i; }
		iterator operator--(int) { iterator i(*this); --* this; return i; }

		bool operator!() const { return !m_iterator; }
		bool operator!() const volatile { return !m_iterator; }

		bool operator==(const iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator==(const remove_token& rt) const { return m_iterator == rt.m_removeToken; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const remove_token& rt) const { return !operator==(rt); }

		const key_t& get_key() const { return m_iterator->get_key(); }
		value_t& get_value() const { return m_iterator->get_value(); }
		value_t& operator*() const { return m_iterator->get_value(); }
		value_t* operator->() const { return &(m_iterator->get_value()); }

		rcptr<const key_t> get_key_obj() const
		{
			rcptr<const key_t> result;
			rcptr<payload> obj = m_iterator.get_obj();
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
			rcptr<payload> obj = m_iterator.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_iterator.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_iterator.get_desc(); }

		iterator next() const { return iterator(m_iterator.next()); }
		iterator prev() const { return iterator(m_iterator.prev()); }
	};

	/// @brief A volatile multimap element iterator
	class volatile_iterator
	{
	private:
		typename container_skiplist_t::volatile_iterator m_iterator;

	protected:
		volatile_iterator(const typename container_skiplist_t::volatile_iterator& itor) : m_iterator(itor) { }

		friend class multimap;

	public:
		void disown() { m_iterator.disown(); }

		volatile_iterator() { }
		volatile_iterator(const volatile_iterator& i) : m_iterator(i.m_iterator) { }
		volatile_iterator(const volatile_remove_token& rt) : m_iterator(rt.m_removeToken) { }
		volatile_iterator(const volatile volatile_iterator& i) : m_iterator(i.m_iterator) { }
		volatile_iterator(const volatile volatile_remove_token& rt) : m_iterator(rt.m_removeToken) { }

		volatile_iterator& operator=(const volatile_iterator& i) { m_iterator = i.m_iterator; return *this; }
		volatile_iterator& operator=(const volatile volatile_iterator& i) { m_iterator = i.m_iterator; return *this; }
		volatile_iterator& operator=(const volatile_remove_token& rt) { m_iterator = rt.m_removeToken; return *this; }
		volatile_iterator& operator=(const volatile volatile_remove_token& rt) { m_iterator = rt.m_removeToken; return *this; }
		void operator=(const volatile_iterator& i) volatile { m_iterator = i.m_iterator; }
		void operator=(const volatile_remove_token& rt) volatile { m_iterator = rt.m_removeToken; }

		bool is_active() const { return m_iterator.is_active(); }
		bool is_active() const volatile { return m_iterator.is_active(); }

		bool is_removed() const { return m_iterator.is_removed(); }
		bool is_removed() const volatile { return m_iterator.is_removed(); }

		void release() { m_iterator.release(); }
		void release() volatile { m_iterator.release(); }

		const volatile_iterator& operator++() { ++m_iterator; return *this; }
		const volatile_iterator& operator--() { --m_iterator; return *this; }

		volatile_iterator operator++() volatile { volatile_iterator result(*this); ++result; return result; }
		volatile_iterator operator--() volatile { volatile_iterator result(*this); --result; return result; }

		volatile_iterator operator++(int) { volatile_iterator i(*this); ++* this; return i; }
		volatile_iterator operator--(int) { volatile_iterator i(*this); --* this; return i; }

		volatile_iterator operator++(int) volatile { return volatile_iterator(m_iterator++); }
		volatile_iterator operator--(int) volatile { return volatile_iterator(m_iterator--); }

		bool operator!() const { return !m_iterator; }
		bool operator!() const volatile { return !m_iterator; }

		bool operator==(const volatile_iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator==(const volatile volatile_iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator==(const volatile_iterator& i) const volatile { return m_iterator == i.m_iterator; }
		bool operator==(const volatile_remove_token& rt) const { return m_iterator == rt.m_removeToken; }
		bool operator==(const volatile volatile_remove_token& rt) const { return m_iterator == rt.m_removeToken; }
		bool operator==(const volatile_remove_token& rt) const volatile { return m_iterator == rt.m_removeToken; }

		bool operator!=(const volatile_iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile { return !operator==(i); }
		bool operator!=(const volatile_remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile volatile_remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile_remove_token& rt) const volatile { return !operator==(rt); }

		const key_t& get_key() const { return m_iterator->get_key(); }
		value_t& get_value() const { return m_iterator->get_value(); }
		value_t& operator*() const { return m_iterator->get_value(); }
		value_t* operator->() const { return &(m_iterator->get_value()); }

		rcptr<const key_t> get_key_obj() const
		{
			rcptr<const key_t> result;
			rcptr<payload> obj = m_iterator.get_obj();
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
			rcptr<payload> obj = m_iterator.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_iterator.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_iterator.get_desc(); }

		volatile_iterator next() const { return volatile_iterator(m_iterator.next()); }
		volatile_iterator prev() const { return volatile_iterator(m_iterator.prev()); }

		bool compare_exchange(const volatile_iterator& src, const volatile_iterator& cmp) volatile
		{
			return m_iterator.compare_exchange(src.m_iterator, cmp.m_iterator);
		}

		bool compare_exchange(const volatile_iterator& src, const volatile_iterator& cmp, volatile_iterator& rtn) volatile
		{
			return m_iterator.compare_exchange(src.m_iterator, cmp.m_iterator, rtn.m_iterator);
		}
	};

	class preallocated_t
	{
	protected:
		friend class multimap;

		typename container_skiplist_t::preallocated_t m_preallocated;

		payload& get_payload() const { return *m_preallocated; }

		preallocated_t(const typename container_skiplist_t::preallocated_t& i) : m_preallocated(i) { }
		preallocated_t& operator=(const typename container_skiplist_t::preallocated_t& i) { m_preallocated = i; return *this; }

	public:
		void disown() { m_preallocated.disown(); }

		preallocated_t() { }

		preallocated_t(const preallocated_t& src)
			: m_preallocated(src.m_preallocated)
		{ }

		void release() { m_preallocated.release(); }

		bool operator!() const { return !m_preallocated; }
		bool operator==(const preallocated_t & i) const { return m_preallocated == i.m_preallocated; }
		bool operator!=(const preallocated_t & i) const { return !operator==(i); }
		preallocated_t& operator=(const preallocated_t & i) { m_preallocated = i.m_preallocated; return *this; }

		key_t& get_key() const { return m_preallocated->get_key(); }
		value_t& get_value() const { return m_preallocated->get_value(); }
		value_t& operator*() const { return m_preallocated->get_value(); }
		value_t* operator->() const { return &(m_preallocated->get_value()); }

		rcptr<key_t> get_key_obj() const
		{
			rcptr<key_t> result;
			rcptr<payload> obj = m_preallocated.get_obj();
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
			rcptr<payload> obj = m_preallocated.get_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<value_t> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_preallocated.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_preallocated.get_desc(); }
	};

	/// @brief A multimap element remove token
	class remove_token
	{
	private:
		typename container_skiplist_t::remove_token m_removeToken;

	protected:
		remove_token(const typename container_skiplist_t::remove_token& rt) : m_removeToken(rt) { }

		friend class multimap;

	public:
		remove_token() { }
		remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		remove_token(const iterator& i) : m_removeToken(i.m_iterator) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }

		remove_token& operator=(const preallocated_t& i) { m_removeToken = i.m_preallocated; return *this; }
		remove_token& operator=(const iterator& i) { m_removeToken = i.m_iterator; return *this; }
		remove_token& operator=(const remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }

		bool is_active() const { return m_removeToken.is_active(); }

		void release() { m_removeToken.release(); }

		bool operator!() const { return !m_removeToken; }

		bool operator==(const iterator& i) const { return m_removeToken == i.m_iterator; }
		bool operator==(const remove_token& rt) const { return m_removeToken == rt.m_removeToken; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
	};

	/// @brief A multimap element volatile remove token
	class volatile_remove_token
	{
	private:
		typename container_skiplist_t::volatile_remove_token m_removeToken;

	protected:
		volatile_remove_token(const typename container_skiplist_t::volatile_remove_token& rt) : m_removeToken(rt) { }

		friend class multimap;

	public:
		volatile_remove_token() { }
		volatile_remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		volatile_remove_token(const volatile_iterator& i) : m_removeToken(i.m_iterator) { }
		volatile_remove_token(const volatile volatile_iterator& i) : m_removeToken(i.m_iterator) { }
		volatile_remove_token(const volatile_remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		volatile_remove_token(const volatile volatile_remove_token& rt) : m_removeToken(rt.m_removeToken) { }

		volatile_remove_token& operator=(const preallocated_t& i) { m_removeToken = i.m_preallocated; return *this; }
		volatile_remove_token& operator=(const volatile_iterator& i) { m_removeToken = i.m_iterator; return *this; }
		volatile_remove_token& operator=(const volatile volatile_iterator& i) { m_removeToken = i.m_iterator; return *this; }
		volatile_remove_token& operator=(const volatile_remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		volatile_remove_token& operator=(const volatile volatile_remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }

		void operator=(const volatile_iterator& i) volatile { m_removeToken = i.m_iterator; }
		void operator=(const volatile_remove_token& rt) volatile { m_removeToken = rt.m_removeToken; }

		bool is_active() const { return m_removeToken.is_active(); }
		bool is_active() const volatile { return m_removeToken.is_active(); }

		void release() { m_removeToken.release(); }
		void release() volatile { m_removeToken.release(); }

		bool operator!() const { return !m_removeToken; }
		bool operator!() const volatile { return !m_removeToken; }

		bool operator==(const volatile_iterator& i) const { return m_removeToken == i.m_iterator; }
		bool operator==(const volatile volatile_iterator& i) const { return m_removeToken == i.m_iterator; }
		bool operator==(const volatile_iterator& i) const volatile { return m_removeToken == i.m_iterator; }
		bool operator==(const volatile_remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile volatile_remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile_remove_token& rt) const volatile { return m_removeToken == rt.m_removeToken; }

		bool operator!=(const volatile_iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile volatile_iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile_iterator& i) const volatile { return !operator==(i); }
		bool operator!=(const volatile_remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile volatile_remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile_remove_token& rt) const volatile { return !operator==(rt); }
	};

	multimap(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	multimap() { }
	multimap(volatile allocator_type& al) : m_contents(al) { }

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
	/// @brief Inserts an element
	/// @param k Key value to insert
	/// @param v Value to insert
	/// @return An iterator to the newly inserted element 
	iterator insert(const key_t& k, const value_t& v)
	{
		typename container_skiplist_t::preallocated_t i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_multi_preallocated(i));
	}
	/// @brief Thread-safe implementation of insert()
	volatile_iterator insert(const key_t& k, const value_t& v) volatile
	{
		typename container_skiplist_t::preallocated_t i = m_contents.preallocate();
		i.get()->construct(k, v);
		return volatile_iterator(m_contents.insert_multi_preallocated(i));
	}
	/// @}

	iterator insert(const key_t& k, const value_t& v, bool& wasEmpty)
	{
		typename container_skiplist_t::preallocated_t i = m_contents.preallocate();
		i.get()->construct(k, v);
		return iterator(m_contents.insert_multi_preallocated(i, wasEmpty));
	}

	volatile_iterator insert(const key_t& k, const value_t& v, bool& wasEmpty) volatile
	{
		typename container_skiplist_t::preallocated_t i = m_contents.preallocate();
		i.get()->construct(k, v);
		return volatile_iterator(m_contents.insert_multi_preallocated(i, wasEmpty));
	}


	preallocated_t preallocate() const volatile
	{
		preallocated_t i = m_contents.preallocate();
		i.get_payload().construct();
		return i;
	}

	preallocated_t preallocate(const key_t& k, const value_t& v) const volatile
	{
		preallocated_t i = m_contents.preallocate();
		i.get_payload().construct(k, v);
		return i;
	}

	preallocated_t preallocate_key(const key_t& k) const volatile
	{
		preallocated_t i = m_contents.preallocate();
		i.get_payload().construct(k);
		return i;
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_preallocated, storage);
		i.get_payload().construct();
		return storage.dereference();
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(const key_t& k, const value_t& v, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_preallocated, storage);
		i.get_payload().construct(k, v);
		return storage.dereference();
	}

	template <typename T>
	const rcref<T>& preallocate_key_with_aux(const key_t& k, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_with_aux<T>(i.m_preallocated, storage);
		i.get_payload().construct(k);
		return storage.dereference();
	}

	volatile_iterator insert_preallocated(const preallocated_t& i) volatile { return volatile_iterator(m_contents.insert_multi_preallocated(i.m_preallocated)); }

	iterator insert_preallocated(const preallocated_t& i) { return iterator(m_contents.insert_multi_preallocated(i.m_preallocated)); }


	volatile_iterator insert_preallocated(const preallocated_t& i, bool& wasEmpty) volatile { return volatile_iterator(m_contents.insert_multi_preallocated(i.m_preallocated, wasEmpty)); }

	iterator insert_preallocated(const preallocated_t& i, bool& wasEmpty) { return iterator(m_contents.insert_multi_preallocated(i.m_preallocated, wasEmpty)); }

	/// @{
	/// @brief Removes an element
	/// @param e Element to remove
	/// @return True if the remove was successful, false if the element was not removed in parallel by another thread.
	bool remove(const remove_token& e) { return m_contents.remove(e.m_removeToken); }
	bool remove(const iterator& e) { return m_contents.remove(e.m_iterator); }
	/// @brief Removes an element
	/// @param e Element to remove
	/// @param[out] wasLast Receives a value indicating whether this was the last element in the list
	/// @return True if the remove was successful, false if the element was not removed in parallel by another thread.
	bool remove(const remove_token& e, bool& wasLast) { bool b = m_contents.remove(e.m_removeToken); wasLast = b && is_empty(); return b; }
	/// @brief Removes an element
	/// @param e Element to remove
	/// @param[out] wasLast Receives a value indicating whether this was the last element in the list
	/// @return True if the remove was successful, false if the element was not removed in parallel by another thread.
	bool remove(const iterator& e, bool& wasLast) { bool b = m_contents.remove(e.m_iterator); wasLast = b && is_empty(); return b; }
	/// @brief Thread-safe implementation of remove()
	bool remove(const volatile_remove_token& e) volatile { return m_contents.remove(e.m_removeToken); }
	/// @brief Thread-safe implementation of remove()
	bool remove(const volatile_iterator& e) volatile { return m_contents.remove(e.m_iterator); }
	/// @}

	iterator pop_first() { return iterator(m_contents.pop_first()); }
	iterator pop_last() { return iterator(m_contents.pop_last()); }
	volatile_iterator pop_first() volatile { return volatile_iterator(m_contents.pop_first()); }
	volatile_iterator pop_last() volatile { return volatile_iterator(m_contents.pop_last()); }

	iterator pop_first(bool& wasLast) { return iterator(m_contents.pop_first(wasLast)); }
	iterator pop_last(bool& wasLast) { return iterator(m_contents.pop_last(wasLast)); }
	volatile_iterator pop_first(bool& wasLast) volatile { return volatile_iterator(m_contents.pop_first(wasLast)); }
	volatile_iterator pop_last(bool& wasLast) volatile { return volatile_iterator(m_contents.pop_last(wasLast)); }

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
};


template <typename key_t, typename value_t, bool favor_lookup = false, class comparator_t = default_comparator, class allocator_type = default_allocator>
class nonvolatile_multimap
{
private:
	typedef nonvolatile_multimap<key_t, value_t, favor_lookup, comparator_t, allocator_type> this_t;

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
			m_allocator.template destruct_deallocate_type<node>(n);
			n = n2;
		}
	}

public:
	class iterator
	{
	private:
		const tree_t* m_tree;
		ref_t m_node;

	protected:
		iterator(const ref_t& n, const tree_t& t)
			: m_tree(&t),
			m_node(n)
		{ }

		friend class nonvolatile_multimap;

	public:
		iterator() { }

		iterator(const iterator& i)
			: m_tree(i.m_tree),
			m_node(i.m_node)
		{ }

		void release() { m_node = 0; }

		iterator& operator++() { if (!!m_node) m_node = m_tree->get_next(m_node); return *this; }
		iterator& operator--() { if (!!m_node) m_node = m_tree->get_prev(m_node); return *this; }
		iterator& operator++(int) { iterator tmp(*this); ++* this; return tmp; }
		iterator& operator--(int) { iterator tmp(*this); --* this; return tmp; }

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
	};

	typedef iterator remove_token;

	nonvolatile_multimap(this_t&& src)
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

	nonvolatile_multimap()
		: m_count(0)
	{ }

	nonvolatile_multimap(volatile allocator_type& al)
		: m_count(0),
		m_allocator(al)
	{ }

	~nonvolatile_multimap()
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

	iterator insert(const key_t& key, const value_t& t)
	{
		ref_t r = m_allocator.template allocate_type<node>();
		typename ref_t::locked_t n = r;
		new (n) node(key, t);
		m_tree.insert_multi(r);
		m_count++;
		return iterator(r, m_tree);
	}

	void remove(const iterator& i)
	{
		m_tree.remove(i.m_node);
		m_allocator.template destruct_deallocate_type<node>(i.m_node);
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
};


}


#endif
