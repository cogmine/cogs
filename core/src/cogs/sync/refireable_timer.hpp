//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_REFIRABLE_TIMER
#define COGS_REFIRABLE_TIMER


#include "cogs/sync/timer.hpp"


namespace cogs {

/// @ingroup Timers
/// @brief A refirable timer
class refireable_timer : public timer
{
private:
	const bool m_wakeAll;

	refireable_timer();
	refireable_timer(const refireable_timer&);
	refireable_timer& operator=(const refireable_timer&);

	refireable_timer(const timeout_t& t, bool wakeAll)
		: timer(t),
			m_wakeAll(wakeAll)
	{ }

	virtual void triggered()
	{
		// const at construction, so no need to reference as volatile
		if (const_cast<refireable_timer*>(this)->m_wakeAll)
			m_event.pulse_all();
		else
			m_event.pulse_one();
	}

public:
	static rcref<refireable_timer> create(const timeout_t::period_t& p, bool wakeAll = true)
	{
		timeout_t t(p);
		rcref<refireable_timer> tmr = rcnew(bypass_constructor_permission<refireable_timer>, t, wakeAll);
		if (!t.is_infinite())
			tmr->defer();
		return tmr;
	}

	// Caller error to call when in unfired state	
	// This refire includes the overhead of actual operation it triggered.
	bool refire()									{ return timer::refire(); }
	bool refire(const timeout_t& t)					{ return timer::refire(t); }

	bool reschedule(const timeout_t::period_t& p)	{ return timer::reschedule(p); }
};


}


#endif
