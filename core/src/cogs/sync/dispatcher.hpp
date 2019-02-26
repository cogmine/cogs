//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_DISPATCHER
#define COGS_HEADER_SYNC_DISPATCHER


#include "cogs/function.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/const_min_int.hpp"
#include "cogs/mem/delayed_construction.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/sync/semaphore.hpp"
#include "cogs/sync/priority_queue.hpp"


namespace cogs {


// A dispatcher is something that can dispatch functions.
class dispatcher;

// waitable is a base class for objects that can be waited on for a signal.
class waitable;

// event is a base class for waitable objects with a public method to signal.
class event;

class task_base;

class dispatched;

template <typename result_t>
class task;

template <typename result_t, typename arg_t>
class function_task;

template <typename result_t>
class signallable_task;

template <typename arg_t>
class task_arg_base;


rcref<task<void> > get_immediate_task();

template <typename T>
rcref<task<std::remove_reference_t<T> > > get_immediate_task(T&& t);


class dispatcher
{
public:
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F>
		&& std::is_void_v<std::invoke_result_t<F> >,
		rcref<task<void> > >
	dispatch(F&& f, int priority = 0) volatile
	{
		rcref<task_base> t = dispatch_default_task(std::forward<F>(f), priority);
		return t->get_task();
	}

	template <typename F, typename... args_t>
	std::enable_if_t<
		std::is_invocable_v<F>,
		rcref<task<std::invoke_result_t<F> > > >
	dispatch(F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers) volatile;

protected:
	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile = 0;

	static rcptr<volatile dispatched> get_dispatched(const task<void>& t);

	static void dispatch_inner(volatile dispatcher& dr, const rcref<task_base>& t, int priority)
	{
		return dr.dispatch_inner(t, priority);
	}

	// The following are overloaded ONLY by immediate_task
	virtual rcref<task_base> dispatch_default_task(const void_function& f, int priority) volatile;

	// The following are only useful if creating a facade for an immediate_task
	static rcref<task_base> dispatch_default_task(volatile dispatcher& dr, const void_function& f, int priority)
	{
		return dr.dispatch_default_task(f, priority);
	}

	// Called by derived task to pass priority change to parent dispatcher
	static void change_priority_inner(volatile dispatcher& dr, volatile dispatched& d, int newPriority)
	{
		return dr.change_priority_inner(d, newPriority);
	}

	static rcref<task<bool> > cancel_inner(volatile dispatcher& dr, volatile dispatched& d)
	{
		return dr.cancel_inner(d);
	}

	// change_priority_inner() and cancel_inner() are called by tasks, on the dispatcher that created them.
	// They do not need to be implemented by a derived dispatcher if it is a proxy of another dispatcher.
	virtual void change_priority_inner(volatile dispatched& d, int newPriority) volatile { }
	virtual rcref<task<bool> > cancel_inner(volatile dispatched& d) volatile { return get_immediate_task(false); }
};


class task_base
{
private:
	template <typename result_t>
	friend class task;

	weak_rcptr<volatile dispatched> m_dispatched;
	
	virtual bool needs_arg() const volatile { return false; }

public:
	virtual bool signal() volatile = 0;
	virtual rcref<task<bool> > cancel() volatile = 0;
	virtual rcref<task<void> > get_task() = 0;

	void set_dispatched(const rcref<volatile dispatched>& d) { m_dispatched = d; }

	// m_dispatched is written once, so cast away volatility for const access
	rcptr<volatile dispatched> get_dispatched() const volatile { return *(const weak_rcptr<volatile dispatched>*)&m_dispatched; }
};


template <typename arg_t>
class task_arg_base : public task_base
{
private:
	virtual bool needs_arg() const volatile { return true; }

public:
	virtual bool signal() volatile { return false; }
	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile = 0;
};


template <>
class task_arg_base<void> : public task_base
{
};


class dispatched : public object
{
private:
	weak_rcptr<volatile dispatcher> m_parentDispatcher;
	rcref<task_base> m_taskBase;

public:
	dispatched(const ptr<rc_obj_base>& desc, const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t)
		: object(desc),
		m_parentDispatcher(parentDispatcher),
		m_taskBase(t)
	{
		COGS_ASSERT(m_taskBase.get_desc() != nullptr);
	}

	// m_parentDispatcher is written only on construction, so cast away volatility for const access
	const weak_rcptr<volatile dispatcher>& get_dispatcher() const volatile { return *(const weak_rcptr<volatile dispatcher>*)&m_parentDispatcher; }

