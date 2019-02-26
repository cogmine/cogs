//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_NEXT_MULTIPLE_OF
#define COGS_HEADER_MATH_NEXT_MULTIPLE_OF


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the next multiple of 'multiple_of', that is larger than n.
/// @tparam n Constant value to use as a minimum
/// @tparam multiple_of Constant value to get a multiple of.
template <size_t n, size_t multiple_of>
class next_multiple_of
{
public:
	static constexpr size_t value =	((n % multiple_of) == 0) ? n
							:	((n < multiple_of) ? multiple_of
							:	n + (multiple_of - (n % multiple_of)));
};
template <size_t n, size_t multiple_of>
inline constexpr size_t next_multiple_of_v = next_multiple_of<n, multiple_of>::value;


}


#endif
