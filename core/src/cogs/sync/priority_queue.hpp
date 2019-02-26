//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_PRIORITY_QUEUE
#define COGS_HEADER_SYNC_PRIORITY_QUEUE


#include "cogs/collections/multimap.hpp"
#include "cogs/collections/multiset.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/operators.hpp"


namespace cogs {


// change_priority() is careful not to allow worse priority items through while rescheduling a better priority item.


/// @ingroup Synchronization
/// @brief A priority queue.
template <typename key_t, typename type = void, class comparator_t = default_comparator, class allocator_type = default_allocator>
class priority_queue
{
private:
	typedef priority_queue<key_t, type, comparator_t, allocator_type> this_t;

	class payload
	{
	public:
		placement<type> m_value;
		volatile boolean m_priorityChanged;	// Used to synchronize concurrent priority changes
		volatile rcptr<payload> m_removed;	// Used to synchronized priority changes with concurrent scheduled removals (gets)
		volatile typename multimap<key_t, payload, comparator_t, allocator_type>::volatile_remove_token m_rescheduledTo;

		void construct(const type& t) { new (&get_value()) type(t); }

		void construct() { new (&get_value()) type; }

		void set_removed(rcptr<payload>&& removed)
		{
			// Do not need volatility here, so cast it away
			*(rcptr<payload>*)&m_removed = std::move(removed);
		}

		~payload()
		{
			rcptr<payload>& removed = *(rcptr<payload>*)&m_removed;
			if (!removed.get_unmarked())
				m_value.destruct();
		}

		type& get_value()
		{
			rcptr<payload> removed = m_removed;	// If resheduled, we retain the original object
			if (!!removed && removed.get_mark() == 0)
				return removed->m_value.get();
			return m_value.get();
		}
	};

	typedef multimap<key_t, payload, comparator_t, allocator_type>	map_t;
	map_t m_contents;

	priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	bool get_inner(const typename map_t::volatile_iterator& itor) volatile
	{
		bool result = false;
		rcptr<payload> oldRemoved = itor->m_removed;
		rcptr<payload> newRemoved;
		newRemoved.set_to_mark(1);
		if (!oldRemoved)
		{
			// If anyone else was going to reschedule us, they would only transition this from 0 to obj, not to 1.
			if (itor->m_removed.compare_exchange(newRemoved, oldRemoved, oldRemoved))
				result = true;
		}
		if (!result)
		{
			COGS_ASSERT(!!oldRemoved);
			COGS_ASSERT(oldRemoved.get_mark() == 0);
			if (oldRemoved->m_removed.compare_exchange(newRemoved, oldRemoved, oldRemoved))
				result = true;
			else
				COGS_ASSERT(oldRemoved == newRemoved);
		}

		return result;
	}

public:
	class preallocated_t;
	class value_token;
	class remove_token;

	/// @brief A preallocated priority_queue element
	class preallocated_t
	{
	protected:
		friend class priority_queue;

		typename map_t::preallocated_t m_preallocated;

		payload& get_payload() { return *m_preallocated; }

		preallocated_t(const typename map_t::preallocated_t& i) : m_preallocated(i)	{ }
		preallocated_t& operator=(const typename map_t::preallocated_t& i)	{ m_preallocated = i; return *this; }

	public:
		void disown()	{ m_preallocated.disown(); }
		
		preallocated_t()																{ }
		preallocated_t(const preallocated_t& src) : m_preallocated(src.m_preallocated)	{ }

		void release()										{ m_preallocated.release(); }

		bool operator!() const								{ return !m_preallocated; }
		bool operator==(const preallocated_t& i) const		{ return m_preallocated == i.m_preallocated; }
		bool operator!=(const preallocated_t& i) const		{ return !operator==(i); }
		preallocated_t& operator=(const preallocated_t& i)	{ m_preallocated = i.m_preallocated; return *this; }

