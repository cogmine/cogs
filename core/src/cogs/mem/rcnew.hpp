//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCNEW
#define COGS_HEADER_MEM_RCNEW


#include "cogs/env.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rc_container_base.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/operators.hpp"


namespace cogs {

// TODO: Move helper function to better place
template<typename... args_t>
constexpr size_t count_args(args_t&&...) { return sizeof...(args_t); }


template <typename base_type>
class bypass_constructor_permission : public base_type
{
public:
	template <typename... args_t>
	bypass_constructor_permission(args_t&&... args)
		: base_type(std::forward<args_t>(args)...)
	{
	}
};


template <typename T, typename... args_t>
std::enable_if_t<std::is_base_of_v<object, T>, void>
placement_rcnew(T* obj, const ptr<rc_obj_base>& desc, args_t&&... args)
{
	new (obj) T(desc, std::forward<args_t>(args)...);
}

template <typename T, typename... args_t>
std::enable_if_t<!std::is_base_of_v<object, T>, void>
placement_rcnew(T* obj, const ptr<rc_obj_base>& desc, args_t&&... args)
{
	new (obj) T(std::forward<args_t>(args)...);
}


template <typename T>
std::enable_if_t<std::is_base_of_v<object, T>, void>
placement_rcnew(T* obj, const ptr<rc_obj_base>& desc)
{
	new (obj) T(desc);
}

template <typename T>
std::enable_if_t<!std::is_base_of_v<object, T>, void>
placement_rcnew(T* obj, const ptr<rc_obj_base>& desc)
{
	new (obj) T;
}


template <typename type, typename allocator_t, typename... args_t>
std::enable_if_t<allocator_t::is_static, rcref<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t* al,
	type* p,
	args_t&&... args)
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

	placement_rcnew(obj, desc, std::forward<args_t>(args)...);
	rcref<type> r(obj, desc);
	return r;
}


template <typename type, typename allocator_t, typename... args_t>
std::enable_if_t<!allocator_t::is_static, rcref<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t* al,
	type* p,
	args_t&&... args)
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

	placement_rcnew(obj, desc, std::forward<args_t>(args)...);
	rcref<type> r(obj, desc);
	return r;
}

template <typename type, typename allocator_t>
std::enable_if_t<allocator_t::is_static, rcref<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t* al,
	type* p)
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

	placement_rcnew(obj, desc);
	rcref<type> r(obj, desc);
	return r;
}


template <typename type, typename allocator_t>
std::enable_if_t<!allocator_t::is_static, rcref<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t* al,
	type* p)
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

	placement_rcnew(obj, desc);
	rcref<type> r(obj, desc);
	return r;
}

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING

#define rcnew(type, ...) rcnew_inner(COGS_DEBUG_AT, (::cogs::default_allocator*)0, (type*)0, ## __VA_ARGS__)
#define static_rcnew(al, type, ...) rcnew_inner(COGS_DEBUG_AT, (al*)0, (type*)0, ## __VA_ARGS__)
#define instance_rcnew(al, type, ...) rcnew_inner(COGS_DEBUG_AT, &(al), (type*)0, ## __VA_ARGS__)
#define container_rcnew(al, type, ...) rcnew_inner(COGS_DEBUG_AT, (al).get_allocator().get_ptr(), (type*)0, ## __VA_ARGS__)

#else

#define rcnew(type, ...) rcnew_inner((::cogs::default_allocator*)0, (type*)0, ## __VA_ARGS__)
#define static_rcnew(al, type, ...) rcnew_inner((al*)0, (type*)0, ## __VA_ARGS__)
#define instance_rcnew(al, type, ...) rcnew_inner(&(al), (type*)0, ## __VA_ARGS__)
#define container_rcnew(al, type, ...) rcnew_inner((al).get_allocator().get_ptr(), (type*)0, ## __VA_ARGS__)

#endif






}


#endif
