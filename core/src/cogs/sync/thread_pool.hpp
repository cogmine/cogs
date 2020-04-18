//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_THREAD_POOL
#define COGS_HEADER_SYNC_THREAD_POOL


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/function.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/quit_dispatcher.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief A thread pool into which delegates can be dispatched
class thread_pool : public dispatcher
{
private:
	class parallel_task
	{
	public:
		function<void(size_t)> m_delegate;
		function<void()> m_doneDelegate;
		size_t m_parallelCount;
		volatile size_type m_progress;

		parallel_task(size_t n, const function<void(size_t)>& d, const function<void()>& doneDelegate)
			: m_delegate(d),
			m_doneDelegate(doneDelegate),
			m_parallelCount(n),
			m_progress(0)
		{ }

		~parallel_task()
		{
			m_doneDelegate();
		}
	};

	typedef container_dlist<rcref<parallel_task> > parallel_task_list_t;

	class parallel_task_level
	{
	public:
		volatile parallel_task_list_t m_parallelTasks;
		volatile parallel_task_list_t::volatile_iterator m_parallelTaskItor;
		volatile boolean m_alternateFlag;

		parallel_task_level() { }

		parallel_task_level(const rcref<parallel_task>& t)
		{
			m_parallelTasks.prepend(t);
		}
	};

	typedef map<int, rcref<parallel_task_level> > parallel_task_level_map_t;

	class main_loop : public object
	{
	public:
		priority_dispatcher m_tasks;

		volatile semaphore m_semaphore;
		volatile parallel_task_level_map_t m_parallelTaskLevelMap;
		volatile boolean m_exiting;

		explicit main_loop(size_t threadCount)
			: m_semaphore(0, threadCount)
		{ }

		void run()
		{
			parallel_task_level_map_t::volatile_iterator currentPriorityLevelItor;
			for (;;)
			{
				currentPriorityLevelItor = m_parallelTaskLevelMap.get_first();
				if (!currentPriorityLevelItor)
				{
					if (m_tasks.invoke())
						continue;

					// Only check of exiting if out of tasks, immediately before acquiring the semaphore.
					// Ensures all tasts have been processed, and we don't consume the semaphore preventing
					// other threads from waking in order to exit.
					if (m_exiting)
						break;
					m_semaphore.acquire();
					continue;
				}

				if (currentPriorityLevelItor->value->m_parallelTasks.is_empty())
				{
					m_parallelTaskLevelMap.remove(currentPriorityLevelItor);
					continue;
				}

				int currentPriority = currentPriorityLevelItor->key;
				int p;
				rcptr<task<void> > vt = m_tasks.peek(p);
				if (!!vt)
				{
					// If a new task has arrived that is better or same priority as parallel tasks in progress, run it.
					if (p <= currentPriority)
					{
						// If better priority, or we haven't run one yet
						if ((p < currentPriority) || !currentPriorityLevelItor->value->m_alternateFlag)
						{
							if (!m_tasks.remove_and_invoke(vt.dereference()))
								continue;

							// Ran higher priority event, recheck priorities
							if (p < currentPriority)
								continue;
							currentPriorityLevelItor->value->m_alternateFlag = true;
						}
					}
				}

				// Run next parallel task
				parallel_task_list_t::volatile_iterator parallelTaskItor = currentPriorityLevelItor->value->m_parallelTaskItor++;
				for (;;)
				{
					if (!!parallelTaskItor)
					{
						size_type progress = (*parallelTaskItor)->m_progress;
						bool done;
						for (;;)
						{
							done = progress == (*parallelTaskItor)->m_parallelCount;
							if (done)
								break;
							size_type progressPlusOne = progress;
							++progressPlusOne;
							if ((*parallelTaskItor)->m_progress.compare_exchange(progressPlusOne, progress, progress))
								break;
						}

						if (done)
						{
							currentPriorityLevelItor->value->m_parallelTasks.remove(parallelTaskItor);
							parallelTaskItor = currentPriorityLevelItor->value->m_parallelTaskItor++;
							continue;
						}

						(*parallelTaskItor)->m_delegate(progress.get_int());
						currentPriorityLevelItor->value->m_alternateFlag = false;
						break;
					}
					parallel_task_list_t::volatile_iterator firstItor = currentPriorityLevelItor->value->m_parallelTasks.get_first();
					if (!firstItor)
					{
						m_parallelTaskLevelMap.remove(currentPriorityLevelItor);
						break;
					}

					currentPriorityLevelItor->value->m_parallelTaskItor.compare_exchange(firstItor, parallelTaskItor, parallelTaskItor);
				}
				//continue;
			}

		}
	};