	// m_task is written only on construction, so cast away volatility for const access
	const rcref<task_base>& get_task_base() const volatile { return *(const rcref<task_base>*)&m_taskBase; }
};


class waitable : public dispatcher
{
public:
	// Returns 0 if timed-out, 1 if completed, -1 if cancelled
	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile = 0;

	// Returns true if signalled, false if cancelled.
	bool wait() const volatile { return timed_wait(timeout_t::infinite()) == 1; }

	bool is_signalled() const volatile { return timed_wait(timeout_t::none()) == 1; }
	bool is_pending() const volatile { return timed_wait(timeout_t::none()) == 0; }
	bool is_cancelled() const volatile { return timed_wait(timeout_t::none()) == -1; }

	bool operator!() const volatile { return !is_signalled(); }

	// A waitable is considered const with regards to waiting on its signal
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F>
		&& std::is_void_v<std::invoke_result_t<F> >,
		rcref<task<void> > >
	dispatch(F&& f, int priority = 0) volatile
	{
		return ((volatile dispatcher*)(this))->dispatch(std::forward<F>(f), priority);
	}

	template <typename F, typename... args_t>
	std::enable_if_t<
		std::is_invocable_v<F>,
		rcref<task<std::invoke_result_t<F> > > >
	dispatch(F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers) volatile
	{
		return ((volatile dispatcher*)(this))->dispatch(std::forward<F>(f), priority, nextDispatcher, nextDispatchers...);
	}
};


class event_base : public waitable
{
public:
	// Returns true if transitioned from unsignalled to signalled state.
	// Returns false if already signalled, or cancelled, or if still in the process of signalling
	// (in which case, a previous call to signal() will have returned true).
	virtual bool signal() volatile = 0;
};


class continuation_dispatched : public dispatched
{
public:
	priority_queue<int, ptr<continuation_dispatched> >::remove_token m_removeToken;

	// m_removeToken is accessed in a thread-safe way.  Cast away volatility
	priority_queue<int, ptr<continuation_dispatched> >::remove_token& get_remove_token() volatile { return ((continuation_dispatched*)this)->m_removeToken; }
	const priority_queue<int, ptr<continuation_dispatched> >::remove_token& get_remove_token() const volatile { return ((const continuation_dispatched*)this)->m_removeToken; }

	continuation_dispatched(const ptr<rc_obj_base>& desc, const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t, const priority_queue<int, ptr<continuation_dispatched> >::remove_token& rt)
		: dispatched(desc, parentDispatcher, t),
		m_removeToken(rt)
	{ }
};


template <>
class task<void> : public waitable, public object
{
protected:
	friend class dispatcher;

	priority_queue<int, ptr<continuation_dispatched> > m_continuationSubTasks;
	volatile boolean m_continuationSubTaskDrainDone;

	virtual rcptr<volatile dispatched> get_dispatched() const volatile { return rcptr<volatile dispatched>(); }

	virtual rcref<task<bool> > cancel_inner(volatile dispatched& d) volatile
	{
		volatile continuation_dispatched& d2 = *(volatile continuation_dispatched*)&d;
		priority_queue<int, ptr<continuation_dispatched> >::remove_token& rt = d2.get_remove_token();
		bool b = m_continuationSubTasks.remove(rt);
		return get_immediate_task(b);
	}

	virtual void change_priority_inner(volatile dispatched& d, int newPriority) volatile
	{
		volatile continuation_dispatched& d2 = *(volatile continuation_dispatched*)&d;
		priority_queue<int, ptr<continuation_dispatched> >::remove_token& rt = d2.get_remove_token();
		m_continuationSubTasks.change_priority(rt, newPriority);
	}

