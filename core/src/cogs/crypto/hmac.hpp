//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_HMAC
#define COGS_HEADER_CRYPTO_HMAC


#include "cogs/crypto/hash.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief HMAC (Hash-base Message Authentication Code) class
/// @tparam hash_t The internal hash type of the HMAC
template <class hash_t>
class hmac : public hash
{
private:
	hash_t m_innerHash;
	hash_t m_outerHash;

	hmac() = delete;

public:
	hmac(const io::buffer& key)
	{
		io::buffer k = key;
		size_t blockSize = m_innerHash.get_block_size();
		size_t keyLength = k.get_length();
		if (keyLength > blockSize)
			k = calculate_hash<hash_t>(k);

		io::buffer pad(blockSize);
		uint8_t* padPtr = (uint8_t*)pad.get_ptr();
		if (keyLength < blockSize)
		{
			const uint8_t* keyPtr = (const uint8_t*)k.get_const_ptr();
			size_t i = 0;
			for (; i < keyLength; i++)
				padPtr[i] = 0x36 ^ keyPtr[i];
			for (; i < blockSize; i++)
				padPtr[i] = 0x36;
			m_innerHash.update(pad);
			i = 0;
			for (; i < keyLength; i++)
				padPtr[i] = 0x5c ^ keyPtr[i];
			for (; i < blockSize; i++)
				padPtr[i] = 0x5c;
		}
		else
		{
			const uint8_t* keyPtr = (const uint8_t*)k.get_const_ptr();
			for (size_t i = 0; i < blockSize; i++)
				padPtr[i] = 0x36 ^ keyPtr[i];
			m_innerHash.update(pad);
			for (size_t i = 0; i < blockSize; i++)
				padPtr[i] = 0x5c ^ keyPtr[i];
		}
		m_outerHash.update(pad);


	}

	hmac(const hmac<hash_t>& src)
		: m_innerHash(src.m_innerHash),
		m_outerHash(src.m_outerHash)
	{ }

	hmac& operator=(const hmac<hash_t>& src)
	{
		m_innerHash = src.m_innerHash;
		m_outerHash = src.m_outerHash;
		return *this;
	}

	virtual size_t get_block_size() const { return m_innerHash.get_block_size(); }

	virtual void update(const io::buffer& block) { m_innerHash.update(block); }

	virtual io::buffer get_hash()
	{
		m_outerHash.update(m_innerHash.get_hash());
		return m_outerHash.get_hash();
	}
};


}
}


#endif
