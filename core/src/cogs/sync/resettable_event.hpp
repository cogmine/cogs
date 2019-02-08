//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_RESETTABLE_EVENT
#define COGS_RESETTABLE_EVENT


#include "cogs/function.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/sync/semaphore.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @ingroup Events
/// @brief A resettable event
class resettable_event : public event_base
{
private:
	class delegates_t : public object
	{
	public:
		delayed_construction<priority_dispatcher>	m_delegates;

		volatile priority_dispatcher& get() volatile	{ return m_delegates.get(); }

		explicit delegates_t()
		{
			placement_rcnew(this_desc, &m_delegates.get());
		}

		void clear()
		{
			m_delegates->clear();
		}

		~delegates_t()
		{
			while (!!m_delegates->invoke())
				;
		}
	};
	
	enum state
	{
		unset_state = 0,
		set_state = 1,
		trigger_state = 2
	};

	class content_t
	{
	public:
		state					m_state;
		rcptr<os::semaphore>	m_osSemaphore;
		size_t					m_stallCount;
		size_t					m_delegateCount;
		rcptr<delegates_t>		m_delegates;

		content_t()
			:	m_state(unset_state),
				m_stallCount(0),
				m_delegateCount(0)
		{ }
	};

	typedef transactable<content_t>::read_token read_token;
	typedef transactable<content_t>::write_token write_token;

	mutable transactable<content_t>	m_contents;

	resettable_event(const resettable_event&) = delete;
	resettable_event& operator=(const resettable_event&) = delete;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		rcptr<delegates_t> newDelegates;
		rcptr<delegates_t> lastSeenDelegates;
		bool insertedToExisting = false;
		for (;;)
		{
			read_token rt;
			m_contents.begin_read(rt);
			if (insertedToExisting)
			{
				if (rt->m_delegates != lastSeenDelegates)	// state has changed sufficiently to already have dispatched.
					break;
			}
			else if (rt->m_state == set_state)
			{
				t->signal();
				if (!!newDelegates)
					newDelegates->clear();
				break;
			}
			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			if (!insertedToExisting)
			{
				if (wt->m_state == trigger_state)
				{
					wt->m_state = unset_state;
					if (!m_contents.end_write(wt))
						continue;
					t->signal();
					if (!!newDelegates)
						newDelegates->clear();
					break;
				}
				if (!!wt->m_delegates)
				{
					// It's actually OK to schedule in multiple priority_dispatchers, as long as the other gets cleared.
					dispatcher::dispatch_inner(wt->m_delegates->get(), t, priority);
					insertedToExisting = true;
					lastSeenDelegates = wt->m_delegates;
				}
				else
				{
					if (!newDelegates)
					{
						newDelegates = rcnew(delegates_t);
						dispatcher::dispatch_inner(newDelegates->get(), t, priority);
					}
					wt->m_delegates = newDelegates;
					lastSeenDelegates = newDelegates;
				}
			}
			++(wt->m_delegateCount);
			if (!m_contents.end_write(wt))
				continue;
			// If inserted to existing, and newDelegates is there, it needs to be cleared.
			if (insertedToExisting && !!newDelegates)
				newDelegates->clear();
			break;
		}
	}