		key_t& get_key() const								{ return m_preallocated.get_key(); }
		type& get_value() const								{ return m_preallocated->get_value(); }

		type& operator*() const								{ return m_preallocated->get_value(); }
		type* operator->() const							{ return &(m_preallocated->get_value()); }
		
		rcptr<key_t> get_key_obj() const					{ return m_preallocated.get_key_obj(); }

		rcptr<type> get_value_obj() const
		{
			rcptr<type> result;
			rcptr<payload> obj = m_preallocated.get_value_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<type> get_obj() const							{ return get_value_obj(); }

		rc_obj_base* get_desc() const		{ return m_preallocated.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_preallocated.get_desc(); }
	};

	/// @brief A priority_queue value token
	class value_token
	{
	protected:
		typename map_t::volatile_iterator m_iterator;

		value_token(const typename map_t::volatile_iterator& i) : m_iterator(i)					{ }
		value_token(const typename map_t::volatile_remove_token& rt) : m_iterator(rt)			{ }
		value_token(const volatile typename map_t::volatile_iterator& i) : m_iterator(i)		{ }
		value_token(const volatile typename map_t::volatile_remove_token& rt) : m_iterator(rt)	{ }

		friend class priority_queue;
	public:
		void disown()	{ m_iterator.disown(); }

		value_token() { }
		value_token(const value_token& vt) : m_iterator(vt.m_iterator)	{ }
		value_token(const remove_token& rt) : m_iterator(rt.m_removeToken)	{ }
		value_token(const volatile value_token& vt) : m_iterator(vt.m_iterator)	{ }
		value_token(const volatile remove_token& rt) : m_iterator(rt.m_removeToken)	{ }

		value_token& operator=(const value_token& vt)			{ m_iterator = vt.m_iterator; return *this; }
		value_token& operator=(const volatile value_token& vt)	{ m_iterator = vt.m_iterator; return *this; }
		void operator=(const value_token& vt) volatile			{ m_iterator = vt.m_iterator; }

