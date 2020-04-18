//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_SHA2
#define COGS_HEADER_CRYPTO_SHA2


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief SHA-2 (Secure Hash Algorithm 2)
/// @tparam result_bits Bit width of the result
/// @tparam digest_bits Bit width of the digest.  Default: Same as bit width of the result
template <size_t result_bits, size_t digest_bits = result_bits>
class sha2 : public hash
{
private:
	sha2() = delete; // Not allowed.  This template is specialized for supported bit sizes
};


template <size_t digest_bits>
class sha2_256_base : public serial_hash<256, digest_bits, 32, endian_t::big, 512>
{
private:
	typedef sha2_256_base<digest_bits> this_t;
	typedef serial_hash<256, digest_bits, 32, endian_t::big, 512> base_t;

	uint32_t m_state[64];
	uint64_t m_bitCount;

	static uint32_t S0(uint32_t x) { return bit_rotate_right(x, 7) ^ bit_rotate_right(x, 18) ^ (x >> 3); }
	static uint32_t S1(uint32_t x) { return bit_rotate_right(x, 17) ^ bit_rotate_right(x, 19) ^ (x >> 10); }
	static uint32_t S2(uint32_t x) { return bit_rotate_right(x, 2) ^ bit_rotate_right(x, 13) ^ bit_rotate_right(x, 22); }
	static uint32_t S3(uint32_t x) { return bit_rotate_right(x, 6) ^ bit_rotate_right(x, 11) ^ bit_rotate_right(x, 25); }

