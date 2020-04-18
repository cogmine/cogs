//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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

		// Used to synchronize concurrent priority changes.
		volatile boolean m_priorityChanged;

		// Used to synchronized priority changes with concurrent scheduled removals (gets)
		// m_removed is set to 1 (marked 0) when removed, or to the original element that has been rescheduled (using this one)
		// If pointing to an payload, it will always be the original element, not a link in a reschedule chain.
		volatile rcptr<payload> m_removed;

		volatile typename multimap<key_t, payload, comparator_t, allocator_type>::volatile_remove_token m_rescheduledTo;

		void construct(const type& t) { m_value.construct(t); }

		void construct() { m_value.construct(); }

		payload() {}

		explicit payload(const rcptr<payload>& removed)
			: m_removed(removed)
		{ }

		explicit payload(rcptr<payload>&& removed)
			: m_removed(std::move(removed))
		{ }

		~payload()
		{
			// An element with an m_removed set to a payload (not just a 1), does not construct its own payload contents
			if (!const_cast<rcptr<payload>*>(&m_removed)->get_unmarked())
				m_value.destruct();
		}

		type& get_value()
		{
			rcptr<payload> removed = m_removed; // If resheduled, we refer to the original object in m_removed
			if (!!removed && removed.get_mark() == 0)
				return removed->m_value.get();
			return m_value.get();
		}
	};

	typedef multimap<key_t, payload, comparator_t, allocator_type> map_t;
	map_t m_contents;

	priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	bool get_inner(const typename map_t::volatile_iterator& itor) volatile
	{
		bool result = false;
		rcptr<payload> oldRemoved = itor->value.m_removed;
		rcptr<payload> newRemoved;
		newRemoved.set_to_mark(1);
		if (!oldRemoved)
		{
			// We only call get_inner() if we successfully removed the element, so no-one else will transition it to 1.
			// If another thread were to change priority, they would transition this from 0 to obj, not to 1.
			if (itor->value.m_removed.compare_exchange(newRemoved, oldRemoved, oldRemoved))
				result = true;
		}
		if (!result)
		{
			COGS_ASSERT(!!oldRemoved);
			COGS_ASSERT(oldRemoved.get_mark() == 0);
			rcptr<payload> empty;
			if (oldRemoved->m_removed.compare_exchange(newRemoved, empty, empty))
				result = true;
			else
				COGS_ASSERT(newRemoved == empty);
		}
		return result;
	}

