//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_DISPATCHER
#define COGS_HEADER_SYNC_DISPATCHER


#include "cogs/assert.hpp"
#include "cogs/function.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/const_min_int.hpp"
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


rcref<task<void> > signaled();

template <typename T>
rcref<task<std::remove_reference_t<T> > > signaled(T&& t);


template <typename T, typename enable = void> struct is_rcref_task : std::false_type {};
template <typename T> struct is_rcref_task<rcref<T>, typename std::enable_if_t<std::is_convertible_v<T*, task<void>*> > > : std::true_type {};
template <typename T> inline constexpr bool is_rcref_task_v = is_rcref_task<T>::value;


class dispatcher
{
public:
	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F>
		&& std::is_void_v<std::invoke_result_t<F> >,
		rcref<task<void> > >
	dispatch(F&& onComplete, int priority = 0) volatile;

	template <typename F1, typename F2>
	std::enable_if_t<
		std::is_invocable_v<F1>
		&& std::is_invocable_v<F2>
		&& std::is_void_v<std::invoke_result_t<F1> >,
		rcref<task<void> > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) volatile;

	template <typename F, typename... args_t>
	inline std::enable_if_t<
		std::is_invocable_v<F>
		&& is_rcref_task_v<std::invoke_result_t<F> >,
		rcref<task<typename std::invoke_result_t<F>::type::result_type> > >
	dispatch(F&& onComplete, int priority = 0) volatile;

	template <typename F1, typename F2, typename... args_t>
	inline std::enable_if_t<
		std::is_invocable_v<F1>
		&& std::is_invocable_v<F2>
		&& is_rcref_task_v<std::invoke_result_t<F1> >,
		rcref<task<typename std::invoke_result_t<F1>::type::result_type> > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) volatile;

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F>
		&& !std::is_void_v<std::invoke_result_t<F> >
		&& !is_rcref_task_v<std::invoke_result_t<F> >,
		rcref<task<std::invoke_result_t<F> > > >
	dispatch(F&& onComplete, int priority = 0) volatile;

	template <typename F1, typename F2>
	std::enable_if_t<
		std::is_invocable_v<F1>
		&& std::is_invocable_v<F2>
		&& !std::is_void_v<std::invoke_result_t<F1> >
		&& !is_rcref_task_v<std::invoke_result_t<F1> >,
		rcref<task<std::invoke_result_t<F1> > > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) volatile;

protected:
	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile = 0;

	static rcptr<volatile dispatched> get_dispatched(const task<void>& t);

	static void dispatch_inner(volatile dispatcher& dr, const rcref<task_base>& t, int priority)
	{
		return dr.dispatch_inner(t, priority);
	}

	static void dispatch_inner(const volatile waitable& w, const rcref<task_base>& t, int priority);

	// The following are overloaded ONLY by immediate_task
	virtual rcref<task_base> dispatch_default_task(const void_function& onComplete, int priority) volatile;
	virtual rcref<task_base> dispatch_default_task(const void_function& onComplete, const void_function& onCancel, int priority) volatile;

	// The following are only useful if creating a facade for an immediate_task
	static rcref<task_base> dispatch_default_task(volatile dispatcher& dr, const void_function& onComplete, int priority)
	{
		return dr.dispatch_default_task(onComplete, priority);
	}

	static rcref<task_base> dispatch_default_task(volatile dispatcher& dr, const void_function& onComplete, const void_function& onCancel, int priority)
	{
		return dr.dispatch_default_task(onComplete, onCancel, priority);
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
	virtual void change_priority_inner(volatile dispatched&, int) volatile { }
	virtual rcref<task<bool> > cancel_inner(volatile dispatched&) volatile { return signaled(false); }

	template <typename F>
	class linked_task;
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
	virtual rcptr<task<void> > get_chained_task()
	{
		rcptr<task<void> > tmp;
		return tmp;
	}

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
	virtual bool signal() volatile = 0;
	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile = 0;
};


template <>
class task_arg_base<void> : public task_base
{
public:
	virtual bool signal() volatile = 0;
	virtual bool signal(const rcref<task<void> >& parentTask) volatile = 0;
};


class dispatched : public virtual object
{
private:
	weak_rcptr<volatile dispatcher> m_parentDispatcher;
	rcref<task_base> m_taskBase;

public:
	dispatched(const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t)
		: m_parentDispatcher(parentDispatcher),
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

	// Returns true if signaled, false if cancelled.
	bool wait() const volatile { return timed_wait(timeout_t::infinite()) == 1; }

	bool is_signaled() const volatile { return timed_wait(timeout_t::none()) == 1; }
	bool is_pending() const volatile { return timed_wait(timeout_t::none()) == 0; }
	bool is_cancelled() const volatile { return timed_wait(timeout_t::none()) == -1; }

	bool operator!() const volatile { return !is_signaled(); }

	// A waitable is considered const with regards to waiting on its signal
	template <typename... args_t>
	auto dispatch(args_t&&... a) const volatile
	{
		return const_cast<volatile dispatcher*>(static_cast<const volatile dispatcher*>(this))->dispatch(std::forward<args_t>(a)...);
	}
};


inline void dispatcher::dispatch_inner(const volatile waitable& w, const rcref<task_base>& t, int priority)
{
	return dispatch_inner(*static_cast<volatile dispatcher*>(const_cast<volatile waitable*>(&w)), t, priority);
}


class condition_base : public waitable
{
public:
	// Returns true if transitioned from unsignaled to signaled state.
	// Returns false if already signaled, or cancelled, or if still in the process of signaling
	// (in which case, a previous call to signal() will have returned true).
	virtual bool signal() volatile = 0;
};


