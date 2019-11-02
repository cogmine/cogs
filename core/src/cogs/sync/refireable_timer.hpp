//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_REFIRABLE_TIMER
#define COGS_HEADER_SYNC_REFIRABLE_TIMER


#include "cogs/sync/timer.hpp"


namespace cogs {

/// @ingroup Timers
/// @brief A refirable timer
class refireable_timer : public timer
{
private:
	const bool m_wakeAll;

	refireable_timer() = delete;
	refireable_timer(const refireable_timer&) = delete;
	refireable_timer& operator=(const refireable_timer&) = delete;

	virtual void triggered()
	{
		// const at construction, so no need to reference as volatile
		if (const_cast<refireable_timer*>(this)->m_wakeAll)
			m_event.pulse_all();
		else
			m_event.pulse_one();
	}

public:
	explicit refireable_timer(rc_obj_base& desc)
		: timer(desc, timeout_t::infinite()),
		m_wakeAll(true)
	{
	}

	refireable_timer(rc_obj_base& desc, const timeout_t& t, bool wakeAll = true)
		: timer(desc, t),
		m_wakeAll(wakeAll)
	{
		if (!t.is_infinite())
			defer();
	}

	// Caller error to call when in unfired state
	// This refire includes the overhead of actual operation it triggered.
	bool refire() { return timer::refire(); }
	bool refire(const timeout_t& t) { return timer::refire(t); }

	bool reschedule(const timeout_t& t) { return timer::reschedule(t); }
};


}


#endif
