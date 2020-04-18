//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_DATETIME
#define COGS_HEADER_OS_DATETIME

#include <time.h>

#include "cogs/env.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/math/time.hpp"


namespace cogs {


// os::datetime implements os level date/time functionality.
// os::datetime is used by cogs::datetime.

// os::datetime is used for calendar dates and times.
// For high resolution timing, use timeout_t


struct datetime_detail
{
	fixed_integer<false, 16> year;
	fixed_integer<false, 4> month;        // January == 1
	fixed_integer<false, 5> day;          // 1-31
	fixed_integer<false, 3> dayOfWeek;    // Sunday == 0
	fixed_integer<false, 5> hour;         // 0-23
	fixed_integer<false, 6> minute;       // 0-59
	fixed_integer<false, 6> second;       // 0-59
	fixed_integer<false, 10> millisecond; // 0-999
};


namespace os {

class datetime
{
private:
	time_t m_stamp;

	datetime(const time_t& t)
		: m_stamp(t)
	{
	}

public:
	datetime()
	{
	}

	datetime(const datetime& src)
		: m_stamp(src.m_stamp)
	{
	}

	datetime& operator=(const datetime& src)
	{
		m_stamp = src.m_stamp;
		return *this;
	}

	static datetime now()
	{
		time_t t;
		::time(&t);
		return datetime(t);
	}

	void get_detail(datetime_detail& d) const
	{
		tm t;

		bool b = gmtime_r(&m_stamp, &t) != NULL;
		COGS_ASSERT(b);

		d.year = 1900 + t.tm_year;
		d.month = (uint8_t)t.tm_mon + 1;
		d.day = (uint8_t)t.tm_mday;
		d.dayOfWeek = (uint8_t)t.tm_wday;
		d.hour = (uint8_t)t.tm_hour;
		d.minute = (uint8_t)t.tm_min;
		d.second = (uint8_t)t.tm_sec;
		d.millisecond = 0;
	}

	typedef measure<fixed_integer<true, (sizeof(time_t)*8*2) >, seconds> period_t;

	period_t operator-(const datetime& dt)
	{
		fixed_integer<true, (sizeof(time_t)*8)> stamp1 = m_stamp;
		fixed_integer<true, (sizeof(time_t)*8)> stamp2 = dt.m_stamp;
		return make_measure<seconds>(stamp1 - stamp2);
	}
};


}
}


#endif
