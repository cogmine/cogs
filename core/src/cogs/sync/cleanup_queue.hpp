//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CLEANUP_QUEUE
#define COGS_CLEANUP_QUEUE


#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/singleton.hpp"
#include "cogs/sync/priority_dispatcher.hpp"


namespace cogs {


class thread_pool;


/// @ingroup Mem
/// @brief Allows delegates or object references to be registered for later invocation or release, when cleaning up.
class cleanup_queue : public dispatcher, public object
{
private:
	delayed_construction<priority_dispatcher>	m_tasks;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		return dispatcher::dispatch_inner(m_tasks.get(), t, priority);
	}

	friend class thread_pool;

protected:
	cleanup_queue()
	{
		placement_rcnew(this_desc, &m_tasks.get());
	}

public:
	static rcref<cleanup_queue> create() { return rcnew(bypass_constructor_permission<cleanup_queue>); }

	~cleanup_queue()
	{
		drain();
	}

	// Invokes all tasks and releases all objects, serially.  drain() is not intended to be called in parallel.
	// Returns false if there was nothing to do.
	bool drain() volatile
	{
		bool anyInvoked = false;
		while (!!m_tasks->invoke())
			anyInvoked = true;
		return anyInvoked;
	}

	template <typename type>
	rcref<task<void> > add(const rcref<type>& obj, int priority = 0) volatile
	{
		return m_tasks->dispatch([r{ obj }]() {}, priority);
	}

	template <typename type>
	rcref<task<void> > add(const rcptr<type>& obj, int priority = 0) volatile
	{
		if (obj.get_desc() == nullptr)
			return get_immediate_task();
		return m_tasks->dispatch([r{ obj }]() {}, priority);
	}

	template <typename type>
	rcref<task<void> > add(const weak_rcptr<type>& obj, int priority = 0) volatile
	{
		if (obj.get_desc() == nullptr)
			return get_immediate_task();
		return m_tasks->dispatch([r{ obj }]() {}, priority);
	}

	static rcref<cleanup_queue> get_global()
	{
		return singleton<cleanup_queue, singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::must_call_shutdown>::get();
	}
};


}


#include "cogs/sync/thread_pool.hpp"

#endif
