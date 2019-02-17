//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_COUNT
#define COGS_HEADER_ENV_MEM_BIT_COUNT


#include "cogs/math/bytes_to_int.hpp"
#include "cogs/load.hpp"


namespace cogs {


template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(unsigned short)),
	size_t
>
bit_count(const T& t)
{
	return __popcnt16((unsigned short)load(t));
}


template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned short))
	&& (sizeof(T) <= sizeof(unsigned int)),
	size_t
>
bit_count(const T& t)
{
	return __popcnt((unsigned int)load(t));
}


#ifdef _M_X64

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned int))
	&& (sizeof(T) <= sizeof(unsigned __int64)),
	size_t
>
bit_count(const T& t)
{
	return __popcnt64((unsigned __int64)load(t));
}


template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned __int64)),
	size_t
>
bit_count(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<std::remove_volatile_t<T> > tmp = (std::make_unsigned_t<std::remove_volatile_t<T> >)load(t);
	while (!!tmp)
	{
		unsigned __int64 ll = (unsigned __int64)tmp;
		result += bit_count(ll);
		tmp >>= (sizeof(unsigned __int64) * 8);
	}
	return result;
}

#else

template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned int)),
	size_t
>
bit_count(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<std::remove_volatile_t<T> > tmp = (std::make_unsigned_t<std::remove_volatile_t<T> >)load(t);
	while (!!tmp)
	{
		unsigned int ll = (unsigned int)tmp;
		result += bit_count(ll);
		tmp >>= (sizeof(unsigned int) * 8);
	}
	return result;
}

#endif

}

#endif

