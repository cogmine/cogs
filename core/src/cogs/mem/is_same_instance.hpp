//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_IS_SAME_INSTANCE
#define COGS_HEADER_MEM_IS_SAME_INSTANCE


#include <memory>
#include <type_traits>


namespace cogs {


template <typename T1, typename T2>
constexpr bool is_same_instance(T1&& t1, T2&& t2)
{
	if constexpr (std::is_lvalue_reference_v<T1> && std::is_lvalue_reference_v<T2>)
	{
		typedef std::remove_reference_t<T1> U1;
		typedef std::remove_reference_t<T2> U2;

		// It's caller error to mix use of volatile and non-volatile references to the same object.
		if (std::is_volatile_v<U1> == std::is_volatile_v<U2>)
		{
			typedef std::remove_cv_t<U1> V1;
			typedef std::remove_cv_t<U2> V2;

			if constexpr (std::is_same_v<V1, V2>)
				return std::addressof(t1) == std::addressof(t2);

			if constexpr (std::is_convertible_v<V2*, V1*>)
			{
				V1* v1 = (V1*)std::addressof(t2);
				return (V1*)std::addressof(t1) == v1;
			}

			if constexpr (std::is_convertible_v<V1*, V2*>)
			{
				V2* v2 = (V2*)std::addressof(t1);
				return v2 == (V2*)std::addressof(t2);
			}
		}
	}
	return false;
}


}

#endif
