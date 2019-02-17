//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_UNOWNED
#define COGS_HEADER_MEM_UNOWNED


namespace cogs {


template <class base_type>
class unowned_t : public base_type
{
public:
	unowned_t()		{ }
	~unowned_t()	{ base_type::disown(); }

	unowned_t<base_type>& get_unowned()	{ return *this; }
};


}


#endif

