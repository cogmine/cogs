//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_DELAYED_CONSTRUCTION
#define COGS_HEADER_MEM_DELAYED_CONSTRUCTION

#include <type_traits>

#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"


namespace cogs {


template <typename T>
class delayed_construction
{
private:
	typedef delayed_construction<T> this_t;
	placement<T> m_contents;

public:
	delayed_construction() { }
	delayed_construction(this_t&& src) { m_contents.construct(std::move(src.get())); }
	delayed_construction(const this_t& src) { m_contents.construct(src.get()); }
	~delayed_construction() { m_contents.destruct(); }

	this_t& operator=(this_t&& src) { get() = std::move(src.get()); return *this; }
	this_t& operator=(const this_t& src) { get() = src.get(); return *this; }

	T& get() { return m_contents.get(); }
	const T& get() const { return m_contents.get(); }
	volatile T& get() volatile { return m_contents.get(); }
	const volatile T& get() const volatile { return m_contents.get(); }

	T& operator*() { return get(); }
	const T& operator*() const { return get(); }
	volatile T& operator*() volatile { return get(); }
	const volatile T& operator*() const volatile { return get(); }

	T* operator->() { return &get(); }
	const T* operator->() const { return &get(); }
	volatile T* operator->() volatile { return &get(); }
	const volatile T* operator->() const volatile { return &get(); }
};


//template <typename T>
//class delayed_construction_obj : public object
//{
//private:
//	typedef delayed_construction_obj<T> this_t;
//
//	placement<T> m_contents;
//
//public:
//	delayed_construction_obj() { }
//	delayed_construction_obj(this_t&& src) { m_contents.construct(std::move(src.get())); }
//	delayed_construction_obj(const this_t& src) { m_contents.construct(src.get()); }
//	~delayed_construction_obj() { m_contents.destruct(); }
//
//	this_t& operator=(this_t&& src) { get() = std::move(src.get()); return *this; }
//	this_t& operator=(const this_t& src) { get() = src.get(); return *this; }
//
//	T& get() { return m_contents.get(); }
//	const T& get() const { return m_contents.get(); }
//	volatile T& get() volatile { return m_contents.get(); }
//	const volatile T& get() const volatile { return m_contents.get(); }
//
//	T& operator*() { return get(); }
//	const T& operator*() const { return get(); }
//	volatile T& operator*() volatile { return get(); }
//	const volatile T& operator*() const volatile { return get(); }
//
//	T* operator->() { return &get(); }
//	const T* operator->() const { return &get(); }
//	volatile T* operator->() volatile { return &get(); }
//	const volatile T* operator->() const volatile { return &get(); }
//
//	const rcref<T>& get_ref(unowned_t<rcptr<T> >& storage = unowned_t<rcptr<T> >().get_unowned())
//	{ return get_self_rcref(&get(), storage); }
//
//	const rcref<const T>& get_ref(unowned_t<rcptr<const T> >& storage = unowned_t<rcptr<const T> >().get_unowned()) const
//	{ return get_self_rcref(&get(), storage); }
//
//	const rcref<volatile T>& get_ref(unowned_t<rcptr<volatile T> >& storage = unowned_t<rcptr<volatile T> >().get_unowned()) volatile
//	{ return get_self_rcref(&get(), storage); }
//
//	const rcref<const volatile T>& get_ref(unowned_t<rcptr<const volatile T> >& storage = unowned_t<rcptr<const volatile T> >().get_unowned()) const volatile
//	{ return get_self_rcref(&get(), storage); }
//};


}


#endif