class continuation_dispatched : public dispatched
{
public:
	priority_queue<int, continuation_dispatched>::remove_token m_removeToken;

	// m_removeToken is accessed in a thread-safe way.  Cast away volatility
	priority_queue<int, continuation_dispatched>::remove_token& get_remove_token() volatile { return ((continuation_dispatched*)this)->m_removeToken; }
	const priority_queue<int, continuation_dispatched>::remove_token& get_remove_token() const volatile { return ((const continuation_dispatched*)this)->m_removeToken; }

	continuation_dispatched(const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t, const priority_queue<int, continuation_dispatched>::remove_token& rt)
		: dispatched(parentDispatcher, t),
		m_removeToken(rt)
	{ }
};


template <>
class task<void> : public waitable, public virtual object
{
protected:
	friend class dispatcher;

	priority_queue<int, continuation_dispatched> m_continuationSubTasks;
	volatile boolean m_continuationSubTaskDrainDone;

	virtual rcptr<volatile dispatched> get_dispatched() const volatile { return rcptr<volatile dispatched>(); }

	virtual rcref<task<bool> > cancel_inner(volatile dispatched& d) volatile
	{
		volatile continuation_dispatched& d2 = *(volatile continuation_dispatched*)&d;
		priority_queue<int, continuation_dispatched>::remove_token& rt = d2.get_remove_token();
		bool b = m_continuationSubTasks.remove(rt).wasRemoved;
		return signaled(b);
	}

	virtual void change_priority_inner(volatile dispatched& d, int newPriority) volatile
	{
		volatile continuation_dispatched& d2 = *(volatile continuation_dispatched*)&d;
		priority_queue<int, continuation_dispatched>::remove_token& rt = d2.get_remove_token();
		m_continuationSubTasks.change_priority(rt, newPriority);
	}

	void cancel_continuations() volatile
	{
		bool setDoneState = false;
		for (;;)
		{
			priority_queue<int, continuation_dispatched>::value_token vt = m_continuationSubTasks.get();
			if (!vt)
			{
				if (setDoneState)
					break;
				setDoneState = true;
				m_continuationSubTaskDrainDone = true;
				continue;
			}
			vt->get_task_base()->cancel();
		}
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		for (;;)
		{
			if (!m_continuationSubTaskDrainDone)
			{
				priority_queue<int, continuation_dispatched>::value_token vt = m_continuationSubTasks.insert_via([&](priority_queue<int, continuation_dispatched>::value_token& vt)
				{
					*const_cast<int*>(&vt.get_priority()) = priority;
					nested_rcnew(&*vt, *vt.get_desc())(this_rcref, t, vt);
					t->set_dispatched(vt.get_obj().dereference().static_cast_to<dispatched>());
				}).valueToken;
				if (!m_continuationSubTaskDrainDone || !m_continuationSubTasks.remove(vt).wasRemoved)
					break; // was handled
			}
			if (is_signaled())
				signal_continuation(*t);
			else
				t->cancel();
			break;
		}
	}

	virtual bool signal_continuation(task_base& t) volatile
	{
		task_arg_base<void>* t2 = (task_arg_base<void>*) & t;
		return t2->signal(this_rcref.template const_cast_to<task<void> >());
	}

	void signal_continuations() volatile
	{
		bool setDoneState = false;
		for (;;)
		{
			priority_queue<int, continuation_dispatched>::value_token vt = m_continuationSubTasks.get();
			if (!vt)
			{
				if (setDoneState)
					break;
				setDoneState = true;
				m_continuationSubTaskDrainDone = true;
				continue;
			}
			signal_continuation(*vt->get_task_base());
		}
	}

	friend class resettable_condition;

	bool signal_one_continuation() volatile
	{
		for (;;)
		{
			priority_queue<int, continuation_dispatched>::value_token vt = m_continuationSubTasks.get();
			if (!vt)
				break;
			if (signal_continuation(*vt->get_task_base()))
				return true;
		}
		return false;
	}

	explicit task(bool signaled = false)
		: m_continuationSubTaskDrainDone(signaled)
	{ }

public:
	typedef void result_type;

	~task()
	{
		for (;;)
		{
			priority_queue<int, continuation_dispatched>::value_token vt = m_continuationSubTasks.get();
			if (!vt)
				break;
			vt->get_task_base()->cancel();
		}
	}

	virtual rcref<task<bool> > cancel() volatile = 0;

	virtual void change_priority(int) volatile { } // default is no-op
	virtual int get_priority() const volatile { return 0; }
};


template <typename result_t>
class task : public task<void>
{
protected:
	using task<void>::signal_continuations;

	static_assert(!std::is_reference_v<result_t>); // task result must be a concrete type

	virtual bool signal_continuation(task_base& t) volatile
	{
		if (!t.needs_arg())
		{
			task_arg_base<void>* t2 = (task_arg_base<void>*)&t;
			return t2->signal(this_rcref.template const_cast_to<task<result_t> >().template static_cast_to<task<void> >());
		}
		else
		{
			task_arg_base<result_t>* t2 = (task_arg_base<result_t>*)&t;
			return t2->signal(this_rcref.template const_cast_to<task<result_t> >());
		}
	}

	explicit task(bool signaled)
		: task<void>(signaled)
	{ }

public:
	typedef result_t result_type;

	task()
	{ }

	virtual rcptr<task<void> > get_chained_task()
	{
		if constexpr (is_rcref_task_v<result_t>)
			return get().template static_cast_to<task<void> >();
		else
		{
			rcptr<task<void> > tmp;
			return tmp;
		}
	}

