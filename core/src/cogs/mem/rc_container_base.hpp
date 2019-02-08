//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_RC_PTR
#define COGS_RC_PTR

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/unowned.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

class object;


/*
	rcptr<> and ref<> are RAII reference container types.  rcptr<> is nullable, ref<> is not.

	RAII - Resource Acquisition Is Initialization
		RAII is antithetical to the use of garbage collection (which is implcit in Java and C#).
		RAII involves using the scope/lifetime of an object instance, to control the acquisition
		and release of a resource.  Generally, any resource requiring initialization and/or
		shutdown/release, should be represented as an object.  The objects scope is used to
		ensure that the resource gets released any time its object goes out of scope (such as
		an exception, or automatically at the end of a block or function).  For a dynamically
		allocated object, a reference-counting pointer type is used (instead of garbage
		collection), to ensure the object is destructed immediately when it is no longer
		referred to.  The reference-counted pointer type is itself an RAII class, as it acquires
		and releases references in its contructors/destructors, and on assignment.  This approach
		allows the dependency relationships between objects to implicity facilitate the management of
		resources. 
*/

template <typename type>
class rcptr;				// A reference-counted pointer class.
							// May refer to a non-reference-counted pointer, or NULL.
							// If type is dynamically allocated, reference-counting is done when
							// copied by value.   To avoid redundant reference counting of
							// function args, use: const rcptr<type>&
							// Note: The const& allows construction of a temporary, if necessary.
							// Only use a rcptr<type> in a subroutine when the object may
							// be retained after the call has returned.  Otherwise, use a type*

template <typename type>
class rcref;				// Like a rcptr<type>, but cannot point to NULL.
							// Choose between rcptr<type> and rcref<type> in the same way you
							// would choose between type* and type&.
							// To avoid redundant reference counting of function args,
							// use: const rcref<type>&
							// Note: The const& allows construction of a temporary, if necessary.
							// Only use a rcref<type> in a subroutine when the object may
							// be retained after the call has returned.  Otherwise, use a type&


template <typename type>
class weak_rcptr;

// Note: rcptr<>, rcref<>, and weak_rcptr<> are all single arg templates.  This is to make them
// similar to ptr<>, and compatible with: template <typename> class ref_type
// when used as a template arg.

#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment constructors specified


template <typename type_in, reference_strength_type refStrengthType = strong>
class rc_container_base;



template <typename type>
struct rc_container_base_content_t
{
	type* m_obj;
	rc_obj_base* m_desc;

	bool operator==(const rc_container_base_content_t& t) const { return (t.m_desc == m_desc) && (t.m_obj == m_obj); }

	bool operator!=(const rc_container_base_content_t& t) const { return !operator==(t); }

};


// specialized version for obj_ref_type of ptr (works with rcnew).
template <typename type_in, reference_strength_type refStrengthType>
class rc_container_base	// specialized version for no resolve required (allocator not needed), and obj_ref_type of ptr (works with rcnew).
{
public:
	typedef rc_container_base<type_in, refStrengthType> this_t;
	typedef type_in type;

	template <typename type2, reference_strength_type refStrengthType2>
	friend class rc_container_base;

	~rc_container_base() { release_inner(); }

protected:
	typedef rc_container_base_content_t<type> content_t;
	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token		read_token;
	typedef typename transactable_t::write_token	write_token;

	read_token begin_read() const volatile { read_token rt; m_contents.begin_read(rt); return rt; }
	void begin_read(read_token& rt) const volatile { m_contents.begin_read(rt); }
	bool end_read(read_token& t) const volatile { return m_contents.end_read(t); }
	void begin_write(write_token& wt) volatile { m_contents.begin_write(wt); }
	template <typename type2>
	void begin_write(write_token& wt, type2& src) volatile { m_contents.begin_write(wt, src); }
	bool promote_read_token(read_token& rt, write_token& wt) volatile { return m_contents.promote_read_token(rt, wt); }
	template <typename type2>
	bool promote_read_token(read_token& rt, write_token& wt, type2& src) volatile { return m_contents.promote_read_token(rt, wt, src); }
	bool end_write(write_token& t) volatile { return m_contents.end_write(t); }
	template <typename type2>
	bool end_write(read_token& t, type2& src) volatile { return m_contents.end_write(t, src); }

