//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_RIPEMD
#define COGS_HEADER_CRYPTO_RIPEMD


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief RIPEMD (RACE Integrity Primitives Evaluation Message Digest) Algorithm
/// @tparam bits Bit width
template <size_t bits>
class ripemd : public hash
{
private:
	ripemd() = delete; // Not allowed.  This template is specialized for supported bit sizes
};



class ripemd_base
{
protected:
	static uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x ^ y ^ z); }
	static uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (z ^ (x & (y ^ z))); }
	static uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return (z ^ (x | ~y)); }
	static uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return (y ^ (z & (x ^ y))); }
	static uint32_t J(uint32_t x, uint32_t y, uint32_t z) { return (x ^ (y | ~z)); }

	static void subround_160_320_F(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + F(b, c, d) + x + k, s) + e;
		c = bit_rotate_left(c, 10);
	}

	static void subround_160_320_G(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + G(b, c, d) + x + k, s) + e;
		c = bit_rotate_left(c, 10);
	}

	static void subround_160_320_H(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + H(b, c, d) + x + k, s) + e;
		c = bit_rotate_left(c, 10);
	}

	static void subround_160_320_I(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + I(b, c, d) + x + k, s) + e;
		c = bit_rotate_left(c, 10);
	}

	static void subround_160_320_J(uint32_t& a, uint32_t b, uint32_t& c, uint32_t d, uint32_t e, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + J(b, c, d) + x + k, s) + e;
		c = bit_rotate_left(c, 10);
	}


	static void subround_128_256_F(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + F(b, c, d) + x + k, s);
	}

	static void subround_128_256_G(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + G(b, c, d) + x + k, s);
	}

	static void subround_128_256_H(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + H(b, c, d) + x + k, s);
	}

	static void subround_128_256_I(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + I(b, c, d) + x + k, s);
	}

	static void subround_128_256_J(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t k)
	{
		a = bit_rotate_left(a + J(b, c, d) + x + k, s);
	}
};


template <>
class ripemd<128> : public serial_hash<128, 128, 32, endian_t::little, 512>, public ripemd_base
{
private:
	typedef serial_hash<128, 128, 32, endian_t::little, 512> base_t;

	uint64_t m_bitCount;
	uint32_t m_state[16];

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += stride_bits;

		uint32_t a1 = m_result[0];
		uint32_t b1 = m_result[1];
		uint32_t c1 = m_result[2];
		uint32_t d1 = m_result[3];
		uint32_t a2 = a1;
		uint32_t b2 = b1;
		uint32_t c2 = c1;
		uint32_t d2 = d1;

		subround_128_256_F(a1, b1, c1, d1, m_state[0], 11, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[1], 14, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[2], 15, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[3], 12, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[4], 5, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[5], 8, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[6], 7, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[7], 9, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[8], 11, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[9], 13, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[10], 14, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[11], 15, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[12], 6, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[13], 7, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[14], 9, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[15], 8, 0);

		subround_128_256_G(a1, b1, c1, d1, m_state[7], 7, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[4], 6, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[13], 8, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[1], 13, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[10], 11, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[6], 9, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[15], 7, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[3], 15, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[12], 7, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[0], 12, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[9], 15, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[5], 9, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[2], 11, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[14], 7, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[11], 13, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[8], 12, 0x5a827999UL);

		subround_128_256_H(a1, b1, c1, d1, m_state[3], 11, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[10], 13, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[14], 6, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[4], 7, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[9], 14, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[15], 9, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[8], 13, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[1], 15, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[2], 14, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[7], 8, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[0], 13, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[6], 6, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[13], 5, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[11], 12, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[5], 7, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[12], 5, 0x6ed9eba1UL);

		subround_128_256_I(a1, b1, c1, d1, m_state[1], 11, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[9], 12, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[11], 14, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[10], 15, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[0], 14, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[8], 15, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[12], 9, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[4], 8, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[13], 9, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[3], 14, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[7], 5, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[15], 6, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[14], 8, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[5], 6, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[6], 5, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[2], 12, 0x8f1bbcdcUL);

