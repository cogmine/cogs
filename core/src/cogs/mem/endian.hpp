//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ENDIAN
#define COGS_ENDIAN


#include <memory.h>


namespace cogs {

/// @ingroup Mem
/// @brief Ending type
enum class endian_t {
	/// @brief Big endian
	big,// = 0,

	/// @brief Little endian
	little// = 1,
};


}


#endif
