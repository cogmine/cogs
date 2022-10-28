//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_DEBUG
#define COGS_HEADER_DEBUG

#include "cogs/env/mem/alignment.hpp"
#include "cogs/macro_stringify.hpp"
#include "cogs/macro_concat.hpp"


#define COGS_DEBUG_AT __FILE__ ":" COGS_STRINGIFY_CHAR(__LINE__)


// Leak detection also does double-delete detection

#ifndef COGS_DEBUG_LEAKED_REF_DETECTION
#define COGS_DEBUG_LEAKED_REF_DETECTION 0
#endif

#ifndef COGS_DEBUG_LEAKED_WEAK_REF_DETECTION
#define COGS_DEBUG_LEAKED_WEAK_REF_DETECTION COGS_DEBUG_LEAKED_REF_DETECTION
#endif

#ifndef COGS_DEBUG_LEAKED_BLOCK_DETECTION
#define COGS_DEBUG_LEAKED_BLOCK_DETECTION 0
#endif

#ifndef COGS_DEBUG_ALLOC_LOGGING
#define COGS_DEBUG_ALLOC_LOGGING 0
#endif

#ifndef COGS_DEBUG_RC_LOGGING
#define COGS_DEBUG_RC_LOGGING 0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_INIT
#define COGS_DEBUG_ALLOC_BUFFER_INIT 0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_DEINIT
#define COGS_DEBUG_ALLOC_BUFFER_DEINIT 0
#endif

#ifndef COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
#define COGS_DEBUG_ALLOC_OVERFLOW_CHECKING 0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE
#define COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE 0xAB
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE
#define COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE 0xDE
#endif

#ifndef COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE
#define COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE 0x89
#endif

#define COGS_OVERFLOW_CHECK_SIZE (largest_alignment * 2 * 10)

#if COGS_DEBUG_ALLOC_BUFFER_INIT || COGS_DEBUG_ALLOC_BUFFER_DEINIT || COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING  || COGS_DEBUG_ALLOC_LOGGING
#define COGS_USE_DEBUG_DEFAULT_ALLOCATOR 1
#else
#define COGS_USE_DEBUG_DEFAULT_ALLOCATOR 0
#endif


// If DEBUG_RC_OBJ is set to 1, rc_obj<> will include a (redundant) pointer to it's content block, easing data navigation when debugging.
#ifndef COGS_DEBUG_RC_OBJ
#ifdef COGS_DEBUG
#define COGS_DEBUG_RC_OBJ 1
#else
#define COGS_DEBUG_RC_OBJ 0
#endif
#endif


// If COGS_DEBUG_TRANSACTABLE is set to 1, thread_safe_transactable will include a (redundant) pointer to the embedded contents,
// easing data navigation when debugging,
// If COGS_DEBUG_RC_OBJ is also set to 1, the descriptior will also include a (redundant) pointer to it's content block.
#ifndef COGS_DEBUG_TRANSACTABLE
#ifdef COGS_DEBUG
#define COGS_DEBUG_TRANSACTABLE 1
#else
#define COGS_DEBUG_TRANSACTABLE 0
#endif
#endif


#endif
