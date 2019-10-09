//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_OS_MAIN
#define COGS_HEADER_OS_MAIN


#include <cstdlib>


namespace cogs {
namespace os {


inline int initialize() { return EXIT_SUCCESS; }
inline void terminate() { }

template <typename F, typename T>
inline int main(F&& mainFunc, T&& uiSubsystem)
{
	int result = mainFunc(std::forward<T>(uiSubsystem));
	rcptr<quit_dispatcher> qd = quit_dispatcher::get();
	if (!!qd)
		qd->get_event().wait();
	return result;
}


}
}


#define COGS_MAIN int main()


#endif