	void cancel_continuations() volatile
	{
		bool setDoneState = false;
		for (;;)
		{
			priority_queue<int, ptr<continuation_dispatched> >::value_token vt = m_continuationSubTasks.get();
			if (!vt)
			{
				if (setDoneState)
					break;
				setDoneState = true;
				m_continuationSubTaskDrainDone = true;
				continue;
			}
			(*vt)->get_task_base()->cancel();
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		priority_queue<int, ptr<continuation_dispatched> >::preallocated_t i;
		auto r = m_continuationSubTasks.preallocate_key_with_aux<delayed_construction<continuation_dispatched> >(priority, i);
		continuation_dispatched* d = &(r->get());
		i.get_value() = d;
		placement_rcnew(d, r.get_desc(), this_rcref, t, i);
		rcref<continuation_dispatched> d2(d, i.get_desc());
		t->set_dispatched(d2);
		i.disown();
		for (;;)
		{
			if (!m_continuationSubTaskDrainDone)
			{
				m_continuationSubTasks.insert_preallocated(i);
				if (!m_continuationSubTaskDrainDone || !m_continuationSubTasks.remove(i))
					break;	// was handled
			}
			i.release();
			if (is_signalled())
				signal_continuation(*d->get_task_base());
			else
				d->get_task_base()->cancel();
			break;
		}
	}

	virtual void signal_continuation(task_base& t) volatile
	{
		t.signal();
	}

	void signal_continuations() volatile
	{
		bool setDoneState = false;
		for (;;)
		{
			priority_queue<int, ptr<continuation_dispatched> >::value_token vt = m_continuationSubTasks.get();
			if (!vt)
			{
				if (setDoneState)
					break;
				setDoneState = true;
				m_continuationSubTaskDrainDone = true;
				continue;
			}

			ptr<continuation_dispatched> d = (*vt).template static_cast_to<continuation_dispatched>();
			signal_continuation(*d->get_task_base());
		}
	}

	task(const ptr<rc_obj_base>& desc, bool signalled = false)
		: object(desc),
		m_continuationSubTaskDrainDone(signalled)
	{ }

public:
	~task()
	{
		for (;;)
		{
			priority_queue<int, ptr<continuation_dispatched> >::value_token vt = m_continuationSubTasks.get();
			if (!vt)
				break;
			(*vt)->get_task_base()->cancel();
		}
	}

	virtual rcref<task<bool> > cancel() volatile = 0;

	virtual void change_priority(int newPriority) volatile { }	// default is no-op
};


template <typename result_t>
class task : public task<void>
{
protected:
	using task<void>::signal_continuations;

	static_assert(!std::is_reference_v<result_t>);	// task result must be a concrete type

	virtual void signal_continuation(task_base& t) volatile
	{
		if (!t.needs_arg())
			t.signal();
		else
		{
			task_arg_base<result_t>* t2 = (task_arg_base<result_t>*)&t;
			t2->signal(this_rcref.template const_cast_to<task<result_t> >());
		}
	}

	task(const ptr<rc_obj_base>& desc, bool signalled)
		: task<void>(desc, signalled)
	{ }

public:
	explicit task(const ptr<rc_obj_base>& desc)
		: task<void>(desc)
	{ }

	using task<void>::dispatch;

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, const result_t&>,
		rcref<task<std::invoke_result_t<F, const result_t&> > > >
	dispatch(F&& f, int priority = 0) const volatile;

	template <typename F, typename... args_t>
	std::enable_if_t<
		std::is_invocable_v<F, const result_t&>,
		rcref<task<std::invoke_result_t<F, const result_t&> > > >
	dispatch(F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers) const volatile;

	// It's the responsibility of the caller to add volatility to the reference returned by get()
	// if needed, if the result may be referenced concurrently (i.e. any time there are multiple
	// continuations on the same task, or multiple callers might call get() on the same task).
	// Note that since it's const, it may not require volatility.

	virtual const result_t& get() const volatile  = 0;	// error to call on incomplete task.
};


// Note: derive from task<void> if the operation can be cancelled, otherwise derive from waitable



inline rcptr<volatile dispatched> dispatcher::get_dispatched(const task<void>& t) { return t.get_dispatched(); }


template <typename result_t>
class signallable_task : public task<result_t>
{
protected:
	using task<result_t>::signal_continuations;

	struct TaskState
	{
		alignas (atomic::get_alignment_v<size_t>) size_t m_numWaiting;
		alignas (atomic::get_alignment_v<os::semaphore*>) os::semaphore* m_osSemaphore;

		// 0 = initial state, 1 = nextTask deployed, 2 = cancelled, 3 = invoked
		// Piggy back 2 bits of state data in the unused 2 bits of m_os_semaphore
		int get_state() const
		{
			ptr<os::semaphore> p = m_osSemaphore;
			return (int)p.get_mark();
		}

		os::semaphore* get_semaphore() const
		{
			ptr<os::semaphore> p = m_osSemaphore;
			return p.get_unmarked();
		}

		void set_state(int s)
		{
			COGS_ASSERT(s <= 3);
			ptr<os::semaphore> p = m_osSemaphore;
			p.set_mark(s);
			m_osSemaphore = p.get_ptr();
		}

		void set_semaphore(os::semaphore* s)
		{
			ptr<os::semaphore> p = s;
			m_osSemaphore = p.get_marked(get_state());
		}
	};

	static_assert(can_atomic_v<TaskState>);

	mutable TaskState m_taskState alignas (atomic::get_alignment_v<TaskState>);

