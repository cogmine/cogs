//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsEnhancement
// Notes: DNS Thread Pool needs to be replaced with implementation of async DNS client

#ifndef COGS_HEADER_OS_IO_NET_IP_ADDRESS
#define COGS_HEADER_OS_IO_NET_IP_ADDRESS


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "cogs/env.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/function.hpp"
#include "cogs/os/io/auto_fd.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/io/net/server.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/sync/singleton.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class endpoint;
class socket;


enum class address_family
{
	inetv4 = AF_INET,
	inetv6 = AF_INET6
};


class address : public net::address
{
private:
	sockaddr_storage m_sockAddr;
	socklen_t m_sockAddrSize;

	class dns_thread_pool : public thread_pool { };

	static rcref<thread_pool> get_dns_thread_pool()
	{
		bool isNew;
		rcref<dns_thread_pool> result = singleton<dns_thread_pool>::get(isNew);
		if (isNew)
			result->start();
		return result;
	}

protected:
	friend class endpoint;
	friend class socket;

	address(sockaddr* sockAddr, socklen_t sockAddrSize)
		: m_sockAddrSize(sockAddrSize)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(sockaddr_in* sockAddr, socklen_t sockAddrSize)
		: m_sockAddrSize(sockAddrSize)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(sockaddr_in6* sockAddr, socklen_t sockAddrSize)
		: m_sockAddrSize(sockAddrSize)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(sockaddr_storage* sockAddr, socklen_t sockAddrSize)
		: m_sockAddrSize(sockAddrSize)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr* sockAddr, socklen_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in* sockAddr, socklen_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in6* sockAddr, socklen_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_storage* sockAddr, socklen_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	unsigned short get_port() const
	{
		unsigned short result = 0;
		if (m_sockAddr.ss_family == AF_INET)
		{
			sockaddr_in* sockAddr = (sockaddr_in*)&m_sockAddr;
			result = sockAddr->sin_port;
		}
		else if (m_sockAddr.ss_family == AF_INET6)
		{
			sockaddr_in6* sockAddr = (sockaddr_in6*)&m_sockAddr;
			result = sockAddr->sin6_port;
		}
		return ntohs(result);
	}

	void set_port(unsigned short port)
	{
		if (m_sockAddr.ss_family == AF_INET)
		{
			sockaddr_in* sockAddr = (sockaddr_in*)&m_sockAddr;
			sockAddr->sin_port = htons(port);
		}
		else if (m_sockAddr.ss_family == AF_INET6)
		{
			sockaddr_in6* sockAddr = (sockaddr_in6*)&m_sockAddr;
			sockAddr->sin6_port = htons(port);
		}
	}

public:
	class lookup_result : public signallable_task_base<lookup_result>
	{
	private:
		composite_string m_inputString;
		vector<address> m_addresses;
		rcptr<task<void> > m_poolTask;

	protected:
		friend class address;

		lookup_result(rc_obj_base& desc, const composite_string& s)
			: signallable_task_base<lookup_result>(desc),
			m_inputString(s)
		{
			rcref<thread_pool> pool = get_dns_thread_pool();
			m_poolTask = pool->dispatch([r{ this_rcref }]()
			{
				r->execute();
			});
		}

		void execute()
		{
			addrinfo* ai = 0;
			int err = getaddrinfo(string_to_cstring(m_inputString).cstr(), 0, 0, &ai);
			if (!err)
			{
				if (ai)
				{
					addrinfo* cur = ai;
					do {
						address addr((sockaddr*)cur->ai_addr, cur->ai_addrlen);
						m_addresses.append(1, addr);
						cur = cur->ai_next;
					} while (cur != 0);
					freeaddrinfo(ai);
				}
			}
			m_inputString.clear(); // not needed anymore, free it
			signal();
		}

		virtual const lookup_result& get() const volatile { return *(const lookup_result*)this; }

	public:
		const vector<address>& get_hosts() const { return m_addresses; }

		virtual rcref<task<bool> > cancel() volatile
		{
			m_poolTask->cancel();
			return signallable_task_base<lookup_result>::cancel();
		}
	};
	friend class lookup_result;

	class reverse_lookup_result : public net::address::reverse_lookup_result
	{
	private:
		sockaddr_storage m_sockAddr;
		socklen_t m_sockAddrSize;

	protected:
		friend class address;

		reverse_lookup_result(rc_obj_base& desc, const address& addr)
			: net::address::reverse_lookup_result(desc)
		{
			m_sockAddrSize = addr.m_sockAddrSize;
			memcpy(&m_sockAddr, &addr.m_sockAddr, m_sockAddrSize);

			rcref<thread_pool> pool = get_dns_thread_pool();
			pool->dispatch([r{ this_rcref }]()
			{
				r->execute();
			});
		}

