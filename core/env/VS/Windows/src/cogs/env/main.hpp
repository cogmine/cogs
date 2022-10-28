//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MAIN
#define COGS_HEADER_ENV_MAIN


#include <utility>
#include "cogs/os/main.hpp"
#include "cogs/gui/subsystem.hpp"


namespace cogs {
namespace env {

inline int initialize() { return os::initialize(); }
inline void terminate() { os::terminate(); }

template <typename F, typename T> inline int main(F&& mainFunc, T&& uiSubsystem) { return os::main(std::forward<F>(mainFunc), std::forward<T>(uiSubsystem)); }


}
}


#endif
