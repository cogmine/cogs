//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV


#include <stdio.h>
#include <new>
#include <typeinfo>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>


#include "cogs/os.hpp"
#include "cogs/math/bytes_to_int.hpp"


namespace cogs {


template <> class bytes_to_int<sizeof(unsigned char), false> { public: typedef unsigned char int_t; };
template <> class bytes_to_int<sizeof(signed char), true> { public: typedef signed char int_t; };

template <> class bytes_to_int<sizeof(unsigned short), false> { public: typedef unsigned short int_t; };
template <> class bytes_to_int<sizeof(signed short), true> { public: typedef signed short int_t; };

template <> class bytes_to_int<sizeof(unsigned long), false> { public: typedef unsigned long int_t; };
template <> class bytes_to_int<sizeof(signed long), true> { public: typedef signed long int_t; };

#define COGS_LONGEST_INT (4)

typedef bytes_to_int_t<COGS_LONGEST_INT> longest;
typedef bytes_to_uint_t<COGS_LONGEST_INT> ulongest;

static constexpr size_t longest_bits = sizeof(longest) * 8;



}


#endif