	void disown()	{ m_contents->m_desc = nullptr; }	// clear the desc ptr not the desc contents

	rc_obj_base* get_desc() const { return m_contents->m_desc; }
	const rc_obj_base* get_desc() const volatile { return begin_read()->m_desc; }

	void clear()
	{
		m_contents->m_obj = 0;
		m_contents->m_desc = 0;	// clear the desc ptr not the desc contents
	}

	void clear() volatile	{ m_contents.set(); }

	void set(const ptr<type>& obj)
	{
		m_contents->m_obj = obj.get_ptr();
		m_contents->m_desc = 0;	// clear the desc ptr not the desc contents
	}

	void set(const ptr<type>& obj) volatile
	{
		content_t tmp { obj.get_ptr(), nullptr };
		m_contents.swap_contents(tmp);
	}

	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc)
	{
		rc_obj_base* oldDesc = m_contents->m_desc;
		m_contents->m_obj = obj.get_ptr();
		m_contents->m_desc = desc.get_ptr();
	}

	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile
	{
		content_t tmp{ obj.get_ptr(), desc.get_ptr() };
		m_contents.swap_contents(tmp);
	}

	bool release_inner()
	{
		if (!!m_contents->m_desc)
			return m_contents->m_desc->release(refStrengthType);
		return false;
	}

	bool acquire_inner()
	{
		if (!!m_contents->m_desc && !m_contents->m_desc->acquire(refStrengthType))
		{
			clear();
			return false;
		}
		return true;
	}
	
	template <reference_strength_type refStrengthType2 = refStrengthType>
	bool guarded_acquire(read_token& rt) const volatile
	{
		volatile hazard& h = rc_obj_base::get_hazard();
		rc_obj_base* desc;
		hazard::pointer p;
		bool acquired;
		for (;;)
		{
			begin_read(rt);
			desc = rt->m_desc;
			if (!desc)
			{
				acquired = true;
				break;
			}

			rc_obj_base* descPtr = desc;
			p.bind(h, descPtr);
			if (!m_contents.is_current(rt) || !p.validate())
				continue;
			acquired = descPtr->acquire(refStrengthType2);
			if (p.release())
				descPtr->dispose();
			if (!acquired)
			{
				// Should be able to acquire weak from any, or strong from strong
				// This implies that any time we're using guarded_acquire with this_t, it will always acquire.
				if ((refStrengthType2 == weak) || (refStrengthType == strong))
					continue;
			}
			break;
		}
		return acquired;
	}

	bool guarded_begin_read(read_token& rt) const volatile	// returns false if it's a released weak reference.  May be treated as NULL.
	{
		bool result = true;
		if (refStrengthType == strong)
			begin_read(rt);
		else
		{
			volatile hazard& h = rc_obj_base::get_hazard();
			rc_obj_base* desc;
			hazard::pointer p;
			for (;;)
			{
				begin_read(rt);
				desc = rt->m_desc;
				if (!desc)
					break;

				rc_obj_base* descPtr = desc;
				p.bind(h, descPtr);
				if (!m_contents.is_current(rt) || !p.validate())
					continue;
				result = !rt->m_desc->is_released();
				if (p.release())
					descPtr->dispose();
				break;
			}
		}
		return result;
	}

	rc_container_base()
	{
		m_contents->m_obj = nullptr;
		m_contents->m_desc = nullptr;
	}

	rc_container_base(this_t&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
	}

	rc_container_base(const this_t& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;
		m_contents->m_desc = src.m_contents->m_desc;
		acquire_inner();
	}

	rc_container_base(const volatile this_t& src)
	{
		read_token rt;
		if (!!src.guarded_acquire<refStrengthType>(rt))
		{
			m_contents->m_obj = rt->m_obj;
			m_contents->m_desc = rt->m_desc;
		}
		else
		{
			m_contents->m_obj = nullptr;
			m_contents->m_desc = nullptr;
		}
	}

	template <typename type2>
	rc_container_base(rc_container_base<type2, refStrengthType>&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;	// <- A failure here means type conversion (caller) error.
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	rc_container_base(const rc_container_base<type2, refStrengthType2>& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;	// <- A failure here means type conversion (caller) error.
		m_contents->m_desc = src.m_contents->m_desc;
		acquire_inner();
	}
	
	template <typename type2, reference_strength_type refStrengthType2>
	rc_container_base(const volatile rc_container_base<type2, refStrengthType2>& src)
	{
		typename rc_container_base<type2, refStrengthType2>::read_token rt;
		if (!!src.guarded_acquire<refStrengthType>(rt))
		{
			COGS_ASSERT(rt->m_obj || !rt->m_desc);
			m_contents->m_obj = rt->m_obj;	// <- A failure here means type conversion (caller) error.
			m_contents->m_desc = rt->m_desc;
			//acquire_inner();

		}
		else
		{
			m_contents->m_obj = nullptr;
			m_contents->m_desc = nullptr;
		}
	}

	rc_container_base(const ptr<type>& obj)
	{
		m_contents->m_obj = obj.get_ptr();
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	rc_container_base(const ptr<type2>& obj)
	{
		m_contents->m_obj = obj.get_ptr();	// <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	rc_container_base(const ptr<type>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr();
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}


	template <typename type2>
	rc_container_base(const ptr<type2>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr();	// <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	
	this_t& operator=(this_t&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		release_inner();
		m_contents->m_obj = src.m_contents->m_obj;
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
		src.m_contents->m_obj = nullptr;
		return *this;
	}

	volatile this_t& operator=(this_t&& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		swap(src);
		src.release_inner();
		src.m_contents->m_desc = nullptr;
		src.m_contents->m_obj = nullptr;
		return *this;
	}

	this_t& operator=(const this_t& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		if (this != &src)
		{
			if (m_contents->m_desc == src.m_contents->m_desc)
				m_contents->m_obj = src.m_contents->m_obj;	// if desc are the same, no need to acquire/release, just copy obj
			else
			{
				rc_obj_base* oldDesc = m_contents->m_desc;	// to release later (in case it's the same one we haven't acquired yet)
				if (!src.m_contents->m_desc)				// If no new desc, just copy obj and clear desc.
				{
					m_contents->m_obj = src.m_contents->m_obj;
					m_contents->m_desc = nullptr;
				}
				else if (!!src.m_contents->m_desc->acquire(refStrengthType))
				{
					COGS_ASSERT(!!src.m_contents->m_obj);
					m_contents->m_obj = src.m_contents->m_obj;
					m_contents->m_desc = src.m_contents->m_desc;
					//acquire_inner();
				}
				else
					clear();// failure to acquire
				
				if (!!oldDesc)
					oldDesc->release(refStrengthType);
			}
		}
		return *this;
	}

	this_t& operator=(const volatile this_t& src)
	{
		if (this != &src)
		{
			rc_obj_base* oldDesc = m_contents->m_desc;	// to release later (in case it's the same one we haven't acquired yet)

			read_token rt;
			if (!!src.guarded_acquire<refStrengthType>(rt))
			{
				COGS_ASSERT(rt->m_obj || !rt->m_desc);
				m_contents->m_obj = rt->m_obj;
				m_contents->m_desc = rt->m_desc;
				//acquire_inner();
			}
			else
				clear();

			if (!!oldDesc)
				oldDesc->release(refStrengthType);
		}
		return *this;
	}

	volatile this_t& operator=(const this_t& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		if (this != &src)
		{
			if (!!src.m_contents->m_desc && !src.m_contents->m_desc->acquire(refStrengthType))	// couldn't acquire, treat as null
				release();
			else
			{
				this_t tmp;
				m_contents.exchange(src.m_contents, tmp.m_contents);
			}
		}
		return *this;
	}

	template <typename type2>
	this_t& operator=(rc_container_base<type2, refStrengthType>&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		release_inner();
		m_contents->m_obj = src.m_contents->m_obj;	// <- A failure here means type conversion (user) error.
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(rc_container_base<type2, refStrengthType>&& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		swap(src);
		src.release_inner();
		src.m_contents->m_desc = nullptr;
		return *this;
	}


	template <typename type2, reference_strength_type refStrengthType2>
	this_t& operator=(const rc_container_base<type2, refStrengthType2>& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		if (m_contents->m_desc == src.m_contents->m_desc)	// If desc are the same, no need to acquire/release, just copy obj
			m_contents->m_obj = src.m_contents->m_obj;		// <- A failure here means type conversion (user) error.
		else
		{
			rc_obj_base* oldDesc = m_contents->m_desc;	// to release later (in case it's the same one we haven't acquired yet)
			if (!src.m_contents->m_desc)				// If no new desc, just copy obj and clear desc.
			{
				m_contents->m_obj = src.m_contents->m_obj;	// <- A failure here means type conversion (user) error.
				m_contents->m_desc = nullptr;
			}
			else if (!src.m_contents->m_desc->acquire(refStrengthType))	// if failure to acquire
				clear();
			else									// acquired desc, copy contents
			{										
				m_contents->m_obj = src.m_contents->m_obj;	// <- A failure here means type conversion (user) error.
				m_contents->m_desc = src.m_contents->m_desc;
			}

			if (!!oldDesc)
				oldDesc->release(refStrengthType);
		}
		return *this;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	this_t& operator=(const volatile rc_container_base<type2, refStrengthType2>& src)
	{
		rc_obj_base* oldDesc = m_contents->m_desc;	// to release later (in case it's the same one we haven't acquired yet)

		typename rc_container_base<type2, refStrengthType2>::read_token rt;
		if (!src.guarded_acquire<refStrengthType>(rt))
			clear();
		else
		{
			COGS_ASSERT(rt->m_obj || !rt->m_desc);
			m_contents->m_obj = rt->m_obj;	// <- A failure here means type conversion (user) error.
			m_contents->m_desc = rt->m_desc;
		}

		if (!!oldDesc)
			oldDesc->release(refStrengthType);

		return *this;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	volatile this_t& operator=(const rc_container_base<type2, refStrengthType2>& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		typename rc_container_base<type2, refStrengthType2>::read_token rt;
		if (!src.guarded_acquire<refStrengthType>(rt))
			release();
		else
		{
			this_t tmp;	// need a matching content_t to atomically swap
			tmp.m_contents->m_obj = rt->m_obj;	// <- A failure here means type conversion (user) error.
			tmp.m_contents->m_desc = rt->m_desc;
			m_contents.swap(tmp.m_contents);
		}
		return *this;
	}

	this_t& operator=(const ptr<type>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr();
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(const ptr<type2>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr();	// <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	volatile this_t& operator=(const ptr<type>& src) volatile
	{
		this_t tmp(src);	// need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(const ptr<type2>& src) volatile
	{
		this_t tmp(src);	// need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	bool release()
	{
		bool b = release_inner();
		clear();
		return b;
	}

	bool release() volatile
	{
		this_t tmp;
		m_contents.exchange_set(*(tmp.m_contents));
		return tmp.release();
	}

	type* get_obj() const
	{
		// If a weak reference, we need a strong reference before we can get a valid pointer value
		// It's sufficient to make sure that the weak reference hasn't expired
		if ((refStrengthType != strong) && !!m_contents->m_desc && m_contents->m_desc->is_released())	// No need to acquire a guard.  A weak ref retains the alloc, if not the object.
			return nullptr;
		return m_contents->m_obj;
	}

	// Not safe to call on a weak reference unless you know it's still in scope somewhere.
	// Otherwise, the return value may be invalidated before use.
	type* get_obj() const volatile
	{
		read_token rt;
		return guarded_begin_read(rt) ? rt->m_obj : nullptr;
	}

	type* peek_obj() const			{ return m_contents->m_obj; }

	type* peek_obj() const volatile	{ return get_obj(); }

	static size_t mark_bits()					{ return ptr<type>::mark_bits(); }
	static size_t mark_mask()					{ return ptr<type>::mark_mask(); }

	size_t get_mark() const { ptr<type> p = m_contents->m_obj; return p.get_mark(); }
	size_t get_mark() const volatile { ptr<type> p = begin_read()->m_obj; return p.get_mark(); }

	type* get_unmarked() const { ptr<type> p = m_contents->m_obj; return p.get_unmarked(); }
	type* get_unmarked() const volatile { ptr<type> p = begin_read()->m_obj; return p.get_unmarked(); }

	type* get_marked(size_t mark) const { ptr<type> p = m_contents->m_obj; return p.get_marked(mark); }
	type* get_marked(size_t mark) const volatile { ptr<type> p = begin_read()->m_obj; return p.get_marked(mark); }

	void clear_mark() { ptr<type> p = m_contents->m_obj; p.clear_mark(); m_contents->m_obj = p.get_ptr(); }
	void clear_mark() volatile
	{
		read_token rt;
		for (;;)
		{
			begin_read(rt);
			ptr<type> p = rt->m_obj;
			if (!p.get_mark())
				break;
			write_token wt;
			if (!promote_read_token(rt, wt))
				continue;

			p.clear_mark();
			wt->m_obj = p.get_ptr();

			if (!!end_write(wt))
				break;
			//continue;
		}
	}

	void set_mark(size_t mark) { ptr<type> p = m_contents->m_obj; p.set_mark(mark); m_contents->m_obj = p.get_ptr(); }
	void set_mark(size_t mark) volatile
	{
		for (;;)
		{
			write_token wt;
			begin_write(wt);

			ptr<type> p = rt->m_obj;
			p.set_mark(mark);
			wt->m_obj = p.get_ptr();

			if (!end_write(wt))
				continue;
			break;
		}
	}

	void set_to_mark(size_t mark)
	{
		release_inner();
		m_contents->m_desc = nullptr;
		ptr<type> p;
		p.set_to_mark(mark);
		m_contents->m_obj = p.get_ptr();
	}

	void set_to_mark(size_t mark) volatile
	{
		this_t tmp;	// need a matching content_t to atomically swap
		tmp.set_to_mark(mark);
		m_contents.swap(tmp.m_contents);
	}

	void set_marked(type* p, size_t mark)
	{
		release_inner();
		m_contents->m_desc = nullptr;
		ptr<type> tmp;
		tmp.set_marked(p, mark);
		m_contents->m_obj = tmp.get_ptr();
	}

	void set_marked(type* p, size_t mark) volatile
	{
		this_t tmp;	// need a matching content_t to atomically swap
		tmp.set_marked(p, mark);
		m_contents.swap(tmp.m_contents);
	}

	bool is_empty() const					{ return !get_obj(); }
	bool is_empty() const volatile			{ return !get_obj(); }

	bool operator!() const						{ return !get_obj(); }
	bool operator!() const volatile				{ return !get_obj(); }

	bool operator==(const this_t& cmp) const
	{
		return (m_contents->m_obj == cmp.m_contents->m_obj) && (m_contents->m_desc == cmp.m_contents->m_desc);
	}

	bool operator==(const this_t& cmp) const volatile
	{
		bool result = true;
		if (this != &cmp)
		{
			read_token rt;
			begin_read(rt);
			result = (rt->m_obj == cmp.m_contents->m_obj) && (rt->m_desc == cmp.m_contents->m_desc);
		}
		return result;
	}

	bool operator==(const volatile this_t& cmp) const
	{ return cmp.operator==(*this); }

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator==(const rc_container_base<type2, refStrengthType2>& cmp) const
	{
		return (m_contents->m_obj == cmp.m_contents->m_obj) && (m_contents->m_desc == cmp.m_contents->m_desc);
	}

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator==(const rc_container_base<type2, refStrengthType2>& cmp) const volatile
	{
		bool result = true;
		read_token rt;
		begin_read(rt);
		result = (rt->m_obj == cmp.m_contents->m_obj) && (rt->m_desc == cmp.m_contents->m_desc);
		return result;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator==(const volatile rc_container_base<type2, refStrengthType2>& cmp) const
	{ return cmp.operator==(*this); }

	bool operator!=(const this_t& cmp) const
	{ return !operator==(cmp); }

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator!=(const rc_container_base<type2, refStrengthType2>& cmp) const
	{ return !operator==(cmp); }

	bool operator!=(const this_t& cmp) const volatile
	{ return !operator==(cmp); }

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator!=(const rc_container_base<type2, refStrengthType2>& cmp) const volatile
	{ return !operator==(cmp); }

	bool operator!=(const volatile this_t& cmp) const
	{ return !operator==(cmp); }

	template <typename type2, reference_strength_type refStrengthType2>
	bool operator!=(const volatile rc_container_base<type2, refStrengthType2>& cmp) const
	{ return !operator==(cmp); }

	bool operator==(const ptr<type>& src) const				{ return (get_obj() == src); }
	bool operator==(const ptr<type>& src) const volatile	{ return (get_obj() == src); }
	bool operator!=(const ptr<type>& src) const				{ return (get_obj() != src); }
	bool operator!=(const ptr<type>& src) const volatile	{ return (get_obj() != src); }


	// swap
	template <typename type2>
	void swap(rc_container_base<type2, refStrengthType>& wth)
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = std::move(tmp);
	}

	template <typename type2>
	void swap(rc_container_base<type2, refStrengthType>& wth) volatile
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = std::move(tmp);
	}

	template <typename type2>
	void swap(volatile rc_container_base<type2, refStrengthType>& wth)	{ wth.swap(*this); }


	// exchange
	template <typename type2, reference_strength_type refStrengthType2>
	this_t exchange(rc_container_base<type2, refStrengthType2>&& src)
	{
		this_t rtn(std::move(src));
		swap(rtn);
		return rtn;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	this_t exchange(const rc_container_base<type2, refStrengthType2>& src)
	{
		this_t rtn(src);
		swap(rtn);
		return rtn;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	this_t exchange(rc_container_base<type2, refStrengthType2>&& src) volatile
	{
		this_t rtn(std::move(src));
		swap(rtn);
		return rtn;
	}

	template <typename type2, reference_strength_type refStrengthType2>
	this_t exchange(const rc_container_base<type2, refStrengthType2>& src) volatile
	{
		this_t rtn(src);
		swap(rtn);
		return rtn;
	}


	template <typename type2, reference_strength_type refStrengthType2>
	void exchange(rc_container_base<type2, refStrengthType2>&& src, this_t& rtn)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		rtn = std::move(tmp);
	}

	template <typename type2, reference_strength_type refStrengthType2>
	void exchange(const rc_container_base<type2, refStrengthType2>& src, this_t& rtn)
	{
		this_t tmp(src);
		swap(tmp);
		rtn = std::move(tmp);
	}

	template <typename type2, reference_strength_type refStrengthType2>
	void exchange(rc_container_base<type2, refStrengthType2>&& src, this_t& rtn) volatile
	{
		this_t tmp(std::move(src));
		swap(tmp);
		rtn = std::move(tmp);
	}

	template <typename type2, reference_strength_type refStrengthType2>
	void exchange(const rc_container_base<type2, refStrengthType2>& src, this_t& rtn) volatile
	{
		this_t tmp(src);
		swap(tmp);
		rtn = std::move(tmp);
	}


	// compare_exchange
	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const rc_container_base<type3, refStrengthType3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(std::move(src));
			swap(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const rc_container_base<type3, refStrengthType3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
		}
		return b;
	}

	template <typename type2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const ptr<type2>& src, const rc_container_base<type3, refStrengthType3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const ptr<type3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(std::move(src));
			swap(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const ptr<type3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
		}
		return b;
	}

	template <typename type2, typename type3>
	bool compare_exchange(const ptr<type2>& src, const ptr<type3>& cmp)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
		}
		return b;
	}




	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const rc_container_base<type3, refStrengthType3>& cmp) volatile
	{
		this_t tmpSrc(std::move(src));
		content_t tmpCmp{ cmp.m_contents->m_obj, cmp.m_contents->m_desc };
		content_t tmpRtn;
		bool b = m_contents.compare_exchange_contents(*tmpSrc.m_contents, tmpCmp, tmpRtn);
		if (b)
		{
			tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
			if (!!tmpRtn.m_desc)	// release the reference we just removed from this
				tmpRtn.m_desc->release(refStrengthType);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const rc_container_base<type3, refStrengthType3>& cmp) volatile
	{
		this_t tmpSrc(src);
		content_t tmpCmp{ cmp.m_contents->m_obj, cmp.m_contents->m_desc };
		content_t tmpRtn;
		bool b = m_contents.compare_exchange_contents(*tmpSrc.m_contents, tmpCmp, tmpRtn);
		if (b)
		{
			tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
			if (!!tmpRtn.m_desc)	// release the reference we just removed from this
				tmpRtn.m_desc->release(refStrengthType);
		}
		return b;
	}

	template <typename type2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const ptr<type2>& src, const rc_container_base<type3, refStrengthType3>& cmp) volatile
	{
		this_t tmpSrc(src);
		content_t tmpCmp{ cmp.m_contents->m_obj, cmp.m_contents->m_desc };
		content_t tmpRtn;
		bool b = m_contents.compare_exchange_contents(*tmpSrc.m_contents, tmpCmp, tmpRtn);
		if (b)
		{
			//tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
			if (!!tmpRtn.m_desc)	// release the reference we just removed from this
				tmpRtn.m_desc->release(refStrengthType);
		}
		return b;
	}


	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const ptr<type3>& cmp) volatile
	{
		this_t tmpSrc(std::move(src));
		read_token rt;
		bool result;
		for (;;)
		{
			begin_read(rt);
			rc_obj_base* oldDesc = rt->m_desc;
			if (rt->m_obj != cmp.get_ptr())		// <- A failure here means type conversion (user) error.
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				if (!!oldDesc)					// release the reference we just removed from this
					oldDesc->release(refStrengthType);
				result = true;
				break;
			}
		}
		return result;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const ptr<type3>& cmp) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		bool result;
		for (;;)
		{
			begin_read(rt);
			rc_obj_base* oldDesc = rt->m_desc;
			if (rt->m_obj != cmp.get_ptr())		// <- A failure here means type conversion (user) error.
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				if (!!oldDesc)					// release the reference we just removed from this
					oldDesc->release(refStrengthType);
				result = true;
				break;
			}
		}
		return result;
	}

	template <typename type2, typename type3>
	bool compare_exchange(const ptr<type2>& src, const ptr<type3>& cmp) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		bool result;
		for (;;)
		{
			begin_read(rt);
			rc_obj_base* oldDesc = rt->m_desc;
			if (rt->m_obj != cmp.get_ptr())		// <- A failure here means type conversion (user) error.
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				//tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				if (!!oldDesc)					// release the reference we just removed from this
					oldDesc->release(refStrengthType);
				result = true;
				break;
			}
		}
		return result;
	}
	




	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(std::move(src));
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}

	template <typename type2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const ptr<type2>& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const ptr<type3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(std::move(src));
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const ptr<type3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}

	template <typename type2, typename type3>
	bool compare_exchange(const ptr<type2>& src, const ptr<type3>& cmp, this_t& rtn)
	{
		bool b = (*this == cmp);
		if (b)
		{
			this_t tmp(src);
			swap(tmp);
			rtn = std::move(tmp);
		}
		return b;
	}


	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(std::move(src));
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if ((rt->m_obj != cmp.m_contents->m_obj) || (rt->m_desc != cmp.m_contents->m_desc))
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if ((rt->m_obj != cmp.m_contents->m_obj) || (rt->m_desc != cmp.m_contents->m_desc))
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}

	template <typename type2, typename type3, reference_strength_type refStrengthType3>
	bool compare_exchange(const ptr<type2>& src, const rc_container_base<type3, refStrengthType3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if ((rt->m_obj != cmp.m_contents->m_obj) || (rt->m_desc != cmp.m_contents->m_desc))
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(rc_container_base<type2, refStrengthType2>&& src, const ptr<type3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(std::move(src));
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if (rt->m_obj != cmp.get_ptr())
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}

	template <typename type2, reference_strength_type refStrengthType2, typename type3>
	bool compare_exchange(const rc_container_base<type2, refStrengthType2>& src, const ptr<type3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if (rt->m_obj != cmp.get_ptr())
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}

	template <typename type2, typename type3>
	bool compare_exchange(const ptr<type2>& src, const ptr<type3>& cmp, this_t& rtn) volatile
	{
		this_t tmpSrc(src);
		read_token rt;
		this_t tmp;	// contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if (rt->m_obj != cmp.get_ptr())
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown();	// tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner();	// release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		rtn = std::move(tmp);
		return result;
	}
};


}


#endif
