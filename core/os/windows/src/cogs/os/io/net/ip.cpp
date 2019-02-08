//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsEnhancement
// Notes: DNS Thread Pool needs to be replaced with implementation of async DNS client

#ifdef COGS_COMPILE_SOURCE


#include "cogs/operators.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/env.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/io/net/ip/tcp.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


placement<rcptr<thread_pool> > ip::address::s_dnsThreadPool;
placement<rcptr<init_token>  > s_initToken;


class ip::init_token
{
public:
	init_token()
	{
		WSAData wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);	// assume this won't fail, for now
		COGS_ASSERT(!result);
	}

	~init_token()
	{
		WSACleanup();
	}
};


void init_token_cleanup()
{
	volatile rcptr<init_token>& initToken = s_initToken.get();
	initToken = (init_token*)nullptr;
}


rcref<init_token> ip::initialize()
{
	volatile rcptr<init_token>& initToken = s_initToken.get();
	rcptr<init_token> myInitToken = initToken;
	if (!myInitToken)
	{
		rcptr<init_token> newInitToken = rcnew(init_token);
		if (initToken.compare_exchange(newInitToken, myInitToken))
		{
			myInitToken = newInitToken;
			cleanup_queue::get_default()->dispatch(&init_token_cleanup);
		}
	}
	return myInitToken.dereference();
}


}
}
}
}


#endif
