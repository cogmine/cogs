//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_IO_NET_IP_SOCKET
#define COGS_HEADER_OS_IO_NET_IP_SOCKET


#include "cogs/os/io/epoll_pool.hpp"
#include "cogs/os/io/net/ip/endpoint.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class socket
{
protected:
	friend class tcp;

	rcref<os::io::epoll_pool>	m_epollPool;
	address_family		m_addressFamily;
	endpoint			m_localEndpoint;
	endpoint			m_remoteEndpoint;
	auto_fd				m_fd;

	// We dup() the socket so we can register reads and writes independently in epoll_pool.
	auto_fd				m_dupReadFd;

	socket(int type, int protocol, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		:	m_fd(::socket(addressFamily, type, protocol)),
			m_epollPool(epp),
			m_addressFamily(addressFamily)
	{
		if (!!m_fd)
		{
			int i = fcntl(m_fd.m_fd, F_SETFL, O_NONBLOCK);
			if (i == -1)
				m_fd.close();
			else
				m_epollPool->register_fd(m_fd.m_fd);
		}
	}

	socket(int sckt, int type, int protocol, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		:	m_fd(sckt),
			m_epollPool(epp),
			m_addressFamily(addressFamily)
	{
		if (!!m_fd)
		{
			int i = fcntl(m_fd.m_fd, F_SETFL, O_NONBLOCK);
			if (i == -1)
				m_fd.close();
			else
				m_epollPool->register_fd(m_fd.m_fd);
		}
	}

	// We dup() the socket so we can register reads and writes independently in epoll_pool.
	int get_dup_read_fd()
	{
		int result = m_dupReadFd.m_fd;
		if (result == -1)
		{
			if (!!m_fd)
			{
				result = m_dupReadFd.m_fd = dup(m_fd.m_fd);
				if (result != -1)
					m_epollPool->register_fd(result);
			}
		}
		return result;
	}

	void read_endpoints()
	{
		// determine local endpoint
		sockaddr_storage sockAddr;
		socklen_t sockLength = sizeof(sockaddr_storage);
		int i = getsockname(m_fd.m_fd, (sockaddr*)&sockAddr, &sockLength);
		m_localEndpoint.set_address_and_port(&sockAddr, sockLength);

		// determine remote endpoint
		sockLength = sizeof(sockaddr_storage);
		i = getpeername(m_fd.m_fd, (sockaddr*)&sockAddr, &sockLength);
		m_remoteEndpoint.set_address_and_port(&sockAddr, sockLength);
	}

	int bind_any(unsigned short localPort = 0)
	{
		int i = -1;
		if (!!m_fd)
		{
			if (m_addressFamily == inetv4)	// ipv4
			{
				sockaddr_in addr;
				memset(&addr, 0, sizeof(sockaddr_in)); 
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
				addr.sin_port = htons(localPort);
				i = bind(m_fd.m_fd, (sockaddr*)&addr, sizeof(sockaddr_in));
			}
			else if (m_addressFamily == inetv6)
			{
				sockaddr_in6 addr;
				memset(&addr, 0, sizeof(sockaddr_in6)); 
				addr.sin6_family = AF_INET6;
				addr.sin6_addr = in6addr_any;
				addr.sin6_port = htons(localPort);
				i = bind(m_fd.m_fd, (sockaddr*)&addr, sizeof(sockaddr_in6));
			}
			else
				COGS_ASSERT(false);	// ??
		}
		return i;
	}

	void close_source()
	{
		if (!!m_fd)
			shutdown(m_fd.m_fd, SHUT_RD);
	}

	void close_sink()
	{
		if (!!m_fd)
			shutdown(m_fd.m_fd, SHUT_WR);
	}

	void close()
	{
		if (!!m_fd)
			shutdown(m_fd.m_fd, SHUT_RD | SHUT_WR);
	}

public:
	~socket()
	{
		close();
	}
};


}
}
}
}


#endif
