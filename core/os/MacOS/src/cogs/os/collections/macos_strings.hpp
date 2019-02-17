//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_MACOS_STRINGS
#define COGS_HEADER_OS_MACOS_STRINGS


#import <Cocoa/Cocoa.h>
#include "cogs/os.hpp"
#include "cogs/collections/string.hpp"


namespace cogs {


#if TARGET_RT_BIG_ENDIAN
    const NSStringEncoding kEncoding_wchar_t = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32BE);
#else
    const NSStringEncoding kEncoding_wchar_t = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE);
#endif


inline NSString* string_to_NSString(const string& s)
{
	return [[NSString alloc] initWithBytes: s.get_const_ptr()
			length: s.get_length() * sizeof(wchar_t)
			encoding: kEncoding_wchar_t ];
}


inline string NSString_to_string(const NSString* nsString)
{
	string result;
	
	NSData* asData = [nsString dataUsingEncoding:kEncoding_wchar_t];
	result.append((wchar_t*)[asData bytes], [asData length] /  sizeof(wchar_t));

	return result;
}


}


#endif
