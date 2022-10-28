//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_ADLER32
#define COGS_HEADER_CRYPTO_ADLER32


#include "cogs/crypto/hash_int.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {


/// @defgroup Crypto Cryptography, Hashes, CRCs, and Checksums
/// @{
/// @brief Cryptography, Hash, CRC, and Checksum algorithms
/// @}


/// @ingroup Crypto
/// @brief Namespace for cryptography, hashes, CRCs and checksums
namespace crypto {

/// @ingroup Crypto
/// @brief Alder-32 Checksum Algorithm
class adler32 : public hash_int<32>
{
private:
	uint32_t m_sum1;
	uint32_t m_sum2;
	size_t m_modCountDown;

public:
	virtual size_t get_block_size() const { return 1; }

	virtual void update(const io::buffer& buf)
	{
		const uint8_t* p = (const uint8_t*)buf.get_const_ptr();
		size_t bufLength = buf.get_length();
		while (bufLength > 0)
		{
			size_t n = m_modCountDown;
			if (n > bufLength)
				n = bufLength;
			bufLength -= n;
			do {
				m_sum1 = m_sum1 + *p++;
				m_sum2 = m_sum2 + m_sum1;
			} while (--n);
			if (m_modCountDown > n)
			{
				m_modCountDown -= n;
				break;
			}

			m_modCountDown = 5552;
			m_sum1 %= 65521;
			m_sum2 %= 65521;
		}
	}

	virtual uint_t get_hash_int() const
	{
		uint32_t sum1 = m_sum1 % 65521;
		uint32_t sum2 = m_sum2 % 65521;
		return (sum2 << 16) | sum1;
	}

	adler32()
	{
		m_sum1 = 1;
		m_sum2 = 0;
		m_modCountDown = 5552;
	}

	adler32(const adler32& src)
	{
		m_sum1 = src.m_sum1;
		m_sum2 = src.m_sum2;
		m_modCountDown = src.m_modCountDown;
	}

	adler32& operator=(const adler32& src)
	{
		m_sum1 = src.m_sum1;
		m_sum2 = src.m_sum2;
		m_modCountDown = src.m_modCountDown;
		return *this;
	}
};


}
}


#endif
