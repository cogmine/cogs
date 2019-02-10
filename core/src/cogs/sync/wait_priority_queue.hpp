//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_WAIT_PRIORITY_QUEUE
#define COGS_WAIT_PRIORITY_QUEUE


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

	mutable semaphore	m_semaphore;
	priority_queue_t	m_priorityQueue;

	wait_priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class preallocated_t;
	class value_token;
	class remove_token;

	/// @brief A preallocated wait_priority_queue element
	class preallocated_t
	{
	protected:
		friend class wait_priority_queue;

		typename priority_queue_t::preallocated_t m_preallocated;

		preallocated_t(const typename priority_queue_t::preallocated_t& i) : m_preallocated(i) { }
		preallocated_t& operator=(const typename priority_queue_t::preallocated_t& i) { m_preallocated = i; return *this; }

	public:
		void disown() { m_preallocated.disown(); }

		preallocated_t() { }
		preallocated_t(const preallocated_t& src) : m_preallocated(src.m_preallocated) { }

		void release() { m_preallocated.release(); }

		bool operator!() const { return !m_preallocated; }
		bool operator==(const preallocated_t& i) const { return m_preallocated == i.m_preallocated; }
		bool operator!=(const preallocated_t& i) const { return !operator==(i); }
		preallocated_t& operator=(const preallocated_t& i) { m_preallocated = i.m_preallocated; return *this; }

		key_t& get_key() const { return m_preallocated.get_key(); }
		type& get_value() const { return m_preallocated.get_value(); }

		type& operator*() const { return m_preallocated.get_value(); }
		type* operator->() const { return &(m_preallocated.get_value()); }

		rcptr<key_t> get_key_obj() const { return m_preallocated.get_key_obj(); }

		rcptr<type> get_value_obj() const { return m_preallocated.get_value_obj(); }

		rcptr<type> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_preallocated.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_preallocated.get_desc(); }
	};

	/// @brief A wait_priority_queue value token
	class value_token
	{
	protected:
		typename priority_queue_t::value_token m_valueToken;

		value_token(const typename priority_queue_t::value_token& i) : m_valueToken(i) { }
		value_token(const typename priority_queue_t::remove_token& rt) : m_valueToken(rt) { }
		value_token(const volatile typename priority_queue_t::value_token& i) : m_valueToken(i) { }
		value_token(const volatile typename priority_queue_t::remove_token& rt) : m_valueToken(rt) { }

		friend class wait_priority_queue;
	public:
		void disown() { m_valueToken.disown(); }

		value_token() { }
		value_token(const value_token& vt) : m_valueToken(vt.m_valueToken) { }
		value_token(const remove_token& rt) : m_valueToken(rt.m_removeToken) { }
		value_token(const volatile value_token& vt) : m_valueToken(vt.m_valueToken) { }
		value_token(const volatile remove_token& rt) : m_valueToken(rt.m_removeToken) { }

		value_token& operator=(const value_token& vt) { m_valueToken = vt.m_valueToken; return *this; }
		value_token& operator=(const volatile value_token& vt) { m_valueToken = vt.m_valueToken; return *this; }
		void operator=(const value_token& vt) volatile { m_valueToken = vt.m_valueToken; }

		value_token& operator=(const remove_token& rt) { m_valueToken = rt.m_removeToken; return *this; }
		value_token& operator=(const volatile remove_token& rt) { m_valueToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile { m_valueToken = rt.m_removeToken; }

		bool is_active() const { return m_valueToken.is_active(); }
		bool is_active() const volatile { return m_valueToken.is_active(); }

		bool is_removed() const { return m_valueToken.is_removed(); }
		bool is_removed() const volatile { return m_valueToken.is_removed(); }

		void release() { m_valueToken.release(); }
		void release() volatile { m_valueToken.release(); }

		bool is_removed() { return m_valueToken.is_removed(); }

		bool operator!() const { return !m_valueToken; }
		bool operator!() const volatile { return !m_valueToken; }

		bool operator==(const value_token& vt) const { return m_valueToken == vt.m_valueToken; }
		bool operator==(const volatile value_token& vt) const { return m_valueToken == vt.m_valueToken; }
		bool operator==(const value_token& vt) const volatile { return m_valueToken == vt.m_valueToken; }

		bool operator==(const remove_token& rt) const { return m_valueToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_valueToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_valueToken == rt.m_removeToken; }

		bool operator!=(const value_token& vt) const { return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const { return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile { return !operator==(vt); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		const key_t& get_key() const { return m_valueToken.get_key(); }
		type& get_value() const { return m_valueToken.get_value(); }
		type& operator*() const { return m_valueToken.get_value(); }
		type* operator->() const { return &(m_valueToken.get_value()); }

		rcptr<const key_t> get_key_obj() const { return m_valueToken.get_key_obj(); }

		rcptr<type> get_value_obj() const { return m_valueToken.get_value_obj(); }

		rcptr<type> get_obj() const { return get_value_obj(); }

		rc_obj_base* get_desc() const { return m_valueToken.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_valueToken.get_desc(); }
	};

	/// @brief A wait_priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class wait_priority_queue;

		typename priority_queue_t::remove_token m_removeToken;

		remove_token(const typename priority_queue_t::value_token& i) : m_removeToken(i) { }
		remove_token(const typename priority_queue_t::remove_token& rt) : m_removeToken(rt) { }
	public:
		remove_token() { }
		remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const value_token& vt) : m_removeToken(vt.m_valueToken) { }
		remove_token(const volatile remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const volatile value_token& vt) : m_removeToken(vt.m_valueToken) { }

		remove_token& operator=(const preallocated_t& i) { m_removeToken = i.m_preallocated; return *this; }
		remove_token& operator=(const preallocated_t& i) volatile { m_removeToken = i.m_preallocated; return *this; }

		remove_token& operator=(const remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile { m_removeToken = rt.m_removeToken; }

		remove_token& operator=(const value_token& vt) { m_removeToken = vt.m_valueToken; return *this; }
		remove_token& operator=(const volatile value_token& vt) { m_removeToken = vt.m_valueToken; return *this; }
		void operator=(const value_token& vt) volatile { m_removeToken = vt.m_valueToken; }

		bool is_active() const { return m_removeToken.is_active(); }
		bool is_active() const volatile { return m_removeToken.is_active(); }

		void release() { m_removeToken.release(); }
		void release() volatile { m_removeToken.release(); }

		bool operator!() const { return !m_removeToken; }
		bool operator!() const volatile { return !m_removeToken; }

		bool operator==(const remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const value_token& vt) const { return m_removeToken == vt.m_valueToken; }
		bool operator==(const value_token& vt) const volatile { return m_removeToken == vt.m_valueToken; }
		bool operator==(const volatile value_token& vt) const { return m_removeToken == vt.m_valueToken; }

		bool operator!=(const remove_token& rt) const { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const remove_token& rt) const volatile { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const volatile remove_token& rt) const { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const value_token& vt) const { return m_removeToken != vt.m_valueToken; }
		bool operator!=(const value_token& vt) const volatile { return m_removeToken != vt.m_valueToken; }
		bool operator!=(const volatile value_token& vt) const { return m_removeToken != vt.m_valueToken; }
	};

	wait_priority_queue() { }

	wait_priority_queue(volatile allocator_type& al) : m_priorityQueue(al) { }

	void clear()							{ m_priorityQueue.clear(); }
	void drain() volatile					{ m_priorityQueue.drain(); }
	bool is_empty() const volatile			{ return m_priorityQueue.is_empty(); }
	bool operator!() const volatile			{ return is_empty(); }
	size_t size() const volatile			{ return m_priorityQueue.size(); }

	value_token insert(const key_t& k, const type& t) volatile
	{
		value_token result(m_priorityQueue.insert(k, t));
		m_semaphore.release();
		return result;
	}

	void insert_multiple(size_t n, const key_t& k, const type& t) volatile
	{
		if (n > 0)
		{
			m_priorityQueue.insert_multiple(n, k, t);
			m_semaphore.release(n);
		}
	}

	preallocated_t preallocate() const volatile { return m_priorityQueue.preallocate(); }
	preallocated_t preallocate(const key_t& k, const type& t) const volatile { return m_priorityQueue.preallocate(k, t); }
	preallocated_t preallocate_key(const key_t& k) const volatile { return m_priorityQueue.preallocate_key(k); }

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_priorityQueue.template preallocate_with_aux<T>(i.m_preallocated, storage);
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(const key_t& k, const type& t, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_priorityQueue.template preallocate_with_aux<T>(k, t, i.m_preallocated, storage);
	}

	template <typename T>
	const rcref<T>& preallocate_key_with_aux(const key_t& k, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_priorityQueue.template preallocate_key_with_aux<T>(k, i.m_preallocated, storage);
	}

	value_token insert_preallocated(const preallocated_t& i) volatile			{ return m_priorityQueue.insert_preallocated(i.m_preallocated); }

	value_token get(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_valueToken = m_priorityQueue.get();
			if (!!result)
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile		{ return m_priorityQueue.try_get(lowestPriority); }

	value_token peek() const volatile	{ return m_priorityQueue.peek(); }

	value_token wait_peek(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) const volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_valueToken = m_priorityQueue.peek();
			if (!!result)
				break;
		}
		return result;
	}

	bool change_priority(value_token& vt, const key_t& newPriority) volatile	{ return m_priorityQueue.change_priority(vt.m_valueToken, newPriority); }
	bool change_priority(remove_token& rt, const key_t& newPriority) volatile	{ return m_priorityQueue.change_priority(rt.m_removeToken, newPriority); }

	bool remove(const value_token& vt) volatile									{ return m_priorityQueue.remove(vt.m_valueToken); }
	bool remove(const remove_token& rt) volatile								{ return m_priorityQueue.remove(rt.m_removeToken); }
};

template <typename key_t, class comparator_t, class allocator_type>
class wait_priority_queue<key_t, void, comparator_t, allocator_type>
{
private:
	typedef wait_priority_queue<key_t, void, comparator_t, allocator_type> this_t;
	typedef priority_queue<key_t, void, comparator_t, allocator_type> priority_queue_t;

	mutable semaphore	m_semaphore;
	priority_queue_t	m_priorityQueue;

	wait_priority_queue(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	class preallocated_t;
	class value_token;
	class remove_token;

	/// @brief A preallocated wait_priority_queue element
	class preallocated_t
	{
	protected:
		typename priority_queue_t::preallocated_t m_preallocated;

		preallocated_t(const typename priority_queue_t::preallocated_t& i) : m_preallocated(i) { }

		friend class wait_priority_queue;

	public:
		void disown() { m_preallocated.disown(); }

		preallocated_t() { }
		preallocated_t(const preallocated_t& src) : m_preallocated(src.m_preallocated) { }

		void release() { m_preallocated.release(); }

		bool operator!() const { return !m_preallocated; }
		bool operator==(const preallocated_t& i) const { return m_preallocated == i.m_preallocated; }
		bool operator!=(const preallocated_t& i) const { return !operator==(i); }
		preallocated_t& operator=(const preallocated_t& i) { m_preallocated = i.m_preallocated; return *this; }

		key_t& operator*() const { return *m_preallocated; }
		key_t* operator->() const { return &*m_preallocated; }

		rcptr<key_t> get_obj() const { return m_preallocated.get_key_obj(); }

		rc_obj_base* get_desc() const { return m_preallocated.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_preallocated.get_desc(); }
	};

	/// @brief A wait_priority_queue value token
	class value_token
	{
	protected:
		typename priority_queue_t::value_token m_valueToken;

		value_token(const typename priority_queue_t::value_token& i) : m_valueToken(i) { }
		value_token(const typename priority_queue_t::remove_token& rt) : m_valueToken(rt) { }
		value_token(const volatile typename priority_queue_t::value_token& i) : m_valueToken(i) { }
		value_token(const volatile typename priority_queue_t::remove_token& rt) : m_valueToken(rt) { }

		friend class wait_priority_queue;
	public:
		void disown() { m_valueToken.disown(); }

		value_token() { }
		value_token(const value_token& vt) : m_valueToken(vt.m_valueToken) { }
		value_token(const remove_token& rt) : m_valueToken(rt.m_removeToken) { }
		value_token(const volatile value_token& vt) : m_valueToken(vt.m_valueToken) { }
		value_token(const volatile remove_token& rt) : m_valueToken(rt.m_removeToken) { }

		value_token& operator=(const value_token& vt) { m_valueToken = vt.m_valueToken; return *this; }
		value_token& operator=(const volatile value_token& vt) { m_valueToken = vt.m_valueToken; return *this; }
		void operator=(const value_token& vt) volatile { m_valueToken = vt.m_valueToken; }

		value_token& operator=(const remove_token& rt) { m_valueToken = rt.m_removeToken; return *this; }
		value_token& operator=(const volatile remove_token& rt) { m_valueToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile { m_valueToken = rt.m_removeToken; }

		bool is_active() const { return m_valueToken.is_active(); }
		bool is_active() const volatile { return m_valueToken.is_active(); }

		bool is_removed() const { return m_valueToken.is_removed(); }
		bool is_removed() const volatile { return m_valueToken.is_removed(); }

		void release() { m_valueToken.release(); }
		void release() volatile { m_valueToken.release(); }

		bool is_removed() { return m_valueToken.is_removed(); }

		bool operator!() const { return !m_valueToken; }
		bool operator!() const volatile { return !m_valueToken; }

		bool operator==(const value_token& vt) const { return m_valueToken == vt.m_valueToken; }
		bool operator==(const volatile value_token& vt) const { return m_valueToken == vt.m_valueToken; }
		bool operator==(const value_token& vt) const volatile { return m_valueToken == vt.m_valueToken; }

		bool operator==(const remove_token& rt) const { return m_valueToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_valueToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_valueToken == rt.m_removeToken; }

		bool operator!=(const value_token& vt) const { return !operator==(vt); }
		bool operator!=(const volatile value_token& vt) const { return !operator==(vt); }
		bool operator!=(const value_token& vt) const volatile { return !operator==(vt); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		const key_t& operator*() const { return m_valueToken.get_key(); }
		const key_t* operator->() const { return &(m_valueToken.get_key()); }

		rcptr<const key_t> get_obj() const { return m_valueToken.get_key_obj(); }

		rc_obj_base* get_desc() const { return m_valueToken.get_desc(); }
		rc_obj_base* get_desc() const volatile { return m_valueToken.get_desc(); }
	};

	/// @brief A wait_priority_queue element remove token
	///
	/// A remove token is like a value token, but keeps a weak reference to the content.
	class remove_token
	{
	protected:
		friend class wait_priority_queue;

		typename priority_queue_t::remove_token m_removeToken;

		remove_token(const typename priority_queue_t::value_token& i) : m_removeToken(i) { }
		remove_token(const typename priority_queue_t::remove_token& rt) : m_removeToken(rt) { }
	public:
		remove_token() { }
		remove_token(const preallocated_t& i) : m_removeToken(i.m_preallocated) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const value_token& vt) : m_removeToken(vt.m_valueToken) { }
		remove_token(const volatile remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const volatile value_token& vt) : m_removeToken(vt.m_valueToken) { }

		remove_token& operator=(const preallocated_t& i) { m_removeToken = i.m_preallocated; return *this; }
		remove_token& operator=(const preallocated_t& i) volatile { m_removeToken = i.m_preallocated; return *this; }

		remove_token& operator=(const remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		void operator=(const remove_token& rt) volatile { m_removeToken = rt.m_removeToken; }

		remove_token& operator=(const value_token& vt) { m_removeToken = vt.m_valueToken; return *this; }
		remove_token& operator=(const volatile value_token& vt) { m_removeToken = vt.m_valueToken; return *this; }
		void operator=(const value_token& vt) volatile { m_removeToken = vt.m_valueToken; }

		bool is_active() const { return m_removeToken.is_active(); }
		bool is_active() const volatile { return m_removeToken.is_active(); }

		void release() { m_removeToken.release(); }
		void release() volatile { m_removeToken.release(); }

		bool operator!() const { return !m_removeToken; }
		bool operator!() const volatile { return !m_removeToken; }

		bool operator==(const remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const value_token& vt) const { return m_removeToken == vt.m_valueToken; }
		bool operator==(const value_token& vt) const volatile { return m_removeToken == vt.m_valueToken; }
		bool operator==(const volatile value_token& vt) const { return m_removeToken == vt.m_valueToken; }

		bool operator!=(const remove_token& rt) const { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const remove_token& rt) const volatile { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const volatile remove_token& rt) const { return m_removeToken != rt.m_removeToken; }
		bool operator!=(const value_token& vt) const { return m_removeToken != vt.m_valueToken; }
		bool operator!=(const value_token& vt) const volatile { return m_removeToken != vt.m_valueToken; }
		bool operator!=(const volatile value_token& vt) const { return m_removeToken != vt.m_valueToken; }
	};

	wait_priority_queue() { }

	wait_priority_queue(volatile allocator_type& al) : m_priorityQueue(al) { }

	void clear() { m_priorityQueue.clear(); }
	void drain() { m_priorityQueue.clear(); }
	void drain() volatile					{ m_priorityQueue.drain(); }
	bool is_empty() const volatile			{ return m_priorityQueue.is_empty(); }
	bool operator!() const volatile			{ return is_empty(); }
	size_t size() const volatile			{ return m_priorityQueue.size(); }

	value_token insert(const key_t& k) volatile
	{
		value_token result(m_priorityQueue.insert(k));
		m_semaphore.release();
		return result;
	}

	void insert_multiple(size_t n, const key_t& k) volatile
	{
		if (n > 0)
		{
			m_priorityQueue.insert_multiple(n, k);
			m_semaphore.release(n);
		}
	}

	preallocated_t preallocate() const volatile { return m_priorityQueue.preallocate(); }
	preallocated_t preallocate(const key_t& k) const volatile { return m_priorityQueue.preallocate(k); }

	template <typename T>
	const rcref<T>& preallocate_with_aux(preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_priorityQueue.template preallocate_with_aux<T>(i.m_preallocated, storage);
	}

	template <typename T>
	const rcref<T>& preallocate_with_aux(const key_t& k, preallocated_t& i, unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned()) const volatile
	{
		return m_priorityQueue.template preallocate_with_aux<T>(k, i.m_preallocated, storage);
	}

	value_token insert_preallocated(const preallocated_t& i) volatile	{ return m_priorityQueue.insert_preallocated(i.m_preallocated); }

	value_token get(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_valueToken = m_priorityQueue.get();
			if (!!result)
				break;
		}
		return result;
	}

	value_token try_get(const key_t& lowestPriority) volatile		{ return m_priorityQueue.try_get(lowestPriority); }

	value_token peek() const volatile	{ return m_priorityQueue.peek(); }

	value_token wait_peek(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) const volatile
	{
		value_token result;
		for (;;)
		{
			if (!m_semaphore.acquire(1, timeout, spinCount))
				break;
			result.m_valueToken = m_priorityQueue.peek();
			if (!!result)
				break;
		}
		return result;
	}

	bool change_priority(value_token& vt, const key_t& newPriority) volatile	{ return m_priorityQueue.change_priority(vt.m_valueToken, newPriority); }
	bool change_priority(remove_token& rt, const key_t& newPriority) volatile	{ return m_priorityQueue.change_priority(rt.m_removeToken, newPriority); }

	bool remove(const value_token& vt) volatile							{ return m_priorityQueue.remove(vt.m_valueToken); }
	bool remove(const remove_token& rt) volatile						{ return m_priorityQueue.remove(rt.m_removeToken); }
};


}


#endif








