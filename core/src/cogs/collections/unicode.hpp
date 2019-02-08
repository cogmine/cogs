//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress, Placeholder


#ifndef COGS_UNICODE
#define COGS_UNICODE


#include "cogs/env.hpp"
#include "cogs/math/int_types.hpp"


namespace cogs {


// Unicode v8.0
class unicode
{
public:
	struct block_properties_t
	{
		uint32_t m_start;
		uint32_t m_end;
		char* m_name;
	};

	enum normalization_form
	{
		NFD,	// Normalization Form D (NFD) Canonical Decomposition 
		NFC,	// Normalization Form C (NFC) Canonical Decomposition, followed by Canonical Composition
		NFKD,	// Normalization Form KD (NFKD) Compatibility Decomposition 
		NFKC	// Normalization Form KC (NFKC) Compatibility Decomposition, followed by Canonical Composition
	};

	static const block_properties_t* block_properties;
	static const size_t block_count;
};


}

#endif
