//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_LEAST_MULTIPLE_OF
#define COGS_LEAST_MULTIPLE_OF


namespace cogs {


/// @ingroup ConstMath
/// @brief Meta template to compute the least multiple of 'multiple_of', that is n or larger.
/// @tparam n Constant value to use as a minimum
/// @tparam multiple_of Constant value to get a multiple of.
template <size_t n, size_t multiple_of>
class least_multiple_of
{
public:
	static constexpr size_t value =	((n % multiple_of) == 0) ? n
							:	((n < multiple_of) ? multiple_of
							:	(n-1) + (multiple_of - ((n-1) % multiple_of)));
};


}


#endif

