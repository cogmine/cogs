//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_CRYPTO_MD4
#define COGS_HEADER_CRYPTO_MD4


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"
#include "cogs/env/mem/bit_rotate.hpp"


namespace cogs {
namespace crypto {


#ifdef DOXYGEN

/// @ingroup Crypto
/// @brief MD4 Message-Digest Algorithm
class md4 : public hash
{
};

#else

class md4 : public serial_hash<128, 128, 32, endian_t::little, 512>
{
private:
	typedef serial_hash<128, 128, 32, endian_t::little, 512> base_t;

	uint64_t m_bitCount;
	digit_t m_state[16];

	static uint32_t F(uint32_t b, uint32_t c, uint32_t d) { return (d ^ (b & (c ^ d))); }
	static uint32_t G(uint32_t b, uint32_t c, uint32_t d) { return (b & c) | (b & d) | (c & d); }
	static uint32_t H(uint32_t b, uint32_t c, uint32_t d) { return b ^ c ^ d; }

	static void r1(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s)
	{
		a = bit_rotate_left(a + F(b, c, d) + x, s);
	}

	static void r2(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s)
	{
		a = bit_rotate_left(a + G(b, c, d) + x + 0x5a827999, s);
	}

	static void r3(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s)
	{
		a = bit_rotate_left(a + H(b, c, d) + x + 0x6ed9eba1, s);
	}

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += stride_bits;

		uint32_t a = m_result[0];
		uint32_t b = m_result[1];
		uint32_t c = m_result[2];
		uint32_t d = m_result[3];

		r1(a, b, c, d, m_state[0], 3);
		r1(d, a, b, c, m_state[1], 7);
		r1(c, d, a, b, m_state[2], 11);
		r1(b, c, d, a, m_state[3], 19);
		r1(a, b, c, d, m_state[4], 3);
		r1(d, a, b, c, m_state[5], 7);
		r1(c, d, a, b, m_state[6], 11);
		r1(b, c, d, a, m_state[7], 19);
		r1(a, b, c, d, m_state[8], 3);
		r1(d, a, b, c, m_state[9], 7);
		r1(c, d, a, b, m_state[10], 11);
		r1(b, c, d, a, m_state[11], 19);
		r1(a, b, c, d, m_state[12], 3);
		r1(d, a, b, c, m_state[13], 7);
		r1(c, d, a, b, m_state[14], 11);
		r1(b, c, d, a, m_state[15], 19);

		r2(a, b, c, d, m_state[0], 3);
		r2(d, a, b, c, m_state[4], 5);
		r2(c, d, a, b, m_state[8], 9);
		r2(b, c, d, a, m_state[12], 13);
		r2(a, b, c, d, m_state[1], 3);
		r2(d, a, b, c, m_state[5], 5);
		r2(c, d, a, b, m_state[9], 9);
		r2(b, c, d, a, m_state[13], 13);
		r2(a, b, c, d, m_state[2], 3);
		r2(d, a, b, c, m_state[6], 5);
		r2(c, d, a, b, m_state[10], 9);
		r2(b, c, d, a, m_state[14], 13);
		r2(a, b, c, d, m_state[3], 3);
		r2(d, a, b, c, m_state[7], 5);
		r2(c, d, a, b, m_state[11], 9);
		r2(b, c, d, a, m_state[15], 13);

		r3(a, b, c, d, m_state[0], 3);
		r3(d, a, b, c, m_state[8], 9);
		r3(c, d, a, b, m_state[4], 11);
		r3(b, c, d, a, m_state[12], 15);
		r3(a, b, c, d, m_state[2], 3);
		r3(d, a, b, c, m_state[10], 9);
		r3(c, d, a, b, m_state[6], 11);
		r3(b, c, d, a, m_state[14], 15);
		r3(a, b, c, d, m_state[1], 3);
		r3(d, a, b, c, m_state[9], 9);
		r3(c, d, a, b, m_state[5], 11);
		r3(b, c, d, a, m_state[13], 15);
		r3(a, b, c, d, m_state[3], 3);
		r3(d, a, b, c, m_state[11], 9);
		r3(c, d, a, b, m_state[7], 11);
		r3(b, c, d, a, m_state[15], 15);

		m_result[0] += a;
		m_result[1] += b;
		m_result[2] += c;
		m_result[3] += d;
	}

	void terminate()
	{
		uint64_t bitCount = m_bitCount + (m_blockProgress * digit_bits) + m_digitProgress;

		// Add terminating 0x80 byte.  Might cause this block to process, if last byte.
		add_byte(0x80);

		// Finish a current digit in progress, if any
		if (m_digitProgress != 0)
			process_digit();

		// We need 2 digits for the length.  If not enough space, add padding to this block, start a new one.
		while (m_blockProgress != stride_digits - 2)
			process_digit();

		m_state[stride_digits - 2] = (digit_t)bitCount;
		m_state[stride_digits - 1] = (digit_t)(bitCount >> digit_bits);
		process_block();
	}

public:
	md4()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
	}

	md4(const md4& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	md4& operator=(const md4& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};

#endif


}
}


#endif
