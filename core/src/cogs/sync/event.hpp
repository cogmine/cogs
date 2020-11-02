//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_EVENT
#define COGS_HEADER_SYNC_EVENT


#include "cogs/collections/function_list.hpp"


namespace cogs {


// An event will invoke all functions when fired
template <typename... args_t>
using event = function_list<void(args_t...)>;


// A handler_list will invoke functions in order until one returns true.
// This allows handlers to optionally overload other handlers, based on their order in the list.
template <typename... args_t>
using handler_list = function_list<bool(args_t...)>;


}


#endif
