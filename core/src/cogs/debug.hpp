//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//



// Status: Good


#include "cogs/env/mem/alignment.hpp"
#include "cogs/macro_stringify.hpp"
#include "cogs/macro_concat.hpp"


#ifndef COGS_DEBUG
#define COGS_DEBUG


#define COGS_DEBUG_AT __FILE__ ":" COGS_STRINGIFY_CHAR(__LINE__)


// Leak detection also does double-delete detection

#ifndef COGS_DEBUG_REF_LEAKED_FUNCTION_DETECTION
#define COGS_DEBUG_REF_LEAKED_FUNCTION_DETECTION	0
#endif

#ifndef COGS_DEBUG_LEAKED_REF_DETECTION
#define COGS_DEBUG_LEAKED_REF_DETECTION				0
#endif

#ifndef COGS_DEBUG_LEAKED_BLOCK_DETECTION
#define COGS_DEBUG_LEAKED_BLOCK_DETECTION			0
#endif

#ifndef COGS_DEBUG_ALLOC_LOGGING
#define COGS_DEBUG_ALLOC_LOGGING					0
#endif

#ifndef COGS_DEBUG_RC_LOGGING
#define COGS_DEBUG_RC_LOGGING						0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_INIT
#define COGS_DEBUG_ALLOC_BUFFER_INIT				0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_DEINIT
#define COGS_DEBUG_ALLOC_BUFFER_DEINIT				0
#endif

#ifndef COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
#define COGS_DEBUG_ALLOC_OVERFLOW_CHECKING			0
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE
#define COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE			0xAB
#endif

#ifndef COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE
#define COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE		0xDE
#endif

#ifndef COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE
#define COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE	0x89
#endif

#define OVERFLOW_CHECK_SIZE (largest_alignment * 2 * 10)

// If DEBUG_RC_OBJ is set to 1, to easing data navigation in debugger,
//		rc_obj<> will include a (redundant) pointer to it's content block.
#ifndef COGS_DEBUG_RC_OBJ
#define COGS_DEBUG_RC_OBJ	1
#endif


// If COGS_DEBUG_TRANSACTABLE is set to 1, to easing data navigation in debugger,
//		thread_safe_transactable will include a (redundant) pointer to the embedded contents.
// If COGS_DEBUG_RC_OBJ is also set to 1, the descriptior will include a (redundant) pointer to it's content block.
#ifndef COGS_DEBUG_TRANSACTABLE
#define COGS_DEBUG_TRANSACTABLE	1
#endif


#endif