	const size_t m_threadCount;
	rcref<main_loop> m_mainLoop;

	volatile container_dlist<rcref<thread> > m_threads;
	volatile uint_type m_state; // 0 == not start, 1 == started, 2 == shutdown

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		COGS_ASSERT(m_state == 1);
		dispatcher::dispatch_inner(m_mainLoop->m_tasks, t, priority);
		m_mainLoop->m_semaphore.release();
	}

	typedef singleton<thread_pool, singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::must_call_shutdown>
		default_thread_pool_singleton_t;

public:
	// Will return NULL if default thread pool has already been shut down
	static rcptr<thread_pool> get_default()
	{
		bool isNew;
		rcptr<thread_pool> result = default_thread_pool_singleton_t::get(isNew);
		if (isNew)
			result->start();
		return result;
	}

	static rcref<volatile dispatcher> get_default_or_immediate()
	{
		rcptr<thread_pool> threadPool = get_default();
		if (!threadPool)
			return signaled().template static_cast_to<volatile dispatcher>();
		return threadPool.template static_cast_to<volatile dispatcher>().dereference();
	}


	// Intended to be called only right before returning from main().
	static void shutdown_default()
	{
		// Phase 1: Full service cleanup - cleanup queue and threading are still online.
		//
		//    First, try to drain the default global cleanup queue (without taking it offline).
		//    This allows new cleanup requests to be chained after existing ones.
		//    (Because threads are still running, new cleanup requests could still come in late).

		rcref<cleanup_queue> cleanupQueue = cleanup_queue::get();
		cleanupQueue->drain();

		// Phase 2: Tear down default thread pool
		//
		//    Cleanup phase has completed, take default thread pool offline for cleanup.
		//    Threading system uses the same cleanup queue.
		default_thread_pool_singleton_t::release();

		do {
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_LEAKED_BLOCK_DETECTION
			thread::join_all(timeout_t(measure<uint8_type, seconds>((uint8_t)10)));
#else
			thread::join_all();
#endif

			// Phase 3: Take the global cleanup queue offline and drain it again.
			//
			//    Now that no other threads are running, once the queue is drained, we're done.
			//
		} while (cleanupQueue->drain());

		// If stuck in the above loop, some cleanup is starting too much back up, preventing proper cleanup.
		// Or, something in the cleaup queue keeps putting more in the cleanup queue, preventing proper cleanup.

		singleton<cleanup_queue, singleton_posthumous_behavior::create_new_per_caller, singleton_cleanup_behavior::must_call_shutdown>::shutdown();
		singleton<quit_dispatcher, singleton_posthumous_behavior::return_null, singleton_cleanup_behavior::must_call_shutdown>::shutdown();
		semaphore::shutdown_os_semaphore_freelist();
		default_thread_pool_singleton_t::shutdown();
	}

	static size_t get_default_size()
	{
		unsigned int n = thread::get_processor_count();
		if (n < 2)
			n = 2;
		return n;
	}

	explicit thread_pool(bool startNow = false, size_t threadCount = get_default_size())
		: m_threadCount(threadCount),
		m_mainLoop(rcnew(main_loop)(threadCount)),
		m_state(0)
	{
		if (startNow)
			start();
	}

	~thread_pool()
	{
		shutdown();
		join();
	}

	bool start() // Can only be shutdown once - cannot be restarted.
	{
		bool result = (m_state.compare_exchange(one_t(), zero_t()));
		if (result)
		{
			for (size_t i = 0; i < m_threadCount; i++)
			{
				m_threads.append(thread::spawn([r{ m_mainLoop }]()
				{
					r->run();
				}));
			}
		}
		return result;
	}

	size_t get_thread_count() const { return m_threadCount; }

	template <typename F>
	std::enable_if_t<
		std::is_invocable_v<F, size_t>
		|| std::is_invocable_v<F>,
		void>
	dispatch_parallel(size_t n, F&& f, int priority = 0) volatile
	{
		dispatch_parallel(n, std::forward<F>(f), []() {}, priority);
	}

	template <typename F, typename D>
	std::enable_if_t<
		(std::is_invocable_v<F, size_t> || std::is_invocable_v<F>)
		&& std::is_invocable_v<D>,
		void>
	dispatch_parallel(size_t n, F&& f, D&& doneFunc, int priority = 0) volatile
	{
		COGS_ASSERT(m_state == 1);
		if (!!n)
		{
			rcref<parallel_task> t = rcnew(parallel_task)(n, std::move(f), std::move(doneFunc));
			rcptr<parallel_task_level> level;

			parallel_task_level_map_t::volatile_iterator i;
			for (;;)
			{
				if (!i)
					i = m_mainLoop->m_parallelTaskLevelMap.find(priority);
				if (!!i)
				{
					if (!i->value->m_parallelTasks.prepend_emplace_if_not_empty(t).inserted)
					{
						COGS_ASSERT(i->value->m_parallelTasks.is_empty());
						m_mainLoop->m_parallelTaskLevelMap.remove(i);
						i.release();
						continue;
					}
					m_mainLoop->m_semaphore.release(n);
					break;
				}
				if (!level)
					level = rcnew(parallel_task_level)(t);
				if (!m_mainLoop->m_parallelTaskLevelMap.insert_unique_emplace(priority, level.dereference()).inserted)
					continue;
				m_mainLoop->m_semaphore.release(n);
				break;
			}
		}
	}

	// To maximize efficiency of dispatching tasks to a thread pool, there is no protection
	// against dispatching to a thread pool that has already been shut down.  It is caller
	// error to dispatch to a thread pool that has already shut down, or to race dispatching
	// with shutting down.
	bool shutdown() // Returns false if already shut down.
	{
		uint_type two(2);
		uint_type oldState;
		m_state.exchange(two, oldState);
		bool result = (oldState == 1);
		if (result)
		{
			m_mainLoop->m_exiting = true;
			m_mainLoop->m_semaphore.release(m_threadCount);
		}
		return result;
	}

	void join()
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_LEAKED_BLOCK_DETECTION
		timeout_t timeout(measure<uint8_type, seconds>((uint8_t)10));
