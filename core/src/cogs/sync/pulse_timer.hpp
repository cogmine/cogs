//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_PULSE_TIMER
#define COGS_PULSE_TIMER


#include "cogs/sync/timer.hpp"


namespace cogs {



/// @ingroup Timers
/// @brief A pulse timer
///
/// A pulse_timer will automatically refire.
/// Refire is independent to the overhead of the waiting operations (potentially bad).
class pulse_timer : public timer
{
private:
	const bool m_wakeAll;

	pulse_timer();
	pulse_timer(const pulse_timer&);
	pulse_timer& operator=(const pulse_timer&);

	pulse_timer(const timeout_t& t, bool wakeAll)
		: timer(t),
			m_wakeAll(wakeAll)
	{ }

	virtual void triggered()
	{
		// const at construction, so no need to reference as volatile
		if (const_cast<pulse_timer*>(this)->m_wakeAll)
			m_event.pulse_all();
		else
			m_event.pulse_one();
		refire();
	}

public:
	static rcref<pulse_timer> create(const timeout_t::period_t& p, bool wakeAll = true)	// wakeAll false means only wake 1 waiter per timer expiration
	{
		timeout_t t(p);
		rcref<pulse_timer> tmr = rcnew(bypass_constructor_permission<pulse_timer>, t, wakeAll);
		if (!t.is_infinite())
			tmr->defer();
		return tmr;
	}

	bool reschedule(const timeout_t::period_t& p)			{ return timer::reschedule(p); }
};


}


#endif
