//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_BYPASS_CONSTRUCTOR_PERMISSION
#define COGS_HEADER_MEM_BYPASS_CONSTRUCTOR_PERMISSION


#include <type_traits>


namespace cogs {


template <typename base_type>
class bypass_constructor_permission : public base_type
{
public:
	template <typename... args_t>
	bypass_constructor_permission(args_t&&... args)
		: base_type(std::forward<args_t>(args)...)
	{
	}
};


}

#endif
