//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MACRO_STRINGIFY
#define COGS_HEADER_MACRO_STRINGIFY


#define COGS_STRINGIFY_CHAR_INNER(x) #x
#define COGS_STRINGIFY_CHAR(x) COGS_STRINGIFY_CHAR_INNER(x)

#define COGS_STRINGIFY_WCHAR_INNER(x) L ## #x
#define COGS_STRINGIFY_WCHAR(x) COGS_STRINGIFY_WCHAR_INNER(x)


#endif
