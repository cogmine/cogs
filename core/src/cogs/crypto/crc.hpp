//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CRC
#define COGS_CRC


#include "cogs/crypto/hash_int.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/bits_to_bytes.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief Generic CRC template
/// @tparam bits Bit width of the CRC
/// @tparam is_input_reflected True if the input value is reflected
/// @tparam is_output_reflected True if the output value is reflected
/// @tparam initial_value The initial value of the CRC
/// @tparam xor_out True if the output value is XOR'ed
template <size_t bits, ulongest poly, bool is_input_reflected, bool is_output_reflected, ulongest initial_value, ulongest xor_out>
class crc : public hash_int<bits>
{
public:
	static const size_t width_bits = bits;
	static const size_t width_bytes = bits_to_bytes<width_bits>::value;

	typedef typename hash_int<bits>::uint_t	crc_t;

	static const crc_t init_value = (crc_t)initial_value;
	static const crc_t poly_value = (crc_t)poly;

	typedef crc<width_bits, poly, is_input_reflected, is_output_reflected, initial_value, xor_out>	this_t;

private:
	template <ulongest x, size_t bits2>
	class const_reflect
	{
	public:
		static const crc_t value = (const_reflect<(x >> 1), bits2>::value >> 1) | ((x & 1) ? ((crc_t)1 << (bits2 - 1)) : 0);
	};

	template <size_t bits2>
	class const_reflect<0, bits2>
	{
	public:
		static const crc_t value = 0;
	};

	template <size_t bits2>
	class const_reflect<1, bits2>
	{
	public:
		static const crc_t value = ((crc_t)1 << (bits2 - 1));
	};

	static const crc_t high_bit = (crc_t)1 << (width_bits - 1);
	static const crc_t mask = ((high_bit - 1) << 1) | 1;
	static const crc_t reflected_poly = const_reflect<poly, width_bits>::value;
	static const crc_t reflected_init_value = const_reflect<init_value, width_bits>::value;

	template <uint8_t index>
	class calculate_crc_table_entry
	{
	private:
		static const crc_t x = is_input_reflected ? index : const_reflect<index, 8>::value;
		static const crc_t y1 = (x >> 1) ^ ((x & 1) ? reflected_poly : 0);
		static const crc_t y2 = (y1 >> 1) ^ ((y1 & 1) ? reflected_poly : 0);
		static const crc_t y3 = (y2 >> 1) ^ ((y2 & 1) ? reflected_poly : 0);
		static const crc_t y4 = (y3 >> 1) ^ ((y3 & 1) ? reflected_poly : 0);
		static const crc_t y5 = (y4 >> 1) ^ ((y4 & 1) ? reflected_poly : 0);
		static const crc_t y6 = (y5 >> 1) ^ ((y5 & 1) ? reflected_poly : 0);
		static const crc_t y7 = (y6 >> 1) ^ ((y6 & 1) ? reflected_poly : 0);
		static const crc_t y8 = (y7 >> 1) ^ ((y7 & 1) ? reflected_poly : 0);
	public:
		static const crc_t value = (is_input_reflected ? y8 : const_reflect<y8, width_bits>::value) & mask;
	};

	static crc_t reflect(const crc_t& c)
	{
		crc_t result = 0;
		ulongest j = 1;
		for (ulongest i = (ulongest)1 << (width_bits - 1); i; i >>= 1)
		{
			if (c & i)
				result |= j;
			j <<= 1;
		}

		return result;
	}

	crc_t	m_crc;
	static const crc_t table[256];

public:
	template <crc_t old_value, uint8_t byte_in>
	class const_update
	{
	private:
		static const size_t shift1 = (width_bits <= 8) ? 0 : 8;
		static const size_t shift2 = ((width_bits <= 8) || is_input_reflected) ? 0 : (width_bits - 8);
		static const uint8_t table_index = (uint8_t)(old_value >> shift2) ^ byte_in;
		static const crc_t table_entry = calculate_crc_table_entry<table_index>::value;
	public:
		static const crc_t value = ((width_bits <= 8)
			? table_entry
			: (is_input_reflected
			? (table_entry ^ (old_value >> shift1))
			: (table_entry ^ (crc_t)(old_value << shift1)))) & mask;
	};

