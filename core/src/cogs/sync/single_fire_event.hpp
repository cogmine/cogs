//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SINGLE_FIRE_EVENT
#define COGS_HEADER_SYNC_SINGLE_FIRE_EVENT


#include "cogs/sync/dispatcher.hpp"


namespace cogs {


class single_fire_event : public event_base
{
private:
	class single_fire_signallable_task : public signallable_task<void>, public task_arg_base<void>
	{
	public:
		virtual rcref<task<void> > get_task() { return this_rcref; }

		virtual rcptr<volatile dispatched> get_dispatched() const volatile { return task_arg_base<void>::get_dispatched(); }

		virtual bool signal() volatile { return signallable_task<void>::signal(); }

		virtual rcref<task<bool> > cancel() volatile { return get_immediate_task(false); }
	};

	rcref<single_fire_signallable_task> m_singleFireSignallableTask;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		return dispatcher::dispatch_inner(*m_singleFireSignallableTask, t, priority);
	}

public:
	single_fire_event()
		: m_singleFireSignallableTask(rcnew(single_fire_signallable_task))
	{ }

	virtual bool signal() volatile
	{
		return m_singleFireSignallableTask->signal();
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		return m_singleFireSignallableTask->timed_wait(timeout, spinCount);
	}
};


}


#include "cogs/sync/thread.hpp"


#endif
