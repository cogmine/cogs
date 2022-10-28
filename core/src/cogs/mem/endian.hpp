//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_ENDIAN
#define COGS_HEADER_MEM_ENDIAN


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
