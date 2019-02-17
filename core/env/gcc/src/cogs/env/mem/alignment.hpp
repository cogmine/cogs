//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALIGNMENT
#define COGS_HEADER_ENV_MEM_ALIGNMENT


#include <stdlib.h>


namespace cogs {

	
//// As of this writing, gcc does not allow the aligned attribute to be applied to template types.  i.e.:
////		Error: sorry, unimplemented: applying attributes to template parameters is not implemented
//// So, we can't easily provide an aligned type through a template, across platforms.
////
////template <typename type_in, unsigned int alignment> class aligned { public: typedef type_in type __attribute__ ((aligned (alignment))); };
//template <typename type_in, unsigned int alignment> class get_aligned_type;
//
//template <typename type_in> class get_aligned_type<type_in,  1>	{ public: typedef type_in type __attribute__ ((aligned( 1))); };
//template <typename type_in> class get_aligned_type<type_in,  2>	{ public: typedef type_in type __attribute__ ((aligned( 2))); };
//template <typename type_in> class get_aligned_type<type_in,  4>	{ public: typedef type_in type __attribute__ ((aligned( 4))); };
//template <typename type_in> class get_aligned_type<type_in,  8>	{ public: typedef type_in type __attribute__ ((aligned( 8))); };
//template <typename type_in> class get_aligned_type<type_in, 16>	{ public: typedef type_in type __attribute__ ((aligned(16))); };

#ifdef _M_X64
//template <typename type_in> class get_aligned_type<type_in, 32>	{ public: typedef __declspec(align(32)) type_in type; };
	static constexpr size_t largest_alignment = 16;// 32;
#else
	static constexpr size_t largest_alignment = 32;// 16;
#endif



}


#endif