	explicit signallable_task(const ptr<rc_obj_base>& desc)
		: task<result_t>(desc),
		m_taskState{ 0, nullptr }
	{ }

	~signallable_task()
	{
		COGS_ASSERT(!m_taskState.m_numWaiting);	// Should not be possible to destruct when some threads still in timed_wait().
		ptr<os::semaphore> p = m_taskState.m_osSemaphore;
		COGS_ASSERT(!p.get_unmarked());	// Should be no semaphore
	}

	int get_state() const volatile
	{
		ptr<os::semaphore> p = atomic::load(m_taskState.m_osSemaphore);
		return (int)p.get_mark();
	}

	void release_waiting(const TaskState& oldTaskState) volatile
	{
		ptr<os::semaphore> osSemaphore = oldTaskState.get_semaphore();
		if (!!osSemaphore)
		{
			COGS_ASSERT(oldTaskState.m_numWaiting > 0);
			osSemaphore->release(oldTaskState.m_numWaiting);
		}
	}

	bool try_set_state(TaskState& oldTaskState, int newState) volatile
	{
		atomic::load(m_taskState, oldTaskState);
		for (;;)
		{
			if (oldTaskState.get_state() >= 2)
				return false;
			ptr<os::semaphore> osSemaphore = oldTaskState.get_semaphore();
			TaskState newTaskState{ oldTaskState.m_numWaiting, osSemaphore.get_marked(newState) };
			if (atomic::compare_exchange(m_taskState, newTaskState, oldTaskState, oldTaskState))
				return true;
			//continue;
		}
	}

	bool try_cancel(TaskState& oldTaskState) volatile
	{
		if (!try_set_state(oldTaskState, 2))
			return false;
		release_waiting(oldTaskState);
		task<result_t>::cancel_continuations();
		return true;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		TaskState oldTaskState;
		bool b = try_cancel(oldTaskState);
		return get_immediate_task(b);
	}

public:
	// Returns -1 if cancelled, 0 if timed-out, 1 if completed
	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		int result = 0;
		unsigned int spinsLeft = spinCount;
		TaskState oldTaskState;
		rcptr<os::semaphore> osSemaphoreRc;
		for (;;)
		{
			atomic::load(m_taskState, oldTaskState);
			int state = oldTaskState.get_state();
			if (state == 3)	// Completed
			{
				result = 1;
				break;
			}
			if (state == 2)	// Cancelled
			{
				result = -1;
				break;
			}				// otherwise, 0 or 1 means in progress
			if (!timeout)
				break;
			if (!!spinsLeft)
			{
				if (!os::thread::spin_once())
				{
					spinsLeft = 0;
					break;
				}
				--spinsLeft;
				continue;
			}
			bool useCachedOsSemaphore = false;
			TaskState newTaskState;
			newTaskState.m_numWaiting = oldTaskState.m_numWaiting + 1;
			ptr<os::semaphore> osSemaphore = oldTaskState.get_semaphore();
			if (!!osSemaphore)
			{
				ptr<os::semaphore> p = osSemaphore.get_marked(state);
				newTaskState.m_osSemaphore = p.get_ptr();
			}
			else
			{
				useCachedOsSemaphore = true;
				if (!osSemaphoreRc)
					osSemaphoreRc = semaphore::get_os_semaphore();
				osSemaphore = osSemaphoreRc.get_ptr();
				newTaskState.m_osSemaphore = osSemaphore.get_marked(state);
			}
			if (!atomic::compare_exchange(m_taskState, newTaskState, oldTaskState, oldTaskState))
				continue;
			if (useCachedOsSemaphore)	// Store our copy of it in osSemaphoreRc
				osSemaphoreRc.get_desc()->acquire();	// Give one reference to state
			else
			{
				osSemaphoreRc.release();	// Borrow rcptr here to clean up our reference to state semaphore
				osSemaphoreRc.set(osSemaphore, osSemaphore->self_acquire());
			}
			bool acquiredSemaphore = osSemaphore->acquire(timeout);
			atomic::load(m_taskState, oldTaskState);
			for (;;)
			{
				COGS_ASSERT(oldTaskState.get_semaphore() == osSemaphore.get_ptr());	// Should not have changed
				COGS_ASSERT(oldTaskState.m_numWaiting > 0);
				state = oldTaskState.get_state();
				newTaskState.m_numWaiting = oldTaskState.m_numWaiting - 1;
				newTaskState.m_osSemaphore = (newTaskState.m_numWaiting == 0) ? (os::semaphore*)(size_t)state : oldTaskState.m_osSemaphore;
				if (atomic::compare_exchange(m_taskState, newTaskState, oldTaskState, oldTaskState))
					break;
				//continue;
			}
			if (state >= 2)
			{
				result = (state == 2) ? -1 : 1;
				if (!acquiredSemaphore)	// If a wake was incurred for us, before we decremented
				{
					bool b = osSemaphore->acquire();
					COGS_ASSERT(b);
				}
			}
			if (newTaskState.m_numWaiting == 0)
				osSemaphore->self_release();
			break;
		}
		return result;
	}

protected:
	void post_signal(const TaskState& oldTaskState) volatile
	{
		release_waiting(oldTaskState);
		signal_continuations();
	}

