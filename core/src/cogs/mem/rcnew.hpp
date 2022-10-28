//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCNEW
#define COGS_HEADER_MEM_RCNEW


#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/operators.hpp"


namespace cogs {


template <class T, class = void> struct has_allocate : public std::false_type {};
template <class T> struct has_allocate<T, std::void_t<decltype(std::declval<T>().allocate())>> : public std::true_type {};
template <class T> static constexpr bool has_allocate_v = has_allocate<T>::value;


template <typename type, typename allocator_t>
type* rcnew_glue(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	std::enable_if_t<has_allocate_v<allocator_t>, allocator_t&&> al,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
	rc_obj<type>* desc = al.allocate();
	new (desc) rc_obj<type>;
	type* obj = desc->get_object();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	desc->set_type_name(typeid(type).name());
	desc->set_debug_str(debugStr);
	desc->set_obj_ptr(obj);
#endif

#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_allocLogCount);
	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
#endif

	temp.m_obj = obj;
	temp.m_desc = desc;
	temp.m_saved = object::rcnew_glue_obj;
	object::rcnew_glue_obj = &temp;
	return obj;
}

template <typename type, typename memory_manager_t>
type* rcnew_glue(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	std::enable_if_t<!has_allocate_v<memory_manager_t>, memory_manager_t&&> mm,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
	rc_obj<type>* desc = mm.template allocate_type<rc_obj<type> >();
	new (desc) rc_obj<type>;
	type* obj = desc->get_object();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	desc->set_type_name(typeid(type).name());
	desc->set_debug_str(debugStr);
	desc->set_obj_ptr(obj);
#endif

#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_allocLogCount);
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
	const char* debugStr,
#endif
	rc_obj<type>* desc,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
	type* obj = desc->get_object();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	desc->set_type_name(typeid(type).name());
	desc->set_debug_str(debugStr);
	desc->set_obj_ptr(obj);
#endif

#if COGS_DEBUG_RC_LOGGING
	unsigned long rcCount = pre_assign_next(g_allocLogCount);
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
	const char* debugStr,
#endif
	const rc_container_content_t<type>& content,
	const rcnew_glue_obj_t& temp = rcnew_glue_obj_t())
{
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


#define rcnew(type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue<type, decltype(::cogs::default_memory_manager())>(COGS_DEBUG_AT, ::cogs::default_memory_manager())) type
#define placement_rcnew(rcObjPtr) (::cogs::rcref<typename std::remove_pointer_t<decltype(rcObjPtr)>::type>)(::cogs::rcnew_glue_t<typename std::remove_pointer_t<decltype(rcObjPtr)>::type>)new (::cogs::rcnew_glue(COGS_DEBUG_AT, (rcObjPtr))) typename std::remove_pointer_t<decltype(rcObjPtr)>::type
#define nested_rcnew(objPtr, descRef) (void)!(::cogs::rcnew_glue_t<std::remove_pointer_t<decltype(objPtr)> >)new (::cogs::rcnew_glue(COGS_DEBUG_AT, rc_container_content_t<std::remove_pointer_t<decltype(objPtr)> >({ (objPtr), &(descRef) }))) std::remove_pointer_t<decltype(objPtr)>


#else


#define rcnew(type) (::cogs::rcref<type>)(::cogs::rcnew_glue_t<type>)new (::cogs::rcnew_glue<type, decltype(::cogs::default_memory_manager())>(::cogs::default_memory_manager())) type
#define placement_rcnew(rcObjPtr) (::cogs::rcref<typename std::remove_pointer_t<decltype(rcObjPtr)>::type>)(::cogs::rcnew_glue_t<typename std::remove_pointer_t<decltype(rcObjPtr)>::type>)new (::cogs::rcnew_glue((rcObjPtr))) typename std::remove_pointer_t<decltype(rcObjPtr)>::type
#define nested_rcnew(objPtr, descRef) (void)!(::cogs::rcnew_glue_t<std::remove_pointer_t<decltype(objPtr)> >)new (::cogs::rcnew_glue(rc_container_content_t<std::remove_pointer_t<decltype(objPtr)> >({ (objPtr), &(descRef) }))) std::remove_pointer_t<decltype(objPtr)>


#endif


}


#include "cogs/collections/container_dlist.hpp"


#endif
