//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_IO_NET_IP_ENDPOINT
#define COGS_HEADER_OS_IO_NET_IP_ENDPOINT


#include "cogs/os/io/net/ip/address.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class socket;


class endpoint : public net::endpoint
{
private:
	address m_address;

protected:
	friend class socket;

	endpoint(sockaddr* sockAddr, size_t sockAddrSize)
		: m_address(sockAddr, sockAddrSize)
	{ }

	endpoint(sockaddr_in* sockAddr, size_t sockAddrSize)
		: m_address(sockAddr, sockAddrSize)
	{ }

	endpoint(sockaddr_in6* sockAddr, size_t sockAddrSize)
		: m_address(sockAddr, sockAddrSize)
	{ }

	endpoint(sockaddr_storage* sockAddr, size_t sockAddrSize)
		: m_address(sockAddr, sockAddrSize)
	{ }

	void set_address_and_port(sockaddr* sockAddr, size_t sockAddrSize)
	{
		m_address.set_address_and_port(sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in* sockAddr, size_t sockAddrSize)
	{
		m_address.set_address_and_port(sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in6* sockAddr, size_t sockAddrSize)
	{
		m_address.set_address_and_port(sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_storage* sockAddr, size_t sockAddrSize)
	{
		m_address.set_address_and_port(sockAddr, sockAddrSize);
	}

	void set_port(unsigned short port) { m_address.set_port(port); }

	void set_address(const address& addr)
	{
		unsigned short port = get_port();
		m_address = addr;
		set_port(port);
	}

public:
	endpoint()
	{ }

	endpoint(const address& addr, unsigned short port)
		: m_address(addr)
	{
		set_port(port);
	}

	endpoint(const endpoint& src)
		: m_address(src.m_address)
	{ }

	endpoint& operator=(const endpoint& src)
	{
		m_address = src.m_address;
		return *this;
	}

	unsigned short get_port() const { return m_address.get_port(); }

	virtual composite_cstring to_cstring() const
	{
		composite_cstring s = m_address.to_cstring();
		s += cstring::contain(":", 1);
		s += ushort_type(get_port()).to_cstring();
		return s;
	}

	virtual composite_string to_string() const
	{
		composite_string s = m_address.to_string();
		s += string::contain(L":", 1);
		s += ushort_type(get_port()).to_string();
		return s;
	}

	virtual address& get_address() { return m_address; }
	virtual const address& get_address() const { return m_address; }

	sockaddr* get_sockaddr() { return get_address().get_sockaddr(); }
	const sockaddr* get_sockaddr() const { return get_address().get_sockaddr(); }

	size_t get_sockaddr_size() const { return get_address().get_sockaddr_size(); }
	void set_sockaddr_size(size_t sz) { get_address().set_sockaddr_size(sz); }
};


}
}
}
}


#endif
