//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress


#ifndef COGS_HEADER_COMPATIBLE
#define COGS_HEADER_COMPATIBLE

#include <type_traits>


namespace cogs {


template <typename T1, typename T2 = T1, typename enable = void>
struct compatible
{
};

template <typename T1, typename T2 = T1, typename enable = void>
using compatible_t = typename compatible<T1, T2, enable>::type;


template <typename T> struct compatible<T, T> { public: typedef T type; };


// By default, map const and/or volatile to the version with no CV qualifier

template <typename T1, typename T2, typename enable> struct compatible<const T1, T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<volatile T1, T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const volatile T1, T2, enable> : public compatible<T1, T2, enable> {};

template <typename T1, typename T2, typename enable> struct compatible<T1, const T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const T1, const T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<volatile T1, const T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const volatile T1, const T2, enable> : public compatible<T1, T2, enable> {};

template <typename T1, typename T2, typename enable> struct compatible<T1, volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const T1, volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<volatile T1, volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const volatile T1, volatile T2, enable> : public compatible<T1, T2, enable> {};

template <typename T1, typename T2, typename enable> struct compatible<T1, const volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const T1, const volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<volatile T1, const volatile T2, enable> : public compatible<T1, T2, enable> {};
template <typename T1, typename T2, typename enable> struct compatible<const volatile T1, const volatile T2, enable> : public compatible<T1, T2, enable> {};


}


#endif
