//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsEnhancement
// Notes: DNS Thread Pool needs to be replaced with implementation of async DNS client

#ifndef COGS_HEADER_OS_IO_NET_IP_ADDRESS
#define COGS_HEADER_OS_IO_NET_IP_ADDRESS


#include "cogs/env.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/function.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/io/net/server.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/io/completion_port.hpp"
#include "cogs/os/io/net/ip/network.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/sync/singleton.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class socket;
class tcp;
class endpoint;


enum address_family
{
	inetv4 = AF_INET,
	inetv6 = AF_INET6
};


class address : public net::address
{
private:
	size_t m_sockAddrSize;
	SOCKADDR_STORAGE m_sockAddr;

	class dns_thread_pool : public thread_pool { };

	static rcref<thread_pool> get_dns_thread_pool()
	{
		bool isNew;
		rcref<dns_thread_pool> result = singleton<dns_thread_pool>::get(isNew);
		if (isNew)
			result->start();
		return result;
	}

	mutable rcptr<network> m_network;

protected:
	friend class socket;
	friend class tcp;
	friend class endpoint;

	address(SOCKADDR* sockAddr, size_t sockAddrSize, const rcptr<network>& n)
		: m_sockAddrSize(sockAddrSize),
		m_network(n)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(sockaddr_in* sockAddr, size_t sockAddrSize, const rcptr<network>& n)
		: m_sockAddrSize(sockAddrSize),
		m_network(n)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(sockaddr_in6* sockAddr, size_t sockAddrSize, const rcptr<network>& n)
		: m_sockAddrSize(sockAddrSize),
		m_network(n)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	address(SOCKADDR_STORAGE* sockAddr, size_t sockAddrSize, const rcptr<network>& n)
		: m_sockAddrSize(sockAddrSize),
		m_network(n)
	{
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(SOCKADDR* sockAddr, size_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in* sockAddr, size_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(sockaddr_in6* sockAddr, size_t sockAddrSize)
	{
		m_sockAddrSize = sockAddrSize;
		memcpy(&m_sockAddr, sockAddr, sockAddrSize);
	}

	void set_address_and_port(SOCKADDR_STORAGE* sockAddr, size_t sockAddrSize)
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
		return result;
	}

	void set_port(unsigned short port)
	{
		if (m_sockAddr.ss_family == AF_INET)
		{
			sockaddr_in* sockAddr = (sockaddr_in*)&m_sockAddr;
			sockAddr->sin_port = port;
		}
		else if (m_sockAddr.ss_family == AF_INET6)
		{
			sockaddr_in6* sockAddr = (sockaddr_in6*)&m_sockAddr;
			sockAddr->sin6_port = port;
		}
	}

public:
	class lookup_result : public signallable_task_base<lookup_result>
	{
	private:
		string m_inputString;
		rcref<network> m_network;
		vector<address> m_addresses;
		rcptr<task<void> > m_poolTask;

	protected:
		friend class address;

		lookup_result(const ptr<rc_obj_base>& desc, const composite_string& s, const rcref<network>& n = network::get_default())
			: signallable_task_base<lookup_result>(desc),
			m_inputString(s.composite()),
			m_network(n)
		{
			rcref<thread_pool> pool = get_dns_thread_pool();
			m_poolTask = pool->dispatch([r{ this_rcref }]()
			{
				r->execute();
			});
		}

		void execute()
		{
			string s;
			size_t requiredSize = m_inputString.get_length() * 4;
			size_t currentSize;
			do {
				s.resize(requiredSize + 1); // +1 because we know we'll need to add a null
				currentSize = requiredSize;
				requiredSize = (size_t)IdnToAscii(0, m_inputString.get_const_ptr(), (int)m_inputString.get_length(), s.get_ptr(), (int)requiredSize);
			} while (requiredSize > currentSize);
			s.truncate_to(requiredSize);

			ADDRINFOW hints = { };
			hints.ai_family = AF_UNSPEC;
			hints.ai_flags = 0x00080000; // AI_DISABLE_IDN_ENCODING // We did the encoding already
			ADDRINFOW* ai = &hints;
			int err = GetAddrInfoW(s.cstr(), 0, 0, &ai);
			if (!err)
			{
				if (ai)
				{
					ADDRINFOW* cur = ai;
					do {
						m_addresses.append(1, address((SOCKADDR*)cur->ai_addr, cur->ai_addrlen, m_network));
						cur = cur->ai_next;
					} while (cur != 0);
					FreeAddrInfoW(ai);
				}
			}
			s.reset();
			m_inputString.reset(); // not needed anymore, free it
			signal();
		}

		virtual const lookup_result& get() const volatile { return *(const lookup_result*)this; }

	public:
		const vector<address> & get_hosts() const { return m_addresses; }

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
		rcref<network> m_network;
		SOCKADDR_STORAGE m_sockAddr;
		size_t m_sockAddrSize;

	protected:
		friend class address;

		reverse_lookup_result(const ptr<rc_obj_base>& desc, const address& addr, const rcref<network>& n = network::get_default())
			: net::address::reverse_lookup_result(desc),
			m_network(n)
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
			string s;
			s.resize(NI_MAXHOST);
			int err = GetNameInfoW((SOCKADDR*)&m_sockAddr, (socklen_t)m_sockAddrSize, s.get_ptr(), NI_MAXHOST, 0, 0, 0);
			if (!err)
				s.truncate_to(s.index_of(0));
			else
				s.clear();

			// Convert from punycode
			string result;
			size_t requiredSize = s.get_length(); // should only get shorter, right?
			size_t currentSize;
			do {
				result.resize(requiredSize);
				currentSize = requiredSize;
				requiredSize = (size_t)IdnToUnicode(0, s.get_const_ptr(), (int)s.get_length(), result.get_ptr(), (int)requiredSize);
			} while (requiredSize > currentSize);
			s.truncate_to(requiredSize);

			complete(result);
		}
	};

