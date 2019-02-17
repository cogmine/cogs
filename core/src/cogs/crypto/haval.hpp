//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_HAVAL
#define COGS_HEADER_CRYPTO_HAVAL


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


#ifdef DOXYGEN

/// @ingroup Crypto
/// @brief HAVAL Hash Algorithm
/// @tparam bits Bit width
/// @tparam passes Number of passes
template <size_t bits, size_t passes>
class haval : public hash
{
};

#else


template <size_t bits, size_t passes>
class haval : public serial_hash<haval<bits, passes>, 256, bits, 32, endian_t::little, 1024>
{
private:
	typedef serial_hash<haval<bits, passes>, 256, bits, 32, endian_t::little, 1024> base_t;
	typedef haval<bits, passes> this_t;

	static constexpr uint32_t version = 1;

	template <class derived_t, size_t, size_t, size_t, endian_t, size_t, size_t>
	friend class serial_hash;

	uint64_t	m_bitCount;
	uint32_t	m_state[32];

	static uint32_t f_1(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return (((x1) & ((x0) ^ (x4))) ^ ((x2) & (x5)) ^ ((x3)& (x6)) ^ (x0));
	}

	static uint32_t f_2(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return (((x2) & (((x1) & ~(x3)) ^ ((x4) & (x5)) ^ (x6) ^ (x0))) ^ ((x4) & ((x1) ^ (x5))) ^ ((x3) & (x5)) ^ (x0));
	}

	static uint32_t f_3(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return (((x3) & (((x1) & (x2)) ^ (x6) ^ (x0))) ^ ((x1) & (x4)) ^ ((x2)& (x5)) ^ (x0));
	}

	static uint32_t f_4(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return (((x4) & (((x5) & ~(x2)) ^ ((x3) & ~(x6)) ^ (x1) ^ (x6) ^ (x0))) ^ ((x3) & (((x1) & (x2)) ^ (x5) ^ (x6))) ^ ((x2) & (x6)) ^ (x0));
	}

	static uint32_t f_5(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return (((x0) & (((x1) & (x2) & (x3)) ^ ~(x5))) ^ ((x1) & (x4)) ^ ((x2)& (x5)) ^ ((x3) & (x6)));
	}

	static uint32_t Fphi_1(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		if (passes == 3)
			return f_1(x1, x0, x3, x5, x6, x2, x4);
		if (passes == 4)
			return f_1(x2, x6, x1, x4, x5, x3, x0);
		return f_1(x3, x4, x1, x0, x5, x2, x6);
	}

	static uint32_t Fphi_2(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		if (passes == 3)
			return f_2(x4, x2, x1, x0, x5, x3, x6);
		if (passes == 4)
			return f_2(x3, x5, x2, x0, x1, x6, x4);
		return f_2(x6, x2, x1, x0, x3, x4, x5);
	}

	static uint32_t Fphi_3(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		if (passes == 3)
			return f_3(x6, x1, x2, x3, x4, x5, x0);
		if (passes == 4)
			return f_3(x1, x4, x3, x6, x0, x2, x5);
		return f_3(x2, x6, x0, x4, x3, x1, x5);
	}

	static uint32_t Fphi_4(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		if (passes == 4)
			return f_4(x6, x4, x0, x5, x2, x1, x3);
		return f_4(x1, x5, x3, x2, x0, x4, x6);
	}

	static uint32_t Fphi_5(uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0)
	{
		return f_5(x2, x5, x0, x6, x4, x3, x1);
	}

	static void FF_1(uint32_t &x7, uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0, uint32_t w)
	{
		uint32_t temp = Fphi_1(x6, x5, x4, x3, x2, x1, x0);
		x7 = bit_rotate_right(temp, 7) + bit_rotate_right((x7), 11) + (w);
	}

	static void FF_2(uint32_t& x7, uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0, uint32_t w, uint32_t c)
	{
		uint32_t temp = Fphi_2(x6, x5, x4, x3, x2, x1, x0);
		x7 = bit_rotate_right(temp, 7) + bit_rotate_right((x7), 11) + (w)+(c);
	}

	static void FF_3(uint32_t& x7, uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0, uint32_t w, uint32_t c)
	{
		uint32_t temp = Fphi_3(x6, x5, x4, x3, x2, x1, x0);
		x7 = bit_rotate_right(temp, 7) + bit_rotate_right((x7), 11) + (w)+(c);
	}

	static void FF_4(uint32_t& x7, uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0, uint32_t w, uint32_t c)
	{
		uint32_t temp = Fphi_4(x6, x5, x4, x3, x2, x1, x0);
		x7 = bit_rotate_right(temp, 7) + bit_rotate_right((x7), 11) + (w)+(c);
	}

