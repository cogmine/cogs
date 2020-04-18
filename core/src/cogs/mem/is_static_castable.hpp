//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_STATIC_CASTABLE
#define COGS_HEADER_MEM_IS_STATIC_CASTABLE


#include <type_traits>


namespace cogs {


template <typename T1, typename T2, typename = void>
struct is_static_castable : std::false_type { };

template <typename T1, typename T2>
struct is_static_castable<T1, T2, std::void_t<decltype(static_cast<T2>(std::declval<T1>()))> > : std::true_type { };

template <typename T1, typename T2>
static constexpr bool is_static_castable_v = is_static_castable<T1, T2>::value;


}

#endif
