//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_WAIT_PRIORITY_QUEUE
#define COGS_HEADER_SYNC_WAIT_PRIORITY_QUEUE


#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/semaphore.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief A priority queue that can perform a blocking wait.
template <typename key_t, typename type = void, class comparator_t = default_comparator, class allocator_type = default_allocator>
class wait_priority_queue
{
private:
	typedef wait_priority_queue<key_t, type, comparator_t, allocator_type> this_t;
	typedef priority_queue<key_t, type, comparator_t, allocator_type> priority_queue_t;

	mutable semaphore m_semaphore;
	priority_queue_t m_priorityQueue;

	wait_priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class value_token;
	class remove_token;

	/// @brief A wait_priority_queue value token
	class value_token
	{
	protected:
		typename priority_queue_t::value_token m_contents;

		value_token(const typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		value_token(const typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		value_token(const volatile typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		value_token(typename priority_queue_t::value_token&& vt) : m_contents(std::move(vt)) { }

		value_token& operator=(const typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		value_token& operator=(const typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		value_token& operator=(const volatile typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename priority_queue_t::value_token&& vt) { m_contents = std::move(vt); return *this; }

		friend class wait_priority_queue;

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

		const key_t& get_key() const { return m_contents.get_key(); }
		type& get_value() const { return m_contents.get_value(); }
		type& operator*() const { return m_contents.get_value(); }
		type* operator->() const { return &(m_contents.get_value()); }

		const rcptr<const key_t>& get_key_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			return m_contents.get_key_obj(storage);
		}

		const rcptr<type>& get_value_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
		{
			return m_contents.get_value_obj(storage);
		}

		const rcptr<type>& get_obj(unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
		{
			return m_contents.get_value_obj(storage);
		}
	};

	/// @brief A wait_priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class wait_priority_queue;

		typename priority_queue_t::remove_token m_contents;

		remove_token(const typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		remove_token(const typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		remove_token(const volatile typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		remove_token(typename priority_queue_t::remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token& operator=(const typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		remove_token& operator=(const typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		remove_token& operator=(const volatile typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename priority_queue_t::remove_token&& rt) { m_contents = std::move(rt); return *this; }

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

	wait_priority_queue() { }

	explicit wait_priority_queue(volatile allocator_type& al) : m_priorityQueue(al) { }

	void clear() { m_priorityQueue.clear(); }
	bool drain() { return m_priorityQueue.drain(); }
	bool drain() volatile { return m_priorityQueue.drain(); }
	bool is_empty() const volatile { return m_priorityQueue.is_empty(); }
	bool operator!() const volatile { return is_empty(); }
	size_t size() const volatile { return m_priorityQueue.size(); }

	struct insert_result
	{
		value_token valueToken;
		bool wasEmpty;
	};

	template <typename F>
	insert_result insert_via(F&& f) volatile
	{
		value_token vt;
		auto p = m_priorityQueue.insert_via([&](typename priority_queue_t::value_token& vt2)
		{
			vt = std::move(vt2);
			f(vt);
		});
		m_semaphore.release();
		return { std::move(vt), p.wasEmpty };
	}

	insert_result insert(const key_t& k, const type& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(k);
			placement_rcnew(&vt.m_contents->get_value(), *vt.get_desc())(v);
		});
	}

	insert_result insert(key_t&& k, const type& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(std::move(k));
			placement_rcnew(&vt.m_contents->get_value(), *vt.get_desc())(v);
		});
	}

	insert_result insert(const key_t& k, type&& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(k);
			placement_rcnew(&vt.m_contents->get_value(), *vt.get_desc())(std::move(v));
		});
	}

	insert_result insert(key_t&& k, type&& v) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(std::move(k));
			placement_rcnew(&vt.m_contents->get_value(), *vt.get_desc())(std::move(v));
		});
	}

	void insert_multiple_via(size_t n, const key_t& k, const type& t) volatile
	{
		for (size_t i = 0; i < n; i++)
			m_priorityQueue.insert(k, t);
		m_semaphore.release(n);
	}

	value_token get(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_contents = m_priorityQueue.get();
			if (!!result)
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile { return m_priorityQueue.try_get(lowestPriority); }

	value_token peek() const volatile { return m_priorityQueue.peek(); }

	value_token wait_peek(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) const volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_contents = m_priorityQueue.peek();
			if (!!result)
				break;
		}
		return result;
	}

	bool change_priority(value_token& vt, const key_t& newPriority) volatile { return m_priorityQueue.change_priority(vt.m_contents, newPriority); }
	bool change_priority(remove_token& rt, const key_t& newPriority) volatile { return m_priorityQueue.change_priority(rt.m_contents, newPriority); }

	typedef typename priority_queue_t::remove_result remove_result;

