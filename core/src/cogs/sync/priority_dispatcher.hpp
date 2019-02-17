//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_PRIORITY_DISPATCHER
#define COGS_HEADER_SYNC_PRIORITY_DISPATCHER


#include "cogs/function.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/wait_priority_queue.hpp"
#include "cogs/sync/semaphore.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief A priority queue dispatcher
class priority_dispatcher : public dispatcher, public object
{
private:
	class priority_dispatched : public dispatched
	{
	public:
		priority_queue<int, ptr<priority_dispatched> >::remove_token m_removeToken;

		priority_dispatched(const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t, const priority_queue<int, ptr<priority_dispatched> >::remove_token& rt)
			: dispatched(parentDispatcher, t),
			m_removeToken(rt)
		{ }
	};

	priority_queue<int, ptr<priority_dispatched> > m_priorityQueue;

	priority_dispatcher(priority_dispatcher&&) = delete;
	priority_dispatcher(const priority_dispatcher&) = delete;
	priority_dispatcher& operator=(priority_dispatcher&&) = delete;
	priority_dispatcher& operator=(const priority_dispatcher&) = delete;

	virtual rcref<task<bool> > cancel_inner(volatile dispatched& t) volatile
	{
		priority_dispatched& d = *(priority_dispatched*)&t;
		bool b = m_priorityQueue.remove(d.m_removeToken);
		return get_immediate_task(b);
	}

	virtual void change_priority_inner(volatile dispatched& t, int newPriority) volatile
	{
		priority_dispatched& d = *(priority_dispatched*)&t;
		m_priorityQueue.change_priority(d.m_removeToken, newPriority);
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		priority_queue<int, ptr<priority_dispatched> >::preallocated_t i;
		auto r = m_priorityQueue.preallocate_key_with_aux<delayed_construction<priority_dispatched> >(priority, i);
		priority_dispatched* d = &(r->get());
		i.get_value() = d;
		placement_rcnew(r.get_desc(), d, this_rcref, t, i);
		m_priorityQueue.insert_preallocated(i);
		rcref<dispatched> d2(d, i.get_desc());
		t->set_dispatched(d2);
		i.disown();
	}

public:
	priority_dispatcher()			{ }

	void clear()					{ m_priorityQueue.clear(); }
	void drain()					{ m_priorityQueue.clear(); }
	void drain() volatile			{ m_priorityQueue.drain(); }
	bool is_empty() const volatile	{ return m_priorityQueue.is_empty(); }
	bool operator!() const volatile	{ return is_empty(); }
	size_t size() const volatile	{ return m_priorityQueue.size(); }

	rcptr<task<void> > peek() const volatile
	{
		priority_queue<int, ptr<priority_dispatched> >::value_token vt = m_priorityQueue.peek();
		if (!!vt)
		{
			priority_dispatched& d = *vt.get_value();
			return d.get_task_base()->get_task();
		}
		
		rcptr<task<void> > result;
		return result;
	}

	rcptr<task<void> > peek(int& priority) const volatile
	{
		priority_queue<int, ptr<priority_dispatched> >::value_token vt = m_priorityQueue.peek();
		if (!!vt)
		{
			priority = vt.get_key();
			priority_dispatched& d = *vt.get_value();
			return d.get_task_base()->get_task();
		}

		rcptr<task<void> > result;
		return result;
	}

	bool invoke() volatile
	{
		for (;;)
		{
			priority_queue<int, ptr<priority_dispatched> >::value_token vt = m_priorityQueue.get();
			if (!vt)
				return false;
			if (!!vt.get_value()->get_task_base()->signal())
				return true;
			//continue;
		}
	}

	bool try_invoke(int lowestPriority) volatile
	{
		for (;;)
		{
			priority_queue<int, ptr<priority_dispatched> >::value_token vt = m_priorityQueue.try_get(lowestPriority);
			if (!vt)
				return false;

			if (!!vt.get_value()->get_task_base()->signal())
				return true;
			//continue;
		}
	}

	bool remove_and_invoke(const rcref<task<void> >& t) volatile
	{
		rcptr<volatile priority_dispatched> d = get_dispatched(*t).static_cast_to<volatile priority_dispatched>();
		if (!d || !m_priorityQueue.remove(d->m_removeToken))
			return false;

		return d->get_task_base()->signal();
	}
};


}


#endif
