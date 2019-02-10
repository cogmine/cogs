//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress
// Note: string formats need work.  Dependent on composite_string type.

#ifndef COGS_DATETIME
#define COGS_DATETIME


#include "cogs/env.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/os/datetime.hpp"


namespace cogs {


/// @ingroup Math
/// @brief A date and time.  Resolution is in seconds.
class datetime
{
private:
	os::datetime m_dt;

	datetime(const os::datetime& dt)
		:	m_dt(dt)
	{ }

public:
	typedef os::datetime::period_t period_t;

	datetime()											{ }
	datetime(const datetime& src)	:	m_dt(src.m_dt)	{ }

	datetime& operator=(const datetime& src)			{ m_dt = src.m_dt; return *this; }

	static datetime	now()								{ return datetime(os::datetime::now()); }

	period_t operator-(const datetime& dt)				{ return m_dt - dt.m_dt; }

	void get_detail(datetime_detail& d) const			{ m_dt.get_detail(d); }

	enum string_format
	{
		RFC1123DateTime = 1,				// Thu, 30 Apr 1998 15:59:59 GMT
		RFC850DateTime = 2,					// Thursday, 30-Apr-98 15:59:59 GMT
		ANSICasctime = 3,					// Thu Apr 30 15:59:59 1998		OR		Thu Apr  9 15:59:59 1998

		ISO8601Date = 101,					// 1998-04-30
		ISO8601Month = 102,					// 1998-04
		ISO8601Year = 103,					// 1998
		ISO8601WeekYear = 104,				// 1998-W19
		ISO8601WeekYearDay = 105,			// 1998-W19-5
		ISO8601YearDay = 106,				// 1998-120
		ISO8601DateCompact = 107,			// 19980430
		ISO8601MonthCompact = 108,			// 199804
		ISO8601WeekYearCompact = 109,		// 1998W19
		ISO8601WeekYearDayCompact = 110,	// 1998W195
		ISO8601YearDayCompact = 111,		// 1998120
		ISO8601Time = 112,					// 23:59:59
		ISO8601TimeMilliseconds = 113,		// 23:59:59.9942
		ISO8601TimeMinutes = 114,			// 23:59
		ISO8601TimeCompact = 115,			// 235959
		ISO8601TimeMinutesCompact = 116,	// 2359
		ISO8601Hour = 117,					// 23
		ISO8601DateTime = 118,				// 1998-04-30 23:59:59
		ISO8601DateTimeCompact = 119		// 19980430T235959
	};

