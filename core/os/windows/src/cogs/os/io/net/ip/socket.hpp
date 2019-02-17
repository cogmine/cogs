//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_IO_NET_IP_SOCKET
#define COGS_HEADER_OS_IO_NET_IP_SOCKET


#include "cogs/os/io/completion_port.hpp"
#include "cogs/os/io/net/ip/endpoint.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class socket
{
protected:
	friend class tcp;

	rcref<network>				m_network;
	rcref<os::io::completion_port>	m_completionPort;
	SOCKET							m_socket;
	address_family					m_addressFamily;
	endpoint						m_localEndpoint;
	endpoint						m_remoteEndpoint;

	socket(int type, int protocol, address_family addressFamily = inetv4, const rcref<os::io::completion_port>& cp = os::io::completion_port::get(), const rcref<network>& n = network::get_default())
		:	m_completionPort(cp),
			m_network(n),
			m_addressFamily(addressFamily)
	{
		// Socket is both overlapped and non-blocking.
		m_socket = WSASocket(addressFamily, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_socket != INVALID_SOCKET)
		{
			m_completionPort->register_handle((HANDLE)m_socket);

			unsigned long argp =  1;
			int i = ioctlsocket(m_socket, FIONBIO, &argp);
		}
	}

	void read_endpoints()
	{
		if (m_socket != INVALID_SOCKET)
		{
			// determine local endpoint
			SOCKADDR_STORAGE sockAddr;
			int sockLength = sizeof(SOCKADDR_STORAGE);
			int i = getsockname(m_socket, (sockaddr*)&sockAddr, &sockLength);
			m_localEndpoint.set_address_and_port(&sockAddr, sockLength);

			// determine remote endpoint
			sockLength = sizeof(SOCKADDR_STORAGE);
			i = getpeername(m_socket, (sockaddr*)&sockAddr, &sockLength);
			m_remoteEndpoint.set_address_and_port(&sockAddr, sockLength);
		}
	}

	int bind_any(unsigned short localPort = 0)
	{
		int i = SOCKET_ERROR;
		if (m_socket != INVALID_SOCKET)
		{
			if (m_addressFamily == AF_INET)	// IPV4
			{
				sockaddr_in addr;
				memset(&addr, 0, sizeof(sockaddr_in));
				addr.sin_family = AF_INET;
				addr.sin_port = htons(localPort);
				addr.sin_addr.S_un.S_addr = INADDR_ANY;

				i = bind(m_socket, (SOCKADDR*)&addr, sizeof(sockaddr_in));
			}
			else if (m_addressFamily == AF_INET6)	// IPV6
			{
				sockaddr_in6 addr;
				memset(&addr, 0, sizeof(sockaddr_in6));
				addr.sin6_family = AF_INET6;
				addr.sin6_port = htons(localPort);
				addr.sin6_addr = in6addr_any;

				i = bind(m_socket, (SOCKADDR*)&addr, sizeof(sockaddr_in6));
			}
		}
		return i;
	}

	void close_source()
	{
		if (m_socket != INVALID_SOCKET)
			shutdown(m_socket, SD_RECEIVE);
	}

	void close_sink()
	{
		if (m_socket != INVALID_SOCKET)
			shutdown(m_socket, SD_SEND);
	}

	void close()
	{
		if (m_socket != INVALID_SOCKET)
			shutdown(m_socket, SD_BOTH);
	}

public:
	~socket()
	{
		if (m_socket != INVALID_SOCKET)
		{
			close();
			closesocket(m_socket);
		}
	}
};


}
}
}
}


#endif
