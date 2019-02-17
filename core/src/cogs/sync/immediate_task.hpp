//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_IMMEDIATE_TASK
#define COGS_HEADER_SYNC_IMMEDIATE_TASK


#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/singleton.hpp"


namespace cogs {



template <typename result_t>
class immediate_task : public task<result_t>
{
private:
	typedef immediate_task<result_t> this_t;

	result_t m_result;

	virtual rcref<task<void> > get_task() { return this_rcref; }

	virtual rcref<task_base> dispatch_default_task(const void_function& f, int priority) volatile
	{
		f();
		return get_immediate_task().static_cast_to<immediate_task<void> >().static_cast_to<task_base>();
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		signal_continuation(*t);
	}

	immediate_task() = delete;
	immediate_task(const this_t&) = delete;
	immediate_task(this_t&&) = delete;
	this_t& operator=(const this_t&) = delete;
	this_t& operator=(this_t&&) = delete;

protected:
	immediate_task(const result_t& r)
		: task<result_t>(true),
		m_result(r)
	{ }

	immediate_task(result_t&& r)
		: task<result_t>(true),
		m_result(std::move(r))
	{ }

public:
	template <typename T>
	static rcref<task<std::remove_reference_t<T> > > create(T&& t)
	{
		return rcnew(bypass_constructor_permission<this_t>, std::forward<T>(t));
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return 1; }

	virtual const result_t& get() const volatile { return *const_cast<const result_t*>(&m_result); }

	virtual rcref<task<bool> > cancel() volatile { return get_immediate_task(false); }
};


template <typename T>
inline rcref<task<std::remove_reference_t<T> > > get_immediate_task(T&& t)
{
	return immediate_task<std::remove_reference_t<T> >::create(std::forward<T>(t));
}


// Use singletons for true and false

inline rcref<task<bool> > get_immediate_task(bool b)
{
	class immediate_task_true : public immediate_task<bool>
	{
	public:
		immediate_task_true() : immediate_task<bool>(true) { }
	};

	class immediate_task_false : public immediate_task<bool>
	{
	public:
		immediate_task_false() : immediate_task<bool>(false) { }
	};

	if (b)
		return singleton<immediate_task_true>::get();
	return singleton<immediate_task_false>::get();
}


template <>
class immediate_task<void> : public task<void>, public task_arg_base<void>
{
private:
	typedef immediate_task<void> this_t;

	virtual rcref<task<void> > get_task() { return this_rcref; }

	virtual bool signal() volatile { return false; }

	virtual rcref<task_base> dispatch_default_task(const void_function& f, int priority) volatile
	{
		f();
		return this_rcref.const_cast_to<this_t>().static_cast_to<task_base>();
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		signal_continuation(*t);
	}

	immediate_task(const this_t&) = delete;
	immediate_task(this_t&&) = delete;
	this_t& operator=(const this_t&) = delete;
	this_t& operator=(this_t&&) = delete;

protected:
	immediate_task()
		: task<void>(true)
	{ }

public:
	static rcref<task<void> > create() { return singleton<immediate_task<void> >::get(); }

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return 1; }

	virtual rcref<task<bool> > cancel() volatile { return get_immediate_task(false); }
};


inline rcref<task<void> > get_immediate_task()
{
	return immediate_task<void>::create();
}


}


#endif
