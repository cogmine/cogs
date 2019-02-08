//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_COUNT_DOWN_EVENT
#define COGS_COUNT_DOWN_EVENT


#include "cogs/sync/dispatcher.hpp"


namespace cogs {


/// @ingroup Events
/// @brief A count-down event
class count_down_event : public event_base
{
private:
	volatile size_type m_count;
	volatile single_fire_event m_event;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		dispatcher::dispatch_inner(m_event, t, priority);
	}

public:
	explicit count_down_event(const function<void()>& d, size_t n = 1)
		:	m_count(n)
	{
		if (!n)
			m_event.signal();
		m_event.dispatch(d);
	}

	explicit count_down_event(size_t n = 1)
		:	m_count(n)
	{
		if (!n)
			m_event.signal();
	}

	bool count_up(size_t n = 1) volatile	// Return false if already released/set, and did not count up.
	{
		size_type oldValue = m_count;
		size_type newValue;
		do {
			COGS_ASSERT(!!oldValue);
			if (!oldValue)
				return false;
			newValue = oldValue;
			newValue += n;
		} while (!m_count.compare_exchange(newValue, oldValue, oldValue));
		return true;
	}

	bool count_down(size_t n = 1) volatile	// returns true if this was what released/set it
	{
		size_type oldValue = m_count;
		size_type newValue;
		do {
			COGS_ASSERT(!!oldValue);
			if (!oldValue)
				return false;
			COGS_ASSERT(oldValue >= n);
			newValue = 0;
			if (oldValue > n)
			{
				newValue = oldValue;
				newValue -= n;
			}
		} while (!m_count.compare_exchange(newValue, oldValue, oldValue));
		if (!newValue)
		{
			m_event.signal();
			return true;
		}
		return false;
	}

	bool operator--() volatile		{ return !count_down(); }
	bool operator++() volatile		{ return count_up(); }

	class reference
	{
	private:
		count_down_event&	m_countDownEvent;
		bool				m_acquired;
		function<void()>	m_delegate;

	public:
		explicit reference(count_down_event& e)
			:	m_countDownEvent(e),
				m_acquired(++e)
		{ }

		explicit reference(const function<void()>& d, count_down_event& e)
			:	m_countDownEvent(e),
				m_acquired(++e),
				m_delegate(d)
		{ }

		bool operator!() const	{ return m_acquired; }

		bool release()
		{
			if (!m_acquired)
				return false;

			m_acquired = false;
			bool released = !--m_countDownEvent;
			if (released)
				m_delegate();
			return released;
		}

		~reference()
		{
			if (m_acquired)
			{
				if (!--m_countDownEvent)
					m_delegate();
			}
		}
	};

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile	{ return m_event.timed_wait(timeout, spinCount); }

	virtual bool signal() volatile	{ return count_down(); }
};



}


#endif
