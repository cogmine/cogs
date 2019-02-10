//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_FNV
#define COGS_FNV


#include "cogs/crypto/hash_int.hpp"
#include "cogs/io/buffer.hpp"
#include "cogs/io/composite_buffer.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief Fowler-Noll-Vo Hash Algorithm FNV-0
/// @tparam bits Bit width
template <size_t bits>
class fnv0 : public hash_int<bits>
{
private:
	fnv0() = delete;	// Not allowed.  This template is specialized for supported bit sizes
};

/// @ingroup Crypto
/// @brief Fowler-Noll-Vo Hash Algorithm, FNV-1
/// @tparam bits Bit width
template <size_t bits>
class fnv1 : public hash
{
private:
	fnv1() = delete;	// Not allowed.  This template is specialized for supported bit sizes
};

/// @ingroup Crypto
/// @brief Fowler-Noll-Vo Hash Algorithm, FNV-1a
/// @tparam bits Bit width
template <size_t bits>
class fnv1a : public hash
{
private:
	fnv1a() = delete;	// Not allowed.  This template is specialized for supported bit sizes
};


template <class derived_t, size_t bits, bool a>
class fnv_int_base : public hash_int<bits>
{
protected:
	typedef fnv_int_base<derived_t, bits, a> this_t;

	typedef typename hash_int<bits>::uint_t uint_t;
	uint_t m_result;

public:
	virtual void update(const io::buffer& block)
	{
		uint_t prime = derived_t::get_prime();
		size_t n = block.get_length();
		const unsigned char* p = (const unsigned char*)block.get_const_ptr();
		for (size_t i = 0; i < n; i++)
		{
			if (a)
			{
				m_result ^= p[i];
				m_result *= prime;
			}
			else
			{
				m_result *= prime;
				m_result ^= p[i];
			}
		}
	}

	virtual uint_t get_hash_int()			{ return m_result; }

	fnv_int_base()							{ }
	fnv_int_base(const this_t& src)			{ m_result = src.m_result; }
	this_t& operator=(const this_t& src)	{ m_result = src.m_result; return *this; }
};

template <class derived_t, size_t bits, bool a>
class fnv_bigint_base : public hash
{
protected:
	typedef fnv_bigint_base<derived_t, bits, a> this_t;

	typedef fixed_integer<false, bits> uint_t;
	uint_t m_result;

public:
	virtual void update(const io::buffer& block)
	{
		size_t n = block.get_length();
		if (n > 0)
		{
			uint_t prime = derived_t::get_prime();
			const unsigned char* p = (const unsigned char*)block.get_const_ptr();
			for (size_t i = 0; i < n; i++)
			{
				if (a)
				{
					m_result ^= p[i];
					m_result *= prime;
				}
				else
				{
					m_result *= prime;
					m_result ^= p[i];
				}
			}
		}
	}

	virtual io::buffer get_hash()
	{
		return m_result.template to_buffer<endian_t::big>();
	}

	fnv_bigint_base()						{ }
	fnv_bigint_base(const this_t& src)		{ m_result = src.m_result; }
	this_t& operator=(const this_t& src)	{ m_result = src.m_result; return *this; }
};


template <>
class fnv0<32> : public fnv_int_base<fnv0<32>, 32, false>
{
private:
	typedef fnv_int_base<fnv0<32>, 32, false> base_t;

public:
	static uint32_t get_prime() { return 0x01000193; }

