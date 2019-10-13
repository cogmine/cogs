//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV


#include <intrin.h>
#include <new>
#include <typeinfo>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>

#include "cogs/os.hpp"
#include "cogs/math/bytes_to_int.hpp"


namespace cogs {

template <> class bytes_to_int<sizeof(unsigned char), false> { public: typedef unsigned char type; };
template <> class bytes_to_int<sizeof(char), true> { public: typedef char type; };

template <> class bytes_to_int<sizeof(unsigned short), false> { public: typedef unsigned short type; };
template <> class bytes_to_int<sizeof(signed short), true> { public: typedef signed short type; };

template <> class bytes_to_int<sizeof(unsigned long), false> { public: typedef unsigned long type; };
template <> class bytes_to_int<sizeof(signed long), true> { public: typedef signed long type; };

template <> class bytes_to_int<sizeof(unsigned __int64), false> { public: typedef unsigned __int64 type; };
template <> class bytes_to_int<sizeof(signed __int64), true> { public: typedef signed __int64 type; };

#define COGS_LONGEST_INT (8)

typedef bytes_to_int_t<COGS_LONGEST_INT> longest;
typedef bytes_to_uint_t<COGS_LONGEST_INT> ulongest;

static constexpr size_t longest_bits = sizeof(longest) * 8;

}


#endif