	template <crc_t crc_value>
	class const_get
	{
	public:
		static const crc_t value = (((is_input_reflected != is_output_reflected) ? const_reflect<crc_value, width_bits>::value : crc_value) ^ xor_out) & mask;
	};

	// A CRC of an empty block
	static const crc_t null_result = const_get<init_value>::value;

private:
	static const crc_t null_crc_adjusted = (init_value ^ xor_out) & mask;

	static const size_t margin_bits = ((width_bytes * 8) > width_bits) ? ((width_bytes * 8) - width_bits) : 0;
	static const size_t margin_shift = ((width_bits <= 8) || is_input_reflected) ? 0 : margin_bits;

	template <size_t byte_index, crc_t old_value>
	class helper
	{
	public:
		template <bool short_circuit, bool unused>
		class helper2;

		template <bool unused>
		class helper2<true, unused>
		{
		public:
			static const crc_t value = old_value;
		};

		template <bool unused>
		class helper2<false, unused>
		{
		private:
			static const bool short_circuit_next = (byte_index <= 1);
			static const size_t next_byte_index = byte_index - 1;
			static const size_t dif = (width_bytes - byte_index);
			static const size_t shift3 = (8 * (is_input_reflected ? dif : next_byte_index));
			static const uint8_t byte_in = (uint8_t)((null_crc_adjusted << margin_shift) >> shift3);
			static const crc_t new_value = const_update<old_value, byte_in>::value;
		public:
			static const crc_t value = helper<next_byte_index, new_value>::template helper2<short_circuit_next, false>::value;
		};
	};

public:

	// If a CRC is parsed at the end of a block, it should result in a known constant CRC value
	//static const crc_t success_result = helper<width_bytes, init_value, false>::value;
	static const crc_t success_result = helper<width_bytes, init_value>::template helper2<false, false>::value;

	crc()
		:	m_crc(init_value)
	{ }

	crc(const this_t& toCopy)
		:	m_crc(toCopy.m_crc)
	{ }

	crc(const void* buf, size_t n)
		:	m_crc(init_value)
	{
		update(buf, n);
	}

	this_t& operator=(const this_t& toCopy)
	{
		m_crc = toCopy.m_crc;
	}

	crc_t peek() const		{ return m_crc; }

	crc_t get_crc() const
	{
		crc_t x;
		if (is_input_reflected != is_output_reflected)
			x = reflect(m_crc);
		else
			x = m_crc;
		x ^= xor_out;
		if ((8 * sizeof(crc_t)) > width_bits)
			x &= mask;
		return x;
	}

	virtual size_t get_block_size() const { return 1; }

	virtual crc_t get_hash_int()		{ return get_crc(); }
	
	virtual io::buffer get_hash()
	{
		crc_t crcValue = m_crc;
		crcValue ^= xor_out;
		if ((8 * sizeof(crc_t)) > width_bits)
			crcValue &= mask;

		if (!is_input_reflected && (width_bits > 8) && ((width_bytes * 8) > width_bits))
			crcValue <<= (width_bytes * 8) - width_bits;

		io::buffer result(width_bytes);
		uint8_t* resultPtr = (uint8_t*)result.get_ptr();
		for (size_t i = 0; i < width_bytes; i++)
		{
			if (is_input_reflected)
				resultPtr[i] = (uint8_t)(crcValue >> (i * 8));
			else
				resultPtr[i] = (uint8_t)(crcValue >> (((width_bytes - 1) - i) * 8));
		}
		return result;
	}

	virtual bool was_block_terminated() const		{ return m_crc == success_result; }
	virtual bool can_terminate() const				{ return true; }

	virtual void update(const io::buffer& buf)
	{
		uint8_t* ptr = (uint8_t*)buf.get_const_ptr();
		size_t bufLength = buf.get_length();
		for (size_t i = 0; i < bufLength; i++)
			update(ptr[i]);
	}