	remove_result remove(const value_token& vt) volatile { return m_priorityQueue.remove(vt.m_contents); }
	remove_result remove(const remove_token& rt) volatile { return m_priorityQueue.remove(rt.m_contents); }
};

template <typename key_t, class comparator_t, class allocator_type>
class wait_priority_queue<key_t, void, comparator_t, allocator_type>
{
private:
	typedef wait_priority_queue<key_t, void, comparator_t, allocator_type> this_t;
	typedef priority_queue<key_t, void, comparator_t, allocator_type> priority_queue_t;

	mutable semaphore m_semaphore;
	priority_queue_t m_priorityQueue;

	wait_priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class value_token;
	class remove_token;

	/// @brief A wait_priority_queue value token
	class value_token
	{
	protected:
		typename priority_queue_t::value_token m_contents;

		value_token(const typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		value_token(const typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		value_token(const volatile typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		value_token(const volatile typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		value_token(typename priority_queue_t::value_token&& vt) : m_contents(std::move(vt)) { }

		value_token& operator=(const typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		value_token& operator=(const typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(const volatile typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		value_token& operator=(const volatile typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		value_token& operator=(typename priority_queue_t::value_token&& vt) { m_contents = std::move(vt); return *this; }

		friend class wait_priority_queue;

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

		const key_t& operator*() const { return m_contents.get_key(); }
		const key_t* operator->() const { return &(m_contents.get_key()); }

		const rcptr<const key_t>& get_key_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			return m_contents.get_key_obj(storage);
		}

		const rcptr<const key_t>& get_obj(unowned_t<rcptr<const key_t> >& storage = unowned_t<rcptr<const key_t> >().get_unowned()) const
		{
			return m_contents.get_key_obj(storage);
		}
	};

	/// @brief A wait_priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class wait_priority_queue;

		typename priority_queue_t::remove_token m_contents;

		remove_token(const typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		remove_token(const typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		remove_token(const volatile typename priority_queue_t::value_token& vt) : m_contents(vt) { }
		remove_token(const volatile typename priority_queue_t::remove_token& rt) : m_contents(rt) { }
		remove_token(typename priority_queue_t::remove_token&& rt) : m_contents(std::move(rt)) { }

		remove_token& operator=(const typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		remove_token& operator=(const typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(const volatile typename priority_queue_t::value_token& vt) { m_contents = vt; return *this; }
		remove_token& operator=(const volatile typename priority_queue_t::remove_token& rt) { m_contents = rt; return *this; }
		remove_token& operator=(typename priority_queue_t::remove_token&& rt) { m_contents = std::move(rt); return *this; }

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

	wait_priority_queue() { }

	explicit wait_priority_queue(volatile allocator_type& al) : m_priorityQueue(al) { }

	void clear() { m_priorityQueue.clear(); }
	bool drain() { return m_priorityQueue.drain(); }
	bool drain() volatile { return m_priorityQueue.drain(); }
	bool is_empty() const volatile { return m_priorityQueue.is_empty(); }
	bool operator!() const volatile { return is_empty(); }
	size_t size() const volatile { return m_priorityQueue.size(); }

	struct insert_result
	{
		value_token valueToken;
		bool wasEmpty;
	};

	template <typename F>
	insert_result insert_via(F&& f) volatile
	{
		value_token vt;
		auto p = m_priorityQueue.insert_via([&](typename priority_queue_t::value_token& vt2)
		{
			vt = std::move(vt2);
			f(vt);
		});
		m_semaphore.release();
		return { std::move(vt), p.wasEmpty };
	}

	insert_result insert(const key_t& k) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(k);
		});
	}

	insert_result insert(key_t&& k) volatile
	{
		return insert_via([&](value_token& vt)
		{
			placement_rcnew(const_cast<key_t*>(&vt.m_contents->get_key()), *vt.get_desc())(std::move(k));
		});
	}

	void insert_multiple_via(size_t n, const key_t& k) volatile
	{
		for (size_t i = 0; i < n; i++)
			m_priorityQueue.insert(k);
		m_semaphore.release(n);
	}

	value_token get(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_contents = m_priorityQueue.get();
			if (!!result)
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile { return m_priorityQueue.try_get(lowestPriority); }

	value_token peek() const volatile { return m_priorityQueue.peek(); }

	value_token wait_peek(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) const volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_contents = m_priorityQueue.peek();
			if (!!result)
				break;
		}
		return result;
	}

	bool change_priority(value_token& vt, const key_t& newPriority) volatile { return m_priorityQueue.change_priority(vt.m_contents, newPriority); }
	bool change_priority(remove_token& rt, const key_t& newPriority) volatile { return m_priorityQueue.change_priority(rt.m_contents, newPriority); }

	typedef typename priority_queue_t::remove_result remove_result;

	remove_result remove(const value_token& vt) volatile { return m_priorityQueue.remove(vt.m_contents); }
	remove_result remove(const remove_token& rt) volatile { return m_priorityQueue.remove(rt.m_contents); }
};


}


#endif
