//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_RAW_POINTER_TYPE
#define COGS_HEADER_MEM_IS_RAW_POINTER_TYPE


#include <type_traits>


namespace cogs {

template <typename T> struct is_raw_pointer_type : public std::false_type {};
template <typename T> struct is_raw_pointer_type<const T> : public is_raw_pointer_type<T> {};
template <typename T> struct is_raw_pointer_type<volatile T> : public is_raw_pointer_type<T> {};
template <typename T> struct is_raw_pointer_type<const volatile T> : public is_raw_pointer_type<T> {};
template <typename T> static constexpr bool is_raw_pointer_type_v = is_raw_pointer_type<T>::value;

template <typename T> struct is_raw_pointer_type<T*> : public std::true_type {};

template <typename type>
class ptr;

template <typename T> struct is_raw_pointer_type<ptr<T> > : public std::true_type {};

}

#endif