	using task<void>::dispatch;

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, const result_t&>
		&& !is_rcref_task_v<std::invoke_result_t<F, const result_t&> >,
		rcref<task<std::invoke_result_t<F, const result_t&> > > >
	dispatch(F&& onComplete, int priority = 0) const volatile;

	template <typename F1, typename F2>
	std::enable_if_t<
		std::is_invocable_v<F1, const result_t&>
		&& std::is_invocable_v<F2>
		&& !is_rcref_task_v<std::invoke_result_t<F1, const result_t&> >,
		rcref<task<std::invoke_result_t<F1, const result_t&> > > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) const volatile;

	template <typename F>
	inline std::enable_if_t<
		std::is_invocable_v<F, const result_t&>
		&& is_rcref_task_v<std::invoke_result_t<F, const result_t&> >,
		rcref<task<typename std::invoke_result_t<F, const result_t&>::type::result_type> > >
	dispatch(F&& onComplete, int priority = 0) const volatile;

	template <typename F1, typename F2>
	inline std::enable_if_t<
		std::is_invocable_v<F1, const result_t&>
		&& std::is_invocable_v<F2>
		&& is_rcref_task_v<std::invoke_result_t<F1, const result_t&> >,
		rcref<task<typename std::invoke_result_t<F1, const result_t&>::type::result_type> > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) const volatile;


	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, const rcref<task<result_t> >&>
		&& !is_rcref_task_v<std::invoke_result_t<F, const rcref<task<result_t> >&> >,
		rcref<task<std::invoke_result_t<F, const rcref<task<result_t> >&> > > >
	dispatch(F&& onComplete, int priority = 0) const volatile;

	template <typename F1, typename F2>
	std::enable_if_t<
		std::is_invocable_v<F1, const rcref<task<result_t> >&>
		&& std::is_invocable_v<F2>
		&& !is_rcref_task_v<std::invoke_result_t<F1, const rcref<task<result_t> >&> >,
		rcref<task<std::invoke_result_t<F1, const rcref<task<result_t> >&> > > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) const volatile;

	template <typename F>
	inline std::enable_if_t<
		std::is_invocable_v<F, const rcref<task<result_t> >&>
		&& is_rcref_task_v<std::invoke_result_t<F, const rcref<task<result_t> >&> >,
		rcref<task<typename std::invoke_result_t<F, const rcref<task<result_t> >&>::type::result_type> > >
	dispatch(F&& onComplete, int priority = 0) const volatile;

	template <typename F1, typename F2>
	inline std::enable_if_t<
		std::is_invocable_v<F1, const rcref<task<result_t> >&>
		&& std::is_invocable_v<F2>
		&& is_rcref_task_v<std::invoke_result_t<F1, const rcref<task<result_t> >&> >,
		rcref<task<typename std::invoke_result_t<F1, const rcref<task<result_t> >&>::type::result_type> > >
	dispatch(F1&& onComplete, F2&& onCancel, int priority = 0) const volatile;


	// It's the responsibility of the caller to add volatility to the reference returned by get()
	// if needed, if the result may be referenced concurrently (i.e. any time there are multiple
	// continuations on the same task, or multiple callers might call get() on the same task).
	// Note that since it's const, it may not require volatility.

	virtual const result_t& get() const volatile = 0; // error to call on incomplete task.
};


// Note: derive from task<void> if the operation can be cancelled, otherwise derive from waitable


inline rcptr<volatile dispatched> dispatcher::get_dispatched(const task<void>& t) { return t.get_dispatched(); }

template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F>
	&& std::is_void_v<std::invoke_result_t<F> >,
	rcref<task<void> > >
dispatcher::dispatch(F&& onComplete, int priority) volatile
{
	rcref<task_base> t = dispatch_default_task(std::forward<F>(onComplete), priority);
	return t->get_task();
}

template <typename F1, typename F2>
inline std::enable_if_t<
	std::is_invocable_v<F1>
	&& std::is_invocable_v<F2>
	&& std::is_void_v<std::invoke_result_t<F1> >,
	rcref<task<void> > >
dispatcher::dispatch(F1&& onComplete, F2&& onCancel, int priority) volatile
{
	rcref<task_base> t = dispatch_default_task(std::forward<F1>(onComplete), std::forward<F2>(onCancel), priority);
	return t->get_task();
}

template <typename result_t>
class signallable_task_base : public task<result_t>
{
protected:
	using task<result_t>::signal_continuations;

	struct task_state
	{
		alignas(atomic::get_alignment_v<size_t>) size_t m_waitingCount;
		alignas(atomic::get_alignment_v<os::semaphore*>) os::semaphore* m_osSemaphore;

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

	static_assert(can_atomic_v<task_state>);

	alignas(atomic::get_alignment_v<task_state>) mutable task_state m_taskState;

	signallable_task_base()
		: m_taskState{ 0, nullptr }
	{ }

	~signallable_task_base()
	{
		COGS_ASSERT(!m_taskState.m_waitingCount); // Should not be possible to destruct when some threads still in timed_wait().
		ptr<os::semaphore> p = m_taskState.m_osSemaphore;
		COGS_ASSERT(!p.get_unmarked()); // Should be no semaphore
	}

	int get_state() const volatile
	{
		ptr<os::semaphore> p = atomic::load(m_taskState.m_osSemaphore);
		return (int)p.get_mark();
	}

	void release_waiting(const task_state& oldTaskState) volatile
	{
		ptr<os::semaphore> osSemaphore = oldTaskState.get_semaphore();
		if (!!osSemaphore)
		{
			COGS_ASSERT(oldTaskState.m_waitingCount > 0);
			osSemaphore->release(oldTaskState.m_waitingCount);
		}
	}

