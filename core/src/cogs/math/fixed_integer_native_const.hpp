//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXED_INTEGER_NATIVE_CONST
#define COGS_HEADER_MATH_FIXED_INTEGER_NATIVE_CONST

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/env.hpp"
#include "cogs/env/math/umul.hpp"
#include "cogs/math/extumul.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/math/is_integral.hpp"
#include "cogs/math/is_arithmetic.hpp"
#include "cogs/math/is_signed.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/value_to_bits.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_bits.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified
#pragma warning (disable: 4307)	// multiple copy constructors specified


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
class fixed_integer_native_const;

template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;


// forward declares

class dynamic_integer;
class dynamic_integer_content;

template <bool has_sign, size_t bits>
class fixed_integer_native;

template <bool has_sign, size_t bits>
class fixed_integer_extended;

class dynamic_integer;

template <typename T>
class string_t;


namespace io
{
	class buffer;
}

template <typename storage_type, typename unit_base>
class measurement;

template <typename T1, typename T2>
class const_compared
{
};


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value> class is_arithmetic<fixed_integer_native_const<has_sign, bits, value> > : public std::true_type { };

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value> class is_integral<fixed_integer_native_const<has_sign, bits, value> > : public std::true_type { };

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value> class is_signed<fixed_integer_native_const<has_sign, bits, value> > { public: static constexpr bool value = has_sign; };

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value> class is_const_type<fixed_integer_native_const<has_sign, bits, value> > : public std::true_type { };


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
class int_to_fixed_integer<fixed_integer_native_const<has_sign, bits, value> >
{
public:
	typedef fixed_integer_native_const<has_sign, bits, value> type;
};

template <bool has_sign_in, size_t bits_in, bits_to_int_t<bits_in, has_sign_in> value>
class fixed_integer_native_const
{
public:
	typedef fixed_integer_native_const<has_sign_in, bits_in, value> this_t;

	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = bits_in;
	typedef bits_to_int_t<bits, has_sign> int_t;
	static constexpr size_t n_digits = 1;

	static constexpr size_t bits_used = sizeof(int_t) * 8;
	typedef fixed_integer_native_const<false, const_bit_scan_reverse_v<bits_used> +1, bits_used> bits_used_t;

	static constexpr bool is_const_negative = !has_sign ? false : (value < 0);
	static constexpr bool is_const_exponent_of_two = is_const_negative
		? ((-(std::make_signed_t<int_t>)value & (~-(std::make_signed_t<int_t>)value + 1)) == -(std::make_signed_t<int_t>)value)
		: ((value & (~value + 1)) == value);

	typedef fixed_integer_extended_const<is_const_negative, bits, (ulongest)value> as_extended_t;

	static constexpr bool is_const_zero = !value;


private:
	static constexpr int_t low_digit = value;
	static constexpr bool is_negative_one = !has_sign ? false : (value == -1);
	static constexpr size_t reduced_bits = is_const_negative ? range_to_bits<(longest)value, 0>::value : range_to_bits<0, (ulongest)value>::value;
	typedef bits_to_int_t<reduced_bits, is_const_negative> reduced_int_t;
	static constexpr reduced_int_t reduced_value = (reduced_int_t)value;

public:
	typedef fixed_integer_native_const<is_const_negative, reduced_bits, reduced_value> reduced_t;

private:


	template <typename std::make_unsigned<int_t>::type scan_value>
	class bit_scan_forward_helper
	{
	public:
		static constexpr size_t value = ((scan_value & 1) == 1) ? 0 : (1 + bit_scan_forward_helper<(scan_value >> 1)>::value);
	};

	template <> class bit_scan_forward_helper<0> { public: static constexpr size_t value = 0; };
	template <> class bit_scan_forward_helper<1> { public: static constexpr size_t value = 0; };

	template <typename std::make_unsigned<int_t>::type scan_value>
	class bit_scan_reverse_helper
	{
	public:
		static constexpr size_t value = 1 + bit_scan_reverse_helper<(scan_value >> 1)>::value;
	};