public:
	class value_token;
	class remove_token;

	/// @brief A priority_queue value token
	class value_token
	{
	protected:
		typename map_t::volatile_iterator m_contents;

		value_token(const typename map_t::iterator& i) : m_contents(i) { }
		value_token(const typename map_t::remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename map_t::iterator& i) : m_contents(i) { }
		value_token(const volatile typename map_t::remove_token& rt) : m_contents(rt) { }
		value_token(typename map_t::iterator&& i) : m_contents(std::move(i)) { }

		value_token(const typename map_t::volatile_iterator& i) : m_contents(i) { }
		value_token(const typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename map_t::volatile_iterator& i) : m_contents(i) { }
		value_token(const volatile typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		value_token(typename map_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		value_token& operator=(const typename map_t::iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename map_t::iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const volatile typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename map_t::iterator&& i) { m_contents = std::move(i); return *this; }

		value_token& operator=(const typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const volatile typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename map_t::volatile_iterator&& i) { m_contents = std::move(i); return *this; }

		friend class priority_queue;

	public:
		value_token() { }
		value_token(const value_token& vt) : m_contents(vt.m_contents) { }
		value_token(const remove_token& rt) : m_contents(rt.m_contents) { }
		value_token(const volatile value_token& vt) : m_contents(vt.m_contents) { }
		value_token(const volatile remove_token& rt) : m_contents(rt.m_contents) { }
		value_token(value_token&& vt) : m_contents(std::move(vt.m_contents)) { }

		value_token& operator=(const value_token& vt) { m_contents = vt.m_contents; return *this; }
		value_token& operator=(const volatile value_token& vt) { m_contents = vt.m_contents; return *this; }
		volatile value_token& operator=(const value_token& vt) volatile { m_contents = vt.m_contents; return *this; }
		value_token& operator=(value_token&& vt) { m_contents = std::move(vt.m_contents); return *this; }
		volatile value_token& operator=(value_token&& vt) volatile { m_contents = std::move(vt.m_contents); return *this; }

		value_token& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		value_token& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		volatile value_token& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool is_removed() const { return m_contents.is_removed(); }
		bool is_removed() const volatile { return m_contents.is_removed(); }

		bool is_removed() { return m_contents.is_removed(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const volatile value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const value_token& vt) const volatile { return m_contents == vt.m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }

		bool operator!=(const value_token& vt) const { return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const { return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile { return !operator==(vt); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		const key_t& get_priority() const { return m_contents->key; }

		type& operator*() const { return m_contents->value.get_value(); }
		type* operator->() const { return &m_contents->value.get_value(); }

		const rcptr<const key_t>& get_priority_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			return m_contents.get_key_obj(storage);
		}

		const rcptr<type>& get_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
		{
			if (!!m_contents)
				storage.set(&m_contents->value.get_value(), m_contents.get_desc());
			return storage;
		}
	};

	/// @brief A priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class priority_queue;

		typename map_t::volatile_remove_token m_contents;

		remove_token(const typename map_t::iterator& i) : m_contents(i) { }
		remove_token(const typename map_t::remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename map_t::iterator& i) : m_contents(i) { }
		remove_token(const volatile typename map_t::remove_token& rt) : m_contents(rt) { }
		remove_token(typename map_t::remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token(const typename map_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(const typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename map_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(const volatile typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		remove_token(typename map_t::volatile_remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token& operator=(const typename map_t::iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename map_t::iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const volatile typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename map_t::remove_token&& rt) { m_contents = std::move(rt); return *this; }

		remove_token& operator=(const typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const volatile typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename map_t::volatile_remove_token&& rt) { m_contents = std::move(rt); return *this; }

	public:
		remove_token() { }
		remove_token(const remove_token& rt) : m_contents(rt.m_contents) { }
		remove_token(const value_token& vt) : m_contents(vt.m_contents) { }
		remove_token(const volatile remove_token& rt) : m_contents(rt.m_contents) { }
		remove_token(const volatile value_token& vt) : m_contents(vt.m_contents) { }
		remove_token(remove_token&& rt) : m_contents(std::move(rt.m_contents)) { }

		remove_token& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		volatile remove_token& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(remove_token&& rt) { m_contents = std::move(rt.m_contents); return *this; }
		volatile remove_token& operator=(remove_token&& rt) volatile { m_contents = std::move(rt.m_contents); return *this; }

		remove_token& operator=(const value_token& vt) { m_contents = vt.m_contents; return *this; }
		remove_token& operator=(const volatile value_token& vt) { m_contents = vt.m_contents; return *this; }
		volatile remove_token& operator=(const value_token& vt) volatile { m_contents = vt.m_contents; return *this; }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const value_token& vt) const volatile { return m_contents == vt.m_contents; }
		bool operator==(const volatile value_token& vt) const { return m_contents == vt.m_contents; }

		bool operator!=(const remove_token& rt) const { return m_contents != rt.m_contents; }
		bool operator!=(const remove_token& rt) const volatile { return m_contents != rt.m_contents; }
		bool operator!=(const volatile remove_token& rt) const { return m_contents != rt.m_contents; }
		bool operator!=(const value_token& vt) const { return m_contents != vt.m_contents; }
		bool operator!=(const value_token& vt) const volatile { return m_contents != vt.m_contents; }
		bool operator!=(const volatile value_token& vt) const { return m_contents != vt.m_contents; }
	};

	priority_queue() { }

	priority_queue(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	explicit priority_queue(volatile allocator_type& al) : m_contents(al) { }

	void clear() { m_contents.clear(); }
	bool drain() { return m_contents.drain(); }
	bool drain() volatile { return m_contents.drain(); }
	bool is_empty() const volatile { return m_contents.is_empty(); }
	bool operator!() const volatile { return is_empty(); }
	size_t size() const volatile { return m_contents.size(); }

	struct insert_result
	{
		value_token valueToken;
		bool wasEmpty;
	};

	template <typename F>
	insert_result insert_via(F&& f) volatile
	{
		value_token vt;
		auto p = m_contents.insert_via([&](typename map_t::iterator& i)
		{
			new (&i->value) payload;
			vt = std::move(i);
			f(vt);
		});
		return { std::move(vt), p.wasEmpty };
	}

	insert_result insert(const key_t& k, const type& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(k);
			placement_rcnew(&vt.m_contents->value, *vt.get_desc())(v);
		});
	}

	insert_result insert(key_t&& k, const type& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(std::move(k));
			placement_rcnew(&vt.m_contents->value, *vt.get_desc())(v);
		});
	}

	insert_result insert(const key_t& k, type&& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(k);
			placement_rcnew(&vt.m_contents->value, *vt.get_desc())(std::move(v));
		});
	}

	insert_result insert(key_t&& k, type&& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(std::move(k));
			placement_rcnew(&vt.m_contents->value, *vt.get_desc())(std::move(v));
		});
	}

	value_token get() volatile
	{
		value_token result;
		for (;;)
		{
			if (m_contents.is_empty())
				break;
			result.m_contents = m_contents.pop_first().iterator;
			if (!result.m_contents || get_inner(result.m_contents))
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
			result.m_contents = m_contents.get_first();
			if (!result.m_contents)
				break;
			if (result.m_contents->key > lowestPriority)
			{
				result.release();
				break;
			}
			if (m_contents.remove(result.m_contents).wasRemoved && get_inner(result.m_contents))
				break;
		}
		return result;
	}

	value_token peek() const volatile { value_token vt(m_contents.get_first()); return vt; }

	// Changing priority invalidates any existing tokens referencing the element,
	// except for the value_token passed in, which is updated to refer to the new element.
	bool change_priority(value_token& vt, const key_t& newPriority) volatile
	{
		bool b = false;

		// Remove risk of a race for priority change of same element.
		if (!!vt)
		{
			key_t oldPriority = vt.m_contents->key;
			b = oldPriority == newPriority;

			// We will replace m_contents if able to set m_priorityChanged to true.
			// Any callers attempting to change priority prior to m_contents being replaces, will no-op.
			if (!b && vt.m_contents->value.m_priorityChanged.compare_exchange(true, false))
			{
				rcptr<payload> oldRemoved = vt.m_contents->value.m_removed;
				for (;;)
				{
					// Another thread might remove this node, but we will detect that later as removal failure.
					if (!oldRemoved)
						oldRemoved = vt.m_contents.get_value_obj();
					else if (oldRemoved.get_mark() != 0) // Already scheduled/removed, nothing to do.
						break;
					else if (oldRemoved->m_removed.get_mark() != 0)
						break;

					// Add new element at new priority
					typename map_t::volatile_iterator itor = m_contents.insert_via([&](typename map_t::iterator& i)
					{
						new (const_cast<key_t*>(&i->key)) key_t(newPriority);
						new (&i->value) payload(std::move(oldRemoved));
					}).inserted;

					// Link old to new element
					vt.m_contents->value.m_rescheduledTo = itor;

					// Try to remove the old element
					bool removedOld = m_contents.remove(vt.m_contents).wasRemoved;
					if (removedOld) // We were able to remove the old one, done
					{
						b = true;
						vt.m_contents = itor;
						break;
					}
					// Failed to remove it.  Must have been scheduled/removed.
					vt.release(); // It's gone, clear vt.

					//  Try to remove the new one, it's superfluous.
					bool removedNew = m_contents.remove(itor).wasRemoved;
					if (removedNew)
						break;

					typename map_t::volatile_iterator itor2 = itor->value.m_rescheduledTo;
					if (!itor2)
					{
						// Both fired.  Did we succeed in changing the priority?  Let's say if the priority was raised, it doesn't matter, Yes.
						// But if the priority was lowered, and it was the higher priority that expired, we would be lying about having
						// successfully deprioritized it.  So, No.
						b = (newPriority < oldPriority); // less than - 'better' priority is 'first'/lesser value
						break;
					}

					// If, upon failing to remove it, it had been successfully rescheduled, we need to follow the trail and remove them all, until one fails.
					while (m_contents.remove(itor2).wasRemoved)
						itor2 = itor2->value.m_rescheduledTo;
					b = true;
					break;
				}
			}
		}

		return b;
	}

	bool change_priority(remove_token& rt, const key_t& newPriority) volatile
	{
		value_token vt(rt.m_contents);
		if (change_priority(vt, newPriority))
		{
			rt = vt;
			return true;
		}
		return false;
	}

	typedef typename map_t::volatile_remove_result remove_result;

	remove_result remove(const value_token& vt) volatile { return m_contents.remove(vt.m_contents); }
	remove_result remove(const remove_token& rt) volatile { return m_contents.remove(rt.m_contents); }
};