	static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return ((x & y) | (z & (x | y))); }
	static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (z ^ (x & (y ^ z))); }

	uint32_t R(uint8_t t) { return (m_state[t] = S1(m_state[t - 2]) + m_state[t - 7] + S0(m_state[t - 15]) + m_state[t - 16]); }

	static void r(uint32_t a, uint32_t b, uint32_t c, uint32_t& d, uint32_t e, uint32_t f, uint32_t g, uint32_t& h, uint32_t x, uint32_t K)
	{
		uint32_t tmp = h + S3(e) + ch(e, f, g) + K + x;
		d += tmp;
		h = tmp + S2(a) + maj(a, b, c);
	}

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += 512;

		uint32_t a = base_t::m_result[0];
		uint32_t b = base_t::m_result[1];
		uint32_t c = base_t::m_result[2];
		uint32_t d = base_t::m_result[3];
		uint32_t e = base_t::m_result[4];
		uint32_t f = base_t::m_result[5];
		uint32_t g = base_t::m_result[6];
		uint32_t h = base_t::m_result[7];

		r(a, b, c, d, e, f, g, h, m_state[0], 0x428a2f98);
		r(h, a, b, c, d, e, f, g, m_state[1], 0x71374491);
		r(g, h, a, b, c, d, e, f, m_state[2], 0xb5c0fbcf);
		r(f, g, h, a, b, c, d, e, m_state[3], 0xe9b5dba5);
		r(e, f, g, h, a, b, c, d, m_state[4], 0x3956c25b);
		r(d, e, f, g, h, a, b, c, m_state[5], 0x59f111f1);
		r(c, d, e, f, g, h, a, b, m_state[6], 0x923f82a4);
		r(b, c, d, e, f, g, h, a, m_state[7], 0xab1c5ed5);
		r(a, b, c, d, e, f, g, h, m_state[8], 0xd807aa98);
		r(h, a, b, c, d, e, f, g, m_state[9], 0x12835b01);
		r(g, h, a, b, c, d, e, f, m_state[10], 0x243185be);
		r(f, g, h, a, b, c, d, e, m_state[11], 0x550c7dc3);
		r(e, f, g, h, a, b, c, d, m_state[12], 0x72be5d74);
		r(d, e, f, g, h, a, b, c, m_state[13], 0x80deb1fe);
		r(c, d, e, f, g, h, a, b, m_state[14], 0x9bdc06a7);
		r(b, c, d, e, f, g, h, a, m_state[15], 0xc19bf174);
		r(a, b, c, d, e, f, g, h, R(16), 0xe49b69c1);
		r(h, a, b, c, d, e, f, g, R(17), 0xefbe4786);
		r(g, h, a, b, c, d, e, f, R(18), 0x0fc19dc6);
		r(f, g, h, a, b, c, d, e, R(19), 0x240ca1cc);
		r(e, f, g, h, a, b, c, d, R(20), 0x2de92c6f);
		r(d, e, f, g, h, a, b, c, R(21), 0x4a7484aa);
		r(c, d, e, f, g, h, a, b, R(22), 0x5cb0a9dc);
		r(b, c, d, e, f, g, h, a, R(23), 0x76f988da);
		r(a, b, c, d, e, f, g, h, R(24), 0x983e5152);
		r(h, a, b, c, d, e, f, g, R(25), 0xa831c66d);
		r(g, h, a, b, c, d, e, f, R(26), 0xb00327c8);
		r(f, g, h, a, b, c, d, e, R(27), 0xbf597fc7);
		r(e, f, g, h, a, b, c, d, R(28), 0xc6e00bf3);
		r(d, e, f, g, h, a, b, c, R(29), 0xd5a79147);
		r(c, d, e, f, g, h, a, b, R(30), 0x06ca6351);
		r(b, c, d, e, f, g, h, a, R(31), 0x14292967);
		r(a, b, c, d, e, f, g, h, R(32), 0x27b70a85);
		r(h, a, b, c, d, e, f, g, R(33), 0x2e1b2138);
		r(g, h, a, b, c, d, e, f, R(34), 0x4d2c6dfc);
		r(f, g, h, a, b, c, d, e, R(35), 0x53380d13);
		r(e, f, g, h, a, b, c, d, R(36), 0x650a7354);
		r(d, e, f, g, h, a, b, c, R(37), 0x766a0abb);
		r(c, d, e, f, g, h, a, b, R(38), 0x81c2c92e);
		r(b, c, d, e, f, g, h, a, R(39), 0x92722c85);
		r(a, b, c, d, e, f, g, h, R(40), 0xa2bfe8a1);
		r(h, a, b, c, d, e, f, g, R(41), 0xa81a664b);
		r(g, h, a, b, c, d, e, f, R(42), 0xc24b8b70);
		r(f, g, h, a, b, c, d, e, R(43), 0xc76c51a3);
		r(e, f, g, h, a, b, c, d, R(44), 0xd192e819);
		r(d, e, f, g, h, a, b, c, R(45), 0xd6990624);
		r(c, d, e, f, g, h, a, b, R(46), 0xf40e3585);
		r(b, c, d, e, f, g, h, a, R(47), 0x106aa070);
		r(a, b, c, d, e, f, g, h, R(48), 0x19a4c116);
		r(h, a, b, c, d, e, f, g, R(49), 0x1e376c08);
		r(g, h, a, b, c, d, e, f, R(50), 0x2748774c);
		r(f, g, h, a, b, c, d, e, R(51), 0x34b0bcb5);
		r(e, f, g, h, a, b, c, d, R(52), 0x391c0cb3);
		r(d, e, f, g, h, a, b, c, R(53), 0x4ed8aa4a);
		r(c, d, e, f, g, h, a, b, R(54), 0x5b9cca4f);
		r(b, c, d, e, f, g, h, a, R(55), 0x682e6ff3);
		r(a, b, c, d, e, f, g, h, R(56), 0x748f82ee);
		r(h, a, b, c, d, e, f, g, R(57), 0x78a5636f);
		r(g, h, a, b, c, d, e, f, R(58), 0x84c87814);
		r(f, g, h, a, b, c, d, e, R(59), 0x8cc70208);
		r(e, f, g, h, a, b, c, d, R(60), 0x90befffa);
		r(d, e, f, g, h, a, b, c, R(61), 0xa4506ceb);
		r(c, d, e, f, g, h, a, b, R(62), 0xbef9a3f7);
		r(b, c, d, e, f, g, h, a, R(63), 0xc67178f2);

		base_t::m_result[0] += a;
		base_t::m_result[1] += b;
		base_t::m_result[2] += c;
		base_t::m_result[3] += d;
		base_t::m_result[4] += e;
		base_t::m_result[5] += f;
		base_t::m_result[6] += g;
		base_t::m_result[7] += h;
	}

	void terminate()
	{
		uint64_t bitCount = m_bitCount + (base_t::m_blockProgress * 32) + base_t::m_digitProgress;

		// Add terminating 0x80 byte.  Might cause this block to process, if last byte.
		base_t::add_byte(0x80);

		// Finish a current digit in progress, if any
		if (base_t::m_digitProgress != 0)
			process_digit();

		// We need 2 digits for the length.  If not enough space, add padding to this block, start a new one.
		if (base_t::m_blockProgress == 15)
			process_digit();

		// Pad block, leaving space at the end for 2 digits
		for (size_t i = base_t::m_blockProgress; i < 14; i++)
			process_digit();

		m_state[14] = (uint32_t)(bitCount >> 32);
		m_state[15] = (uint32_t)bitCount;
		process_block();
	}