	bool try_set_state(task_state& oldTaskState, int newState) volatile
	{
		atomic::load(m_taskState, oldTaskState);
		for (;;)
		{
			if (oldTaskState.get_state() >= 2)
				return false;
			ptr<os::semaphore> osSemaphore = oldTaskState.get_semaphore();
			task_state newTaskState{ oldTaskState.m_waitingCount, osSemaphore.get_marked(newState) };
			if (atomic::compare_exchange(m_taskState, newTaskState, oldTaskState, oldTaskState))
				return true;
			//continue;
		}
	}

	virtual bool try_cancel(task_state& oldTaskState) volatile
	{
		if (!try_set_state(oldTaskState, 2))
			return false;
		release_waiting(oldTaskState);
		task<result_t>::cancel_continuations();
		return true;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		task_state oldTaskState;
		bool b = try_cancel(oldTaskState);
		return signaled(b);
	}

	virtual bool signal() volatile
	{
		task_state oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		post_signal(oldTaskState);
		return true;
	}

public:
	// Returns -1 if cancelled, 0 if timed-out, 1 if completed
	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		int result = 0;
		unsigned int spinsLeft = spinCount;
		task_state oldTaskState;
		rcptr<os::semaphore> osSemaphoreRc;
		for (;;)
		{
			atomic::load(m_taskState, oldTaskState);
			int state = oldTaskState.get_state();
			if (state == 3) // Completed
			{
				result = 1;
				break;
			}
			if (state == 2) // Cancelled
			{
				result = -1;
				break;
			} // otherwise, 0 or 1 means in progress
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
			task_state newTaskState;
			newTaskState.m_waitingCount = oldTaskState.m_waitingCount + 1;
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
			if (useCachedOsSemaphore) // Store our copy of it in osSemaphoreRc
				osSemaphoreRc.get_desc()->acquire(); // Give one reference to state
			else
			{
				osSemaphoreRc.release(); // Borrow rcptr here to clean up our reference to state semaphore
				osSemaphoreRc.set(osSemaphore, osSemaphore->self_acquire());
			}
			bool acquiredSemaphore = osSemaphore->acquire(timeout);
			atomic::load(m_taskState, oldTaskState);
			for (;;)
			{
				COGS_ASSERT(oldTaskState.get_semaphore() == osSemaphore.get_ptr()); // Should not have changed
				COGS_ASSERT(oldTaskState.m_waitingCount > 0);
				state = oldTaskState.get_state();
				newTaskState.m_waitingCount = oldTaskState.m_waitingCount - 1;
				newTaskState.m_osSemaphore = (newTaskState.m_waitingCount == 0) ? (os::semaphore*)(size_t)state : oldTaskState.m_osSemaphore;
				if (atomic::compare_exchange(m_taskState, newTaskState, oldTaskState, oldTaskState))
					break;
				//continue;
			}
			if (state >= 2)
			{
				result = (state == 2) ? -1 : 1;
				if (!acquiredSemaphore) // If a wake was incurred for us, before we decremented
				{
					bool b = osSemaphore->acquire();
					COGS_ASSERT(b);
				}
			}
			if (newTaskState.m_waitingCount == 0)
				osSemaphore->self_release();
			break;
		}
		return result;
	}

protected:
	void post_signal(const task_state& oldTaskState) volatile
	{
		release_waiting(oldTaskState);
		signal_continuations();
	}
};


template <typename result_t>
class signallable_task : public signallable_task_base<result_t>
{
private:
	placement<result_t> m_result;

	result_t* get_result_ptr() volatile { return &const_cast<signallable_task<result_t>*>(this)->m_result.get(); }

protected:
	using typename signallable_task_base<result_t>::task_state;
	using signallable_task_base<result_t>::m_taskState;
	using signallable_task_base<result_t>::try_set_state;
	using signallable_task_base<result_t>::post_signal;

public:
	~signallable_task()
	{
		ptr<os::semaphore> p = m_taskState.m_osSemaphore;
		int state = (int)p.get_mark();
		if (state == 3)
			m_result.destruct();
	}

	template <typename... args_t>
	bool signal(args_t&&... a) volatile
	{
		task_state oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		placement_construct(get_result_ptr(), std::forward<args_t>(a)...);
		post_signal(oldTaskState);
		return true;
	}

	virtual const result_t& get() const volatile { return const_cast<const signallable_task<result_t>*>(this)->m_result.get(); }

	using signallable_task_base<result_t>::cancel;
};


template <>
class signallable_task<void> : public signallable_task_base<void>
{
protected:
	void get() const volatile {};

public:
	using signallable_task_base<void>::cancel;
	using signallable_task_base<void>::signal;
};


template <typename result_t, typename arg_t>
class function_task_base : public signallable_task<result_t>, public task_arg_base<arg_t>
{
public:
	using signallable_task<result_t>::get;
	using typename signallable_task<result_t>::task_state;
	using signallable_task<result_t>::try_set_state;
	using signallable_task<result_t>::post_signal;

	function<void()> m_cancelFunc;

