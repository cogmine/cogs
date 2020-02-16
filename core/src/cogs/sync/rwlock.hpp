//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup, NeedsTesting

#ifndef COGS_HEADER_SYNC_RWLOCK
#define COGS_HEADER_SYNC_RWLOCK


#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/sync/semaphore.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief A read/write lock
class rwlock
{
private:
	semaphore m_readerGate;
	semaphore m_writerGate;

	class content_t
	{
	public:
		bool m_blockReaders = false;
		size_t m_readerOwners = 0;
		size_t m_readersStalled = 0;
		size_t m_writersStalled = 0;
	};

	volatile transactable<content_t> m_contents;

	rwlock(const rwlock&);
	rwlock& operator=(const rwlock&);

public:
	rwlock()
	{ }

	~rwlock()
	{
		transactable<content_t>::read_token rt;
		m_contents.begin_read(rt);
		COGS_ASSERT((rt->m_blockReaders == false) && (rt->m_readerOwners == 0)); // Should not be acquired
	}

	bool read_acquire(const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		bool result = true;
		for (;;)
		{
			transactable<content_t>::write_token wt;
			m_contents.begin_write(wt);
			if (!wt->m_blockReaders)
			{
				wt->m_readerOwners++;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}
			// else blocking readers
			wt->m_readersStalled++;
			if (!m_contents.end_write(wt))
				continue;

			bool b = m_readerGate.acquire(1, timeout, spinCount);
			if (b)
				break;

			// If we expired, we need to decrement the reader stall count
			for (;;)
			{
				m_contents.begin_write(wt);
				if (!wt->m_readersStalled)
				{
					// If there is nothing in the reader stall count, readers must have
					// just been woken.  ... Meaning there is an extre release given
					// to m_readerGate().
					m_readerGate.acquire(1);
					break;
				}
				wt->m_readersStalled--;
				if (!m_contents.end_write(wt))
					continue;
				result = false;
				break;
			}
			break;
		}
		return result;
	}

	//void read_notify(const function<void()>& d) volatile
	//{
	//	bool callNow = false;
	//	for (;;)
	//	{
	//		transactable<content_t>::write_token wt;
	//		m_contents.begin_write(wt);
	//		if (!wt->m_blockReaders)
	//		{
	//			wt->m_readerOwners++;
	//			if (!m_contents.end_write(wt))
	//				continue;
	//			callNow = true;
	//			break;
	//		}
	//		// else blocking readers
	//		wt->m_readersStalled++;
	//		if (!m_contents.end_write(wt))
	//			continue;
	//
	//		m_readerGate.dispatch(d);
	//		break;
	//	}
	//	if (callNow)
	//		d();
	//}

	void read_release() volatile
	{
		for (;;)
		{
			transactable<content_t>::write_token wt;
			m_contents.begin_write(wt);
			COGS_ASSERT(wt->m_readerOwners > 0);
			if (wt->m_readerOwners > 1)
			{
				wt->m_readerOwners--;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}
			// Must have been the last owner
			if (wt->m_writersStalled == 0)
			{
				// If there are no waiting writers, then there are no stalled readers
				wt->m_readerOwners = 0;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}
			// Must be a waiting writer to wake
			wt->m_blockReaders = true;
			wt->m_writersStalled--;
			if (!m_contents.end_write(wt))
				continue;
			m_writerGate.release(1);
			break;
		}
	}


	bool write_acquire(bool writePriority = true, const timeout_t& timeout = timeout_t::infinite(), unsigned int spinCount = 0) volatile
	{
		bool result = true;
		for (;;)
		{
			transactable<content_t>::write_token wt;
			m_contents.begin_write(wt);
			if ((wt->m_readerOwners == 0) && (wt->m_blockReaders == false))
			{
				wt->m_blockReaders = true;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}
			if (writePriority)
				wt->m_blockReaders = true;
			wt->m_writersStalled++;

			if (!m_contents.end_write(wt))
				continue;

			bool b = m_writerGate.acquire(1, timeout, spinCount);
			if (b)
				break;

			// expired.  We need to decrement the writer stall count
			for (;;)
			{
				m_contents.begin_write(wt);
				if (!wt->m_writersStalled)
				{
					// If there is nothing in the reader stall count, readers must have
					// just been woken.  ... Meaning there is an extra release given
					// to m_writerGate.
					m_writerGate.acquire(1);
					break;
				}
				wt->m_writersStalled--;
				if (!m_contents.end_write(wt))
					continue;
				result = false;
				break;
			}
			break;
		}
		return result;
	}

	//void write_notify(const function<void()>& d, bool writePriority = true) volatile
	//{
	//	bool callNow = false;
	//	for (;;)
	//	{
	//		transactable<content_t>::write_token wt;
	//		m_contents.begin_write(wt);
	//		if ((wt->m_readerOwners == 0) && (wt->m_blockReaders == false))
	//		{
	//			wt->m_blockReaders = true;
	//			if (!m_contents.end_write(wt))
	//				continue;
	//			callNow = true;
	//			break;
	//		}
	//		if (writePriority)
	//			wt->m_blockReaders = true;
	//		wt->m_writersStalled++;
	//
	//		if (!m_contents.end_write(wt))
	//			continue;
	//
	//		m_writerGate.dispatch(d);
	//		break;
	//	}
	//	if (callNow)
	//		d();
	//}

	void write_release(bool readPriority = false) volatile
	{
		for (;;)
		{
			transactable<content_t>::write_token wt;
			m_contents.begin_write(wt);
			COGS_ASSERT(wt->m_blockReaders == true);

			if ((!wt->m_readersStalled) && (!wt->m_writersStalled))
			{
				wt->m_blockReaders = false;
				if (!m_contents.end_write(wt))
					continue;
				break;
			}

			if (!wt->m_writersStalled || readPriority)
			{
				size_t numStalledReaders = wt->m_readersStalled;
				wt->m_readerOwners = wt->m_readersStalled;
				wt->m_readersStalled = 0;
				wt->m_blockReaders = false;
				if (!m_contents.end_write(wt))
					continue;
				m_readerGate.release((unsigned int)numStalledReaders);
				break;
			}

			wt->m_writersStalled--;
			if (!m_contents.end_write(wt))
				continue;
			m_writerGate.release(1);
			break;
		}
	}

};


}


#endif