	fnv0()										{ m_result = 0; }
	fnv0(const fnv0<32>& src) : base_t(src)		{ }
	fnv0<32>& operator=(const fnv0<32>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv0<64> : public fnv_int_base<fnv0<64>, 64, false>
{
private:
	typedef fnv_int_base<fnv0<64>, 64, false> base_t;

public:
	static uint64_t get_prime() { return 0x00000100000001b3ULL; }

	fnv0()										{ m_result = 0; }
	fnv0(const fnv0<64>& src) : base_t(src)		{ }
	fnv0<64>& operator=(const fnv0<64>& src)	{ base_t::operator=(src); return *this; }
};


template <>
class fnv1<32> : public fnv_int_base<fnv1<32>, 32, false>
{
private:
	typedef fnv_int_base<fnv1<32>, 32, false> base_t;

public:
	static uint32_t get_prime() { return 0x01000193; }

	fnv1()										{ m_result = 0x811c9dc5; }
	fnv1(const fnv1<32>& src) : base_t(src)		{ }
	fnv1<32>& operator=(const fnv1<32>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1<64> : public fnv_int_base<fnv1<64>, 64, false>
{
private:
	typedef fnv_int_base<fnv1<64>, 64, false> base_t;

public:
	static uint64_t get_prime() { return 0x00000100000001b3ULL; }

	fnv1()										{ m_result = 0xcbf29ce484222325ULL; }
	fnv1(const fnv1<64>& src) : base_t(src)		{ }
	fnv1<64>& operator=(const fnv1<64>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1<128> : public fnv_bigint_base<fnv1<128>, 128, false>
{
private:
	typedef fnv_bigint_base<fnv1<128>, 128, false> base_t;

public:
	static fixed_integer<false, 128> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 128>::int_t) * 8);
		static constexpr size_t shiftBy = 88 % digit_bits;
		static constexpr size_t shiftIndex = 88 / digit_bits;

		fixed_integer<false, 128> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 128>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x3b));
		return result;
	}

	fnv1()
	{
		m_result = 0x6C62272E07BB0142ULL;
		m_result <<= 64;
		m_result |= 0x62B821756295C58DULL;
	}

	fnv1(const fnv1<128>& src) : base_t(src)	{ }
	fnv1<128>& operator=(const fnv1<128>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1<256> : public fnv_bigint_base<fnv1<256>, 256, false>
{
private:
	typedef fnv_bigint_base<fnv1<256>, 256, false> base_t;

public:
	static fixed_integer<false, 256> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 256>::int_t) * 8);
		static constexpr size_t shiftBy = 168 % digit_bits;
		static constexpr size_t shiftIndex = 168 / digit_bits;

		fixed_integer<false, 256> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 256>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x63));
		return result;
	}

	fnv1()
	{
		m_result = 0xDD268DBCAAC55036ULL;
		m_result <<= 64;
		m_result |= 0x2D98C384C4E576CCULL;
		m_result <<= 64;
		m_result |= 0xC8B1536847B6BBB3ULL;
		m_result <<= 64;
		m_result |= 0x1023B4C8CAEE0535ULL;
	}

	fnv1(const fnv1<256>& src) : base_t(src)	{ }
	fnv1<256>& operator=(const fnv1<256>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1<512> : public fnv_bigint_base<fnv1<512>, 512, false>
{
private:
	typedef fnv_bigint_base<fnv1<512>, 512, false> base_t;

public:
	static fixed_integer<false, 512> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 512>::int_t) * 8);
		static constexpr size_t shiftBy = 344 % digit_bits;
		static constexpr size_t shiftIndex = 344 / digit_bits;

		fixed_integer<false, 512> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 512>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x57));
		return result;
	}

	fnv1()
	{
		m_result = 0xB86DB0B1171F4416ULL;
		m_result <<= 64;
		m_result |= 0xDCA1E50F309990ACULL;
		m_result <<= 64;
		m_result |= 0xAC87D059C9000000ULL;
		m_result <<= 64;
		m_result |= 0x0000000000000D21ULL;
		m_result <<= 64;
		m_result |= 0xE948F68A34C192F6ULL;
		m_result <<= 64;
		m_result |= 0x2EA79BC942DBE7CEULL;
		m_result <<= 64;
		m_result |= 0x182036415F56E34BULL;
		m_result <<= 64;
		m_result |= 0xAC982AAC4AFE9FD9ULL;
	}

	fnv1(const fnv1<512>& src) : base_t(src)	{ }
	fnv1<512>& operator=(const fnv1<512>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1<1024> : public fnv_bigint_base<fnv1<1024>, 1024, false>
{
private:
	typedef fnv_bigint_base<fnv1<1024>, 1024, false> base_t;

public:
	static fixed_integer<false, 1024> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 1024>::int_t) * 8);
		static constexpr size_t shiftBy = 680 % digit_bits;
		static constexpr size_t shiftIndex = 680 / digit_bits;

		fixed_integer<false, 1024> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 1024>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x8D));
		io::buffer tmpBuf = result.template to_buffer<endian_t::big>();
		return result;
	}

	fnv1()
	{
		m_result = 0x005F7A76758ECC4DULL;
		m_result <<= 64;
		m_result |= 0x32E56D5A591028B7ULL;
		m_result <<= 64;
		m_result |= 0x4B29FC4223FDADA1ULL;
		m_result <<= 64;
		m_result |= 0x6C3BF34EDA3674DAULL;
		m_result <<= 64;
		m_result |= 0x9A21D90000000000ULL;
		m_result <<= 384;
		m_result |= 0x000000000004C6D7ULL;
		m_result <<= 64;
		m_result |= 0xEB6E73802734510AULL;
		m_result <<= 64;
		m_result |= 0x555F256CC005AE55ULL;
		m_result <<= 64;
		m_result |= 0x6BDE8CC9C6A93B21ULL;
		m_result <<= 64;
		m_result |= 0xAFF4B16C71EE90B3ULL;
	}

	fnv1(const fnv1<1024>& src) : base_t(src)		{ }
	fnv1<1024>& operator=(const fnv1<1024>& src)	{ base_t::operator=(src); return *this; }
};


