//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_FLETCHER
#define COGS_HEADER_CRYPTO_FLETCHER


#include "cogs/crypto/hash_int.hpp"
#include "cogs/math/extumul.hpp"
#include "cogs/math/const_uroot.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief Fletcher Checksum Algorithm
/// @tparam bits Bit width
template <size_t bits>
class fletcher : public hash_int<bits>
{
public:
	typedef typename hash_int<bits>::uint_t uint_t;

private:
	static constexpr size_t width = bits;
	static constexpr size_t sum_width = width / 2;

	// x
	static constexpr size_t accumulator_width = ((sizeof(unsigned int) * 8) > width) ? width : (sizeof(unsigned int) * 8);

	// y	// Will be smaller than the largest int, since sizeof(width) must be an int
	static constexpr size_t register_width = (width > (sizeof(unsigned int) * 8)) ? (width / 2) : (accumulator_width / 2);

	typedef bits_to_uint_t<width>					result_t;
	typedef bits_to_uint_t<sum_width>				sum_t;
	typedef bits_to_uint_t<register_width>			register_t;
	typedef bits_to_uint_t<accumulator_width>		accumulator_t;
	typedef bits_to_uint_t<accumulator_width * 2>	larger_t;

	static constexpr accumulator_t mod_by = ((ulongest)1 << sum_width) - 1;
	static constexpr sum_t init_value = (sum_t)(((ulongest)1 << register_width) - 1);

	// The width of the fletcher algorithm implies the size of the sums (each width/2)
	// as well as the size of the input words (also, width/2).
	//
	// A sum accumulator that is larger than the sum word can be used to
	// optimize the inner loop by making the mod necesssary less frequently.
	//
	// In order for the inner loop to be efficient, the sum accumulator should not be
	// larger than a native int, as emulation of a larger int could incur significantly more
	// overhead than calling mod more frequently.

	template <bool too_large = (width > (sizeof(unsigned int) * 8)), bool unused = true>
	class helper
	{
	public:
		// 2 ^ register_width
		static constexpr ulongest y = (ulongest)1 << register_width;
		
		// 2 ^ width
		static constexpr ulongest x = (ulongest)1 << accumulator_width;

		static constexpr ulongest high_x = (sizeof(ulongest) == (accumulator_width / 8)) ? 1 : 0;
		static constexpr ulongest low_x = (sizeof(ulongest) == (accumulator_width / 8)) ? 0 : ((ulongest)1 << accumulator_width);

		// Calculate max loop iterations before a mod is needed
		//
		//	n = (sqrt((y-1) * (8x + 25 * (y-1))) - (5 * (y-1))) / y

		static constexpr ulongest a = y - 1;				// i.e. 0x0000FFFF

		//	n = (sqrt(a * (8x + 25a)) - 5a) / y

		static constexpr ulongest high_b = (sizeof(ulongest) == (accumulator_width / 8)) ? 8 : 0;
		static constexpr ulongest low_b = (a * 25) + ((sizeof(ulongest) == (accumulator_width / 8)) ? 0 : (8 * x));	// will not overflow

		//	n = (sqrt(ab) - 5a) / y

		static constexpr ulongest high_ab = const_extumul2<high_b, low_b, 0, a>::high_part;
		static constexpr ulongest low_ab = const_extumul2<high_b, low_b, 0, a>::low_part;

		static constexpr ulongest root = const_uroot_v<2, low_ab, high_ab>;

		static constexpr ulongest loop_max = ((sum_width >= (sizeof(accumulator_t) * 8)) ? 0 : ((root - (a * 5)) >> register_width)) / (accumulator_width / register_width);
	};

	template <bool unused>
	class helper<true, unused>
	{
	public:
		static constexpr ulongest y = 0;
		static constexpr ulongest x = 0;
		static constexpr ulongest high_x = 0;
		static constexpr ulongest low_x = 0;
		static constexpr ulongest a = 0;
		static constexpr ulongest high_b = 0;
		static constexpr ulongest low_b = 0;
		static constexpr ulongest high_ab = 0;
		static constexpr ulongest low_ab = 0;
		static constexpr ulongest root = 0;

		static constexpr ulongest loop_max = 0;
	};

