//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_HANDLE
#define COGS_OS_HANDLE


#include "cogs/env/mem/alignment.hpp"
#include "cogs/os.hpp"
#include "cogs/operators.hpp"
#include "cogs/mem/auto_handle.hpp"


namespace cogs {
namespace os {


inline void auto_handle_impl_CloseHandle(HANDLE h) { CloseHandle(h); }
typedef auto_handle<HANDLE, INVALID_HANDLE_VALUE, auto_handle_impl_CloseHandle> auto_HANDLE;

}
}


#endif