template <>
class fnv1a<32> : public fnv_int_base<fnv1a<32>, 32, true>
{
private:
	typedef fnv_int_base<fnv1a<32>, 32, true> base_t;

public:
	static uint32_t get_prime() { return 0x01000193; }

	fnv1a()										{ m_result = 0x811c9dc5; }
	fnv1a(const fnv1a<32>& src) : base_t(src)	{ }
	fnv1a<32>& operator=(const fnv1a<32>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1a<64> : public fnv_int_base<fnv1a<64>, 64, true>
{
private:
	typedef fnv_int_base<fnv1a<64>, 64, true> base_t;

public:
	static uint64_t get_prime() { return 0x00000100000001b3ULL; }

	fnv1a()										{ m_result = 0xcbf29ce484222325ULL; }
	fnv1a(const fnv1a<64>& src) : base_t(src)	{ }
	fnv1a<64>& operator=(const fnv1a<64>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1a<128> : public fnv_bigint_base<fnv1a<128>, 128, true>
{
private:
	typedef fnv_bigint_base<fnv1a<128>, 128, true> base_t;

public:
	static fixed_integer<false, 128> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 128>::int_t) * 8);
		static constexpr size_t shiftBy = 88 % digit_bits;
		static constexpr size_t shiftIndex = 88 / digit_bits;

		fixed_integer<false, 128> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 128>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x3b));
		return result;
	}

	fnv1a()
	{
		m_result = 0x6C62272E07BB0142ULL;
		m_result <<= 64;
		m_result |= 0x62B821756295C58DULL;
	}

	fnv1a(const fnv1a<128>& src) : base_t(src)		{ }
	fnv1a<128>& operator=(const fnv1a<128>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1a<256> : public fnv_bigint_base<fnv1a<256>, 256, true>
{
private:
	typedef fnv_bigint_base<fnv1a<256>, 256, true> base_t;

public:
	static fixed_integer<false, 256> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 256>::int_t) * 8);
		static constexpr size_t shiftBy = 168 % digit_bits;
		static constexpr size_t shiftIndex = 168 / digit_bits;

		fixed_integer<false, 256> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 256>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x63));
		return result;
	}

	fnv1a()
	{
		m_result = 0xDD268DBCAAC55036ULL;
		m_result <<= 64;
		m_result |= 0x2D98C384C4E576CCULL;
		m_result <<= 64;
		m_result |= 0xC8B1536847B6BBB3ULL;
		m_result <<= 64;
		m_result |= 0x1023B4C8CAEE0535ULL;
	}

	fnv1a(const fnv1a<256>& src) : base_t(src)		{ }
	fnv1a<256>& operator=(const fnv1a<256>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1a<512> : public fnv_bigint_base<fnv1a<512>, 512, true>
{
private:
	typedef fnv_bigint_base<fnv1a<512>, 512, true> base_t;

public:
	static fixed_integer<false, 512> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 512>::int_t) * 8);
		static constexpr size_t shiftBy = 344 % digit_bits;
		static constexpr size_t shiftIndex = 344 / digit_bits;

		fixed_integer<false, 512> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 512>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x57));
		return result;
	}

	fnv1a()
	{
		m_result = 0xB86DB0B1171F4416ULL;
		m_result <<= 64;
		m_result |= 0xDCA1E50F309990ACULL;
		m_result <<= 64;
		m_result |= 0xAC87D059C9000000ULL;
		m_result <<= 64;
		m_result |= 0x0000000000000D21ULL;
		m_result <<= 64;
		m_result |= 0xE948F68A34C192F6ULL;
		m_result <<= 64;
		m_result |= 0x2EA79BC942DBE7CEULL;
		m_result <<= 64;
		m_result |= 0x182036415F56E34BULL;
		m_result <<= 64;
		m_result |= 0xAC982AAC4AFE9FD9ULL;
	}

	fnv1a(const fnv1a<512>& src) : base_t(src)		{ }
	fnv1a<512>& operator=(const fnv1a<512>& src)	{ base_t::operator=(src); return *this; }
};

