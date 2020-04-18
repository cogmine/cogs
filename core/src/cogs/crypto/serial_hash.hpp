//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_SERIAL_HASH
#define COGS_HEADER_CRYPTO_SERIAL_HASH


#include "cogs/crypto/hash.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/endian.hpp"
#include "cogs/math/bits_to_int.hpp"


namespace cogs {
namespace crypto {


//// Prototype example
//class serial_hash_derived_t
//{
//public:
//	void process_digit(const digit_t& x); // optional.  Default sets next state digit
//	void process_block();
//	void terminate();
//};


template <size_t result_bits_in, size_t digest_bits_in, size_t digit_bits_in, endian_t digit_endian_in, size_t stride_bits_in, size_t result_contribution_bits_in = result_bits_in>
class serial_hash : public hash
{
public:
	static constexpr size_t digest_bits = digest_bits_in;
	static constexpr size_t result_bits = result_bits_in;
	static constexpr size_t digit_bits = digit_bits_in;
	static constexpr endian_t digit_endian = digit_endian_in;
	static constexpr size_t stride_bits = stride_bits_in;
	static constexpr size_t result_contribution_bits = result_contribution_bits_in;

	typedef bits_to_uint_t<digit_bits> digit_t;

	static constexpr size_t digest_bytes = digest_bits / 8;
	static constexpr size_t result_bytes = result_bits / 8;
	static constexpr size_t digit_bytes = digit_bits / 8;
	static constexpr size_t stride_bytes = stride_bits / 8;
	static constexpr size_t result_contribution_bytes = result_contribution_bits / 8;

	static constexpr size_t digest_digits = digest_bytes / digit_bytes;
	static constexpr size_t result_digits = result_bytes / digit_bytes;
	static constexpr size_t stride_digits = stride_bytes / digit_bytes;
	static constexpr size_t result_contribution_digits = result_contribution_bytes / digit_bytes;

	typedef serial_hash<result_bits, digest_bits, digit_bits, digit_endian, stride_bits, result_contribution_bits> this_t;

protected:
	size_t m_digitProgress = 0;
	size_t m_blockProgress = 0;
	digit_t m_result[result_digits];
	digit_t m_curDigit = 0;

	function<void()> m_processDigitFunc;
	function<void()> m_processBlockFunc;
	function<void()> m_terminateFunc;

	template <typename F1, typename F2, typename F3>
	serial_hash(const this_t& src, F1&& processDigitFunc, F2&& processBlockFunc, F3&& terminateFunc)
		: m_digitProgress(src.m_digitProgress),
		m_blockProgress(src.m_blockProgress),
		m_curDigit(src.m_curDigit),
		m_processDigitFunc(std::forward<F1>(processDigitFunc)),
		m_processBlockFunc(std::forward<F2>(processBlockFunc)),
		m_terminateFunc(std::forward<F3>(terminateFunc))
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
		return *this;
	}

public:
	virtual size_t get_block_size() const { return stride_bytes; }

	template <typename F1, typename F2, typename F3>
	serial_hash(F1&& processDigitFunc, F2&& processBlockFunc, F3&& terminateFunc)
		: m_processDigitFunc(std::forward<F1>(processDigitFunc)),
		m_processBlockFunc(std::forward<F2>(processBlockFunc)),
		m_terminateFunc(std::forward<F3>(terminateFunc))
	{
	}

	void advance_digit()
	{
		m_digitProgress = 0;
		m_curDigit = 0;
		if (++m_blockProgress == stride_digits)
		{
			m_processBlockFunc();
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
			m_processDigitFunc();
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
		m_terminateFunc();

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
				m_processBlockFunc();
			}
		}

		return resultBuffer;
	}
};


}
}


#endif
