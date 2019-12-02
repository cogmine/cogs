//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_REFERENCE_TYPE
#define COGS_HEADER_MEM_IS_REFERENCE_TYPE


#include "cogs/mem/is_raw_reference_type.hpp"
#include "cogs/mem/is_rc_reference_type.hpp"


namespace cogs {

template <typename T> struct is_reference_type : public std::conditional_t<is_raw_reference_type_v<T> || is_rc_reference_type_v<T>,  std::true_type, std::false_type> {};
template <typename T> static constexpr bool is_reference_type_v = is_reference_type<T>::value;

}

#endif
