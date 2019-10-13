//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RC_OBJ
#define COGS_HEADER_MEM_RC_OBJ


#include "cogs/mem/allocator_container.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"


namespace cogs {


// A base class for objects that contain self-references.
// Enables use of: this_rcref, this_rcptr, this_weak_rcptr
class rc_object_base
{
};


#ifdef DOXYGEN
/// @brief A reference-counted object container.
/// 
/// rc_obj instances contain reference counts.  rc_obj is used internally by rcref, rcptr, weak_rcptr.
/// @tparam T Type to contain
/// @tparam allocator_type Allocator type to use to allocate reference-counted object (and reference counts).  
/// Default: default_allocator
template <typename T, class allocator_type = default_allocator>
class rc_obj : public rc_obj_base // default is: rc_obj<type, allocator_type, false, true, false>
{
public:
	/// @brief Alies to encapsulated type
	typedef T type;

	/// @brief Alias to the reference type
	typedef typename allocator_type::ref_t::template cast_t<this_t> ref_t;

	/// @{
	/// @brief Allocate an object
	/// @param al Allocator to allocate from (if instance based)
	/// @return A reference to a newly allocated reference-counted object.
	static ref_t allocate(volatile allocator_type& al);

	/// @brief Allocate an object
	/// @return A reference to a newly allocated reference-counted object.
	static ref_t allocate();
	/// @}

	/// @{
	type* get_obj();
	const type* get_obj() const;
	volatile type* get_obj() volatile;
	const volatile type* get_obj() const volatile;
	/// @}

	/// @{
	static this_t* from_obj(type* obj);
	static const this_t* from_obj(const type* obj);
	static volatile this_t* from_obj(volatile type* obj);
	static const volatile this_t* from_obj(const volatile type* obj);
	/// @{

	/// @{
	ref_t& get_self_reference();
	const ref_t& get_self_reference() const;
	volatile ref_t& get_self_reference() volatile;
	const volatile ref_t& get_self_reference() const volatile;
	/// @}
};

#endif


template <typename T,
	class allocator_type = default_allocator,
	bool is_static = allocator_type::is_static,
	bool is_ptr_based = std::is_same_v<typename allocator_type::ref_t, ptr<void> > >
class rc_obj : public rc_obj_base // default is: rc_obj<type, allocator_type, true, false>
{
public:
	typedef rc_obj<T, allocator_type, true, false> this_t;
	typedef T type;
	typedef typename allocator_type::ref_t::template cast_t<this_t> ref_t;

private:
	placement<type> m_contents;
	ref_t m_selfReference;
#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif

	virtual void released()
	{
		ptr<type> p = get_obj();
		p.get_ptr()->type::~type();
	}

	virtual void dispose()
	{
		ref_t selfRef = m_selfReference;
		~rc_obj();
		allocator_container<allocator_type>::deallocate(selfRef);
	}

	rc_obj(const ref_t& selfReference)
		: m_selfReference(selfReference)
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_obj();
#endif
	}

	rc_obj(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	static ref_t allocate(volatile allocator_type& al) { COGS_ASSERT(false); ref_t empty; return empty; }

	static ref_t allocate()
	{
		ref_t r = allocator_container<allocator_type>::template allocate_type<this_t>();
		if (!!r)
			new (r.get_ptr()) this_t(r);
		return r;
	}

	type* get_obj() { return &m_contents.get(); }
	const type* get_obj() const { return &m_contents.get(); }
	volatile type* get_obj() volatile { return &m_contents.get(); }
	const volatile type* get_obj() const volatile { return &m_contents.get(); }

	static this_t* from_obj(type* obj)
	{
		static const ptrdiff_t offset = (unsigned char*)&(((this_t*)(unsigned char*)1)->m_contents) - (unsigned char*)((this_t*)(unsigned char*)1);

		// Casting to unsigned char* is a special case in C++ which allows bypassing aliasing/type-punning warnings
		return reinterpret_cast<this_t*>(((unsigned char*)obj) - offset);
	}

	static const this_t* from_obj(const type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }
	static volatile this_t* from_obj(volatile type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }
	static const volatile this_t* from_obj(const volatile type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }

	ref_t& get_self_reference() { return m_selfReference; }
	const ref_t& get_self_reference() const { return m_selfReference; }
	volatile ref_t& get_self_reference() volatile { return m_selfReference; }
	const volatile ref_t& get_self_reference() const volatile { return m_selfReference; }
};


template <typename T, class allocator_type>
class rc_obj<T, allocator_type, false, false> : public rc_obj_base
{
public:
	typedef rc_obj<T, allocator_type, false, false> this_t;
	typedef T type;
	typedef typename allocator_type::ref_t::template cast_t<this_t> ref_t;

private:
	placement<type> m_contents;
	ref_t m_selfReference;
#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif
	allocator_container<allocator_type> m_allocator;

