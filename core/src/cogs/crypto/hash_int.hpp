//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_HASH_INT
#define COGS_HEADER_CRYPTO_HASH_INT


#include "cogs/crypto/hash.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief A base class for hash algorithms that produce an integral result
/// @tparam bits Bit width
template <size_t bits>
class hash_int : public virtual hash
{
public:
	static constexpr size_t width_bits = bits;
	static constexpr size_t width_bytes = bits_to_bytes_v<width_bits>;
	typedef bits_to_uint_t<width_bits> uint_t;

	virtual bool is_hash_int() const { return true; }

	virtual uint_t get_hash_int() const = 0;

	virtual io::buffer get_hash_int_as_buffer() const
	{
		fixed_integer<false, width_bits> n = get_hash_int();
		return n.template to_buffer<endian_t::big>();
	}

	// Default implementation assumes most significant bit is output first (big endian)
	virtual io::buffer get_hash()
	{
		return get_hash_int_as_buffer();
	}
};


}
}


#endif