public:
	sha2_256_base()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
	}

	sha2_256_base(const this_t& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


template <size_t digest_bits>
class sha2_512_base : public serial_hash<512, digest_bits, 64, endian_t::big, 1024>
{
private:
	typedef sha2_512_base<digest_bits> this_t;
	typedef serial_hash<512, digest_bits, 64, endian_t::big, 1024> base_t;

	uint64_t m_state[80];
	fixed_integer<false, 128> m_bitCount;

	static uint64_t S0(uint64_t x) { return bit_rotate_right(x, 1) ^ bit_rotate_right(x, 8) ^ (x >> 7); }
	static uint64_t S1(uint64_t x) { return bit_rotate_right(x, 19) ^ bit_rotate_right(x, 61) ^ (x >> 6); }

	static uint64_t S2(uint64_t x) { return bit_rotate_right(x, 28) ^ bit_rotate_right(x, 34) ^ bit_rotate_right(x, 39); }
	static uint64_t S3(uint64_t x) { return bit_rotate_right(x, 14) ^ bit_rotate_right(x, 18) ^ bit_rotate_right(x, 41); }

	static uint64_t maj(uint64_t x, uint64_t y, uint64_t z) { return ((x & y) | (z & (x | y))); }
	static uint64_t ch(uint64_t x, uint64_t y, uint64_t z) { return (z ^ (x & (y ^ z))); }

	uint64_t R(uint8_t t) { return (m_state[t] = S1(m_state[t - 2]) + m_state[t - 7] + S0(m_state[t - 15]) + m_state[t - 16]); }

	static void r(uint64_t a, uint64_t b, uint64_t c, uint64_t& d, uint64_t e, uint64_t f, uint64_t g, uint64_t& h, uint64_t x, uint64_t K)
	{
		uint64_t tmp = h + S3(e) + ch(e, f, g) + K + x;
		d += tmp;
		h = tmp + S2(a) + maj(a, b, c);
	}

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += 1024;

		uint64_t a = base_t::m_result[0];
		uint64_t b = base_t::m_result[1];
		uint64_t c = base_t::m_result[2];
		uint64_t d = base_t::m_result[3];
		uint64_t e = base_t::m_result[4];
		uint64_t f = base_t::m_result[5];
		uint64_t g = base_t::m_result[6];
		uint64_t h = base_t::m_result[7];

		r(a, b, c, d, e, f, g, h, m_state[0], 0x428a2f98d728ae22ULL);
		r(h, a, b, c, d, e, f, g, m_state[1], 0x7137449123ef65cdULL);
		r(g, h, a, b, c, d, e, f, m_state[2], 0xb5c0fbcfec4d3b2fULL);
		r(f, g, h, a, b, c, d, e, m_state[3], 0xe9b5dba58189dbbcULL);
		r(e, f, g, h, a, b, c, d, m_state[4], 0x3956c25bf348b538ULL);
		r(d, e, f, g, h, a, b, c, m_state[5], 0x59f111f1b605d019ULL);
		r(c, d, e, f, g, h, a, b, m_state[6], 0x923f82a4af194f9bULL);
		r(b, c, d, e, f, g, h, a, m_state[7], 0xab1c5ed5da6d8118ULL);
		r(a, b, c, d, e, f, g, h, m_state[8], 0xd807aa98a3030242ULL);
		r(h, a, b, c, d, e, f, g, m_state[9], 0x12835b0145706fbeULL);
		r(g, h, a, b, c, d, e, f, m_state[10], 0x243185be4ee4b28cULL);
		r(f, g, h, a, b, c, d, e, m_state[11], 0x550c7dc3d5ffb4e2ULL);
		r(e, f, g, h, a, b, c, d, m_state[12], 0x72be5d74f27b896fULL);
		r(d, e, f, g, h, a, b, c, m_state[13], 0x80deb1fe3b1696b1ULL);
		r(c, d, e, f, g, h, a, b, m_state[14], 0x9bdc06a725c71235ULL);
		r(b, c, d, e, f, g, h, a, m_state[15], 0xc19bf174cf692694ULL);
		r(a, b, c, d, e, f, g, h, R(16), 0xe49b69c19ef14ad2ULL);
		r(h, a, b, c, d, e, f, g, R(17), 0xefbe4786384f25e3ULL);
		r(g, h, a, b, c, d, e, f, R(18), 0x0fc19dc68b8cd5b5ULL);
		r(f, g, h, a, b, c, d, e, R(19), 0x240ca1cc77ac9c65ULL);
		r(e, f, g, h, a, b, c, d, R(20), 0x2de92c6f592b0275ULL);
		r(d, e, f, g, h, a, b, c, R(21), 0x4a7484aa6ea6e483ULL);
		r(c, d, e, f, g, h, a, b, R(22), 0x5cb0a9dcbd41fbd4ULL);
		r(b, c, d, e, f, g, h, a, R(23), 0x76f988da831153b5ULL);
		r(a, b, c, d, e, f, g, h, R(24), 0x983e5152ee66dfabULL);
		r(h, a, b, c, d, e, f, g, R(25), 0xa831c66d2db43210ULL);
		r(g, h, a, b, c, d, e, f, R(26), 0xb00327c898fb213fULL);
		r(f, g, h, a, b, c, d, e, R(27), 0xbf597fc7beef0ee4ULL);
		r(e, f, g, h, a, b, c, d, R(28), 0xc6e00bf33da88fc2ULL);
		r(d, e, f, g, h, a, b, c, R(29), 0xd5a79147930aa725ULL);
		r(c, d, e, f, g, h, a, b, R(30), 0x06ca6351e003826fULL);
		r(b, c, d, e, f, g, h, a, R(31), 0x142929670a0e6e70ULL);
		r(a, b, c, d, e, f, g, h, R(32), 0x27b70a8546d22ffcULL);
		r(h, a, b, c, d, e, f, g, R(33), 0x2e1b21385c26c926ULL);
		r(g, h, a, b, c, d, e, f, R(34), 0x4d2c6dfc5ac42aedULL);
		r(f, g, h, a, b, c, d, e, R(35), 0x53380d139d95b3dfULL);
		r(e, f, g, h, a, b, c, d, R(36), 0x650a73548baf63deULL);
		r(d, e, f, g, h, a, b, c, R(37), 0x766a0abb3c77b2a8ULL);
		r(c, d, e, f, g, h, a, b, R(38), 0x81c2c92e47edaee6ULL);
		r(b, c, d, e, f, g, h, a, R(39), 0x92722c851482353bULL);
		r(a, b, c, d, e, f, g, h, R(40), 0xa2bfe8a14cf10364ULL);
		r(h, a, b, c, d, e, f, g, R(41), 0xa81a664bbc423001ULL);
		r(g, h, a, b, c, d, e, f, R(42), 0xc24b8b70d0f89791ULL);
		r(f, g, h, a, b, c, d, e, R(43), 0xc76c51a30654be30ULL);
		r(e, f, g, h, a, b, c, d, R(44), 0xd192e819d6ef5218ULL);
		r(d, e, f, g, h, a, b, c, R(45), 0xd69906245565a910ULL);
		r(c, d, e, f, g, h, a, b, R(46), 0xf40e35855771202aULL);
		r(b, c, d, e, f, g, h, a, R(47), 0x106aa07032bbd1b8ULL);
		r(a, b, c, d, e, f, g, h, R(48), 0x19a4c116b8d2d0c8ULL);
		r(h, a, b, c, d, e, f, g, R(49), 0x1e376c085141ab53ULL);
		r(g, h, a, b, c, d, e, f, R(50), 0x2748774cdf8eeb99ULL);
		r(f, g, h, a, b, c, d, e, R(51), 0x34b0bcb5e19b48a8ULL);
		r(e, f, g, h, a, b, c, d, R(52), 0x391c0cb3c5c95a63ULL);
		r(d, e, f, g, h, a, b, c, R(53), 0x4ed8aa4ae3418acbULL);
		r(c, d, e, f, g, h, a, b, R(54), 0x5b9cca4f7763e373ULL);
		r(b, c, d, e, f, g, h, a, R(55), 0x682e6ff3d6b2b8a3ULL);
		r(a, b, c, d, e, f, g, h, R(56), 0x748f82ee5defb2fcULL);
		r(h, a, b, c, d, e, f, g, R(57), 0x78a5636f43172f60ULL);
		r(g, h, a, b, c, d, e, f, R(58), 0x84c87814a1f0ab72ULL);
		r(f, g, h, a, b, c, d, e, R(59), 0x8cc702081a6439ecULL);
		r(e, f, g, h, a, b, c, d, R(60), 0x90befffa23631e28ULL);
		r(d, e, f, g, h, a, b, c, R(61), 0xa4506cebde82bde9ULL);
		r(c, d, e, f, g, h, a, b, R(62), 0xbef9a3f7b2c67915ULL);
		r(b, c, d, e, f, g, h, a, R(63), 0xc67178f2e372532bULL);
		r(a, b, c, d, e, f, g, h, R(64), 0xca273eceea26619cULL);
		r(h, a, b, c, d, e, f, g, R(65), 0xd186b8c721c0c207ULL);
		r(g, h, a, b, c, d, e, f, R(66), 0xeada7dd6cde0eb1eULL);
		r(f, g, h, a, b, c, d, e, R(67), 0xf57d4f7fee6ed178ULL);
		r(e, f, g, h, a, b, c, d, R(68), 0x06f067aa72176fbaULL);
		r(d, e, f, g, h, a, b, c, R(69), 0x0a637dc5a2c898a6ULL);
		r(c, d, e, f, g, h, a, b, R(70), 0x113f9804bef90daeULL);
		r(b, c, d, e, f, g, h, a, R(71), 0x1b710b35131c471bULL);
		r(a, b, c, d, e, f, g, h, R(72), 0x28db77f523047d84ULL);
		r(h, a, b, c, d, e, f, g, R(73), 0x32caab7b40c72493ULL);
		r(g, h, a, b, c, d, e, f, R(74), 0x3c9ebe0a15c9bebcULL);
		r(f, g, h, a, b, c, d, e, R(75), 0x431d67c49c100d4cULL);
		r(e, f, g, h, a, b, c, d, R(76), 0x4cc5d4becb3e42b6ULL);
		r(d, e, f, g, h, a, b, c, R(77), 0x597f299cfc657e2aULL);
		r(c, d, e, f, g, h, a, b, R(78), 0x5fcb6fab3ad6faecULL);
		r(b, c, d, e, f, g, h, a, R(79), 0x6c44198c4a475817ULL);

		base_t::m_result[0] += a;
		base_t::m_result[1] += b;
		base_t::m_result[2] += c;
		base_t::m_result[3] += d;
		base_t::m_result[4] += e;
		base_t::m_result[5] += f;
		base_t::m_result[6] += g;
		base_t::m_result[7] += h;
	}

	void terminate()
	{
		fixed_integer<false, 128> bitCount = m_bitCount + (base_t::m_blockProgress * 64) + base_t::m_digitProgress;

		// Add terminating 0x80 byte.  Might cause this block to process, if last byte.
		base_t::add_byte(0x80);

		// Finish a current digit in progress, if any
		if (base_t::m_digitProgress != 0)
			process_digit();

		// We need 2 digits for the length.  If not enough space, add padding to this block, start a new one.
		while (base_t::m_blockProgress != base_t::stride_digits - 2)
			process_digit();

		m_state[14] = (uint64_t)(bitCount >> base_t::digit_bits).get_int();
		m_state[15] = (uint64_t)bitCount.get_int();
		process_block();
	}

public:
	sha2_512_base()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
	}

	sha2_512_base(const this_t& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


template <>
class sha2<256, 224> : public sha2_256_base<224>
{
public:
	sha2()
	{
		m_result[0] = 0xc1059ed8;
		m_result[1] = 0x367cd507;
		m_result[2] = 0x3070dd17;
		m_result[3] = 0xf70e5939;
		m_result[4] = 0xffc00b31;
		m_result[5] = 0x68581511;
		m_result[6] = 0x64f98fa7;
		m_result[7] = 0xbefa4fa4;
	}

	sha2(const sha2<256, 224>& src)
		: sha2_256_base<224>(src)
	{ }

	sha2<256, 224>& operator=(const sha2<256, 224>& src)
	{
		sha2_256_base<224>::operator=(src);
		return *this;
	}
};


template <>
class sha2<256, 256> : public sha2_256_base<256>
{
public:
	sha2()
	{
		m_result[0] = 0x6a09e667;
		m_result[1] = 0xbb67ae85;
		m_result[2] = 0x3c6ef372;
		m_result[3] = 0xa54ff53a;
		m_result[4] = 0x510e527f;
		m_result[5] = 0x9b05688c;
		m_result[6] = 0x1f83d9ab;
		m_result[7] = 0x5be0cd19;
	}

	sha2(const sha2<256, 256>& src)
		: sha2_256_base<256>(src)
	{ }

	sha2<256, 256>& operator=(const sha2<256, 256>& src)
	{
		sha2_256_base<256>::operator=(src);
		return *this;
	}
};


template <>
class sha2<512, 512> : public sha2_512_base<512>
{
public:
	sha2()
	{
		m_result[0] = 0x6a09e667f3bcc908ULL;
		m_result[1] = 0xbb67ae8584caa73bULL;
		m_result[2] = 0x3c6ef372fe94f82bULL;
		m_result[3] = 0xa54ff53a5f1d36f1ULL;
		m_result[4] = 0x510e527fade682d1ULL;
		m_result[5] = 0x9b05688c2b3e6c1fULL;
		m_result[6] = 0x1f83d9abfb41bd6bULL;
		m_result[7] = 0x5be0cd19137e2179ULL;
	}

	sha2(const sha2<512, 512>& src)
		: sha2_512_base<512>(src)
	{
	}

	sha2<512, 512>& operator=(const sha2<512, 512>& src)
	{
		sha2_512_base<512>::operator=(src);
		return *this;
	}
};


template <>
class sha2<512, 224> : public sha2_512_base<224>
{
public:
	sha2()
	{
		m_result[0] = 0x8C3D37C819544DA2ULL;
		m_result[1] = 0x73E1996689DCD4D6ULL;
		m_result[2] = 0x1DFAB7AE32FF9C82ULL;
		m_result[3] = 0x679DD514582F9FCFULL;
		m_result[4] = 0x0F6D2B697BD44DA8ULL;
		m_result[5] = 0x77E36F7304C48942ULL;
		m_result[6] = 0x3F9D85A86A1D36C8ULL;
		m_result[7] = 0x1112E6AD91D692A1ULL;
	}

	sha2(const sha2<512, 224>& src)
		: sha2_512_base<224>(src)
	{
	}

	sha2<512, 224>& operator=(const sha2<512, 224>& src)
	{
		sha2_512_base<224>::operator=(src);
		return *this;
	}
};


template <>
class sha2<512, 256> : public sha2_512_base<256>
{
public:
	sha2()
	{
		m_result[0] = 0x22312194FC2BF72CULL;
		m_result[1] = 0x9F555FA3C84C64C2ULL;
		m_result[2] = 0x2393B86B6F53B151ULL;
		m_result[3] = 0x963877195940EABDULL;
		m_result[4] = 0x96283EE2A88EFFE3ULL;
		m_result[5] = 0xBE5E1E2553863992ULL;
		m_result[6] = 0x2B0199FC2C85B8AAULL;
		m_result[7] = 0x0EB72DDC81C52CA2ULL;
	}

	sha2(const sha2<512, 256>& src)
		: sha2_512_base<256>(src)
	{
	}

	sha2<512, 256>& operator=(const sha2<512, 256>& src)
	{
		sha2_512_base<256>::operator=(src);
		return *this;
	}
};


template <>
class sha2<512, 384> : public sha2_512_base<384>
{
public:
	sha2()
	{
		m_result[0] = 0xcbbb9d5dc1059ed8ULL;
		m_result[1] = 0x629a292a367cd507ULL;
		m_result[2] = 0x9159015a3070dd17ULL;
		m_result[3] = 0x152fecd8f70e5939ULL;
		m_result[4] = 0x67332667ffc00b31ULL;
		m_result[5] = 0x8eb44a8768581511ULL;
		m_result[6] = 0xdb0c2e0d64f98fa7ULL;
		m_result[7] = 0x47b5481dbefa4fa4ULL;
	}

	sha2(const sha2<512, 384>& src)
		: sha2_512_base<384>(src)
	{
	}

	sha2<512, 384>& operator=(const sha2<512, 384>& src)
	{
		sha2_512_base<384>::operator=(src);
		return *this;
	}
};


typedef sha2<256, 224> sha_224;
typedef sha2<256> sha_256;
typedef sha2<512> sha_512;
typedef sha2<512, 224> sha_512_224;
typedef sha2<512, 256> sha_512_256;
typedef sha2<512, 384> sha_384;


}
}


#endif