		value_token& operator=(const remove_token& rt)			{ m_iterator = rt.m_removeToken; return *this; }
		value_token& operator=(const volatile remove_token& rt)	{ m_iterator = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile			{ m_iterator = rt.m_removeToken; }

		bool is_active() const									{ return m_iterator.is_active(); }
		bool is_active() const volatile							{ return m_iterator.is_active(); }

		bool is_removed() const									{ return m_iterator.is_removed(); }
		bool is_removed() const volatile						{ return m_iterator.is_removed(); }

		void release()											{ m_iterator.release(); }
		void release() volatile									{ m_iterator.release(); }

		bool is_removed()										{ return m_iterator.is_removed(); }

		bool operator!() const									{ return !m_iterator; }
		bool operator!() const volatile							{ return !m_iterator; }

		bool operator==(const value_token& vt) const			{ return m_iterator == vt.m_iterator; }
		bool operator==(const volatile value_token& vt) const	{ return m_iterator == vt.m_iterator; }
		bool operator==(const value_token& vt) const volatile	{ return m_iterator == vt.m_iterator; }

		bool operator==(const remove_token& rt) const			{ return m_iterator == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const	{ return m_iterator == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile	{ return m_iterator == rt.m_removeToken; }

		bool operator!=(const value_token& vt) const			{ return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const	{ return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile	{ return !operator==(vt); }

		bool operator!=(const remove_token& rt) const			{ return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const	{ return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile	{ return !operator==(rt); }

		const key_t& get_key() const					{ return m_iterator.get_key(); }
		type& get_value() const							{ return m_iterator->get_value(); }
		type& operator*() const							{ return m_iterator->get_value(); }
		type* operator->() const						{ return &(m_iterator->get_value()); }

		rcptr<const key_t> get_key_obj() const			{ return m_iterator.get_key_obj(); }

		rcptr<type> get_value_obj() const
		{
			rcptr<type> result;
			rcptr<payload> obj = m_iterator.get_value_obj();
			if (!!obj)
			{
				result.set(&(obj->get_value()), obj.get_desc());
				obj.disown();
			}
			return result;
		}

		rcptr<type> get_obj() const	{ return get_value_obj(); }

		rc_obj_base* get_desc() const		{ return m_iterator.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_iterator.get_desc(); }
	};

	/// @brief A priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class priority_queue;

		typename map_t::volatile_remove_token m_removeToken;

		remove_token(const typename map_t::volatile_iterator& i) : m_removeToken(i)	{ }
		remove_token(const typename map_t::volatile_remove_token& rt) : m_removeToken(rt)	{ }
	public:
		remove_token()	{ }
		remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const value_token& vt) : m_removeToken(vt.m_iterator) { }
		remove_token(const volatile remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const volatile value_token& vt) : m_removeToken(vt.m_iterator) { }

		remove_token& operator=(const preallocated_t& i)			{ m_removeToken = i.m_preallocated; return *this; }
		remove_token& operator=(const preallocated_t& i) volatile	{ m_removeToken = i.m_preallocated; return *this; }

		remove_token& operator=(const remove_token& rt)				{ m_removeToken = rt.m_removeToken; return *this; }
		remove_token& operator=(const volatile remove_token& rt)	{ m_removeToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile				{ m_removeToken = rt.m_removeToken; }

		remove_token& operator=(const value_token& vt)				{ m_removeToken = vt.m_iterator; return *this; }
		remove_token& operator=(const volatile value_token& vt)		{ m_removeToken = vt.m_iterator; return *this; }
		void operator=(const value_token& vt) volatile				{ m_removeToken = vt.m_iterator; }

		bool is_active() const										{ return m_removeToken.is_active(); }
		bool is_active() const volatile								{ return m_removeToken.is_active(); }

		void release()												{ m_removeToken.release(); }
		void release() volatile										{ m_removeToken.release(); }

		bool operator!() const										{ return !m_removeToken; }
		bool operator!() const volatile								{ return !m_removeToken; }

		bool operator==(const remove_token& rt) const				{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile		{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const		{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const value_token& vt) const				{ return m_removeToken == vt.m_iterator; }
		bool operator==(const value_token& vt) const volatile		{ return m_removeToken == vt.m_iterator; }
		bool operator==(const volatile value_token& vt) const		{ return m_removeToken == vt.m_iterator; }

		bool operator!=(const remove_token& rt) const				{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const remove_token& rt) const volatile		{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const volatile remove_token& rt) const		{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const value_token& vt) const				{ return m_removeToken != vt.m_iterator; }
		bool operator!=(const value_token& vt) const volatile		{ return m_removeToken != vt.m_iterator; }
		bool operator!=(const volatile value_token& vt) const		{ return m_removeToken != vt.m_iterator; }
	};

	priority_queue() { }

	priority_queue(volatile allocator_type& al) : m_contents(al) { }

	void clear() { m_contents.clear(); }
	void drain() { m_contents.clear(); }
	void drain() volatile					{ m_contents.drain(); }
	bool is_empty() const volatile			{ return m_contents.is_empty(); }
	bool operator!() const volatile			{ return is_empty(); }
	size_t size() const volatile			{ return m_contents.size(); }

	value_token insert(const key_t& k, const type& t) volatile
	{
		typename map_t::preallocated_t i = m_contents.preallocate_key(k);
		i.get_value().construct(t);
		value_token result(m_contents.insert_preallocated(i));
		return result;
	}

	void insert_multiple(size_t n, const key_t& k, const type& t) volatile
	{
		for (size_t i = 0; i < n; i++)
		{
			typename map_t::preallocated_t p = m_contents.preallocate_key(k);
			p.get_value().construct(t);
			m_contents.insert_preallocated(p);
		}
	}


	value_token insert(const key_t& k, const type& t, bool& wasEmpty) volatile
	{
		typename map_t::preallocated_t i = m_contents.preallocate_key(k);
		i.get_value().construct(t);
		value_token result(m_contents.insert_preallocated(i, wasEmpty));
		return result;
	}

	void insert_multiple(size_t n, const key_t& k, const type& t, bool& wasEmpty) volatile
	{
		for (size_t i = 0; i < n; i++)
		{
			typename map_t::preallocated_t p = m_contents.preallocate_key(k);
			p.get_value().construct(t);
			m_contents.insert_preallocated(p, wasEmpty);
		}
	}



	preallocated_t preallocate() const volatile
	{
		preallocated_t i = m_contents.preallocate();
		i.get_payload().construct();
		return i;
	}

	preallocated_t preallocate(const key_t& k, const type& t) const volatile
	{
		preallocated_t i = m_contents.preallocate_key(k);
		i.get_payload().construct(t);
		return i;
	}

	preallocated_t preallocate_key(const key_t& k) const volatile
	{
		preallocated_t i = m_contents.preallocate_key(k);
		i.get_payload().construct();
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
	const rcref<T>& preallocate_with_aux(const key_t& k, const type& t, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_key_with_aux<T>(k, i.m_preallocated, storage);
		i.get_payload().construct(t);
		return storage.dereference();
	}

	template <typename T>
	const rcref<T>& preallocate_key_with_aux(const key_t& k, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		m_contents.template preallocate_key_with_aux<T>(k, i.m_preallocated, storage);
		i.get_payload().construct();
		return storage.dereference();
	}

	value_token insert_preallocated(const preallocated_t& i) volatile { value_token result(m_contents.insert_preallocated(i.m_preallocated)); return result; }
	
	value_token insert_preallocated(const preallocated_t& i, bool& wasEmpty) volatile { value_token result(m_contents.insert_preallocated(i.m_preallocated, wasEmpty)); return result; }

	value_token get() volatile
	{
		value_token result;
		for (;;)
		{
			if (m_contents.is_empty())
				break;
			result.m_iterator = m_contents.pop_first();
			if (!result.m_iterator || get_inner(result.m_iterator))
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile
	{
		value_token result;
		for (;;)
		{
			if (m_contents.is_empty())
				break;
			result.m_iterator = m_contents.get_first();
			if (!result.m_iterator)
				break;
			if (result.m_iterator.get_key() > lowestPriority)
			{
				result.release();
				break;
			}
			if (m_contents.remove(result.m_iterator) && get_inner(result.m_iterator))
				break;
		}
		return result;
	}

	value_token peek() const volatile	{ value_token vt(m_contents.get_first()); return vt; }

	bool change_priority(value_token& vt, const key_t& newPriority) volatile
	{
		bool b = false;

		// Remove risk of a race for priority change of same element.
		// We will replace m_iterator if able to set m_priorityChanged to true.
		// Any callers attempting to change priority prior to m_iterator being replaces, will no-op.
		if (!!vt && vt.m_iterator->m_priorityChanged.compare_exchange(true, false))
		{
			int oldPriority = vt.m_iterator.get_key();
			b = oldPriority == newPriority;
			if (!b)
			{
				typename map_t::volatile_iterator itor;
				rcptr<payload> oldRemoved = vt.m_iterator->m_removed;
				for (;;)
				{
					if (!oldRemoved)
						oldRemoved = vt.m_iterator.get_value_obj();
					else if (oldRemoved.get_mark() != 0)	// Already scheduled/removed, nothing to do.
						break;
					else if (oldRemoved->m_removed.get_mark() != 0)
						break;

					// Add new element at new priority
					preallocated_t i = m_contents.preallocate_key(newPriority);
					i.get_payload().set_removed(std::move(oldRemoved));
					itor = m_contents.insert_preallocated(i.m_preallocated);
					
					// Link old to new element
					vt.m_iterator->m_rescheduledTo = itor;

					// Try to remove the old element
					bool removedOld = m_contents.remove(vt.m_iterator);
					if (removedOld)	// We were able to remove the old one, done
					{
						b = true;
						vt.m_iterator = itor;	// TODO: Does this write to vt.m_iterator need to be atomic?
						break;
					}
					// Failed to remove it.  Must have been scheduled/removed.
					vt.release();	// It's gone, clear vt.

					//  Try to remove the new one, it's superfluous.
					bool removedNew = m_contents.remove(itor);
					if (removedNew)
						break;

					typename map_t::volatile_iterator itor2 = itor->m_rescheduledTo;
					if (!itor2)
					{
						// Both fired.  Did we succeed in changing the priority?  Let's say if the priority was raised, it doesn't matter, Yes.
						// But if the priority was lowered, and it was the higher priority that expired, we would be lying about having
						// successfully deprioritized it.  So, No.
						b = (newPriority < oldPriority);	// less than - 'better' priority is 'first'/lesser value
						break;
					}
					
					// If, upon failing to remove it, it had been successfully rescheduled, we need to follow the trail and remove them all, until one fails.
					while (!!m_contents.remove(itor2))
						itor2 = itor2->m_rescheduledTo;
					b = true;
					break;
				}
			}
		}

		return b;
	}

	bool change_priority(remove_token& rt, const key_t& newPriority) volatile
	{
		value_token vt(rt.m_removeToken);
		if (change_priority(vt, newPriority))
		{
			rt = vt;
			return true;
		}
		return false;
	}

	bool remove(const value_token& vt) volatile		{ return m_contents.remove(vt.m_iterator); }
	bool remove(const remove_token& rt) volatile	{ return m_contents.remove(rt.m_removeToken); }
};

template <typename key_t, class comparator_t, class allocator_type>
class priority_queue<key_t, void, comparator_t, allocator_type>
{
private:
	typedef priority_queue<key_t, void, comparator_t, allocator_type> this_t;

	class payload
	{
	public:
		volatile boolean m_priorityChanged;	// Used to synchronize concurrent priority changes
		volatile rcptr<payload> m_removed;	// Used to synchronized priority changes with concurrent scheduled removals (gets)
		volatile typename multimap<key_t, payload, comparator_t, allocator_type>::volatile_remove_token m_rescheduledTo;
	
		void set_removed(rcptr<payload>&& removed)
		{
			// Do not need volatility here, so cast it away
			*(rcptr<payload>*)&m_removed = std::move(removed);
		}
	};

	typedef multimap<key_t, payload, comparator_t, allocator_type>	map_t;
	map_t m_contents;

	priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	bool get_inner(const typename map_t::volatile_iterator& itor) volatile
	{
		bool result = false;
		rcptr<volatile boolean> oldRemoved = itor->m_removed;
		if (!oldRemoved)
		{
			rcptr<volatile boolean> newRemoved;
			newRemoved.set_to_mark(1);	// If anyone else was going to reschedule us, they would only transition this from 0 to obj, not to 1.
			if (itor->m_removed.compare_exchange(newRemoved, oldRemoved, oldRemoved))
				result = true;
		}
		if (!result)
		{
			COGS_ASSERT(!!oldRemoved);
			COGS_ASSERT(oldRemoved.get_mark() == 0);
			if (oldRemoved->compare_exchange(true, false))
				result = true;
		}

		return result;
	}

public:
	class preallocated_t;
	class remove_token;
	class value_token;

	/// @brief A preallocated priority_queue element
	class preallocated_t
	{
	protected:
		typename map_t::preallocated_t m_preallocated;

		preallocated_t(const typename map_t::preallocated_t& i) : m_preallocated(i)		{ }

		friend class priority_queue;

	public:
		void disown()	{ m_preallocated.disown(); }

		preallocated_t()																{ }
		preallocated_t(const preallocated_t& src) : m_preallocated(src.m_preallocated)	{ }

		void release()										{ m_preallocated.release(); }

		bool operator!() const								{ return !m_preallocated; }
		bool operator==(const preallocated_t& i) const		{ return m_preallocated == i.m_preallocated; }
		bool operator!=(const preallocated_t& i) const		{ return !operator==(i); }
		preallocated_t& operator=(const preallocated_t& i)	{ m_preallocated = i.m_preallocated; return *this; }

		key_t& operator*() const							{ return *m_preallocated; }
		key_t* operator->() const							{ return &*m_preallocated; }

		rcptr<key_t> get_obj() const						{ return m_preallocated.get_key_obj(); }

		rc_obj_base* get_desc() const		{ return m_preallocated.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_preallocated.get_desc(); }
	};

	/// @brief A priority_queue value token
	class value_token
	{
	protected:
		typename map_t::volatile_iterator m_iterator;

		value_token(const typename map_t::volatile_iterator& i) : m_iterator(i)					{ }
		value_token(const typename map_t::volatile_remove_token& rt) : m_iterator(rt)			{ }
		value_token(const volatile typename map_t::volatile_iterator& i) : m_iterator(i)		{ }
		value_token(const volatile typename map_t::volatile_remove_token& rt) : m_iterator(rt)	{ }

		friend class priority_queue;
	public:
		void disown()	{ m_iterator.disown(); }

		value_token() { }
		value_token(const value_token& vt) : m_iterator(vt.m_iterator)	{ }
		value_token(const remove_token& rt) : m_iterator(rt.m_removeToken)	{ }
		value_token(const volatile value_token& vt) : m_iterator(vt.m_iterator)	{ }
		value_token(const volatile remove_token& rt) : m_iterator(rt.m_removeToken)	{ }

		value_token& operator=(const value_token& vt)			{ m_iterator = vt.m_iterator; return *this; }
		value_token& operator=(const volatile value_token& vt)	{ m_iterator = vt.m_iterator; return *this; }
		void operator=(const value_token& vt) volatile			{ m_iterator = vt.m_iterator; }

		value_token& operator=(const remove_token& rt)			{ m_iterator = rt.m_removeToken; return *this; }
		value_token& operator=(const volatile remove_token& rt)	{ m_iterator = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile			{ m_iterator = rt.m_removeToken; }

		bool is_active() const									{ return m_iterator.is_active(); }
		bool is_active() const volatile							{ return m_iterator.is_active(); }

		bool is_removed() const									{ return m_iterator.is_removed(); }
		bool is_removed() const volatile						{ return m_iterator.is_removed(); }

		void release()											{ m_iterator.release(); }
		void release() volatile									{ m_iterator.release(); }

		bool is_removed()										{ return m_iterator.is_removed(); }

		bool operator!() const									{ return !m_iterator; }
		bool operator!() const volatile							{ return !m_iterator; }

		bool operator==(const value_token& vt) const			{ return m_iterator == vt.m_iterator; }
		bool operator==(const volatile value_token& vt) const	{ return m_iterator == vt.m_iterator; }
		bool operator==(const value_token& vt) const volatile	{ return m_iterator == vt.m_iterator; }

		bool operator==(const remove_token& rt) const			{ return m_iterator == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const	{ return m_iterator == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile	{ return m_iterator == rt.m_removeToken; }

		bool operator!=(const value_token& vt) const			{ return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const	{ return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile	{ return !operator==(vt); }

		bool operator!=(const remove_token& rt) const			{ return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const	{ return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile	{ return !operator==(rt); }

		const key_t& operator*() const							{ return m_iterator.get_key(); }
		const key_t* operator->() const							{ return &(m_iterator.get_key()); }

		rcptr<const key_t> get_obj() const						{ return m_iterator.get_key_obj(); }

		rc_obj_base* get_desc() const		{ return m_iterator.get_desc(); }
		rc_obj_base* get_desc() const volatile		{ return m_iterator.get_desc(); }
	};

	/// @brief A preallocated element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class priority_queue;

		typename map_t::volatile_remove_token m_removeToken;

		remove_token(const typename map_t::volatile_iterator& i) : m_removeToken(i)	{ }
		remove_token(const typename map_t::volatile_remove_token& rt) : m_removeToken(rt)	{ }
	public:
		remove_token()	{ }
		remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const value_token& vt) : m_removeToken(vt.m_iterator) { }
		remove_token(const volatile remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const volatile value_token& vt) : m_removeToken(vt.m_iterator) { }

		remove_token& operator=(const preallocated_t& i)			{ m_removeToken = i.m_preallocated; return *this; }
		remove_token& operator=(const preallocated_t& i) volatile	{ m_removeToken = i.m_preallocated; return *this; }

		remove_token& operator=(const remove_token& rt)				{ m_removeToken = rt.m_removeToken; return *this; }
		remove_token& operator=(const volatile remove_token& rt)	{ m_removeToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile				{ m_removeToken = rt.m_removeToken; }

		remove_token& operator=(const value_token& vt)				{ m_removeToken = vt.m_iterator; return *this; }
		remove_token& operator=(const volatile value_token& vt)		{ m_removeToken = vt.m_iterator; return *this; }
		void operator=(const value_token& vt) volatile				{ m_removeToken = vt.m_iterator; }

		bool is_active() const										{ return m_removeToken.is_active(); }
		bool is_active() const volatile								{ return m_removeToken.is_active(); }

		void release()												{ m_removeToken.release(); }
		void release() volatile										{ m_removeToken.release(); }

		bool operator!() const										{ return !m_removeToken; }
		bool operator!() const volatile								{ return !m_removeToken; }

		bool operator==(const remove_token& rt) const				{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile		{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const		{ return m_removeToken == rt.m_removeToken; }
		bool operator==(const value_token& vt) const				{ return m_removeToken == vt.m_iterator; }
		bool operator==(const value_token& vt) const volatile		{ return m_removeToken == vt.m_iterator; }
		bool operator==(const volatile value_token& vt) const		{ return m_removeToken == vt.m_iterator; }

		bool operator!=(const remove_token& rt) const				{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const remove_token& rt) const volatile		{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const volatile remove_token& rt) const		{ return m_removeToken != rt.m_removeToken; }
		bool operator!=(const value_token& vt) const				{ return m_removeToken != vt.m_iterator; }
		bool operator!=(const value_token& vt) const volatile		{ return m_removeToken != vt.m_iterator; }
		bool operator!=(const volatile value_token& vt) const		{ return m_removeToken != vt.m_iterator; }
	};

	priority_queue(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	priority_queue() { }

	priority_queue(volatile allocator_type& al) : m_contents(al) { }

	void clear()							{ m_contents.clear(); }
	void drain() volatile					{ m_contents.drain(); }
	bool is_empty() const volatile			{ return m_contents.is_empty(); }
	bool operator!() const volatile			{ return is_empty(); }
	size_t size() const volatile			{ return m_contents.size(); }

	value_token insert(const key_t& k) volatile
	{
		value_token result(m_contents.insert_preallocated(m_contents.preallocate_key(k)));
		return result;
	}

	void insert_multiple(size_t n, const key_t& k) volatile
	{
		for (size_t i = 0; i < n; i++)
			m_contents.insert_preallocated(m_contents.preallocate_key(k));
	}

	preallocated_t preallocate() const volatile { return preallocated_t(m_contents.preallocate()); }
	preallocated_t preallocate(const key_t& k) const volatile { return preallocated_t(m_contents.preallocate_key(k)); }

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated_t& i, rcref<T>& storage = unowned_t<rcref<T> >().get_unowned()) const volatile
	{
		return m_contents.template preallocate_with_aux<T>(i.m_preallocated, storage);
	}
	
	template <typename T>
	const rcref<T>& preallocate_with_aux(const key_t& k, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_contents.template preallocate_key_with_aux<T>(k, i.m_preallocated, storage);
	}

	value_token insert_preallocated(const preallocated_t& i) volatile			{ value_token result(m_contents.insert_preallocated(i.m_preallocated)); return result; }

	value_token get() volatile
	{
		value_token result;
		for (;;)
		{
			result.m_iterator = m_contents.pop_first();
			if (!result.m_iterator || get_inner(result.m_iterator))
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile
	{
		value_token result;
		for (;;)
		{
			result.m_iterator = m_contents.get_first();
			if (!result.m_iterator)
				break;
			if (result.m_iterator.get_key() > lowestPriority)
			{
				result.release();
				break;
			}
			if (m_contents.remove(result.m_iterator) && get_inner(result.m_iterator))
				break;
		}
		return result;
	}

	value_token peek() const volatile { value_token vt(m_contents.get_first()); return vt; }

	bool change_priority(value_token& vt, const key_t& newPriority) volatile
	{
		bool b = false;
		
		// Remove risk of a race for priority change of same element.
		// We will replace m_iterator if able to set m_priorityChanged to true.
		// Any callers attempting to change priority prior to m_iterator being replaces, will no-op.
		if (!!vt && vt.m_iterator->m_priorityChanged.compare_exchange(true, false))
		{
			int oldPriority = vt.m_iterator.get_key();
			b = oldPriority == newPriority;
			if (!b)
			{
				typename map_t::volatile_iterator itor;
				rcptr<volatile boolean> newRemoved;
				rcptr<volatile boolean> oldRemoved = vt.m_iterator->m_removed;
				for (;;)
				{
					if (!oldRemoved)
						oldRemoved = vt.get_value_obj();
					else if (oldRemoved.get_mark() != 0)	// Already scheduled/removed, nothing to do.
						break;

					// Add new element at new priority
					preallocated_t i = m_contents.preallocate_key(newPriority);
					i.get_payload().set_removed(oldRemoved);
					itor = m_contents.insert_preallocated(i.m_preallocated);

					// Link old to new element
					vt.m_iterator->m_rescheduledTo = itor;

					// Try to remove the old element
					bool removedOld = m_contents.remove(vt.m_iterator);
					if (removedOld)	// We were able to remove the old one, done
					{
						b = true;
						vt.m_iterator = itor;	// TODO: Does this write to vt.m_iterator need to be atomic?
						break;
					}
					// Failed to remove it.  Must have been scheduled/removed.
					vt.release();	// It's gone, clear vt.

					// Try to remove the new one, it's superfluous.
					bool removedNew = m_contents.remove(itor);
					if (removedNew)
						break;

					typename map_t::volatile_iterator itor2 = itor->m_rescheduledTo;
					if (!itor2)
					{
						// Both fired.  Did we succeed in changing the priority?  Let's say if the priority was raised, it doesn't matter, Yes.
						// But if the priority was lowered, and it was the higher priority that expired, we would be lying about having
						// successfully deprioritized it.  So, No.
						b = (newPriority < oldPriority);	// less than - 'better' priority is 'first'/lesser value
						break;
					}

					// If, upon failing to remove it, it had been successfully rescheduled, we need to follow the trail and remove them all, until one fails.
					while (!!m_contents.remove(itor2))
						itor2 = itor2->m_rescheduledTo;
					b = true;
					break;
				}
			}
		}

		return b;
	}

	bool change_priority(remove_token& rt, const key_t& newPriority) volatile
	{
		value_token vt(rt.m_removeToken);
		if (change_priority(vt, newPriority))
		{
			rt = vt;
			return true;
		}
		return false;
	}

	bool remove(const value_token& vt) volatile		{ return m_contents.remove(vt.m_iterator); }
	bool remove(const remove_token& rt) volatile	{ return m_contents.remove(rt.m_removeToken); }
};


}


#endif