	template <typename char_t>
	composite_string_t<char_t> to_string_t(string_format strFormat = RFC1123DateTime) const
	{
		static constexpr char_t space = (char_t)' ';
		static constexpr char_t colon = (char_t)':';
		static constexpr char_t dash = (char_t)'-';
		static constexpr char_t tee = (char_t)'T';

		static constexpr char_t gmt[] = { (char_t)' ', (char_t)'G', (char_t)'M', (char_t)'T' };

		static constexpr char_t Jan[] = { (char_t)'J', (char_t)'a', (char_t)'n', (char_t)0 };
		static constexpr char_t Feb[] = { (char_t)'F', (char_t)'e', (char_t)'b', (char_t)0 };
		static constexpr char_t Mar[] = { (char_t)'M', (char_t)'a', (char_t)'r', (char_t)0 };
		static constexpr char_t Apr[] = { (char_t)'A', (char_t)'p', (char_t)'r', (char_t)0 };
		static constexpr char_t May[] = { (char_t)'M', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Jun[] = { (char_t)'J', (char_t)'u', (char_t)'n', (char_t)0 };
		static constexpr char_t Jul[] = { (char_t)'J', (char_t)'u', (char_t)'l', (char_t)0 };
		static constexpr char_t Aug[] = { (char_t)'A', (char_t)'u', (char_t)'g', (char_t)0 };
		static constexpr char_t Sep[] = { (char_t)'S', (char_t)'e', (char_t)'p', (char_t)0 };
		static constexpr char_t Oct[] = { (char_t)'O', (char_t)'c', (char_t)'t', (char_t)0 };
		static constexpr char_t Nov[] = { (char_t)'N', (char_t)'o', (char_t)'v', (char_t)0 };
		static constexpr char_t Dec[] = { (char_t)'D', (char_t)'e', (char_t)'c', (char_t)0 };

		static constexpr char_t Sunday[] = { (char_t)'S', (char_t)'u', (char_t)'n', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Monday[] = { (char_t)'M', (char_t)'o', (char_t)'n', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Tuesday[] = { (char_t)'T', (char_t)'u', (char_t)'e', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Wednesday[] = { (char_t)'W', (char_t)'e', (char_t)'d', (char_t)'n', (char_t)'e', (char_t)'s', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Thursday[] = { (char_t)'T', (char_t)'h', (char_t)'u', (char_t)'r', (char_t)'s', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Friday[] = { (char_t)'F', (char_t)'r', (char_t)'i', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };
		static constexpr char_t Saturday[] = { (char_t)'S', (char_t)'a', (char_t)'t', (char_t)'u', (char_t)'r', (char_t)'d', (char_t)'a', (char_t)'y', (char_t)0 };

		static constexpr char_t Sun[] = { (char_t)'S', (char_t)'u', (char_t)'n', (char_t)0 };
		static constexpr char_t Mon[] = { (char_t)'M', (char_t)'o', (char_t)'n', (char_t)0 };
		static constexpr char_t Tue[] = { (char_t)'T', (char_t)'u', (char_t)'e', (char_t)0 };
		static constexpr char_t Wed[] = { (char_t)'W', (char_t)'e', (char_t)'d', (char_t)0 };
		static constexpr char_t Thu[] = { (char_t)'T', (char_t)'h', (char_t)'u', (char_t)0 };
		static constexpr char_t Fri[] = { (char_t)'F', (char_t)'r', (char_t)'i', (char_t)0 };
		static constexpr char_t Sat[] = { (char_t)'S', (char_t)'a', (char_t)'t', (char_t)0 };

		static constexpr char_t commaSpace[] = { (char_t)',', (char_t)' ', (char_t)0 };

		static constexpr const char_t* Month3[12] = { Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec };
		static constexpr const char_t* WeekDay[7] = { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday };
		static constexpr const char_t* WeekDay3[7] = { Sun, Mon, Tue, Wed, Thu, Fri, Sat };

		datetime_detail d;
		get_detail(d);

		composite_string_t<char_t> result;
		switch (strFormat)
		{
		case RFC1123DateTime:	// Thu, 30 Apr 1998 15:59:59 GMT
		{
			result = string_t<char_t>::literal(WeekDay3[d.m_dayOfWeek.get_int()]);
			result += string_t<char_t>::literal(commaSpace);

			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			string_t<char_t> spaceStr = string_t<char_t>::contain(&space, 1);
			result += spaceStr;

			result += string_t<char_t>::literal(Month3[d.m_month.get_int() - 1]);
			result += spaceStr;

			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result += yearString;
			result += spaceStr;

			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result += hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			result += colonStr;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			result += string_t<char_t>::contain(gmt, 4);
			break;
		}
		case RFC850DateTime:	// Thursday, 30-Apr-98 15:59:59 GMT
		{
			result = string_t<char_t>::literal(WeekDay[d.m_dayOfWeek.get_int()]);
			result += string_t<char_t>::literal(commaSpace);

			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			string_t<char_t> dashStr = string_t<char_t>::contain(&dash, 1);
			result += dashStr;

			result += string_t<char_t>::literal(Month3[d.m_month.get_int() - 1]);
			result += dashStr;

			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(2);
			result += yearString;
			string_t<char_t> spaceStr = string_t<char_t>::contain(&space, 1);
			result += spaceStr;

			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result += hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			result += colonStr;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			result += string_t<char_t>::contain(gmt, 4);
			break;
		}
		case ANSICasctime:	// Thu Apr 30 15:59:59 1998		OR		Thu Apr  9 15:59:59 1998
		{
			result = string_t<char_t>::literal(WeekDay3[d.m_dayOfWeek.get_int()]);
			string_t<char_t> spaceStr = string_t<char_t>::contain(&space, 1);
			result += spaceStr;

			result += string_t<char_t>::literal(Month3[d.m_month.get_int() - 1]);
			result += spaceStr;

			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			result += spaceStr;

			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result += hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			result += colonStr;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;

			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result += yearString;
			break;
		}

		case ISO8601Date:	// 1998-04-30
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> dashStr = string_t<char_t>::contain(&dash, 1);
			result += dashStr;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			result += dashStr;
			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			break;
		}
		case ISO8601Month:					// 1998-04
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> dashStr = string_t<char_t>::contain(&dash, 1);
			result += dashStr;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			break;
		}
		case ISO8601Year:					// 1998
		{
			result = d.m_year.to_string_t<char_t>(10, 4);
			result.truncate_to_right(4);
			break;
		}
		//case ISO8601WeekYear:				// 1998-W19
		//{
		//	break;
		//}
		//case 	ISO8601WeekYearDay:			// 1998-W19-5
		//{
		//	break;
		//}
		//case ISO8601YearDay:				// 1998-120
		//{
		//	break;
		//}
		case 		ISO8601DateCompact:			// 19980430
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			break;
		}
		case 	ISO8601MonthCompact:			// 199804
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			break;
		}
		//case 	ISO8601WeekYearCompact:		// 1998W19
		//{
		//	break;
		//}
		//case 	ISO8601WeekYearDayCompact:	// 1998W195
		//{
		//	break;
		//}
		//case 	ISO8601YearDayCompact:		// 1998120
		//{
		//	break;
		//}
		case 	ISO8601Time:					// 23:59:59
		{
			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			result += colonStr;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			break;
		}
		//case 	ISO8601TimeMilliseconds:		// 23:59:59.9942
		//{
		//	break;
		//}
		case 	ISO8601TimeMinutes:			// 23:59
		{
			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			break;
		}
		case 	ISO8601TimeCompact:			// 235959
		{
			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			break;
		}
		case 	ISO8601TimeMinutesCompact:	// 2359
		{
			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			break;
		}
		case 	ISO8601Hour:					// 23
		{
			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;
			break;
		}
		case 	ISO8601DateTime:				// 1998-04-30T23:59:59
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> dashStr = string_t<char_t>::contain(&dash, 1);
			result += dashStr;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			result += dashStr;
			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			result += string_t<char_t>::contain(&tee, 1);

			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;
			string_t<char_t> colonStr = string_t<char_t>::contain(&colon, 1);
			result += colonStr;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;
			result += colonStr;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			break;
		}
		case 		ISO8601DateTimeCompact:		// 19980430T235959
		{
			string_t<char_t> yearString = d.m_year.to_string_t<char_t>(10, 4);
			yearString.truncate_to_right(4);
			result = yearString;
			string_t<char_t> monthString = d.m_month.to_string_t<char_t>(10, 2);
			result += monthString;
			string_t<char_t> dayString = d.m_day.to_string_t<char_t>(10, 2);
			result += dayString;
			result += string_t<char_t>::contain(&tee, 1);;

			string_t<char_t> hourString = d.m_hour.to_string_t<char_t>(10, 2);
			result = hourString;

			string_t<char_t> minuteString = d.m_minute.to_string_t<char_t>(10, 2);
			result += minuteString;

			string_t<char_t> secondString = d.m_second.to_string_t<char_t>(10, 2);
			result += secondString;
			break;
		}
		default:
			COGS_ASSERT(false);	// TBD
		}

		return result;
	}

	composite_string to_string(string_format strFormat = RFC1123DateTime) const
	{
		return to_string_t<wchar_t>(strFormat);
	}

	composite_cstring to_cstring(string_format strFormat = RFC1123DateTime) const
	{
		return to_string_t<char>(strFormat);
	}
};


}


#endif
