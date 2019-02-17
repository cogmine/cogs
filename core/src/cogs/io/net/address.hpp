//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_NET_ADDRESS
#define COGS_HEADER_IO_NET_ADDRESS


#include "cogs/collections/composite_string.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/sync/dispatcher.hpp"


namespace cogs {
namespace io {

/// @defgroup Net Network I/O
/// @{
/// @ingroup IO
/// @brief Network I/O
/// @}

/// @ingroup Net
/// @brief Namespace for network I/O
namespace net {



/// @ingroup Net
/// @brief An interface for network address objects.
///
/// An address represents an origin of a datasource, or a destination of a datasink.
/// Unlike an enpoint, an address does not include information specific to the transport -
/// such as a port/channel number, or file offset.
class address
{
public:
	class reverse_lookup_result : public signallable_task<reverse_lookup_result>
	{
	private:
		composite_string m_result;

	protected:
		void complete(const composite_string& result)
		{
			m_result = result;
			signal();
		}

		virtual const reverse_lookup_result& get() const volatile { return *(const reverse_lookup_result*)this; }

	public:
		const composite_string& get_name() const { return m_result; }
	};

	virtual rcref<reverse_lookup_result> reverse_lookup() const	= 0;

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
};


cstring get_host_name_cstring();
composite_string get_host_name_string();



}
}
}


#endif

