//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_LIMITER
#define COGS_HEADER_IO_LIMITER


#include "cogs/io/buffer.hpp"
#include "cogs/io/filter.hpp"


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A derived I/O filter that limits reading to a specifed number of bytes.
///
/// When the data limit is exceeded, the limiter becomes unreadable, 
/// effectively closing the connection from the perspective of a reader/datasink
/// coupled to it.  A coupled datasource/writer will have any in-transit data
/// returned to its overflow buffer.
class limiter : public filter
{
private:
	size_t m_remaining;

public:
	limiter(rc_obj_base& desc, size_t n)
		: filter(desc),
		m_remaining(n)
	{ }

	virtual rcref<task<composite_buffer> > filtering(composite_buffer& src)
	{
		composite_buffer result;
		size_t n = m_remaining;
		if (!!n)
		{
			if (n > src.get_length())
				n = src.get_length();

			result = src.split_off_before(n);
			m_remaining -= n;
		}
		return get_immediate_task(result);
	}
};


}
}


#endif
