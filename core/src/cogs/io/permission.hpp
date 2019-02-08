//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

	
// Status: Good

#ifndef COGS_IO_PERMISSION
#define COGS_IO_PERMISSION


namespace cogs {
namespace io {


enum permission
{
	read_only = 1,
	write_only = 2,
	read_write = 3	// read_only | write_only
};


}
}


#endif
