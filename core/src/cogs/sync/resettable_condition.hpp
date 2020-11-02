//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_RESETTABLE_EVENT
#define COGS_HEADER_SYNC_RESETTABLE_EVENT


#include <utility>

#include "cogs/function.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/sync/semaphore.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


class resettable_condition : public condition_base
{
private:
	mutable rcptr<waitable> m_condition;

	rcref<waitable> lazy_init_condition() const volatile
	{
		rcptr<waitable> e = m_condition;
		if (!e)
		{
			rcptr<waitable> newCondition = rcnew(single_fire_condition);
			if (m_condition.compare_exchange(newCondition, e, e))
				e = newCondition;
		}
		return std::move(e.dereference());
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		rcref<waitable> e = lazy_init_condition();
		return dispatcher::dispatch_inner(*e, t, priority);
	}

public:
	resettable_condition() { }

	explicit resettable_condition(bool initialState)
	{
		if (initialState)
			m_condition = signaled();
	}

	virtual bool signal() volatile
	{
		rcptr<waitable> e = m_condition;
		for (;;)
		{
			if (!!e)
			{
				if (e->is_signaled())
					return false;
				return e.static_cast_to<single_fire_condition>()->signal();
			}
			if (m_condition.compare_exchange(signaled(), e, e))
				return true;
		}
	}

	// returns true if the event was in signaled state
	bool reset() volatile
	{
		rcptr<waitable> e = m_condition;
		for (;;)
		{
			if (!e || !e->is_signaled())
				return false;
			rcptr<waitable> empty;
			if (m_condition.compare_exchange(empty, e, e))
				return true;
		}
	}

	void pulse_all() volatile
	{
		rcptr<waitable> e;
		m_condition.swap(e);
		if (!!e && !e->is_signaled())
			e.static_cast_to<single_fire_condition>()->signal();
	}

	bool pulse_one() volatile
	{
		rcptr<waitable> e = m_condition;
		for (;;)
		{
			if (!e)
				return false;
			if (!e->is_signaled())
				return e.static_cast_to<task<void>>()->signal_one_continuation();
			rcptr<waitable> empty;
			if (m_condition.compare_exchange(empty, e, e))
				return false;
		}
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		rcref<waitable> e = lazy_init_condition();
		return e->timed_wait(timeout, spinCount);
	}
};


}


#endif
