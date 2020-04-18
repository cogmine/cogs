//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_PULSE_TIMER
#define COGS_HEADER_SYNC_PULSE_TIMER


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

	pulse_timer(const pulse_timer&) = delete;
	pulse_timer(pulse_timer&&) = delete;
	pulse_timer& operator=(const pulse_timer&) = delete;
	pulse_timer& operator=(pulse_timer&&) = delete;

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

	// wakeAll false means only wake 1 waiter per timer expiration

	pulse_timer()
		: timer(timeout_t::infinite()),
		m_wakeAll(true)
	{
	}

	explicit pulse_timer(const timeout_t& t, bool wakeAll = true)
		: timer(t),
		m_wakeAll(wakeAll)
	{
		if (!t.is_infinite())
			defer();
	}

	bool reschedule(const timeout_t& t)
	{
		return timer::reschedule(t);
	}
};


}


#endif