	function<void()>& get_cancel_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<function_task_base<result_t, arg_t>*>(this)->m_cancelFunc;
	}

	alignas(atomic::get_alignment_v<int>) int m_priority;

	virtual int get_priority() const volatile
	{
		return atomic::load(m_priority);
	}

	virtual rcref<task<void> > get_task() { return this_rcref; }

	virtual rcptr<volatile dispatched> get_dispatched() const volatile { return task_arg_base<arg_t>::get_dispatched(); }

	explicit function_task_base(int priority)
		: m_priority(priority)
	{
	}

	template <typename F>
	function_task_base(F&& f, int priority)
		: m_cancelFunc(std::forward<F>(f)),
		m_priority(priority)
	{
	}

	virtual bool try_cancel(task_state& oldTaskState) volatile
	{
		if (!signallable_task<result_t>::try_cancel(oldTaskState))
			return false;
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
		invoke_cancel();
		return true;
	}

	virtual bool signal() volatile
	{
		task_state oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		invoke_completion();
		post_signal(oldTaskState);
		return true;
	}

	virtual bool signal(const rcref<task<arg_t> >& parentTask) volatile
	{
		task_state oldTaskState;
		if (!try_set_state(oldTaskState, 3))
			return false;
		invoke_completion(parentTask);
		post_signal(oldTaskState);
		return true;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		return signallable_task<result_t>::cancel();
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
					if (newPriority == newerPriority) // Address race condition in which another thread is also changing the priority
						break;
					newPriority = newerPriority;
				}
			}
		}
	}

protected:
	virtual void invoke_completion() volatile = 0;
	virtual void invoke_completion(const rcref<task<arg_t> >& parentTask) volatile = 0;
	virtual void invoke_cancel() volatile = 0;
};


template <typename result_t, typename arg_t>
class forwarding_function_task : public function_task_base<result_t, arg_t>
{
protected:
	using function_task_base<result_t, arg_t>::get;
	using function_task_base<result_t, arg_t>::get_cancel_func;

	function<result_t(arg_t)> m_primaryFunc;

