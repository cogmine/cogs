//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_POINTER_TYPE
#define COGS_HEADER_MEM_IS_POINTER_TYPE


#include "cogs/mem/is_raw_pointer_type.hpp"
#include "cogs/mem/is_rc_pointer_type.hpp"


namespace cogs {

template <typename T> struct is_pointer_type : public std::conditional_t<is_raw_pointer_type_v<T> || is_rc_pointer_type_v<T>, std::true_type, std::false_type> {};
template <typename T> static constexpr bool is_pointer_type_v = is_pointer_type<T>::value;

}

#endif
