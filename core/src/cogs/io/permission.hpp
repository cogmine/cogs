//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifndef COGS_HEADER_IO_PERMISSION
#define COGS_HEADER_IO_PERMISSION


// Status: Good


namespace cogs {
namespace io {


enum class permission
{
	read = 1,
	write = 2,
	read_write = 3 // read | write
};


}
}


#endif
