//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_DATETIME
#define COGS_HEADER_OS_DATETIME


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
	fixed_integer<false, 16> m_year;
	fixed_integer<false, 4> m_month; // January == 1
	fixed_integer<false, 5> m_day; // 1-31
	fixed_integer<false, 3> m_dayOfWeek; // Sunday == 0
	fixed_integer<false, 5> m_hour; // 0-23
	fixed_integer<false, 6> m_minute; // 0-59
	fixed_integer<false, 6> m_second; // 0-59
	fixed_integer<false, 10> m_millisecond; // 0-999
};


namespace os {

class datetime
{
private:
	FILETIME m_stamp;

	datetime(const FILETIME& ft)
		: m_stamp(ft)
	{ }

public:
	datetime()
	{ }

	datetime(const datetime& src)
		: m_stamp(src.m_stamp)
	{ }

	datetime& operator=(const datetime& src)
	{
		m_stamp = src.m_stamp;
		return *this;
	}

	static datetime now()
	{
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		return datetime(ft);
	}

	void get_detail(datetime_detail& d) const
	{
		SYSTEMTIME st;

		BOOL b = FileTimeToSystemTime(&m_stamp, &st); // Years prior to 1601 will likely fail!
		COGS_ASSERT(b);

		d.m_year = st.wYear;
		d.m_month = (uint8_t)st.wMonth;
		d.m_day = (uint8_t)st.wDay;
		d.m_dayOfWeek = (uint8_t)st.wDayOfWeek;
		d.m_hour = (uint8_t)st.wHour;
		d.m_minute = (uint8_t)st.wMinute;
		d.m_second = (uint8_t)st.wSecond;
		d.m_millisecond = (uint16_t)st.wMilliseconds;
	}

	typedef measure<fixed_integer<true, (sizeof(DWORD)*8*2) >, nano100s> period_t;

	period_t operator-(const datetime& dt)
	{
		uint64_t p1 = m_stamp.dwHighDateTime;
		p1 <<= sizeof(DWORD);
		p1 += m_stamp.dwLowDateTime;

		uint64_t p2;
		p2 = dt.m_stamp.dwHighDateTime;
		p2 <<= sizeof(DWORD);
		p2 += dt.m_stamp.dwLowDateTime;

		p1 -= p2;
		return make_measure<nano100s>(p1);
	}
};


}
}


#endif