	void update(uint8_t c)
	{
		if (width_bits <= 8)
			m_crc = table[(uint8_t)m_crc ^ c];
		else
		{
			static const size_t shift1 = (width_bits <= 8) ? 0 : 8;
			if (is_input_reflected)
				m_crc = table[(uint8_t)m_crc ^ c] ^ (m_crc >> shift1);
			else
			{
				static const size_t shift2 = (width_bits <= 8) ? 0 : (width_bits - 8);
				m_crc = table[(uint8_t)(m_crc >> shift2) ^ c] ^ (crc_t)(m_crc << shift1);
			}
		}

		if ((8 * sizeof(crc_t)) > width_bits)
			m_crc &= mask;
	}
};


// Constant CRC table, generated at compile time.
template <size_t bits, ulongest poly, bool is_input_reflected, bool is_output_reflected, ulongest initial_value, ulongest xor_out>
const typename crc<bits, poly, is_input_reflected, is_output_reflected, initial_value, xor_out>::crc_t crc<bits, poly, is_input_reflected, is_output_reflected, initial_value, xor_out>::table[256] =
{
	calculate_crc_table_entry<0>::value, calculate_crc_table_entry<1>::value, calculate_crc_table_entry<2>::value, calculate_crc_table_entry<3>::value,
	calculate_crc_table_entry<4>::value, calculate_crc_table_entry<5>::value, calculate_crc_table_entry<6>::value, calculate_crc_table_entry<7>::value,
	calculate_crc_table_entry<8>::value, calculate_crc_table_entry<9>::value, calculate_crc_table_entry<10>::value, calculate_crc_table_entry<11>::value,
	calculate_crc_table_entry<12>::value, calculate_crc_table_entry<13>::value, calculate_crc_table_entry<14>::value, calculate_crc_table_entry<15>::value,
	calculate_crc_table_entry<16>::value, calculate_crc_table_entry<17>::value, calculate_crc_table_entry<18>::value, calculate_crc_table_entry<19>::value,
	calculate_crc_table_entry<20>::value, calculate_crc_table_entry<21>::value, calculate_crc_table_entry<22>::value, calculate_crc_table_entry<23>::value,
	calculate_crc_table_entry<24>::value, calculate_crc_table_entry<25>::value, calculate_crc_table_entry<26>::value, calculate_crc_table_entry<27>::value,
	calculate_crc_table_entry<28>::value, calculate_crc_table_entry<29>::value, calculate_crc_table_entry<30>::value, calculate_crc_table_entry<31>::value,
	calculate_crc_table_entry<32>::value, calculate_crc_table_entry<33>::value, calculate_crc_table_entry<34>::value, calculate_crc_table_entry<35>::value,
	calculate_crc_table_entry<36>::value, calculate_crc_table_entry<37>::value, calculate_crc_table_entry<38>::value, calculate_crc_table_entry<39>::value,
	calculate_crc_table_entry<40>::value, calculate_crc_table_entry<41>::value, calculate_crc_table_entry<42>::value, calculate_crc_table_entry<43>::value,
	calculate_crc_table_entry<44>::value, calculate_crc_table_entry<45>::value, calculate_crc_table_entry<46>::value, calculate_crc_table_entry<47>::value,
	calculate_crc_table_entry<48>::value, calculate_crc_table_entry<49>::value, calculate_crc_table_entry<50>::value, calculate_crc_table_entry<51>::value,
	calculate_crc_table_entry<52>::value, calculate_crc_table_entry<53>::value, calculate_crc_table_entry<54>::value, calculate_crc_table_entry<55>::value,
	calculate_crc_table_entry<56>::value, calculate_crc_table_entry<57>::value, calculate_crc_table_entry<58>::value, calculate_crc_table_entry<59>::value,
	calculate_crc_table_entry<60>::value, calculate_crc_table_entry<61>::value, calculate_crc_table_entry<62>::value, calculate_crc_table_entry<63>::value,
	calculate_crc_table_entry<64>::value, calculate_crc_table_entry<65>::value, calculate_crc_table_entry<66>::value, calculate_crc_table_entry<67>::value,
	calculate_crc_table_entry<68>::value, calculate_crc_table_entry<69>::value, calculate_crc_table_entry<70>::value, calculate_crc_table_entry<71>::value,
	calculate_crc_table_entry<72>::value, calculate_crc_table_entry<73>::value, calculate_crc_table_entry<74>::value, calculate_crc_table_entry<75>::value,
	calculate_crc_table_entry<76>::value, calculate_crc_table_entry<77>::value, calculate_crc_table_entry<78>::value, calculate_crc_table_entry<79>::value,
	calculate_crc_table_entry<80>::value, calculate_crc_table_entry<81>::value, calculate_crc_table_entry<82>::value, calculate_crc_table_entry<83>::value,
	calculate_crc_table_entry<84>::value, calculate_crc_table_entry<85>::value, calculate_crc_table_entry<86>::value, calculate_crc_table_entry<87>::value,
	calculate_crc_table_entry<88>::value, calculate_crc_table_entry<89>::value, calculate_crc_table_entry<90>::value, calculate_crc_table_entry<91>::value,
	calculate_crc_table_entry<92>::value, calculate_crc_table_entry<93>::value, calculate_crc_table_entry<94>::value, calculate_crc_table_entry<95>::value,
	calculate_crc_table_entry<96>::value, calculate_crc_table_entry<97>::value, calculate_crc_table_entry<98>::value, calculate_crc_table_entry<99>::value,
	calculate_crc_table_entry<100>::value, calculate_crc_table_entry<101>::value, calculate_crc_table_entry<102>::value, calculate_crc_table_entry<103>::value,
	calculate_crc_table_entry<104>::value, calculate_crc_table_entry<105>::value, calculate_crc_table_entry<106>::value, calculate_crc_table_entry<107>::value,
	calculate_crc_table_entry<108>::value, calculate_crc_table_entry<109>::value, calculate_crc_table_entry<110>::value, calculate_crc_table_entry<111>::value,
	calculate_crc_table_entry<112>::value, calculate_crc_table_entry<113>::value, calculate_crc_table_entry<114>::value, calculate_crc_table_entry<115>::value,
	calculate_crc_table_entry<116>::value, calculate_crc_table_entry<117>::value, calculate_crc_table_entry<118>::value, calculate_crc_table_entry<119>::value,
	calculate_crc_table_entry<120>::value, calculate_crc_table_entry<121>::value, calculate_crc_table_entry<122>::value, calculate_crc_table_entry<123>::value,
	calculate_crc_table_entry<124>::value, calculate_crc_table_entry<125>::value, calculate_crc_table_entry<126>::value, calculate_crc_table_entry<127>::value,
	calculate_crc_table_entry<128>::value, calculate_crc_table_entry<129>::value, calculate_crc_table_entry<130>::value, calculate_crc_table_entry<131>::value,
	calculate_crc_table_entry<132>::value, calculate_crc_table_entry<133>::value, calculate_crc_table_entry<134>::value, calculate_crc_table_entry<135>::value,
	calculate_crc_table_entry<136>::value, calculate_crc_table_entry<137>::value, calculate_crc_table_entry<138>::value, calculate_crc_table_entry<139>::value,
	calculate_crc_table_entry<140>::value, calculate_crc_table_entry<141>::value, calculate_crc_table_entry<142>::value, calculate_crc_table_entry<143>::value,
	calculate_crc_table_entry<144>::value, calculate_crc_table_entry<145>::value, calculate_crc_table_entry<146>::value, calculate_crc_table_entry<147>::value,
	calculate_crc_table_entry<148>::value, calculate_crc_table_entry<149>::value, calculate_crc_table_entry<150>::value, calculate_crc_table_entry<151>::value,
	calculate_crc_table_entry<152>::value, calculate_crc_table_entry<153>::value, calculate_crc_table_entry<154>::value, calculate_crc_table_entry<155>::value,
	calculate_crc_table_entry<156>::value, calculate_crc_table_entry<157>::value, calculate_crc_table_entry<158>::value, calculate_crc_table_entry<159>::value,
	calculate_crc_table_entry<160>::value, calculate_crc_table_entry<161>::value, calculate_crc_table_entry<162>::value, calculate_crc_table_entry<163>::value,
	calculate_crc_table_entry<164>::value, calculate_crc_table_entry<165>::value, calculate_crc_table_entry<166>::value, calculate_crc_table_entry<167>::value,
	calculate_crc_table_entry<168>::value, calculate_crc_table_entry<169>::value, calculate_crc_table_entry<170>::value, calculate_crc_table_entry<171>::value,
	calculate_crc_table_entry<172>::value, calculate_crc_table_entry<173>::value, calculate_crc_table_entry<174>::value, calculate_crc_table_entry<175>::value,
	calculate_crc_table_entry<176>::value, calculate_crc_table_entry<177>::value, calculate_crc_table_entry<178>::value, calculate_crc_table_entry<179>::value,
	calculate_crc_table_entry<180>::value, calculate_crc_table_entry<181>::value, calculate_crc_table_entry<182>::value, calculate_crc_table_entry<183>::value,
	calculate_crc_table_entry<184>::value, calculate_crc_table_entry<185>::value, calculate_crc_table_entry<186>::value, calculate_crc_table_entry<187>::value,
	calculate_crc_table_entry<188>::value, calculate_crc_table_entry<189>::value, calculate_crc_table_entry<190>::value, calculate_crc_table_entry<191>::value,
	calculate_crc_table_entry<192>::value, calculate_crc_table_entry<193>::value, calculate_crc_table_entry<194>::value, calculate_crc_table_entry<195>::value,
	calculate_crc_table_entry<196>::value, calculate_crc_table_entry<197>::value, calculate_crc_table_entry<198>::value, calculate_crc_table_entry<199>::value,
	calculate_crc_table_entry<200>::value, calculate_crc_table_entry<201>::value, calculate_crc_table_entry<202>::value, calculate_crc_table_entry<203>::value,
	calculate_crc_table_entry<204>::value, calculate_crc_table_entry<205>::value, calculate_crc_table_entry<206>::value, calculate_crc_table_entry<207>::value,
	calculate_crc_table_entry<208>::value, calculate_crc_table_entry<209>::value, calculate_crc_table_entry<210>::value, calculate_crc_table_entry<211>::value,
	calculate_crc_table_entry<212>::value, calculate_crc_table_entry<213>::value, calculate_crc_table_entry<214>::value, calculate_crc_table_entry<215>::value,
	calculate_crc_table_entry<216>::value, calculate_crc_table_entry<217>::value, calculate_crc_table_entry<218>::value, calculate_crc_table_entry<219>::value,
	calculate_crc_table_entry<220>::value, calculate_crc_table_entry<221>::value, calculate_crc_table_entry<222>::value, calculate_crc_table_entry<223>::value,
	calculate_crc_table_entry<224>::value, calculate_crc_table_entry<225>::value, calculate_crc_table_entry<226>::value, calculate_crc_table_entry<227>::value,
	calculate_crc_table_entry<228>::value, calculate_crc_table_entry<229>::value, calculate_crc_table_entry<230>::value, calculate_crc_table_entry<231>::value,
	calculate_crc_table_entry<232>::value, calculate_crc_table_entry<233>::value, calculate_crc_table_entry<234>::value, calculate_crc_table_entry<235>::value,
	calculate_crc_table_entry<236>::value, calculate_crc_table_entry<237>::value, calculate_crc_table_entry<238>::value, calculate_crc_table_entry<239>::value,
	calculate_crc_table_entry<240>::value, calculate_crc_table_entry<241>::value, calculate_crc_table_entry<242>::value, calculate_crc_table_entry<243>::value,
	calculate_crc_table_entry<244>::value, calculate_crc_table_entry<245>::value, calculate_crc_table_entry<246>::value, calculate_crc_table_entry<247>::value,
	calculate_crc_table_entry<248>::value, calculate_crc_table_entry<249>::value, calculate_crc_table_entry<250>::value, calculate_crc_table_entry<251>::value,
	calculate_crc_table_entry<252>::value, calculate_crc_table_entry<253>::value, calculate_crc_table_entry<254>::value, calculate_crc_table_entry<255>::value
};


