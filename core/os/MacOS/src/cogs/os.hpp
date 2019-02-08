//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_MACOS
#define COGS_OS_MACOS


#import <Cocoa/Cocoa.h>
#include <stdint.h>
#include <malloc/malloc.h>


#include "cogs/arch.hpp"


namespace cogs {
namespace ui {
namespace os {
	inline void beep()	{ NSBeep(); }
}
}
}




#endif