	friend class reverse_lookup_result;

	address()
		: m_sockAddrSize(0)
	{ }

	explicit address(const rcptr<network>& n)
		: m_sockAddrSize(0),
		m_network(n)
	{ }

//	address(address_family addressFamily)
//		: m_sockAddrSize(0)
//	{
//		m_sockAddr.ss_family = (ADDRESS_FAMILY)addressFamily;
//	}

	address(const address& addr)
		: m_sockAddrSize(addr.m_sockAddrSize),
		m_network(addr.m_network)
	{
		memcpy(&m_sockAddr, &addr.m_sockAddr, addr.m_sockAddrSize);
	}

	// Does NOT support host names and FQDN.  Supports only numeric ipv4, ipv6
	// For host names and FQDN, use lookup() instead, as they may have multiple associated numeric addresses.
	address(const composite_string& str, address_family addressFamily = inetv4, const rcref<network>& n = network::get_default() ) // AF_INET6 for ipv6
		: m_network(n)
	{
		INT i = sizeof(m_sockAddr);
		INT err = WSAStringToAddress((LPWSTR)str.composite().cstr(), addressFamily, 0, (SOCKADDR*)&m_sockAddr, &i);
		if (!err)
			m_sockAddrSize = i;
		else
			m_sockAddrSize = 0;
	}

	static address ipv4(const composite_string& str) { return address(str, inetv4); }
	static address ipv6(const composite_string& str) { return address(str, inetv6); }

	address& operator=(const address& addr)
	{
		m_sockAddrSize = addr.m_sockAddrSize;
		m_network = addr.m_network;
		memcpy(&m_sockAddr, &addr.m_sockAddr, addr.m_sockAddrSize);
		return *this;
	}

	// Any addr string, including host name or FQDN
	static rcref<lookup_result> lookup(const composite_string& addr)
	{
		return rcnew(bypass_constructor_permission<lookup_result>, addr);
	}

	virtual rcref<net::address::reverse_lookup_result> reverse_lookup() const
	{
		return rcnew(bypass_constructor_permission<reverse_lookup_result>, *this);
	}

	static address get_local_host(address_family addressFamily = inetv4)
	{
		address result;
		if (addressFamily == inetv4)
		{
			result.m_sockAddr.ss_family = AF_INET;
			sockaddr_in* sockAddr = (sockaddr_in*)&result.m_sockAddr;
			sockAddr->sin_addr = in4addr_loopback;
			sockAddr->sin_port = 0;
		}
		else if (addressFamily == inetv6)
		{
			result.m_sockAddr.ss_family = AF_INET6;
			sockaddr_in6* sockAddr = (sockaddr_in6*)&result.m_sockAddr;
			sockAddr->sin6_addr = in6addr_loopback;
			sockAddr->sin6_port = 0;
		}
		return result;
	}

	SOCKADDR* get_sockaddr() { return (SOCKADDR*)&m_sockAddr; }
	const SOCKADDR* get_sockaddr() const { return (SOCKADDR*)&m_sockAddr; }

	size_t get_sockaddr_size() const { return m_sockAddrSize; }
	void set_sockaddr_size(size_t sz) { m_sockAddrSize = sz; }

	address_family get_address_family() const { return (!m_sockAddrSize) ? (address_family)AF_UNSPEC : (address_family)m_sockAddr.ss_family; }

	virtual composite_cstring to_cstring() const
	{
		cstring result;
		if (m_sockAddrSize)
		{
			if (!m_network)
				m_network = network::get_default();
			result.resize(NI_MAXHOST); // slight waste of space, but safe
			int err = getnameinfo((sockaddr*)&m_sockAddr, (DWORD)m_sockAddrSize, result.get_ptr(), NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (!err)
				result.truncate_to(result.index_of(0));
			else
				result.clear();
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
