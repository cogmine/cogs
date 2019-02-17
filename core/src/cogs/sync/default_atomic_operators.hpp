//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_HEADER_ARCH_SYNC_ATOMIC_OPERATORS
#ifndef COGS_HEADER_SYNC_DEFAULT_ATOMIC_OPERATORS
#define COGS_HEADER_SYNC_DEFAULT_ATOMIC_OPERATORS

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/arch/sync/atomic.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"
#include "cogs/sync/atomic_load.hpp"
#include "cogs/sync/atomic_store.hpp"
#include "cogs/sync/atomic_exchange.hpp"
#include "cogs/sync/atomic_compare_exchange.hpp"

namespace cogs {
namespace atomic {
namespace defaults {


#define COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname, condition)\
template <typename T>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	void>\
assign_ ## fname(T& t)\
{ atomic::compare_exchange_retry_loop(t, [](const std::remove_volatile_t<T>& t2) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp); return tmp; }); }\
template <typename T>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	std::remove_volatile_t<T> >\
pre_assign_ ## fname(T& t)\
{ return atomic::compare_exchange_retry_loop_pre(t, [](const std::remove_volatile_t<T>& t2) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp); return tmp; }); }\
template <typename T>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	std::remove_volatile_t<T> >\
post_assign_ ## fname(T& t)\
{ return atomic::compare_exchange_retry_loop_post(t, [](const std::remove_volatile_t<T>& t2) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp); return tmp; }); }\


#define COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname, std::is_arithmetic_v<T>)

#define COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname, std::is_integral_v<T>)

#define COGS_DEFINE_VOLATILE_POINTER_DEFAULT_UNARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname, std::is_pointer_v<T>)

#define COGS_DEFINE_VOLATILE_POINTER_DEFAULT_UNARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_UNARY_ASSIGN_OPERATORS(fname, std::is_pointer_v<T>)

#define COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_BINARY_ASSIGN_OPERATORS(fname, condition)\
template <typename T, typename A1>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	void>\
assign_ ## fname(T& t, const A1& a)\
{ atomic::compare_exchange_retry_loop(t, [](const std::remove_volatile_t<T>& t2, const A1& a) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp, a); return tmp; }); }\
template <typename T, typename A1>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	std::remove_volatile_t<T> >\
pre_assign_ ## fname(T& t, const A1& a)\
{ return atomic::compare_exchange_retry_loop_pre(t, [](const std::remove_volatile_t<T>& t2, const A1& a) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp, a); return tmp; }); }\
template <typename T, typename A1>\
inline std::enable_if_t<\
	std::is_scalar_v<T>\
	&& can_atomic_v<T>\
	&& (condition)\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	std::remove_volatile_t<T> >\
post_assign_ ## fname(T& t, const A1& a)\
{ return atomic::compare_exchange_retry_loop_post(t, [](const std::remove_volatile_t<T>& t2, const A1& a) { const std::remove_volatile_t<T> tmp(t2); assign_ ## fname(tmp, a); return tmp; }); }\

#define COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_BINARY_ASSIGN_OPERATORS(fname, std::is_arithmetic_v<T>)

#define COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_BINARY_ASSIGN_OPERATORS(fname, std::is_integral_v<T>)

#define COGS_DEFINE_VOLATILE_POINTER_DEFAULT_BINARY_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_VOLATILE_SCALAR_CONDITIONAL_DEFAULT_BINARY_ASSIGN_OPERATORS(fname, std::is_pointer_v<T>)


COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(not)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(abs)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(negative)

COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_UNARY_ASSIGN_OPERATORS(bit_not)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_UNARY_ASSIGN_OPERATORS(bit_count)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_UNARY_ASSIGN_OPERATORS(bit_scan_forward)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_UNARY_ASSIGN_OPERATORS(bit_scan_reverse)

COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_rotate_right)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_rotate_left)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_shift_right)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_shift_left)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(endian_swap)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(next)	// increment
COGS_DEFINE_VOLATILE_POINTER_DEFAULT_UNARY_ASSIGN_OPERATORS(next)		// increment
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(prev)	// decrement
COGS_DEFINE_VOLATILE_POINTER_DEFAULT_UNARY_ASSIGN_OPERATORS(prev)		// decrement

COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_and)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_or)
COGS_DEFINE_VOLATILE_INTEGRAL_DEFAULT_BINARY_ASSIGN_OPERATORS(bit_xor)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(add)
COGS_DEFINE_VOLATILE_POINTER_DEFAULT_BINARY_ASSIGN_OPERATORS(add)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(subtract)
COGS_DEFINE_VOLATILE_POINTER_DEFAULT_BINARY_ASSIGN_OPERATORS(subtract)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(inverse_subtract)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(multiply)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(modulo)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(inverse_modulo)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(divide)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(inverse_divide)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(reciprocal)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(divide_whole)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_BINARY_ASSIGN_OPERATORS(inverse_divide_whole)

COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(gcd)
COGS_DEFINE_VOLATILE_ARITHMETIC_DEFAULT_UNARY_ASSIGN_OPERATORS(lcm)


#define COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATOR(fname)\
template <typename T, typename... args_t>\
inline std::enable_if_t<\
	can_atomic_v<T>\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	decltype(os::atomic::fname(std::declval<T&>(), std::declval<args_t>()...))>\
fname(T& t, args_t&&... a)\
{ return os::atomic::fname(t, std::forward<args_t>(a)...); }

#define COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATOR(fname)\
template <typename T, typename... args_t>\
inline std::enable_if_t<\
	can_atomic_v<T>\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	decltype(arch::atomic::fname(std::declval<T&>(), std::declval<args_t>()...))>\
fname(T& t, args_t&&... a)\
{ return arch::atomic::fname(t, std::forward<args_t>(a)...); }

#define COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATOR(fname)\
template <typename T, typename... args_t>\
inline std::enable_if_t<\
	can_atomic_v<T>\
	&& std::is_volatile_v<T>\
	&& !std::is_const_v<T>,\
	decltype(cogs::atomic::defaults::fname(std::declval<T&>(), std::declval<args_t>()...))>\
fname(T& t, args_t&&... a)\
{ return cogs::atomic::defaults::fname(t, std::forward<args_t>(a)...); }



#define COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATOR(assign_ ## fname)\
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATOR(pre_assign_ ## fname)\
COGS_DEFINE_ENV_DEFAULT_VOLATILE_ASSIGN_OPERATOR(post_assign_ ## fname)

#define COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATOR(assign_ ## fname)\
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATOR(pre_assign_ ## fname)\
COGS_DEFINE_OS_DEFAULT_VOLATILE_ASSIGN_OPERATOR(post_assign_ ## fname)

#define COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATORS(fname)\
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATOR(assign_ ## fname)\
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATOR(pre_assign_ ## fname)\
COGS_DEFINE_ARCH_DEFAULT_VOLATILE_ASSIGN_OPERATOR(post_assign_ ## fname)


}
}
}


#endif
#endif

#include "cogs/arch/sync/atomic_operators.hpp"


