//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RC_OBJ
#define COGS_HEADER_MEM_RC_OBJ


#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"


namespace cogs {

/// @brief A reference-counted object container.
///
/// rc_obj instances contain reference counts.  rc_obj is used internally by rcref, rcptr, weak_rcptr.
/// @tparam T Type to contain
template <typename T>
class rc_obj : public rc_obj_base
{
public:
	/// @brief Alias to encapsulated type
	typedef T type;

private:
	typedef rc_obj<T> this_t;

	placement<type> m_contents;

#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif

	virtual void released()
	{
		type* p = get_object();
		placement_destruct(p);
	}

	virtual bool contains(void* obj) const
	{
		const type* start = &m_contents.get();
		const unsigned char* p = (const unsigned char*)obj;
		return (p >= (const unsigned char*)start) && (p < (const unsigned char*)(start + 1));
	}

	virtual void dispose()
	{
		default_memory_manager::destruct_deallocate_type(this);
	}

	rc_obj(this_t&&) = delete;
	rc_obj(const this_t&) = delete;
	this_t& operator=(this_t&&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	rc_obj()
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_object();
#endif
	}

	virtual ~rc_obj() { }

	type* get_object() { return &m_contents.get(); }
};

}


#include "cogs/mem/object.hpp"


#endif