public:
	resettable_event()
	{ }

	~resettable_event()
	{
		if (!!m_contents->m_delegates)
			m_contents->m_delegates->clear();
	}

	// returns true if event was 'unset'.
	virtual bool signal() volatile
	{
		bool result;
		for (;;)
		{
			read_token rt;
			m_contents.begin_read(rt);
			result = (rt->m_state == unset_state);
			if (rt->m_state == set_state)
				break;
			size_t wakeCount = rt->m_stallCount;
			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			wt->m_state = set_state;
			rcptr<os::semaphore> osSemaphore;
			if (wakeCount)
			{
				osSemaphore = wt->m_osSemaphore;
				wt->m_osSemaphore = 0;
				wt->m_stallCount = 0;
			}
			rcptr<delegates_t> delegates = wt->m_delegates;
			wt->m_delegates.release();
			wt->m_delegateCount = 0;
			if (!m_contents.end_write(wt))
				continue;
			if (wakeCount)
				osSemaphore->release(wakeCount);
			// calls delegates on dispose
			break;
		}
		return result;
	}

	// returns true if the event was not 'unset'	(set or triggered)
	bool reset() volatile
	{
		bool wasUnset;
		for (;;)
		{
			read_token rt;
			m_contents.begin_read(rt);
			wasUnset = (rt->m_state == unset_state);
			if (wasUnset)
				break;
			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			wt->m_state = unset_state;
			if (m_contents.end_write(wt))
				break;
		}
		return !wasUnset;
	}

	bool pulse_all() volatile	// returns true if any waiters woken/dispatched 
	{
		bool wokeAny = false;
		for (;;)
		{
			read_token rt;
			m_contents.begin_read(rt);
			// If set, changes to unset.
			// if triggered, change to unset.
			// If unset, wake/dispatch, leave unset
			size_t wakeCount = rt->m_stallCount;
			if (!wakeCount && (rt->m_state == unset_state) && !!rt->m_delegates && !rt->m_delegates->get().is_empty())
				break;
			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			rcptr<os::semaphore> osSemaphore;
			if (!!wakeCount)
			{
				osSemaphore = wt->m_osSemaphore;
				wt->m_stallCount = 0;
				wt->m_osSemaphore = 0;
			}
			rcptr<delegates_t> delegates = wt->m_delegates;
			wt->m_delegates.release();
			wt->m_delegateCount = 0;
			wt->m_state = unset_state;
			if (!m_contents.end_write(wt))
				continue;
			if (!!wakeCount)
			{
				osSemaphore->release(wakeCount);
				wokeAny = true;
			}
			wokeAny = (!!delegates && !delegates->get().is_empty());
			break;
		}
		return wokeAny;
	}

	bool wake_next() volatile	// returns true if event delivered before return.
	{
		bool wokeAny = false;
		for (;;)
		{
			read_token rt;
			m_contents.begin_read(rt);
			if (rt->m_state == trigger_state)	// if trigger, already done
				break;
			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			rcptr<os::semaphore> osSemaphore;
			rcptr<delegates_t> delegates;
			if (!!wt->m_stallCount)
			{
				osSemaphore = wt->m_osSemaphore;
				if (!--(wt->m_stallCount))
					wt->m_osSemaphore.release();
			}
			else if (!!wt->m_delegateCount)
			{
				if (wt->m_delegates->get().is_empty())
				{
					wt->m_delegateCount = 0;
					wt->m_state = trigger_state;
				}
				else
				{
					delegates = wt->m_delegates;
					--(wt->m_delegateCount);
				}
			}
			else
				wt->m_state = trigger_state;
			if (!m_contents.end_write(wt))
				continue;
			if (!!osSemaphore)
			{
				osSemaphore->release(1);
				wokeAny = true;
			}
			else if (!!delegates)
			{
				if (!delegates->get().invoke())
					continue;
				wokeAny = true;
			}
			break;
		}
		return wokeAny;
	}

	bool pulse_one() volatile
	{
		bool wokeAny = false;
		for (;;)
		{
			write_token wt;
			rcptr<os::semaphore> osSemaphore;
			rcptr<delegates_t> delegates;
			read_token rt;
			m_contents.begin_read(rt);
			if ((rt->m_state == unset_state) && !rt->m_stallCount && !rt->m_delegateCount)
				break;
			if (!m_contents.promote_read_token(rt, wt))
				continue;
			if (rt->m_state != unset_state)
				wt->m_state = unset_state;
			else
			{
				if (!!wt->m_stallCount)
				{
					osSemaphore = wt->m_osSemaphore;
					if (!--(wt->m_stallCount))
						wt->m_osSemaphore.release();
				}
				else if (!!wt->m_delegateCount)
				{
					if (wt->m_delegates->get().is_empty())
					{
						wt->m_delegateCount = 0;
						wt->m_state = trigger_state;
					}
					else
					{
						delegates = wt->m_delegates;
						--(wt->m_delegateCount);
					}
				}
				else
					wt->m_state = trigger_state;
			}
			if (!m_contents.end_write(wt))
				continue;
			if (!!osSemaphore)
			{
				osSemaphore->release(1);
				wokeAny = true;
			}
			else if (!!delegates)
			{
				if (!delegates->get().invoke())
					continue;
				wokeAny = true;
			}
			break;
		}
		return wokeAny;
	}

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile
	{
		bool result = false;
		unsigned int spinsLeft = 0;
		if ((spinCount > 0) && (get_num_processors() != 1))
			spinsLeft = spinCount;
		rcptr<os::semaphore> osSemaphore;
		for (;;)
		{
			osSemaphore.release();
			read_token rt;
			m_contents.begin_read(rt);
			if (rt->m_state == set_state)
			{
				result = true;
				break;
			}

			if (rt->m_state == unset_state)	// else trigger_state
			{
				if (!timeout)
				{
					result = false;
					break;
				}

				if (!!spinsLeft)
				{
					--spinsLeft;
					if (os::thread::spin_once())
						continue;
				}
			}

			if ((!timeout) && (rt->m_state != trigger_state))
			{
				result = false;
				break;
			}

			write_token wt;
			if (!m_contents.promote_read_token(rt, wt))
				continue;

			if (wt->m_state == trigger_state)
				wt->m_state = unset_state;
			else
			{
				if (!wt->m_stallCount)
					wt->m_osSemaphore = get_os_semaphore();
				osSemaphore = wt->m_osSemaphore;
				++(wt->m_stallCount);
			}
			if (!m_contents.end_write(wt))
				continue;
			break;
		}
		if (!!osSemaphore)
		{
			result = osSemaphore->acquire(timeout);
			if (!result)	// not yet conclusive
			{
				// Need to remove self from stallCount.
				for (;;)
				{
					read_token rt2;
					m_contents.begin_read(rt2);
					if (rt2->m_osSemaphore != osSemaphore)	// If our sync token is not present, we don't need to dec, we're definitely signalled.
					{
						rt2.release();
						osSemaphore->acquire();
						result = true;
						break;
					}
					write_token wt2;
					if (!m_contents.promote_read_token(rt2, wt2))
						continue;
					if (!--(wt2->m_stallCount))
						wt2->m_osSemaphore.release();
					if (!m_contents.end_write(wt2))
						continue;
					break;
				}
			}
		}
		return result ? 1 : 0;
	}
};


}


#endif
