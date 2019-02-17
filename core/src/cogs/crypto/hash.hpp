//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_HASH
#define COGS_HEADER_CRYPTO_HASH


#include "cogs/io/composite_buffer.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/env/mem/bit_rotate.hpp"


namespace cogs {
namespace crypto {


/// @ingroup Crypto
/// @brief A base class for hash algorithms
class hash
{
public:
	virtual size_t get_block_size() const { return 1; }

	virtual void update(const io::buffer& block) = 0;

	void update(const io::composite_buffer& buf)
	{
		size_t numBuffers = buf.get_inner_count();
		for (size_t bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++)
			update(buf.get_inner(bufferIndex));
	}

	virtual io::buffer get_hash() = 0;

	// Some hash algorithms can sign the end of a block, resulting in a constant known-good final value.
	// i.e. CRC, fletcher
	virtual bool can_terminate() const					{ return false; }
	virtual bool was_block_terminated() const			{ return false; }

	virtual bool is_hash_int() const					{ return false; }

	virtual io::buffer get_hash_int_as_buffer() const	{ return io::buffer(); }
};


template <class hash_t>
inline io::buffer calculate_hash(const io::buffer& message)
{
	hash_t h;
	h.update(message);
	return h.get_hash();
}


}
}

#endif
