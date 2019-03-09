//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_SYNC_TIMEOUT
#define COGS_HEADER_OS_SYNC_TIMEOUT


#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "cogs/env.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/math/time.hpp"


namespace cogs {


// A timeout_t represents an absolute time.  Re-using a timeout_t to call
// subroutines accepting a timeout_t will not extend the duration of the operation, as
// it would if timeout_t represented a period.

// At the os level, a timeout_t is used with only os::semaphore and os:thread::join().

// construction of a timeout_t starts a timeout timer of the specificed duration.
// When that duration is complete, the timeout is expired.  If the timeout_t
// is 'refired', a new expiration is computed to coinside with the next interval
// of the duration time, starting at the original start time.  This is useful to
// support refiring timers.

// With pthreads, the wait routine (sem_timedwait) uses absolute time (as opposed to relative).
// All platforms must still keep track of start time, next expiration time, and period.
class timeout_t
{
public:
	typedef fixed_integer<false, (sizeof(time_t) * 8) + 30> period_unit_t;
	typedef nanoseconds period_unitbase;
	typedef measure<period_unit_t, period_unitbase> period_t;

	static period_t now()
	{
		struct timespec tm;
		int err = clock_gettime(CLOCK_REALTIME, &tm);
		COGS_ASSERT(err == 0);
		period_t ns(tm.tv_nsec);
		seconds_t secs(tm.tv_sec);
		ns += secs;
		return ns;
	}

private:
	// timespec is a time_t (integral) of seconds, and a long of nanoseconds in which only 30 bits are used.
	// We store that as a single large fixed integer of nanoseconds.
	typedef measure<fixed_integer<false, sizeof(time_t)*8>, seconds> seconds_t;

	period_t	m_startTime;
	period_t	m_period;
	period_t	m_expireTime;
	bool		m_infinite;

	bool expired_inner(const period_t& n, const period_t& expireTime) const
	{
		// Little trick to compensate for range overflow (however unlikely to occur)
		//
		// N=Now, S=Start, E=Expiration
		//
		//         S<E N<S E<=N	<- Only one of these conditions can be true if unexpired (N=Now, S=Start, E=Expiration)
		//
		// SNE   =  1 ^ 0 ^ 0  =  1	<- not expired yet	<- normal, no counter loop
		// ESN   =  0 ^ 0 ^ 1  =  1	<- not expired yet	<- counter will loop before timer expires
		// NES   =  0 ^ 1 ^ 0  =  1	<- not expired yet	<- counter will loop before timer expires
		//
		// SEN   =  1 ^ 0 ^ 1  =  0	<- expired			<- normal, no counter loop
		// NSE   =  1 ^ 1 ^ 0  =  0	<- expired			<- counter looped since timer expired
		// ENS   =  0 ^ 1 ^ 1  =  0	<- expired			<- counter looped before timer expired
		//
		// If equal :
		//
		// (ES)N =  0 ^ 0 ^ 1  = 1 <- S==E Implies that the period is 0.  This function should not be called if the period is 0.
		// N(ES) =  0 ^ 1 ^ 0  = 1 <- S==E Implies that the period is 0.  This function should not be called if the period is 0.
		//
		// (NS)E =  1 ^ 0 ^ 0  = 1 <- not expired yet	<- Assume N==S means it just started
		// E(NS) =  0 ^ 0 ^ 1  = 1 <- not expired yet	<- Assume N==S means it just started
		//
		// S(EN) =  1 ^ 0 ^ 1  = 0	<- expired			<- Assume E==N means it just expired
		// (EN)S =  0 ^ 1 ^ 1  = 0	<- expired			<- Assume E==N means it just expired
		//
		return (!((m_startTime < expireTime) ^ (n < m_startTime) ^ (expireTime <= n)));
	}

	timeout_t(period_t&& startTime, period_t&& period, period_t&& expireTime, bool inf)
		: m_startTime(std::move(startTime)),
		m_period(std::move(period)),
		m_expireTime(std::move(expireTime)),
		m_infinite(inf)
	{
	}

public:
	timeout_t()
		: m_startTime(0),
		m_period(0),
		m_expireTime(0),
		m_infinite(false)
	{ }

	timeout_t(const timeout_t& t)
		: m_startTime(t.m_startTime),
		m_period(t.m_period),
		m_expireTime(t.m_expireTime),
		m_infinite(t.m_infinite)
	{ }

	timeout_t(timeout_t&& t)
		: m_startTime(std::move(t.m_startTime)),
		m_period(std::move(t.m_period)),
		m_expireTime(std::move(t.m_expireTime)),
		m_infinite(t.m_infinite)
	{ }

	template <typename unit_t, typename unitbase_t>
	timeout_t(const measure<unit_t, unitbase_t>& n)
		: m_startTime(now()),
		m_period(n),
		m_infinite(false)
	{
		m_expireTime = m_startTime;
		m_expireTime += m_period;
	}

