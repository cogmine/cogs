//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCNEW
#define COGS_HEADER_MEM_RCNEW


#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rc_container.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/operators.hpp"


namespace cogs {


template <typename type>
class rcnew_helper
{
public:
	type* m_obj;
	rc_obj_base* m_desc;

	rcnew_helper(type* obj, rc_obj_base* desc)
		: m_obj(obj),
		m_desc(desc)
	{ }

	template <typename F, typename... args_t>
	void placement_construct(F&& f, args_t&&... args)
	{
		f(m_obj, m_desc, std::forward<args_t>(args)...);
	}

	template <typename F, typename... args_t>
	rcref<type> get_rcref(F&& f, args_t&&... args)
	{
		f(m_obj, m_desc, std::forward<args_t>(args)...);
		return rcref<type>(m_obj, m_desc);
	}
};


template <typename type, typename F>
class rcnew_helper2
{
public:
	type* m_obj;
	rc_obj_base* m_desc;
	F m_constructor;

	rcnew_helper2(type* obj, rc_obj_base* desc, F&& f)
		: m_obj(obj),
		m_desc(desc),
		m_constructor(std::move(f))
	{ }

	//template <typename... args_t>
	//rcref<type> operator()(args_t&&... args)
	//{
	//	m_constructor(std::forward<args_t>(args)...);
	//	return rcref<type>(m_obj, m_desc);
	//}

//	template <typename F, typename... args_t>
//	void placement_construct(F&& f, args_t&&... args)
//	{
//		f(m_obj, m_desc, std::forward<args_t>(args)...);
//}
//
//	template <typename F, typename... args_t>
//	rcref<type> get_rcref(F&& f, args_t&&... args)
//	{
//		f(m_obj, m_desc, std::forward<args_t>(args)...);
//		return rcref<type>(m_obj, m_desc);
//	}
};


//inline static thread_local void* s_rcnewObj;
//inline static thread_local rc_obj_base* s_rcnewDesc;
//
//template <typename type, typename allocator_t>
//std::enable_if_t<allocator_t::is_static, rcref<type> >
//get_rcnew_helper2(
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	const char* debugStr,
//#endif
//	volatile allocator_t*)
//{
//	typedef rc_obj<type, allocator_t> rc_obj_t;
//	rc_obj_t* desc = rc_obj_t::allocate().get_ptr();
//	type* obj = desc->get_obj();
//
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	desc->set_type_name(typeid(type).name());
//	desc->set_debug_str(debugStr);
//	desc->set_obj_ptr(obj);
//#endif
//
//#if COGS_DEBUG_RC_LOGGING
//	unsigned long rcCount = pre_assign_next(g_rcLogCount);
//	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
//#endif
//
//	//s_rcnewObj = obj;
//	//s_rcnewDesc = desc;
//	//return rcnew_helper2<type, F>(obj, desc);
//	return rcref<type>(obj, desc);
//}
//
//
//template <typename type, typename allocator_t>
//std::enable_if_t<!allocator_t::is_static, rcref<type> >
//get_rcnew_helper2(
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	const char* debugStr,
//#endif
//	volatile allocator_t* al)
//{
//	typedef rc_obj<type, allocator_t> rc_obj_t;
//	rc_obj_t* desc = rc_obj_t::allocate(*al).get_ptr();
//	type* obj = desc->get_obj();
//
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	desc->set_type_name(typeid(type).name());
//	desc->set_debug_str(debugStr);
//	desc->set_obj_ptr(obj);
//#endif
//
//#if COGS_DEBUG_RC_LOGGING
//	unsigned long rcCount = pre_assign_next(g_rcLogCount);
//	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
//#endif
//
//	//s_rcnewObj = obj;
//	//s_rcnewDesc = desc;
//	//return rcnew_helper2<type>(obj, desc);
//	return rcref<type>(obj, desc);
//}



struct rcnew_glue_element_t
{
	mutable void* m_obj;
	mutable rc_obj_base* m_desc;
	mutable rcnew_glue_element_t* m_saved;
};

inline static thread_local rcnew_glue_element_t* rcnew_glue_element;

template <typename type, typename allocator_t>
std::enable_if_t<allocator_t::is_static, void>
get_rcnew_helper3(
	type*,
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t*,
	const rcnew_glue_element_t& temp = rcnew_glue_element_t())
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
	temp.m_saved = rcnew_glue_element;
	rcnew_glue_element = &temp;
	//return rcnew_helper2<type, F>(obj, desc);
	//return rcref<type>(obj, desc);
}


