//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_CRYPTO_SHA3
#define COGS_HEADER_CRYPTO_SHA3


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {


#ifdef DOXYGEN

/// @ingroup Crypto
/// @brief Keccak Cryptographic Hash Algorithm
/// @tparam bits Bit width.  Default: 512
/// @tparam terminator value.  Default: 0x01
/// @tparam stride_bits Numbers of bits read per input block.  Default: (calculated)
template <size_t bits = 512, uint8_t terminator = 0x01, size_t stride_bits = ((200 - (2 * (bits / 8))) * 8)>
class keccak : public hash
{
};


#else


template <size_t bits = 512, uint8_t terminator = 0x01, size_t stride_bits = ((200 - (2 * (bits / 8))) * 8)>
class keccak : public serial_hash<keccak<bits, terminator, stride_bits>, 1600, bits, 64, endian_t::little, stride_bits, stride_bits>
{
private:
	typedef keccak<bits, terminator, stride_bits> this_t;
	typedef serial_hash<keccak<bits, terminator, stride_bits>, 1600, bits, 64, endian_t::little, stride_bits, stride_bits> base_t;

	template <class derived_t, size_t, size_t, size_t, endian_t, size_t, size_t>
	friend class serial_hash;

	void r(uint64_t k1, uint64_t k2)
	{
		uint64_t Aba = base_t::m_result[0];
		uint64_t Abe = base_t::m_result[1];
		uint64_t Abi = base_t::m_result[2];
		uint64_t Abo = base_t::m_result[3];
		uint64_t Abu = base_t::m_result[4];
		uint64_t Aga = base_t::m_result[5];
		uint64_t Age = base_t::m_result[6];
		uint64_t Agi = base_t::m_result[7];
		uint64_t Ago = base_t::m_result[8];
		uint64_t Agu = base_t::m_result[9];
		uint64_t Aka = base_t::m_result[10];
		uint64_t Ake = base_t::m_result[11];
		uint64_t Aki = base_t::m_result[12];
		uint64_t Ako = base_t::m_result[13];
		uint64_t Aku = base_t::m_result[14];
		uint64_t Ama = base_t::m_result[15];
		uint64_t Ame = base_t::m_result[16];
		uint64_t Ami = base_t::m_result[17];
		uint64_t Amo = base_t::m_result[18];
		uint64_t Amu = base_t::m_result[19];
		uint64_t Asa = base_t::m_result[20];
		uint64_t Ase = base_t::m_result[21];
		uint64_t Asi = base_t::m_result[22];
		uint64_t Aso = base_t::m_result[23];
		uint64_t Asu = base_t::m_result[24];

		// Theta
		uint64_t BCa = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
		uint64_t BCe = Abe ^ Age ^ Ake ^ Ame ^ Ase;
		uint64_t BCi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
		uint64_t BCo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
		uint64_t BCu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

		// thetaRhoPiChiIotaPrepareTheta(round  , A, E)
		uint64_t Da = BCu ^ bit_rotate_left(BCe, 1);
		uint64_t De = BCa ^ bit_rotate_left(BCi, 1);
		uint64_t Di = BCe ^ bit_rotate_left(BCo, 1);
		uint64_t Do = BCi ^ bit_rotate_left(BCu, 1);
		uint64_t Du = BCo ^ bit_rotate_left(BCa, 1);

		Aba ^= Da;
		BCa = Aba;
		Age ^= De;
		BCe = bit_rotate_left(Age, 44);
		Aki ^= Di;
		BCi = bit_rotate_left(Aki, 43);
		Amo ^= Do;
		BCo = bit_rotate_left(Amo, 21);
		Asu ^= Du;
		BCu = bit_rotate_left(Asu, 14);
		uint64_t Eba = BCa ^ ((~BCe) & BCi);
		Eba ^= k1;
		uint64_t Ebe = BCe ^ ((~BCi) & BCo);
		uint64_t Ebi = BCi ^ ((~BCo) & BCu);
		uint64_t Ebo = BCo ^ ((~BCu) & BCa);
		uint64_t Ebu = BCu ^ ((~BCa) & BCe);

		Abo ^= Do;
		BCa = bit_rotate_left(Abo, 28);
		Agu ^= Du;
		BCe = bit_rotate_left(Agu, 20);
		Aka ^= Da;
		BCi = bit_rotate_left(Aka, 3);
		Ame ^= De;
		BCo = bit_rotate_left(Ame, 45);
		Asi ^= Di;
		BCu = bit_rotate_left(Asi, 61);
		uint64_t Ega = BCa ^ ((~BCe) & BCi);
		uint64_t Ege = BCe ^ ((~BCi) & BCo);
		uint64_t Egi = BCi ^ ((~BCo) & BCu);
		uint64_t Ego = BCo ^ ((~BCu) & BCa);
		uint64_t Egu = BCu ^ ((~BCa) & BCe);

		Abe ^= De;
		BCa = bit_rotate_left(Abe, 1);
		Agi ^= Di;
		BCe = bit_rotate_left(Agi, 6);
		Ako ^= Do;
		BCi = bit_rotate_left(Ako, 25);
		Amu ^= Du;
		BCo = bit_rotate_left(Amu, 8);
		Asa ^= Da;
		BCu = bit_rotate_left(Asa, 18);
		uint64_t Eka = BCa ^ ((~BCe) & BCi);
		uint64_t Eke = BCe ^ ((~BCi) & BCo);
		uint64_t Eki = BCi ^ ((~BCo) & BCu);
		uint64_t Eko = BCo ^ ((~BCu) & BCa);
		uint64_t Eku = BCu ^ ((~BCa) & BCe);

		Abu ^= Du;
		BCa = bit_rotate_left(Abu, 27);
		Aga ^= Da;
		BCe = bit_rotate_left(Aga, 36);
		Ake ^= De;
		BCi = bit_rotate_left(Ake, 10);
		Ami ^= Di;
		BCo = bit_rotate_left(Ami, 15);
		Aso ^= Do;
		BCu = bit_rotate_left(Aso, 56);
		uint64_t Ema = BCa ^ ((~BCe) & BCi);
		uint64_t Eme = BCe ^ ((~BCi) & BCo);
		uint64_t Emi = BCi ^ ((~BCo) & BCu);
		uint64_t Emo = BCo ^ ((~BCu) & BCa);
		uint64_t Emu = BCu ^ ((~BCa) & BCe);

		Abi ^= Di;
		BCa = bit_rotate_left(Abi, 62);
		Ago ^= Do;
		BCe = bit_rotate_left(Ago, 55);
		Aku ^= Du;
		BCi = bit_rotate_left(Aku, 39);
		Ama ^= Da;
		BCo = bit_rotate_left(Ama, 41);
		Ase ^= De;
		BCu = bit_rotate_left(Ase, 2);
		uint64_t Esa = BCa ^ ((~BCe) & BCi);
		uint64_t Ese = BCe ^ ((~BCi) & BCo);
		uint64_t Esi = BCi ^ ((~BCo) & BCu);
		uint64_t Eso = BCo ^ ((~BCu) & BCa);
		uint64_t Esu = BCu ^ ((~BCa) & BCe);

		// Theta
		BCa = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
		BCe = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
		BCi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
		BCo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
		BCu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

		// thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
		Da = BCu ^ bit_rotate_left(BCe, 1);
		De = BCa ^ bit_rotate_left(BCi, 1);
		Di = BCe ^ bit_rotate_left(BCo, 1);
		Do = BCi ^ bit_rotate_left(BCu, 1);
		Du = BCo ^ bit_rotate_left(BCa, 1);

		Eba ^= Da;
		BCa = Eba;
		Ege ^= De;
		BCe = bit_rotate_left(Ege, 44);
		Eki ^= Di;
		BCi = bit_rotate_left(Eki, 43);
		Emo ^= Do;
		BCo = bit_rotate_left(Emo, 21);
		Esu ^= Du;
		BCu = bit_rotate_left(Esu, 14);
		Aba = BCa ^ ((~BCe) & BCi);
		Aba ^= k2;
		Abe = BCe ^ ((~BCi) & BCo);
		Abi = BCi ^ ((~BCo) & BCu);
		Abo = BCo ^ ((~BCu) & BCa);
		Abu = BCu ^ ((~BCa) & BCe);

		Ebo ^= Do;
		BCa = bit_rotate_left(Ebo, 28);
		Egu ^= Du;
		BCe = bit_rotate_left(Egu, 20);
		Eka ^= Da;
		BCi = bit_rotate_left(Eka, 3);
		Eme ^= De;
		BCo = bit_rotate_left(Eme, 45);
		Esi ^= Di;
		BCu = bit_rotate_left(Esi, 61);
		Aga = BCa ^ ((~BCe) & BCi);
		Age = BCe ^ ((~BCi) & BCo);
		Agi = BCi ^ ((~BCo) & BCu);
		Ago = BCo ^ ((~BCu) & BCa);
		Agu = BCu ^ ((~BCa) & BCe);

		Ebe ^= De;
		BCa = bit_rotate_left(Ebe, 1);
		Egi ^= Di;
		BCe = bit_rotate_left(Egi, 6);
		Eko ^= Do;
		BCi = bit_rotate_left(Eko, 25);
		Emu ^= Du;
		BCo = bit_rotate_left(Emu, 8);
		Esa ^= Da;
		BCu = bit_rotate_left(Esa, 18);
		Aka = BCa ^ ((~BCe) & BCi);
		Ake = BCe ^ ((~BCi) & BCo);
		Aki = BCi ^ ((~BCo) & BCu);
		Ako = BCo ^ ((~BCu) & BCa);
		Aku = BCu ^ ((~BCa) & BCe);

		Ebu ^= Du;
		BCa = bit_rotate_left(Ebu, 27);
		Ega ^= Da;
		BCe = bit_rotate_left(Ega, 36);
		Eke ^= De;
		BCi = bit_rotate_left(Eke, 10);
		Emi ^= Di;
		BCo = bit_rotate_left(Emi, 15);
		Eso ^= Do;
		BCu = bit_rotate_left(Eso, 56);
		Ama = BCa ^ ((~BCe) & BCi);
		Ame = BCe ^ ((~BCi) & BCo);
		Ami = BCi ^ ((~BCo) & BCu);
		Amo = BCo ^ ((~BCu) & BCa);
		Amu = BCu ^ ((~BCa) & BCe);

		Ebi ^= Di;
		BCa = bit_rotate_left(Ebi, 62);
		Ego ^= Do;
		BCe = bit_rotate_left(Ego, 55);
		Eku ^= Du;
		BCi = bit_rotate_left(Eku, 39);
		Ema ^= Da;
		BCo = bit_rotate_left(Ema, 41);
		Ese ^= De;
		BCu = bit_rotate_left(Ese, 2);
		Asa = BCa ^ ((~BCe) & BCi);
		Ase = BCe ^ ((~BCi) & BCo);
		Asi = BCi ^ ((~BCo) & BCu);
		Aso = BCo ^ ((~BCu) & BCa);
		Asu = BCu ^ ((~BCa) & BCe);

		base_t::m_result[0] = Aba;
		base_t::m_result[1] = Abe;
		base_t::m_result[2] = Abi;
		base_t::m_result[3] = Abo;
		base_t::m_result[4] = Abu;
		base_t::m_result[5] = Aga;
		base_t::m_result[6] = Age;
		base_t::m_result[7] = Agi;
		base_t::m_result[8] = Ago;
		base_t::m_result[9] = Agu;
		base_t::m_result[10] = Aka;
		base_t::m_result[11] = Ake;
		base_t::m_result[12] = Aki;
		base_t::m_result[13] = Ako;
		base_t::m_result[14] = Aku;
		base_t::m_result[15] = Ama;
		base_t::m_result[16] = Ame;
		base_t::m_result[17] = Ami;
		base_t::m_result[18] = Amo;
		base_t::m_result[19] = Amu;
		base_t::m_result[20] = Asa;
		base_t::m_result[21] = Ase;
		base_t::m_result[22] = Asi;
		base_t::m_result[23] = Aso;
		base_t::m_result[24] = Asu;
	}

protected:
	void process_block()
	{
		r(0x0000000000000001ULL, 0x0000000000008082ULL);
		r(0x800000000000808aULL, 0x8000000080008000ULL);
		r(0x000000000000808bULL, 0x0000000080000001ULL);
		r(0x8000000080008081ULL, 0x8000000000008009ULL);
		r(0x000000000000008aULL, 0x0000000000000088ULL);
		r(0x0000000080008009ULL, 0x000000008000000aULL);
		r(0x000000008000808bULL, 0x800000000000008bULL);
		r(0x8000000000008089ULL, 0x8000000000008003ULL);
		r(0x8000000000008002ULL, 0x8000000000000080ULL);
		r(0x000000000000800aULL, 0x800000008000000aULL);
		r(0x8000000080008081ULL, 0x8000000000008080ULL);
		r(0x0000000080000001ULL, 0x8000000080008008ULL);
	}

	void process_digit()
	{
		base_t::m_result[base_t::m_blockProgress] ^= base_t::m_curDigit;
		base_t::advance_digit();
	}

	void terminate()
	{
		// If there is only one byte left, terminate the block on the mark as well
		if ((base_t::m_blockProgress == base_t::stride_digits - 1) && (base_t::m_digitProgress == base_t::digit_bytes - 1))
			base_t::add_byte(0x80 | terminator);
		else
		{
			base_t::add_byte(terminator);

			// Pad block, leaving space at the end for 2 digits
			for (size_t i = base_t::m_blockProgress; i < base_t::stride_digits - 1; i++)
				process_digit();

			base_t::m_curDigit |= 0x8000000000000000ULL;
			process_digit();
		}
	}

public:
	keccak()
	{
		for (int i = 0; i < 25; i++)
			base_t::m_result[i] = 0;
	}

	keccak(const this_t& src)
		: base_t(src)
	{ }

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		return *this;
	}
};

