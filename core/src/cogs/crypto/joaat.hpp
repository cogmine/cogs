//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_JOAAT
#define COGS_HEADER_CRYPTO_JOAAT


#include "cogs/crypto/hash_int.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief Jenkins Hash Algorithm
class joaat : public hash_int<32>
{
private:
	uint32_t m_result;

public:
	virtual uint_t get_hash_int() const
	{
		uint32_t result = m_result;
		result += (result << 3);
		result ^= (result >> 11);
		result += (result << 15);
		return result;
	}

	virtual void update(const io::buffer& block)
	{
		size_t n = block.get_length();
		if (n > 0)
		{
			const unsigned char* p = (const unsigned char*)block.get_const_ptr();
			for (size_t i = 0; i < n; i++)
			{
				m_result += p[i];
				m_result += (m_result << 10);
				m_result ^= (m_result >> 6);
			}
		}
	}

	joaat() { m_result = 0; }
	joaat(const joaat& src) { m_result = src.m_result; }
	joaat& operator=(const joaat& src) { m_result = src.m_result; return *this; }
};


}
}

#endif
