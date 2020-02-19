//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_NET_CONNECTION
#define COGS_HEADER_IO_NET_CONNECTION


#include "cogs/io/datastream.hpp"
#include "cogs/io/net/endpoint.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/rcref.hpp"


namespace cogs {
namespace io {
namespace net {


/// @ingroup Net
/// @brief An interface for datastreams that represent connections, and include endpoint addresses.
class connection : public datastream
{
public:
	virtual rcref<const endpoint> get_local_endpoint() const = 0;
	virtual rcref<const endpoint> get_remote_endpoint() const = 0;
};


}
}
}


#endif