// Some predefined CRCs.
// Mainly from the web.  There may be errors.
//
typedef crc<3, 0x03, true, true, 0x07, 0>		crc3_rohc;

typedef crc<4, 0x03, true, true, 0, 0>			crc4_itu;

typedef crc<5, 0x09, false, false, 0x09, 0>		crc5_epc;
typedef crc<5, 0x15, true, true, 0, 0>			crc5_itu;
typedef crc<5, 0x05, true, true, 0x1f, 0x1f>	crc5_usb;

typedef crc<6, 0x27, false, false, 0x3f, 0>		crc6_cdma2000a;
typedef crc<6, 0x07, false, false, 0x3f, 0>		crc6_cdma2000b;
typedef crc<6, 0x19, true, true, 0, 0>			crc6_darc;
typedef crc<6, 0x03, true, true, 0, 0>			crc6_itu;

typedef crc<7, 0x09, false, false, 0, 0>		crc7;
typedef crc<7, 0x4f, true, true, 0x7f, 0>		crc7_rohc;

typedef crc<8, 0x07, false, false, 0, 0>						crc8;
typedef crc<8, 0x9b, false, false, 0xff, 0>						crc8_cdma2000;
typedef crc<8, 0x39, true, true, 0, 0>							crc8_darc;
typedef crc<8, 0xd5, false, false, 0, 0>						crc8_dvb_s2;
typedef crc<8, 0x1d, true, true, 0xff, 0>						crc8_ebu;
typedef crc<8, 0x1D, false, false, 0xFD, 0>						crc8_icode;
typedef crc<8, 0x07, false, false, 0, 0x55>						crc8_itu;
typedef crc<8, 0x1D, false, false, 0xFF, 0xFF>					crc8_j1850;
typedef crc<8, 0x31, true, true, 0, 0>							crc8_maxim;
typedef crc<8, 0x07, true, true, 0xFF, 0>						crc8_rohc;
typedef crc<8, 0x9B, true, true, 0, 0>							crc8_wcdma;

