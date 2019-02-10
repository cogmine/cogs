//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_CIPHER
#define COGS_CIPHER


#include "cogs/io/buffer.hpp"
#include "cogs/io/composite_buffer.hpp"


namespace cogs {
namespace crypto {


template <class derived_t, size_t key_bits, size_t stride_bits = key_bits>
class block_cipher
{
public:
	static constexpr size_t key_bytes = bits_to_bytes<key_bits>::value;
	static constexpr size_t stride_bytes = bits_to_bytes<stride_bits>::value;

	void set_key(const unsigned char(&key)[key_bytes])
	{
		derived_t::set_key(key);
	}

	void process_block(const unsigned char(&input)[stride_bytes], unsigned char(&output)[stride_bytes])
	{
		derived_t::process_block(input, output);
	}
};


template <class derived_t, class block_cipher_t>
class cipher_mode
{
public:
	static constexpr size_t key_bytes = block_cipher_t::key_bytes;
	static constexpr size_t stride_bytes = block_cipher_t::stride_bytes;

private:
	block_cipher_t m_blockCipher;
	unsigned int m_blockIndex;
	unsigned char m_currentBlock[stride_bytes];

public:
	cipher_mode()
	{
		m_blockIndex = 0;
	}

	void set_key(const unsigned char(&key)[key_bytes])
	{
		m_blockCipher.set_key(key);
	}

	void set_key(const io::buffer& buf)
	{
		unsigned int keyIndex = 0;
		unsigned char key[key_bytes];
		io::buffer::const_iterator itor = buf.get_first_const_iterator();
		while (!!itor)
		{
			key[keyIndex++] = (unsigned char)*itor;
			if (keyIndex == key_bytes)
				break;
			itor++;
		}
		while (keyIndex < key_bytes)
			key[keyIndex++] = 0;
		m_blockCipher.set_key(key);
	}

	void set_key(const io::composite_buffer& buf)
	{
		unsigned int keyIndex = 0;
		unsigned char key[key_bytes];
		io::composite_buffer::const_iterator itor = buf.get_first_const_iterator();
		while (!!itor)
		{
			key[keyIndex++] = (unsigned char)*itor;
			if (keyIndex == key_bytes)
				break;
			itor++;
		}
		while (keyIndex < key_bytes)
			key[keyIndex++] = 0;
		m_blockCipher.set_key(key);
	}

	io::buffer process_buffer(const io::buffer& buf)
	{
		io::composite_buffer buf2(buf);
		return derived_t::process_block(buf2);
	}

	io::buffer process_buffer(const io::composite_buffer& buf)
	{
		return derived_t::process_block(buf);
	}