	template <typename unit_t, typename unitbase_t>
	timeout_t(measure<unit_t, unitbase_t>&& n)
		: m_startTime(now()),
		m_period(std::move(n)),
		m_infinite(false)
	{
		m_expireTime = m_startTime;
		m_expireTime += m_period;
	}

	timeout_t& operator=(const timeout_t&t)
	{
		m_startTime = t.m_startTime;
		m_period = t.m_period;
		m_expireTime = t.m_expireTime;
		m_infinite = t.m_infinite;
		return *this;
	}

	timeout_t& operator=(timeout_t&& t)
	{
		m_startTime = std::move(t.m_startTime);
		m_period = std::move(t.m_period);
		m_expireTime = std::move(t.m_expireTime);
		m_infinite = t.m_infinite;
		return *this;
	}

	template <typename unit_t, typename unitbase_t>
	timeout_t& operator=(const measure<unit_t, unitbase_t>& n)
	{
		m_startTime = now();
		m_period = n;
		m_expireTime = m_startTime;
		m_expireTime += m_period;
		m_infinite = false;
		return *this;
	}

	template <typename unit_t, typename unitbase_t>
	timeout_t& operator=(measure<unit_t, unitbase_t>&& n)
	{
		m_startTime = now();
		m_period = std::move(n);
		m_expireTime = m_startTime;
		m_expireTime += m_period;
		m_infinite = false;
		return *this;
	}

	static timeout_t infinite()		{ return timeout_t(period_t(0), period_t(0), period_t(0), true); }
	static timeout_t none()			{ return timeout_t(); }

	bool operator!() const			{ return expired(); }

	period_t get_period() const		{ return m_period; }
	period_t get_expiration() const	{ return m_expireTime; }

	bool is_infinite() const		{ return m_infinite; }

	period_t get_pending() const
	{
		if (!m_period || m_infinite)
			return period_t(0);

		period_t n = now();

		if (expired_inner(n, m_expireTime))
			return period_t(0);

		period_t result = m_expireTime;
		result -= n;
		return result;
	}

	bool expired() const
	{
		if (m_infinite)
			return false;
		//if (!m_period)
		//	return true;
		return expired_inner(now(), m_expireTime);
	}

	void refire()
	{
		if (!m_period || m_infinite)
			return;

		// This facilitates periodic-timers.
		// A periodic timer fires repeatedly, once every N units of time.  To compensate for
		// the overhead of the operation itself, and try to ensure that the operation is
		// completed as close as possible to once evert N units of time, we factor the time
		// since last fired into the new expiration time.  If more than that period has
		// transpired since the timer last expired, the new expiration is immediate.
		// (by changing the start time, so without modifying the period of the timeout object).
		
		period_t n = now();
		if (!expired_inner(n, m_expireTime))
			return;		// You can't refire() a timer until after it expires.
		// We bother with that check because we need to ensure m_startTime
		// is never in the future, or our range overflow trick fails.

		period_t newExpireTime = m_expireTime;
		newExpireTime += m_period;
		if (expired_inner(n, newExpireTime))
		{
			m_expireTime = n;
			m_startTime = n;
			m_startTime -= m_period;
		}
		else
		{
			m_expireTime = newExpireTime;
			m_startTime += m_period;
		}
	}

	void get_expiration(timespec& ts) const
	{
		period_t ns(m_expireTime);
		seconds_t s(m_expireTime);
		ns -= s;	// remove seconds from nanoseconds part

		ts.tv_sec = s.get().get_int();
		ts.tv_nsec = ns.get().get_int();
	}

	bool operator<(const timeout_t& cmp) const
	{
		if (m_infinite)
			return false;
		if (cmp.m_infinite)
			return true;
		return m_expireTime < cmp.m_expireTime;
	}

	bool operator>=(const timeout_t& cmp) const { return !operator<(cmp); }

	bool operator>(const timeout_t& cmp) const
	{
		if (m_infinite)
			return !cmp.m_infinite;
		if (cmp.m_infinite)
			return false;
		return m_expireTime > cmp.m_expireTime;
	}

	bool operator<=(const timeout_t& cmp) const { return !operator>(cmp); }

	template <typename unit_t, typename unitbase_t>
	timeout_t& operator+=(const measure<unit_t, unitbase_t>& n)	// extends the period
	{
		if (!m_infinite)
		{
			m_period += n;
			m_expireTime = m_startTime;
			m_expireTime += m_period;
		}
		return *this;
	}

	template <typename unit_t, typename unitbase_t>
	timeout_t operator+(const measure<unit_t, unitbase_t>& n)	// extends the period
	{
		timeout_t result = *this;
		result += n;
		return result;
	}

};


}


#endif