	bool signal() volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		post_signal(oldTaskState);
		return true;
	}
};


template <typename result_t>
class signallable_task_with_payload : public signallable_task<result_t>
{
protected:
	using signallable_task<result_t>::m_taskState;

	placement<result_t> m_result;

public:
	explicit signallable_task_with_payload(const ptr<rc_obj_base>& desc)
		: signallable_task<result_t>(desc)
	{ }

	~signallable_task_with_payload()
	{
		ptr<os::semaphore> p = m_taskState.m_osSemaphore;
		int state = (int)p.get_mark();
		if (state == 3)
			m_result.destruct();
	}

	virtual const result_t& get() const volatile { return m_result.get(); }
};


template <>
class signallable_task_with_payload<void> : public signallable_task<void>
{
protected:
	explicit signallable_task_with_payload(const ptr<rc_obj_base>& desc)
		: signallable_task<void>(desc)
	{ }

	void get() const volatile {};
};


template <typename result_t, typename arg_t>
class function_task_base : public signallable_task_with_payload<result_t>, public task_arg_base<arg_t>
{
private:
	template <typename arg_t2, bool unused = true>
	class helper
	{
	public:
		typedef function<result_t(arg_t2)> func_t;
	};

	template <bool unused>
	class helper<void, unused>
	{
	public:
		typedef function<result_t()> func_t;
	};

public:
	using signallable_task_with_payload<result_t>::get;
	using typename signallable_task_with_payload<result_t>::TaskState;

	typedef typename helper<arg_t>::func_t func_t;
	func_t m_primaryFunc;

	// If in a task chain, and a task other than the primary one is cancelled,
	// we need to propagate that cancellation.
	volatile weak_rcptr<task<void> > m_previousTask;

	alignas (atomic::get_alignment_v<int>) int m_priority;

	int get_priority() const volatile
	{
		return atomic::load(m_priority);
	}

	virtual rcref<task<void> > get_task() { return this_rcref; }

	virtual rcptr<volatile dispatched> get_dispatched() const volatile { return task_arg_base<arg_t>::get_dispatched(); }

	template <typename F>
	function_task_base(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: signallable_task_with_payload<result_t>(desc),
		m_primaryFunc(std::forward<F>(f)),
		m_priority(priority)
	{
	}
	
	bool try_cancel(TaskState& oldTaskState) volatile
	{
		if (!signallable_task_with_payload<result_t>::try_cancel(oldTaskState))
			return false;
		weak_rcptr<task<void> > weakPreviousTask;
		weakPreviousTask.set_mark(1);	// used to prevents task from being set if already cancelled
		m_previousTask.swap(weakPreviousTask);
		COGS_ASSERT(weakPreviousTask.get_mark() == 0);	// Only set by us, if try_cancel was successful.  Should not already be set.
		weak_rcptr<task<void> > previousTask = weakPreviousTask;
		if (!!previousTask)
		{
			previousTask->cancel();
		}
		(*(func_t*)&m_primaryFunc).release();
		int state = oldTaskState.get_state();
		if (state == 0)
		{
			rcptr<volatile dispatched> d = get_dispatched();
			if (!!d)
			{
				rcptr<volatile dispatcher> dr = d->get_dispatcher();
				if (!!dr)
					dispatcher::cancel_inner(*dr, *d);
			}
		}
		return true;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		typename signallable_task<result_t>::TaskState oldTaskState;
		bool b = try_cancel(oldTaskState);
		return get_immediate_task(b);
	}

	virtual void change_priority(int newPriority) volatile
	{
		rcptr<volatile dispatched> d = get_dispatched();
		if (!!d)
		{
			rcptr<volatile dispatcher> dr = d->get_dispatcher();
			if (!!dr)
			{
				atomic::store(m_priority, newPriority);
				for (;;)
				{
					dispatcher::change_priority_inner(*dr, *d, newPriority);
					int newerPriority = atomic::load(m_priority);
					if (newPriority == newerPriority)	// Address race condition in which another thread is also changing the priority
						break;
					newPriority = newerPriority;
				}
			}
		}
	}
};



template <typename result_t, typename arg_t, typename... args_t>
class forwarding_function_task
{
};


template <typename result_t, typename arg_t>
class forwarding_function_task<result_t, arg_t> : public function_task_base<result_t, arg_t>
{
protected:
	using typename function_task_base<result_t, arg_t>::TaskState;
	using typename function_task_base<result_t, arg_t>::func_t;
	using function_task_base<result_t, arg_t>::m_primaryFunc;
	using function_task_base<result_t, arg_t>::get;
	using function_task_base<result_t, arg_t>::try_set_state;
	using function_task_base<result_t, arg_t>::post_signal;

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<result_t, arg_t>(desc, std::forward<F>(f), priority)
	{
	}

	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		new ((result_t*)&get()) result_t((*(func_t*)&m_primaryFunc)(parentTask->get()));
		post_signal(oldTaskState);
		(*(func_t*)&m_primaryFunc).release();
		return true;
	}
};


