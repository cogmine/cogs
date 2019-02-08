//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_IP_INIT_TOKEN
#define COGS_OS_IP_INIT_TOKEN


#include "cogs/mem/rcref.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class init_token;


// Used internally, to initialize the network subsystem.  Teardown does
// not occur until all references to init_token are out of scope.
rcref<init_token> initialize();


}
}
}
}


#endif