template <>
class fnv1a<1024> : public fnv_bigint_base<fnv1a<1024>, 1024, true>
{
private:
	typedef fnv_bigint_base<fnv1a<1024>, 1024, true> base_t;

public:
	static fixed_integer<false, 1024> get_prime()
	{
		static constexpr size_t digit_bits = (sizeof(fixed_integer<false, 1024>::int_t) * 8);
		static constexpr size_t shiftBy = 680 % digit_bits;
		static constexpr size_t shiftIndex = 680 / digit_bits;

		fixed_integer<false, 1024> result(0);
		result.set_digit(shiftIndex, (fixed_integer<false, 1024>::int_t)1 << shiftBy);
		result.set_digit(0, result.get_digit(0) | ((1 << 8) + 0x8D));
		io::buffer tmpBuf = result.template to_buffer<endian_t::big>();
		return result;
	}

	fnv1a()
	{
		m_result = 0x005F7A76758ECC4DULL;
		m_result <<= 64;
		m_result |= 0x32E56D5A591028B7ULL;
		m_result <<= 64;
		m_result |= 0x4B29FC4223FDADA1ULL;
		m_result <<= 64;
		m_result |= 0x6C3BF34EDA3674DAULL;
		m_result <<= 64;
		m_result |= 0x9A21D90000000000ULL;
		m_result <<= 384;
		m_result |= 0x000000000004C6D7ULL;
		m_result <<= 64;
		m_result |= 0xEB6E73802734510AULL;
		m_result <<= 64;
		m_result |= 0x555F256CC005AE55ULL;
		m_result <<= 64;
		m_result |= 0x6BDE8CC9C6A93B21ULL;
		m_result <<= 64;
		m_result |= 0xAFF4B16C71EE90B3ULL;
	}

	fnv1a(const fnv1a<1024>& src) : base_t(src)		{ }
	fnv1a<1024>& operator=(const fnv1a<1024>& src)	{ base_t::operator=(src); return *this; }
};


}
}

#endif