template <typename result_t>
class forwarding_function_task<result_t, void> : public function_task_base<result_t, void>
{
protected:
	using typename function_task_base<result_t, void>::TaskState;
	using typename function_task_base<result_t, void>::func_t;
	using function_task_base<result_t, void>::m_primaryFunc;
	using function_task_base<result_t, void>::get;
	using function_task_base<result_t, void>::try_set_state;
	using function_task_base<result_t, void>::post_signal;

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<result_t, void>(desc, std::forward<F>(f), priority)
	{
	}

	virtual bool signal() volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		new ((result_t*)&get()) result_t((*(func_t*)&m_primaryFunc)());
		post_signal(oldTaskState);
		(*(func_t*)&m_primaryFunc).release();
		return true;
	}
};


template <typename arg_t>
class forwarding_function_task<void, arg_t> : public function_task_base<void, arg_t>
{
protected:
	using typename function_task_base<void, arg_t>::TaskState;
	using typename function_task_base<void, arg_t>::func_t;
	using function_task_base<void, arg_t>::m_primaryFunc;
	using function_task_base<void, arg_t>::get;
	using function_task_base<void, arg_t>::try_set_state;
	using function_task_base<void, arg_t>::post_signal;

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<void, arg_t>(desc, std::forward<F>(f), priority)
	{
	}

	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		(*(func_t*)&m_primaryFunc)(parentTask->get());
		post_signal(oldTaskState);
		(*(func_t*)&m_primaryFunc).release();
		return true;
	}
};


template <>
class forwarding_function_task<void, void> : public function_task_base<void, void>
{
protected:
	using typename function_task_base<void, void>::TaskState;
	using typename function_task_base<void, void>::func_t;
	using function_task_base<void, void>::m_primaryFunc;
	using function_task_base<void, void>::get;
	using function_task_base<void, void>::try_set_state;
	using function_task_base<void, void>::post_signal;

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<void, void>(desc, std::forward<F>(f), priority)
	{
	}

	virtual bool signal() volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		(*(func_t*)&m_primaryFunc)();
		post_signal(oldTaskState);
		(*(func_t*)&m_primaryFunc).release();
		return true;
	}
};



template <typename result_t, typename arg_t>
class forwarding_function_task_base : public function_task_base<result_t, arg_t>
{
protected:
	using typename function_task_base<result_t, arg_t>::func_t;
	using function_task_base<result_t, arg_t>::m_primaryFunc;
	using function_task_base<result_t, arg_t>::get;

	template <typename F>
	explicit forwarding_function_task_base(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<result_t, arg_t>(desc, f, priority)
	{ }

	void invoke(const rcref<task<arg_t> >& parentTask) volatile
	{
		new ((result_t*)&get()) result_t((*(func_t*)&m_primaryFunc)(parentTask->get()));
	};
};

template <typename result_t>
class forwarding_function_task_base<result_t, void> : public function_task_base<result_t, void>
{
protected:
	using typename function_task_base<result_t, void>::func_t;
	using function_task_base<result_t, void>::m_primaryFunc;
	using function_task_base<result_t, void>::get;

	template <typename F>
	explicit forwarding_function_task_base(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<result_t, void>(desc, f, priority)
	{ }

	void invoke(const rcref<task<void> >& parentTask) volatile
	{
		new ((result_t*)&get()) result_t((*(func_t*)&m_primaryFunc)());
	};
};

template <typename arg_t>
class forwarding_function_task_base<void, arg_t> : public function_task_base<void, arg_t>
{
protected:
	using typename function_task_base<void, arg_t>::func_t;
	using function_task_base<void, arg_t>::m_primaryFunc;
	using function_task_base<void, arg_t>::get;
	
