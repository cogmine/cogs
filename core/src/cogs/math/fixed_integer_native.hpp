//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_MATH_FIXEDINT_NATIVE
#define COGS_HEADER_MATH_FIXEDINT_NATIVE

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/compatible.hpp"
#include "cogs/env.hpp"
#include "cogs/env/math/umul.hpp"
#include "cogs/math/fixed_integer.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/is_arithmetic.hpp"
#include "cogs/math/is_integral.hpp"
#include "cogs/math/is_signed.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_bits.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified


template <bool has_sign, size_t n_bits>
class fixed_integer_native;

template <bool has_sign, size_t n_bits>
class fixed_integer_extended;

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
class fixed_integer_native_const;

template <bool has_sign, size_t bits, ulongest... values>
class fixed_integer_extended_const;


class dynamic_integer;
class dynamic_integer_content;

template <typename numerator_t, typename denominator_t>
class fraction;

template <typename T>
class string_t;


namespace io
{
	class buffer;
}

template <bool has_sign, size_t bits> class is_arithmetic<fixed_integer_native<has_sign, bits> > : public std::true_type { };

template <bool has_sign, size_t n_bits> class is_integral<fixed_integer_native<has_sign, n_bits> > : public std::true_type { };

template <bool has_sign, size_t n_bits> class is_signed<fixed_integer_native<has_sign, n_bits> > { public: static constexpr bool value = has_sign; };


template <bool has_sign, size_t bits>
class int_to_fixed_integer<fixed_integer_native<has_sign, bits> >
{
public:
	typedef fixed_integer_native<has_sign, bits> type;
};


// Native int representation
template <bool has_sign_in, size_t n_bits_in>
class fixed_integer_native
{
public:
	typedef fixed_integer_native<has_sign_in, n_bits_in> this_t;

	static constexpr bool has_sign = has_sign_in;
	static constexpr size_t bits = n_bits_in;
	static constexpr size_t n_digits = 1;

	typedef bits_to_int_t<bits, true> signed_int_t;
	typedef bits_to_int_t<bits, false> unsigned_int_t;
	typedef bits_to_int_t<bits, has_sign> int_t;

	static constexpr size_t bits_used = sizeof(int_t) * 8;
	typedef fixed_integer_native_const<false, const_bit_scan_reverse_v<bits_used> + 1, bits_used> bits_used_t;

	int_t simplify_type() const { return m_int; }
	int_t simplify_type() const volatile { return atomic::load(m_int); }

private:
	static_assert(bits > 0);
	static_assert(bits <= (sizeof(longest) * 8));

	template <bool, size_t>
	friend class fixed_integer_native;

	template <bool, size_t>
	friend class fixed_integer_extended;

