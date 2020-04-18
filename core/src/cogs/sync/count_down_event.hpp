//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_COUNT_DOWN_EVENT
#define COGS_HEADER_SYNC_COUNT_DOWN_EVENT


#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/single_fire_event.hpp"


namespace cogs {


/// @ingroup Events
/// @brief A count-down event
class count_down_event : public event_base, public object
{
private:
	static constexpr size_t doneValue = (size_t)-1;

	volatile single_fire_event m_event;
	volatile size_type m_count;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		dispatcher::dispatch_inner(m_event, t, priority);
	}

public:
	explicit count_down_event(size_t n)
		: m_count(n)
	{
		COGS_ASSERT(n != doneValue); // max value is not supported (used internally to indicate fired, to allow init from 0)
	}

	count_down_event(size_t n, const function<void()>& d)
		: m_count(n)
	{
		COGS_ASSERT(n != doneValue); // max value is not supported (used internally to indicate fired, to allow init from 0)
		m_event.dispatch(d);
	}

	bool count_up(size_t n = 1) volatile // Return false if already released/set, and did not count up.
	{
		size_type oldValue = m_count;
		size_type newValue;
		do {
			if (oldValue == doneValue)
				return false;
			newValue = oldValue;
			newValue += n;
		} while (!m_count.compare_exchange(newValue, oldValue, oldValue));
		return true;
	}

	bool count_down(size_t n = 1) volatile // returns true if this was what released/set it
	{
		size_type oldValue = m_count;
		size_type newValue;
		do {
			if (oldValue == doneValue)
				return false;
			if (oldValue <= n)
				newValue = doneValue;
			else
			{
				newValue = oldValue;
				newValue -= n;
			}
		} while (!m_count.compare_exchange(newValue, oldValue, oldValue));
		if (newValue == doneValue)
		{
			m_event.signal();
			return true;
		}
		return false;
	}

	rcref<task<void> > count(const rcref<waitable>& waitFor)
	{
		if (!count_up())
			return signaled();

		return waitFor->dispatch([e{ this_rcptr }]()
		{
			--*e;
		});
	}

	rcref<task<void> > operator+=(const rcref<waitable>& waitFor)
	{
		return count(waitFor);
	}

	bool operator--() volatile { return !count_down(); }
	bool operator++() volatile { return count_up(); }

	bool operator--(int) volatile { return !count_down(); }
	bool operator++(int) volatile { return count_up(); }

	class reference
	{
	private:
		rcptr<count_down_event> m_countDownEvent;

	public:
		explicit reference(const rcref<count_down_event>& e)
			: m_countDownEvent(++*e ? rcptr<count_down_event>(e) : rcptr<count_down_event>())
		{ }

		bool operator!() const { return !!m_countDownEvent; }

		~reference()
		{
			if (!!m_countDownEvent)
				--*m_countDownEvent;
		}
	};

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }

	virtual bool signal() volatile { return count_down(); }
};


}


#endif
