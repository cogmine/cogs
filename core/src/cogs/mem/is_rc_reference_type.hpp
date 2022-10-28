//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_RC_REFERENCE_TYPE
#define COGS_HEADER_MEM_IS_RC_REFERENCE_TYPE


#include <type_traits>


namespace cogs {

template <typename T> struct is_rc_reference_type : public std::false_type {};
template <typename T> struct is_rc_reference_type<const T> : public is_rc_reference_type<T> {};
template <typename T> struct is_rc_reference_type<volatile T> : public is_rc_reference_type<T> {};
template <typename T> struct is_rc_reference_type<const volatile T> : public is_rc_reference_type<T> {};
template <typename T> static constexpr bool is_rc_reference_type_v = is_rc_reference_type<T>::value;

template <typename type>
class rcref;

template <typename T> struct is_rc_reference_type<rcref<T> > : public std::true_type {};

}

#endif