template <typename key_t, class comparator_t, class allocator_type>
class priority_queue<key_t, void, comparator_t, allocator_type>
{
private:
	typedef priority_queue<key_t, void, comparator_t, allocator_type> this_t;

	class payload
	{
	public:
		// Used to synchronize concurrent priority changes.
		volatile boolean m_priorityChanged;

		// Used to synchronized priority changes with concurrent scheduled removals (gets)
		// m_removed is set to 1 (marked 0) when removed, or to the original element that has been rescheduled (using this one).
		// If pointing to an payload, it will always be the original element, not a link in a reschedule chain.
		volatile rcptr<payload> m_removed;

		volatile typename multimap<key_t, payload, comparator_t, allocator_type>::volatile_remove_token m_rescheduledTo;

		payload() {}

		explicit payload(const rcptr<payload>& removed)
			: m_removed(removed)
		{ }

		explicit payload(rcptr<payload>&& removed)
			: m_removed(std::move(removed))
		{ }
	};

	typedef multimap<key_t, payload, comparator_t, allocator_type> map_t;
	map_t m_contents;

	priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	bool get_inner(const typename map_t::volatile_iterator& itor) volatile
	{
		bool result = false;
		rcptr<payload> oldRemoved = itor->value.m_removed;
		rcptr<payload> newRemoved;
		newRemoved.set_to_mark(1);
		if (!oldRemoved)
		{
			// We only call get_inner() if we successfully removed the element, so no-one else will transition it to 1.
			// If another thread were to change priority, they would transition this from 0 to obj, not to 1.
			if (itor->value.m_removed.compare_exchange(newRemoved, oldRemoved, oldRemoved))
				result = true;
		}
		if (!result)
		{
			COGS_ASSERT(!!oldRemoved);
			COGS_ASSERT(oldRemoved.get_mark() == 0);
			rcptr<payload> empty;
			if (oldRemoved->m_removed.compare_exchange(newRemoved, empty, empty))
				result = true;
			else
				COGS_ASSERT(newRemoved == empty);
		}
		return result;
	}

public:
	class remove_token;
	class value_token;