	alignas (atomic::get_alignment_v<int_t>) int_t m_int;

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void add(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_int = (int_t)src1.get_int() + (int_t)src2.get_int();
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void subtract(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_int = (int_t)src1.get_int() - (int_t)src2.get_int();
	}

	template <bool has_sign2, size_t bits2, bool has_sign3, size_t bits3>
	void multiply(const fixed_integer_native<has_sign2, bits2>& src1, const fixed_integer_native<has_sign3, bits3>& src2)
	{
		m_int = (int_t)src1.get_int() * (int_t)src2.get_int();
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) const
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt != 0) // 00 = -256 
						{
							if (srcInt < 0) // High bit is set, so srcInt value is accurate
							{
								result = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
								break;
							}

							// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
							result = cogs::negative(cogs::divide_whole(m_int, (ulongest)-srcInt)); // unsigned/unsigned divide, wont grow
							break;
						}
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::divide_whole(m_int, src.get_int());
					break;
				}
			}

			result.clear();
			break;
		}

		return result;
	}

	auto divide_whole(const dynamic_integer_content& src) const;
	//{
	//	fixed_integer<true, bits + 1> result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt != 0) // 00 = -256 
	//					{
	//						if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						{
	//							result = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//							break;
	//						}
	//
	//						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//						result = cogs::negative(cogs::divide_whole(m_int, (ulongest)-srcInt)); // unsigned/unsigned divide, wont grow
	//						break;
	//					}
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.get_length() <= 1)
	//			{
	//				result = cogs::divide_whole(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result.clear();
	//		break;
	//	}
	//
	//	return result;
	//}

	template <bool has_sign2, size_t bits2>
	void assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		*this = divide_whole(src);
	}

	template <bool has_sign2, size_t bits2>
	void assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		if (src.is_negative())
		{
			if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
					{
						cogs::assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
						return;
					}
				}
				else // this range will be 0..255 == 00..FF
				{
					if (srcInt != 0) // 00 = -256 
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
						{
							cogs::assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							return;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t)
						{
							return cogs::negative(divide_whole(t, srcInt2));
						}); 
						return;
					}
				}
			}
		}
		else
		{
			if (src.test_sign_extension(false, 1))
			{
				cogs::assign_divide_whole(m_int, src.get_int());
				return;
			}
		}

		clear();
	}

	void assign_divide_whole(const dynamic_integer_content& src);
	//{
	//	*this = divide_whole(src);
	//}

	void assign_divide_whole(const dynamic_integer_content& src) volatile;
	//{
	//	if (src.is_negative())
	//	{
	//		if (src.get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//		{
	//			longest srcInt = (longest)src.get_int();
	//			if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//			{
	//				if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//				{
	//					cogs::assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//					return;
	//				}
	//			}
	//			else // this range will be 0..255 == 00..FF
	//			{
	//				if (srcInt != 0) // 00 = -256 
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						cogs::assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						return;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t)
	//					{
	//						return cogs::negative(divide_whole(t, srcInt2));
	//					});
	//					return;
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		if (src.get_length() <= 1)
	//		{
	//			cogs::assign_divide_whole(m_int, src.get_int());
	//			return;
	//		}
	//	}
	//
	//	clear();
	//}

	template <bool has_sign2, size_t bits2>
	const this_t& pre_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		assign_divide_whole(src);
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		if (src.is_negative())
		{
			if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						return cogs::pre_assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
				}
				else // this range will be 0..255 == 00..FF
				{
					if (srcInt != 0) // 00 = -256 
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
							return cogs::pre_assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t)
						{
							return cogs::negative(divide_whole(t, srcInt2));
						});
					}
				}
			}
		}
		else
		{
			if (src.test_sign_extension(false, 1))
				return cogs::pre_assign_divide_whole(m_int, src.get_int());
		}

		cogs::clear(m_int);
		return 0;
	}

	const this_t& pre_assign_divide_whole(const dynamic_integer_content& src);
	//{
	//	*this = divide_whole(src);
	//	return *this;
	//}

	this_t pre_assign_divide_whole(const dynamic_integer_content& src) volatile;
	//{
	//	if (src.is_negative())
	//	{
	//		if (src.get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//		{
	//			longest srcInt = (longest)src.get_int();
	//			if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//			{
	//				if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					return cogs::pre_assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//			}
	//			else // this range will be 0..255 == 00..FF
	//			{
	//				if (srcInt != 0) // 00 = -256 
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						return cogs::pre_assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t)
	//					{
	//						return cogs::negative(divide_whole(t, srcInt2));
	//					});
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		if (src.get_length() <= 1)
	//			return cogs::pre_assign_divide_whole(m_int, src.get_int());
	//	}
	//
	//	cogs::clear(m_int);
	//	return 0;
	//}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		this_t tmp(*this);
		assign_divide_whole(src);
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		if (src.is_negative())
		{
			if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						return cogs::post_assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
				}
				else // this range will be 0..255 == 00..FF
				{
					if (srcInt != 0) // 00 = -256 
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
							return cogs::post_assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
						{
							return cogs::negative(divide_whole(t, srcInt2));
						});
					}
				}
			}
		}
		else
		{
			if (src.test_sign_extension(false, 1))
				return cogs::post_assign_divide_whole(m_int, src.get_int());
		}

		return cogs::exchange(m_int, 0);
	}

	this_t post_assign_divide_whole(const dynamic_integer_content& src);
	//{
	//	this_t tmp(*this);
	//	assign_divide_whole(src);
	//	return tmp;
	//}

	this_t post_assign_divide_whole(const dynamic_integer_content& src) volatile;
	//{
	//	if (src.is_negative())
	//	{
	//		if (src.get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//		{
	//			longest srcInt = (longest)src.get_int();
	//			if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//			{
	//				if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					return cogs::post_assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//			}
	//			else // this range will be 0..255 == 00..FF
	//			{
	//				if (srcInt != 0) // 00 = -256 
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						return cogs::post_assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
	//					{
	//						return cogs::negative(divide_whole(t, srcInt2));
	//					});
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		if (src.get_length() <= 1)
	//			return cogs::post_assign_divide_whole(m_int, src.get_int());
	//	}
	//
	//	return cogs::exchange(m_int, 0);
	//}

	template <bool has_sign2, size_t bits2> // bits < bits2
	this_t operator%(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) const
	{
		this_t result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::modulo(m_int, srcInt);
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt != 0) // 00 = -256 
						{
							if (srcInt < 0) // High bit is set, so srcInt value is accurate
							{
								result = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
								break;
							}

							// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
							result = cogs::modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
							break;
						}
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::modulo(m_int, src.get_int());
					break;
				}
			}

			result = m_int;
			break;
		}

		return result;
	}

	this_t operator%(const dynamic_integer_content& src) const;
	//{
	//	this_t result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::modulo(m_int, srcInt);
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt != 0) // 00 = -256 
	//					{
	//						if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						{
	//							result = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//							break;
	//						}
	//
	//						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//						result = cogs::modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
	//						break;
	//					}
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.get_length() <= 1)
	//			{
	//				result = cogs::modulo(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result = m_int;
	//		break;
	//	}
	//
	//	return result;
	//}

	template <bool has_sign2, size_t bits2>
	this_t& operator%=(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							cogs::assign_modulo(m_int, srcInt);
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt != 0) // 00 = -256 
						{
							if (srcInt < 0) // High bit is set, so srcInt value is accurate
							{
								cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
								break;
							}

							// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
							cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
							break;
						}
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					cogs::assign_modulo(m_int, src.get_int());
					break;
				}
			}

			break;
		}

		return *this;
	}

	template <bool has_sign2, size_t bits2>
	volatile this_t& operator%=(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							cogs::assign_modulo(m_int, srcInt);
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt != 0) // 00 = -256 
						{
							if (srcInt < 0) // High bit is set, so srcInt value is accurate
							{
								cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
								break;
							}

							// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
							cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
							break;
						}
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					cogs::assign_modulo(m_int, src.get_int());
					break;
				}
			}

			break;
		}

		return *this;
	}

	this_t& operator%=(const dynamic_integer_content& src);
	//{
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						cogs::assign_modulo(m_int, srcInt);
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt != 0) // 00 = -256 
	//					{
	//						if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						{
	//							cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//							break;
	//						}
	//
	//						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//						cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
	//						break;
	//					}
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				cogs::assign_modulo(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		break;
	//	}
	//
	//	return *this;
	//}

	volatile this_t& operator%=(const dynamic_integer_content& src) volatile;
	//{
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						cogs::assign_modulo(m_int, srcInt);
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt != 0) // 00 = -256 
	//					{
	//						if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						{
	//							cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//							break;
	//						}
	//
	//						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//						cogs::assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
	//						break;
	//					}
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				cogs::assign_modulo(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		break;
	//	}
	//
	//	return *this;
	//}

	template <bool has_sign2, size_t bits2>
	const this_t& pre_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		assign_modulo(src);
		return *this;
	}

	template <bool has_sign2, size_t bits2>
	this_t pre_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		if (src.is_negative())
		{
			if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						return cogs::pre_assign_modulo(m_int, srcInt);
				}
				else // this range will be 0..255 == 00..FF
				{
					if (srcInt != 0) // 00 = -256 
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
							return cogs::pre_assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						return cogs::pre_assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
					}
				}
			}
		}
		else
		{
			if (src.test_sign_extension(false, 1))
				return cogs::pre_assign_modulo(m_int, src.get_int());
		}

		return *this;
	}

	const this_t& pre_assign_modulo(const dynamic_integer_content& src);
	//{
	//	assign_modulo(src);
	//	return *this;
	//}

	this_t pre_assign_modulo(const dynamic_integer_content& src) volatile;
	//{
	//	if (src.is_negative())
	//	{
	//		if (src.m_contents->get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//		{
	//			longest srcInt = (longest)src.get_int();
	//			if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//			{
	//				if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					return cogs::pre_assign_modulo(m_int, srcInt);
	//			}
	//			else // this range will be 0..255 == 00..FF
	//			{
	//				if (srcInt != 0) // 00 = -256 
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						return cogs::pre_assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					return pre_assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		if (src.m_contents->get_length() <= 1)
	//			return cogs::pre_assign_modulo(m_int, src.get_int());
	//	}
	//
	//	return *this;
	//}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		this_t tmp(*this);
		assign_modulo(src);
		return tmp;
	}

	template <bool has_sign2, size_t bits2>
	this_t post_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		if (src.is_negative())
		{
			if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
			{
				longest srcInt = (longest)src.get_int();
				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
				{
					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						return cogs::post_assign_modulo(m_int, srcInt);
				}
				else // this range will be 0..255 == 00..FF
				{
					if (srcInt != 0) // 00 = -256 
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
							return cogs::post_assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						return cogs::post_assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
					}
				}
			}
		}
		else
		{
			if (src.test_sign_extension(false, 1))
				return cogs::post_assign_modulo(m_int, src.get_int());
		}

		return *this;
	}

	this_t post_assign_modulo(const dynamic_integer_content& src);
	//{
	//	this_t tmp(*this);
	//	assign_modulo(src);
	//	return tmp;
	//}

	this_t post_assign_modulo(const dynamic_integer_content& src) volatile;
	//{
	//	if (src.is_negative())
	//	{
	//		if (src.m_contents->get_length() == 1) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
	//		{
	//			longest srcInt = (longest)src.get_int();
	//			if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//			{
	//				if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					return cogs::post_assign_modulo(m_int, srcInt);
	//			}
	//			else // this range will be 0..255 == 00..FF
	//			{
	//				if (srcInt != 0) // 00 = -256 
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//						return cogs::post_assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					return post_assign_modulo(m_int, (ulongest)-srcInt); // src sign doesn't affect the sign of the result
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		if (src.m_contents->get_length() <= 1)
	//			return cogs::post_assign_modulo(m_int, src.get_int());
	//	}
	//
	//	return *this;
	//}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) const
	{
		typedef fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_t;
		std::pair<divide_t, this_t> result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result.first = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
							result.second = cogs::modulo(m_int, srcInt);
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt != 0) // 00 = -256 
						{
							if (srcInt < 0) // High bit is set, so srcInt value is accurate
							{
								result.first = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
								result.second = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
								break;
							}

							// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
							ulongest srcInt2 = (ulongest)-srcInt;
							result.first = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
							result.second = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
							break;
						}
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result.first = cogs::divide_whole(m_int, src.get_int());
					result.second = cogs::modulo(m_int, src.get_int());
					break;
				}
			}

			result.second = m_int;
			result.first.clear();
			break;
		}

		return result;
	}

	auto divide_whole_and_modulo(const dynamic_integer_content& src) const;
	//{
	//	typedef fixed_integer<true, bits + 1> divide_t;
	//	std::pair<divide_t, this_t> result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result.first = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//						result.second = cogs::modulo(m_int, srcInt);
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						result.first = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						result.second = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//						break;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					result.first = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
	//					result.second = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				result.first = cogs::divide_whole(m_int, src.get_int());
	//				result.second = cogs::modulo(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result.second = m_int;
	//		result.first.clear();
	//		break;
	//	}
	//
	//	return result;
	//}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
							cogs::assign_modulo(m_int, srcInt);
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
						{
							result = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						result = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
						cogs::assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::divide_whole(m_int, src.get_int());
					cogs::assign_modulo(m_int, src.get_int());
					break;
				}
			}

			result.clear();
			break;
		}

		return result;
	}

	template <bool has_sign2, size_t bits2>
	auto divide_whole_and_assign_modulo(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
						{
							result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						int_t tmp = cogs::post_assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
						result = cogs::negative(cogs::divide_whole(tmp, srcInt2)); // unsigned/unsigned divide, wont grow
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::divide_whole(cogs::post_assign_modulo(m_int, src.get_int()), src.get_int());
					break;
				}
			}

			result.clear();
			break;
		}

		return result;
	}

	auto divide_whole_and_assign_modulo(const dynamic_integer_content& src);
	//{
	//	fixed_integer<true, bits + 1> result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//						cogs::assign_modulo(m_int, srcInt);
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						result = cogs::divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						cogs::assign_modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//						break;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					result = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
	//					cogs::assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				result = cogs::divide_whole(m_int, src.get_int());
	//				cogs::assign_modulo(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result.clear();
	//		break;
	//	}
	//
	//	return result;
	//}

	auto divide_whole_and_assign_modulo(const dynamic_integer_content& src) volatile;
	//{
	//	fixed_integer<true, bits + 1> result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						result = cogs::divide_whole(cogs::post_assign_modulo(m_int, srcInt), srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						// unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//						break;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					int_t tmp = cogs::post_assign_modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
	//					result = cogs::negative(cogs::divide_whole(tmp, srcInt2)); // unsigned/unsigned divide, wont grow
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				result = cogs::divide_whole(cogs::post_assign_modulo(m_int, src.get_int()), src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result.clear();
	//		break;
	//	}
	//
	//	return result;
	//}

	template <bool has_sign2, size_t bits2>
	this_t modulo_and_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src)
	{
		this_t result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::modulo(m_int, srcInt);
							cogs::assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
						{
							result.second = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							cogs::assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						result = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
						*this = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::modulo(m_int, src.get_int());
					cogs::assign_divide_whole(m_int, src.get_int());
					break;
				}
			}

			result = m_int;
			cogs::clear(m_int);
			break;
		}

		return result;
	}

	template <bool has_sign2, size_t bits2>
	this_t modulo_and_assign_divide_whole(const typename fixed_integer_extended<has_sign2, bits2>::content_t& src) volatile
	{
		this_t result;
		for (;;)
		{
			if (src.is_negative())
			{
				if (src.test_sign_extension(true, 1)) // src's range will be -256..0.  -256..-129..-128..-1 == 0..7F..80..FF == 0..007F FF80..FFFF
				{
					longest srcInt = (longest)src.get_int();
					if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
					{
						if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
						{
							result = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt);
							// signed/signed divide, so may grow, i.e.: -128/-1=128
							break;
						}
					}
					else // this range will be 0..255 == 00..FF
					{
						if (srcInt < 0) // High bit is set, so srcInt value is accurate
						{
							result.second = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
							// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
							break;
						}

						// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
						ulongest srcInt2 = (ulongest)-srcInt;
						int_t tmp = atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
						{
							return cogs::negative(cogs::divide_whole(t, srcInt2)); // unsigned/unsigned divide, wont grow
						});
						result = cogs::modulo(tmp, srcInt2); // src sign doesn't affect the sign of the result
					}
				}
			}
			else
			{
				if (src.test_sign_extension(false, 1))
				{
					result = cogs::modulo(cogs::post_assign_divide_whole(m_int, src.get_int()), src.get_int());
					break;
				}
			}

			result = cogs::exchange(m_int, 0);
			break;
		}

		return result;
	}

	this_t modulo_and_assign_divide_whole(const dynamic_integer_content& src);
	//{
	//	this_t result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::modulo(m_int, srcInt);
	//						cogs::assign_divide_whole(m_int, srcInt); // signed/signed divide, so may grow, i.e.: -128/-1=128
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						result.second = cogs::modulo(m_int, srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//						cogs::assign_divide_whole(m_int, srcInt); // unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						break;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					result = cogs::modulo(m_int, srcInt2); // src sign doesn't affect the sign of the result
	//					*this = cogs::negative(cogs::divide_whole(m_int, srcInt2)); // unsigned/unsigned divide, wont grow
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				result = cogs::modulo(m_int, src.get_int());
	//				cogs::assign_divide_whole(m_int, src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result = m_int;
	//		cogs::clear(m_int);
	//		break;
	//	}
	//
	//	return result;
	//}

	this_t modulo_and_assign_divide_whole(const dynamic_integer_content& src) volatile;
	//{
	//	this_t result;
	//	for (;;)
	//	{
	//		if (src.is_negative())
	//		{
	//			if (src.m_contents->get_length() == 1)
	//			{
	//				longest srcInt = (longest)src.get_int();
	//				if (has_sign) // this range will be -128..-1..0..127 == 80..FF == FF80..FFFF
	//				{
	//					if (srcInt < 0) // Therefor, if src is negative (80..FF), it's within range of this. 
	//					{
	//						result = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt);
	//						// signed/signed divide, so may grow, i.e.: -128/-1=128
	//						break;
	//					}
	//				}
	//				else // this range will be 0..255 == 00..FF
	//				{
	//					if (srcInt < 0) // High bit is set, so srcInt value is accurate
	//					{
	//						result.second = cogs::modulo(cogs::post_assign_divide_whole(m_int, srcInt), srcInt); // unsigned/signed modulo, so may grow, i.e.: 255/-1=-255
	//						// unsigned/signed divide, so may grow, i.e.: 255/-1=-255
	//						break;
	//					}
	//
	//					// High bit is zero, but the number is still negative. -255..-129, -255==1, -128=128, -129=127
	//					ulongest srcInt2 = (ulongest)-srcInt;
	//					int_t tmp = atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t)
	//					{
	//						return cogs::negative(cogs::divide_whole(t, srcInt2)); // unsigned/unsigned divide, wont grow
	//					});
	//					result = cogs::modulo(tmp, srcInt2); // src sign doesn't affect the sign of the result
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (src.m_contents->get_length() <= 1)
	//			{
	//				result = cogs::modulo(cogs::post_assign_divide_whole(m_int, src.get_int()), src.get_int());
	//				break;
	//			}
	//		}
	//
	//		result = cogs::exchange(m_int, 0);
	//		break;
	//	}
	//
	//	return result;
	//}

	template <bool has_sign2, size_t bits2>
	fixed_integer_native(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		operator=(src);
	}

	template <bool has_sign2, size_t bits2>
	this_t& operator=(const fixed_integer_extended_content<has_sign2, bits2>& src)
	{
		cogs::assign(m_int, src.get_int());
		return *this;
	}

	fixed_integer_native(const dynamic_integer_content& src);
	//{
	//	operator=(src);
	//}

	this_t& operator=(const dynamic_integer_content& src);
	//{
	//	cogs::assign(m_int, src.get_int());
	//	return *this;
	//}

public:
	fixed_integer_native()
	{ }

	fixed_integer_native(const dynamic_integer& src); //{ operator=(src); }

	fixed_integer_native(const volatile dynamic_integer& src); //{ operator=(src); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	fixed_integer_native(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	fixed_integer_native(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	fixed_integer_native(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2, ulongest... values2>
	fixed_integer_native(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_native(const fixed_integer_native<has_sign2, bits2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_native(const volatile fixed_integer_native<has_sign2, bits2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_native(const fixed_integer_extended<has_sign2, bits2>& src)
	{ operator=(src); }

	template <bool has_sign2, size_t bits2>
	fixed_integer_native(const volatile fixed_integer_extended<has_sign2, bits2>& src)
	{ operator=(src); }

	fixed_integer_native(const this_t& src)
	{ operator=(src); }

	fixed_integer_native(const volatile this_t& src)
	{ operator=(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	fixed_integer_native(const int_t2& src)
	{ operator=(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	fixed_integer_native(const volatile int_t2& src)
	{ operator=(src); }

	template <typename numerator_t, typename denominator_t>
	fixed_integer_native(const fraction<numerator_t, denominator_t>& src)
	{ operator=(src); }

	template <typename numerator_t, typename denominator_t>
	fixed_integer_native(const volatile fraction<numerator_t, denominator_t>& src)
	{ operator=(src); }




	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	this_t& operator=(const fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(value2);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	this_t& operator=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src)
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(value2);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	volatile this_t& operator=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(value2);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
	volatile this_t& operator=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile
	{
		typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(value2);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	this_t& operator=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{
		typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	this_t& operator=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src)
	{
		typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	volatile this_t& operator=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile
	{
		typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2, ulongest... values2>
	volatile this_t& operator=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile
	{
		typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src);
		operator=(tmp);
		return *this;
	}

	template <bool has_sign2, size_t bits2> this_t& operator=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign(m_int, src.get_int()); return *this; }

	this_t& operator=(const dynamic_integer& src); // { cogs::assign(m_int, src.get_int()); return *this; }
	this_t& operator=(const volatile dynamic_integer& src); // { cogs::assign(m_int, src.get_int()); return *this; }
	volatile this_t& operator=(const dynamic_integer& src) volatile; // { cogs::assign(m_int, src.get_int()); return *this; }
	volatile this_t& operator=(const volatile dynamic_integer& src) volatile; // { cogs::assign(m_int, src.get_int()); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator=(const int_t2& src) { cogs::assign(m_int, src); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator=(const volatile int_t2& src) { cogs::assign(m_int, src); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator=(const int_t2& src) volatile { cogs::assign(m_int, src); return *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator=(const volatile int_t2& src) volatile { cogs::assign(m_int, src); return *this; }

	template <typename numerator_t, typename denominator_t>
	this_t& operator=(const fraction<numerator_t, denominator_t>& src)
	{ return operator=(src.floor()); }

	template <typename numerator_t, typename denominator_t>
	this_t& operator=(const volatile fraction<numerator_t, denominator_t>& src)
	{ return operator=(src.floor()); }

	template <typename numerator_t, typename denominator_t>
	volatile this_t& operator=(const fraction<numerator_t, denominator_t>& src) volatile
	{ return operator=(src.floor()); }

	template <typename numerator_t, typename denominator_t>
	volatile this_t& operator=(const volatile fraction<numerator_t, denominator_t>& src) volatile
	{ return operator=(src.floor()); }


	int_t& get_int() { return m_int; }
	const int_t& get_int() const { return m_int; }
	//volatile int_t& get_int() volatile { return m_int; }
	int_t get_int() const volatile { return atomic::load(m_int); }

	//operator int_t() const { return m_int; }
	//operator int_t() const volatile { return atomic::load(m_int); }

	void clear() { cogs::clear(m_int); }
	void clear() volatile { cogs::clear(m_int); }

	bool operator!() const { return cogs::lnot(m_int); }
	bool operator!() const volatile { return cogs::lnot(m_int); }

	this_t operator~() const { return cogs::bit_not(m_int); }
	this_t operator~() const volatile { return cogs::bit_not(m_int); }
	void assign_bit_not() { cogs::assign_bit_not(m_int); }
	void assign_bit_not() volatile { cogs::assign_bit_not(m_int); }
	const this_t& pre_assign_bit_not() { cogs::assign_bit_not(m_int); return *this; }
	this_t pre_assign_bit_not() volatile { return cogs::pre_assign_bit_not(m_int); }
	this_t post_assign_bit_not() { return cogs::post_assign_bit_not(m_int); }
	this_t post_assign_bit_not() volatile { return cogs::post_assign_bit_not(m_int); }

	bool is_negative() const { return cogs::is_negative(m_int); }
	bool is_negative() const volatile { return cogs::is_negative(m_int); }

	bool is_exponent_of_two() const { return cogs::is_exponent_of_two(m_int); }
	bool is_exponent_of_two() const volatile { return cogs::is_exponent_of_two(m_int); }

	constexpr bool has_fractional_part() const volatile { return false; }

	fixed_integer_native<false, bits> abs() const { return cogs::abs(m_int); }
	fixed_integer_native<false, bits> abs() const volatile { return cogs::abs(m_int); }
	fixed_integer_native<false, bits> abs_inner() const { return (unsigned_int_t)-(signed_int_t)m_int; }
	fixed_integer_native<false, bits> abs_inner() const volatile { return (unsigned_int_t)-(signed_int_t)m_int; }
	void assign_abs() { cogs::assign_abs(m_int); }
	void assign_abs() volatile { cogs::assign_abs(m_int); }
	const this_t& pre_assign_abs() { cogs::assign_abs(m_int); return *this; }
	this_t pre_assign_abs() volatile { return cogs::pre_assign_abs(m_int); }
	this_t post_assign_abs() { return cogs::post_assign_abs(m_int); }
	this_t post_assign_abs() volatile { return cogs::post_assign_abs(m_int); }

	auto operator-() const { fixed_integer<true, bits + 1> i(*this); i.assign_negative(); return i; }
	auto operator-() const volatile { fixed_integer<true, bits + 1> i(*this); i.assign_negative(); return i; }
	void assign_negative() { cogs::assign_negative(m_int); }
	void assign_negative() volatile { cogs::assign_negative(m_int); }
	const this_t& pre_assign_negative() { cogs::assign_negative(m_int); return *this; }
	this_t pre_assign_negative() volatile { return cogs::pre_assign_negative(m_int); }
	this_t post_assign_negative() { return cogs::post_assign_negative(m_int); }
	this_t post_assign_negative() volatile { return cogs::post_assign_negative(m_int); }

	size_t bit_count() const { return cogs::bit_count(m_int); }
	size_t bit_count() const volatile { return cogs::bit_count(m_int); }
	void assign_bit_count() { cogs::assign_bit_count(m_int); }
	void assign_bit_count() volatile { cogs::assign_bit_count(m_int); }
	const this_t& pre_assign_bit_count() { cogs::assign_bit_count(m_int); return *this; }
	this_t pre_assign_bit_count() volatile { return cogs::pre_assign_bit_count(m_int); }
	this_t post_assign_bit_count() { return cogs::post_assign_bit_count(m_int); }
	this_t post_assign_bit_count() volatile { return cogs::post_assign_bit_count(m_int); }

	size_t bit_scan_forward() const { return cogs::bit_scan_forward(m_int); }
	size_t bit_scan_forward() const volatile { return cogs::bit_scan_forward(m_int); }
	void assign_bit_scan_forward() { cogs::assign_bit_scan_forward(m_int); }
	void assign_bit_scan_forward() volatile { cogs::assign_bit_scan_forward(m_int); }
	const this_t& pre_assign_bit_scan_forward() { cogs::assign_bit_scan_forward(m_int); return *this; }
	this_t pre_assign_bit_scan_forward() volatile { return cogs::pre_assign_bit_scan_forward(m_int); }
	this_t post_assign_bit_scan_forward() { return cogs::post_assign_bit_scan_forward(m_int); }
	this_t post_assign_bit_scan_forward() volatile { return cogs::post_assign_bit_scan_forward(m_int); }

	size_t bit_scan_reverse() const { return cogs::bit_scan_reverse(m_int); }
	size_t bit_scan_reverse() const volatile { return cogs::bit_scan_reverse(m_int); }
	void assign_bit_scan_reverse() { cogs::assign_bit_scan_reverse(m_int); }
	void assign_bit_scan_reverse() volatile { cogs::assign_bit_scan_reverse(m_int); }
	const this_t& pre_assign_bit_scan_reverse() { cogs::assign_bit_scan_reverse(m_int); return *this; }
	this_t pre_assign_bit_scan_reverse() volatile { return cogs::pre_assign_bit_scan_reverse(m_int); }
	this_t post_assign_bit_scan_reverse() { return cogs::post_assign_bit_scan_reverse(m_int); }
	this_t post_assign_bit_scan_reverse() volatile { return cogs::post_assign_bit_scan_reverse(m_int); }

	this_t next() const { return cogs::next(m_int); }
	this_t next() const volatile { return cogs::next(m_int); }
	void assign_next() { cogs::assign_next(m_int); }
	void assign_next() volatile { cogs::assign_next(m_int); }
	this_t& operator++() { cogs::assign_next(m_int); return *this; }
	this_t operator++() volatile { return cogs::pre_assign_next(m_int); }
	this_t operator++(int) { return cogs::post_assign_next(m_int); }
	this_t operator++(int) volatile { return cogs::post_assign_next(m_int); }

	this_t prev() const { return cogs::prev(m_int); }
	this_t prev() const volatile { return cogs::prev(m_int); }
	void assign_prev() { cogs::assign_prev(m_int); }
	void assign_prev() volatile { cogs::assign_prev(m_int); }
	this_t& operator--() { cogs::assign_prev(m_int); return *this; }
	this_t operator--() volatile { return cogs::pre_assign_prev(m_int); }
	this_t operator--(int) { return cogs::post_assign_prev(m_int); }
	this_t operator--(int) volatile { return cogs::post_assign_prev(m_int); }

	this_t endian_swap() const { return cogs::endian_swap(m_int); }
	this_t endian_swap() const volatile { return cogs::endian_swap(m_int); }
	void assign_endian_swap() { cogs::assign_endian_swap(m_int); }
	void assign_endian_swap() volatile { cogs::assign_endian_swap(m_int); }
	const this_t& pre_assign_endian_swap() { cogs::assign_endian_swap(m_int); return *this; }
	this_t pre_assign_endian_swap() volatile { return cogs::pre_assign_endian_swap(m_int); }
	this_t post_assign_endian_swap() { return cogs::post_assign_endian_swap(m_int); }
	this_t post_assign_endian_swap() volatile { return cogs::post_assign_endian_swap(m_int); }

	// bit_rotate_right
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_rotate_right(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_rotate_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_rotate_right(get_int(), src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const dynamic_integer& src) const; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const dynamic_integer& src) const volatile; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const volatile dynamic_integer& src) const; // { return bit_rotate_right(src % bits_used_t()); }
	this_t bit_rotate_right(const volatile dynamic_integer& src) const volatile ; //{ return bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_right(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_rotate_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_rotate_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const volatile dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const dynamic_integer& src) volatile; // { assign_bit_rotate_right(src % bits_used_t()); }
	void assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	const this_t& pre_assign_bit_rotate_right(const dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_rotate_right(const volatile dynamic_integer& src); // { assign_bit_rotate_right(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_rotate_right(const dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	this_t pre_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_right(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_rotate_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_rotate_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_rotate_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const dynamic_integer& src); // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const volatile dynamic_integer& src); // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const dynamic_integer& src) volatile; // { return post_assign_bit_rotate_right(src % bits_used_t()); }
	this_t post_assign_bit_rotate_right(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_rotate_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_right(tmp); }

	// bit_rotate_left
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_rotate_left(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_rotate_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_rotate_left(get_int(), src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const dynamic_integer& src) const; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const dynamic_integer& src) const volatile; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const volatile dynamic_integer& src) const; // { return bit_rotate_left(src % bits_used_t()); }
	this_t bit_rotate_left(const volatile dynamic_integer& src) const volatile; // { return bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t bit_rotate_left(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_rotate_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_rotate_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const volatile dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const dynamic_integer& src) volatile; // { assign_bit_rotate_left(src % bits_used_t()); }
	void assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_rotate_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	const this_t& pre_assign_bit_rotate_left(const dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_rotate_left(const volatile dynamic_integer& src); // { assign_bit_rotate_left(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_rotate_left(const dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	this_t pre_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_rotate_left(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_rotate_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_rotate_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_rotate_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const dynamic_integer& src); // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const volatile dynamic_integer& src); // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const dynamic_integer& src) volatile; // { return post_assign_bit_rotate_left(src % bits_used_t()); }
	this_t post_assign_bit_rotate_left(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_rotate_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_rotate_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_rotate_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_rotate_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_rotate_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_rotate_left(tmp); }

	// bit_shift_right
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_shift_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_shift_right(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_shift_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_shift_right(get_int(), src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator>>(src % bits_used_t()); }
	this_t operator>>(const dynamic_integer& src) const; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const dynamic_integer& src) const volatile; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const volatile dynamic_integer& src) const; // { return operator>>(src % bits_used_t()); }
	this_t operator>>(const volatile dynamic_integer& src) const volatile; // { return operator>>(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& operator>>(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator>>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator>>(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator>>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator>>(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator>>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_shift_right(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_shift_right(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { operator>>=(src % bits_used_t()); return *this; }
	this_t& operator>>=(const dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	this_t& operator>>=(const volatile dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	volatile this_t& operator>>=(const dynamic_integer& src) volatile; // { operator>>=(src % bits_used_t()); return *this; }
	volatile this_t& operator>>=(const volatile dynamic_integer& src) volatile; // { operator>>=(src % bits_used_t()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t&  operator>>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t&  operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator>>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator>>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator>>=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t&  operator>>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return operator>>=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t&  operator>>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return operator>>=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator>>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return operator>>=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator>>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return operator>>=(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator>>=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator>>=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator>>=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator>>=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator>>=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_right(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_shift_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_shift_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator>>=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	const this_t& pre_assign_bit_shift_right(const dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_shift_right(const volatile dynamic_integer& src); // { operator>>=(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_shift_right(const dynamic_integer& src) volatile; // { return pre_assign_bit_shift_right(src % bits_used_t()); }
	this_t pre_assign_bit_shift_right(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_shift_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_right(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_shift_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_shift_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_shift_right(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_shift_right(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const dynamic_integer& src); // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const volatile dynamic_integer& src); // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const dynamic_integer& src) volatile; // { return post_assign_bit_shift_right(src % bits_used_t()); }
	this_t post_assign_bit_shift_right(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_shift_right(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_right(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_right(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_right(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_right(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_right(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_right(tmp); }

	// bit_shift_left
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_shift_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_shift_left(get_int(), src.m_int); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_shift_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_shift_left(get_int(), src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return operator<<(src % bits_used_t()); }
	this_t operator<<(const dynamic_integer& src) const; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const dynamic_integer& src) const volatile; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const volatile dynamic_integer& src) const; // { return operator<<(src % bits_used_t()); }
	this_t operator<<(const volatile dynamic_integer& src) const volatile; // { return operator<<(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& operator<<(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t operator<<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator<<(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t operator<<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return operator<<(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t operator<<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_shift_left(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_shift_left(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { operator<<=(src % bits_used_t()); return *this; }
	this_t& operator<<=(const dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	this_t& operator<<=(const volatile dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	volatile this_t& operator<<=(const dynamic_integer& src) volatile; // { operator<<=(src % bits_used_t()); return *this; }
	volatile this_t& operator<<=(const volatile dynamic_integer& src) volatile; // { operator<<=(src % bits_used_t()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t&  operator<<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t&  operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator<<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<=(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator<<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return operator<<=(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t&  operator<<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return operator<<=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t&  operator<<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return operator<<=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator<<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return operator<<=(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator<<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return operator<<=(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator<<=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator<<=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator<<=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator<<=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator<<=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_shift_left(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_shift_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_shift_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator<<=(src % bits_used_t()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	const this_t& pre_assign_bit_shift_left(const dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	const this_t& pre_assign_bit_shift_left(const volatile dynamic_integer& src); // { operator<<=(src % bits_used_t()); return *this; }
	this_t pre_assign_bit_shift_left(const dynamic_integer& src) volatile; // { return pre_assign_bit_shift_left(src % bits_used_t()); }
	this_t pre_assign_bit_shift_left(const volatile dynamic_integer& src) volatile; // { return pre_assign_bit_shift_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return pre_assign_bit_shift_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_shift_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_shift_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_shift_left(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_shift_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_shift_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_shift_left(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_shift_left(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const dynamic_integer& src); // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const volatile dynamic_integer& src); // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const dynamic_integer& src) volatile; // { return post_assign_bit_shift_left(src % bits_used_t()); }
	this_t post_assign_bit_shift_left(const volatile dynamic_integer& src) volatile; // { return post_assign_bit_shift_left(src % bits_used_t()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_shift_left(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_shift_left(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_shift_left(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { return post_assign_bit_shift_left(src % bits_used_t()); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_shift_left(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_shift_left(tmp); }


	// bit and
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), (bits < bits2 ? bits : bits2)> operator&(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), (bits < bits2 ? bits : bits2)> operator&(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), (bits < bits2 ? bits : bits2)> operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign2 : has_sign)), (bits < bits2 ? bits : bits2)> operator&(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src & *this; }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src & *this; }

	template <bool has_sign2, size_t bits2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator&(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator&(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this & tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator&(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_and(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator&=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator&=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator&=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator&=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator&=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this &= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator&=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this &= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator&=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator&=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator&=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator&=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator&=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator&=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_and(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_and(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_and(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_and(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_and(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_and(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_and(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_and(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_and(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_and(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_and(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_and(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_and(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_and(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_and(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_and(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_and(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_and(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_and(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_and(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_and(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_and(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_and(tmp); }

	// bit or
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator|(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator|(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator|(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src | *this; }
	template <bool has_sign2, size_t bits2> auto operator|(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src | *this; }

	template <bool has_sign2, size_t bits2> const this_t& operator|(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator|(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator|(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator|(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator|(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator|(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this | tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator|(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_or(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator|=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator|=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator|=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator|=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator|=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this |= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator|=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this |= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator|=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator|=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator|=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator|=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator|=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator|=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_or(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_or(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_or(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_or(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_or(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_or(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_or(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_or(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_or(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_or(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_or(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_or(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_or(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_or(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_or(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_or(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_or(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_or(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_or(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_or(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_or(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_or(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_or(tmp); }

	// bit_xor
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator^(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator^(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<((bits == bits2) ? (has_sign && has_sign2) : ((bits > bits2) ? has_sign : has_sign2)), ((bits > bits2) ? bits : bits2)> operator^(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src ^ *this; }
	template <bool has_sign2, size_t bits2> auto operator^(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src ^ *this; }

	template <bool has_sign2, size_t bits2> this_t operator^(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator~(); }
	template <bool has_sign2, size_t bits2> this_t operator^(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator~(); }
	template <bool has_sign2, size_t bits2> this_t operator^(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator~(); }
	template <bool has_sign2, size_t bits2> this_t operator^(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator~(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator^(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator^(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^ tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator^(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator^=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator^=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator^=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator^=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator^=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator^=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this ^= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator^=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator^=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator^=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator^=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator^=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator^=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_bit_xor(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_xor(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_bit_xor(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_bit_not(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return pre_assign_bit_not(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return pre_assign_bit_not(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_bit_xor(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_xor(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_xor(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_xor(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_xor(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_xor(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_xor(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_xor(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_bit_xor(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_xor(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_bit_xor(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); assign_bit_not(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); assign_bit_not(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return post_assign_bit_not(); }
	template <bool has_sign2, size_t bits2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return post_assign_bit_not(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_bit_xor(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_bit_xor(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_bit_xor(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_xor(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_xor(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_xor(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_xor(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_xor(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_xor(tmp); }

	// add
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> operator+(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> operator+(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign | has_sign2, (bits > bits2 ? bits : bits2) + 1> operator+(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.operator+(*this); }
	template <bool has_sign2, size_t bits2> auto operator+(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.operator+(*this); }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.operator+(*this); }
	template <bool has_sign2, size_t bits2> auto operator+(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.operator+(*this); }
	auto operator+(const dynamic_integer& src) const; // { return src.operator+(*this); }
	auto operator+(const dynamic_integer& src) const volatile; // { return src.operator+(*this); }
	auto operator+(const volatile dynamic_integer& src) const; // { return src.operator+(*this); }
	auto operator+(const volatile dynamic_integer& src) const volatile; // { return src.operator+(*this); }

	template <bool has_sign2, size_t bits2> const this_t& operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator+(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator+(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator+(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator+(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this + tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator+(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_add(m_int, src.get_int()); return *this; }
	this_t& operator+=(const dynamic_integer& src); // { cogs::assign_add(m_int, src.get_int()); return *this; }
	this_t& operator+=(const volatile dynamic_integer& src); // { cogs::assign_add(m_int, src.get_int()); return *this; }
	volatile this_t& operator+=(const dynamic_integer& src) volatile; // { cogs::assign_add(m_int, src.get_int()); return *this; }
	volatile this_t& operator+=(const volatile dynamic_integer& src) volatile; // { cogs::assign_add(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator+=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator+=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator+=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this += tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator+=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this += tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator+=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator+=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator+=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator+=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator+=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator+=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_add(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_add(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_add(m_int, src.get_int()); }
	const this_t& pre_assign_add(const dynamic_integer& src); // { cogs::assign_add(m_int, src.get_int()); return *this; }
	const this_t& pre_assign_add(const volatile dynamic_integer& src); // { cogs::assign_add(m_int, src.get_int()); return *this; }
	this_t pre_assign_add(const dynamic_integer& src) volatile; // { return cogs::pre_assign_add(m_int, src.get_int()); }
	this_t pre_assign_add(const volatile dynamic_integer& src) volatile; // { return cogs::pre_assign_add(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_add(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_add(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_add(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_add(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_add(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_add(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_add(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_add(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_add(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_add(m_int, src.get_int()); }
	this_t post_assign_add(const dynamic_integer& src); // { return cogs::post_assign_add(m_int, src.get_int()); }
	this_t post_assign_add(const volatile dynamic_integer& src); // { return cogs::post_assign_add(m_int, src.get_int()); }
	this_t post_assign_add(const dynamic_integer& src) volatile; // { return cogs::post_assign_add(m_int, src.get_int()); }
	this_t post_assign_add(const volatile dynamic_integer& src) volatile; // { return cogs::post_assign_add(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_add(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_add(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_add(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_add(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_add(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_add(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_add(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_add(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_add(tmp); }

	// subtract
	template <bool has_sign2, size_t bits2> fixed_integer<true, (bits > bits2 ? bits : bits2) + 1> operator-(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<true, (bits > bits2 ? bits : bits2) + 1> operator-(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<true, (bits > bits2 ? bits : bits2) + 1> operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<true, (bits > bits2 ? bits : bits2) + 1> operator-(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<true, bits2 + 1> result; result.subtract(*this, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this);  fixed_integer_extended<true, bits2 + 1> result; result.subtract(tmp, src); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { fixed_integer_extended<has_sign2, bits2> tmp(*this);  fixed_integer_extended<true, bits2 + 1> result; result.subtract(*this, tmp); return result; }
	template <bool has_sign2, size_t bits2> auto operator-(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { fixed_integer_extended<has_sign2, bits2> tmp(*this);  fixed_integer_extended<true, bits2 + 1> result; result.subtract(*this, tmp); return result; }
	auto operator-(const dynamic_integer& src) const; // { return src.inverse_subtract(*this); }
	auto operator-(const dynamic_integer& src) const volatile; // { return src.inverse_subtract(*this); }
	auto operator-(const volatile dynamic_integer& src) const; // { return src.inverse_subtract(*this); }
	auto operator-(const volatile dynamic_integer& src) const volatile; // { return src.inverse_subtract(*this); }

	template <bool has_sign2, size_t bits2> const this_t& operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator-(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator-(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator-(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator-(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this - tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator-(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	this_t& operator-=(const dynamic_integer& src); // { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	this_t& operator-=(const volatile dynamic_integer& src); // { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	volatile this_t& operator-=(const dynamic_integer& src) volatile; // { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	volatile this_t& operator-=(const volatile dynamic_integer& src) volatile; // { cogs::assign_subtract(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator-=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator-=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator-=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this -= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator-=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this -= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator-=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator-=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator-=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator-=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator-=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator-=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_subtract(m_int, src.get_int()); }
	const this_t& pre_assign_subtract(const dynamic_integer& src); // { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	const this_t& pre_assign_subtract(const volatile dynamic_integer& src); // { cogs::assign_subtract(m_int, src.get_int()); return *this; }
	this_t pre_assign_subtract(const dynamic_integer& src) volatile ; //{ return cogs::pre_assign_subtract(m_int, src.get_int()); }
	this_t pre_assign_subtract(const volatile dynamic_integer& src) volatile; // { return cogs::pre_assign_subtract(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_subtract(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_subtract(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_bit_subtract(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_bit_subtract(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_bit_subtract(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_subtract(m_int, src.get_int()); }
	this_t post_assign_subtract(const dynamic_integer& src); // { return cogs::post_assign_subtract(m_int, src.get_int()); }
	this_t post_assign_subtract(const volatile dynamic_integer& src); // { return cogs::post_assign_subtract(m_int, src.get_int()); }
	this_t post_assign_subtract(const dynamic_integer& src) volatile; // { return cogs::post_assign_subtract(m_int, src.get_int()); }
	this_t post_assign_subtract(const volatile dynamic_integer& src) volatile; // { return cogs::post_assign_subtract(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_subtract(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_subtract(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_subtract(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_bit_subtract(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_bit_subtract(tmp); }

	// inverse_subtract
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src - *this; }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src - *this; }
	auto inverse_subtract(const dynamic_integer& src) const; // { return src - *this; }
	auto inverse_subtract(const dynamic_integer& src) const volatile; // { return src - *this; }
	auto inverse_subtract(const volatile dynamic_integer& src) const; // { return src - *this; }
	auto inverse_subtract(const volatile dynamic_integer& src) const volatile; // { return src - *this; }

	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp - *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_subtract(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	void assign_inverse_subtract(const dynamic_integer& src); // { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	void assign_inverse_subtract(const volatile dynamic_integer& src) ; //{ cogs::assign_inverse_subtract(m_int, src.get_int()); }
	void assign_inverse_subtract(const dynamic_integer& src) volatile; // { cogs::assign_inverse_subtract(m_int, src.get_int()); }
	void assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { cogs::assign_inverse_subtract(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_subtract(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_subtract(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_subtract(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_subtract(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }
	const this_t& pre_assign_inverse_subtract(const dynamic_integer& src); // { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
	const this_t& pre_assign_inverse_subtract(const volatile dynamic_integer& src); // { cogs::assign_inverse_subtract(m_int, src.get_int()); return *this; }
	this_t pre_assign_inverse_subtract(const dynamic_integer& src) volatile; // { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }
	this_t pre_assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { return cogs::pre_assign_inverse_subtract(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_subtract(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_subtract(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_subtract(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_subtract(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_subtract(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	this_t post_assign_inverse_subtract(const dynamic_integer& src); // { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	this_t post_assign_inverse_subtract(const volatile dynamic_integer& src); // { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	this_t post_assign_inverse_subtract(const dynamic_integer& src) volatile; // { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }
	this_t post_assign_inverse_subtract(const volatile dynamic_integer& src) volatile; // { return cogs::post_assign_inverse_subtract(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_subtract(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_subtract(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_subtract(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_subtract(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_subtract(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_subtract(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_subtract(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_subtract(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_subtract(tmp); }

	// multiply
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + bits2> operator*(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + bits2> operator*(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + bits2> operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + bits2> operator*(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.operator*(*this); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.operator*(*this); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.operator*(*this); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.operator*(*this); }
	auto operator*(const dynamic_integer& src) const; // { return src.operator*(*this); }
	auto operator*(const dynamic_integer& src) const volatile; // { return src.operator*(*this); }
	auto operator*(const volatile dynamic_integer& src) const; // { return src.operator*(*this); }
	auto operator*(const volatile dynamic_integer& src) const volatile; // { return src.operator*(*this); }

	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> const this_t& operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const volatile this_t& operator*(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> const volatile this_t& operator*(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto operator*(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator*(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator*(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto operator*(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator*(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator*(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this * tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator*(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	this_t& operator*=(const dynamic_integer& src); // { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	this_t& operator*=(const volatile dynamic_integer& src); // { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	volatile this_t& operator*=(const dynamic_integer& src) volatile; // { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	volatile this_t& operator*=(const volatile dynamic_integer& src) volatile; // { cogs::assign_multiply(m_int, src.get_int()); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> this_t& operator*=(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t& operator*=(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator*=(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator*=(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator*=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator*=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator*=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this *= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator*=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this *= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator*=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator*=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator*=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator*=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator*=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator*=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_multiply(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::pre_assign_multiply(m_int, src.get_int()); }
	const this_t& pre_assign_multiply(const dynamic_integer& src); // { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	const this_t& pre_assign_multiply(const volatile dynamic_integer& src); // { cogs::assign_multiply(m_int, src.get_int()); return *this; }
	this_t pre_assign_multiply(const dynamic_integer& src) volatile; // { return cogs::pre_assign_multiply(m_int, src.get_int()); }
	this_t pre_assign_multiply(const volatile dynamic_integer& src) volatile; // { return cogs::pre_assign_multiply(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_multiply(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_multiply(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_multiply(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_multiply(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_multiply(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_multiply(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_multiply(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return cogs::post_assign_multiply(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_multiply(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return cogs::post_assign_multiply(m_int, src.get_int()); }
	this_t post_assign_multiply(const dynamic_integer& src); // { return cogs::post_assign_multiply(m_int, src.get_int()); }
	this_t post_assign_multiply(const volatile dynamic_integer& src); // { return cogs::post_assign_multiply(m_int, src.get_int()); }
	this_t post_assign_multiply(const dynamic_integer& src) volatile; // { return cogs::post_assign_multiply(m_int, src.get_int()); }
	this_t post_assign_multiply(const volatile dynamic_integer& src) volatile; // { return cogs::post_assign_multiply(m_int, src.get_int()); }

	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_multiply(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_multiply(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_multiply(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_multiply(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_multiply(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_multiply(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_multiply(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_multiply(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_multiply(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_multiply(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_multiply(tmp); }

	//   signed/signed  :  127 % -128 =  127 (8 bits, signed)
	//   signed/signed  :  127 %  127 =    0 (8 bits, signed)
	//   signed/signed  : -128 % -128 =    0 (8 bits, signed)
	//   signed/signed  : -128 %  127 =   -1 (8 bits, signed)
	//
	// unsigned/signed  :  255 % -128 =  127 (7 bits, unsigned)
	// unsigned/signed  :  255 %  127 =    1 (7 bits, unsigned)
	//
	//   signed/unsigned:  127 %  255 =  127 (8 bits, signed)
	//   signed/unsigned: -127 %  255 = -127 (8 bits, signed)
	//
	// unsigned/unsigned:  255 %  255 =    0 (8 bits, unsigned)

	// modulo
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> operator%(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> operator%(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> operator%(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t operator%(const fixed_integer_extended<has_sign2, bits2>& src) const { return operator%(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t operator%(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); return tmp % src; }
	template <bool has_sign2, size_t bits2> this_t operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return operator%(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> this_t operator%(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); return tmp % src; }
	this_t operator%(const dynamic_integer& src) const; // { return operator%(*(src.m_contents)); }
	this_t operator%(const dynamic_integer& src) const volatile; // { this_t tmp(*this); return tmp % src; }
	this_t operator%(const volatile dynamic_integer& src) const; // { auto rt = src.guarded_begin_read(); auto result = operator%(*rt); rt->release(); return result; }
	this_t operator%(const volatile dynamic_integer& src) const volatile; // { this_t tmp(*this); return tmp % src; }

	template <bool has_sign2, size_t bits2> void operator%(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator%(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { return zero_t(); }
	template <size_t bits2> auto operator%(const fixed_integer_native_const<true, bits2, -1>&) const;// { return zero_t(); }
	template <size_t bits2> auto operator%(const volatile fixed_integer_native_const<true, bits2, -1>&) const;// { return zero_t(); }
	template <size_t bits2> auto operator%(const fixed_integer_native_const<true, bits2, -1>&) const volatile;// { return zero_t(); }
	template <size_t bits2> auto operator%(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator%(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator%(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this % tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator%(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%(tmp); }

	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_modulo(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_modulo(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) { operator%=(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { operator%=(*(src.being_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { operator%=(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { operator%=(*(src.m_contents)); return *this; }
	this_t& operator%=(const dynamic_integer& src); // { operator%=(*(src.m_contents)); return *this; }
	this_t& operator%=(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return *this; }
	volatile this_t& operator%=(const dynamic_integer& src) volatile; // { operator%=(*(src.m_contents)); return *this; }
	volatile this_t& operator%=(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); operator%=(*rt); rt->release(); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { clear(); return *this; }
	template <size_t bits2> this_t& operator%=(const fixed_integer_native_const<true, bits2, -1>&) { clear(); return *this; }
	template <size_t bits2> this_t& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>&) { clear(); return *this; }
	template <size_t bits2> volatile this_t& operator%=(const fixed_integer_native_const<true, bits2, -1>&) volatile { clear(); return *this; }
	template <size_t bits2> volatile this_t& operator%=(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { clear(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator%=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator%=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator%=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this %= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator%=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this %= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator%=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator%=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator%=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator%=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator%=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator%=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return pre_assign_modulo(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_modulo(*(src.begin_read())); }
	const this_t& pre_assign_modulo(const dynamic_integer& src); // { return pre_assign_modulo(*(src.m_contents)); }
	const this_t& pre_assign_modulo(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); assign_modulo(*rt); rt->release(); return *this; }
	this_t pre_assign_modulo(const dynamic_integer& src) volatile; // { return pre_assign_modulo(*(src.m_contents)); }
	this_t pre_assign_modulo(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = pre_assign_modulo(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { clear(); return 0; }
	template <size_t bits2> const this_t& pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) { clear(); return *this; }
	template <size_t bits2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) { clear(); return *this; }
	template <size_t bits2> this_t pre_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) volatile { clear(); return 0; }
	template <size_t bits2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_modulo(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_modulo(m_int, *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_modulo(m_int, *(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return post_assign_modulo(m_int, *(src.begin_read())); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_modulo(m_int, *(src.begin_read())); }
	this_t post_assign_modulo(const dynamic_integer& src); // { return post_assign_modulo(*(src.m_contents)); }
	this_t post_assign_modulo(const dynamic_integer& src) volatile; // { return post_assign_modulo(*(src.m_contents)); }
	this_t post_assign_modulo(const volatile dynamic_integer& src); // { this_t tmp(*this);  auto rt = src.guarded_begin_read(); assign_modulo(*rt); rt->release(); return tmp; }
	this_t post_assign_modulo(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = post_assign_modulo(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return cogs::exchange(m_int, 0); }
	template <size_t bits2> const this_t& post_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return tmp; }
	template <size_t bits2> const this_t& post_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return tmp; }
	template <size_t bits2> this_t post_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) volatile { return cogs::exchange(m_int, 0); }
	template <size_t bits2> this_t post_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_modulo(tmp); }

	// inverse_modulo
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::modulo(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::modulo(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::modulo(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign2, bits2> inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::modulo(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src % *this; }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const; // { return src % *this; }
	auto inverse_modulo(const dynamic_integer& src) const volatile; // { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const; // { return src % *this; }
	auto inverse_modulo(const volatile dynamic_integer& src) const volatile; // { return src % *this; }

	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp % *this; }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return tmp % *this; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src % t; }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { this_t tmp(src); assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src % *this; }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src % t; }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src % t; }); }
	void assign_inverse_modulo(const dynamic_integer& src); // { *this = src % *this; }
	void assign_inverse_modulo(const volatile dynamic_integer& src); // { *this = src % *this; }
	void assign_inverse_modulo(const dynamic_integer& src) volatile; // { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src % t; }); }
	void assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = exchange(0); COGS_ASSERT(!!result); }
	template <bool has_sign2, size_t bits2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = exchange(0); COGS_ASSERT(!!result); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_modulo(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_modulo(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src % t; }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src % t; }); }
	const this_t& pre_assign_inverse_modulo(const dynamic_integer& src); // { assign_inverse_modulo(src); return *this; }
	const this_t& pre_assign_inverse_modulo(const volatile dynamic_integer& src); // { assign_inverse_modulo(src); return *this; }
	this_t pre_assign_inverse_modulo(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src % t; }); }
	this_t pre_assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = exchange(0); COGS_ASSERT(!!result); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = exchange(0); COGS_ASSERT(!!result); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) { return post_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::post_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_modulo(m_int, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_modulo(m_int, *(src.m_contents)); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src % t; }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src % t; }); }
	this_t post_assign_inverse_modulo(const dynamic_integer& src); // { this_t tmp(*this); assign_inverse_modulo(src); return tmp; }
	this_t post_assign_inverse_modulo(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_inverse_modulo(src); return tmp; }
	this_t post_assign_inverse_modulo(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src % t; }); }
	this_t post_assign_inverse_modulo(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_inverse_modulo(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); COGS_ASSERT(!!tmp); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = cogs::exchange(m_int, 0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = cogs::exchange(m_int, 0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_modulo(tmp); }

	// divide
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_native<has_sign2, bits2> > operator/(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_native<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	template <bool has_sign2, size_t bits2> fraction<this_t, fixed_integer_extended<has_sign2, bits2> > operator/(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<this_t, fixed_integer_extended<has_sign2, bits2> >(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const dynamic_integer& src) const; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const dynamic_integer& src) const volatile; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const volatile dynamic_integer& src) const; // { return fraction<this_t, dynamic_integer>(*this, src); }
	fraction<this_t, dynamic_integer> operator/(const volatile dynamic_integer& src) const volatile; // { return fraction<this_t, dynamic_integer>(*this, src); }

	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void operator/(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator/(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t operator/(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto operator/(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto operator/(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto operator/(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<this_t, fixed_integer_native_const<has_sign2, bits2, value2> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto operator/(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<this_t, fixed_integer_extended_const<has_sign2, bits2, values2...> >(*this, src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const { return fraction<this_t, int_t2>(*this, i); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto operator/(const volatile int_t2& i) const volatile { return fraction<this_t, int_t2>(*this, i); }


	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native<has_sign2, bits2>& src) { m_int /= src.m_int; return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_divide_whole(m_int, src.m_int); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_divide_whole(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_divide_whole(m_int, src.get_int()); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(*(src.begin_read())); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(*(src.m_contents)); return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(*(src.begin_read())); return *this; }
	this_t& operator/=(const dynamic_integer& src); // { assign_divide_whole(*(src.m_contents)); return *this; }
	this_t& operator/=(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); return *this; }
	volatile this_t& operator/=(const dynamic_integer& src) volatile; // { assign_divide_whole(*(src.m_contents)); return *this; }
	volatile this_t& operator/=(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); return *this; }

	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> this_t& operator/=(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator/=(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <size_t bits2> volatile this_t& operator/=(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); return *this; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator/=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> volatile this_t& operator/=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator/=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> volatile this_t& operator/=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this /= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator/=(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator/=(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t& operator/=(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	volatile this_t& operator/=(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return operator/=(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(*(src.begin_read())); }
	const this_t& pre_assign_divide(const dynamic_integer& src); // { return *this /= src; }
	const this_t& pre_assign_divide(const volatile dynamic_integer& src); // { return *this /= src; }
	this_t pre_assign_divide(const dynamic_integer& src) volatile; // { return pre_assign_divide_whole(*(src.m_contents)); }
	this_t pre_assign_divide(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t pre_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> this_t pre_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(*(src.begin_read())); }
	this_t post_assign_divide(const dynamic_integer& src); // { this_t tmp(*this); *this /= src; return tmp; }
	this_t post_assign_divide(const volatile dynamic_integer& src); // { this_t tmp(*this); *this /= src; return tmp; }
	this_t post_assign_divide(const dynamic_integer& src) volatile; // { return post_assign_divide_whole(*(src.m_contents)); }
	this_t post_assign_divide(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> const this_t& post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_divide(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_divide(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide(tmp); }

	// reciprocal
	auto reciprocal() const;// { return fraction<one_t, this_t>(one_t(), *this); }
	auto reciprocal() const volatile;// { return fraction<one_t, this_t>(one_t(), *this); }
	void assign_reciprocal() { cogs::assign_reciprocal(m_int); }
	void assign_reciprocal() volatile { cogs::assign_reciprocal(m_int); }
	const this_t& pre_assign_reciprocal() { cogs::assign_reciprocal(m_int); return *this; }
	this_t pre_assign_reciprocal() volatile { return cogs::pre_assign_reciprocal(m_int); }
	this_t post_assign_reciprocal() { this_t result(*this); cogs::assign_reciprocal(m_int); return result; }
	this_t post_assign_reciprocal() volatile { return cogs::post_assign_reciprocal(m_int); }

	// inverse_divide
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_native<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_native<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2> fraction<fixed_integer_extended<has_sign2, bits2>, this_t> inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return fraction<fixed_integer_extended<has_sign2, bits2>, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const dynamic_integer& src) const; // { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const dynamic_integer& src) const volatile; // { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const volatile dynamic_integer& src) const; // { return fraction<dynamic_integer, this_t>(src, *this); }
	fraction<dynamic_integer, this_t> inverse_divide(const volatile dynamic_integer& src) const volatile; // { return fraction<dynamic_integer, this_t>(src, *this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { return fraction<fixed_integer_native_const<has_sign2, bits2, value2>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { return fraction<fixed_integer_extended_const<has_sign2, bits2, values2...>, this_t>(src, *this); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const { return fraction<int_t2, this_t>(i, *this); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide(const volatile int_t2& i) const volatile { return fraction<int_t2, this_t>(i, *this); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_divide(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_divide(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	void assign_inverse_divide(const dynamic_integer& src); // { *this = src.divide_whole(*this); }
	void assign_inverse_divide(const volatile dynamic_integer& src); // { *this = src.divide_whole(*this); }
	void assign_inverse_divide(const dynamic_integer& src) volatile; // { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	void assign_inverse_divide(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_inverse_divide(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	const this_t& pre_assign_inverse_divide(const dynamic_integer& src); // { assign_inverse_divide(src); return *this; }
	const this_t& pre_assign_inverse_divide(const volatile dynamic_integer& src); // { assign_inverse_divide(src); return *this; }
	this_t pre_assign_inverse_divide(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t pre_assign_inverse_divide(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_divide(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t post_assign_inverse_divide(const dynamic_integer& src); // { this_t tmp(*this); post_assign_inverse_divide(src); return tmp; }
	this_t post_assign_inverse_divide(const volatile dynamic_integer& src); // { this_t tmp(*this); post_assign_inverse_divide(src); return tmp; }
	this_t post_assign_inverse_divide(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t post_assign_inverse_divide(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_inverse_divide(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); COGS_ASSERT(!!*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); COGS_ASSERT(!!*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide(tmp); }

	// floor
	const this_t& floor() const { return *this; }
	this_t floor() const volatile { return *this; }
	void assign_floor() { }
	void assign_floor() volatile { }
	const this_t& pre_assign_floor() { return *this; }
	this_t pre_assign_floor() volatile { return *this; }
	const this_t& post_assign_floor() { return *this; }
	this_t post_assign_floor() volatile { return *this; }

	// ceil
	const this_t& ceil() const { return *this; }
	this_t ceil() const volatile { return *this; }
	void assign_ceil() { }
	void assign_ceil() volatile { }
	const this_t& pre_assign_ceil() { return *this; }
	this_t pre_assign_ceil() volatile { return *this; }
	const this_t& post_assign_ceil() { return *this; }
	this_t post_assign_ceil() volatile { return *this; }

	// fractional_part
	auto fractional_part() const;// { return zero_t(); }
	auto fractional_part() const volatile;// { return zero_t(); }
	void assign_fractional_part() { clear(); }
	void assign_fractional_part() volatile { clear(); }
	this_t pre_assign_fractional_part() { clear(); return 0; }
	this_t pre_assign_fractional_part() volatile { clear(); return 0; }
	this_t post_assign_fractional_part() { this_t tmp(*this); clear(); return tmp; }
	this_t post_assign_fractional_part() volatile { return cogs::exchange(m_int, 0); }

	//   signed/signed  :  127 / -1 = -127 (8 bits, signed)
	//   signed/signed  :  127 /  1 =  127 (8 bits, signed)
	//   signed/signed  : -128 / -1 =  128 (9 bits, signed) ***
	//   signed/signed  : -128 /  1 = -128 (8 bits, signed)
	//
	// unsigned/signed  :  255 / -1 = -255 (9 bits, signed) ***
	// unsigned/signed  :  255 /  1 =  255 (9 bits, signed) ***
	//
	//   signed/unsigned:  127 /  1 =  127 (8 bits, signed)
	//   signed/unsigned: -127 /  1 = -127 (8 bits, signed)
	//
	// unsigned/unsigned:  255 /  1 =  255 (8 bits, unsigned)

	// divide_whole
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const { return divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); return tmp.divide_whole(src); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return divide_whole(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> auto divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this); return tmp.divide_whole(src); }
	auto divide_whole(const dynamic_integer& src) const; // { return divide_whole(*(src.m_contents)); }
	auto divide_whole(const dynamic_integer& src) const volatile; // { this_t tmp(*this); return tmp.divide_whole(src); }
	auto divide_whole(const volatile dynamic_integer& src) const; // { dynamic_integer tmp(src); return divide_whole(tmp); }
	auto divide_whole(const volatile dynamic_integer& src) const volatile; // { this_t tmp(*this); return tmp.divide_whole(src); }

	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return *this; }
	template <bool has_sign2, size_t bits2> this_t divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return *this; }
	template <size_t bits2> auto divide_whole(const fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return operator-(); }
	template <size_t bits2> auto divide_whole(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <size_t bits2> auto divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return operator-(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_divide_whole(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_divide_whole(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_divide_whole(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { assign_divide_whole(*(src.begin_read())); }
	void assign_divide_whole(const dynamic_integer& src); // { assign_divide_whole(*(src.m_contents)); }
	void assign_divide_whole(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release(); }
	void assign_divide_whole(const dynamic_integer& src) volatile; // { assign_divide_whole(*(src.m_contents)) }
	void assign_divide_whole(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); assign_divide_whole(*rt); rt->release();}

	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { }
	template <bool has_sign2, size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { }
	template <size_t bits2> void assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); }
	template <size_t bits2> void assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return *this /= src; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return pre_assign_divide_whole(*(src.begin_read())); }
	const this_t& pre_assign_divide_whole(const dynamic_integer& src); // { return *this /= src; }
	const this_t& pre_assign_divide_whole(const volatile dynamic_integer& src); // { return *this /= src; }
	this_t pre_assign_divide_whole(const dynamic_integer& src) volatile; // { return pre_assign_divide_whole(*(src.m_contents)); }
	this_t pre_assign_divide_whole(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_negative(); return *this; }
	template <size_t bits2> this_t pre_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <size_t bits2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); *this /= src; return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return post_assign_divide_whole(*(src.begin_read())); }
	this_t post_assign_divide_whole(const dynamic_integer& src); // { this_t tmp(*this); *this /= src; return tmp; }
	this_t post_assign_divide_whole(const volatile dynamic_integer& src); // { this_t tmp(*this); *this /= src; return tmp; }
	this_t post_assign_divide_whole(const dynamic_integer& src) volatile; // { return post_assign_divide_whole(*(src.m_contents)); }
	this_t post_assign_divide_whole(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = pre_assign_divide_whole(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <bool has_sign2, size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return *this; }
	template <size_t bits2> const this_t& post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> const this_t& post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_negative(); return tmp; }
	template <size_t bits2> this_t post_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <size_t bits2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_negative(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_divide_whole(tmp); }

	// inverse_divide_whole
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits2 + (has_sign ? 1 : 0)> inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits2 + (has_sign ? 1 : 0)> inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits2 + (has_sign ? 1 : 0)> inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits2 + (has_sign ? 1 : 0)> inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole(src.m_int, m_int); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const; // { return src.divide_whole(*this); }
	auto inverse_divide_whole(const dynamic_integer& src) const volatile; // { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const; // { return src.divide_whole(*this); }
	auto inverse_divide_whole(const volatile dynamic_integer& src) const volatile; // { return src.divide_whole(*this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { COGS_ASSERT(!!*this); return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_divide_whole(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_inverse_divide_whole(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.divide_whole(*this); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	void assign_inverse_divide_whole(const dynamic_integer& src); // { *this = src.divide_whole(*this); }
	void assign_inverse_divide_whole(const volatile dynamic_integer& src); // { *this = src.divide_whole(*this); }
	void assign_inverse_divide_whole(const dynamic_integer& src) volatile; // { atomic::compare_exchange_retry_loop(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	void assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { *this = inverse_divide_whole(src); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { *this = inverse_divide_whole(src); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_inverse_divide_whole(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	const this_t& pre_assign_inverse_divide_whole(const dynamic_integer& src); // { assign_inverse_divide_whole(src); return *this; }
	const this_t& pre_assign_inverse_divide_whole(const volatile dynamic_integer& src); // { assign_inverse_divide_whole(src); return *this; }
	this_t pre_assign_inverse_divide_whole(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t pre_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { COGS_ASSERT(!!*this); clear(); return 0; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_inverse_divide(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_inverse_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t post_assign_inverse_divide_whole(const dynamic_integer& src); // { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	this_t post_assign_inverse_divide_whole(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_inverse_divide_whole(src); return tmp; }
	this_t post_assign_inverse_divide_whole(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.divide_whole(t); }); }
	this_t post_assign_inverse_divide_whole(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_inverse_divide_whole(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> const this_t& post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { COGS_ASSERT(!!*this); this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = cogs::exchange(m_int, 0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { auto result = cogs::exchange(m_int, 0); COGS_ASSERT(!!result); return result; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_inverse_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_inverse_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_inverse_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_inverse_divide_whole(tmp); }

	// divide_whole_and_modulo
	template <bool has_sign2, size_t bits2> std::pair<fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)>, fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> > divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole_and_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> std::pair<fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)>, fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> > divide_whole_and_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole_and_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> std::pair<fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)>, fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> > divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::divide_whole_and_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> std::pair<fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)>, fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> > divide_whole_and_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::divide_whole_and_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return divide_whole_and_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this);  return tmp.divide_whole_and_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return divide_whole_and_modulo(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { this_t tmp(*this);  return tmp.divide_whole_and_modulo(*(src.begin_read())); }
	auto divide_whole_and_modulo(const dynamic_integer& src) const; // { return divide_whole_and_modulo(*(src.m_contents)); }
	auto divide_whole_and_modulo(const dynamic_integer& src) const volatile; // { this_t tmp(*this);  return tmp.divide_whole_and_modulo(*(src.m_contents)); }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const; // { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_modulo(*rt); rt->release(); return result; }
	auto divide_whole_and_modulo(const volatile dynamic_integer& src) const volatile; // { this_t tmp(*this); return tmp.divide_whole_and_modulo(src); }

	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { std::pair<this_t, zero_t>(*this, zero_t()); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { std::pair<this_t, zero_t>(*this, zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const;// { std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const;// { std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const fixed_integer_native_const<true, bits2, -1>&) const volatile;// { std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <size_t bits2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile;// { std::pair<fixed_integer<true, bits + 1>, zero_t>(operator-(), zero_t()); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_modulo(tmp); }

	// inverse_divide_whole_and_inverse_modulo
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.divide_whole_and_modulo(*this); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const; // { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const dynamic_integer& src) const volatile; // { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const; // { return src.divide_whole_and_modulo(*this); }
	auto inverse_divide_whole_and_inverse_modulo(const volatile dynamic_integer& src) const volatile; // { return src.divide_whole_and_modulo(*this); }

	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { COGS_ASSERT(!!*this); return std::pair<zero_t, zero_t>(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto inverse_divide_whole_and_inverse_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return inverse_divide_whole_and_inverse_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto inverse_divide_whole_and_inverse_modulo(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return inverse_divide_whole_and_inverse_modulo(tmp); }

	// divide_whole_and_assign_modulo
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::divide_whole_and_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole_and_assign_modulo(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::divide_whole_and_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::divide_whole_and_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<(has_sign || has_sign2), bits + (has_sign2 ? 1 : 0)> divide_whole_and_assign_modulo(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::divide_whole_and_assign_modulo(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) { return divide_whole_and_assign_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return divide_whole_and_assign_modulo(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return divide_whole_and_assign_modulo(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return divide_whole_and_assign_modulo(*(src.begin_read())); }
	auto divide_whole_and_assign_modulo(const dynamic_integer& src); // { return divide_whole_and_assign_modulo(*(src.m_contents)); }
	auto divide_whole_and_assign_modulo(const dynamic_integer& src) volatile; // { return divide_whole_and_assign_modulo(*(src.m_contents)); }
	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_assign_modulo(*rt); rt->release(); return result; }
	auto divide_whole_and_assign_modulo(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = divide_whole_and_assign_modulo(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <bool has_sign2, size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return exchange(0); }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); clear(); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<true, bits2, -1>&) volatile { this_t tmp(cogs::exchange(m_int, 0)); return -tmp; }
	template <size_t bits2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { this_t tmp(cogs::exchange(m_int, 0)); return -tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto divide_whole_and_assign_modulo(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return divide_whole_and_assign_modulo(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto divide_whole_and_assign_modulo(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return divide_whole_and_assign_modulo(tmp); }

	// modulo_and_assign_divide_whole
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> modulo_and_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) { return cogs::modulo_and_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> modulo_and_assign_divide_whole(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::modulo_and_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) { return cogs::modulo_and_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer_native<has_sign, ((bits < bits2) ? bits : (bits2 - ((!has_sign && has_sign2) ? 1 : 0)))> modulo_and_assign_divide_whole(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::modulo_and_assign_divide_whole(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) { return modulo_and_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return modulo_and_assign_divide_whole(*(src.m_contents)); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) { return modulo_and_assign_divide_whole(*(src.begin_read())); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return modulo_and_assign_divide_whole(*(src.begin_read())); }
	auto modulo_and_assign_divide_whole(const dynamic_integer& src); // { return modulo_and_assign_divide_whole(*(src.m_contents)); }
	auto modulo_and_assign_divide_whole(const dynamic_integer& src) volatile; // { return modulo_and_assign_divide_whole(*(src.m_contents)); }
	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src); // { auto rt = src.guarded_begin_read(); auto result = modulo_and_assign_divide_whole(*rt); rt->release(); return result; }
	auto modulo_and_assign_divide_whole(const volatile dynamic_integer& src) volatile; // { auto rt = src.guarded_begin_read(); auto result = modulo_and_assign_divide_whole(*rt); rt->release(); return result; }

	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> void modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile = delete;
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&);// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&);// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&);// { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&);// { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<true, bits2, -1>&) volatile;// { assign_negative(); return zero_t(); }
	template <size_t bits2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile;// { assign_negative(); return zero_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto modulo_and_assign_divide_whole(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return modulo_and_assign_divide_whole(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto modulo_and_assign_divide_whole(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return modulo_and_assign_divide_whole(tmp); }

	// gcd
	template <bool has_sign2, size_t bits2> fixed_integer<false, ((bits < bits2) ? bits : bits2)> gcd(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, ((bits < bits2) ? bits : bits2)> gcd(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, ((bits < bits2) ? bits : bits2)> gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, ((bits < bits2) ? bits : bits2)> gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.gcd(*this); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.gcd(*this); }
	auto gcd(const dynamic_integer& src) const; // { return src.gcd(*this); }
	auto gcd(const dynamic_integer& src) const volatile; // { return src.gcd(*this); }
	auto gcd(const volatile dynamic_integer& src) const; // { return src.gcd(*this); }
	auto gcd(const volatile dynamic_integer& src) const volatile; // { return src.gcd(*this); }

	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const;// { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { return one_t(); }
	template <bool has_sign2, size_t bits2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile;// { return one_t(); }
	template <size_t bits2> auto gcd(const fixed_integer_native_const<true, bits2, -1>&) const;// { return one_t(); }
	template <size_t bits2> auto gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) const;// { return one_t(); }
	template <size_t bits2> auto gcd(const fixed_integer_native_const<true, bits2, -1>&) const volatile;// { return one_t(); }
	template <size_t bits2> auto gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile;// { return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return gcd(tmp); }

	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_gcd(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_gcd(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.gcd(*this); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.gcd(*this); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.gcd(*this); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.gcd(*this); }
	void assign_gcd(const dynamic_integer& src); // { *this = src.gcd(*this); }
	void assign_gcd(const volatile dynamic_integer& src); // { *this = src.gcd(*this); }
	void assign_gcd(const dynamic_integer& src) volatile; // { *this = src.gcd(*this); }
	void assign_gcd(const volatile dynamic_integer& src) volatile; // { *this = src.gcd(*this); }

	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&);// { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&);// { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { *this = one_t(); }
	template <bool has_sign2, size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const fixed_integer_native_const<true, bits2, -1>&);// { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&);// { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile;// { *this = one_t(); }
	template <size_t bits2> void assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile;// { *this = one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_gcd(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	const this_t& pre_assign_gcd(const dynamic_integer& src); // { assign_gcd(src); return *this; }
	const this_t& pre_assign_gcd(const volatile dynamic_integer& src); // { assign_gcd(src); return *this; }
	this_t pre_assign_gcd(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	this_t pre_assign_gcd(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&);// { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&);// { *this = one_t(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { *this = one_t(); return one_t(); }
	template <size_t bits2> const this_t& pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&);// { *this = one_t(); return *this; }
	template <size_t bits2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&);// { *this = one_t(); return *this; }
	template <size_t bits2> this_t pre_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile;// { *this = one_t(); return one_t(); }
	template <size_t bits2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile;// { *this = one_t(); return one_t(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_gcd(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_gcd(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	this_t post_assign_gcd(const dynamic_integer& src); // { this_t tmp(*this); assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_gcd(src); return tmp; }
	this_t post_assign_gcd(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.gcd(t); }); }
	this_t post_assign_gcd(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_gcd(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&);// { this_t tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&);// { this_t tmp(*this); *this = one_t(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { return cogs::exchange(m_int, 1); }
	template <bool has_sign2, size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile;// { return cogs::exchange(m_int, 1); }
	template <size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&);// { this_t tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&);// { this_t tmp(*this); *this = one_t(); return tmp; }
	template <size_t bits2> this_t post_assign_gcd(const fixed_integer_native_const<true, bits2, -1>&) volatile { return cogs::exchange(m_int, 1); }
	template <size_t bits2> this_t post_assign_gcd(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return cogs::exchange(m_int, 1); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_gcd(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_gcd(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_gcd(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_gcd(tmp); }

	// lcm
	template <bool has_sign2, size_t bits2> fixed_integer<false, bits + bits2> lcm(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, bits + bits2> lcm(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, bits + bits2> lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<false, bits + bits2> lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.lcm(*this); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.lcm(*this); }
	auto lcm(const dynamic_integer& src) const; // { return src.lcm(*this); }
	auto lcm(const dynamic_integer& src) const volatile; // { return src.lcm(*this); }
	auto lcm(const volatile dynamic_integer& src) const; // { return src.lcm(*this); }
	auto lcm(const volatile dynamic_integer& src) const volatile; // { return src.lcm(*this); }

	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) const volatile;// { return zero_t(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return abs(); }
	template <bool has_sign2, size_t bits2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) const volatile { return abs(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const { return abs(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const { return abs(); }
	template <size_t bits2> auto lcm(const fixed_integer_native_const<true, bits2, -1>&) const volatile { return abs(); }
	template <size_t bits2> auto lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) const volatile { return abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lcm(tmp); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_lcm(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_lcm(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.lcm(*this); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.lcm(*this); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.lcm(*this); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.lcm(*this); }
	void assign_lcm(const dynamic_integer& src); // { *this = src.lcm(*this); }
	void assign_lcm(const volatile dynamic_integer& src); // { *this = src.lcm(*this); }
	void assign_lcm(const dynamic_integer& src) volatile; // { *this = src.lcm(*this); }
	void assign_lcm(const volatile dynamic_integer& src) volatile; // { *this = src.lcm(*this); }

	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { assign_abs(); }
	template <bool has_sign2, size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { assign_abs(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); }
	template <size_t bits2> void assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { assign_abs(); }
	template <size_t bits2> void assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); *this = gcd(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); *this = gcd(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lcm(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	const this_t& pre_assign_lcm(const dynamic_integer& src); // { assign_lcm(src); return *this; }
	const this_t& pre_assign_lcm(const volatile dynamic_integer& src); // { assign_lcm(src); return *this; }
	this_t pre_assign_lcm(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	this_t pre_assign_lcm(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { clear(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { clear(); return 0; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { assign_abs(); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return pre_assign_abs(); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return pre_assign_abs(); }
	template <size_t bits2> const this_t& pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); return *this; }
	template <size_t bits2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { assign_abs(); return *this; }
	template <size_t bits2> this_t pre_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_abs(); }
	template <size_t bits2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return pre_assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_lcm(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lcm(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	this_t post_assign_lcm(const dynamic_integer& src); // { this_t tmp(*this); assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_lcm(src); return tmp; }
	this_t post_assign_lcm(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lcm(t); }); }
	this_t post_assign_lcm(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_lcm(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) { this_t tmp(*this); clear(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 0>&) volatile { return cogs::exchange(m_int, 0); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return post_assign_abs(); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, 1>&) volatile { return post_assign_abs(); }
	template <size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) { this_t tmp(*this); assign_abs(); return tmp; }
	template <size_t bits2> this_t post_assign_lcm(const fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_abs(); }
	template <size_t bits2> this_t post_assign_lcm(const volatile fixed_integer_native_const<true, bits2, -1>&) volatile { return post_assign_abs(); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lcm(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lcm(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lcm(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lcm(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lcm(tmp); }




	// greater
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> greater(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> greater(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign && has_sign2, ((bits > bits2) ? bits : bits2)> greater(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.greater(*this); }
	template <bool has_sign2, size_t bits2> auto greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.greater(*this); }
	auto greater(const dynamic_integer& src) const; // { return src.greater(*this); }
	auto greater(const dynamic_integer& src) const volatile; // { return src.greater(*this); }
	auto greater(const volatile dynamic_integer& src) const; // { return src.greater(*this); }
	auto greater(const volatile dynamic_integer& src) const volatile; // { return src.greater(*this); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return greater(tmp); }

	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_greater(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_greater(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.greater(*this); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.greater(*this); }
	template <bool has_sign2, size_t bits2> void assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.greater(*this); }
	template <bool has_sign2, size_t bits2> void assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.greater(*this); }
	void assign_greater(const dynamic_integer& src); // { *this = src.greater(*this); }
	void assign_greater(const volatile dynamic_integer& src); // { *this = src.greater(*this); }
	void assign_greater(const dynamic_integer& src) volatile; // { *this = src.greater(*this); }
	void assign_greater(const volatile dynamic_integer& src) volatile; // { *this = src.greater(*this); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_greater(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_greater(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.greater(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.greater(t); }); }
	const this_t& pre_assign_greater(const dynamic_integer& src); // { assign_greater(src); return *this; }
	const this_t& pre_assign_greater(const volatile dynamic_integer& src); // { assign_greater(src); return *this; }
	this_t pre_assign_greater(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.greater(t); }); }
	this_t pre_assign_greater(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_greater(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_greater(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.greater(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_greater(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.greater(t); }); }
	this_t post_assign_greater(const dynamic_integer& src); // { this_t tmp(*this); assign_greater(src); return tmp; }
	this_t post_assign_greater(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_greater(src); return tmp; }
	this_t post_assign_greater(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.greater(t); }); }
	this_t post_assign_greater(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_greater(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_greater(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_greater(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_greater(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_greater(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_greater(tmp); }


	// lesser
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> lesser(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> lesser(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> fixed_integer<has_sign || has_sign2, ((bits >= bits2) ? (has_sign ? bits : bits2) : (has_sign2 ? bits2 : bits))> lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_extended<has_sign2, bits2>& src) const { return src.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src.lesser(*this); }
	template <bool has_sign2, size_t bits2> auto lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src.lesser(*this); }
	auto lesser(const dynamic_integer& src) const; // { return src.lesser(*this); }
	auto lesser(const dynamic_integer& src) const volatile; // { return src.lesser(*this); }
	auto lesser(const volatile dynamic_integer& src) const; // { return src.lesser(*this); }
	auto lesser(const volatile dynamic_integer& src) const volatile; // { return src.lesser(*this); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> auto lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> auto lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	auto lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return lesser(tmp); }

	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { cogs::assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_lesser(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { cogs::assign_lesser(m_int, src.get_int()); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { *this = src.lesser(*this); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { *this = src.lesser(*this); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.lesser(*this); }
	template <bool has_sign2, size_t bits2> void assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { *this = src.lesser(*this); }
	void assign_lesser(const dynamic_integer& src); // { *this = src.lesser(*this); }
	void assign_lesser(const volatile dynamic_integer& src); // { *this = src.lesser(*this); }
	void assign_lesser(const dynamic_integer& src) volatile; // { *this = src.lesser(*this); }
	void assign_lesser(const volatile dynamic_integer& src) volatile; // { *this = src.lesser(*this); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> void assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> void assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	void assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::pre_assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> const this_t& pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { assign_lesser(src); return *this; }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	template <bool has_sign2, size_t bits2> this_t pre_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	const this_t& pre_assign_lesser(const dynamic_integer& src); // { assign_lesser(src); return *this; }
	const this_t& pre_assign_lesser(const volatile dynamic_integer& src); // { assign_lesser(src); return *this; }
	this_t pre_assign_lesser(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_pre(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	this_t pre_assign_lesser(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return pre_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> const this_t& pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t pre_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> const this_t& pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t pre_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return pre_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	const this_t& pre_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t pre_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return pre_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_native<has_sign2, bits2>& src) volatile { return cogs::post_assign_lesser(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) { this_t tmp(*this); assign_lesser(src); return tmp; }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	template <bool has_sign2, size_t bits2> this_t post_assign_lesser(const volatile fixed_integer_extended<has_sign2, bits2>& src) volatile { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	this_t post_assign_lesser(const dynamic_integer& src); // { this_t tmp(*this); assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const volatile dynamic_integer& src); // { this_t tmp(*this); assign_lesser(src); return tmp; }
	this_t post_assign_lesser(const dynamic_integer& src) volatile; // { return atomic::compare_exchange_retry_loop_post(m_int, [&](const int_t& t) { return src.lesser(t); }); }
	this_t post_assign_lesser(const volatile dynamic_integer& src) volatile; // { dynamic_integer tmp(src); return post_assign_lesser(tmp); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> this_t post_assign_lesser(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> this_t post_assign_lesser(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return post_assign_lesser(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const volatile int_t2& i) { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	this_t post_assign_lesser(const volatile int_t2& i) volatile { int_to_fixed_integer_t<int_t2> tmp(i); return post_assign_lesser(tmp); }



	// equals
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const { return src == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src == *this; }
	template <bool has_sign2, size_t bits2> bool operator==(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src == *this; }
	bool operator==(const dynamic_integer& src) const; // { return src == *this; }
	bool operator==(const dynamic_integer& src) const volatile; // { return src == *this; }
	bool operator==(const volatile dynamic_integer& src) const; // { return src == *this; }
	bool operator==(const volatile dynamic_integer& src) const volatile; // { return src == *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator==(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this == tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator==(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this == tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator==(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this == tmp; }

	// ??  Work around a VS 2017 bug where unsigned long long is not recognized as integral
	bool operator==(const unsigned long long& i) const { int_to_fixed_integer_t<unsigned long long> tmp(i); return *this == tmp; }
	bool operator==(const unsigned long long& i) const volatile { int_to_fixed_integer_t<unsigned long long> tmp(i); return *this == tmp; }
	bool operator==(const volatile unsigned long long& i) const { int_to_fixed_integer_t<unsigned long long> tmp(i); return *this == tmp; }
	bool operator==(const volatile unsigned long long& i) const volatile { int_to_fixed_integer_t<unsigned long long> tmp(i); return *this == tmp; }


	// not_equals
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::not_equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::not_equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::not_equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::not_equals(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& src) const { return src != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src != *this; }
	template <bool has_sign2, size_t bits2> bool operator!=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src != *this; }
	bool operator!=(const dynamic_integer& src) const; // { return src != *this; }
	bool operator!=(const dynamic_integer& src) const volatile; // { return src != *this; }
	bool operator!=(const volatile dynamic_integer& src) const; // { return src != *this; }
	bool operator!=(const volatile dynamic_integer& src) const volatile; // { return src != *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator!=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this != tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator!=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this != tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator!=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this != tmp; }

	// is_less_than
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_less_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_less_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_less_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_less_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& src) const { return src > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src > *this; }
	template <bool has_sign2, size_t bits2> bool operator<(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src > *this; }
	bool operator<(const dynamic_integer& src) const; // { return src > *this; }
	bool operator<(const dynamic_integer& src) const volatile; // { return src > *this; }
	bool operator<(const volatile dynamic_integer& src) const; // { return src > *this; }
	bool operator<(const volatile dynamic_integer& src) const volatile; // { return src > *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this < tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this < tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this < tmp; }

	// is_greater_than
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_greater_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_greater_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_greater_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_greater_than(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& src) const { return src < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src < *this; }
	template <bool has_sign2, size_t bits2> bool operator>(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src < *this; }
	bool operator>(const dynamic_integer& src) const; // { return src < *this; }
	bool operator>(const dynamic_integer& src) const volatile; // { return src < *this; }
	bool operator>(const volatile dynamic_integer& src) const; // { return src < *this; }
	bool operator>(const volatile dynamic_integer& src) const volatile; // { return src < *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this > tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this > tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this > tmp; }

	// is_less_than_or_equal
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_less_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_less_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_less_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_less_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& src) const { return src >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src >= *this; }
	template <bool has_sign2, size_t bits2> bool operator<=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src >= *this; }
	bool operator<=(const dynamic_integer& src) const; // { return src >= *this; }
	bool operator<=(const dynamic_integer& src) const volatile; // { return src >= *this; }
	bool operator<=(const volatile dynamic_integer& src) const; // { return src >= *this; }
	bool operator<=(const volatile dynamic_integer& src) const volatile; // { return src >= *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator<=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this <= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator<=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this <= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator<=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this <= tmp; }

	// is_greater_than_or_equal
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_greater_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_greater_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::is_greater_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::is_greater_than_or_equal(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& src) const { return src <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return src <= *this; }
	template <bool has_sign2, size_t bits2> bool operator>=(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return src <= *this; }
	bool operator>=(const dynamic_integer& src) const; // { return src <= *this; }
	bool operator>=(const dynamic_integer& src) const volatile; // { return src <= *this; }
	bool operator>=(const volatile dynamic_integer& src) const; // { return src <= *this; }
	bool operator>=(const volatile dynamic_integer& src) const volatile; // { return src <= *this; }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> bool operator>=(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this >= tmp; }
	template <bool has_sign2, size_t bits2, ulongest... values2> bool operator>=(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return *this >= tmp; }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	bool operator>=(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return *this >= tmp; }

	// compare
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& src) const { return cogs::compare(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::compare(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& src) const { return cogs::compare(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_native<has_sign2, bits2>& src) const volatile { return cogs::compare(m_int, src.m_int); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& src) const { return -src.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const fixed_integer_extended<has_sign2, bits2>& src) const volatile { return -src.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& src) const { return -src.compare(*this); }
	template <bool has_sign2, size_t bits2> int compare(const volatile fixed_integer_extended<has_sign2, bits2>& src) const volatile { return -src.compare(*this); }
	int compare(const dynamic_integer& src) const; // { return -src.compare(*this); }
	int compare(const dynamic_integer& src) const volatile; // { return -src.compare(*this); }
	int compare(const volatile dynamic_integer& src) const; // { return -src.compare(*this); }
	int compare(const volatile dynamic_integer& src) const volatile; // { return -src.compare(*this); }

	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2> int compare(const volatile fixed_integer_native_const<has_sign2, bits2, value2>& src) const volatile { typename fixed_integer_native_const<has_sign2, bits2, value2>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }
	template <bool has_sign2, size_t bits2, ulongest... values2> int compare(const volatile fixed_integer_extended_const<has_sign2, bits2, values2...>& src) const volatile { typename fixed_integer_extended_const<has_sign2, bits2, values2...>::non_const_t tmp(src); return compare(tmp); }

	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }
	template <typename int_t2, typename = std::enable_if_t<std::is_integral_v<int_t2> > >
	int compare(const volatile int_t2& i) const volatile { int_to_fixed_integer_t<int_t2> tmp(i); return compare(tmp); }

	// swap
	void swap(this_t& wth) { cogs::swap(m_int, wth.m_int); }
	void swap(this_t& wth) volatile { cogs::swap(m_int, wth.m_int); }
	void swap(volatile this_t& wth) { cogs::swap(m_int, wth.m_int); }


	// exchange
	this_t exchange(const this_t& src) { this_t tmp; cogs::exchange(m_int, src.m_int, tmp.m_int); return tmp; }
	this_t exchange(const this_t& src) volatile { this_t tmp; cogs::exchange(m_int, src.m_int, tmp.m_int); return tmp; }

	this_t exchange(const volatile this_t& src) { this_t tmp; cogs::exchange(m_int, src.get_int(), tmp.m_int); return tmp; }
	this_t exchange(const volatile this_t& src) volatile { this_t tmp; cogs::exchange(m_int, src.get_int(), tmp.m_int); return tmp; }

	void exchange(const this_t& src, this_t& rtn) { cogs::exchange(m_int, src.m_int, rtn.m_int); }
	void exchange(const this_t& src, this_t& rtn) volatile { cogs::exchange(m_int, src.m_int, rtn.m_int); }

	void exchange(const volatile this_t& src, this_t& rtn) { cogs::exchange(m_int, src.get_int(), rtn.m_int); }
	void exchange(const volatile this_t& src, this_t& rtn) volatile { cogs::exchange(m_int, src.get_int(), rtn.m_int); }

	void exchange(const this_t& src, volatile this_t& rtn) { this_t tmp; cogs::exchange(m_int, src.m_int, tmp.m_int); rtn = tmp; }
	void exchange(const this_t& src, volatile this_t& rtn) volatile { this_t tmp; cogs::exchange(m_int, src.m_int, tmp.m_int); rtn = tmp; }

	void exchange(const volatile this_t& src, volatile this_t& rtn) { this_t tmp; cogs::exchange(m_int, src.get_int(), tmp.m_int); rtn = tmp; }
	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { this_t tmp; cogs::exchange(m_int, src.get_int(), tmp.m_int); rtn = tmp; }


	template <typename S>
	this_t exchange(S&& src)
	{
		this_t rtn = *this;
		cogs::assign(*this, std::forward<S>(src));
		return rtn;
	}

	template <typename S>
	this_t exchange(S&& src) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t rtn;
		atomic::exchange(m_int, tmpSrc.m_int, rtn.m_int);
		return rtn;
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn)
	{
		this_t tmp;
		cogs::assign(tmp, std::forward<S>(src));
		cogs::assign(rtn, *this);
		*this = tmp;
	}

	template <typename S, typename R>
	void exchange(S&& src, R& rtn) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpRtn;
		atomic::exchange(m_int, tmpSrc.m_int, tmpRtn.m_int);
		cogs::assign(rtn, tmpRtn);
	}



	// compare_exchange
	template <typename S, typename C>
	bool compare_exchange(S&& src, C&& cmp)
	{
		bool b = cogs::equals(m_int, std::forward<C>(cmp));
		if (b)
			cogs::assign(m_int, std::forward<S>(src));
		return b;
	}

	template <typename S, typename C>
	bool compare_exchange(S&& src, C&& cmp) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		return atomic::compare_exchange(m_int, tmpSrc.m_int, tmpCmp.m_int);
	}


	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn)
	{
		this_t tmp(*this);
		bool b = cogs::equals(tmp, std::forward<C>(cmp));
		if (b)
			cogs::assign(*this, std::forward<S>(src));
		cogs::assign(rtn, tmp);
		return b;
	}

	template <typename S, typename C, typename R>
	bool compare_exchange(S&& src, C&& cmp, R& rtn) volatile
	{
		this_t tmpSrc;
		cogs::assign(tmpSrc, std::forward<S>(src));
		this_t tmpCmp;
		cogs::assign(tmpCmp, std::forward<C>(cmp));
		this_t tmpRtn;
		bool b = atomic::compare_exchange(m_int, tmpSrc.m_int, tmpCmp.m_int, tmpRtn.m_int);
		cogs::assign(rtn, tmpRtn);
		return b;
	}


	void set_bit(size_t i) { assign_bit_or((int_t)((int_t)1 << i)); }
	void set_bit(size_t i) volatile { assign_bit_or((int_t)((int_t)1 << i)); }
	const this_t& pre_set_bit(size_t i) { return pre_assign_bit_or((int_t)((int_t)1 << i)); }
	this_t pre_set_bit(size_t i) volatile { return pre_assign_bit_or((int_t)((int_t)1 << i)); }
	this_t post_set_bit(size_t i) { return post_assign_bit_or((int_t)((int_t)1 << i)); }
	this_t post_set_bit(size_t i) volatile { return post_assign_bit_or((int_t)((int_t)1 << i)); }

	void set_bit(size_t i, bool b) { if (b) set_bit(i); else reset_bit(i); }
	void set_bit(size_t i, bool b) volatile { if (b) set_bit(i); else reset_bit(i); }
	const this_t& pre_set_bit(size_t i, bool b) { if (b) pre_set_bit(i); else pre_reset_bit(i); }
	this_t pre_set_bit(size_t i, bool b) volatile { if (b) pre_set_bit(i); else pre_reset_bit(i); }
	this_t post_set_bit(size_t i, bool b) { if (b) post_set_bit(i); else post_reset_bit(i); }
	this_t post_set_bit(size_t i, bool b) volatile { if (b) post_set_bit(i); else post_reset_bit(i); }

	void reset_bit(size_t i) { assign_bit_and((int_t)(~((int_t)1 << i))); }
	void reset_bit(size_t i) volatile { assign_bit_and((int_t)(~((int_t)1 << i))); }
	const this_t& pre_reset_bit(size_t i) { return pre_assign_bit_and((int_t)(~((int_t)1 << i))); }
	this_t pre_reset_bit(size_t i) volatile { return pre_assign_bit_and((int_t)(~((int_t)1 << i))); }
	this_t post_reset_bit(size_t i) { return post_assign_bit_and((int_t)(~((int_t)1 << i))); }
	this_t post_reset_bit(size_t i) volatile { return post_assign_bit_and((int_t)(~((int_t)1 << i))); }

	void invert_bit(size_t i) { assign_bit_xor((int_t)((int_t)1 << i)); }
	void invert_bit(size_t i) volatile { assign_bit_xor((int_t)((int_t)1 << i)); }
	const this_t& pre_invert_bit(size_t i) { return pre_assign_bit_xor((int_t)((int_t)1 << i)); }
	this_t pre_invert_bit(size_t i) volatile { return pre_assign_bit_xor((int_t)((int_t)1 << i)); }
	this_t post_invert_bit(size_t i) { return post_assign_bit_xor((int_t)((int_t)1 << i)); }
	this_t post_invert_bit(size_t i) volatile { return post_assign_bit_xor((int_t)((int_t)1 << i)); }

	bool test_bit(size_t i) const { return ((m_int & ((int_t)((int_t)1 << i))) != 0); }
	bool test_bit(size_t i) const volatile { return ((get_int() & ((int_t)((int_t)1 << i))) != 0); }

	static this_t max_value()
	{
		if (has_sign)
			return (signed_int_t)(~(unsigned_int_t)0 >> 1);
		return ~(int_t)0;
	}

	static this_t min_value()
	{
		if (has_sign)
			return (signed_int_t)(~(unsigned_int_t)0 >> 1) + 1;
		return 0;
	}



	template <typename char_t>
	string_t<char_t> to_string_t(unsigned int radix = 10, size_t minDigits = 1) const;

	string_t<wchar_t> to_string(int radix = 10, size_t minDigits = 1) const;

	string_t<char> to_cstring(int radix = 10, size_t minDigits = 1) const;

	template <endian_t e>
	io::buffer to_buffer() const;

	template <endian_t e>
	io::buffer to_buffer() const volatile;

	void randomize() { m_int = get_random_int<int_t>(); }
	void randomize() volatile { assign(get_random_int<int_t>()); }
};



template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2, ulongest... values2>
class compatible<fixed_integer_native<has_sign1, bits1>, fixed_integer_extended_const<has_sign2, bits2, values2...> >
{
public:
	typedef typename compatible<fixed_integer_extended_const<has_sign2, bits2, values2...>, fixed_integer_native<has_sign1, bits1> >::type type;
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2, bits_to_int_t<bits2, has_sign2> value2>
class compatible<fixed_integer_native<has_sign1, bits1>, fixed_integer_native_const<has_sign2, bits2, value2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2),
		(bits1 > bits2) ?
		(bits1 + ((has_sign2 && !has_sign1) ? 1 : 0))
		: (bits2 + ((has_sign1 && !has_sign2) ? 1 : 0))
		> type;
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_native<has_sign1, bits1>, fixed_integer_native<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), bits1 + ((has_sign1 && !has_sign2) ? 1 : 0)> type;
};

template <bool has_sign1, size_t bits1, bool has_sign2, size_t bits2>
class compatible<fixed_integer_native<has_sign1, bits1>, fixed_integer_extended<has_sign2, bits2> >
{
public:
	typedef fixed_integer<(has_sign1 || has_sign2), (bits1 > bits2 ? bits1 : bits2) + (((has_sign1 != has_sign2) && ((!has_sign1 && (bits1 >= bits2)) || (!has_sign2 && (bits2 >= bits1)))) ? 1 : 0)> type;
};

template <typename int_t2, bool has_sign2, size_t bits2>
class compatible<fixed_integer_native<has_sign2, bits2>, int_t2, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<fixed_integer_native<has_sign2, bits2>, int_to_fixed_integer_t<int_t2> >::type type;
};

template <typename int_t2, bool has_sign2, size_t bits2>
class compatible<int_t2, fixed_integer_native<has_sign2, bits2>, std::enable_if_t<std::is_integral_v<int_t2> > >
{
public:
	typedef typename compatible<int_to_fixed_integer_t<int_t2>, fixed_integer_native<has_sign2, bits2> >::type type;
};

template <bool has_sign, size_t bits>
class compatible<fixed_integer_native<has_sign, bits>, dynamic_integer>
{
public:
	typedef dynamic_integer type;
};



template <typename int_t2, typename int_t3>
class compatible<int_t2, int_t3, std::enable_if_t<std::is_integral_v<int_t2> && std::is_integral_v<int_t3> > >
{
public:
	typedef decltype(
		std::declval<
			typename compatible<
				int_to_fixed_integer_t<int_t2>,
				int_to_fixed_integer_t<int_t3>
			>::type
		>().simplify_type()
	) type;
};




typedef fixed_integer_native<false, 1> bit_t;

typedef int_to_fixed_integer_t<size_t> size_type;
typedef int_to_fixed_integer_t<ptrdiff_t> ptrdiff_type;

typedef int_to_fixed_integer_t<int> int_type;
typedef int_to_fixed_integer_t<unsigned int> uint_type;

typedef int_to_fixed_integer_t<short> short_type;
typedef int_to_fixed_integer_t<unsigned short> ushort_type;

typedef int_to_fixed_integer_t<long> long_type;
typedef int_to_fixed_integer_t<unsigned long> ulong_type;

typedef int_to_fixed_integer_t<long long> longlong_type;
typedef int_to_fixed_integer_t<unsigned long long> ulonglong_type;

typedef int_to_fixed_integer_t<int8_t> int8_type;
typedef int_to_fixed_integer_t<uint8_t> uint8_type;

typedef int_to_fixed_integer_t<int16_t> int16_type;
typedef int_to_fixed_integer_t<uint16_t> uint16_type;

typedef int_to_fixed_integer_t<int32_t> int32_type;
typedef int_to_fixed_integer_t<uint32_t> uint32_type;


#if COGS_LONGEST_INT >= 8

typedef int_to_fixed_integer_t<int64_t> int64_type;
typedef int_to_fixed_integer_t<uint64_t> uint64_type;

#if COGS_LONGEST_INT >= 16

typedef int_to_fixed_integer_t<bytes_to_int_t<16> > int128_type;
typedef int_to_fixed_integer_t<bytes_to_uint_t<16> > uint128_type;

#endif
#endif


typedef int_to_fixed_integer_t<longest> longest_type;
typedef int_to_fixed_integer_t<ulongest> ulongest_type;




#pragma warning(pop)


}



#endif