	template <typename F>
	explicit forwarding_function_task_base(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<void, arg_t>(desc, f, priority)
	{ }

	void invoke(const rcref<task<arg_t> >& parentTask) volatile
	{
		(*(func_t*)&m_primaryFunc)(parentTask->get());
	};
};

template <>
class forwarding_function_task_base<void, void> : public function_task_base<void, void>
{
protected:
	using typename function_task_base<void, void>::func_t;
	using function_task_base<void, void>::m_primaryFunc;
	using function_task_base<void, void>::get;

	template <typename F>
	explicit forwarding_function_task_base(const ptr<rc_obj_base>& desc, F&& f, int priority)
		: function_task_base<void, void>(desc, f, priority)
	{ }

	void invoke(const rcref<task<void> >& parentTask) volatile
	{
		(*(func_t*)&m_primaryFunc)();
	};
};


template <typename result_t, typename arg_t, typename T>
class forwarding_function_task<result_t, arg_t, T> : public forwarding_function_task_base<result_t, arg_t>
{
private:
	using typename forwarding_function_task_base<result_t, arg_t>::TaskState;
	using typename forwarding_function_task_base<result_t, arg_t>::func_t;
	using forwarding_function_task_base<result_t, arg_t>::get_priority;
	using forwarding_function_task_base<result_t, arg_t>::m_primaryFunc;
	using forwarding_function_task_base<result_t, arg_t>::get_state;
	using forwarding_function_task_base<result_t, arg_t>::cancel;
	using forwarding_function_task_base<result_t, arg_t>::try_set_state;

	rcref<volatile dispatcher> m_nextDispatcher;
	rcptr<task<void> > m_nextTask;

	void forward_signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		int priority = get_priority();
		rcref<task<void> > nextTask = m_nextDispatcher->dispatch([r{ this_rcref }, parentTask]()
		{
			TaskState oldTaskState2;
			if (r->try_set_state(oldTaskState2, 3))
			{
				r->invoke(parentTask);
				//new ((result_t*)&r->get()) result_t((*(func_t*)&r->m_primaryFunc)(parentTask->get()));
				r->post_signal(oldTaskState2);
				(*(func_t*)&r->m_primaryFunc).release();
			}
		}, priority);
		m_nextTask = nextTask;
		if (get_state() == 2)
			nextTask->cancel();
		else if (!nextTask.template static_cast_to<forwarding_function_task<void, void> >()->m_previousTask.compare_exchange(this_rcref, nullptr))
			cancel();	// Failure here indicates it was cancelled.
		else
		{
			int newPriority = get_priority();
			while (newPriority != priority)
			{
				nextTask->change_priority(newPriority);
				priority = newPriority;
				newPriority = get_priority();
			}
		}
	}

	void forward_cancel() volatile
	{
		rcptr<task<result_t> > nextTask = m_nextTask;
		if (!!nextTask)
			nextTask->cancel();
	}

	void forward_change_priority(int newPriority) volatile
	{
		rcptr<task<result_t> > nextTask = m_nextTask;
		if (!!nextTask)
			nextTask->change_priority(newPriority);
	}

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher)
		: forwarding_function_task_base<result_t, arg_t>(desc, std::forward<F>(f), priority),
		m_nextDispatcher(nextDispatcher)
	{
	}

	bool try_cancel(TaskState& oldTaskState) volatile
	{
		bool b = forwarding_function_task_base<result_t, arg_t>::try_cancel(oldTaskState);
		forward_cancel();
		return b;
	}

	virtual void change_priority(int newPriority) volatile
	{
		forwarding_function_task_base<result_t, arg_t>::change_priority(newPriority);
		forward_change_priority(newPriority);
	}

	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 1))
			return false;
		forward_signal(parentTask);
		return true;
	}
};


template <typename result_t, typename arg_t, typename T, typename... args_t>
class forwarding_function_task<result_t, arg_t, T, args_t...> : public forwarding_function_task<result_t, arg_t, args_t...>
{
private:
	using typename forwarding_function_task<result_t, arg_t, args_t...>::TaskState;
	using forwarding_function_task<result_t, arg_t, args_t...>::get_priority;
	using forwarding_function_task<result_t, arg_t, args_t...>::get_state;
	using forwarding_function_task<result_t, arg_t, args_t...>::cancel;