	template <> class bit_scan_reverse_helper<0> { public: static constexpr size_t value = 0; };
	template <> class bit_scan_reverse_helper<1> { public: static constexpr size_t value = 0; };

	template <typename std::make_unsigned<int_t>::type count_value>
	class bit_count_helper
	{
	public:
		static constexpr size_t value = ((value & 1) ? 1 : 0) + bit_count_helper<(count_value >> 1)>::value;
	};

	template <>
	class bit_count_helper<0> { public: static constexpr size_t value = 0; };


public:
	static constexpr reduced_int_t int_value = (reduced_int_t)value;

	typedef fixed_integer_native<is_const_negative, (reduced_bits == 0 ? 1 : reduced_bits)> non_const_t;

	static constexpr size_t const_bit_scan_forward = bit_scan_forward_helper<(typename std::make_unsigned<int_t>::type)value>::value;
	static constexpr size_t const_bit_scan_reverse = bit_scan_reverse_helper<(typename std::make_unsigned<int_t>::type)value>::value;
	static constexpr size_t const_bit_count = bit_count_helper<(typename std::make_unsigned<int_t>::type)value>::value;

	fixed_integer_native_const() { }
	
	fixed_integer_native_const(const this_t&) { }
	fixed_integer_native_const(const volatile this_t&) { }

	this_t& operator=(const volatile this_t&) { return *this; }
	volatile this_t& operator=(const volatile this_t&) volatile { return *this; }

	constexpr reduced_int_t get_int() const volatile { return int_value; }
	constexpr reduced_int_t simplify_type() const volatile { return int_value; }

	constexpr bool operator!() const volatile { return is_const_zero; }

	auto operator~() const volatile
	{
		typename fixed_integer_native_const<has_sign, bits, ~value>::reduced_t tmp;
		return tmp;
	}

	constexpr bool is_negative() const volatile { return is_const_negative; }

	constexpr bool is_exponent_of_two() const volatile { return is_const_exponent_of_two; }
	constexpr bool has_fractional_part() const volatile { return false; }

	// abs
	auto abs() const volatile
	{
		std::conditional_t<is_const_negative,
			decltype(-std::declval<reduced_t>()),
			reduced_t
		> tmp;
		return tmp;
	}

	auto operator-() const volatile
	{
		// since result may require another digit, leverage extended
		typename as_extended_t::negative_t tmp;
		return tmp;
	}


	constexpr size_t bit_count() const volatile { return const_bit_count; }

	constexpr size_t bit_scan_forward() const volatile { return const_bit_scan_forward; }

	constexpr size_t bit_scan_reverse() const volatile { return const_bit_scan_reverse; }

	auto next() const volatile { return *this + one_t(); }
	auto prev() const volatile { return *this - one_t(); }


