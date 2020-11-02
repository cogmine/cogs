//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SEMAPHORE
#define COGS_HEADER_SYNC_SEMAPHORE


#include "cogs/env.hpp"
#include "cogs/os/sync/semaphore.hpp"
#include "cogs/os/sync/thread.hpp"
#include "cogs/mem/rcref_freelist.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


// A properly implemented semaphore should allow the acquiring of more than
// one reference at a time, without creating a potential deadlock.
// For example, if 2 threads both attempt to acquire both of 2 resources, a
// deadlock should not occur due to each acquiring only one.
//
// This implementation works around this problem but has some consequences on
// performance.
//	- When requesting 1 or equal refs at a time, native thread priority is honored.
//	- When mixing requests of unequal # of refs, one may reserve some of those
//		resources without yet waking, and block requests for fewer despite sufficient
//		resources being available.  Larger requests will not be blocked.  This is
//		rather 'fair'.  Assuming it is possible to satisfy the larger requests, no
//		request is ultimately blocked from occuring.
//	- Each reference request has an equivalent likelihood of reciving a new resource,
//		but multi-reference requests share a single accumulation.  Requests for fewer
//		references are more likely to wake than requests for greater numbers of references.
//	- Each release that does not yet wake a request for multiple, will incur an
//		unnecessary wake and re-stall of a thread.  (kernel mode transitions, paging of
//		thread resources such as the stack)
//	- Because the woken thread removes the os::semaphore, it may linger between when the
//		waker triggers the wake, and the woken wakes up.  Other threads may wait on it
//		unnecessarily, incurring kernel mode transitions (but no wait).
//
// If not attempting to acquire multiple resources at a time, behavior is the same
// as the native OS semaphore.

/// @ingroup Synchronization
/// @brief A semaphore.
class semaphore
{
private:
	class content_t
	{
	public:
		rcptr<os::semaphore> m_osSemaphore;
		ptrdiff_t m_resourceCount;// dec'ed by woken thread.
		size_t m_stallCount; // Number of threads stalled, inc'ed before waiting, dec'ed by woken threads

		// Number of threads waiting to wake, inc'ed by waker thread, dec's by woken thread
		// Helps ensure timeouts don't lead to false-wakes on os::semaphore
		size_t m_wakeCount;

		explicit content_t(ptrdiff_t n = 0)
			: m_resourceCount(n),
			 m_stallCount(0),
			m_wakeCount(0)
		{ }
	};

	typedef transactable<content_t> transactable_t;
	typedef transactable_t::read_token read_token;
	typedef transactable_t::write_token write_token;

	mutable volatile transactable_t m_contents;
	const ptrdiff_t m_maxResources;

	semaphore(const semaphore&);
	semaphore& operator=(const semaphore&);

	typedef rcref_freelist<os::semaphore, 10> os_semaphore_freelist_t;

	inline static placement<rcptr<os_semaphore_freelist_t> > s_osSemaphoreFreeList;

	static rcref<os_semaphore_freelist_t> get_os_semaphore_freelist()
	{
		volatile rcptr<os_semaphore_freelist_t>* freeList = &s_osSemaphoreFreeList.get();
		rcptr<os_semaphore_freelist_t> myFreeList = *freeList;
		if (!myFreeList)
		{
			rcptr<os_semaphore_freelist_t> newFreeList = rcnew(os_semaphore_freelist_t);
			if (freeList->compare_exchange(newFreeList, myFreeList, myFreeList))
				myFreeList = newFreeList; // Return the one we just created.
		}
		return myFreeList.dereference();
	}

	// Called only when the default thread pool terminates, and there is no risk of future calls to get_os_semaphore_freelist()
	static void shutdown_os_semaphore_freelist()
	{
		volatile rcptr<os_semaphore_freelist_t>* freeList = &s_osSemaphoreFreeList.get();
		freeList->release();
	}

	friend class thread_pool;

public:
	// Convenient place to doll out os semaphores from a freelist.
	// OS semaphores should not be acquired until they are actually needed to block a thread.
	// This keeps this number of OS semaphores to approximately the maximum number of threads that needed
	// to be blocked at the same time, on different synchronization objects.
	static rcref<os::semaphore> get_os_semaphore()
	{
		return get_os_semaphore_freelist()->get();
	}

	semaphore()
		: m_maxResources(0)
	{ }

	explicit semaphore(size_t n, size_t maxResources = 0)
		: m_contents(typename transactable_t::construct_embedded_t(), n),
		 m_maxResources(maxResources)
	{ }