		void execute()
		{
			cstring str;
			str.resize(1024);
			int err = getnameinfo((sockaddr*)&m_sockAddr, m_sockAddrSize, str.get_ptr(), 1024, 0, 0, 0);
			if (!err)
				str.truncate_to(str.index_of(0));
			else
				str.clear();
			complete(cstring_to_string(str)); // TBD, IdnToUnicode conversion
		}
	};

	address()
		: m_sockAddrSize(0)
	{ }

	address(const address& addr)
		: m_sockAddrSize(addr.m_sockAddrSize)
	{
		memcpy(&m_sockAddr, &addr.m_sockAddr, addr.m_sockAddrSize);
	}

	// Does NOT support host names and FQDN.  Supports only numeric ipv4, ipv6
	// For host names and FQDN, use lookup() instead, as they may have multiple associated numeric addresses.
	address(const composite_string& str, address_family addressFamily = address_family::inetv4) // AF_INET6 for ipv6
	{
		int err = inet_pton((int)addressFamily, string_to_cstring(str).cstr(), &m_sockAddr);
		if (!err)
			m_sockAddrSize = 0;
		else
			m_sockAddrSize = sizeof(m_sockAddr);
	}

	static address ipv4(const composite_string& str) { return address(str, address_family::inetv4); }
	static address ipv6(const composite_string& str) { return address(str, address_family::inetv6); }

	address& operator=(const address& addr)
	{
		m_sockAddrSize = addr.m_sockAddrSize;
		memcpy(&m_sockAddr, &addr.m_sockAddr, addr.m_sockAddrSize);
		return *this;
	}

	// Any addr string, including host name or FQDN
	static rcref<lookup_result> lookup(const composite_string& addr) { return rcnew(lookup_result, addr); }
	virtual rcref<net::address::reverse_lookup_result> reverse_lookup() const { return rcnew(reverse_lookup_result, *this); }

	static address get_local_host(address_family addressFamily = address_family::inetv4)
	{
		address result;
		if (addressFamily == address_family::inetv4)
		{
			result.m_sockAddr.ss_family = AF_INET;
			sockaddr_in* sockAddr = (sockaddr_in*)&result.m_sockAddr;
			sockAddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			sockAddr->sin_port = 0;
		}
		else if (addressFamily == address_family::inetv6)
		{
			result.m_sockAddr.ss_family = AF_INET6;
			sockaddr_in6* sockAddr = (sockaddr_in6*)&result.m_sockAddr;
			sockAddr->sin6_addr = in6addr_loopback;
			sockAddr->sin6_port = 0;
		}
		return result;
	}

	sockaddr* get_sockaddr() { return (sockaddr*)&m_sockAddr; }
	const sockaddr* get_sockaddr() const { return (const sockaddr*)&m_sockAddr; }

	socklen_t get_sockaddr_size() const { return m_sockAddrSize; }
	void set_sockaddr_size(socklen_t sz) { m_sockAddrSize = sz; }

	address_family get_address_family() const { return (!m_sockAddrSize) ? (address_family)AF_UNSPEC : (address_family)m_sockAddr.ss_family; }

	virtual composite_cstring to_cstring() const
	{
		cstring result;
		if (m_sockAddrSize)
		{
			socklen_t bufSize = INET6_ADDRSTRLEN;
			for (;;)
			{
				result.resize(bufSize);
				socklen_t prevBufSize = bufSize;
				const char* s = inet_ntop((int)get_address_family(), &m_sockAddr, result.get_ptr(), bufSize);
				if (s != NULL)
					result.resize(result.index_of(0));
				else
				{
					if ((errno == ENOSPC) && (bufSize > prevBufSize)) // only retry if buffer was insufficient
						continue;
					result.clear();
				}
				break;
			}
		}

		return result;
	}

	virtual composite_string to_string() const
	{
		// The numeric form of an address is always possible to represent in ASCII.  No translation needed to convert to wide chars.
		return cstring_to_string(to_cstring());
	}
};


}


inline cstring get_host_name_cstring()
{
	cstring cstr;
	cstr.resize(256);
	char* buf = cstr.get_ptr();
	buf[0] = 0;
	gethostname(buf, 256);
	cstr.resize(strlen(buf));
	return cstr;
}

inline composite_string get_host_name_string()
{
	return cstring_to_string(get_host_name_cstring());
}


}
}
}


#endif