	// bit_rotate_right
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, bits,
			((value1 >> value2) | (value1 << ((sizeof(int_t) * 8) - value2)))>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, bits,
			((value1 >> value2) | (value1 << ((sizeof(int_t) * 8) - value2)))>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return bit_rotate_right(i % bits_used_t());
	}


	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return bit_rotate_right(i % bits_used_t());
	}

	template <bool has_sign2, size_t bits2> auto bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_right(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_right(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_right(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_right(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto bit_rotate_right(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto bit_rotate_right(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }


	// bit_rotate_left
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, bits,
			((value1 << value2) | (value1 >> ((sizeof(int_t) * 8) - value2)))>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, bits,
			((value1 << value2) | (value1 >> ((sizeof(int_t) * 8) - value2)))>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return bit_rotate_left(i % bits_used_t());
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return bit_rotate_left(i % bits_used_t());
	}

	template <bool has_sign2, size_t bits2> auto bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_left(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_left(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_left(src); }
	template <bool has_sign2, size_t bits2> auto bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.bit_rotate_left(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto bit_rotate_left(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto bit_rotate_left(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }


	// bit_shift_right
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, (bits - value2), (value >> value2)>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fixed_integer_native_const<has_sign, (bits - value2), (value >> value2)>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile { return zero_t(); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile { return zero_t(); }

	template <bool has_sign2, size_t bits2> auto operator>>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }
	template <bool has_sign2, size_t bits2> auto operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator>>(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator>>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator>>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }


	// bit_shift_left
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		// since result may require more digits, leverage extended
		as_extended_t tmp;
		return tmp << i;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		// since result may require more digits, leverage extended
		as_extended_t tmp;
		return tmp << i;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile { return zero_t(); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile { return zero_t(); }

	template <bool has_sign2, size_t bits2> auto operator<<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }
	template <bool has_sign2, size_t bits2> auto operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { non_const_t tmp(*this); return tmp.operator<<(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator<<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator<<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }


	// bit_or
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = (bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2);
		static constexpr size_t bits3 = (bits > bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t3;
		static constexpr int_t3 value = (int_t3)value1 | (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = (bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2);
		static constexpr size_t bits3 = (bits > bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t3;
		static constexpr int_t3 value = (int_t3)value1 | (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i | *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i | *this;
	}

	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src | *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }

	// bit_and
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = has_sign && has_sign2;
		static constexpr size_t bits3 = (bits < bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t3;
		static constexpr int_t3 value = (int_t3)value1 & (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = has_sign && has_sign2;
		static constexpr size_t bits3 = (bits < bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t3;
		static constexpr int_t3 value = (int_t3)value1 & (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i & *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i & *this;
	}

	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }

	// bit_xor
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = (bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2);
		static constexpr size_t bits3 = (bits > bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t;
		static constexpr int_t3 value = (int_t3)value ^ (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool has_sign3 = (bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2);
		static constexpr size_t bits3 = (bits > bits2) ? bits : bits2;
		typedef bits_to_int_t<bits3, has_sign3> int_t;
		static constexpr int_t3 value = (int_t3)value ^ (int_t3)value2;

		typename fixed_integer_native_const<has_sign3, bits3, value3>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i ^ *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i ^ *this;
	}

	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src ^ *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }

	// add
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;

		static constexpr size_t new_bits = (bits > bits2 ? bits : bits2) + 1;

		static constexpr ulongest lowDigit = (ulongest)((ulongest)value + (ulongest)value2);
		static constexpr bool overflow = ((ulongest)value + (ulongest)value2) < value;
		static constexpr ulongest highDigit = (is_const_negative == is_const_negative2) ?
			(is_const_negative2 ?
			(overflow ? ~(ulongest)0 : (~(ulongest)0 - 1))
				:
				(overflow ? 1 : 0)
				)
			: (overflow ? 0 : ~(ulongest)0);

		typename fixed_integer_extended_const<((longest)highDigit < 0), new_bits, lowDigit, highDigit>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;

		static constexpr size_t new_bits = (bits > bits2 ? bits : bits2) + 1;

		static constexpr ulongest lowDigit = (ulongest)((ulongest)value + (ulongest)value2);
		static constexpr bool overflow = ((ulongest)value + (ulongest)value2) < value;
		static constexpr ulongest highDigit = (is_const_negative == is_const_negative2) ?
			(is_const_negative2 ?
			(overflow ? ~(ulongest)0 : (~(ulongest)0 - 1))
				:
				(overflow ? 1 : 0)
				)
			: (overflow ? 0 : ~(ulongest)0);

		typename fixed_integer_extended_const<((longest)highDigit < 0), new_bits, lowDigit, highDigit>::reduced_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i + *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i + *this;
	}

	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src + *this; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src + *this; }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src + *this; }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src + *this; }
	auto operator+(const dynamic_integer& src) const volatile { return src.operator+(*this); }
	auto operator+(const volatile dynamic_integer& src) const volatile { return src.operator+(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }

	// subtract
	//template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	//auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	//{
	//	return *this + -i;
	//}


	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this + -i;
	}

	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_subtract(*this); }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_subtract(*this); }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_subtract(*this); }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_subtract(*this); }
	auto operator-(const dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }
	auto operator-(const volatile dynamic_integer& src) const volatile { return src.inverse_subtract(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }

	// inverse_subtract
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i + -*this;
	}

	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	auto inverse_subtract(const dynamic_integer& src) const volatile { return src - *this; }
	auto inverse_subtract(const volatile dynamic_integer& src) const volatile { return src - *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }

	// multiply
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<this_t>().abs()) abs_t1;
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;
		static constexpr ulongest tmp1 = (ulongest)(abs_t1::int_value);
		static constexpr ulongest tmp2 = (ulongest)(abs_t2::int_value);

		static constexpr ulongest lowDigit = const_extumul<tmp1, tmp2>::low_part;
		static constexpr ulongest highDigit = const_extumul<tmp1, tmp2>::high_part;
		typedef typename fixed_integer_extended_const<false, bits + bits2, lowDigit, highDigit>::reduced_t tmp_t;
		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;

		std::conditional_t<negate_result,
			decltype(-std::declval<tmp_t>()),
			tmp_t
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typedef decltype(std::declval<this_t>().abs()) abs_t1;
		typedef decltype(std::declval<fixed_integer_native_const<has_sign2, bits2, value2> >().abs()) abs_t2;
		static constexpr ulongest tmp1 = (ulongest)(abs_t1::int_value);
		static constexpr ulongest tmp2 = (ulongest)(abs_t2::int_value);

		static constexpr ulongest lowDigit = const_extumul<tmp1, tmp2>::low_part;
		static constexpr ulongest highDigit = const_extumul<tmp1, tmp2>::high_part;
		typedef typename fixed_integer_extended_const<false, bits + bits2, lowDigit, highDigit>::reduced_t tmp_t;
		static constexpr bool negate_result = is_const_negative != fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;

		std::conditional_t<negate_result,
			decltype(-std::declval<tmp_t>()),
			tmp_t
		> tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i.operator*(*this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return i.operator*(*this);
	}

	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src * *this; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src * *this; }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src * *this; }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src * *this; }
	
	auto operator*(const dynamic_integer& src) const volatile { return src.operator*(*this); }
	auto operator*(const volatile dynamic_integer& src) const volatile { return src.operator*(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }

	// modulo
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this - (divide_whole(i) * i);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return *this - (divide_whole(i) * i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this - (divide_whole(i) * i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return *this - (divide_whole(i) * i);
	}

	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_modulo(*this); }
	auto operator%(const dynamic_integer& src) const volatile { return src.inverse_modulo(*this); }
	auto operator%(const volatile dynamic_integer& src) const volatile { return src.inverse_modulo(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }

	// inverse_modulo
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		return i - (i.divide_whole(*this) * *this);
	}

	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const volatile { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const volatile { return src % *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }

	// get_ratio
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	auto operator/(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2>
	auto operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2>
	auto operator/(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <bool has_sign2, size_t bits2>
	auto operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	auto operator/(const dynamic_integer& src) const volatile
	{ return fraction<this_t, dynamic_integer>(*this, src); }

	auto operator/(const volatile dynamic_integer& src) const volatile
	{ return fraction<this_t, dynamic_integer>(*this, src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }

	// reciprocal
	auto reciprocal() const volatile { return fraction<one_t, this_t>(); }

	// inverse_divide
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		typename fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		typename fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>::simplified_t tmp;
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile
	{ return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	template <bool has_sign2, size_t bits2>
	auto inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile
	{ return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	auto inverse_divide(const dynamic_integer& src) const volatile
	{ return fraction<dynamic_integer, this_t>(src, *this); }

	auto inverse_divide(const volatile dynamic_integer& src) const volatile
	{ return fraction<dynamic_integer, this_t>(src, *this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }


	// floor
	this_t floor() const volatile { return this_t(); }

	// ceil
	this_t ceil() const volatile { return this_t(); }

	// fractional_part
	auto fractional_part() const volatile { return zero_t(); }

	// divide_whole
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		as_extended_t tmp1;
		return tmp1.divide_whole(i);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		as_extended_t tmp1;
		return tmp1.divide_whole(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.divide_whole(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.divide_whole(i);
	}

	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole(*this); }
	auto divide_whole(const dynamic_integer& src) const volatile { return src.inverse_divide_whole(*this); }
	auto divide_whole(const volatile dynamic_integer& src) const volatile { return src.inverse_divide_whole(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }

	// inverse_divide_whole
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;

		return tmp2.divide_whole(tmp1);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;

		return tmp2.divide_whole(tmp1);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return i.divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return i.divide_whole(tmp);
	}

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const volatile { return src.divide_whole(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }

	// divide_whole_and_modulo
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return std::make_pair(divide_whole(i), operator%(i));
	}

	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }
	auto divide_whole_and_modulo(const dynamic_integer& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile { return src.inverse_divide_whole_and_inverse_modulo(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }

	// gcd
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.gcd(tmp2);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.gcd(tmp2);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.gcd(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.gcd(i);
	}

	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const dynamic_integer& t2) const volatile { return t2.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile dynamic_integer& t2) const volatile { return t2.gcd(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }

	// lcm
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.lcm(tmp2);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.lcm(tmp2);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		as_extended_t tmp;
		return tmp.lcm(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>&) const volatile
	{
		as_extended_t tmp;
		return tmp.lcm(i);
	}

	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const dynamic_integer& t2) const volatile { return t2.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile dynamic_integer& t2) const volatile { return t2.lcm(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }

	// greater
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.greater(tmp2);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.greater(tmp2);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.greater(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.greater(i);
	}

	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const dynamic_integer& t2) const volatile { return t2.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile dynamic_integer& t2) const volatile { return t2.greater(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }


	// lesser
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.lesser(tmp2);
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		as_extended_t tmp1;
		typename fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t tmp2;
		return tmp1.lesser(tmp2);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.lesser(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return tmp.lesser(i);
	}

	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_native<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& t2) const volatile { return t2.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const dynamic_integer& t2) const volatile { return t2.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile dynamic_integer& t2) const volatile { return t2.lesser(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }



	// equals
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) == 0;
	}

	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp == *this; }
	bool operator==(const dynamic_integer& cmp) const volatile { return cmp == *this; }
	bool operator==(const volatile dynamic_integer& cmp) const volatile { return cmp == *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }

	// not_equals
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) != 0;
	}

	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp != *this; }
	bool operator!=(const dynamic_integer& cmp) const volatile { return cmp != *this; }
	bool operator!=(const volatile dynamic_integer& cmp) const volatile { return cmp != *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }

	// is_less_than
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) < 0;
	}

	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp > *this; }
	bool operator<(const dynamic_integer& cmp) const volatile { return cmp > *this; }
	bool operator<(const volatile dynamic_integer& cmp) const volatile { return cmp > *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }

	// is_greater_than
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) > 0;
	}

	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp < *this; }
	bool operator>(const dynamic_integer& cmp) const volatile { return cmp < *this; }
	bool operator>(const volatile dynamic_integer& cmp) const volatile { return cmp < *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }

	// is_less_than_or_equal
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) <= 0;
	}

	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp >= *this; }
	bool operator<=(const dynamic_integer& cmp) const volatile { return cmp >= *this; }
	bool operator<=(const volatile dynamic_integer& cmp) const volatile { return cmp >= *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }

	// is_greater_than_or_equal
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		return compare(i) >= 0;
	}

	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return cmp <= *this; }
	bool operator>=(const dynamic_integer& cmp) const volatile { return cmp <= *this; }
	bool operator>=(const volatile dynamic_integer& cmp) const volatile { return cmp <= *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }

	// compare
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr int compare(const fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		static constexpr bool is_const_negative2 = fixed_integer_native_const<has_sign2, bits2, value2>::is_const_negative;
		static constexpr bool is_same_sign = (is_const_negative == is_const_negative2);
		static constexpr int value = !is_same_sign ? (is_const_negative ? -1 : 1) : ((value < value2) ? -1 : ((value2 < value) ? 1 : 0));
		return value;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	constexpr int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>&) const volatile
	{
		return (value == value2) ? 0 : ((value < value2) ? -1 : 1);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return const_compare(i);
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	constexpr int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& i) const volatile
	{
		as_extended_t tmp;
		return const_compare(i);
	}

	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& cmp) const volatile { return -cmp.compare(*this); }
	int compare(const dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }
	int compare(const volatile dynamic_integer& cmp) const volatile { return -cmp.compare(*this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }


	bool test_bit(size_t i) const volatile { non_const_t tmp(int_value); return tmp.test_bit(i); }

	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const volatile;

	string_t<wchar_t> to_string(int radix = 10, size_t minDigits = 1) const volatile;

	string_t<char> to_cstring(int radix = 10, size_t minDigits = 1) const volatile;

	template <endian_t e>
	io::buffer to_buffer() const volatile;
};