	virtual void released()
	{
		ptr<type> p = get_obj();
		p.get_ptr()->type::~type();
	}

	virtual void dispose()
	{
		ref_t selfRef = m_selfReference;
		~rc_obj();
		m_allocator.deallocate(selfRef);
	}

	rc_obj(const ref_t& selfReference, allocator& al)
		: m_selfReference(selfReference),
		m_allocator(al)
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_obj();
#endif
	}

	rc_obj(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	static ref_t allocate() { COGS_ASSERT(false); ref_t empty; return empty; }

	static ref_t allocate(volatile allocator_type& al)
	{
		allocator_container<allocator_type> alw(al);
		ref_t r = alw.template allocate_type<this_t>();
		if (!!r)
			new (r.get_ptr()) this_t(r, al);
		return r;
	}

	type* get_obj() { return &m_contents.get(); }
	const type* get_obj() const { return &m_contents.get(); }
	volatile type* get_obj() volatile { return &m_contents.get(); }
	const volatile type* get_obj() const volatile { return &m_contents.get(); }

	static this_t* from_obj(type* obj)
	{
		static const ptrdiff_t offset = (unsigned char*)&(((this_t*)(unsigned char*)1)->m_contents) - (unsigned char*)((this_t*)(unsigned char*)1);

		// Casting to unsigned char* is a special case in C++ which allows bypassing aliasing/type-punning warnings
		return reinterpret_cast<this_t*>(((unsigned char*)obj) - offset);
	}
	static const this_t* from_obj(const type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }
	static volatile this_t* from_obj(volatile type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }
	static const volatile this_t* from_obj(const volatile type* obj) { return const_cast<this_t*>(from_obj(const_cast<type*>(obj))); }

	ref_t& get_self_reference() { return m_selfReference; }
	const ref_t& get_self_reference() const { return m_selfReference; }
	volatile ref_t& get_self_reference() volatile { return m_selfReference; }
	const volatile ref_t& get_self_reference() const volatile { return m_selfReference; }
};


template <typename T, class allocator_type>
class rc_obj<T, allocator_type, true, true> : public rc_obj_base
{
public:
	typedef rc_obj<T, allocator_type, true, true> this_t;
	typedef T type;
	typedef ptr<this_t> ref_t;

private:
#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif

	virtual void released()
	{
		ptr<type> p = get_obj();
		p.get_ptr()->type::~type();
	}

	virtual void dispose()
	{
		allocator_container<default_allocator>::destruct_deallocate_type<this_t>(this);
	}

	rc_obj()
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_obj();
#endif
	}

	rc_obj(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	static ref_t allocate(volatile allocator_type& al) { COGS_ASSERT(false); ref_t empty; return empty; }

	static ref_t allocate()
	{
		ref_t r = allocator_container<allocator_type>::template allocate_type_with_header<this_t, type>();
		if (!!r)
			new (r.get_ptr()) this_t;
		return r;
	}

	type* get_obj() { return get_type_block_from_header<this_t, type>(this); }
	const type* get_obj() const { return get_type_block_from_header<const this_t, const type>(this); }
	volatile type* get_obj() volatile { return get_type_block_from_header<volatile this_t, volatile type>(this); }
	const volatile type* get_obj() const volatile { return get_type_block_from_header<const volatile this_t, const volatile type>(this); }

	static this_t* from_obj(type* obj) { return get_header_from_type_block<this_t, type>(obj); }
	static const this_t* from_obj(const type* obj) { return get_header_from_type_block<const this_t, const type>(obj); }
	static volatile this_t* from_obj(volatile type* obj) { return get_header_from_type_block<volatile this_t, volatile type>(obj); }
	static const volatile this_t* from_obj(const volatile type* obj) { return get_header_from_type_block<const volatile this_t, const volatile type>(obj); }

	ref<this_t> get_self_reference() { return *this; }
	ref<const this_t> get_self_reference() const { return *this; }
	ref<volatile this_t> get_self_reference() volatile { return *this; }
	ref<const volatile this_t> get_self_reference() const volatile { return *this; }

};


template <typename T, class allocator_type>
class rc_obj<T, allocator_type, false, true> : public rc_obj_base
{
public:
	typedef rc_obj<T, allocator_type, false, true> this_t;
	typedef T type;
	typedef ptr<this_t> ref_t;

private:
#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif
	allocator_container<allocator_type> m_allocator;