	static constexpr ulongest loop_max = helper<>::loop_max;

	sum_t m_sum1;
	sum_t m_sum2;
	register_t m_carry;
	unsigned int m_carryBits;

	bool get_register(register_t& w, const uint8_t*& p, size_t& remaining)
	{
		if (!remaining)
			return false;

		if (width == 8)
		{
			if (m_carryBits != 0)
			{
				m_carryBits = 0;
				w = m_carry;
				remaining--;
			}
			else
			{
				w = *p++;
				m_carry = w >> 4;
				m_carryBits = 4;
				w &= 0x0f;
			}
			return true;
		}

		if (register_width <= 8)
		{
			w = *p++;
			remaining--;
			return true;
		}

		register_t result;

		// Finish current carry
		unsigned int carryBits = m_carryBits;
		if (carryBits > 0)
		{
			result = m_carry;
			for (;;)
			{
				remaining--;
				result |= (register_t)(*p++) << carryBits;
				if (carryBits == register_width - 8)
				{
					w = result;
					m_carry = 0;
					m_carryBits = 0;
					return true;
				}
				carryBits += 8;
				if (!remaining)
				{
					m_carryBits = carryBits;
					m_carry = result;
					return false;
				}
			}
		}

		result = (register_t)(*p++);
		if (remaining < sizeof(register_t))
		{
			for (;;)
			{
				carryBits += 8;
				if (!--remaining)
					break;
				result |= ((register_t)(*p++) << carryBits);
			}
			m_carryBits = carryBits;
			m_carry = result;
			return false;
		}

		for (;;)
		{
			if (carryBits == register_width - 8)
				break;
			carryBits += 8;
			result |= (register_t)(*p++) << carryBits;
		}

		w = result;
		remaining -= register_width / 8;
		return true;
	}

	void finalize(sum_t& s1, sum_t& s2) const
	{
		static constexpr size_t sum_width2 = (sizeof(accumulator_t) == sizeof(register_t)) ? 1 : sum_width;
		if (accumulator_width == register_width)
		{
			larger_t sum1 = m_sum1;
			larger_t sum2 = m_sum2;

			if (m_carryBits != 0)
				sum2 += sum1 += m_carry;

			sum1 %= mod_by;
			sum2 %= mod_by;

			s1 = (sum_t)sum1;
			s2 = (sum_t)sum2;
		}
		else
		{
			accumulator_t sum1 = m_sum1;
			accumulator_t sum2 = m_sum2;
			if (register_width > 8)
			{
				if (m_carryBits != 0)
				{
					sum2 += sum1 += m_carry;

					// half the bit shifts until == sum_width
					size_t shiftBy = sizeof(accumulator_t) * 4;
					for (;;)
					{
						accumulator_t mask = (1 << shiftBy) - 1;
						sum1 = (sum1 & mask) + (sum1 >> shiftBy);
						sum2 = (sum2 & mask) + (sum2 >> shiftBy);
						if (shiftBy == sum_width)
						{
							sum1 = (sum1 & mask) + (sum1 >> sum_width2);
							sum2 = (sum2 & mask) + (sum2 >> sum_width2);
							//if (sum1 >= mod_by)
							//{
							//	sum1 -= mod_by;
							//	if (sum1 >= mod_by)
							//		sum1 -= mod_by;
							//}
							//if (sum2 >= mod_by)
							//{
							//	sum2 -= mod_by;
							//	if (sum2 >= mod_by)
							//		sum2 -= mod_by;
							//}
								
							break;
						}
						shiftBy >>= 1;
					}
				}
			}

			sum1 %= mod_by;
			sum2 %= mod_by;

			s1 = (sum_t)sum1;
			s2 = (sum_t)sum2;
		}
	}

public:
	static constexpr uint_t success_result = 0;
	static constexpr uint_t null_result = 0;

	virtual size_t get_block_size() const { return 1; }