//template <typename type, typename allocator_t>
//std::enable_if_t<!allocator_t::is_static, void>
//get_rcnew_helper3(
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	const char* debugStr,
//#endif
//	volatile allocator_t* al)
//{
//	typedef rc_obj<type, allocator_t> rc_obj_t;
//	rc_obj_t* desc = rc_obj_t::allocate(*al).get_ptr();
//	type* obj = desc->get_obj();
//
//#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
//	desc->set_type_name(typeid(type).name());
//	desc->set_debug_str(debugStr);
//	desc->set_obj_ptr(obj);
//#endif
//
//#if COGS_DEBUG_RC_LOGGING
//	unsigned long rcCount = pre_assign_next(g_rcLogCount);
//	printf("(%lu) RC_NEW: %p (desc) %p (ptr) %s @ %s\n", rcCount, (rc_obj_base*)desc, obj, typeid(type).name(), debugStr);
//#endif
//
//	s_rcnewObj = obj;
//	s_rcnewDesc = desc;
//
//	//s_rcnewObj = obj;
//	//s_rcnewDesc = desc;
//	//return rcnew_helper2<type>(obj, desc);
//	//return rcref<type>(obj, desc);
//}


template <typename type, typename allocator_t>
std::enable_if_t<allocator_t::is_static, rcnew_helper<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t*)
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

	return rcnew_helper<type>(obj, desc);
}


template <typename type, typename allocator_t>
std::enable_if_t<!allocator_t::is_static, rcnew_helper<type> >
rcnew_inner(
#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
	const char* debugStr,
#endif
	volatile allocator_t* al)
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

	return rcnew_helper<type>(obj, desc);
}


#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING

#define rcnew(type, ...) (::cogs::rcnew_inner<type>(COGS_DEBUG_AT, (::cogs::default_allocator*)0).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))
#define static_rcnew(al, type, ...) (::cogs::rcnew_inner<type>(COGS_DEBUG_AT, (al*)0).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))
#define instance_rcnew(al, type, ...) (::cogs::rcnew_inner<type>(COGS_DEBUG_AT, &(al)).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))
#define container_rcnew(al, type, ...) (::cogs::rcnew_inner<type>(COGS_DEBUG_AT, (al).get_allocator().get_ptr()).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))


#else

//template <typename type, typename... args_t>
//rcref<type> get_rcnew_lambda(args_t&&... args)
//{
//	//[](args_t&&... args2) {
//	rcref<type> result = ::cogs::get_rcnew_helper2<type>((::cogs::default_allocator*)0);
//	if constexpr (std::is_base_of_v<::cogs::object, type>)
//		new (result.get_obj()) type(*result.get_desc(), std::forward<args_t>(args)...);
//	else
//		new (result.get_obj()) type(std::forward<args_t>(args)...);
//	return result;
//	//};
//}
//
//#define rcnewX(type) ::cogs::get_rcnew_lambda<type>


//---

//#define rcnewX(type)\
//(\
//	[&](auto&&... cogs_rcnew_args) {\
//		auto result = ::cogs::get_rcnew_helper2<type>((::cogs::default_allocator*)0);\
//		if constexpr (std::is_base_of_v<::cogs::object, type>)\
//			new (result.get_obj()) type(*result.get_desc(), std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
//		else\
//			new (result.get_obj()) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
//		return result;\
//	}\
//)


template <typename type>
struct rcnew_helper_t
{
	rcnew_helper_t(type* p)
	{
	}

	operator rcref<type>()
	{
		rcref<type> result(rcnew_glue_element->m_obj, rcnew_glue_element->m_desc);
		rcnew_glue_element = rcnew_glue_element->m_saved;
		return result;
	}
};

#define rcnewX(type) (::cogs::rcref<type>)(::cogs::rcnew_helper_t<type>)new ((::cogs::get_rcnew_helper3((type*)0, (::cogs::default_allocator*)0), (type*)rcnew_glue_element->m_obj)) type