#endif

/// @ingroup Crypto
/// @brief SHA-3 (Secure Hash Algorithm 3)
/// @tparam bits Bit width.  Default: 512
template <size_t bits = 512>
class sha3 : public keccak<bits, 0x06>
{
public:
	sha3()	{ }

	sha3(const sha3<bits>& src)
		: keccak<bits, 0x06>(src)
	{ }

	sha3<bits>& operator=(const sha3<bits>& src)
	{
		keccak<bits, 0x06>::operator=(src);
		return *this;
	}
};


/// @ingroup Crypto
/// @brief SHAKE128 Hash Algorithm
template <size_t bits>
class shake128 : public keccak<bits, 0x1F, 1344>
{
public:
	shake128()	{ }

	shake128(const shake128<bits>& src)
		: keccak<bits, 0x1F, 1344>(src)
	{ }

	shake128<bits>& operator=(const shake128<bits>& src)
	{
		keccak<bits, 0x1F, 1344>::operator=(src);
		return *this;
	}
};


/// @ingroup Crypto
/// @brief SHAKE256 Hash Algorithm
template <size_t bits>
class shake256 : public keccak<bits, 0x1F, 1088>
{
public:
	shake256()	{ }

	shake256(const shake256<bits>& src)
		: keccak<bits, 0x1F, 1088>(src)
	{ }

	shake256<bits>& operator=(const shake256<bits>& src)
	{
		keccak<bits, 0x1F, 1088>::operator=(src);
		return *this;
	}
};


}
}


#endif
