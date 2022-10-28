//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_FLAG_ENUM
#define COGS_HEADER_MEM_FLAG_ENUM


#include <type_traits>
#include "cogs/operators.hpp"


/// @brief Root namespace for cogs library
namespace cogs {


template <typename T> struct is_flag_enum : public std::false_type { };
template <typename T> static constexpr bool is_flag_enum_v = is_flag_enum<T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename T> struct is_flag_enum<const T> : public is_flag_enum<T> { };
template <typename T> struct is_flag_enum<volatile T> : public is_flag_enum<T> { };
template <typename T> struct is_flag_enum<const volatile T> : public is_flag_enum<T> { };

template <typename M, typename T> struct is_flag_mask_of : public std::false_type { };
template <typename M, typename T> static constexpr bool is_flag_mask_of_v = is_flag_mask_of<M, T>::value;

// By default, map const and/or volatile to the version with no CV qualifier
template <typename M, typename T> struct is_flag_mask_of<const M, T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<volatile M, T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const volatile M, T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const M, const T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<volatile M, const T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const volatile M, const T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const M, volatile T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<volatile M, volatile T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const volatile M, volatile T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const M, const volatile T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<volatile M, const volatile T> : public is_flag_mask_of<M, T> { };
template <typename M, typename T> struct is_flag_mask_of<const volatile M, const volatile T> : public is_flag_mask_of<M, T> { };

template <typename T>
struct is_flag_mask
{
private:
	template <typename U> static uint16_t test(decltype(U::none)*);
	template <typename U> static uint8_t test(...);

public:
	static constexpr bool value = sizeof(test<T>(0)) == sizeof(uint16_t);
};

template <typename T> static constexpr bool is_flag_mask_v = is_flag_mask<T>::value;

template <typename T, typename = std::enable_if_t<is_flag_enum_v<T>>>
struct flag_mask
{
	typedef std::remove_cv_t<T> flag_t;
	typedef std::underlying_type_t<std::remove_cv_t<T>> int_t;

	enum class type : int_t
	{
		none = 0
	};

	class iterator;

	friend iterator begin(const type& t) { iterator result(t); return result; }

	friend iterator end(const type&) { iterator result; return result; }
};

template <typename T> using flag_mask_t = typename flag_mask<T>::type;

template <typename T> struct is_flag_mask_of<flag_mask_t<T>, T> : public std::true_type { };
template <typename T> struct is_flag_mask_of<flag_mask_t<const T>, T> : public std::true_type { };
template <typename T> struct is_flag_mask_of<flag_mask_t<volatile T>, T> : public std::true_type { };
template <typename T> struct is_flag_mask_of<flag_mask_t<const volatile T>, T> : public std::true_type { };


template <typename T, typename = std::enable_if_t<is_flag_mask_v<T>>>
inline size_t count_flags(const T& t)
{
	return bit_count(static_cast<std::underlying_type_t<T>>(load(t)));
}

// t must not be none
template <typename T, typename = std::enable_if_t<is_flag_mask_v<T>>>
inline size_t get_first_flag_index(const T& t)
{
	std::underlying_type_t<T> x = static_cast<std::underlying_type_t<T>>(load(t));
	COGS_ASSERT(!!x);
	return bit_scan_forward(x);
}

// t must not be none
template <typename T, typename = std::enable_if_t<is_flag_mask_v<T>>>
inline typename T::type get_first_flag(const T& t)
{
	return static_cast<typename T::type>(1 << get_first_flag_index(t));
}

// t must not be none
template <typename T, typename = std::enable_if_t<is_flag_mask_v<T>>>
inline size_t get_last_flag_index(const T& t)
{
	std::underlying_type_t<T> x = static_cast<std::underlying_type_t<T>>(load(t));
	COGS_ASSERT(!!x);
	return bit_scan_reverse(x);
}

// t must not be none
template <typename T, typename = std::enable_if_t<is_flag_mask_v<T>>>
inline typename T::type get_last_flag(const T& t)
{
	return static_cast<typename T::type>(1 << get_last_flag_index(t));
}


template <typename T, typename = std::enable_if_t<is_flag_enum_v<T>>>
inline size_t get_flag_index(const T& t)
{
	return bit_scan_forward(static_cast<std::underlying_type_t<T>>(load(t)));
}

template <typename T, typename = std::enable_if_t<is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> maskify(const T& t)
{
	return static_cast<cogs::flag_mask_t<T>>(static_cast<std::underlying_type_t<T>>(t));
}

// Hijack the dereference operator for maskify
template <typename T, typename = std::enable_if_t<is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator*(const T& t)
{
	return maskify(t);
}


}