	virtual void update(const io::buffer& buf)
	{
		size_t bufLength = buf.get_length();
		if (bufLength > 0)
		{
			const uint8_t* p = (const uint8_t*)buf.get_const_ptr();
			accumulator_t sum1 = m_sum1;
			accumulator_t sum2 = m_sum2;
			if (accumulator_width == register_width)
			{
				for (;;)
				{
					register_t r;
					if (!get_register(r, p, bufLength))
						break;
					sum1 = (accumulator_t)(((larger_t)sum1 + r) % mod_by);
					sum2 = (accumulator_t)(((larger_t)sum2 + sum1) % mod_by);
				}
			}
			else
			{
				size_t i = 0;
				size_t shiftBy;
				for (;;)
				{
					register_t r;
					if (!get_register(r, p, bufLength))
						break;
					sum2 += sum1 += r;
					if (++i == loop_max)
					{
						i = 0;
						
						// half the bit shifts until == sum_width
						shiftBy = sizeof(accumulator_t) * 4;
						for (;;)
						{
							accumulator_t mask = (1 << shiftBy) - 1;
							sum1 = (sum1 & mask) + (sum1 >> shiftBy);
							sum2 = (sum2 & mask) + (sum2 >> shiftBy);
							if (shiftBy == sum_width)
								break;
							shiftBy >>= 1;
						}
					}
				}

				// half the bit shifts until == sum_width
				shiftBy = sizeof(accumulator_t) * 4;
				for (;;)
				{
					accumulator_t mask = (1 << shiftBy) - 1;
					sum1 = (sum1 & mask) + (sum1 >> shiftBy);
					sum2 = (sum2 & mask) + (sum2 >> shiftBy);
					if (shiftBy == sum_width)
					{
						static constexpr size_t sum_width2 = (accumulator_width == register_width) ? 1 : sum_width;
						sum1 = (sum1 & mask) + (sum1 >> sum_width2);
						sum2 = (sum2 & mask) + (sum2 >> sum_width2);
						break;
					}

					shiftBy >>= 1;
				}
			}

			m_sum1 = (sum_t)sum1;
			m_sum2 = (sum_t)sum2;
		}
	}

	virtual bool can_terminate() const	{ return true; }

	virtual bool was_block_terminated() const
	{
		sum_t sum1;
		sum_t sum2;
		finalize(sum1, sum2);
		return !sum1 && !sum2;
	}

	uint_t peek()
	{
		sum_t sum1;
		sum_t sum2;
		finalize(sum1, sum2);
		return ((result_t)sum2 << sum_width) | sum1;
	}

	virtual uint_t get_hash_int()
	{ 
		sum_t sum1;
		sum_t sum2;
		finalize(sum1, sum2);
		sum_t s1 = (sum_t)(mod_by - (((result_t)sum1 + sum2) % mod_by));
		sum_t s2 = (sum_t)(mod_by - (((result_t)sum1 + s1) % mod_by));
		return ((result_t)s2 << sum_width) | s1;
	}

	virtual io::buffer get_hash()
	{
		sum_t sum1;
		sum_t sum2;
		finalize(sum1, sum2);
		sum_t s1 = (sum_t)(mod_by - (((result_t)sum1 + sum2) % mod_by));
		sum_t s2 = (sum_t)(mod_by - (((result_t)sum1 + s1) % mod_by));
		fixed_integer<false, sizeof(uint_t) * 8> n = ((result_t)s2 << sum_width) | s1;
		return n.template to_buffer<endian_t::little>();
	}

	fletcher()
	{
		if (sizeof(accumulator_t) == sizeof(register_t))
		{
			m_sum1 = 0;
			m_sum2 = 0;
		}
		else
		{
			m_sum1 = init_value;
			m_sum2 = init_value;
		}
		if ((width == 8) || (width > 16))
		{
			m_carry = 0;
			m_carryBits = 0;
		}
	}

	fletcher(const fletcher<width>& src)
	{
		m_sum1 = src.m_sum1;
		m_sum2 = src.m_sum2;
		if (width > 16)
		{
			m_carry = src.m_carry;
			m_carryBits = src.m_carryBits;
		}
		if (width == 8) 
		{
			m_carry = 0;
			m_carryBits = 0;
		}
	}

	fletcher<width>& operator=(const fletcher<width>& src)
	{
		m_sum1 = src.m_sum1;
		m_sum2 = src.m_sum2;
		if (width > 16)
		{
			m_carry = src.m_carry;
			m_carryBits = src.m_carryBits;
		}
		return *this;
	}
};


}
}


#endif
