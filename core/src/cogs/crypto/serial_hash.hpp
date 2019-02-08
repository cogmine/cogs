//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_SERIAL_HASH
#define COGS_SERIAL_HASH


#include "cogs/crypto/hash.hpp"
#include "cogs/mem/endian.hpp"
#include "cogs/math/bits_to_int.hpp"


namespace cogs {
namespace crypto {


//// Prototype example
//class serial_hash_derived_t
//{
//public:
//	void process_digit(const digit_t& x);	// optional.  Default sets next state digit
//	void process_block();
//	void terminate();
//};



template <class derived_t,
	size_t result_bits_in,
	size_t digest_bits_in,
	size_t digit_bits_in,
	endian_t digit_endian_in,
	size_t stride_bits_in,
	size_t result_contribution_bits_in = result_bits_in>
class serial_hash : public hash
{
public:
	static const size_t digest_bits = digest_bits_in;
	static const size_t result_bits = result_bits_in;
	static const size_t digit_bits = digit_bits_in;
	static const endian_t digit_endian = digit_endian_in;
	static const size_t stride_bits = stride_bits_in;
	static const size_t result_contribution_bits = result_contribution_bits_in;

	typedef bits_to_uint_t<digit_bits> digit_t;

	static const size_t digest_bytes = digest_bits / 8;
	static const size_t result_bytes = result_bits / 8;
	static const size_t digit_bytes = digit_bits / 8;
	static const size_t stride_bytes = stride_bits / 8;
	static const size_t result_contribution_bytes = result_contribution_bits / 8;

	static const size_t digest_digits = digest_bytes / digit_bytes;
	static const size_t result_digits = result_bytes / digit_bytes;
	static const size_t stride_digits = stride_bytes / digit_bytes;
	static const size_t result_contribution_digits = result_contribution_bytes / digit_bytes;

	typedef serial_hash<derived_t, result_bits, digest_bits, digit_bits, digit_endian, stride_bits, result_contribution_bits> this_t;
	
protected:
	size_t m_digitProgress;
	size_t m_blockProgress;
	digit_t	m_result[result_digits];
	digit_t m_curDigit;

public:
	virtual size_t get_block_size() const				{ return stride_bytes; }

	serial_hash()
	{
		m_digitProgress = 0;
		m_blockProgress = 0;
		m_curDigit = 0;
	}

	serial_hash(const this_t& src)
		: m_digitProgress(src.m_digitProgress),
			m_blockProgress(src.m_blockProgress),
			m_curDigit(src.m_curDigit)
	{
		for (size_t i = 0; i < result_digits; i++)
			m_result[i] = src.m_result[i];
	}

	this_t& operator=(const this_t& src)
	{
		m_digitProgress = src.m_digitProgress;
		m_blockProgress = src.m_blockProgress;
		m_curDigit = src.m_curDigit;
		for (size_t i = 0; i < result_digits; i++)
			m_result[i] = src.m_result[i];
	}

	void advance_digit()
	{
		m_digitProgress = 0;
		m_curDigit = 0;
		if (++m_blockProgress == stride_digits)
		{
			derived_t* d = static_cast<derived_t*>(this);
			d->process_block();
			m_blockProgress = 0;
		}
	}

	void add_byte(uint8_t c)
	{
		if (digit_endian == endian_t::big)
		{
			m_digitProgress += 8;
			m_curDigit |= (digit_t)c << (digit_bits - m_digitProgress);
		}
		else
		{
			m_curDigit |= (digit_t)c << m_digitProgress;
			m_digitProgress += 8;
		}

		if (m_digitProgress == digit_bits)
		{
			derived_t* d = static_cast<derived_t*>(this);
			d->process_digit();
		}
	}

	virtual void update(const io::buffer& buf)
	{
		unsigned char* ptr = (unsigned char*)buf.get_const_ptr();
		size_t bufLength = buf.get_length();
		for (size_t i = 0; i < bufLength; i++)
			add_byte(ptr[i]);
	}

	virtual io::buffer get_hash()
	{
		derived_t* d = static_cast<derived_t*>(this);
		d->terminate();

		// Copy result buffer out
		io::buffer resultBuffer(digest_bytes);
		unsigned char* bufPtr = (unsigned char*)resultBuffer.get_ptr();

		const size_t digit_bytes_mask = digit_bytes - 1;
		if (digest_bytes <= result_contribution_bytes)
		{
			for (size_t i = 0; i < digest_bytes; i++)
			{
				if (digit_endian == endian_t::big)
				{
					size_t shiftBy = (digit_bytes_mask - (i & digit_bytes_mask)) * 8;
					bufPtr[i] = (unsigned char)(m_result[i / digit_bytes] >> shiftBy);
				}
				else
				{
					size_t shiftBy = (i & digit_bytes_mask) * 8;
					bufPtr[i] = (unsigned char)(m_result[i / digit_bytes] >> shiftBy);
				}
			}
		}
		else
		{
			size_t dstIndex = 0;
			size_t endDstIndex = digest_bytes;
			bool done = false;
			for (;;)
			{
				for (size_t i = 0; i < result_contribution_bytes; i++)
				{
					if (digit_endian == endian_t::big)
					{
						size_t shiftBy = (digit_bytes_mask - (i & digit_bytes_mask)) * 8;
						bufPtr[dstIndex] = (unsigned char)(m_result[i / digit_bytes] >> shiftBy);
					}
					else
					{
						size_t shiftBy = (i & digit_bytes_mask) * 8;
						bufPtr[dstIndex] = (unsigned char)(m_result[i / digit_bytes] >> shiftBy);
					}
					if (dstIndex == digest_bytes - 1)
					{
						done = true;
						break;
					}
					dstIndex++;
				}
				if (!!done)
					break;
				derived_t* d = static_cast<derived_t*>(this);
				d->process_block();
			}
		}

		return resultBuffer;
	}
};


}
}


#endif