	function<result_t(arg_t)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<result_t, arg_t>*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<result_t, arg_t>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<result_t, arg_t>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<arg_t> >& parentTask) volatile
	{
		placement_construct(&get(), get_primary_func()(parentTask->get()));
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <typename result_t>
class forwarding_function_task<result_t, void> : public function_task_base<result_t, void>
{
protected:
	using function_task_base<result_t, void>::get;
	using function_task_base<result_t, void>::get_cancel_func;

	function<result_t()> m_primaryFunc;

	function<result_t()>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<result_t, void>*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<result_t, void>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<result_t, void>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		placement_construct(&get(), get_primary_func()());
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_completion(const rcref<task<void> >&) volatile
	{
		invoke_completion();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <typename arg_t>
class forwarding_function_task<void, arg_t> : public function_task_base<void, arg_t>
{
protected:
	using function_task_base<void, arg_t>::get_cancel_func;

	function<void(arg_t)> m_primaryFunc;

	function<void(arg_t)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<void, arg_t>*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<void, arg_t>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<void, arg_t>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<arg_t> >& parentTask) volatile
	{
		get_primary_func()(parentTask->get());
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <>
class forwarding_function_task<void, void> : public function_task_base<void, void>
{
protected:
	using function_task_base<void, void>::get_cancel_func;

	function<void()> m_primaryFunc;

	function<void()>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<void, void>*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<void, void>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<void, void>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		get_primary_func()();
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_completion(const rcref<task<void> >&) volatile
	{
		invoke_completion();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <typename result_t, typename arg_t>
class forwarding_function_task<result_t, rcref<task<arg_t> > > : public function_task_base<result_t, arg_t>
{
protected:
	using function_task_base<result_t, arg_t>::get;
	using function_task_base<result_t, arg_t>::get_cancel_func;

	function<result_t(rcref<task<arg_t> >)> m_primaryFunc;

	function<result_t(rcref<task<arg_t> >)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<result_t, rcref<task<arg_t> > >*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<result_t, arg_t>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<result_t, arg_t>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<arg_t> >& parentTask) volatile
	{
		placement_construct(&get(), get_primary_func()(parentTask));
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};

template <typename arg_t>
class forwarding_function_task<void, rcref<task<arg_t> > > : public function_task_base<void, arg_t>
{
protected:
	using function_task_base<void, arg_t>::get_cancel_func;

	function<void(rcref<task<arg_t> >)> m_primaryFunc;

	function<void(rcref<task<arg_t> >)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<void, rcref<task<arg_t> > >*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<void, arg_t>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<void, arg_t>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<arg_t> >& parentTask) volatile
	{
		get_primary_func()(parentTask);
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <typename result_t>
class forwarding_function_task<result_t, rcref<task<void> > > : public function_task_base<result_t, void>
{
protected:
	using function_task_base<result_t, void>::get;
	using function_task_base<result_t, void>::get_cancel_func;

	function<result_t(rcref<task<void> >)> m_primaryFunc;

	function<result_t(rcref<task<void> >)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<result_t, rcref<task<void> > >*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<result_t, void>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<result_t, void>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<void> >& parentTask) volatile
	{
		placement_construct(&get(), get_primary_func()(parentTask));
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};

template <>
class forwarding_function_task<void, rcref<task<void> > > : public function_task_base<void, void>
{
protected:
	using function_task_base<void, void>::get_cancel_func;

	function<void(rcref<task<void> >)> m_primaryFunc;

	function<void(rcref<task<void> >)>& get_primary_func() volatile
	{
		// Set only on construction, and clearing is synchronized.  Remove volatility.
		return const_cast<forwarding_function_task<void, rcref<task<void> > >*>(this)->m_primaryFunc;
	}

public:
	template <typename F>
	forwarding_function_task(F&& f, int priority)
		: function_task_base<void, void>(priority),
		m_primaryFunc(std::forward<F>(f))
	{
	}

	template <typename F1, typename F2>
	forwarding_function_task(F1&& f1, F2&& f2, int priority)
		: function_task_base<void, void>(std::forward<F2>(f2), priority),
		m_primaryFunc(std::forward<F1>(f1))
	{
	}

	virtual void invoke_completion() volatile
	{
		COGS_ASSERT(false);
	}

	virtual void invoke_completion(const rcref<task<void> >& parentTask) volatile
	{
		get_primary_func()(parentTask);
		get_primary_func().release();
		get_cancel_func().release();
	}

	virtual void invoke_cancel() volatile
	{
		get_cancel_func()();
		get_cancel_func().release();
		get_primary_func().release();
	}
};


template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F>
	&& !std::is_void_v<std::invoke_result_t<F> >
	&& !is_rcref_task_v<std::invoke_result_t<F> >,
	rcref<task<std::invoke_result_t<F> > > >
dispatcher::dispatch(F&& onComplete, int priority) volatile
{
	typedef std::invoke_result_t<F> result_t2;
	typedef forwarding_function_task<result_t2, void> task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F>(onComplete), priority)).template static_cast_to<task<result_t2> >();
	dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


template <typename F1, typename F2>
inline std::enable_if_t<
	std::is_invocable_v<F1>
	&& std::is_invocable_v<F2>
	&& !std::is_void_v<std::invoke_result_t<F1> >
	&& !is_rcref_task_v<std::invoke_result_t<F1> >,
	rcref<task<std::invoke_result_t<F1> > > >
dispatcher::dispatch(F1&& onComplete, F2&& onCancel, int priority) volatile
{
	typedef std::invoke_result_t<F1> result_t2;
	typedef forwarding_function_task<result_t2, void> task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F1>(onComplete), std::forward<F2>(onCancel), priority)).template static_cast_to<task<result_t2> >();
	dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


inline rcref<task_base> dispatcher::dispatch_default_task(const void_function& onComplete, int priority) volatile
{
	typedef forwarding_function_task<void, void> task_t;
	rcref<task_base> t = (rcnew(task_t)(onComplete, priority)).template static_cast_to<task_base>();
	dispatch_inner(t, priority);
	return t;
}


inline rcref<task_base> dispatcher::dispatch_default_task(const void_function& onComplete, const void_function& onCancel, int priority) volatile
{
	typedef forwarding_function_task<void, void> task_t;
	rcref<task_base> t = (rcnew(task_t)(onComplete, onCancel, priority)).template static_cast_to<task_base>();
	dispatch_inner(t, priority);
	return t;
}


template <typename result_t>
template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F, const result_t&>
	&& !is_rcref_task_v<std::invoke_result_t<F, const result_t&> >,
	rcref<task<std::invoke_result_t<F, const result_t&> > > >
task<result_t>::dispatch(F&& onComplete, int priority) const volatile
{
	typedef std::invoke_result_t<F, const result_t&> result_t2;
	typedef forwarding_function_task<result_t2, result_t> task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F>(onComplete), priority)).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


template <typename result_t>
template <typename F1, typename F2>
inline std::enable_if_t<
	std::is_invocable_v<F1, const result_t&>
	&& std::is_invocable_v<F2>
	&& !is_rcref_task_v<std::invoke_result_t<F1, const result_t&> >,
	rcref<task<std::invoke_result_t<F1, const result_t&> > > >
task<result_t>::dispatch(F1&& onComplete, F2&& onCancel, int priority) const volatile
{
	typedef std::invoke_result_t<F1, const result_t&> result_t2;
	typedef forwarding_function_task<result_t2, result_t> task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F1>(onComplete), std::forward<F2>(onCancel), priority)).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


// Used internally by dispatcher::dispatch()
template <typename result_t>
class dispatcher::linked_task : public signallable_task_base<result_t>
{
public:
	typedef signallable_task_base<result_t> base_t;

	rcptr<task<result_t> > m_innerTask1;
	volatile rcptr<task<result_t> > m_innerTask2;
	volatile rcptr<task<void> > m_innerTask3;
	signallable_task<bool> m_cancelTask;
	alignas(atomic::get_alignment_v<int>) int m_priority;

	virtual int get_priority() const volatile
	{
		return atomic::load(m_priority);
	}

	virtual const result_t& get() const volatile
	{
		return m_innerTask2.template const_cast_to<task<result_t> >()->get();
	}

	explicit linked_task(int priority)
		: m_priority(priority)
	{ }

	void signal() volatile
	{
		base_t::signal();
		m_cancelTask.signal(false);
	}

	bool cancel_inner() volatile
	{
		bool b = base_t::cancel()->get(); // signallable_task<bool> will complete immediately.
		m_cancelTask.signal(b);
		return b;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		if (!base_t::is_pending())
			return signaled(false);

		rcptr<task<result_t> > oldValue = m_innerTask2;
		while (!oldValue)
		{
			rcptr<task<void> > newValue;
			newValue.set_mark(1);
			if (m_innerTask2.compare_exchange(newValue, oldValue, oldValue))
			{
				// Don't need volatility to access m_innerTask1
				const_cast<linked_task<result_t>*>(this)->m_innerTask1->cancel();
				break;
			}
		}
		if (!oldValue.get_mark())
			oldValue->cancel();

		return this_rcref.member_cast_to(m_cancelTask).template const_cast_to<signallable_task<bool> >().template static_cast_to<task<bool> >();
	}

	virtual void change_priority(int newPriority) volatile
	{
		if (!base_t::is_pending())
			return;

		int oldPriority;
		atomic::exchange(m_priority, newPriority, oldPriority);
		while (oldPriority != newPriority)
		{
			oldPriority = newPriority;
			atomic::store(m_priority, newPriority);
			rcptr<task<result_t> > oldValue = m_innerTask2;
			if (oldValue.get_mark())
				return; // cancelled
			if (!!oldValue)
				oldValue->change_priority(newPriority);
			else
				m_innerTask1->change_priority(newPriority);
			rcptr<task<void> > oldValue2 = m_innerTask3;
			if (!!oldValue2)
				oldValue2->change_priority(newPriority);
			newPriority = atomic::load(m_priority);
		}
	}
};


template <>
class dispatcher::linked_task<void> : public signallable_task_base<void>
{
private:
	using signallable_task_base<void>::cancel_inner;

public:
	typedef signallable_task_base<void> base_t;

	rcptr<task<void> > m_innerTask1;
	volatile rcptr<task<void> > m_innerTask2;
	volatile rcptr<task<void> > m_innerTask3;
	signallable_task<bool> m_cancelTask;
	alignas(atomic::get_alignment_v<int>) int m_priority;

	virtual int get_priority() const volatile
	{
		return atomic::load(m_priority);
	}

	explicit linked_task(int priority)
		: m_priority(priority)
	{ }

	void signal2() volatile
	{
		base_t::signal();
		m_cancelTask.signal(false);
	}

	bool cancel_inner() volatile
	{
		bool b = base_t::cancel()->get(); // signallable_task<bool> will complete immediately.
		m_cancelTask.signal(b);
		return b;
	}

	virtual rcref<task<bool> > cancel() volatile
	{
		if (!is_pending())
			return signaled(false);

		rcptr<task<void> > oldValue = m_innerTask2;
		while (!oldValue)
		{
			rcptr<task<void> > newValue;
			newValue.set_mark(1);
			if (m_innerTask2.compare_exchange(newValue, oldValue, oldValue))
			{
				// Don't need volatility to access m_innerTask1
				const_cast<linked_task<void>*>(this)->m_innerTask1->cancel();
				break;
			}
		}
		if (!oldValue.get_mark())
			oldValue->cancel();

		return this_rcref.member_cast_to(m_cancelTask).template const_cast_to<signallable_task<bool> >().template static_cast_to<task<bool> >();
	}

	virtual void change_priority(int newPriority) volatile
	{
		if (!base_t::is_pending())
			return;

		int oldPriority;
		atomic::exchange(m_priority, newPriority, oldPriority);
		while (oldPriority != newPriority)
		{
			oldPriority = newPriority;
			atomic::store(m_priority, newPriority);
			rcptr<task<void> > oldValue = m_innerTask2;
			if (oldValue.get_mark())
				return; // cancelled
			if (!!oldValue)
				oldValue->change_priority(newPriority);
			else
				m_innerTask1->change_priority(newPriority);
			rcptr<task<void> > oldValue2 = m_innerTask3;
			if (!!oldValue2)
				oldValue2->change_priority(newPriority);
			newPriority = atomic::load(m_priority);
		}
	}
};


template <typename F, typename... args_t>
inline std::enable_if_t<
	std::is_invocable_v<F>
	&& is_rcref_task_v<std::invoke_result_t<F> >,
	rcref<task<typename std::invoke_result_t<F>::type::result_type> > >
dispatcher::dispatch(F&& onComplete, int priority) volatile
{
	typedef std::invoke_result_t<F> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t;
	rcref<linked_task<result_t> > t = rcnew(linked_task<result_t>)(priority);
	t->m_innerTask1 = dispatch([onComplete{ std::forward<F>(onComplete) }, t]()
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
		else
		{
			rcref<result_task_t> t2 = onComplete();
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t]()
			{
				t->cancel_inner();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t]()
	{
		t->cancel_inner();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


template <typename F1, typename F2, typename... args_t>
inline std::enable_if_t<
	std::is_invocable_v<F1>
	&& std::is_invocable_v<F2>
	&& is_rcref_task_v<std::invoke_result_t<F1> >,
	rcref<task<typename std::invoke_result_t<F1>::type::result_type> > >
dispatcher::dispatch(F1&& onComplete, F2&& onCancel, int priority) volatile
{
	typedef std::invoke_result_t<F1> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t;
	rcref<linked_task<result_t> > t = rcnew(linked_task<result_t>)(priority);
	t->m_innerTask1 = dispatch([onComplete{ std::forward<F1>(onComplete) }, onCancel, t, priority]()
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
		{
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
			onCancel();
		}
		else
		{
			rcref<result_task_t> t2 = onComplete();
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t, onCancel]()
			{
				t->cancel_inner();
				onCancel();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t, onCancel]()
	{
		t->cancel_inner();
		onCancel();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


template <typename result_t>
template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F, const result_t&>
	&& is_rcref_task_v<std::invoke_result_t<F, const result_t&> >,
	rcref<task<typename std::invoke_result_t<F, const result_t&>::type::result_type> > >
task<result_t>::dispatch(F&& onComplete, int priority) const volatile
{
	typedef std::invoke_result_t<F, const result_t&> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t2;
	rcref<linked_task<result_t2> > t = rcnew(linked_task<result_t2>)(priority);
	t->m_innerTask1 = dispatch([onComplete{ std::forward<F>(onComplete) }, t, priority](const result_t& r)
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
		else
		{
			rcref<result_task_t> t2 = onComplete(r);
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t]()
			{
				t->cancel_inner();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t]()
	{
		t->cancel_inner();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


template <typename result_t>
template <typename F1, typename F2>
inline std::enable_if_t<
	std::is_invocable_v<F1, const result_t&>
	&& std::is_invocable_v<F2>
	&& is_rcref_task_v<std::invoke_result_t<F1, const result_t&> >,
	rcref<task<typename std::invoke_result_t<F1, const result_t&>::type::result_type> > >
task<result_t>::dispatch(F1&& onComplete, F2&& onCancel, int priority) const volatile
{
	typedef std::invoke_result_t<F1, const result_t&> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t2;
	rcref<linked_task<result_t2> > t = rcnew(linked_task<result_t2>)(priority);
	rcref<result_task_t> innerTask1 = dispatch([onComplete{ std::forward<F1>(onComplete) }, onCancel, t, priority](const result_t& r)
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
		{
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
			onCancel();
		}
		else
		{
			rcref<result_task_t> t2 = onComplete(r);
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t, onCancel]()
			{
				t->cancel_inner();
				onCancel();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t, onCancel]()
	{
		t->cancel_inner();
		onCancel();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


template <typename result_t>
template <typename F>
std::enable_if_t<
	std::is_invocable_v<F, const rcref<task<result_t> >&>
	&& !is_rcref_task_v<std::invoke_result_t<F, const rcref<task<result_t> >&> >,
	rcref<task<std::invoke_result_t<F, const rcref<task<result_t> >&> > > >
task<result_t>::dispatch(F&& onComplete, int priority) const volatile
{
	typedef std::invoke_result_t<F, const rcref<task<result_t> >&> result_t2;
	typedef forwarding_function_task<result_t2, rcref<task<result_t> > > task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F>(onComplete), priority)).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}

template <typename result_t>
template <typename F1, typename F2>
std::enable_if_t<
	std::is_invocable_v<F1, const rcref<task<result_t> >&>
	&& std::is_invocable_v<F2>
	&& !is_rcref_task_v<std::invoke_result_t<F1, const rcref<task<result_t> >&> >,
	rcref<task<std::invoke_result_t<F1, const rcref<task<result_t> >&> > > >
task<result_t>::dispatch(F1&& onComplete, F2&& onCancel, int priority) const volatile
{
	typedef std::invoke_result_t<F1, const rcref<task<result_t> >&> result_t2;
	typedef forwarding_function_task<result_t2, rcref<task<result_t> >> task_t;
	rcref<task<result_t2> > t = (rcnew(task_t)(std::forward<F1>(onComplete), std::forward<F2>(onCancel), priority)).template static_cast_to<task<result_t2> >();
	((volatile task<result_t>*)this)->dispatch_inner(t.template static_cast_to<task_t>().template static_cast_to<task_base>(), priority);
	return t;
}


template <typename result_t>
template <typename F>
inline std::enable_if_t<
	std::is_invocable_v<F, const rcref<task<result_t> >&>
	&& is_rcref_task_v<std::invoke_result_t<F, const rcref<task<result_t> >&> >,
	rcref<task<typename std::invoke_result_t<F, const rcref<task<result_t> >&>::type::result_type> > >
task<result_t>::dispatch(F&& onComplete, int priority) const volatile
{
	typedef std::invoke_result_t<F, const rcref<task<result_t> >&> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t2;
	rcref<linked_task<result_t2> > t = rcnew(linked_task<result_t2>)(priority);
	t->m_innerTask1 = dispatch([onComplete{ std::forward<F>(onComplete) }, t, priority](const rcref<task<result_t> >& r)
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
		else
		{
			rcref<result_task_t> t2 = onComplete(r);
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t]()
			{
				t->cancel_inner();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t]()
	{
		t.cancel_inner();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


template <typename result_t>
template <typename F1, typename F2>
inline std::enable_if_t<
	std::is_invocable_v<F1, const rcref<task<result_t> >&>
	&& std::is_invocable_v<F2>
	&& is_rcref_task_v<std::invoke_result_t<F1, const rcref<task<result_t> >&> >,
	rcref<task<typename std::invoke_result_t<F1, const rcref<task<result_t> >&>::type::result_type> > >
task<result_t>::dispatch(F1&& onComplete, F2&& onCancel, int priority) const volatile
{
	typedef std::invoke_result_t<F1, const rcref<task<result_t> >&> result_rcref_task_t;
	typedef typename result_rcref_task_t::type result_task_t;
	typedef typename result_task_t::result_type result_t2;
	rcref<linked_task<result_t2> > t = rcnew(linked_task<result_t2>)(priority);
	t->m_innerTask1 = dispatch([onComplete{ std::forward<F1>(onComplete) }, onCancel, t, priority](const rcref<task<result_t> >& r)
	{
		rcptr<task<void> > oldValue = t->m_innerTask2;
		if (!!oldValue) // Marked, cancel is in progress
		{
			t->cancel_inner(); // We consider cancellation successful as long as the last in the chain does not compelete.
			onCancel();
		}
		else
		{
			rcref<result_task_t> t2 = onComplete(r);
			int priority2 = t->get_priority();
			if (!t->m_innerTask2.compare_exchange(t2, oldValue, oldValue)) // If it was cancelled, but we've already started the next task.
				t2->cancel();
			else
				t2->change_priority(priority2);
			t->m_innerTask3 = t2->dispatch([t]()
			{
				t->signal2();
			}, [t, onCancel]()
			{
				t->cancel_inner();
				onCancel();
			}, priority2);
			for (;;)
			{
				int priority3 = t->get_priority();
				if (priority3 == priority2)
					break;
				t->m_innerTask2.get_unmarked()->change_priority(priority3);
				t->m_innerTask3->change_priority(priority3);
				priority2 = priority3;
			}
		}
	}, [t, onCancel]()
	{
		t.cancel_inner();
		onCancel();
	}, priority);
	return t.template static_cast_to<task<result_t> >();
}


}


#include "cogs/sync/singleton.hpp"


#endif
