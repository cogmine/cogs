//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_DATETIME
#define COGS_OS_DATETIME

#include <time.h>

#include "cogs/env.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/math/time.hpp"


namespace cogs {


// os::datetime implements os level date/time functionality.
// os::datetime is used by cogs::datetime.

// os::datetime is used for calendar dates and times.
// For high resolution timing, use timeout_t


struct datetime_detail
{
	fixed_integer<false, 16>	m_year;
	fixed_integer<false,  4>	m_month;		// January == 1
	fixed_integer<false,  5>	m_day;			// 1-31
	fixed_integer<false,  3>	m_dayOfWeek;	// Sunday == 0
	fixed_integer<false,  5>	m_hour;			// 0-23
	fixed_integer<false,  6>	m_minute;		// 0-59
	fixed_integer<false,  6>	m_second;		// 0-59
	fixed_integer<false, 10>	m_millisecond;	// 0-999
};


namespace os {

class datetime
{
private:
	time_t m_stamp;

	datetime(const time_t& t)
		:	m_stamp(t)
	{
	}

public:
	datetime()
	{
	}

	datetime(const datetime& src)
		:	m_stamp(src.m_stamp)
	{
	}

	datetime& operator=(const datetime& src)
	{
		m_stamp = src.m_stamp;
		return *this;
	}

	static datetime	now()
	{
		time_t t;
		time(&t);
		return datetime(t);
	}

	void get_detail(datetime_detail& d) const
	{
		tm t;

		bool b = gmtime_r(&m_stamp, &t) != NULL;
		COGS_ASSERT(b);

		d.m_year = 1900 + t.tm_year;
		d.m_month = (uint8_t)t.tm_mon + 1;
		d.m_day = (uint8_t)t.tm_mday;
		d.m_dayOfWeek = (uint8_t)t.tm_wday;
		d.m_hour = (uint8_t)t.tm_hour;
		d.m_minute = (uint8_t)t.tm_min;
		d.m_second = (uint8_t)t.tm_sec;
		d.m_millisecond = 0;
	}

	typedef measure<fixed_integer<true, (sizeof(time_t)*8*2) >, seconds> period_t;

	period_t operator-(const datetime& dt)
	{
		measure<fixed_integer<true, (sizeof(time_t)*8) >, seconds> stamp1 = m_stamp;
		measure<fixed_integer<true, (sizeof(time_t)*8) >, seconds> stamp2 = dt.m_stamp;
		return stamp1 - stamp2;
	}
};


}
}


#endif
