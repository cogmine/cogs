//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_CLEANUP_QUEUE
#define COGS_HEADER_SYNC_CLEANUP_QUEUE


#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/bypass_constructor_permission.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/sync/priority_dispatcher.hpp"


namespace cogs {


class thread_pool;


/// @ingroup Mem
/// @brief Allows delegates or object references to be registered for later invocation or release, when cleaning up.
class cleanup_queue : public dispatcher, public object
{
private:
	priority_dispatcher m_tasks;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		return dispatcher::dispatch_inner(m_tasks, t, priority);
	}

	friend class thread_pool;

public:
	~cleanup_queue()
	{
		drain();
	}

	// Invokes all tasks and releases all objects, serially.  drain() is not intended to be called in parallel.
	// Returns false if there was nothing to do.
	bool drain() volatile
	{
		bool anyInvoked = false;
		while (!!m_tasks.invoke())
			anyInvoked = true;
		return anyInvoked;
	}

	template <typename type>
	rcref<task<void> > add(const rcref<type>& obj, int priority = 0) volatile
	{
		return m_tasks.dispatch([r{ obj }]() {}, priority);
	}

	template <typename type>
	rcref<task<void> > add(const rcptr<type>& obj, int priority = 0) volatile
	{
		if (obj.get_desc() == nullptr)
			return signaled();
		return m_tasks.dispatch([r{ obj }]() {}, priority);
	}

	template <typename type>
	rcref<task<void> > add(const weak_rcptr<type>& obj, int priority = 0) volatile
	{
		if (obj.get_desc() == nullptr)
			return signaled();
		return m_tasks.dispatch([r{ obj }]() {}, priority);
	}

	static rcref<cleanup_queue> get()
	{
		return singleton<cleanup_queue, singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::must_call_shutdown>::get();
	}
};


template <typename T>
template <singleton_posthumous_behavior posthumous_behavior, singleton_cleanup_behavior cleanup_behavior>
inline rcptr<T> singleton_base<T>::get(bool& isNew)
{
	isNew = false;
	rcptr<T> result;
	volatile weak_rcptr<T>* g = &s_global.get();
	weak_rcptr<T> oldValue = *g;

	bool b;
	if constexpr (posthumous_behavior == singleton_posthumous_behavior::create_new_singleton)
	{
		result = oldValue;
		b = !result;
	}
	else
	{
		b = false;
		if (oldValue.get_desc() != nullptr)
			result = oldValue; // won't be marked, if desc was not nullptr
		else if (oldValue.get_mark() == 0) // not set up yet
			b = true;
	}

	if (b)
	{
		rcptr<T> newValue = rcnew(bypass_constructor_permission<T>);
		newValue.get_desc()->acquire(reference_strength::strong);
		if (!g->compare_exchange(newValue, oldValue, oldValue))
		{
			newValue.get_desc()->release(reference_strength::strong);
			if constexpr (posthumous_behavior == singleton_posthumous_behavior::create_new_singleton)
				result = oldValue;
			else if (oldValue.get_mark() == 0)
				result = oldValue;
		}
		else
		{
			isNew = true;
			result = std::move(newValue); // Return the one we just created.
			if constexpr (cleanup_behavior == singleton_cleanup_behavior::use_cleanup_queue)
				cleanup_queue::get()->dispatch(&singleton_base<T>::template shutdown<posthumous_behavior>);
		}
	}

	if constexpr (posthumous_behavior == singleton_posthumous_behavior::create_new_per_caller)
	{
		if (!result)
			result = rcnew(bypass_constructor_permission<T>);
	}
	return result;
}


}


#include "cogs/sync/quit_dispatcher.hpp"


#endif
