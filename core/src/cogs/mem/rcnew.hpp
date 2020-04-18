//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCNEW
#define COGS_HEADER_MEM_RCNEW


#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/operators.hpp"


namespace cogs {


template <typename type, typename allocator_t>
std::enable_if_t<allocator_t::is_static, type*>
rcnew_glue(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	type*,
	volatile allocator_t*,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
	typedef rc_obj<type, allocator_t> rc_obj_t;
	rc_obj_t* desc = rc_obj_t::allocate().get_ptr();
	type* obj = desc->get_obj();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	desc->set_type_name(typeid(type).name());
	desc->set_debug_str(debugStr);
	desc->set_obj_ptr(obj);
#endif

#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_rcLogCount);
	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
#endif

	temp.m_obj = obj;
	temp.m_desc = desc;
	temp.m_saved = object::rcnew_glue_obj;
	object::rcnew_glue_obj = &temp;
	return obj;
}

template <typename type, typename allocator_t>
std::enable_if_t<!allocator_t::is_static, type*>
rcnew_glue(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	type*,
	volatile allocator_t* al,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
	typedef rc_obj<type, allocator_t> rc_obj_t;
	rc_obj_t* desc = rc_obj_t::allocate(*al).get_ptr();
	type* obj = desc->get_obj();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	desc->set_type_name(typeid(type).name());
	desc->set_debug_str(debugStr);
	desc->set_obj_ptr(obj);
#endif

#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_rcLogCount);
	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
#endif

	temp.m_obj = obj;
	temp.m_desc = desc;
	temp.m_saved = object::rcnew_glue_obj;
	object::rcnew_glue_obj = &temp;
	return obj;
}


template <typename type>
type* rcnew_glue(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char*
#if COGS_DEBUG_RC_LOGGING
	debugStr
#endif
	,
#endif
	const rc_container_content_t<type>& content,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_rcLogCount);
	printf("(%lu) RC_NEW (placement): %p (desc) %p (ptr) %s @ %s\n", rcCount, &desc, obj, typeid(type).name(), debugStr);
#endif

	temp.m_obj = content.m_obj;
	temp.m_desc = content.m_desc;
	temp.m_saved = object::rcnew_glue_obj;
	object::rcnew_glue_obj = &temp;
	return content.m_obj;
}

template <typename type>
struct rcnew_glue_t
{
	rcnew_glue_t(type*) { }

	operator rcref<type>()
	{
		rcref<type> result((type*)object::rcnew_glue_obj->m_obj, object::rcnew_glue_obj->m_desc);
		object::rcnew_glue_obj = object::rcnew_glue_obj->m_saved;
		return result;
	}

	void operator!()
	{
		object::rcnew_glue_obj = object::rcnew_glue_obj->m_saved;
	}
};


#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING


#define rcnew(type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue(COGS_DEBUG_AT, (type*)0, (::cogs::default_allocator*)0)) type
#define static_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue(COGS_DEBUG_AT, (type*)0, (al*)0)) type
#define instance_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue(COGS_DEBUG_AT, (type*)0, &(al))) type
#define container_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue(COGS_DEBUG_AT, (type*)0, (al).get_allocator().get_ptr())) type
#define placement_rcnew(objPtr, descRef) (void)!(::cogs::rcnew_glue_t<std::remove_pointer_t<decltype(objPtr)> >)new (::cogs::rcnew_glue(COGS_DEBUG_AT, rc_container_content_t<std::remove_pointer_t<decltype(objPtr)> >({ (objPtr), &(descRef) }))) std::remove_pointer_t<decltype(objPtr)>


#else


#define rcnew(type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue((type*)0, (::cogs::default_allocator*)0)) type
#define static_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue((type*)0, (al*)0)) type
#define instance_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue((type*)0, &(al))) type
#define container_rcnew(al, type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue((type*)0, (al).get_allocator().get_ptr())) type
#define placement_rcnew(objPtr, descRef) (void)!(::cogs::rcnew_glue_t<std::remove_pointer_t<decltype(objPtr)> >)new (::cogs::rcnew_glue(rc_container_content_t<std::remove_pointer_t<decltype(objPtr)> >({ (objPtr), &(descRef) }))) std::remove_pointer_t<decltype(objPtr)>


#endif


}


#include "cogs/collections/container_dlist.hpp"


#endif
