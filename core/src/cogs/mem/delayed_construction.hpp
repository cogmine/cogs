//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_DELAYED_CONSTRUCTION
#define COGS_HEADER_MEM_DELAYED_CONSTRUCTION

#include <type_traits>

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


}


#endif
