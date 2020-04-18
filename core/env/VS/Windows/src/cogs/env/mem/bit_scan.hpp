//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_ENV_MEM_BIT_SCAN
#define COGS_HEADER_ENV_MEM_BIT_SCAN

#include <type_traits>

#include "cogs/load.hpp"
#include "cogs/env.hpp"


namespace cogs {


// env/bit_scan.hpp provides env level implementation of bit_scan_forward and bit_scan_reverse.
// Exposes the BSR/BSF instructions

// In VS, use _BitScanReverse/_BitScanForward, _BitScanReverse64/_BitScanForward64, for bit scanning


// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(unsigned long)),
	size_t
>
bit_scan_reverse(const T& t)
{
	unsigned long index;
	bool b = !!_BitScanReverse(&index, (unsigned long)t);
	COGS_ASSERT(b);
	return (size_t)index;
}


#ifdef _M_X64

// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long))
	&& (sizeof(T) <= sizeof(unsigned __int64)),
	size_t
>
bit_scan_reverse(const T& t)
{
	unsigned long index;
	bool b = !!_BitScanReverse64(&index, (unsigned long)load(t));
	COGS_ASSERT(b);
	return (size_t)index;
}

// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned __int64)),
	size_t
>
bit_scan_reverse(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<T> bits2 = (std::make_unsigned_t<T>)load(t);
	unsigned __int64 ll;
	for (;;)
	{
		ll = (unsigned __int64)bits2;
		bits2 >>= (sizeof(unsigned __int64) * 8);
		if (!bits2)
			break;
		result += (sizeof(unsigned __int64) * 8);
	}
	return result + bit_scan_reverse(ll);
}


#else

// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long)),
	size_t
>
bit_scan_reverse(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<T> bits2 = (std::make_unsigned_t<T>)load(t);
	unsigned long ll;
	for (;;)
	{
		ll = (unsigned long)bits2;
		bits2 >>= (sizeof(unsigned long) * 8);
		if (!bits2)
			break;
		result += (sizeof(unsigned long) * 8);
	}
	return result + bit_scan_reverse(ll);
}

#endif


// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) <= sizeof(unsigned long)),
	size_t
>
bit_scan_forward(const T& t)
{
	unsigned long index;
	bool b = !!_BitScanForward(&index, (unsigned long)load(t));
	COGS_ASSERT(b);
	return (size_t)index;
}


#ifdef _M_X64

// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long))
	&& (sizeof(T) <= sizeof(unsigned __int64)),
	size_t
>
bit_scan_forward(const T& t)
{
	unsigned long index;
	bool b = !!_BitScanForward64(&index, (unsigned long)load(t));
	COGS_ASSERT(b);
	return (size_t)index;
}

// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned __int64)),
	size_t
>
bit_scan_forward(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<T> bits2 = (std::make_unsigned_t<T>)load(t);
	unsigned __int64 ll;
	for (;;)
	{
		ll = (unsigned __int64)bits2;
		if (!!ll)
			break;

		bits2 >>= (sizeof(__int64) * 8);
		result += (sizeof(__int64) * 8);
	}
	return result + bit_scan_forward(ll);
}


#else


// t must not be zero
template <typename T>
inline std::enable_if_t<
	std::is_integral_v<T>
	&& (sizeof(T) > sizeof(unsigned long)),
	size_t
>
bit_scan_forward(const T& t)
{
	size_t result = 0;
	std::make_unsigned_t<T> bits2 = (std::make_unsigned_t<T>)load(t);
	unsigned long ll;
	for (;;)
	{
		ll = (unsigned long)bits2;
		if (!!ll)
			break;

		bits2 >>= (sizeof(unsigned long) * 8);
		result += (sizeof(unsigned long) * 8);
	}
	return result + bit_scan_forward(ll);
}


#endif


}


#endif