	/// @brief A priority_queue value token
	class value_token
	{
	protected:
		typename map_t::volatile_iterator m_contents;

		value_token(const typename map_t::iterator& i) : m_contents(i) { }
		value_token(const typename map_t::remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename map_t::iterator& i) : m_contents(i) { }
		value_token(const volatile typename map_t::remove_token& rt) : m_contents(rt) { }
		value_token(typename map_t::iterator&& i) : m_contents(std::move(i)) { }

		value_token(const typename map_t::volatile_iterator& i) : m_contents(i) { }
		value_token(const typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename map_t::volatile_iterator& i) : m_contents(i) { }
		value_token(const volatile typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		value_token(typename map_t::volatile_iterator&& i) : m_contents(std::move(i)) { }

		value_token& operator=(const typename map_t::iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename map_t::iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const volatile typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename map_t::iterator&& i) { m_contents = std::move(i); return *this; }

		value_token& operator=(const typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		value_token& operator=(const volatile typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename map_t::volatile_iterator&& i) { m_contents = std::move(i); return *this; }

		friend class priority_queue;

	public:
		value_token() { }
		value_token(const value_token& vt) : m_contents(vt.m_contents) { }
		value_token(const remove_token& rt) : m_contents(rt.m_contents) { }
		value_token(const volatile value_token& vt) : m_contents(vt.m_contents) { }
		value_token(const volatile remove_token& rt) : m_contents(rt.m_contents) { }
		value_token(value_token&& vt) : m_contents(std::move(vt.m_contents)) { }

		value_token& operator=(const value_token& vt) { m_contents = vt.m_contents; return *this; }
		value_token& operator=(const volatile value_token& vt) { m_contents = vt.m_contents; return *this; }
		volatile value_token& operator=(const value_token& vt) volatile { m_contents = vt.m_contents; return *this; }
		value_token& operator=(value_token&& vt) { m_contents = std::move(vt.m_contents); return *this; }
		volatile value_token& operator=(value_token&& vt) volatile { m_contents = std::move(vt.m_contents); return *this; }

		value_token& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		value_token& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		volatile value_token& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }

		void disown() { m_contents.disown(); }
		void disown() volatile { m_contents.disown(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		bool is_removed() const { return m_contents.is_removed(); }
		bool is_removed() const volatile { return m_contents.is_removed(); }

		bool is_removed() { return m_contents.is_removed(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const volatile value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const value_token& vt) const volatile { return m_contents == vt.m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }

		bool operator!=(const value_token& vt) const { return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const { return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile { return !operator==(vt); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		rc_obj_base* get_desc() const { return m_contents.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_contents.get_desc(); }

		const key_t& operator*() const { return m_contents->key; }
		const key_t* operator->() const { return &m_contents->value; }

		const rcptr<const key_t>& get_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			return m_contents.get_key_obj(storage);
		}
	};

	/// @brief A priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class priority_queue;

		typename map_t::volatile_remove_token m_contents;

		remove_token(const typename map_t::iterator& i) : m_contents(i) { }
		remove_token(const typename map_t::remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename map_t::iterator& i) : m_contents(i) { }
		remove_token(const volatile typename map_t::remove_token& rt) : m_contents(rt) { }
		remove_token(typename map_t::remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token(const typename map_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(const typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename map_t::volatile_iterator& i) : m_contents(i) { }
		remove_token(const volatile typename map_t::volatile_remove_token& rt) : m_contents(rt) { }
		remove_token(typename map_t::volatile_remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token& operator=(const typename map_t::iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename map_t::iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const volatile typename map_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename map_t::remove_token&& rt) { m_contents = std::move(rt); return *this; }

		remove_token& operator=(const typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename map_t::volatile_iterator& i) { m_contents = i; return *this; }
		remove_token& operator=(const volatile typename map_t::volatile_remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename map_t::volatile_remove_token&& rt) { m_contents = std::move(rt); return *this; }

	public:
		remove_token() { }
		remove_token(const remove_token& rt) : m_contents(rt.m_contents) { }
		remove_token(const value_token& vt) : m_contents(vt.m_contents) { }
		remove_token(const volatile remove_token& rt) : m_contents(rt.m_contents) { }
		remove_token(const volatile value_token& vt) : m_contents(vt.m_contents) { }
		remove_token(remove_token&& rt) : m_contents(std::move(rt.m_contents)) { }

		remove_token& operator=(const remove_token& rt) { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_contents = rt.m_contents; return *this; }
		volatile remove_token& operator=(const remove_token& rt) volatile { m_contents = rt.m_contents; return *this; }
		remove_token& operator=(remove_token&& rt) { m_contents = std::move(rt.m_contents); return *this; }
		volatile remove_token& operator=(remove_token&& rt) volatile { m_contents = std::move(rt.m_contents); return *this; }

		remove_token& operator=(const value_token& vt) { m_contents = vt.m_contents; return *this; }
		remove_token& operator=(const volatile value_token& vt) { m_contents = vt.m_contents; return *this; }
		volatile remove_token& operator=(const value_token& vt) volatile { m_contents = vt.m_contents; return *this; }

		bool is_active() const { return m_contents.is_active(); }
		bool is_active() const volatile { return m_contents.is_active(); }

		void release() { m_contents.release(); }
		void release() volatile { m_contents.release(); }

		bool operator!() const { return !m_contents; }
		bool operator!() const volatile { return !m_contents; }

		bool operator==(const remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const remove_token& rt) const volatile { return m_contents == rt.m_contents; }
		bool operator==(const volatile remove_token& rt) const { return m_contents == rt.m_contents; }
		bool operator==(const value_token& vt) const { return m_contents == vt.m_contents; }
		bool operator==(const value_token& vt) const volatile { return m_contents == vt.m_contents; }
		bool operator==(const volatile value_token& vt) const { return m_contents == vt.m_contents; }

		bool operator!=(const remove_token& rt) const { return m_contents != rt.m_contents; }
		bool operator!=(const remove_token& rt) const volatile { return m_contents != rt.m_contents; }
		bool operator!=(const volatile remove_token& rt) const { return m_contents != rt.m_contents; }
		bool operator!=(const value_token& vt) const { return m_contents != vt.m_contents; }
		bool operator!=(const value_token& vt) const volatile { return m_contents != vt.m_contents; }
		bool operator!=(const volatile value_token& vt) const { return m_contents != vt.m_contents; }
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

	explicit priority_queue(volatile allocator_type& al) : m_contents(al) { }

	void clear() { m_contents.clear(); }
	bool drain() { return m_contents.drain(); }
	bool drain() volatile { return m_contents.drain(); }
	bool is_empty() const volatile { return m_contents.is_empty(); }
	bool operator!() const volatile { return is_empty(); }
	size_t size() const volatile { return m_contents.size(); }

	struct insert_result
	{
		value_token valueToken;
		bool wasEmpty;
	};

	template <typename F>
	insert_result insert_via(F&& f) volatile
	{
		value_token vt;
		auto p = m_contents.insert([&](typename map_t::iterator& i)
		{
			vt = std::move(i);
			f(vt);
		});
		return { std::move(vt), p.wasEmpty };
	}

	insert_result insert(const key_t& k) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(k);
		});
	}

	insert_result insert(key_t&& k) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->key), *vt.get_desc())(std::move(k));
		});
	}

