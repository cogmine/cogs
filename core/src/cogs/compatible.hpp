//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress


#ifndef COGS_HEADER_COMPATIBLE
#define COGS_HEADER_COMPATIBLE

#include <type_traits>


namespace cogs {


template <typename T1, typename T2 = T1, typename enable = void>
class compatible
{
};

template <typename T1, typename T2 = T1, typename enable = void>
using compatible_t = typename compatible<T1, T2, typename enable>::type;


template <typename T> class compatible<T, T> { public: typedef T type; };


// By default, map const and/or volatile to the version with no CV qualifier

template <typename T1, typename T2, typename enable> class compatible<const T1, T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<volatile T1, T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const volatile T1, T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };

template <typename T1, typename T2, typename enable> class compatible<T1, const T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const T1, const T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<volatile T1, const T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const volatile T1, const T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };

template <typename T1, typename T2, typename enable> class compatible<T1, volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const T1, volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<volatile T1, volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const volatile T1, volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };

template <typename T1, typename T2, typename enable> class compatible<T1, const volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const T1, const volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<volatile T1, const volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };
template <typename T1, typename T2, typename enable> class compatible<const volatile T1, const volatile T2, enable> { public: typedef typename compatible<T1, T2, enable>::type type; };





}


#endif