	virtual void released()
	{
		ptr<type> p = get_obj();
		p.get_ptr()->type::~type();
	}

	virtual void dispose()
	{
		allocator_container<default_allocator>::destruct_deallocate_type<this_t>(this);
	}

	rc_obj(allocator* al)
		: m_allocator(al)
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_obj();
#endif
	}

	rc_obj(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	static ref_t allocate() { COGS_ASSERT(false); ref_t empty; return empty; }

	static ref_t allocate(volatile allocator& al)
	{
		allocator_container<allocator_type> alw(al);
		ref_t r = alw.template allocate_type_with_header<this_t, type>();
		if (!!r)
			new (r.get_ptr()) this_t(al);
		return r;
	}

	type* get_obj() { return get_type_block_from_header<this_t, type>(this); }
	const type* get_obj() const { return get_type_block_from_header<const this_t, const type>(this); }
	volatile type* get_obj() volatile { return get_type_block_from_header<volatile this_t, volatile type>(this); }
	const volatile type* get_obj() const volatile { return get_type_block_from_header<const volatile this_t, const volatile type>(this); }

	static this_t* from_obj(type* obj) { return get_header_from_type_block<this_t, type>(obj); }
	static const this_t* from_obj(const type* obj) { return get_header_from_type_block<const this_t, const type>(obj); }
	static volatile this_t* from_obj(volatile type* obj) { return get_header_from_type_block<volatile this_t, volatile type>(obj); }
	static const volatile this_t* from_obj(const volatile type* obj) { return get_header_from_type_block<const volatile this_t, const volatile type>(obj); }

	ref<this_t> get_self_reference() { return *this; }
	ref<const this_t> get_self_reference() const { return *this; }
	ref<volatile this_t> get_self_reference() volatile { return *this; }
	ref<const volatile this_t> get_self_reference() const volatile { return *this; }
};


template <typename T>
class rc_obj<T, default_allocator, true, true> : public rc_obj_base
{
public:
	typedef rc_obj<T, default_allocator, true, true> this_t;
	typedef T type;
	typedef ref<this_t> ref_t;

private:
#if COGS_DEBUG_RC_OBJ
	type* m_contentsPtrDebug;
#endif

	virtual void released()
	{
		placement_destruct(get_obj());
	}

	virtual void dispose()
	{
		allocator_container<default_allocator>::destruct_deallocate_type<this_t>(this);
	}

	rc_obj()
	{
#if COGS_DEBUG_RC_OBJ
		m_contentsPtrDebug = get_obj();
#endif
	}

	rc_obj(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

public:
	static ref_t allocate(volatile default_allocator& al) { COGS_ASSERT(false); ptr<this_t> empty; return empty.dereference(); }

	static ref_t allocate()
	{
		ref_t r = allocator_container<default_allocator>::template allocate_type_with_header<this_t, type>().dereference();
		new (r.get_ptr()) this_t;
		return r;
	}

	type* get_obj() { return get_type_block_from_header<this_t, type>(this); }
	const type* get_obj() const { return get_type_block_from_header<const this_t, const type>(this); }
	volatile type* get_obj() volatile { return get_type_block_from_header<volatile this_t, volatile type>(this); }
	const volatile type* get_obj() const volatile { return get_type_block_from_header<const volatile this_t, const volatile type>(this); }

	static this_t* from_obj(type* obj) { return get_header_from_type_block<this_t, type>(obj); }
	static const this_t* from_obj(const type* obj) { return get_header_from_type_block<const this_t, const type>(obj); }
	static volatile this_t* from_obj(volatile type* obj) { return get_header_from_type_block<volatile this_t, volatile type>(obj); }
	static const volatile this_t* from_obj(const volatile type* obj) { return get_header_from_type_block<const volatile this_t, const volatile type>(obj); }

	ref<this_t> get_self_reference() { return *this; }
	ref<const this_t> get_self_reference() const { return *this; }
	ref<volatile this_t> get_self_reference() volatile { return *this; }
	ref<const volatile this_t> get_self_reference() const volatile { return *this; }
};


}


#endif
