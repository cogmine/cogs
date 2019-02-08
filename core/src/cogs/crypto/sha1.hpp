//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_SHA1
#define COGS_SHA1


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


#ifdef DOXYGEN

/// @ingroup Crypto
/// @brief SHA-1 (Secure Hash Algorithm 1)
class sha1 : public hash
{
};

#else

/// @ingroup Crypto
/// @brief SHA-1 (Secure Hash Algorithm 1)
class sha1 : public serial_hash<sha1, 160, 160, 32, endian_t::big, 512>
{
private:
	typedef serial_hash<sha1, 160, 160, 32, endian_t::big, 512> base_t;

	template <class derived_t, size_t, size_t, size_t, endian_t, size_t, size_t>
	friend class serial_hash;

	uint64_t	m_bitCount;
	digit_t		m_state[80];
		
	static uint32_t ch(uint32_t x, uint32_t y, uint32_t z)		{ return (z ^ (x & (y ^ z))); }
	static uint32_t maj(uint32_t x, uint32_t y, uint32_t z)		{ return ((x & y) | (z & (x | y))); }
	static uint32_t parity(uint32_t x, uint32_t y, uint32_t z)	{ return (x ^ y ^ z); }

	uint32_t R(uint8_t i)	{ return (m_state[i] = bit_rotate_left((m_state[i - 3] ^ m_state[i - 8] ^ m_state[i - 14] ^ m_state[i - 16]), 1)); }

	void r0(uint32_t a, uint32_t& b, uint32_t c, uint32_t d, uint32_t& e, uint8_t i)
	{
		e += ch(b, c, d) + m_state[i] + 0x5A827999 + bit_rotate_left(a, 5);
		b = bit_rotate_left(b, 30);
	}

	void r1(uint32_t a, uint32_t& b, uint32_t c, uint32_t d, uint32_t& e, uint8_t i)
	{
		e += ch(b, c, d) + R(i) + 0x5A827999 + bit_rotate_left(a, 5);
		b = bit_rotate_left(b, 30);
	}

	void r2(uint32_t a, uint32_t& b, uint32_t c, uint32_t d, uint32_t& e, uint8_t i)
	{
		e += parity(b, c, d) + R(i) + 0x6ED9EBA1 + bit_rotate_left(a, 5);
		b = bit_rotate_left(b, 30);
	}

	void r3(uint32_t a, uint32_t& b, uint32_t c, uint32_t d, uint32_t& e, uint8_t i)
	{
		e += maj(b, c, d) + R(i) + 0x8F1BBCDC + bit_rotate_left(a, 5);
		b = bit_rotate_left(b, 30);
	}

	void r4(uint32_t a, uint32_t& b, uint32_t c, uint32_t d, uint32_t& e, uint8_t i)
	{
		e += parity(b, c, d) + R(i) + 0xCA62C1D6 + bit_rotate_left(a, 5);
		b = bit_rotate_left(b, 30);
	}

protected:
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
		uint32_t e = m_result[4];

		r0(a, b, c, d, e, 0);  r0(e, a, b, c, d, 1);  r0(d, e, a, b, c, 2);  r0(c, d, e, a, b, 3);
		r0(b, c, d, e, a, 4);  r0(a, b, c, d, e, 5);  r0(e, a, b, c, d, 6);  r0(d, e, a, b, c, 7);
		r0(c, d, e, a, b, 8);  r0(b, c, d, e, a, 9);  r0(a, b, c, d, e, 10); r0(e, a, b, c, d, 11);
		r0(d, e, a, b, c, 12); r0(c, d, e, a, b, 13); r0(b, c, d, e, a, 14); r0(a, b, c, d, e, 15);
		r1(e, a, b, c, d, 16); r1(d, e, a, b, c, 17); r1(c, d, e, a, b, 18); r1(b, c, d, e, a, 19);
		r2(a, b, c, d, e, 20); r2(e, a, b, c, d, 21); r2(d, e, a, b, c, 22); r2(c, d, e, a, b, 23);
		r2(b, c, d, e, a, 24); r2(a, b, c, d, e, 25); r2(e, a, b, c, d, 26); r2(d, e, a, b, c, 27);
		r2(c, d, e, a, b, 28); r2(b, c, d, e, a, 29); r2(a, b, c, d, e, 30); r2(e, a, b, c, d, 31);
		r2(d, e, a, b, c, 32); r2(c, d, e, a, b, 33); r2(b, c, d, e, a, 34); r2(a, b, c, d, e, 35);
		r2(e, a, b, c, d, 36); r2(d, e, a, b, c, 37); r2(c, d, e, a, b, 38); r2(b, c, d, e, a, 39);
		r3(a, b, c, d, e, 40); r3(e, a, b, c, d, 41); r3(d, e, a, b, c, 42); r3(c, d, e, a, b, 43);
		r3(b, c, d, e, a, 44); r3(a, b, c, d, e, 45); r3(e, a, b, c, d, 46); r3(d, e, a, b, c, 47);
		r3(c, d, e, a, b, 48); r3(b, c, d, e, a, 49); r3(a, b, c, d, e, 50); r3(e, a, b, c, d, 51);
		r3(d, e, a, b, c, 52); r3(c, d, e, a, b, 53); r3(b, c, d, e, a, 54); r3(a, b, c, d, e, 55);
		r3(e, a, b, c, d, 56); r3(d, e, a, b, c, 57); r3(c, d, e, a, b, 58); r3(b, c, d, e, a, 59);
		r4(a, b, c, d, e, 60); r4(e, a, b, c, d, 61); r4(d, e, a, b, c, 62); r4(c, d, e, a, b, 63);
		r4(b, c, d, e, a, 64); r4(a, b, c, d, e, 65); r4(e, a, b, c, d, 66); r4(d, e, a, b, c, 67);
		r4(c, d, e, a, b, 68); r4(b, c, d, e, a, 69); r4(a, b, c, d, e, 70); r4(e, a, b, c, d, 71);
		r4(d, e, a, b, c, 72); r4(c, d, e, a, b, 73); r4(b, c, d, e, a, 74); r4(a, b, c, d, e, 75);
		r4(e, a, b, c, d, 76); r4(d, e, a, b, c, 77); r4(c, d, e, a, b, 78); r4(b, c, d, e, a, 79);

		m_result[0] += a;
		m_result[1] += b;
		m_result[2] += c;
		m_result[3] += d;
		m_result[4] += e;
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

		m_state[stride_digits - 2] = (digit_t)(bitCount >> digit_bits);
		m_state[stride_digits - 1] = (digit_t)bitCount;
		process_block();
	}

public:
	sha1()
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301;
		m_result[1] = 0xefcdab89;
		m_result[2] = 0x98badcfe;
		m_result[3] = 0x10325476;
		m_result[4] = 0xc3d2e1f0;
	}

	sha1(const sha1& src)
		: base_t(src)
	{
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	sha1& operator=(const sha1& src)
	{
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
