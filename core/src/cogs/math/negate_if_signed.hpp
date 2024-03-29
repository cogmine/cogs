//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_NEGATE_IF_SIGNED
#define COGS_HEADER_MATH_NEGATE_IF_SIGNED


#include <type_traits>


namespace cogs {


/// @ingroup Math
/// @brief Template helper to negate a value, if it's signed.
/// @tparam int_t Unsigned int type
template <typename int_t>
class negate_if_signed
{
private:
	static constexpr bool has_sign = ((int_t)~(int_t)0) < (int_t)0;

	class unsigned_getter
	{
	public:
		template <typename int_t2>
		static int_t get(const int_t2& i)
		{
			return i;
		}
	};

	class signed_getter
	{
	public:
		template <typename int_t2>
		static int_t get(const int_t2& i)
		{
			return -i;
		}
	};

public:
	static int_t get(const int_t& i) { return std::conditional_t<has_sign, signed_getter, unsigned_getter>::get(i); }
};


}


#endif
