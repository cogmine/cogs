//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_MD5
#define COGS_HEADER_CRYPTO_MD5


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


#ifdef DOXYGEN


/// @ingroup Crypto
/// @brief MD5 Message-Digest Algorithm
class md5 : public hash
{
};


#else


class md5 : public serial_hash<160, 128, 32, endian_t::little, 512>
{
private:
	typedef serial_hash<160, 128, 32, endian_t::little, 512> base_t;

	uint64_t	m_bitCount;
	digit_t		m_state[16];

	static uint32_t F(uint32_t b, uint32_t c, uint32_t d)	{ return (d ^ (b & (c ^ d))); }
	static uint32_t G(uint32_t b, uint32_t c, uint32_t d)	{ return (c ^ (d & (b ^ c))); }
	static uint32_t H(uint32_t b, uint32_t c, uint32_t d)	{ return b ^ c ^ d; }
	static uint32_t I(uint32_t b, uint32_t c, uint32_t d)	{ return c ^ (b | ~d); }
	
	static void r1(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t data, uint32_t k, uint32_t s)
	{
		a = bit_rotate_left(a + F(b, c, d) + data + k, s) + b;
	}

	static void r2(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t data, uint32_t k, uint32_t s)
	{
		a = bit_rotate_left(a + G(b, c, d) + data + k, s) + b;
	}

	static void r3(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t data, uint32_t k, uint32_t s)
	{
		a = bit_rotate_left(a + H(b, c, d) + data + k, s) + b;
	}

	static void r4(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t data, uint32_t k, uint32_t s)
	{
		a = bit_rotate_left(a + I(b, c, d) + data + k, s) + b;
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

		r1(a, b, c, d, m_state[0], 0xd76aa478, 7);
		r1(d, a, b, c, m_state[1], 0xe8c7b756, 12);
		r1(c, d, a, b, m_state[2], 0x242070db, 17);
		r1(b, c, d, a, m_state[3], 0xc1bdceee, 22);
		r1(a, b, c, d, m_state[4], 0xf57c0faf, 7);
		r1(d, a, b, c, m_state[5], 0x4787c62a, 12);
		r1(c, d, a, b, m_state[6], 0xa8304613, 17);
		r1(b, c, d, a, m_state[7], 0xfd469501, 22);
		r1(a, b, c, d, m_state[8], 0x698098d8, 7);
		r1(d, a, b, c, m_state[9], 0x8b44f7af, 12);
		r1(c, d, a, b, m_state[10], 0xffff5bb1, 17);
		r1(b, c, d, a, m_state[11], 0x895cd7be, 22);
		r1(a, b, c, d, m_state[12], 0x6b901122, 7);
		r1(d, a, b, c, m_state[13], 0xfd987193, 12);
		r1(c, d, a, b, m_state[14], 0xa679438e, 17);
		r1(b, c, d, a, m_state[15], 0x49b40821, 22);

		r2(a, b, c, d, m_state[1], 0xf61e2562, 5);
		r2(d, a, b, c, m_state[6], 0xc040b340, 9);
		r2(c, d, a, b, m_state[11], 0x265e5a51, 14);
		r2(b, c, d, a, m_state[0], 0xe9b6c7aa, 20);
		r2(a, b, c, d, m_state[5], 0xd62f105d, 5);
		r2(d, a, b, c, m_state[10], 0x02441453, 9);
		r2(c, d, a, b, m_state[15], 0xd8a1e681, 14);
		r2(b, c, d, a, m_state[4], 0xe7d3fbc8, 20);
		r2(a, b, c, d, m_state[9], 0x21e1cde6, 5);
		r2(d, a, b, c, m_state[14], 0xc33707d6, 9);
		r2(c, d, a, b, m_state[3], 0xf4d50d87, 14);
		r2(b, c, d, a, m_state[8], 0x455a14ed, 20);
		r2(a, b, c, d, m_state[13], 0xa9e3e905, 5);
		r2(d, a, b, c, m_state[2], 0xfcefa3f8, 9);
		r2(c, d, a, b, m_state[7], 0x676f02d9, 14);
		r2(b, c, d, a, m_state[12], 0x8d2a4c8a, 20);

		r3(a, b, c, d, m_state[5], 0xfffa3942, 4);
		r3(d, a, b, c, m_state[8], 0x8771f681, 11);
		r3(c, d, a, b, m_state[11], 0x6d9d6122, 16);
		r3(b, c, d, a, m_state[14], 0xfde5380c, 23);
		r3(a, b, c, d, m_state[1], 0xa4beea44, 4);
		r3(d, a, b, c, m_state[4], 0x4bdecfa9, 11);
		r3(c, d, a, b, m_state[7], 0xf6bb4b60, 16);
		r3(b, c, d, a, m_state[10], 0xbebfbc70, 23);
		r3(a, b, c, d, m_state[13], 0x289b7ec6, 4);
		r3(d, a, b, c, m_state[0], 0xeaa127fa, 11);
		r3(c, d, a, b, m_state[3], 0xd4ef3085, 16);
		r3(b, c, d, a, m_state[6], 0x04881d05, 23);
		r3(a, b, c, d, m_state[9], 0xd9d4d039, 4);
		r3(d, a, b, c, m_state[12], 0xe6db99e5, 11);
		r3(c, d, a, b, m_state[15], 0x1fa27cf8, 16);
		r3(b, c, d, a, m_state[2], 0xc4ac5665, 23);

		r4(a, b, c, d, m_state[0], 0xf4292244, 6);
		r4(d, a, b, c, m_state[7], 0x432aff97, 10);
		r4(c, d, a, b, m_state[14], 0xab9423a7, 15);
		r4(b, c, d, a, m_state[5], 0xfc93a039, 21);
		r4(a, b, c, d, m_state[12], 0x655b59c3, 6);
		r4(d, a, b, c, m_state[3], 0x8f0ccc92, 10);
		r4(c, d, a, b, m_state[10], 0xffeff47d, 15);
		r4(b, c, d, a, m_state[1], 0x85845dd1, 21);
		r4(a, b, c, d, m_state[8], 0x6fa87e4f, 6);
		r4(d, a, b, c, m_state[15], 0xfe2ce6e0, 10);
		r4(c, d, a, b, m_state[6], 0xa3014314, 15);
		r4(b, c, d, a, m_state[13], 0x4e0811a1, 21);
		r4(a, b, c, d, m_state[4], 0xf7537e82, 6);
		r4(d, a, b, c, m_state[11], 0xbd3af235, 10);
		r4(c, d, a, b, m_state[2], 0x2ad7d2bb, 15);
		r4(b, c, d, a, m_state[9], 0xeb86d391, 21);

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
		if (base_t::m_digitProgress != 0)
			process_digit();

		// We need 2 digits for the length.  If not enough space, add padding to this block, start a new one.
		while (base_t::m_blockProgress != stride_digits - 2)
			process_digit();

		m_state[stride_digits - 2] = (digit_t)bitCount;
		m_state[stride_digits - 1] = (digit_t)(bitCount >> digit_bits);
		process_block();
	}

public:
	md5()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
	}

	md5(const md5& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	md5& operator=(const md5& src)
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
