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


class socket : public object
{
private:
	rcref<os::io::epoll_pool> m_epollPool;
	address_family m_addressFamily;
	endpoint m_localEndpoint;
	endpoint m_remoteEndpoint;
	auto_fd m_fd;

	// We dup() the socket so we can register reads and writes independently in epoll_pool.
	auto_fd m_dupReadFd;

public:
	socket(rc_obj_base& desc, int type, int protocol, address_family addressFamily = address_family::inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		: object(desc),
		m_fd(::socket((int)addressFamily, type, protocol)),
		m_epollPool(epp),
		m_addressFamily(addressFamily)
	{
		if (!!m_fd)
		{
			int i = fcntl(m_fd.get(), F_SETFL, O_NONBLOCK);
			if (i == -1)
				m_fd.release();
			else
				m_epollPool->register_fd(m_fd.get());
		}
	}

	socket(rc_obj_base& desc, int sckt, int type, int protocol, address_family addressFamily = address_family::inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		: object(desc),
		m_fd(sckt),
		m_epollPool(epp),
		m_addressFamily(addressFamily)
	{
		if (!!m_fd)
		{
			int i = fcntl(m_fd.get(), F_SETFL, O_NONBLOCK);
			if (i == -1)
				m_fd.release();
			else
				m_epollPool->register_fd(m_fd.get());
		}
	}

	endpoint& get_local_endpoint() { return m_localEndpoint; }
	const endpoint& get_local_endpoint() const { return m_localEndpoint; }
	void set_local_endpoint(const endpoint& ep) { m_localEndpoint = ep; }

	endpoint& get_remote_endpoint() { return m_remoteEndpoint; }
	const endpoint& get_remote_endpoint() const { return m_remoteEndpoint; }
	void set_remote_endpoint(const endpoint& ep) { m_remoteEndpoint = ep; }

	rcref<const net::endpoint> get_local_endpoint_ref() const { return this_rcref.member_cast_to(m_localEndpoint); }
	rcref<const net::endpoint> get_remote_endpoint_ref() const { return this_rcref.member_cast_to(m_remoteEndpoint); }

	~socket()
	{
		close();
	}

	// We dup() the socket so we can register reads and writes independently in epoll_pool.
	int get_dup_read_fd()
	{
		int result = m_dupReadFd.get();
		if (result == -1)
		{
			if (!!m_fd)
			{
				result = m_dupReadFd.get() = dup(m_fd.get());
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
		int i = getsockname(m_fd.get(), (sockaddr*)&sockAddr, &sockLength);
		m_localEndpoint.set_address_and_port(&sockAddr, sockLength);

		// determine remote endpoint
		sockLength = sizeof(sockaddr_storage);
		i = getpeername(m_fd.get(), (sockaddr*)&sockAddr, &sockLength);
		m_remoteEndpoint.set_address_and_port(&sockAddr, sockLength);
	}

	int bind_any(unsigned short localPort = 0)
	{
		int i = -1;
		if (!!m_fd)
		{
			int enable = 1;
			setsockopt(m_fd.get(), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

			if (m_addressFamily == address_family::inetv4) // ipv4
			{
				sockaddr_in addr;
				memset(&addr, 0, sizeof(sockaddr_in));
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
				addr.sin_port = htons(localPort);
				i = bind(m_fd.get(), (sockaddr*)&addr, sizeof(sockaddr_in));
			}
			else if (m_addressFamily == address_family::inetv6)
			{
				sockaddr_in6 addr;
				memset(&addr, 0, sizeof(sockaddr_in6));
				addr.sin6_family = AF_INET6;
				addr.sin6_addr = in6addr_any;
				addr.sin6_port = htons(localPort);
				i = bind(m_fd.get(), (sockaddr*)&addr, sizeof(sockaddr_in6));
			}
			else
				COGS_ASSERT(false); // ??
		}
		return i;
	}

	void close_source()
	{
		if (!!m_fd)
			shutdown(m_fd.get(), SHUT_RD);
	}

	void close_sink()
	{
		if (!!m_fd)
			shutdown(m_fd.get(), SHUT_WR);
	}

	void close()
	{
		if (!!m_fd)
			shutdown(m_fd.get(), SHUT_RD | SHUT_WR);
	}

	int get()
	{
		return m_fd.get();
	}

	int get_dup()
	{
		return m_dupReadFd.get();
	}

	os::io::epoll_pool& get_pool()
	{
		return *m_epollPool;
	}
};


}
}
}
}


#endif
