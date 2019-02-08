//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsEnhancement
// Notes: DNS Thread Pool needs to be replaced with implementation of async DNS client

#include "cogs/env.hpp"

#include "cogs/io/net/ip/address.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/sync/thread_pool.hpp"



namespace cogs {


	// Unfortunately, there is not yet a proper async DNS lookup API available.
	// For now, use a thread_pool to defer DNS lookups to another thread.
	// I may have to code DNS protocol/connections directly.  :/



placement<rcptr<thread_pool> >	io::net::ip::address::s_dnsThreadPool;

}