	io::buffer terminate()
	{
		return derived_t::terminate();
	}
};


// Electronic Codebook
//	Each block is encrypted/decrypted separately.
//	Encryption and decryption can both be parallelized.
//	Stream can recover from bad data, provided block length is not disrupted.
template <class block_cipher_t>
class ECB : public cipher_mode<ECB<block_cipher_t>, block_cipher_t>
{
};

// Cipher Block Chaining
//	The result of each cipher block is XOR'ed into the input of the next.
//	Encryption cannot be parallelized, but decryption can be.
//	Stream can recover from bad data, provided block length is not disrupted.
template <class block_cipher_t>
class CBC : public cipher_mode<CBC<block_cipher_t>, block_cipher_t>
{
};

// Propagating Cipher Block Chaining
//	The result of each cipher block is XOR'ed with its own input and into the input of the next.
//	Neither Encryption nor decryption can be parallelized.
template <class block_cipher_t>
class PCBC : public cipher_mode<PCBC<block_cipher_t>, block_cipher_t>
{
};

// Cipher Feedback
//	IV is first state.  State is encryped, then input is XOR'ed with the state.  Repeated.
//	Encryption cannot be parallelized, but decryption can be.
//	Stream can recover from bad data, provided block length is not disrupted.
template <class block_cipher_t>
class CFB : public cipher_mode<CFB<block_cipher_t>, block_cipher_t>
{
};

// Output Feedback
//	IV is first state.  State is encrypted, state is saved, input is XOR'ed with state to generate cipher text, saved state becomes new state.
//	Decryption is idential to encryption.
//	Neither Encryption nor decryption can be parallelized.
template <class block_cipher_t>
class OFB : public cipher_mode<OFB<block_cipher_t>, block_cipher_t>
{
};

// Counter
//	Similar to OFB, but encrypts an IV and Counter as the only direct input to the cipher.
// Encryption and Decryption can both be parallized.
template <class block_cipher_t, class counter_t>
class CTR : public cipher_mode<CTR<block_cipher_t, counter_t>, block_cipher_t>
{
};

//CTS/RBT

// CCM
// OCB
// GCM	- Galois/Counter Mode
// XEX/XTR




class DES : public block_cipher<DES, 64>
{
private:
	uint32_t m_keyScheduleHigh[16];
	uint32_t m_keyScheduleLow[16];

public:
	void set_key(const unsigned char(&key)[8])
	{
		static constexpr uint32_t left_halves[16] =
		{
			0x00000000, 0x00000001, 0x00000100, 0x00000101,
			0x00010000, 0x00010001, 0x00010100, 0x00010101,
			0x01000000, 0x01000001, 0x01000100, 0x01000101,
			0x01010000, 0x01010001, 0x01010100, 0x01010101
		};

		static constexpr uint32_t right_halves[16] =
		{
			0x00000000, 0x01000000, 0x00010000, 0x01010000,
			0x00000100, 0x01000100, 0x00010100, 0x01010100,
			0x00000001, 0x01000001, 0x00010001, 0x01010001,
			0x00000101, 0x01000101, 0x00010101, 0x01010101,
		};

		uint32_t left = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
		uint32_t right = (key[4] << 24) | (key[5] << 16) | (key[6] << 8) | key[7];

		uint32_t tmp = ((right >> 4) ^ left) & 0x0F0F0F0F;
		left ^= tmp;
		right ^= (tmp << 4);
		tmp = (right ^ left) & 0x10101010;
		left ^= tmp;
		right ^= tmp;

		left = (left_halves[left & 0xF] << 3) | (left_halves[(left >> 8) & 0xF] << 2)
			| (left_halves[(left >> 16) & 0xF] << 1) | (left_halves[(left >> 24) & 0xF])
			| (left_halves[(left >> 5) & 0xF] << 7) | (left_halves[(left >> 13) & 0xF] << 6)
			| (left_halves[(left >> 21) & 0xF] << 5) | (left_halves[left >> 29] << 4);

		right = (right_halves[(right >> 1) & 0xF] << 3) | (right_halves[(right >> 9) & 0xF] << 2)
			| (right_halves[(right >> 17) & 0xF] << 1) | (right_halves[(right >> 25) & 0xF])
			| (right_halves[(right >> 4) & 0xF] << 7) | (right_halves[(right >> 12) & 0xF] << 6)
			| (right_halves[(right >> 20) & 0xF] << 5) | (right_halves[right >> 28] << 4);

		left &= 0x0FFFFFFF;
		right &= 0x0FFFFFFF;

		for (unsigned int i = 0; i < 16; i++)
		{
			if (i < 2 || i == 8 || i == 15)
			{
				left = ((left << 1) | (left >> 27)) & 0x0FFFFFFF;
				right = ((right << 1) | (right >> 27)) & 0x0FFFFFFF;
			}
			else
			{
				left = ((left << 2) | (left >> 26)) & 0x0FFFFFFF;
				right = ((right << 2) | (right >> 26)) & 0x0FFFFFFF;
			}

			m_keyScheduleHigh[i] = ((left << 4) & 0x24000000) | ((left << 28) & 0x10000000)
				| ((left << 14) & 0x08000000) | ((left << 18) & 0x02080000)
				| ((left << 6) & 0x01000000) | ((left << 9) & 0x00200000)
				| ((left >> 1) & 0x00100000) | ((left << 10) & 0x00040000)
				| ((left << 2) & 0x00020000) | ((left >> 10) & 0x00010000)
				| ((right >> 13) & 0x00002000) | ((right >> 4) & 0x00001000)
				| ((right << 6) & 0x00000800) | ((right >> 1) & 0x00000400)
				| ((right >> 14) & 0x00000200) | ((right)& 0x00000100)
				| ((right >> 5) & 0x00000020) | ((right >> 10) & 0x00000010)
				| ((right >> 3) & 0x00000008) | ((right >> 18) & 0x00000004)
				| ((right >> 26) & 0x00000002) | ((right >> 24) & 0x00000001);

			m_keyScheduleLow[i] = ((left << 15) & 0x20000000) | ((left << 17) & 0x10000000)
				| ((left << 10) & 0x08000000) | ((left << 22) & 0x04000000)
				| ((left >> 2) & 0x02000000) | ((left << 1) & 0x01000000)
				| ((left << 16) & 0x00200000) | ((left << 11) & 0x00100000)
				| ((left << 3) & 0x00080000) | ((left >> 6) & 0x00040000)
				| ((left << 15) & 0x00020000) | ((left >> 4) & 0x00010000)
				| ((right >> 2) & 0x00002000) | ((right << 8) & 0x00001000)
				| ((right >> 14) & 0x00000808) | ((right >> 9) & 0x00000400)
				| ((right)& 0x00000200) | ((right << 7) & 0x00000100)
				| ((right >> 7) & 0x00000020) | ((right >> 3) & 0x00000011)
				| ((right << 2) & 0x00000004) | ((right >> 21) & 0x00000002);
		}
	}

