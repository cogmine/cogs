//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good


#ifndef COGS_HEADER_ENV_MAIN
#define COGS_HEADER_ENV_MAIN


#include <utility>
#include "cogs/os/main.hpp"


namespace cogs {
namespace env {


inline int initialize() { return os::initialize(); }
inline void terminate() { os::terminate(); }

template <typename F> inline int main(F&& main_func) { return os::main(std::forward<F>(main_func)); }


}
}


#endif
