//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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

	refireable_timer(const refireable_timer&) = delete;
	refireable_timer(refireable_timer&&) = delete;
	refireable_timer& operator=(const refireable_timer&) = delete;
	refireable_timer& operator=(refireable_timer&&) = delete;

	virtual void triggered()
	{
		// const at construction, so no need to reference as volatile
		if (const_cast<refireable_timer*>(this)->m_wakeAll)
			m_condition.pulse_all();
		else
			m_condition.pulse_one();
	}

public:
	refireable_timer()
		: timer(timeout_t::infinite()),
		m_wakeAll(true)
	{ }

	explicit refireable_timer(const timeout_t& t, bool wakeAll = true)
		: timer(t),
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