	value_token get() volatile
	{
		value_token result;
		for (;;)
		{
			result.m_contents = m_contents.pop_first().iterator;
			if (!result.m_contents || get_inner(result.m_contents))
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile
	{
		value_token result;
		for (;;)
		{
			result.m_contents = m_contents.get_first();
			if (!result.m_contents)
				break;
			if (result.m_contents->key > lowestPriority)
			{
				result.release();
				break;
			}
			if (m_contents.remove(result.m_contents).wasRemoved && get_inner(result.m_contents))
				break;
		}
		return result;
	}

	value_token peek() const volatile { value_token vt(m_contents.get_first()); return vt; }

	// Changing priority invalidates any existing tokens referencing the element,
	// except for the value_token passed in, which is updated to refer to the new element.
	bool change_priority(value_token& vt, const key_t& newPriority) volatile
	{
		bool b = false;

		// Remove risk of a race for priority change of same element.
		if (!!vt)
		{
			key_t oldPriority = vt.m_contents->key;
			b = oldPriority == newPriority;

			// We will replace m_contents if able to set m_priorityChanged to true.
			// Any callers attempting to change priority prior to m_contents being replaces, will no-op.
			if (!b && vt.m_contents->value.m_priorityChanged.compare_exchange(true, false))
			{
				typename map_t::volatile_iterator itor;
				rcptr<volatile boolean> oldRemoved = vt.m_contents->value.m_removed;
				for (;;)
				{
					// Another thread might remove this node, but we will detect that later as removal failure.
					if (!oldRemoved)
						oldRemoved = vt.get_value_obj();
					else if (oldRemoved.get_mark() != 0) // Already scheduled/removed, nothing to do.
						break;

					// Add new element at new priority
					m_contents.insert_via([&](typename map_t::iterator& i)
					{
						new (const_cast<key_t*>(&i->key)) key_t(newPriority);
						new (&i->value) payload(std::move(oldRemoved));
					});

					// Link old to new element
					vt.m_contents->value.m_rescheduledTo = itor;

					// Try to remove the old element
					bool removedOld = m_contents.remove(vt.m_contents);
					if (removedOld) // We were able to remove the old one, done
					{
						b = true;
						vt.m_contents = itor;
						break;
					}
					// Failed to remove it.  Must have been scheduled/removed.
					vt.release(); // It's gone, clear vt.

					// Try to remove the new one, it's superfluous.
					bool removedNew = m_contents.remove(itor);
					if (removedNew)
						break;

					typename map_t::volatile_iterator itor2 = itor->value.m_rescheduledTo;
					if (!itor2)
					{
						// Both fired.  Did we succeed in changing the priority?  Let's say if the priority was raised, it doesn't matter, Yes.
						// But if the priority was lowered, and it was the higher priority that expired, we would be lying about having
						// successfully deprioritized it.  So, No.
						b = (newPriority < oldPriority); // less than - 'better' priority is 'first'/lesser value
						break;
					}

					// If, upon failing to remove it, it had been successfully rescheduled, we need to follow the trail and remove them all, until one fails.
					while (!!m_contents.remove(itor2))
						itor2 = itor2->value.m_rescheduledTo;
					b = true;
					break;
				}
			}
		}

		return b;
	}

	bool change_priority(remove_token& rt, const key_t& newPriority) volatile
	{
		value_token vt(rt.m_contents);
		if (change_priority(vt, newPriority))
		{
			rt = vt;
			return true;
		}
		return false;
	}

	typedef typename map_t::volatile_remove_result remove_result;

	remove_result remove(const value_token& vt) volatile { return m_contents.remove(vt.m_contents); }
	remove_result remove(const remove_token& rt) volatile { return m_contents.remove(rt.m_contents); }
};


}


#endif
