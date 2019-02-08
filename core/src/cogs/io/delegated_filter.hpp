//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_IO_DELEGATED_FILTER
#define COGS_IO_DELEGATED_FILTER


#include "cogs/function.hpp"
#include "cogs/io/filter.hpp"


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A derived I/O filter that accepts a delegate to perform the filtering.
class delegated_filter : public filter
{
public:
	typedef function<composite_buffer(composite_buffer&)> filter_delegate_t;

private:
	filter_delegate_t	m_delegate;

protected:
	virtual composite_buffer filtering(composite_buffer& src)
	{
		composite_buffer result;
		filter_delegate_t d = m_delegate;
		if (!!d)
			result = d(src);
		return result;
	}

public:
	explicit delegated_filter(const filter_delegate_t& d)
		:	m_delegate(d)
	{ }
};



}
}


#endif
