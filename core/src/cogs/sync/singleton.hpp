//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_SYNC_SINGLETON
#define COGS_HEADER_SYNC_SINGLETON

#include "cogs/env.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {

// singleton lifecycle is thread-safe and lock-free.

enum class singleton_posthumous_behavior
{
	create_new_singleton,
	create_new_per_caller,
	return_null
};


enum class singleton_cleanup_behavior
{
	use_cleanup_queue,
	must_call_shutdown
};


template <typename T,
	singleton_posthumous_behavior posthumous_behavior = singleton_posthumous_behavior::create_new_singleton,
	singleton_cleanup_behavior cleanup_behavior  = singleton_cleanup_behavior::use_cleanup_queue>
class singleton;


template <typename T>
class singleton_base
{
protected:
	template <typename, singleton_posthumous_behavior, singleton_cleanup_behavior>
	friend class singleton;

	inline static placement<weak_rcptr<T> > s_global;
	inline static placement<boolean> s_isReleased;

	template <singleton_posthumous_behavior posthumous_behavior, singleton_cleanup_behavior cleanup_behavior>
	static rcptr<T> get(bool& isNew);
	//{
	//	isNew = false;
	//	rcptr<T> result;
	//	volatile weak_rcptr<T>* g = &s_global.get();
	//	weak_rcptr<T> oldValue = *g;
	//
	//	bool b;
	//	if (posthumous_behavior == singleton_posthumous_behavior::create_new_singleton)
	//	{
	//		result = oldValue;
	//		b = !result;
	//	}
	//	else
	//	{
	//		b = false;
	//		if (oldValue.get_desc() != nullptr)
	//			result = oldValue; // won't be marked, if desc was not nullptr
	//		else if (oldValue.get_mark() == 0) // not set up yet
	//			b = true;
	//	}
	//
	//	if (b)
	//	{
	//		rcptr<T> newValue = rcnew(bypass_constructor_permission<T>);
	//		newValue.get_desc()->acquire(strong);
	//		if (!g->compare_exchange(newValue, oldValue, oldValue))
	//		{
	//			newValue.get_desc()->release(strong);
	//			if (posthumous_behavior == singleton_posthumous_behavior::create_new_singleton || oldValue.get_mark() == 0)
	//				result = oldValue;
	//		}
	//		else
	//		{
	//			isNew = true;
	//			result = std::move(newValue); // Return the one we just created.
	//			if (cleanup_behavior == singleton_cleanup_behavior::use_cleanup_queue)
	//				cleanup_queue::get()->dispatch(&singleton_base<T>::shutdown<posthumous_behavior>);
	//		}
	//	}
	//
	//	if (posthumous_behavior == singleton_posthumous_behavior::create_new_per_caller && !result)
	//		result = rcnew(bypass_constructor_permission<T>);
	//
	//	return result;
	//}

	template <singleton_posthumous_behavior posthumous_behavior>
	static void shutdown()
	{
		volatile weak_rcptr<T>* g = &s_global.get();
		if (posthumous_behavior == singleton_posthumous_behavior::create_new_singleton)
		{
			weak_rcptr<T> tmp;
			g->swap(tmp);
			if (tmp.get_desc() != nullptr)
				tmp.get_desc()->release(strong);
		}
		else
		{
			weak_rcptr<T> tmp;
			tmp.set_to_mark(1);
			g->swap(tmp);
			volatile boolean* b = &s_isReleased.get();
			if ((tmp.get_desc() != nullptr) && (b->compare_exchange(true, false)))
				tmp.get_desc()->release(strong);
		}
	}


	// Returns null if never allocated or was already shut down.
	// Safe to call 0 or N times - will be released on the first release.
	static rcptr<T> release()
	{
		volatile weak_rcptr<T>* g = &s_global.get();
		weak_rcptr<T> oldValue = *g;
		rcptr<T> result;
		for (;;)
		{
			if (oldValue.get_desc() == nullptr)
			{
				if (oldValue.get_mark() != 0)
					break;
				weak_rcptr<T> tmp;
				tmp.set_to_mark(1);
				if (g->compare_exchange(tmp, oldValue, oldValue))
					break;
				if (oldValue.get_mark() != 0)
					break;
			}
			result = oldValue;
			if (!result)
				break;
			volatile boolean * b = &s_isReleased.get();
			if (b->compare_exchange(true, false))
				result.get_desc()->release(strong);
			break;
		}
		return result;
	}
};

template <typename T>
class singleton<T, singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::use_cleanup_queue> : public singleton_base<T>
{
public:
	static rcref<T> get() { bool isNew; return get(isNew); }
	static rcref<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::use_cleanup_queue>(isNew)).dereference(); }
};


template <typename T>
class singleton<T, singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::must_call_shutdown> : public singleton_base<T>
{
public:
	static rcref<T> get() { bool isNew; return get(isNew); }
	static rcref<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::must_call_shutdown>(isNew)).dereference(); }

	static void shutdown() { singleton_base<T>::template shutdown<singleton_posthumous_behavior::create_new_singleton>(); }
};

template <typename T>
class singleton<T, singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::must_call_shutdown> : public singleton_base<T>
{
public:
	static rcptr<T> get() { bool isNew; return get(isNew); }
	static rcptr<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::must_call_shutdown>(isNew)); }

	static rcptr<T> release() { return singleton_base<T>::release(); }
	static void shutdown() { singleton_base<T>::template shutdown<singleton_posthumous_behavior::return_null>(); }
};

template <typename T>
class singleton<T, singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::use_cleanup_queue> : public singleton_base<T>
{
public:
	static rcptr<T> get() { bool isNew; return get(isNew); }
	static rcptr<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::use_cleanup_queue>(isNew)); }
};

template <typename T>
class singleton<T, singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::must_call_shutdown> : public singleton_base<T>
{
public:
	static rcref<T> get() { bool isNew; return get(isNew); }
	static rcref<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::must_call_shutdown>(isNew)).dereference(); }

	static rcptr<T> release() { return singleton_base<T>::release(); }

	static void shutdown() { singleton_base<T>::template shutdown<singleton_posthumous_behavior::create_new_per_caller>(); }
};

template <typename T>
class singleton<T, singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::use_cleanup_queue> : public singleton_base<T>
{
public:
	static rcref<T> get() { bool isNew; return get(isNew); }
	static rcref<T> get(bool& isNew) { return (singleton_base<T>::template get<singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::use_cleanup_queue>(isNew)).dereference(); }
};


}


#include "cogs/sync/cleanup_queue.hpp"


#endif
