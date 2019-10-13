//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_IO_NET_IP_NETWORK
#define COGS_HEADER_OS_IO_NET_IP_NETWORK


#include "cogs/os.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/sync/cleanup_queue.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


// Keeps the network subsystem in scope.
// Teardown does not occur until all references to network are out of scope.
class network
{
protected:
	network()
	{
		WSAData wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // assume this won't fail, for now
		COGS_ASSERT(!result);
	}

public:
	~network()
	{
		WSACleanup();
	}

	static rcref<network> get_default()
	{
		return singleton<network>::get();
	}
};


}
}
}
}


#endif
