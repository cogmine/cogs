//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_SINGLE_FIRE_TIMER
#define COGS_HEADER_SYNC_SINGLE_FIRE_TIMER


#include "cogs/sync/timer.hpp"


namespace cogs {


/// @ingroup Timers
/// @brief A single-fire timer
///
/// A single_fire_timer goes off once, and cannot be reset.
/// A single_fire_timer cannot be refired, but if it hasn't gone off yet it can be rescheduled.
/// Once aborted, a single_fire_timer does not change state.
class single_fire_timer : public timer
{
private:
	single_fire_timer() = delete;
	single_fire_timer(const single_fire_timer&) = delete;
	single_fire_timer& operator=(const single_fire_timer&) = delete;

	virtual void triggered()
	{
		// const at construction, so no need to reference as volatile
		m_event.signal();
	}

protected:
	single_fire_timer(const ptr<rc_obj_base>& desc, const timeout_t& t)
		: timer(desc, t)
	{ }

public:
	static rcref<single_fire_timer> create(const timeout_t& t)
	{
		rcref<single_fire_timer> tmr = rcnew(bypass_constructor_permission<single_fire_timer>, t);
		if (!t.is_infinite())
			tmr->defer();
		return tmr;
	}

	bool reschedule(const timeout_t& t)			{ return timer::reschedule(t); }
};


}


#endif