	rcref<volatile dispatcher> m_nextDispatcher;
	rcptr<task<void> > m_nextTask;

	void forward_signal(const rcref<task<arg_t> >& parentTask)
	{
		int priority = get_priority();
		rcref<task<void> > nextTask = m_nextDispatcher->dispatch([r{ this_rcref }, parentTask]()
		{
			r->forwarding_function_task<result_t, arg_t, args_t...>::forward_signal(parentTask);
		}, priority);
		m_nextTask = nextTask;
		if (get_state() == 2)
			nextTask->cancel();
		else if (!nextTask.template static_cast_to<forwarding_function_task<void, void> >()->m_previousTask.compare_exchange(this_rcref, nullptr))
			cancel();	// Failure here indicates it was cancelled.
		else
		{
			int newPriority = get_priority();
			while (newPriority != priority)
			{
				nextTask->change_priority(newPriority);
				priority = newPriority;
				newPriority = get_priority();
			}
		}
	}

	void forward_cancel() volatile
	{
		rcptr<task<result_t> > nextTask = m_nextTask;
		if (!!nextTask)
		{
			nextTask->cancel();
			forwarding_function_task<result_t, arg_t, args_t...>::forward_cancel();
		}
	}

	void forward_change_priority(int newPriority) volatile
	{
		rcptr<task<result_t> > nextTask = m_nextTask;
		if (!!nextTask)
		{
			nextTask->change_priority(newPriority);
			forwarding_function_task<result_t, arg_t, args_t...>::forward_change_priority(newPriority);
		}
	}

	bool try_cancel(TaskState& oldTaskState) volatile
	{
		bool b = forwarding_function_task<result_t, arg_t, args_t...>::try_cancel(oldTaskState);
		forward_cancel();
		return b;
	}

public:
	template <typename F>
	forwarding_function_task(const ptr<rc_obj_base>& desc, F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers)
		: forwarding_function_task<result_t, arg_t, args_t...>(desc, std::forward<F>(f), priority, nextDispatchers...),
		m_nextDispatcher(nextDispatcher)
	{
	}

	virtual void change_priority(int newPriority) volatile
	{
		forwarding_function_task<result_t, arg_t, args_t...>::change_priority(newPriority);
		forward_change_priority(newPriority);
	}

	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		TaskState oldTaskState;
		if (!try_set_state(oldTaskState, 1))
			return false;
		forward_signal(parentTask);
		return true;
	}
};


template <typename F, typename... args_t>
inline std::enable_if_t<
	std::is_invocable_v<F>,
	rcref<task<std::invoke_result_t<F> > > >
dispatcher::dispatch(F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers) volatile
{
	typedef std::invoke_result_t<F> result_t2;
	typedef forwarding_function_task<result_t2, void, args_t...> task_t;
	rcref<task<result_t2> > t = rcnew(task_t, std::forward<F>(f), priority, nextDispatcher, nextDispatchers...).template static_cast_to<task<result_t2> >();
	dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


inline rcref<task_base> dispatcher::dispatch_default_task(const void_function& f, int priority) volatile
{
	typedef forwarding_function_task<void, void> task_t;
	rcref<task_base> t = rcnew(task_t, f, priority).template static_cast_to<task_base>();
	dispatch_inner(t, priority);
	return t;
}


template <typename result_t>
template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F, const result_t&>,
	rcref<task<std::invoke_result_t<F, const result_t&> > > >
task<result_t>::dispatch(F&& f, int priority) const volatile
{
	typedef std::invoke_result_t<F, const result_t&> result_t2;
	typedef forwarding_function_task<result_t2, result_t> task_t;
	rcref<task<result_t2> > t = rcnew(task_t, std::forward<F>(f), priority).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


template <typename result_t>
template <typename F, typename... args_t>
inline std::enable_if_t<
	std::is_invocable_v<F, const result_t&>,
	rcref<task<std::invoke_result_t<F, const result_t&> > > >
task<result_t>::dispatch(F&& f, int priority, const rcref<volatile dispatcher>& nextDispatcher, const rcref<volatile args_t>&... nextDispatchers) const volatile
{
	typedef std::invoke_result_t<F, const result_t&> result_t2;
	typedef forwarding_function_task<result_t2, result_t, args_t...> task_t;
	rcref<task<result_t2> > t = rcnew(task_t, std::forward<F>(f), priority, nextDispatcher, nextDispatchers...).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


}


#include "cogs/sync/single_fire_event.hpp"
#include "cogs/sync/immediate_task.hpp"


#endif
