//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV
#define COGS_HEADER_ENV

#include <stdint.h>
#include <new>
#include <typeinfo>
#include <stdio.h>
#include <wchar.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>

#include "cogs/os.hpp"
#include "cogs/math/bytes_to_int.hpp"

namespace cogs {


template <> class bytes_to_int<sizeof(uint8_t), false> { public: typedef uint8_t type; };
template <> class bytes_to_int<sizeof(int8_t), true> { public: typedef int8_t type; };

template <> class bytes_to_int<sizeof(uint16_t), false> { public: typedef uint16_t type; };
template <> class bytes_to_int<sizeof(int16_t), true> { public: typedef int16_t type; };

template <> class bytes_to_int<sizeof(uint32_t), false> { public: typedef uint32_t type; };
template <> class bytes_to_int<sizeof(int32_t), true> { public: typedef int32_t type; };

template <> class bytes_to_int<sizeof(uint64_t), false> { public: typedef uint64_t type; };
template <> class bytes_to_int<sizeof(int64_t), true> { public: typedef int64_t type; };

#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__)
#define COGS_LONGEST_INT (16)
template <> class bytes_to_int<sizeof(__uint128_t), false> { public: typedef __uint128_t type; };
template <> class bytes_to_int<sizeof(__int128_t), true> { public: typedef __int128_t type; };
#else
#define COGS_LONGEST_INT (8)
#endif


typedef bytes_to_int_t<COGS_LONGEST_INT> longest;
typedef bytes_to_uint_t<COGS_LONGEST_INT> ulongest;

static constexpr size_t longest_bits = sizeof(longest) * 8;


}



#endif
