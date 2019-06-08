//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_HEADER_MAIN
#define COGS_HEADER_MAIN


#include <mutex>
#include "cogs/env.hpp"
#include "cogs/env/main.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {


class init_token
{
private:
	inline static placement<ptr<std::mutex> > s_mutex;	// zero-initialized, allocation leaked intentionally

	struct count_and_result_t
	{
		alignas (atomic::get_alignment_v<size_t>) size_t m_count;
		alignas (atomic::get_alignment_v<int>) int m_lastResult;
	};

	alignas (atomic::get_alignment_v<count_and_result_t>) inline static count_and_result_t s_countAndResult = { };

	static int initialize()
	{
		volatile count_and_result_t& countAndResult = s_countAndResult;
		count_and_result_t oldValue = atomic::load(countAndResult);
		while (oldValue.m_count > 2)
		{
			count_and_result_t newValue;
			newValue.m_count = oldValue.m_count + 1;
			newValue.m_lastResult = oldValue.m_lastResult;
			if (atomic::compare_exchange(countAndResult, newValue, oldValue))
				return oldValue.m_lastResult;
		}
		volatile ptr<std::mutex>& mtx = s_mutex.get();
		std::mutex* oldMtx = mtx.get_ptr();
		while (!oldMtx)
		{
			std::mutex* newMtx = new std::mutex;
			if (mtx.compare_exchange(newMtx, nullptr, oldMtx))
				oldMtx = newMtx;
			else
				delete newMtx;
			break;
		}
		std::lock_guard<std::mutex> l(*oldMtx);
		oldValue = atomic::load(countAndResult);
		if (oldValue.m_count == 0)
		{
			oldValue.m_count = 1;
			oldValue.m_lastResult = env::initialize();
			atomic::store(countAndResult, oldValue);
		}
		return oldValue.m_lastResult;
	}

	static void terminate()
	{
		volatile count_and_result_t& countAndResult = s_countAndResult;
		count_and_result_t oldValue = atomic::load(countAndResult);
		while (oldValue.m_count > 2)
		{
			count_and_result_t newValue;
			newValue.m_count = oldValue.m_count - 1;
			newValue.m_lastResult = oldValue.m_lastResult;
			if (atomic::compare_exchange(countAndResult, newValue, oldValue))
				return;
		}
		COGS_ASSERT(oldValue.m_count == 1);
		volatile ptr<std::mutex>& mtx = s_mutex.get();
		std::mutex* oldMtx = mtx.get_ptr();
		COGS_ASSERT(!!oldMtx);
		std::lock_guard<std::mutex> l(*oldMtx);
		oldValue = atomic::load(countAndResult);
		COGS_ASSERT(oldValue.m_count > 0);
		if (oldValue.m_count == 1)
		{
			force_quit();
			env::terminate();
			thread_pool::shutdown_default();
			default_allocator::shutdown();
			cogs::assign(countAndResult.m_count, 0);
		}
	}

	friend int initialize();
	friend void terminate();

	bool m_isEmpty = false;

public:
	init_token()
	{
		initialize();
	}

	init_token(init_token&& i)
	{
		m_isEmpty = i.m_isEmpty;
		i.m_isEmpty = true;
	}

	~init_token()
	{
		if (!m_isEmpty)
			terminate();
	}
};


inline int initialize() { return init_token::initialize(); }
inline void terminate() { init_token::terminate(); }

template <typename F>
inline int main(F&& main_func)
{
	int result = 0;
	initialize();
	result = env::main(std::forward<F>(main_func));
	terminate();
	return result;
}


}


#endif