	// Waits for at least 1 to be available, but will acquire as many as present, if multiple.
	size_t acquire_any(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		unsigned int spinsLeft = (os::thread::get_processor_count() == 1) ? 0 : spinCount;
		size_t result = 0;
		rcptr<os::semaphore> osSemaphore = 0;
		rcptr<os::semaphore> newOsSemaphore;
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_resourceCount <= 0)
			{
				if (!timeout)
					break;
				if (!!spinsLeft)
				{
					--spinsLeft;
					if (os::thread::spin_once())
						continue;
				}
			}
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			if (wt->m_resourceCount > 0)
			{
				result = (size_t)(wt->m_resourceCount);
				wt->m_resourceCount = 0;
			}
			else
			{
				if (!wt->m_osSemaphore)
				{
					if (!newOsSemaphore)
						newOsSemaphore = get_os_semaphore();
					wt->m_osSemaphore = newOsSemaphore;
				}
				osSemaphore = wt->m_osSemaphore;
				wt->m_stallCount++;
			}
			if (m_contents.end_write(wt))
				break;
			osSemaphore = 0;
			result = 0;
			//continue;
		}
		newOsSemaphore = 0;
		rt.release();
		if (!!osSemaphore)
		{
			bool expired;
			do {
				expired = !osSemaphore->acquire(timeout);
				do {
					result = 0;
					m_contents.begin_write(wt);

					if (!expired)
					{
						if (!!wt->m_wakeCount) // Otherwise, someone else took it, no need to dec it.
							wt->m_wakeCount--;
					}
					else if (wt->m_wakeCount == wt->m_stallCount) // wakeCount will/must never exceed stallCount
					{
						// This handles the race condition in which a timeout expired just as another thread was
						// trying to release a resource to us.  Normally, we would let one of the other threads
						// have it, but if all threads are intended to be woken, we need to claim one ourselves.
						// It may leave a superfluous wake on the os::semaphore, but that's OK.
						wt->m_wakeCount--;
					}

					if (wt->m_resourceCount > 0)
					{
						result = (size_t)(wt->m_resourceCount);
						wt->m_resourceCount = 0;
					}

					if (result || expired) // If we're not going to wait again, dec the stallCount
					{
						size_t stallCount = --(wt->m_stallCount);
						if (!stallCount) // If no more stalled threads, release the osSemaphore
							wt->m_osSemaphore = 0;
					}

				} while (!m_contents.end_write(wt));
			} while (!result && !expired);
		}
		return result;
	}

	bool acquire(ptrdiff_t n = 1, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		if (n <= 0)
		{
			if (n < 0)
				release(-n);
			return true;
		} // ensure n is positive

		unsigned int spinsLeft = (os::thread::get_processor_count() == 1) ? 0 : spinCount;
		bool result = false;
		rcptr<os::semaphore> osSemaphore;
		rcptr<os::semaphore> newOsSemaphore;
		read_token rt;
		write_token wt;
		for (;;)
		{
			m_contents.begin_read(rt);
			if (rt->m_resourceCount < n)
			{
				if (!timeout)
					break;
				if (!!spinsLeft)
				{
					--spinsLeft;
					if (os::thread::spin_once())
						continue;
				}
			}
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			for (;;)
			{
				if (wt->m_resourceCount > 0)
				{
					size_t available = (size_t)(wt->m_resourceCount);
					if (available > wt->m_stallCount)
					{
						available -= wt->m_stallCount; // Leave one resource per thread currently stalled
						if (available >= (size_t)n)
						{
							wt->m_resourceCount -= n;
							result = true;
							break;
						}
					}
				}
				if (!wt->m_osSemaphore)
				{
					if (!newOsSemaphore)
						newOsSemaphore = get_os_semaphore();
					wt->m_osSemaphore = newOsSemaphore;
				}
				osSemaphore = wt->m_osSemaphore;
				wt->m_stallCount++;
				break;
			}
			if (m_contents.end_write(wt))
				break;
			result = false;
			osSemaphore = 0;
		}
		newOsSemaphore = 0;

		if (!!osSemaphore)
		{
			bool expired;
			do {
				expired = !osSemaphore->acquire(timeout);
				size_t nWake;
				do {
					result = false;
					nWake = 0;
					m_contents.begin_write(wt);
					if (!expired)
					{
						if (!!wt->m_wakeCount) // Otherwise, someone else took it, no need to dec it.
							wt->m_wakeCount--;
					}
					else if (wt->m_wakeCount == wt->m_stallCount) // wakeCount will/must never exceed stallCount
					{
						// This handles the race condition in which a timeout expired just as another thread was
						// trying to release a resource to us.  Normally, we would let one of the other threads
						// have it, but if all threads are intended to be woken, we need to claim one ourselves.
						// It may leave a superfluous wake on the os::semaphore, but that's OK.
						wt->m_wakeCount--;
					}

					if (wt->m_resourceCount >= n) // If enough resources, we take them.
					{ // Doesn't matter if our waking was intentional or an expiration.
						result = true;
						wt->m_resourceCount -= n;
					}

					if (result || expired) // If we're not going to wait again, dec the stallCount
					{
						size_t stallCount = --(wt->m_stallCount);
						if (!stallCount) // If no more stalled threads, release the osSemaphore
							wt->m_osSemaphore = 0;

						// If this waiter is done, and had been waiting on multiple resources, it's possible a condition
						// has arisen in which all waiting threads could actually get their requested resources.  Since
						// all may be 1-resource waiters (which won't make this correct themselves), we need to ensure
						// one waiter per available resources is given a chance to rechec.

						if (n > 1)
						{
							size_t resCount = 0;
							if (wt->m_resourceCount > 0)
								resCount = (size_t)(wt->m_resourceCount);

							size_t wakeCount = wt->m_wakeCount;
							if ((resCount > wakeCount) && (stallCount > wakeCount))
							{
								nWake = (resCount <= stallCount) ? resCount : stallCount;
								nWake -= wakeCount;

								wt->m_wakeCount += nWake;
							}
						}
					}
				} while (!m_contents.end_write(wt));

				if (!!nWake)
					osSemaphore->release(nWake);

			} while (!result && !expired);
		}
		return result;
	}

	void release(size_t n = 1) volatile
	{
		if (n != 0)
		{
			write_token wt;
			size_t numToWake;
			rcptr<os::semaphore> osSemaphore;
			if (m_maxResources != 0)
			{
				read_token rt;
				for (;;)
				{
					numToWake = 0;
					m_contents.begin_read(rt);
					COGS_ASSERT(rt->m_resourceCount <= m_maxResources);
					if (rt->m_resourceCount == m_maxResources)
						break;

					size_t numToAdd = m_maxResources - rt->m_resourceCount;
					if (numToAdd > n)
						numToAdd = n;

					numToWake = rt->m_stallCount - rt->m_wakeCount;
					if (numToWake > numToAdd)
						numToWake = numToAdd; // Wake however many are really stalled, or however many res we are adding, whichever is lesser.

					if (!m_contents.promote_read_token(rt, wt))
						continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition

					wt->m_wakeCount += numToWake;
					wt->m_resourceCount += numToAdd;
					osSemaphore = wt->m_osSemaphore;
					if (m_contents.end_write(wt))
						break;
				}
			}
			else
			{
				do {
					m_contents.begin_write(wt);

					numToWake = wt->m_stallCount - wt->m_wakeCount;
					if (numToWake > n)
						numToWake = n; // Wake however many are really stalled, or however many res we are adding, whichever is lesser.

					wt->m_wakeCount += numToWake;
					wt->m_resourceCount += n;
					osSemaphore = wt->m_osSemaphore;
				} while (!m_contents.end_write(wt));
			}
			if (!!numToWake)
				osSemaphore->release(numToWake);
		}
	}

	// shrink() will asynchronously reduce the number of resources in circulation.
	// If as many resources are present in the object, they are removed.
	// If fewer resources than requested to shrink() are available, the count becomes
	// negative, and subsequent release()'s will increment the resource count without
	// waking threads until the count is back in the positive.
	void shrink(size_t n = 1) volatile
	{
		if (n != 0)
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->m_resourceCount -= n;
			} while (!m_contents.end_write(wt));
		}
	}
};

class auto_semaphore
{
private:
	rcptr<semaphore> m_semaphore;
	unsigned int m_count;

	auto_semaphore(const auto_semaphore&);
	auto_semaphore& operator=(const auto_semaphore&);

public:
	auto_semaphore()
		: m_count(0)
	{ }

	explicit auto_semaphore(const rcref<semaphore>& s, unsigned int n = 1)
		: m_semaphore(s), m_count(n)
	{
		s->acquire(n);
	}

	~auto_semaphore()
	{
		if (m_count)
			m_semaphore->release(m_count);
	}

	void acquire(unsigned int n = 1)
	{
		COGS_ASSERT(!!m_semaphore);
		m_count += n;
		m_semaphore->acquire(n);
	}

	void acquire(const rcref<semaphore>& s, unsigned int n = 1)
	{
		if (m_count)
			m_semaphore->release(m_count);
		m_semaphore = s;
		m_count = n;
		s->acquire(n);
	}

	void release(int n = 1)
	{
		m_semaphore->release(n);
		m_count -= n;
	}

};


}


#endif
