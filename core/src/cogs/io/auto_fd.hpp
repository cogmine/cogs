//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_AUTOFD
#define COGS_AUTOFD


#include "cogs/os.hpp"


namespace cogs {


class auto_fd
{
public:
	int m_fd;

	auto_fd()
		: m_fd(-1)
	{ }

	auto_fd(int fd)
		: m_fd(fd)
	{ }

	bool operator!() const		{ return m_fd == -1; }

	void close()
	{
		int fd = m_fd;
		m_fd = -1;
		::_close(fd);
	}

	~auto_fd()
	{
		if (m_fd != -1)
			::_close(m_fd);
	}
};


}



#endif