#define rcnew(type, ...)\
(\
	::cogs::rcnew_inner<type>((::cogs::default_allocator*)0)\
	.get_rcref(\
		[&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args)\
		{\
			if constexpr (std::is_base_of_v<::cogs::object, type>)\
				new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
			else\
			{\
				(void)cogs_rcnew_desc;\
				new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
			}\
		}\
	, ## __VA_ARGS__ )\
)

// It would be better to call new with parenthesis, as that would avoid potential zero-initialization.
// But, alas, clang doesn't like it.
//
//#define rcnew(type, ...)\
//(\
//	::cogs::rcnew_inner<type>((::cogs::default_allocator*)0)\
//	.get_rcref(\
//		[&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args)\
//		{\
//			if constexpr (std::is_base_of_v<::cogs::object, type>)\
//				new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); \
//			else\
//			{\
//				(void)cogs_rcnew_desc;\
//				if constexpr (sizeof...(cogs_rcnew_args) != 0) \
//					new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
//				else \
//					new (cogs_rcnew_obj) type;\
//			}\
//		}\
//	, ## __VA_ARGS__ )\
//)


#define static_rcnew(al, type, ...) (::cogs::rcnew_inner<type>((al*)0).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))
#define instance_rcnew(al, type, ...) (::cogs::rcnew_inner<type>(&(al)).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))
#define container_rcnew(al, type, ...) (::cogs::rcnew_inner<type>((al).get_allocator().get_ptr()).get_rcref([&](type* cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args) { if constexpr (std::is_base_of_v<::cogs::object, type>) new (cogs_rcnew_obj) type(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else if constexpr (sizeof...(cogs_rcnew_args) != 0) new (cogs_rcnew_obj) type(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...); else new (cogs_rcnew_obj) type; }, ## __VA_ARGS__ ))

#endif


#define placement_rcnew(objPtr, descRef, ...) \
(\
	::cogs::rcnew_helper<std::remove_pointer_t<decltype(objPtr)> >(objPtr, &descRef)\
	.placement_construct(\
		[&](decltype(objPtr) cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args)\
		{\
			if constexpr (std::is_base_of_v<::cogs::object, std::remove_pointer_t<decltype(cogs_rcnew_obj)> >)\
				new (cogs_rcnew_obj) std::remove_pointer_t<decltype(cogs_rcnew_obj)>(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
			else\
			{\
				(void)cogs_rcnew_desc;\
				new (cogs_rcnew_obj) std::remove_pointer_t<decltype(cogs_rcnew_obj)>(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
			}\
		}, ## __VA_ARGS__ \
	)\
)

// It would be better to call new with parenthesis, as that would avoid potential zero-initialization.
// But, alas, clang doesn't like it.
//
//#define placement_rcnew(objPtr, descRef, ...) \
//(\
//	::cogs::rcnew_helper<std::remove_pointer_t<decltype(objPtr)> >(objPtr, &descRef)\
//	.placement_construct(\
//		[&](decltype(objPtr) cogs_rcnew_obj, ::cogs::rc_obj_base* cogs_rcnew_desc, auto&&... cogs_rcnew_args)\
//		{\
//			if constexpr (std::is_base_of_v<::cogs::object, std::remove_pointer_t<decltype(cogs_rcnew_obj)> >)\
//				new (cogs_rcnew_obj) std::remove_pointer_t<decltype(cogs_rcnew_obj)>(*cogs_rcnew_desc, std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
//			else\
//			{\
//				(void)cogs_rcnew_desc;\
//				if constexpr (sizeof...(cogs_rcnew_args) != 0)\
//					new (cogs_rcnew_obj) std::remove_pointer_t<decltype(cogs_rcnew_obj)>(std::forward<decltype(cogs_rcnew_args)>(cogs_rcnew_args)...);\
//				else\
//					new (cogs_rcnew_obj) std::remove_pointer_t<decltype(cogs_rcnew_obj)>;\
//			}\
//		}, ## __VA_ARGS__ \
//	)\
//)


}


#include "cogs/mem/rcptr.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/mem/weak_rcptr.hpp"
#include "cogs/collections/container_dlist.hpp"


#endif
