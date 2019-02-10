//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_THREAD_POOL
#define COGS_THREAD_POOL


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/function.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/immediate_task.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/singleton.hpp"


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

		parallel_task()
			:	m_progress(0)
		{ }

		~parallel_task()
		{
			m_doneDelegate();
		}
	};

	typedef container_dlist<parallel_task> parallel_task_list_t;

	class parallel_task_level
	{
	public:
		volatile parallel_task_list_t m_parallelTasks;
		volatile parallel_task_list_t::volatile_iterator m_parallelTaskItor;
		volatile boolean m_alternateFlag;
	};

	typedef map<int, parallel_task_level> parallel_task_level_map_t;

	class main_loop : public object
	{
	public:
		delayed_construction<priority_dispatcher>	m_tasks;

		volatile semaphore					m_semaphore;
		volatile parallel_task_level_map_t	m_parallelTaskLevelMap;
		volatile boolean					m_exiting;

		explicit main_loop(size_t numThreads)
			: m_semaphore(0, numThreads)
		{
			placement_rcnew(this_desc, &m_tasks.get());
		}

		void run()
		{
			parallel_task_level_map_t::volatile_iterator currentPriorityLevelItor;
			for (;;)
			{
				currentPriorityLevelItor = m_parallelTaskLevelMap.get_first();
				if (!currentPriorityLevelItor)
				{
					if (m_tasks->invoke())
						continue;
					
					if (m_exiting)	// Only check of exiting if out of tasks, immediately before acquiring the semaphore.
						break;		// Ensures all tasts have been processed, and we don't consume the semaphore preventing
									// other threads from waking in order to exit.
					m_semaphore.acquire();
					continue;
				}

				if (currentPriorityLevelItor->m_parallelTasks.is_empty())
				{
					m_parallelTaskLevelMap.remove(currentPriorityLevelItor);
					continue;
				}

				int currentPriority = currentPriorityLevelItor.get_key();
				int p;
				rcptr<task<void> > vt = m_tasks->peek(p);
				if (!!vt)
				{
					// If a new task has arrived that is better or same priority as parallel tasks in progress, run it.
					if (p <= currentPriority)
					{
						// If better priority, or we haven't run one yet
						if ((p < currentPriority) || !currentPriorityLevelItor->m_alternateFlag)
						{
							if (!m_tasks->remove_and_invoke(vt.dereference()))
								continue;
							
							// Ran higher priority event, recheck priorities
							if (p < currentPriority)
								continue;
							currentPriorityLevelItor->m_alternateFlag = true;
						}
					}
				}
				
				// Run next parallel task
				parallel_task_list_t::volatile_iterator parallelTaskItor = currentPriorityLevelItor->m_parallelTaskItor++;
				for (;;)
				{
					if (!!parallelTaskItor)
					{
						size_type progress = parallelTaskItor->m_progress;
						bool done;
						for (;;)
						{
							done = progress == parallelTaskItor->m_parallelCount;
							if (done)
								break;
							size_type progressPlusOne = progress;
							++progressPlusOne;
							if (parallelTaskItor->m_progress.compare_exchange(progressPlusOne, progress, progress))
								break;
						}

						if (done)
						{
							currentPriorityLevelItor->m_parallelTasks.remove(parallelTaskItor);
							parallelTaskItor = currentPriorityLevelItor->m_parallelTaskItor++;
							continue;
						}

						parallelTaskItor->m_delegate(progress.get_int());
						currentPriorityLevelItor->m_alternateFlag = false;
						break;
					}
					parallel_task_list_t::volatile_iterator firstItor = currentPriorityLevelItor->m_parallelTasks.get_first();
					if (!firstItor)
					{
						m_parallelTaskLevelMap.remove(currentPriorityLevelItor);
						break;
					}

					currentPriorityLevelItor->m_parallelTaskItor.compare_exchange(firstItor, parallelTaskItor, parallelTaskItor);
				}
				//continue;
			}

		}
	};

	rcref<main_loop>	m_mainLoop;
	const size_t		m_numThreads;

	volatile container_dlist<rcref<thread> >	m_threads;
	volatile uint_type							m_state;	// 0 == not start, 1 == started, 2 == shutdown

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		COGS_ASSERT(m_state == 1);
		dispatcher::dispatch_inner(m_mainLoop->m_tasks.get(), t, priority);
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
			return get_immediate_task().static_cast_to<volatile dispatcher>();
		return threadPool.static_cast_to<volatile dispatcher>().dereference();
	}


	// Intended to be called only right before returning from main().
	static void shutdown_default()
	{
		// Phase 1: Full service cleanup - cleanup queue and threading are still online.
		//
		//	First, try to drain the default global cleanup queue (without taking it offline).
		//	This allows new cleanup requests to be chained after existing ones.
		//	(Because threads are still running, new cleanup requests could still come in late).

		rcref<cleanup_queue> cleanupQueue = cleanup_queue::get_global();
		cleanupQueue->drain();

		// Phase 2: Tear down default thread pool
		//	
		//	Cleanup phase has completed, take default thread pool offline for cleanup.
		//	Threading system uses the same cleanup queue.
		default_thread_pool_singleton_t::release();

		do {
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_LEAKED_BLOCK_DETECTION
			thread::join_all(timeout_t(measure<uint8_type, seconds>((uint8_t)10)));
#else
			thread::join_all();
#endif

			// Phase 3: Take the global cleanup queue offline and drain it again.
			//	
			//	Now that no other threads are running, once the queue is drained, we're done.
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

	explicit thread_pool(bool startNow = false, size_t numThreads = get_default_size())
		: m_numThreads(numThreads),
		m_mainLoop(rcnew(main_loop, numThreads)),
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

	bool start()	// Can only be shutdown once - cannot be restarted.
	{
		bool result = (m_state.compare_exchange(one_t(), zero_t()));
		if (result)
		{
			for (size_t i = 0; i < m_numThreads; i++)
			{
				m_threads.append(thread::spawn([r{ m_mainLoop }]()
				{
					r->run();
				}));
			}
		}
		return result;
	}

	size_t get_num_threads() const	{ return m_numThreads; }

	void dispatch_parallel(size_t n, const function<void(size_t)>& d, const function<void()>& doneDelegate = function<void()>(), int priority = 0) volatile
	{
		COGS_ASSERT(m_state == 1);
		if (!!n)
		{
			parallel_task_list_t::preallocated_t preallocatedTask;
			parallel_task_level_map_t::preallocated_t preallocatedLevel;
			parallel_task_level_map_t::volatile_iterator i;
			for (;;)
			{
				if (!i)
					i = m_mainLoop->m_parallelTaskLevelMap.find(priority);
				if (!!i)
				{
					if (!preallocatedTask)
					{
						preallocatedTask = i->m_parallelTasks.preallocate();
						preallocatedTask->m_delegate = d;
						preallocatedTask->m_doneDelegate = doneDelegate;
						preallocatedTask->m_parallelCount = n;
					}
					parallel_task_list_t::volatile_iterator taskItor = i->m_parallelTasks.prepend_preallocated(preallocatedTask, insert_mode::only_if_not_empty);
					if (!taskItor)	// was not added, preallocatedTask is still good.
					{
						COGS_ASSERT(i->m_parallelTasks.is_empty());
						m_mainLoop->m_parallelTaskLevelMap.remove(i);
						i.release();
						continue;
					}
					m_mainLoop->m_semaphore.release(n);
					break;
				}
				if (!preallocatedLevel)
				{
					preallocatedLevel = m_mainLoop->m_parallelTaskLevelMap.preallocate();
					if (!preallocatedTask)
					{
						preallocatedTask = preallocatedLevel->m_parallelTasks.preallocate();
						preallocatedTask->m_delegate = d;
						preallocatedTask->m_doneDelegate = doneDelegate; 
						preallocatedTask->m_parallelCount = n;
					}
					preallocatedLevel->m_parallelTasks.prepend_preallocated(preallocatedTask);
					preallocatedTask.release();
				}
				bool collision;
				i = m_mainLoop->m_parallelTaskLevelMap.try_insert_preallocated(preallocatedLevel, collision);
				if (!!collision)
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
	bool shutdown()	// Returns false if already shut down.
	{
		uint_type two(2);
		uint_type oldState;
		m_state.exchange(two, oldState);
		bool result = (oldState == 1);
		if (result)
		{
			m_mainLoop->m_exiting = true;
			m_mainLoop->m_semaphore.release(m_numThreads);
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



}


#endif
