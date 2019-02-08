//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_STORAGE_UNION
#define COGS_STORAGE_UNION


#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/gcd.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {



/// @ingroup Mem
/// @brief Placement storage for a union of two types.  
///
/// The value is not intended to be shared between the two types,
/// they would be used at different times.
/// Otherwise, type punning would be an issue.
/// @tparam T1 First type in union
/// @tparam T2 Second type in union
template <typename T1, typename T2>
class storage_union
{
public:
	static const size_t common_alignment = const_lcm<std::alignment_of<T1>::value, std::alignment_of<T2>::value>::value;
	static const size_t greater_size = (sizeof(T1) > sizeof(T2)) ? sizeof(T1) : sizeof(T2);
	
	typedef typename placement_storage<greater_size, common_alignment>::placment_t placement_t;

	placement_t m_contents;

	T1& get_first() { return m_contents.get<T1>(); }
	const T1& get_first() const { return m_contents.get<T1>(); }
	volatile T1& get_first() volatile { return m_contents.get<T1>(); }
	const volatile T1& get_first()const volatile { return m_contents.get<T1>(); }

	T2& get_second() { return m_contents.get<T2>(); }
	const T2& get_second() const { return m_contents.get<T2>(); }
	volatile T2& get_second() volatile { return m_contents.get<T2>(); }
	const volatile T2& get_second()const volatile { return m_contents.get<T2>(); }
};


}


#endif
