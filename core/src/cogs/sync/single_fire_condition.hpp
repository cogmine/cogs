//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SINGLE_FIRE_EVENT
#define COGS_HEADER_SYNC_SINGLE_FIRE_EVENT


#include "cogs/sync/immediate_task.hpp"


namespace cogs {


// A single_fire_condition is similar to a signallable_task, but doesn't provide cancel()


class single_fire_condition : public condition_base, public object
{
private:
	signallable_task<void> m_signallableTask;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		return dispatcher::dispatch_inner(m_signallableTask, t, priority);
	}

public:
	virtual bool signal() volatile
	{
		return m_signallableTask.signal();
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		return m_signallableTask.timed_wait(timeout, spinCount);
	}
};


}


#include "cogs/sync/thread.hpp"


#endif