template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class const_compared<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	static constexpr int value = const_compared<fixed_integer_native_const<has_sign1, bits1, value1>::as_extended_t,
		fixed_integer_native_const<has_sign2, bits2, value2>::as_extended_t>::value;
};

template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2, ulongest... values2>
class const_compared<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
public:
	static constexpr int value = const_compared<fixed_integer_native_const<has_sign1, bits1, value1>::as_extended_t,
		fixed_integer_extended_const<has_sign2, bits2, values2...> >::value;
};



template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2),
		(bits1 > bits2) ?
			(bits1 + ((has_sign2 && !has_sign1) ? 1 : 0))
			: (bits2 + ((has_sign1 && !has_sign2) ? 1 : 0))
			> type;
};

template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2, ulongest... values2>
class compatible<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
public:
	typedef typename compatible<fixed_integer_extended_const<has_sign2, bits2, values2...>, fixed_integer_native_const<has_sign1, bits1, value1> >::type type;
};

template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_native<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), bits1 + ((has_sign1 && !has_sign2) ? 1 : 0)> type;
};

template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_native_const<has_sign1, bits1, value1>, fixed_integer_extended<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), (bits1 > bits2 ? bits1 : bits2) + (((has_sign1 != has_sign2) && ((!has_sign1 && (bits1 >= bits2)) || (!has_sign2 && (bits2 >= bits1)))) ? 1 : 0)> type;
};

template <typename int_t2, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<fixed_integer_native_const<has_sign2, bits2, value2>, int_t2, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<fixed_integer_native_const<has_sign2, bits2, value2>, int_to_fixed_integer_t<int_t2> >::type type;
};

template <typename int_t2, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<int_t2, fixed_integer_native_const<has_sign2, bits2, value2>, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<int_to_fixed_integer_t<int_t2>, fixed_integer_native_const<has_sign2, bits2, value2> >::type type;
};

template <bool has_sign1, size_t bits1, bits_to_int_t<bits1, has_sign1> value1>
class compatible<fixed_integer_native_const<has_sign1, bits1, value1>, dynamic_integer>
{
public:
	typedef typename compatible<dynamic_integer, fixed_integer_native_const<has_sign1, bits1, value1> >::type type;
};



typedef fixed_integer_native_const<false, 0, 0> zero_t;
typedef fixed_integer_native_const<false, 1, 1> one_t;
typedef fixed_integer_native_const<true, 1, -1> minus_one_t;



#pragma warning(pop)

}



#endif
