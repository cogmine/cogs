//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

	
// Status: Good

#ifndef COGS_HEADER_IO_NET_ENDPOINT
#define COGS_HEADER_IO_NET_ENDPOINT


#include "cogs/collections/string.hpp"
#include "cogs/io/net/address.hpp"


namespace cogs {
namespace io {
namespace net {



/// @ingroup Net
/// @brief An interface for network endpoint objects.
///
/// An endpoint represents an origin of a datasource, or a destination of a datasink.
/// Unlike an address, an endpoint also includes information specific to the transport - such as a port/channel
/// number, or file offset.
class endpoint
{
protected:
	endpoint()
	{ }

public:
	virtual composite_string to_string() const = 0;
	virtual composite_cstring to_cstring() const = 0;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		if (std::is_same<char_t, char>::value)
		{
			return to_cstring();
		}
		if (std::is_same<char_t, wchar_t>::value)
		{
			to_string();
		}

		return composite_string_t<char_t>();
	}

	virtual       address& get_address()       = 0;
	virtual const address& get_address() const = 0;
};


}
}
}


#endif