	static void FF_5(uint32_t& x7, uint32_t x6, uint32_t x5, uint32_t x4, uint32_t x3, uint32_t x2, uint32_t x1, uint32_t x0, uint32_t w, uint32_t c)
	{
		uint32_t temp = Fphi_5(x6, x5, x4, x3, x2, x1, x0);
		x7 = bit_rotate_right(temp, 7) + bit_rotate_right((x7), 11) + (w)+(c);
	}

	uint32_t EB(uint32_t a, uint32_t b, uint32_t c)
	{
		return (base_t::m_result[a] & (((~(uint32_t)0) << b) & ((~(uint32_t)0) >> (8 * sizeof(uint32_t) - b - c))));
	}

	static uint32_t S(uint32_t a, uint32_t b)
	{
		return (a > b) ? (a - b) : (32 + a - b);
	}

	uint32_t T128(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
	{
		return bit_rotate_right(EB(7, b, S(a, b)) | EB(6, c, S(b, c)) | EB(5, d, S(c, d)) | EB(4, e, S(d, e)), e);
	}

	uint32_t T160(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
	{
		return bit_rotate_right(EB(7, b, S(a, b)) | EB(6, c, S(b, c)) | EB(5, d, S(c, d)), d);
	}

	uint32_t T192(uint32_t a, uint32_t b, uint32_t c)
	{
		return bit_rotate_right(EB(7, b, S(a, b)) | EB(6, c, S(b, c)), c);
	}

	uint32_t T224(uint32_t a, uint32_t b)
	{
		return bit_rotate_right(EB(7, b, S(a, b)), b);
	}

protected:
	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		m_bitCount += base_t::stride_bits;

		uint32_t t0 = base_t::m_result[0];
		uint32_t t1 = base_t::m_result[1];
		uint32_t t2 = base_t::m_result[2];
		uint32_t t3 = base_t::m_result[3];
		uint32_t t4 = base_t::m_result[4];
		uint32_t t5 = base_t::m_result[5];
		uint32_t t6 = base_t::m_result[6];
		uint32_t t7 = base_t::m_result[7];

		uint32_t* w = m_state;

		// Pass 1
		FF_1(t7, t6, t5, t4, t3, t2, t1, t0, *(w));
		FF_1(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 1));
		FF_1(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 2));
		FF_1(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 3));
		FF_1(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 4));
		FF_1(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 5));
		FF_1(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 6));
		FF_1(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 7));

		FF_1(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 8));
		FF_1(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 9));
		FF_1(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 10));
		FF_1(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 11));
		FF_1(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 12));
		FF_1(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 13));
		FF_1(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 14));
		FF_1(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 15));

		FF_1(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 16));
		FF_1(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 17));
		FF_1(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 18));
		FF_1(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 19));
		FF_1(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 20));
		FF_1(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 21));
		FF_1(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 22));
		FF_1(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 23));

		FF_1(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 24));
		FF_1(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 25));
		FF_1(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 26));
		FF_1(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 27));
		FF_1(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 28));
		FF_1(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 29));
		FF_1(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 30));
		FF_1(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 31));

		// Pass 2
		FF_2(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 5), 0x452821E6);
		FF_2(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 14), 0x38D01377);
		FF_2(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 26), 0xBE5466CF);
		FF_2(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 18), 0x34E90C6C);
		FF_2(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 11), 0xC0AC29B7);
		FF_2(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 28), 0xC97C50DD);
		FF_2(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 7), 0x3F84D5B5);
		FF_2(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 16), 0xB5470917);

		FF_2(t7, t6, t5, t4, t3, t2, t1, t0, *(w), 0x9216D5D9);
		FF_2(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 23), 0x8979FB1B);
		FF_2(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 20), 0xD1310BA6);
		FF_2(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 22), 0x98DFB5AC);
		FF_2(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 1), 0x2FFD72DB);
		FF_2(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 10), 0xD01ADFB7);
		FF_2(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 4), 0xB8E1AFED);
		FF_2(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 8), 0x6A267E96);

		FF_2(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 30), 0xBA7C9045);
		FF_2(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 3), 0xF12C7F99);
		FF_2(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 21), 0x24A19947);
		FF_2(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 9), 0xB3916CF7);
		FF_2(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 17), 0x0801F2E2);
		FF_2(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 24), 0x858EFC16);
		FF_2(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 29), 0x636920D8);
		FF_2(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 6), 0x71574E69);

		FF_2(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 19), 0xA458FEA3);
		FF_2(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 12), 0xF4933D7E);
		FF_2(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 15), 0x0D95748F);
		FF_2(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 13), 0x728EB658);
		FF_2(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 2), 0x718BCD58);
		FF_2(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 25), 0x82154AEE);
		FF_2(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 31), 0x7B54A41D);
		FF_2(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 27), 0xC25A59B5);

		// Pass 3
		FF_3(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 19), 0x9C30D539);
		FF_3(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 9), 0x2AF26013);
		FF_3(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 4), 0xC5D1B023);
		FF_3(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 20), 0x286085F0);
		FF_3(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 28), 0xCA417918);
		FF_3(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 17), 0xB8DB38EF);
		FF_3(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 8), 0x8E79DCB0);
		FF_3(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 22), 0x603A180E);

		FF_3(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 29), 0x6C9E0E8B);
		FF_3(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 14), 0xB01E8A3E);
		FF_3(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 25), 0xD71577C1);
		FF_3(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 12), 0xBD314B27);
		FF_3(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 24), 0x78AF2FDA);
		FF_3(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 30), 0x55605C60);
		FF_3(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 16), 0xE65525F3);
		FF_3(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 26), 0xAA55AB94);

		FF_3(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 31), 0x57489862);
		FF_3(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 15), 0x63E81440);
		FF_3(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 7), 0x55CA396A);
		FF_3(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 3), 0x2AAB10B6);
		FF_3(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 1), 0xB4CC5C34);
		FF_3(t2, t1, t0, t7, t6, t5, t4, t3, *(w), 0x1141E8CE);
		FF_3(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 18), 0xA15486AF);
		FF_3(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 27), 0x7C72E993);

		FF_3(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 13), 0xB3EE1411);
		FF_3(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 6), 0x636FBC2A);
		FF_3(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 21), 0x2BA9C55D);
		FF_3(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 10), 0x741831F6);
		FF_3(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 23), 0xCE5C3E16);
		FF_3(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 11), 0x9B87931E);
		FF_3(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 5), 0xAFD6BA33);
		FF_3(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 2), 0x6C24CF5C);

		if (passes >= 4)
		{
			// Pass 4. executed only when PASS =4 or 5
			FF_4(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 24), 0x7A325381);
			FF_4(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 4), 0x28958677);
			FF_4(t5, t4, t3, t2, t1, t0, t7, t6, *(w), 0x3B8F4898);
			FF_4(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 14), 0x6B4BB9AF);
			FF_4(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 2), 0xC4BFE81B);
			FF_4(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 7), 0x66282193);
			FF_4(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 28), 0x61D809CC);
			FF_4(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 23), 0xFB21A991);

			FF_4(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 26), 0x487CAC60);
			FF_4(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 6), 0x5DEC8032);
			FF_4(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 30), 0xEF845D5D);
			FF_4(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 20), 0xE98575B1);
			FF_4(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 18), 0xDC262302);
			FF_4(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 25), 0xEB651B88);
			FF_4(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 19), 0x23893E81);
			FF_4(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 3), 0xD396ACC5);

			FF_4(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 22), 0x0F6D6FF3);
			FF_4(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 11), 0x83F44239);
			FF_4(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 31), 0x2E0B4482);
			FF_4(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 21), 0xA4842004);
			FF_4(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 8), 0x69C8F04A);
			FF_4(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 27), 0x9E1F9B5E);
			FF_4(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 12), 0x21C66842);
			FF_4(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 9), 0xF6E96C9A);

			FF_4(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 1), 0x670C9C61);
			FF_4(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 29), 0xABD388F0);
			FF_4(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 5), 0x6A51A0D2);
			FF_4(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 15), 0xD8542F68);
			FF_4(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 17), 0x960FA728);
			FF_4(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 10), 0xAB5133A3);
			FF_4(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 16), 0x6EEF0B6C);
			FF_4(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 13), 0x137A3BE4);
		}

		if (passes == 5)
		{
			// Pass 5. executed only when PASS = 5
			FF_5(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 27), 0xBA3BF050);
			FF_5(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 3), 0x7EFB2A98);
			FF_5(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 21), 0xA1F1651D);
			FF_5(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 26), 0x39AF0176);
			FF_5(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 17), 0x66CA593E);
			FF_5(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 11), 0x82430E88);
			FF_5(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 20), 0x8CEE8619);
			FF_5(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 29), 0x456F9FB4);

			FF_5(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 19), 0x7D84A5C3);
			FF_5(t6, t5, t4, t3, t2, t1, t0, t7, *(w), 0x3B8B5EBE);
			FF_5(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 12), 0xE06F75D8);
			FF_5(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 7), 0x85C12073);
			FF_5(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 13), 0x401A449F);
			FF_5(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 8), 0x56C16AA6);
			FF_5(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 31), 0x4ED3AA62);
			FF_5(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 10), 0x363F7706);

			FF_5(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 5), 0x1BFEDF72);
			FF_5(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 9), 0x429B023D);
			FF_5(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 14), 0x37D0D724);
			FF_5(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 30), 0xD00A1248);
			FF_5(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 18), 0xDB0FEAD3);
			FF_5(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 6), 0x49F1C09B);
			FF_5(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 28), 0x075372C9);
			FF_5(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 24), 0x80991B7B);

			FF_5(t7, t6, t5, t4, t3, t2, t1, t0, *(w + 2), 0x25D479D8);
			FF_5(t6, t5, t4, t3, t2, t1, t0, t7, *(w + 23), 0xF6E8DEF7);
			FF_5(t5, t4, t3, t2, t1, t0, t7, t6, *(w + 16), 0xE3FE501A);
			FF_5(t4, t3, t2, t1, t0, t7, t6, t5, *(w + 22), 0xB6794C3B);
			FF_5(t3, t2, t1, t0, t7, t6, t5, t4, *(w + 4), 0x976CE0BD);
			FF_5(t2, t1, t0, t7, t6, t5, t4, t3, *(w + 1), 0x04C006BA);
			FF_5(t1, t0, t7, t6, t5, t4, t3, t2, *(w + 25), 0xC1A94FB6);
			FF_5(t0, t7, t6, t5, t4, t3, t2, t1, *(w + 15), 0x409F60C4);
		}

		base_t::m_result[0] += t0;
		base_t::m_result[1] += t1;
		base_t::m_result[2] += t2;
		base_t::m_result[3] += t3;
		base_t::m_result[4] += t4;
		base_t::m_result[5] += t5;
		base_t::m_result[6] += t6;
		base_t::m_result[7] += t7;
	}

	void terminate()
	{
		uint64_t bitCount = m_bitCount + (base_t::m_blockProgress * base_t::digit_bits) + base_t::m_digitProgress;

		// Add terminating byte.  Might cause this block to process, if last byte.
		base_t::add_byte(0x01);

		if (base_t::m_blockProgress == base_t::stride_digits - 3)
		{
			if (base_t::m_digitProgress > 2)
				process_digit();
		}

		while (base_t::m_blockProgress != base_t::stride_digits - 3)
			process_digit();

		m_state[base_t::stride_digits - 3] = base_t::m_curDigit | (((uint32_t)base_t::digest_bytes << 25) | ((uint32_t)passes << 19) | ((uint32_t)version << 16));
		m_state[base_t::stride_digits - 2] = (uint32_t)bitCount;
		m_state[base_t::stride_digits - 1] = (uint32_t)(bitCount >> base_t::digit_bits);
		process_block();

		switch (base_t::digest_bits)
		{
		case 128:
			base_t::m_result[0] += T128(8, 0, 24, 16, 8);
			base_t::m_result[1] += T128(16, 8, 0, 24, 16);
			base_t::m_result[2] += T128(24, 16, 8, 0, 24);
			base_t::m_result[3] += T128(0, 24, 16, 8, 0);
			break;

		case 160:
			base_t::m_result[0] += T160(6, 0, 25, 19);
			base_t::m_result[1] += T160(12, 6, 0, 25);
			base_t::m_result[2] += T160(19, 12, 6, 0);
			base_t::m_result[3] += T160(25, 19, 12, 6);
			base_t::m_result[4] += T160(0, 25, 19, 12);
			break;

		case 192:
			base_t::m_result[0] += T192(5, 0, 26);
			base_t::m_result[1] += T192(10, 5, 0);
			base_t::m_result[2] += T192(16, 10, 5);
			base_t::m_result[3] += T192(21, 16, 10);
			base_t::m_result[4] += T192(26, 21, 16);
			base_t::m_result[5] += T192(0, 26, 21);
			break;

		case 224:
			base_t::m_result[0] += T224(0, 27);
			base_t::m_result[1] += T224(27, 22);
			base_t::m_result[2] += T224(22, 18);
			base_t::m_result[3] += T224(18, 13);
			base_t::m_result[4] += T224(13, 9);
			base_t::m_result[5] += T224(9, 4);
			base_t::m_result[6] += T224(4, 0);
			break;

		case 256:
			break;

		default:
			COGS_ASSERT(false);
		}
	}

public:
	haval()
	{
		m_bitCount = 0;
		base_t::m_result[0] = 0x243F6A88;
		base_t::m_result[1] = 0x85A308D3;
		base_t::m_result[2] = 0x13198A2E;
		base_t::m_result[3] = 0x03707344;
		base_t::m_result[4] = 0xA4093822;
		base_t::m_result[5] = 0x299F31D0;
		base_t::m_result[6] = 0x082EFA98;
		base_t::m_result[7] = 0xEC4E6C89;
	}

	haval(const this_t& src)
		: base_t(src)
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	this_t& operator=(const this_t& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < base_t::m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


#endif


}
}


#endif
