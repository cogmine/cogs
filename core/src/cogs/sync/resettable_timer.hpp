//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_SYNC_MANUAL_RESET_TIMER
#define COGS_HEADER_SYNC_MANUAL_RESET_TIMER


#include "cogs/sync/timer.hpp"


namespace cogs {



/// @ingroup Timers
/// @brief A manually reset timer
///
/// A resettable_timer goes off and remains signalled, but can be manually reset.
/// A resettable_timer can be refired.  If it hasn't gone off yet it can be rescheduled.
/// If aborted, reschedule() will restart it.
class resettable_timer : public timer
{
private:
	resettable_timer();
	resettable_timer(const resettable_timer&);
	resettable_timer& operator=(const resettable_timer&);

	virtual void triggered()
	{
		m_event.signal();
	}

protected:
	resettable_timer(const timeout_t& t)
		: timer(t)
	{ }

public:
	static rcref<resettable_timer> create(const timeout_t& t)
	{
		rcref<resettable_timer> tmr = rcnew(bypass_constructor_permission<resettable_timer>, t);
		if (!t.is_infinite())
			tmr->defer();
		return tmr;
	}

	// reschedule() is used to alter the expiration time of a pending timer.
	// If the timer has already gone off, reschedule() will return false.
	bool reschedule(const timeout_t& t)			{ return timer::reschedule(t); }

	// refire() is used when the timer has already gone off.
	// It should not be called if the timer is pending.
	// Therefore, the caller must ensure there is no thread contention over refire().
	bool refire()								{ return timer::refire(); }
	bool refire(const timeout_t& t)				{ return timer::refire(t); }
};


}


#endif