typedef crc<10, 0x233, false, false, 0, 0>						crc10;
typedef crc<10, 0x3d9, false, false, 0x03FF, 0>					crc10_cdma2000;

typedef crc<11, 0x385, false, false, 0x01a, 0>					crc11;

typedef crc<12, 0x80f, false, true, 0, 0>						crc12_3gpp;
typedef crc<12, 0xf13, false, false, 0xfff, 0>					crc12_cdma2000;
typedef crc<12, 0x80f, false, false, 0, 0>						crc12_dect;

typedef crc<13, 0x1cf5, false, false, 0, 0>						crc13_bbc;

typedef crc<14, 0x0805, true, true, 0, 0>						crc14_darc;

typedef crc<15, 0x4599, false, false, 0, 0>						crc15;

typedef crc<15, 0x6815, false, false, 0, 0x01>					crc15_mpt1327;

typedef crc<16, 0x8005, true, true, 0, 0>						crc16;
typedef crc<16, 0x1021, false, false, 0x1D0F, 0>				crc16_aug_ccitt;
typedef crc<16, 0x8005, false, false, 0, 0>						crc16_buypass;
typedef crc<16, 0x1021, false, false, 0xFFFF, 0>				crc16_ccitt_false;
typedef crc<16, 0xc867, false, false, 0xFFFF, 0>				crc16_cdma2000;
typedef crc<16, 0x8005, false, false, 0x800D, 0>				crc16_dds_110;
typedef crc<16, 0x0589, false, false, 0, 0x0001>				crc16_dect_r;
typedef crc<16, 0x0589, false, false, 0, 0>						crc16_dect_x;
typedef crc<16, 0x3D65, true, true, 0, 0xFFFF>					crc16_dnp;
typedef crc<16, 0x3D65, false, false, 0, 0xFFFF>				crc16_en_13757;
typedef crc<16, 0x1021, false, false, 0xFFFF, 0xFFFF>			crc16_genibus;
typedef crc<16, 0x8005, true, true, 0, 0xFFFF>					crc16_maxim;
typedef crc<16, 0x1021, true, true, 0xFFFF, 0>					crc16_mcrf4xx;
typedef crc<16, 0x1021, true, true, 0x554D, 0>					crc16_riello;
typedef crc<16, 0x8BB7, false, false, 0, 0>						crc16_t10_dif;
typedef crc<16, 0xA097, false, false, 0, 0>						crc16_teledisk;
typedef crc<16, 0x1021, true, true, 0x3791, 0>					crc16_tms37157;
typedef crc<16, 0x8005, true, true, 0xFFFF, 0xFFFF>				crc16_usb;
typedef crc<16, 0x1021, true, true, 0x6363, 0>					crc16a;
typedef crc<16, 0x1021, true, true, 0, 0>						crc16_kermit;
typedef crc<16, 0x8005, true, true, 0xFFFF, 0>					crc16_modbus;
typedef crc<16, 0x1021, true, true, 0xFFFF, 0xFFFF>				crc16_x25;
typedef crc<16, 0x1021, false, false, 0, 0>						crc16_xmodem;