#endif
		container_dlist<rcref<thread> >::volatile_iterator i;
		for (;;)
		{
			i = m_threads.get_first();
			if (!i)
				break;

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_LEAKED_BLOCK_DETECTION
			(*i)->join(timeout);
#else
			(*i)->join();
#endif

			m_threads.remove(i);
		}
	}
};


inline void thread::register_waiter(const rcref<thread>& t)
{
	typedef singleton<thread_waiters_t, singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::must_call_shutdown>
		thread_waiters_singleton_t;
	rcref<volatile container_dlist<rcref<thread> > > threadWaiters = thread_waiters_singleton_t::get();
	t->m_removeToken = threadWaiters->prepend(t).inserted;
}

inline void thread::deregister_waiter() const
{
	typedef singleton<thread_waiters_t, singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::must_call_shutdown>
		thread_waiters_singleton_t;
	rcref<volatile container_dlist<rcref<thread> > > threadWaiters = thread_waiters_singleton_t::get();
	threadWaiters->remove(m_removeToken);
}

inline void thread::join_all(const timeout_t& timeout) // To be called in main thread only.  Called automatically at exit.  Do not create new threads after calling.
{
	typedef singleton<thread_waiters_t, singleton_posthumous_behavior::create_new_singleton, singleton_cleanup_behavior::must_call_shutdown>
		thread_waiters_singleton_t;
	rcref<volatile container_dlist<rcref<thread> > > threadWaiters = thread_waiters_singleton_t::get();

	bool anySinceLastLoop = false;
	container_dlist<rcref<thread> >::volatile_iterator itor = threadWaiters->get_first();
	for (;;)
	{
		if (!itor)
		{
			thread_waiters_singleton_t::shutdown();
			break;
		}
		int i = (*itor)->join(timeout);
		COGS_ASSERT(i != 0); // should only be called by main thread
		if (i == 1)
			anySinceLastLoop = true;
		++itor;
		if (!itor)
		{
			if (!anySinceLastLoop)
				break;
			anySinceLastLoop = false;
			itor = threadWaiters->get_first();
		}
	}
}


}


#endif