		subround_128_256_I(a2, b2, c2, d2, m_state[5], 8, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[14], 9, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[7], 9, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[0], 11, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[9], 13, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[2], 15, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[11], 15, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[4], 5, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[13], 7, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[6], 7, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[15], 8, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[8], 11, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[1], 14, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[10], 14, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[3], 12, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[12], 6, 0x50a28be6UL);

		subround_128_256_H(a2, b2, c2, d2, m_state[6], 9, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[11], 13, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[3], 15, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[7], 7, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[0], 12, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[13], 8, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[5], 9, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[10], 11, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[14], 7, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[15], 7, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[8], 12, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[12], 7, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[4], 6, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[9], 15, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[1], 13, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[2], 11, 0x5c4dd124UL);

		subround_128_256_G(a2, b2, c2, d2, m_state[15], 9, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[5], 7, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[1], 15, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[3], 11, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[7], 8, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[14], 6, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[6], 6, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[9], 14, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[11], 12, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[8], 13, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[12], 5, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[2], 14, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[10], 13, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[0], 13, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[4], 7, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[13], 5, 0x6d703ef3UL);

		subround_128_256_F(a2, b2, c2, d2, m_state[8], 15, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[6], 5, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[4], 8, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[1], 11, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[3], 14, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[11], 14, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[15], 6, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[0], 14, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[5], 6, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[12], 9, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[2], 12, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[13], 9, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[9], 12, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[7], 5, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[10], 15, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[14], 8, 0);