	void process_block(const unsigned char(&input)[8], unsigned char(&output)[8])
	{
		static constexpr uint32_t SB1[64] =
		{
			0x01010400, 0x00000000, 0x00010000, 0x01010404,
			0x01010004, 0x00010404, 0x00000004, 0x00010000,
			0x00000400, 0x01010400, 0x01010404, 0x00000400,
			0x01000404, 0x01010004, 0x01000000, 0x00000004,
			0x00000404, 0x01000400, 0x01000400, 0x00010400,
			0x00010400, 0x01010000, 0x01010000, 0x01000404,
			0x00010004, 0x01000004, 0x01000004, 0x00010004,
			0x00000000, 0x00000404, 0x00010404, 0x01000000,
			0x00010000, 0x01010404, 0x00000004, 0x01010000,
			0x01010400, 0x01000000, 0x01000000, 0x00000400,
			0x01010004, 0x00010000, 0x00010400, 0x01000004,
			0x00000400, 0x00000004, 0x01000404, 0x00010404,
			0x01010404, 0x00010004, 0x01010000, 0x01000404,
			0x01000004, 0x00000404, 0x00010404, 0x01010400,
			0x00000404, 0x01000400, 0x01000400, 0x00000000,
			0x00010004, 0x00010400, 0x00000000, 0x01010004
		};

		static constexpr uint32_t SB2[64] =
		{
			0x80108020, 0x80008000, 0x00008000, 0x00108020,
			0x00100000, 0x00000020, 0x80100020, 0x80008020,
			0x80000020, 0x80108020, 0x80108000, 0x80000000,
			0x80008000, 0x00100000, 0x00000020, 0x80100020,
			0x00108000, 0x00100020, 0x80008020, 0x00000000,
			0x80000000, 0x00008000, 0x00108020, 0x80100000,
			0x00100020, 0x80000020, 0x00000000, 0x00108000,
			0x00008020, 0x80108000, 0x80100000, 0x00008020,
			0x00000000, 0x00108020, 0x80100020, 0x00100000,
			0x80008020, 0x80100000, 0x80108000, 0x00008000,
			0x80100000, 0x80008000, 0x00000020, 0x80108020,
			0x00108020, 0x00000020, 0x00008000, 0x80000000,
			0x00008020, 0x80108000, 0x00100000, 0x80000020,
			0x00100020, 0x80008020, 0x80000020, 0x00100020,
			0x00108000, 0x00000000, 0x80008000, 0x00008020,
			0x80000000, 0x80100020, 0x80108020, 0x00108000
		};

		static constexpr uint32_t SB3[64] =
		{
			0x00000208, 0x08020200, 0x00000000, 0x08020008,
			0x08000200, 0x00000000, 0x00020208, 0x08000200,
			0x00020008, 0x08000008, 0x08000008, 0x00020000,
			0x08020208, 0x00020008, 0x08020000, 0x00000208,
			0x08000000, 0x00000008, 0x08020200, 0x00000200,
			0x00020200, 0x08020000, 0x08020008, 0x00020208,
			0x08000208, 0x00020200, 0x00020000, 0x08000208,
			0x00000008, 0x08020208, 0x00000200, 0x08000000,
			0x08020200, 0x08000000, 0x00020008, 0x00000208,
			0x00020000, 0x08020200, 0x08000200, 0x00000000,
			0x00000200, 0x00020008, 0x08020208, 0x08000200,
			0x08000008, 0x00000200, 0x00000000, 0x08020008,
			0x08000208, 0x00020000, 0x08000000, 0x08020208,
			0x00000008, 0x00020208, 0x00020200, 0x08000008,
			0x08020000, 0x08000208, 0x00000208, 0x08020000,
			0x00020208, 0x00000008, 0x08020008, 0x00020200
		};

		static constexpr uint32_t SB4[64] =
		{
			0x00802001, 0x00002081, 0x00002081, 0x00000080,
			0x00802080, 0x00800081, 0x00800001, 0x00002001,
			0x00000000, 0x00802000, 0x00802000, 0x00802081,
			0x00000081, 0x00000000, 0x00800080, 0x00800001,
			0x00000001, 0x00002000, 0x00800000, 0x00802001,
			0x00000080, 0x00800000, 0x00002001, 0x00002080,
			0x00800081, 0x00000001, 0x00002080, 0x00800080,
			0x00002000, 0x00802080, 0x00802081, 0x00000081,
			0x00800080, 0x00800001, 0x00802000, 0x00802081,
			0x00000081, 0x00000000, 0x00000000, 0x00802000,
			0x00002080, 0x00800080, 0x00800081, 0x00000001,
			0x00802001, 0x00002081, 0x00002081, 0x00000080,
			0x00802081, 0x00000081, 0x00000001, 0x00002000,
			0x00800001, 0x00002001, 0x00802080, 0x00800081,
			0x00002001, 0x00002080, 0x00800000, 0x00802001,
			0x00000080, 0x00800000, 0x00002000, 0x00802080
		};

		static constexpr uint32_t SB5[64] =
		{
			0x00000100, 0x02080100, 0x02080000, 0x42000100,
			0x00080000, 0x00000100, 0x40000000, 0x02080000,
			0x40080100, 0x00080000, 0x02000100, 0x40080100,
			0x42000100, 0x42080000, 0x00080100, 0x40000000,
			0x02000000, 0x40080000, 0x40080000, 0x00000000,
			0x40000100, 0x42080100, 0x42080100, 0x02000100,
			0x42080000, 0x40000100, 0x00000000, 0x42000000,
			0x02080100, 0x02000000, 0x42000000, 0x00080100,
			0x00080000, 0x42000100, 0x00000100, 0x02000000,
			0x40000000, 0x02080000, 0x42000100, 0x40080100,
			0x02000100, 0x40000000, 0x42080000, 0x02080100,
			0x40080100, 0x00000100, 0x02000000, 0x42080000,
			0x42080100, 0x00080100, 0x42000000, 0x42080100,
			0x02080000, 0x00000000, 0x40080000, 0x42000000,
			0x00080100, 0x02000100, 0x40000100, 0x00080000,
			0x00000000, 0x40080000, 0x02080100, 0x40000100
		};

		static constexpr uint32_t SB6[64] =
		{
			0x20000010, 0x20400000, 0x00004000, 0x20404010,
			0x20400000, 0x00000010, 0x20404010, 0x00400000,
			0x20004000, 0x00404010, 0x00400000, 0x20000010,
			0x00400010, 0x20004000, 0x20000000, 0x00004010,
			0x00000000, 0x00400010, 0x20004010, 0x00004000,
			0x00404000, 0x20004010, 0x00000010, 0x20400010,
			0x20400010, 0x00000000, 0x00404010, 0x20404000,
			0x00004010, 0x00404000, 0x20404000, 0x20000000,
			0x20004000, 0x00000010, 0x20400010, 0x00404000,
			0x20404010, 0x00400000, 0x00004010, 0x20000010,
			0x00400000, 0x20004000, 0x20000000, 0x00004010,
			0x20000010, 0x20404010, 0x00404000, 0x20400000,
			0x00404010, 0x20404000, 0x00000000, 0x20400010,
			0x00000010, 0x00004000, 0x20400000, 0x00404010,
			0x00004000, 0x00400010, 0x20004010, 0x00000000,
			0x20404000, 0x20000000, 0x00400010, 0x20004010
		};

		static constexpr uint32_t SB7[64] =
		{
			0x00200000, 0x04200002, 0x04000802, 0x00000000,
			0x00000800, 0x04000802, 0x00200802, 0x04200800,
			0x04200802, 0x00200000, 0x00000000, 0x04000002,
			0x00000002, 0x04000000, 0x04200002, 0x00000802,
			0x04000800, 0x00200802, 0x00200002, 0x04000800,
			0x04000002, 0x04200000, 0x04200800, 0x00200002,
			0x04200000, 0x00000800, 0x00000802, 0x04200802,
			0x00200800, 0x00000002, 0x04000000, 0x00200800,
			0x04000000, 0x00200800, 0x00200000, 0x04000802,
			0x04000802, 0x04200002, 0x04200002, 0x00000002,
			0x00200002, 0x04000000, 0x04000800, 0x00200000,
			0x04200800, 0x00000802, 0x00200802, 0x04200800,
			0x00000802, 0x04000002, 0x04200802, 0x04200000,
			0x00200800, 0x00000000, 0x00000002, 0x04200802,
			0x00000000, 0x00200802, 0x04200000, 0x00000800,
			0x04000002, 0x04000800, 0x00000800, 0x00200002
		};

		static constexpr uint32_t SB8[64] =
		{
			0x10001040, 0x00001000, 0x00040000, 0x10041040,
			0x10000000, 0x10001040, 0x00000040, 0x10000000,
			0x00040040, 0x10040000, 0x10041040, 0x00041000,
			0x10041000, 0x00041040, 0x00001000, 0x00000040,
			0x10040000, 0x10000040, 0x10001000, 0x00001040,
			0x00041000, 0x00040040, 0x10040040, 0x10041000,
			0x00001040, 0x00000000, 0x00000000, 0x10040040,
			0x10000040, 0x10001000, 0x00041040, 0x00040000,
			0x00041040, 0x00040000, 0x10041000, 0x00001000,
			0x00000040, 0x10040040, 0x00001000, 0x00041040,
			0x10001000, 0x00000040, 0x10000040, 0x10040000,
			0x10040040, 0x10000000, 0x00040000, 0x10001040,
			0x00000000, 0x10041040, 0x00040040, 0x10000040,
			0x10040000, 0x10001000, 0x10001040, 0x00000000,
			0x10041040, 0x00041000, 0x00041000, 0x00001040,
			0x00001040, 0x00040040, 0x10000000, 0x10041000
		};

		uint32_t* keyScheduleHigh = m_keyScheduleHigh;
		uint32_t* keyScheduleLow = m_keyScheduleLow;

		uint32_t left = (input[0] << 24) | (input[1] << 16) | (input[2] << 8) | input[3];
		uint32_t right = (input[4] << 24) | (input[5] << 16) | (input[6] << 8) | input[7];

		uint32_t tmp = ((left >>  4) ^ right) & 0x0F0F0F0F;
		right ^= tmp;
		left ^= (tmp <<  4);
		tmp = ((left >> 16) ^ right) & 0x0000FFFF;
		right ^= tmp;
		left ^= (tmp << 16);
		tmp = ((right >>  2) ^ left) & 0x33333333;
		left ^= tmp;
		right ^= (tmp <<  2);
		tmp = ((right >>  8) ^ left) & 0x00FF00FF;
		left ^= tmp;
		right ^= (tmp <<  8);
		right = ((right << 1) | (right >> 31)) & 0xFFFFFFFF;
		tmp = (left ^ right) & 0xAAAAAAAA;
		right ^= tmp;
		left ^= tmp;
		left = ((left << 1) | (left >> 31)) & 0xFFFFFFFF;

		for (unsigned int i = 0; i < 8; i++)
		{
			tmp = *keyScheduleHigh++ ^ right;
			left ^= SB8[tmp & 0x3F] ^ SB6[(tmp >> 8) & 0x3F] ^ SB4[(tmp >> 16) & 0x3F] ^ SB2[(tmp >> 24) & 0x3F];
			tmp = *keyScheduleLow++ ^ ((right << 28) | (right >> 4));
			left ^= SB7[tmp & 0x3F] ^ SB5[(tmp >> 8) & 0x3F] ^ SB3[(tmp >> 16) & 0x3F] ^ SB1[(tmp >> 24) & 0x3F];

			tmp = *keyScheduleHigh++ ^ left;
			right ^= SB8[tmp & 0x3F] ^ SB6[(tmp >> 8) & 0x3F] ^ SB4[(tmp >> 16) & 0x3F] ^ SB2[(tmp >> 24) & 0x3F];
			tmp = *keyScheduleLow++ ^ ((left << 28) | (left >> 4));
			right ^= SB7[tmp & 0x3F] ^ SB5[(tmp >> 8) & 0x3F] ^ SB3[(tmp >> 16) & 0x3F] ^ SB1[(tmp >> 24) & 0x3F];
		}

		right = ((right << 31) | (right >> 1)) & 0xFFFFFFFF;
		tmp = (right ^ left) & 0xAAAAAAAA;
		right ^= tmp;
		left ^= tmp;
		left = ((left << 31) | (left >> 1)) & 0xFFFFFFFF;
		tmp = ((left >> 8) ^ right) & 0x00FF00FF;
		right ^= tmp;
		left ^= (tmp << 8);
		tmp = ((left >> 2) ^ right) & 0x33333333;
		right ^= tmp;
		left ^= (tmp << 2);
		tmp = ((right >> 16) ^ left) & 0x0000FFFF;
		left ^= tmp;
		right ^= (tmp << 16);
		tmp = ((right >> 4) ^ left) & 0x0F0F0F0F;
		left ^= tmp;
		right ^= (tmp << 4);
		
		output[0] = (unsigned char)(right >> 24);
		output[1] = (unsigned char)(right >> 16);
		output[2] = (unsigned char)(right >> 8);
		output[3] = (unsigned char)(right);
		output[4] = (unsigned char)(left >> 24);
		output[5] = (unsigned char)(left >> 16);
		output[6] = (unsigned char)(left >> 8);
		output[7] = (unsigned char)(left);
	}
};


}
}


#endif