// operator| - add flag
template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator|(const T& t1, const T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator|(const volatile T& t1, const T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator|(const T& t1, const volatile T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator|(const volatile T& t1, const volatile T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const volatile M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const volatile M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const T& t1, const M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const volatile T& t1, const M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const T& t1, const volatile M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator|(const volatile T& t1, const volatile M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator|(const T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator|(const T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator|(const volatile T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator|(const volatile T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator|=(M& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator|=(volatile M& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator|=(M& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator|=(volatile M& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator|=(T& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator|=(volatile T& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator|=(T& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator|=(volatile T& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


// operator+ - add flag.  Same as operator|
template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator+(const T& t1, const T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator+(const volatile T& t1, const T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator+(const T& t1, const volatile T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T>>>
inline constexpr cogs::flag_mask_t<T> operator+(const volatile T& t1, const volatile T& t2)
{ return static_cast<cogs::flag_mask_t<T>>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const volatile M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const volatile M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const T& t1, const M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const volatile T& t1, const M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const T& t1, const volatile M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator+(const volatile T& t1, const volatile M& t2)
{ return static_cast<M>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator+(const T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator+(const T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator+(const volatile T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator+(const volatile T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_or(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator+=(M& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator+=(volatile M& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator+=(M& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator+=(volatile M& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator+=(T& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator+=(volatile T& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator+=(T& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator+=(volatile T& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_or(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


// operator- - remove flag

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator-(const M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), ~static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator-(const volatile M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), ~static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator-(const M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator-(const volatile M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator-(const T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), ~static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator-(const T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator-(const volatile T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), ~static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator-(const volatile T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator-=(M& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator-=(volatile M& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator-=(M& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator-=(volatile M& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator-=(T& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator-=(volatile T& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator-=(T& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*) & t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(load(t2)));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator-=(volatile T& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, ~static_cast<std::underlying_type_t<T>>(load(t2)));
}



// operator^ - toggle flag
template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator^(const M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator^(const volatile M& t1, const T& t2)
{ return static_cast<M>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator^(const M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename M, typename T, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr M operator^(const volatile M& t1, const volatile T& t2)
{ return static_cast<M>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator^(const T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator^(const T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator^(const volatile T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator^(const volatile T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_xor(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator^=(M& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator^=(volatile M& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator^=(M& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator^=(volatile M& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator^=(T& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, ~static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator^=(volatile T& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, ~static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator^=(T& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*) & t1;
	cogs::assign_bit_xor(*p, ~static_cast<std::underlying_type_t<T>>(load(t2)));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator^=(volatile T& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_xor(*p, ~static_cast<std::underlying_type_t<T>>(load(t2)));
}



// operator& - Mask flag
template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const M& t1, const T& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2)) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const volatile M& t1, const T& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2)) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const M& t1, const volatile T& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2))) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const volatile M& t1, const volatile T& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2))) != 0; }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const T& t1, const M& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2)) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const volatile T& t1, const M& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2)) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const T& t1, const volatile M& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2))) != 0; }

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline constexpr bool operator&(const volatile T& t1, const volatile M& t2)
{ return cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2))) != 0; }


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator&(const T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator&(const T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(t1), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator&(const volatile T& t1, const T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(t2))); }

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline constexpr T operator&(const volatile T& t1, const volatile T& t2)
{ return static_cast<T>(cogs::bit_and(static_cast<std::underlying_type_t<T>>(cogs::load(t1)), static_cast<std::underlying_type_t<T>>(cogs::load(t2)))); }


template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator&=(M& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator&=(volatile M& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline M& operator&=(M& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename M, typename = std::enable_if_t<cogs::is_flag_enum_v<T> && cogs::is_flag_mask_of_v<M, T>>>
inline void operator&=(volatile M& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator&=(T& t1, const T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(t2));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator&=(volatile T& t1, const T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(t2));
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline T& operator&=(T& t1, const volatile T& t2)
{
	std::underlying_type_t<T>* p = (std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
	return t1;
}

template <typename T, typename = std::enable_if_t<cogs::is_flag_mask_v<T>>>
inline void operator&=(volatile T& t1, const volatile T& t2)
{
	volatile std::underlying_type_t<T>* p = (volatile std::underlying_type_t<T>*)&t1;
	cogs::assign_bit_and(*p, static_cast<std::underlying_type_t<T>>(cogs::load(t2)));
}


// operator!
template <typename M, typename = std::enable_if_t<cogs::is_flag_mask_v<M>>>
inline constexpr bool operator!(const M& t)
{
	return t == M::none;
}

template <typename M, typename = std::enable_if_t<cogs::is_flag_mask_v<M>>>
inline constexpr bool operator!(const volatile M& t)
{
	return cogs::load(t) == M::none;
}


// operator==
template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator==(const T1& t1, const T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(t1) == static_cast<std::underlying_type_t<T2>>(t2);
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator==(const T1& t1, const volatile T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(t1) == static_cast<std::underlying_type_t<T2>>(cogs::load(t2));
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator==(const volatile T1& t1, const T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(cogs::load(t1)) == static_cast<std::underlying_type_t<T2>>(t2);
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator==(const volatile T1& t1, const volatile T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(cogs::load(t1)) == static_cast<std::underlying_type_t<T2>>(cogs::load(t2));
}


// operator!=
template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator!=(const T1& t1, const T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(t1) != static_cast<std::underlying_type_t<T2>>(t2);
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator!=(const T1& t1, const volatile T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(t1) != static_cast<std::underlying_type_t<T2>>(cogs::load(t2));
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator!=(const volatile T1& t1, const T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(cogs::load(t1)) != static_cast<std::underlying_type_t<T2>>(t2);
}

template <typename T1, typename T2, typename = std::enable_if_t<(cogs::is_flag_enum_v<T1> || cogs::is_flag_mask_v<T1>) && (cogs::is_flag_enum_v<T2> || cogs::is_flag_mask_v<T2>)>>
inline constexpr bool operator!=(const volatile T1& t1, const volatile T2& t2)
{
	return static_cast<std::underlying_type_t<T1>>(cogs::load(t1)) != static_cast<std::underlying_type_t<T2>>(cogs::load(t2));
}


namespace cogs {


// Modifying a mask invalidates iterators.
// Also, modifying a mask leads to undefined behavior for range-based for loops.
template <typename T, typename enable>
class flag_mask<T, enable>::iterator
{
private:
	type m_remaining;
	flag_t m_flag;

public:
	iterator()
		: m_remaining(type::none)
	{ }

	iterator(const type& m)
		: m_remaining(m)
	{
		if (m_remaining != type::none)
			m_flag = static_cast<flag_t>(1 << get_first_flag_index(m_remaining));
	}

	iterator(const volatile type& m)
		: iterator(load(m))
	{ }

	iterator(const iterator& src)
		: m_remaining(src.m_remaining),
		m_flag(src.m_flag)
	{ }

	iterator& operator=(const type& m)
	{
		m_remaining = *m;
		m_flag = static_cast<flag_t>(1 << get_first_flag_index(m_remaining));
	}

	iterator& operator=(const volatile type& m)
	{
		m_remaining = *load(m);
		m_flag = static_cast<flag_t>(1 << get_first_flag_index(m_remaining));
	}

	iterator& operator=(const iterator& src)
	{
		m_remaining = src.m_remaining;
		m_flag = src.m_flag;
	}

	bool operator==(const iterator& src) const
	{
		if (m_remaining != src.m_remaining)
			return false;
		if (m_remaining == type::none)
			return true;
		return m_flag == src.m_flag;
	}

	bool operator!=(const iterator& src) const { return !operator==(src); }

	bool operator!() const { return !m_remaining; }

	size_t get_flag_index() const { return get_first_flag_index(m_flag); }

	flag_t operator*() const { return m_flag; }

	iterator& operator++()
	{
		if (m_remaining != type::none)
		{
			m_remaining ^= m_flag;
			if (m_remaining != type::none)
				m_flag = static_cast<flag_t>(1 << get_first_flag_index(m_remaining));
		}
		return *this;
	}

	iterator operator++(int)
	{
		iterator result = *this;
		++*this;
		return result;
	}
};


}

#endif