typedef crc<24, 0x864CFB, false, false, 0xB704CE, 0>			crc24;
typedef crc<24, 0x5D6DCB, false, false, 0xFEDCBA, 0>			crc24_flexray_a;
typedef crc<24, 0x5D6DCB, false, false, 0xABCDEF, 0>			crc24_flexray_b;

typedef crc<31, 0x04c11db7, false, false, 0x7fffffff, 0x7fffffff>	crc31_philips;

typedef crc<32, 0x04C11DB7, true, true, 0xFFFFFFFF, 0xFFFFFFFF>		crc32;
typedef crc<32, 0x04C11DB7, false, false, 0xFFFFFFFF, 0xFFFFFFFF>	crc32_bzip2;
typedef crc<32, 0x1EDC6F41, true, true, 0xFFFFFFFF, 0xFFFFFFFF>		crc32c;
typedef crc<32, 0xA833982B, true, true, 0xFFFFFFFF, 0xFFFFFFFF>		crc32d;
typedef crc<32, 0x04C11DB7, false, false, 0xFFFFFFFF, 0>			crc32_mpeg2;
typedef crc<32, 0x04C11DB7, false, false, 0, 0xFFFFFFFF>			crc32_posix;
typedef crc<32, 0x814141AB, false, false, 0, 0>						crc32q;
typedef crc<32, 0x04C11DB7, true, true, 0xFFFFFFFF, 0>				crc32_jam;
typedef crc<32, 0x000000AF, false, false, 0, 0>						crc32_xfer;

typedef crc<40, 0x0000000004820009ULL, false, false, 0, 0x000000ffffffffffULL >		crc40_gsm;

typedef crc<64, 0x42F0E1EBA9EA3693ULL, false, false, 0, 0>			crc64;
typedef crc<64, 0x000000000000001BULL, true, true, 0, 0>			crc64_iso;
typedef crc<64, 0x42F0E1EBA9EA3693ULL, false, false, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL>	crc64_we;
typedef crc<64, 0x42F0E1EBA9EA3693ULL, true, true, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL>	crc64_xz;
typedef crc<64, 0xAD93D23594C935A9ULL, true, true, 0xFFFFFFFFFFFFFFFFULL, 0>						crc64_jones;

//typedef crc<82, 0x0308c0111011401440411, true, true, 0, 0>		crc82_darc;

}
}


#endif