		c1 = m_result[1] + c1 + d2;
		m_result[1] = m_result[2] + d1 + a2;
		m_result[2] = m_result[3] + a1 + b2;
		m_result[3] = m_result[0] + b1 + c2;
		m_result[0] = c1;
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
	ripemd()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
	}

	ripemd(const ripemd<128>& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	ripemd<128>& operator=(const ripemd<128>& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


template <>
class ripemd<256> : public serial_hash<256, 256, 32, endian_t::little, 512>, public ripemd_base
{
private:
	typedef serial_hash<256, 256, 32, endian_t::little, 512> base_t;

	uint64_t m_bitCount;
	uint32_t m_state[16];

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += stride_bits;

		uint32_t a1 = m_result[0];
		uint32_t b1 = m_result[1];
		uint32_t c1 = m_result[2];
		uint32_t d1 = m_result[3];
		uint32_t a2 = m_result[4];
		uint32_t b2 = m_result[5];
		uint32_t c2 = m_result[6];
		uint32_t d2 = m_result[7];

		subround_128_256_F(a1, b1, c1, d1, m_state[0], 11, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[1], 14, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[2], 15, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[3], 12, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[4], 5, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[5], 8, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[6], 7, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[7], 9, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[8], 11, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[9], 13, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[10], 14, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[11], 15, 0);
		subround_128_256_F(a1, b1, c1, d1, m_state[12], 6, 0);
		subround_128_256_F(d1, a1, b1, c1, m_state[13], 7, 0);
		subround_128_256_F(c1, d1, a1, b1, m_state[14], 9, 0);
		subround_128_256_F(b1, c1, d1, a1, m_state[15], 8, 0);

		subround_128_256_I(a2, b2, c2, d2, m_state[5], 8, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[14], 9, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[7], 9, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[0], 11, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[9], 13, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[2], 15, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[11], 15, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[4], 5, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[13], 7, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[6], 7, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[15], 8, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[8], 11, 0x50a28be6UL);
		subround_128_256_I(a2, b2, c2, d2, m_state[1], 14, 0x50a28be6UL);
		subround_128_256_I(d2, a2, b2, c2, m_state[10], 14, 0x50a28be6UL);
		subround_128_256_I(c2, d2, a2, b2, m_state[3], 12, 0x50a28be6UL);
		subround_128_256_I(b2, c2, d2, a2, m_state[12], 6, 0x50a28be6UL);

		uint32_t t = a1; a1 = a2; a2 = t;

		subround_128_256_G(a1, b1, c1, d1, m_state[7], 7, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[4], 6, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[13], 8, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[1], 13, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[10], 11, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[6], 9, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[15], 7, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[3], 15, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[12], 7, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[0], 12, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[9], 15, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[5], 9, 0x5a827999UL);
		subround_128_256_G(a1, b1, c1, d1, m_state[2], 11, 0x5a827999UL);
		subround_128_256_G(d1, a1, b1, c1, m_state[14], 7, 0x5a827999UL);
		subround_128_256_G(c1, d1, a1, b1, m_state[11], 13, 0x5a827999UL);
		subround_128_256_G(b1, c1, d1, a1, m_state[8], 12, 0x5a827999UL);

		subround_128_256_H(a2, b2, c2, d2, m_state[6], 9, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[11], 13, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[3], 15, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[7], 7, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[0], 12, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[13], 8, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[5], 9, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[10], 11, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[14], 7, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[15], 7, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[8], 12, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[12], 7, 0x5c4dd124UL);
		subround_128_256_H(a2, b2, c2, d2, m_state[4], 6, 0x5c4dd124UL);
		subround_128_256_H(d2, a2, b2, c2, m_state[9], 15, 0x5c4dd124UL);
		subround_128_256_H(c2, d2, a2, b2, m_state[1], 13, 0x5c4dd124UL);
		subround_128_256_H(b2, c2, d2, a2, m_state[2], 11, 0x5c4dd124UL);

		t = b1; b1 = b2; b2 = t;

		subround_128_256_H(a1, b1, c1, d1, m_state[3], 11, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[10], 13, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[14], 6, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[4], 7, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[9], 14, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[15], 9, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[8], 13, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[1], 15, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[2], 14, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[7], 8, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[0], 13, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[6], 6, 0x6ed9eba1UL);
		subround_128_256_H(a1, b1, c1, d1, m_state[13], 5, 0x6ed9eba1UL);
		subround_128_256_H(d1, a1, b1, c1, m_state[11], 12, 0x6ed9eba1UL);
		subround_128_256_H(c1, d1, a1, b1, m_state[5], 7, 0x6ed9eba1UL);
		subround_128_256_H(b1, c1, d1, a1, m_state[12], 5, 0x6ed9eba1UL);

		subround_128_256_G(a2, b2, c2, d2, m_state[15], 9, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[5], 7, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[1], 15, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[3], 11, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[7], 8, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[14], 6, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[6], 6, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[9], 14, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[11], 12, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[8], 13, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[12], 5, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[2], 14, 0x6d703ef3UL);
		subround_128_256_G(a2, b2, c2, d2, m_state[10], 13, 0x6d703ef3UL);
		subround_128_256_G(d2, a2, b2, c2, m_state[0], 13, 0x6d703ef3UL);
		subround_128_256_G(c2, d2, a2, b2, m_state[4], 7, 0x6d703ef3UL);
		subround_128_256_G(b2, c2, d2, a2, m_state[13], 5, 0x6d703ef3UL);

		t = c1; c1 = c2; c2 = t;

		subround_128_256_I(a1, b1, c1, d1, m_state[1], 11, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[9], 12, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[11], 14, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[10], 15, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[0], 14, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[8], 15, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[12], 9, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[4], 8, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[13], 9, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[3], 14, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[7], 5, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[15], 6, 0x8f1bbcdcUL);
		subround_128_256_I(a1, b1, c1, d1, m_state[14], 8, 0x8f1bbcdcUL);
		subround_128_256_I(d1, a1, b1, c1, m_state[5], 6, 0x8f1bbcdcUL);
		subround_128_256_I(c1, d1, a1, b1, m_state[6], 5, 0x8f1bbcdcUL);
		subround_128_256_I(b1, c1, d1, a1, m_state[2], 12, 0x8f1bbcdcUL);

		subround_128_256_F(a2, b2, c2, d2, m_state[8], 15, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[6], 5, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[4], 8, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[1], 11, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[3], 14, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[11], 14, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[15], 6, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[0], 14, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[5], 6, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[12], 9, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[2], 12, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[13], 9, 0);
		subround_128_256_F(a2, b2, c2, d2, m_state[9], 12, 0);
		subround_128_256_F(d2, a2, b2, c2, m_state[7], 5, 0);
		subround_128_256_F(c2, d2, a2, b2, m_state[10], 15, 0);
		subround_128_256_F(b2, c2, d2, a2, m_state[14], 8, 0);

		t = d1; d1 = d2; d2 = t;

		m_result[0] += a1;
		m_result[1] += b1;
		m_result[2] += c1;
		m_result[3] += d1;
		m_result[4] += a2;
		m_result[5] += b2;
		m_result[6] += c2;
		m_result[7] += d2;
	}

	void terminate()
	{
		uint64_t bitCount = m_bitCount + (base_t::m_blockProgress * digit_bits) + base_t::m_digitProgress;

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
	ripemd()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
		m_result[4] = 0x76543210L;
		m_result[5] = 0xfedcba98L;
		m_result[6] = 0x89abcdefL;
		m_result[7] = 0x01234567L;
	}

	ripemd(const ripemd<256>& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	ripemd<256>& operator=(const ripemd<256>& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


template <>
class ripemd<160> : public serial_hash<160, 160, 32, endian_t::little, 512>, public ripemd_base
{
private:
	typedef serial_hash<160, 160, 32, endian_t::little, 512> base_t;

	uint64_t m_bitCount;
	uint32_t m_state[16];

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += stride_bits;

		uint32_t a1 = m_result[0];
		uint32_t b1 = m_result[1];
		uint32_t c1 = m_result[2];
		uint32_t d1 = m_result[3];
		uint32_t e1 = m_result[4];
		uint32_t a2 = a1;
		uint32_t b2 = b1;
		uint32_t c2 = c1;
		uint32_t d2 = d1;
		uint32_t e2 = e1;

		subround_160_320_F(a1, b1, c1, d1, e1, m_state[0], 11, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[1], 14, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[2], 15, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[3], 12, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[4], 5, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[5], 8, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[6], 7, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[7], 9, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[8], 11, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[9], 13, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[10], 14, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[11], 15, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[12], 6, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[13], 7, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[14], 9, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[15], 8, 0);

		subround_160_320_G(e1, a1, b1, c1, d1, m_state[7], 7, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[4], 6, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[13], 8, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[1], 13, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[10], 11, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[6], 9, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[15], 7, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[3], 15, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[12], 7, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[0], 12, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[9], 15, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[5], 9, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[2], 11, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[14], 7, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[11], 13, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[8], 12, 0x5a827999UL);

		subround_160_320_H(d1, e1, a1, b1, c1, m_state[3], 11, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[10], 13, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[14], 6, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[4], 7, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[9], 14, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[15], 9, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[8], 13, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[1], 15, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[2], 14, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[7], 8, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[0], 13, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[6], 6, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[13], 5, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[11], 12, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[5], 7, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[12], 5, 0x6ed9eba1UL);

		subround_160_320_I(c1, d1, e1, a1, b1, m_state[1], 11, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[9], 12, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[11], 14, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[10], 15, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[0], 14, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[8], 15, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[12], 9, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[4], 8, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[13], 9, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[3], 14, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[7], 5, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[15], 6, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[14], 8, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[5], 6, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[6], 5, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[2], 12, 0x8f1bbcdcUL);

		subround_160_320_J(b1, c1, d1, e1, a1, m_state[4], 9, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[0], 15, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[5], 5, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[9], 11, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[7], 6, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[12], 8, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[2], 13, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[10], 12, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[14], 5, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[1], 12, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[3], 13, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[8], 14, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[11], 11, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[6], 8, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[15], 5, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[13], 6, 0xa953fd4eUL);

		subround_160_320_J(a2, b2, c2, d2, e2, m_state[5], 8, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[14], 9, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[7], 9, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[0], 11, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[9], 13, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[2], 15, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[11], 15, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[4], 5, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[13], 7, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[6], 7, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[15], 8, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[8], 11, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[1], 14, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[10], 14, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[3], 12, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[12], 6, 0x50a28be6UL);

		subround_160_320_I(e2, a2, b2, c2, d2, m_state[6], 9, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[11], 13, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[3], 15, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[7], 7, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[0], 12, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[13], 8, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[5], 9, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[10], 11, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[14], 7, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[15], 7, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[8], 12, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[12], 7, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[4], 6, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[9], 15, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[1], 13, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[2], 11, 0x5c4dd124UL);

		subround_160_320_H(d2, e2, a2, b2, c2, m_state[15], 9, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[5], 7, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[1], 15, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[3], 11, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[7], 8, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[14], 6, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[6], 6, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[9], 14, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[11], 12, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[8], 13, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[12], 5, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[2], 14, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[10], 13, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[0], 13, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[4], 7, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[13], 5, 0x6d703ef3UL);

		subround_160_320_G(c2, d2, e2, a2, b2, m_state[8], 15, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[6], 5, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[4], 8, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[1], 11, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[3], 14, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[11], 14, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[15], 6, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[0], 14, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[5], 6, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[12], 9, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[2], 12, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[13], 9, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[9], 12, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[7], 5, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[10], 15, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[14], 8, 0x7a6d76e9UL);

		subround_160_320_F(b2, c2, d2, e2, a2, m_state[12], 8, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[15], 5, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[10], 12, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[4], 9, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[1], 12, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[5], 5, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[8], 14, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[7], 6, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[6], 8, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[2], 13, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[13], 6, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[14], 5, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[0], 15, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[3], 13, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[9], 11, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[11], 11, 0);

		c1 = m_result[1] + c1 + d2;
		m_result[1] = m_result[2] + d1 + e2;
		m_result[2] = m_result[3] + e1 + a2;
		m_result[3] = m_result[4] + a1 + b2;
		m_result[4] = m_result[0] + b1 + c2;
		m_result[0] = c1;
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
	ripemd()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
		m_result[4] = 0xc3d2e1f0L;
	}

	ripemd(const ripemd<160>& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	ripemd<160>& operator=(const ripemd<160>& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};

template <>
class ripemd<320> : public serial_hash<320, 320, 32, endian_t::little, 512>, public ripemd_base
{
private:
	typedef serial_hash<320, 320, 32, endian_t::little, 512> base_t;

	uint64_t m_bitCount;
	uint32_t m_state[16];

	void process_digit()
	{
		m_state[m_blockProgress] = m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += stride_bits;

		uint32_t a1 = m_result[0];
		uint32_t b1 = m_result[1];
		uint32_t c1 = m_result[2];
		uint32_t d1 = m_result[3];
		uint32_t e1 = m_result[4];
		uint32_t a2 = m_result[5];
		uint32_t b2 = m_result[6];
		uint32_t c2 = m_result[7];
		uint32_t d2 = m_result[8];
		uint32_t e2 = m_result[9];

		subround_160_320_F(a1, b1, c1, d1, e1, m_state[0], 11, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[1], 14, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[2], 15, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[3], 12, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[4], 5, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[5], 8, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[6], 7, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[7], 9, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[8], 11, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[9], 13, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[10], 14, 0);
		subround_160_320_F(e1, a1, b1, c1, d1, m_state[11], 15, 0);
		subround_160_320_F(d1, e1, a1, b1, c1, m_state[12], 6, 0);
		subround_160_320_F(c1, d1, e1, a1, b1, m_state[13], 7, 0);
		subround_160_320_F(b1, c1, d1, e1, a1, m_state[14], 9, 0);
		subround_160_320_F(a1, b1, c1, d1, e1, m_state[15], 8, 0);

		subround_160_320_J(a2, b2, c2, d2, e2, m_state[5], 8, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[14], 9, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[7], 9, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[0], 11, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[9], 13, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[2], 15, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[11], 15, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[4], 5, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[13], 7, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[6], 7, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[15], 8, 0x50a28be6UL);
		subround_160_320_J(e2, a2, b2, c2, d2, m_state[8], 11, 0x50a28be6UL);
		subround_160_320_J(d2, e2, a2, b2, c2, m_state[1], 14, 0x50a28be6UL);
		subround_160_320_J(c2, d2, e2, a2, b2, m_state[10], 14, 0x50a28be6UL);
		subround_160_320_J(b2, c2, d2, e2, a2, m_state[3], 12, 0x50a28be6UL);
		subround_160_320_J(a2, b2, c2, d2, e2, m_state[12], 6, 0x50a28be6UL);

		uint32_t t = a1; a1 = a2; a2 = t;

		subround_160_320_G(e1, a1, b1, c1, d1, m_state[7], 7, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[4], 6, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[13], 8, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[1], 13, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[10], 11, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[6], 9, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[15], 7, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[3], 15, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[12], 7, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[0], 12, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[9], 15, 0x5a827999UL);
		subround_160_320_G(d1, e1, a1, b1, c1, m_state[5], 9, 0x5a827999UL);
		subround_160_320_G(c1, d1, e1, a1, b1, m_state[2], 11, 0x5a827999UL);
		subround_160_320_G(b1, c1, d1, e1, a1, m_state[14], 7, 0x5a827999UL);
		subround_160_320_G(a1, b1, c1, d1, e1, m_state[11], 13, 0x5a827999UL);
		subround_160_320_G(e1, a1, b1, c1, d1, m_state[8], 12, 0x5a827999UL);

		subround_160_320_I(e2, a2, b2, c2, d2, m_state[6], 9, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[11], 13, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[3], 15, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[7], 7, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[0], 12, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[13], 8, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[5], 9, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[10], 11, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[14], 7, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[15], 7, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[8], 12, 0x5c4dd124UL);
		subround_160_320_I(d2, e2, a2, b2, c2, m_state[12], 7, 0x5c4dd124UL);
		subround_160_320_I(c2, d2, e2, a2, b2, m_state[4], 6, 0x5c4dd124UL);
		subround_160_320_I(b2, c2, d2, e2, a2, m_state[9], 15, 0x5c4dd124UL);
		subround_160_320_I(a2, b2, c2, d2, e2, m_state[1], 13, 0x5c4dd124UL);
		subround_160_320_I(e2, a2, b2, c2, d2, m_state[2], 11, 0x5c4dd124UL);

		t = b1; b1 = b2; b2 = t;

		subround_160_320_H(d1, e1, a1, b1, c1, m_state[3], 11, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[10], 13, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[14], 6, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[4], 7, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[9], 14, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[15], 9, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[8], 13, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[1], 15, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[2], 14, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[7], 8, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[0], 13, 0x6ed9eba1UL);
		subround_160_320_H(c1, d1, e1, a1, b1, m_state[6], 6, 0x6ed9eba1UL);
		subround_160_320_H(b1, c1, d1, e1, a1, m_state[13], 5, 0x6ed9eba1UL);
		subround_160_320_H(a1, b1, c1, d1, e1, m_state[11], 12, 0x6ed9eba1UL);
		subround_160_320_H(e1, a1, b1, c1, d1, m_state[5], 7, 0x6ed9eba1UL);
		subround_160_320_H(d1, e1, a1, b1, c1, m_state[12], 5, 0x6ed9eba1UL);

		subround_160_320_H(d2, e2, a2, b2, c2, m_state[15], 9, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[5], 7, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[1], 15, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[3], 11, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[7], 8, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[14], 6, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[6], 6, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[9], 14, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[11], 12, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[8], 13, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[12], 5, 0x6d703ef3UL);
		subround_160_320_H(c2, d2, e2, a2, b2, m_state[2], 14, 0x6d703ef3UL);
		subround_160_320_H(b2, c2, d2, e2, a2, m_state[10], 13, 0x6d703ef3UL);
		subround_160_320_H(a2, b2, c2, d2, e2, m_state[0], 13, 0x6d703ef3UL);
		subround_160_320_H(e2, a2, b2, c2, d2, m_state[4], 7, 0x6d703ef3UL);
		subround_160_320_H(d2, e2, a2, b2, c2, m_state[13], 5, 0x6d703ef3UL);

		t = c1; c1 = c2; c2 = t;

		subround_160_320_I(c1, d1, e1, a1, b1, m_state[1], 11, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[9], 12, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[11], 14, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[10], 15, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[0], 14, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[8], 15, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[12], 9, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[4], 8, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[13], 9, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[3], 14, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[7], 5, 0x8f1bbcdcUL);
		subround_160_320_I(b1, c1, d1, e1, a1, m_state[15], 6, 0x8f1bbcdcUL);
		subround_160_320_I(a1, b1, c1, d1, e1, m_state[14], 8, 0x8f1bbcdcUL);
		subround_160_320_I(e1, a1, b1, c1, d1, m_state[5], 6, 0x8f1bbcdcUL);
		subround_160_320_I(d1, e1, a1, b1, c1, m_state[6], 5, 0x8f1bbcdcUL);
		subround_160_320_I(c1, d1, e1, a1, b1, m_state[2], 12, 0x8f1bbcdcUL);

		subround_160_320_G(c2, d2, e2, a2, b2, m_state[8], 15, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[6], 5, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[4], 8, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[1], 11, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[3], 14, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[11], 14, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[15], 6, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[0], 14, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[5], 6, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[12], 9, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[2], 12, 0x7a6d76e9UL);
		subround_160_320_G(b2, c2, d2, e2, a2, m_state[13], 9, 0x7a6d76e9UL);
		subround_160_320_G(a2, b2, c2, d2, e2, m_state[9], 12, 0x7a6d76e9UL);
		subround_160_320_G(e2, a2, b2, c2, d2, m_state[7], 5, 0x7a6d76e9UL);
		subround_160_320_G(d2, e2, a2, b2, c2, m_state[10], 15, 0x7a6d76e9UL);
		subround_160_320_G(c2, d2, e2, a2, b2, m_state[14], 8, 0x7a6d76e9UL);

		t = d1; d1 = d2; d2 = t;

		subround_160_320_J(b1, c1, d1, e1, a1, m_state[4], 9, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[0], 15, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[5], 5, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[9], 11, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[7], 6, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[12], 8, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[2], 13, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[10], 12, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[14], 5, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[1], 12, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[3], 13, 0xa953fd4eUL);
		subround_160_320_J(a1, b1, c1, d1, e1, m_state[8], 14, 0xa953fd4eUL);
		subround_160_320_J(e1, a1, b1, c1, d1, m_state[11], 11, 0xa953fd4eUL);
		subround_160_320_J(d1, e1, a1, b1, c1, m_state[6], 8, 0xa953fd4eUL);
		subround_160_320_J(c1, d1, e1, a1, b1, m_state[15], 5, 0xa953fd4eUL);
		subround_160_320_J(b1, c1, d1, e1, a1, m_state[13], 6, 0xa953fd4eUL);

		subround_160_320_F(b2, c2, d2, e2, a2, m_state[12], 8, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[15], 5, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[10], 12, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[4], 9, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[1], 12, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[5], 5, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[8], 14, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[7], 6, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[6], 8, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[2], 13, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[13], 6, 0);
		subround_160_320_F(a2, b2, c2, d2, e2, m_state[14], 5, 0);
		subround_160_320_F(e2, a2, b2, c2, d2, m_state[0], 15, 0);
		subround_160_320_F(d2, e2, a2, b2, c2, m_state[3], 13, 0);
		subround_160_320_F(c2, d2, e2, a2, b2, m_state[9], 11, 0);
		subround_160_320_F(b2, c2, d2, e2, a2, m_state[11], 11, 0);

		t = e1; e1 = e2; e2 = t;

		m_result[0] += a1;
		m_result[1] += b1;
		m_result[2] += c1;
		m_result[3] += d1;
		m_result[4] += e1;
		m_result[5] += a2;
		m_result[6] += b2;
		m_result[7] += c2;
		m_result[8] += d2;
		m_result[9] += e2;
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
	ripemd()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		m_result[0] = 0x67452301L;
		m_result[1] = 0xefcdab89L;
		m_result[2] = 0x98badcfeL;
		m_result[3] = 0x10325476L;
		m_result[4] = 0xc3d2e1f0L;
		m_result[5] = 0x76543210L;
		m_result[6] = 0xfedcba98L;
		m_result[7] = 0x89abcdefL;
		m_result[8] = 0x01234567L;
		m_result[9] = 0x3c2d1e0fL;
	}

	ripemd(const ripemd<320>& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	ripemd<320>& operator=(const ripemd<320>& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


}
}


#endif
