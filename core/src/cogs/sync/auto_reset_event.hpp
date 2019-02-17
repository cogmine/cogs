//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_AUTO_RESET_EVENT
#define COGS_HEADER_SYNC_AUTO_RESET_EVENT


#include "cogs/sync/resettable_event.hpp"


namespace cogs {


/// @ingroup Events
/// @brief An auto-reset event
class auto_reset_event : public event_base
{
private:
	resettable_event m_resettableEvent;

	auto_reset_event(const auto_reset_event&) = delete;
	auto_reset_event& operator=(const auto_reset_event&) = delete;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		dispatcher::dispatch_inner(m_resettableEvent, t, priority);
	}

public:
	auto_reset_event()
	{ }

	// returns true if event delivered before return.
	virtual bool signal() volatile { return m_resettableEvent.wake_next(); }

	// returns true if the event was triggered
	bool reset() volatile		{ return m_resettableEvent.reset(); }

	// returns true if any waiters woken/dispatched 
	bool pulse_all() volatile	{ return m_resettableEvent.pulse_all(); }
	bool pulse_one() volatile	{ return m_resettableEvent.pulse_one(); }

	virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile	{ return m_resettableEvent.timed_wait(timeout, spinCount); }
};




}


#endif
