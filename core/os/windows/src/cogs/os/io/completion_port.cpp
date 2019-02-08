//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good


#ifdef COGS_COMPILE_SOURCE

#include "cogs/mem/placement.hpp"
#include "cogs/os/io/completion_port.hpp"


namespace cogs {
namespace os {
namespace io {


placement<rcptr<completion_port> >			completion_port::s_defaultCompletionPort;


rcref<completion_port> completion_port::get()
{
	volatile rcptr<completion_port>& defaultCompletionPort = s_defaultCompletionPort.get();
	rcptr<completion_port> myCompletionPort = defaultCompletionPort;
	if (!myCompletionPort)
	{
		rcptr<completion_port> newCompletionPort = rcnew(bypass_constructor_permission<completion_port>);
		if (defaultCompletionPort.compare_exchange(newCompletionPort, myCompletionPort, myCompletionPort))
		{
			newCompletionPort->start();
			myCompletionPort = newCompletionPort;
			cleanup_queue::get_default()->dispatch(&cleanup_globals);
		}
	}
	return myCompletionPort.dereference();
}


}
}
}

#endif
