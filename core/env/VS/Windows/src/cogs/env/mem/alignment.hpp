//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_ALIGNMENT
#define COGS_HEADER_ENV_MEM_ALIGNMENT


#include <malloc.h>


namespace cogs {

//
////// As of this writing, gcc does not allow the aligned attribute to be applied to template types.  i.e.:
//////      Error: sorry, unimplemented: applying attributes to template parameters is not implemented
////// So, we can't easily provide an aligned type through a template, across platforms.  (yet)
//////
//////template <typename type_in, unsigned int alignment> class aligned { public: typedef __declspec(align(alignment)) type_in type; };
////
//// Furthermore, VS2010 is not accepting template constants passed to __declspec(align()), so must
//// be specialized for each alignment constant.
//
//template <typename type_in, unsigned int alignment> class get_aligned_type;
//
//template <typename type_in> class get_aligned_type<type_in,  1> { public: typedef __declspec(align( 1)) type_in type; };
//template <typename type_in> class get_aligned_type<type_in,  2> { public: typedef __declspec(align( 2)) type_in type; };
//template <typename type_in> class get_aligned_type<type_in,  4> { public: typedef __declspec(align( 4)) type_in type; };
//template <typename type_in> class get_aligned_type<type_in,  8> { public: typedef __declspec(align( 8)) type_in type; };
//template <typename type_in> class get_aligned_type<type_in, 16> { public: typedef __declspec(align(16)) type_in type; };

#ifdef _M_X64
//template <typename type_in> class get_aligned_type<type_in, 32> { public: typedef __declspec(align(32)) type_in type; };
	static constexpr size_t largest_alignment = 16;// 32;
#else
	static constexpr size_t largest_alignment = 8;// 16;
#endif



}


#endif
