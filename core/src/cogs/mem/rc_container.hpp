//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RC_CONTAINER_BASE
#define COGS_HEADER_MEM_RC_CONTAINER_BASE

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/mem/is_same_instance.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/unowned.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

class object;


// rcptr<> and rcref<> are RAII reference container types.  rcptr<> is nullable, rcref<> is not.

//	RAII - Resource Acquisition Is Initialization
//		RAII is antithetical to the use of garbage collection (which is implcit in Java and C#).
//		RAII involves using the scope/lifetime of an object instance, to control the acquisition
//		and release of a resource.  Generally, any resource requiring initialization and/or
//		shutdown/release, should be represented as an object.  The objects scope is used to
//		ensure that the resource gets released any time its object goes out of scope (such as
//		an exception, or automatically at the end of a block or function).  For a dynamically
//		allocated object, a reference-counting pointer type is used (instead of garbage
//		collection), to ensure the object is destructed immediately when it is no longer
//		referred to.  The reference-counted pointer type is itself an RAII class, as it acquires
//		and releases references in its contructors/destructors, and on assignment.  This approach
//		allows the dependency relationships between objects to implicity facilitate the management of
//		resources.

// A reference-counted pointer class.
// May refer to a non-reference-counted pointer, or NULL.
// If type is dynamically allocated, reference-counting is done when
// copied by value.  To avoid redundant reference counting of
// function args, use: const rcptr<type>&
// Note: The const& allows construction of a temporary, if necessary.
// Only receive a rcptr<type> as an argument if the object may
// be retained after the call has returned.  Otherwise, use a type*
template <typename type>
class rcptr;

// Like a rcptr<type>, but cannot point to NULL.
// Choose between rcptr<type> and rcref<type> in the same way you
// would choose between type* and type&.
// To avoid redundant reference counting of function args,
// use: const rcref<type>&
// Note: The const& allows construction of a temporary, if necessary.
// Only receive a rcref<type> as an argument if the object may
// be retained after the call has returned.  Otherwise, use a type*
template <typename type>
class rcref;

template <typename type>
class weak_rcptr;

// Note: rcptr<>, rcref<>, and weak_rcptr<> are all single arg templates.  This is to make them
// similar to ptr<>, and compatible with: template <typename> class ref_type
// when used as a template arg.


template <typename type_in, reference_strength referenceStrength = reference_strength::strong>
class rc_container;


template <typename type>
struct rc_container_content_t
{
	type* m_obj;
	rc_obj_base* m_desc;

	bool operator==(const rc_container_content_t& t) const { return (t.m_desc == m_desc) && (t.m_obj == m_obj); }

	bool operator!=(const rc_container_content_t& t) const { return !operator==(t); }
};

template <typename type_in, reference_strength referenceStrength>
class rc_container
{
public:
	typedef rc_container<type_in, referenceStrength> this_t;
	typedef type_in type;

	template <typename type2, reference_strength referenceStrength2>
	friend class rc_container;

	~rc_container() { release_inner(); }

private:
	typedef rc_container_content_t<type> content_t;
	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token read_token;
	typedef typename transactable_t::write_token write_token;

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

	// Helper function for exchange() and compare_exchange()

	template <typename type2, reference_strength referenceStrength2>
	static void set_return(rc_container<type2, referenceStrength2>& rtn, const this_t& src) { rtn = src; }

	template <typename type2, reference_strength referenceStrength2>
	static void set_return(volatile rc_container<type2, referenceStrength2>& rtn, const this_t& src) { rtn = src; }

	template <typename type2>
	static void set_return(ptr<type2>& rtn, const this_t& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(volatile ptr<type2>& rtn, const this_t& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(ref<type2>& rtn, const this_t& src) { rtn.get_ptr_ref() = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(volatile ref<type2>& rtn, const this_t& src) { atomic::store(rtn.get_ptr_ref(), src.m_contents->m_obj); }


	template <typename type2>
	static void set_return(type2*& rtn, const this_t& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(type2* volatile& rtn, const this_t& src) { atomic::store(rtn, src.m_contents->m_obj); }


	template <typename type2, reference_strength referenceStrength2>
	static void set_return(rc_container<type2, referenceStrength2>& rtn, const volatile this_t& src) { rtn = src; }

	template <typename type2, reference_strength referenceStrength2>
	static void set_return(volatile rc_container<type2, referenceStrength2>& rtn, const volatile this_t& src) { rtn = src; }

	template <typename type2>
	static void set_return(ptr<type2>& rtn, const volatile this_t& src) { rtn = src.begin_read()->m_obj; }

	template <typename type2>
	static void set_return(volatile ptr<type2>& rtn, const volatile this_t& src) { rtn = src.begin_read()->m_obj; }

	template <typename type2>
	static void set_return(ref<type2>& rtn, const volatile this_t& src) { rtn.get_ptr_ref() = src.begin_read()->m_obj; }

	template <typename type2>
	static void set_return(volatile ref<type2>& rtn, const volatile this_t& src) { atomic::store(rtn.get_ptr_ref(), src.m_contents->m_obj); }

	template <typename type2>
	static void set_return(type2*& rtn, const volatile this_t& src) { rtn = src.begin_read()->m_obj; }

	template <typename type2>
	static void set_return(type2* volatile& rtn, const volatile this_t& src) { atomic::store(rtn, src.begin_read()->m_obj); }

	template <typename type2, reference_strength referenceStrength2>
	static void set_return(rc_container<type2, referenceStrength2>& rtn, this_t&& src) { rtn = std::move(src); }

	template <typename type2, reference_strength referenceStrength2>
	static void set_return(volatile rc_container<type2, referenceStrength2>& rtn, this_t&& src) { rtn = std::move(src); }

	template <typename type2>
	static void set_return(ptr<type2>& rtn, this_t&& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(volatile ptr<type2>& rtn, this_t&& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(ref<type2>& rtn, this_t&& src) { rtn.get_ptr_ref() = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(volatile ref<type2>& rtn, this_t&& src) { atomic::store(rtn.get_ptr_ref(), src.m_contents->m_obj); }

	template <typename type2>
	static void set_return(type2*& rtn, this_t&& src) { rtn = src.m_contents->m_obj; }

	template <typename type2>
	static void set_return(type2* volatile& rtn, this_t&& src) { atomic::store(rtn, src.m_contents->m_obj); }


	//template <typename type2, reference_strength referenceStrength2>
	//rc_container<type2, referenceStrength2> get_as(rc_container<type2, referenceStrength2>&)
	//{ rc_container<type2, referenceStrength2> rtn(*this); return rtn; }

	//template <typename type2, reference_strength referenceStrength2>
	//rc_container<type2, referenceStrength2> get_as(volatile rc_container<type2, referenceStrength2>&)
	//{ rc_container<type2, referenceStrength2> rtn(*this); return rtn; }

	//template <typename type2>
	//ptr<type2> get_as(ptr<type2>&) { ptr<type2> rtn(src.m_contents->m_obj); return rtn; }

	//template <typename type2>
	//ptr<type2> get_as(volatile ptr<type2>&) { ptr<type2> rtn(src.m_contents->m_obj); return rtn; }

	//template <typename type2>
	//type2* get_as(type2*&) { type2* rtn(src.m_contents->m_obj); return rtn; }

	//template <typename type2>
	//type2* get_as(type2* volatile&) { type2* rtn(src.m_contents->m_obj); return rtn; }


	template <typename type2, reference_strength referenceStrength2>
	static content_t make_content(const rc_container<type2, referenceStrength2>& src)
	{ content_t rtn { src.m_contents->m_obj, src.m_contents->m_desc }; return rtn; }

	template <typename type2, reference_strength referenceStrength2>
	static content_t make_content(const volatile rc_container<type2, referenceStrength2>& src)
	{ auto rt = src.begin_read(); content_t rtn { rt->m_obj, rt->m_m_desc }; return rtn; }

	template <typename type2>
	static content_t make_content(const ptr<type2>& src) { content_t rtn{ src.get_ptr(), nullptr }; return rtn; }

	template <typename type2>
	static content_t make_content(const volatile ptr<type2>& src) { content_t rtn{ src.get_ptr(), nullptr }; return rtn; }

	template <typename type2>
	static content_t make_content(const ref<type2>& src) { content_t rtn{ src.get_ptr(), nullptr }; return rtn; }

	template <typename type2>
	static content_t make_content(const volatile ref<type2>& src) { content_t rtn{ src.get_ptr(), nullptr }; return rtn; }

	template <typename type2>
	static content_t make_content(type2* const& src) { content_t rtn{ src, nullptr }; return rtn; }

	template <typename type2>
	static content_t make_content(type2* const volatile& src) { content_t rtn{ atomic::load(src), nullptr }; return rtn; }


	template <typename type2, reference_strength referenceStrength2>
	static bool compare_content(const content_t& c, const rc_container<type2, referenceStrength2>& cmp)
	{
		return (c.m_obj == cmp.m_contents->m_obj) && (c.m_desc == cmp.m_contents->m_desc);
	}

	template <typename type2, reference_strength referenceStrength2>
	static bool compare_content(const content_t& c, const volatile rc_container<type2, referenceStrength2>& src)
	{
		auto rt = src.begin_read();
		return (c.m_obj == rt->m_obj) && (c.m_desc == rt->m_desc);
	}

	template <typename type2>
	static bool compare_content(const content_t& c, const ptr<type2>& src) { return c.m_obj == src.get_ptr(); }

	template <typename type2>
	static bool compare_content(const content_t& c, const volatile ptr<type2>& src) { return c.m_obj == src.get_ptr(); }

	template <typename type2>
	static bool compare_content(const content_t& c, const ref<type2>& src) { return c.m_obj == src.get_ptr(); }

	template <typename type2>
	static bool compare_content(const content_t& c, const volatile ref<type2>& src) { return c.m_obj == src.get_ptr(); }

	template <typename type2>
	static bool compare_content(const content_t& c, type2* const& src) { return c.m_obj == src; }

	template <typename type2>
	static bool compare_content(const content_t& c, type2* const volatile& src) { return c.m_obj == atomic::load(src); }


public:
	rc_obj_base* disown() // clear the desc ptr not the desc contents
	{
		rc_obj_base* desc = m_contents->m_desc;
		m_contents->m_desc = nullptr;
		return desc;
	}

	rc_obj_base* disown() volatile
	{
		rc_obj_base* desc;
		write_token wt;
		do {
			begin_write(wt);
			desc = wt->m_desc;
			if (desc == nullptr)
				break;
			wt->m_desc = nullptr;
		} while (!end_write(wt));
		return desc;
	}

	rc_obj_base* get_desc() const { return m_contents->m_desc; }
	rc_obj_base* get_desc() const volatile { return begin_read()->m_desc; }

	void clear()
	{
		m_contents->m_obj = 0;
		m_contents->m_desc = 0; // clear the desc ptr not the desc contents
	}

	void clear() volatile { m_contents.set(); }

	void set(const ptr<type>& obj)
	{
		m_contents->m_obj = obj.get_ptr();
		m_contents->m_desc = 0; // clear the desc ptr not the desc contents
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
		if (!!oldDesc)
			oldDesc->release(referenceStrength);
	}

	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile
	{
		content_t tmp{ obj.get_ptr(), desc.get_ptr() };
		m_contents.swap_contents(tmp);
	}


	bool release_inner()
	{
		if (!!m_contents->m_desc)
			return m_contents->m_desc->release(referenceStrength);
		return false;
	}

	bool acquire_inner()
	{
		if (!!m_contents->m_desc && !m_contents->m_desc->acquire(referenceStrength))
		{
			clear();
			return false;
		}
		return true;
	}

	template <reference_strength referenceStrength2 = referenceStrength>
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
			p.bind_unacquired(h, descPtr);
			if (!m_contents.is_current(rt) || !p.validate())
				continue;
			acquired = descPtr->acquire(referenceStrength2);
			if (p.release())
				descPtr->dispose();
			if (!acquired)
			{
				// Should be able to acquire weak from any, or strong from strong
				// This implies that any time we're using guarded_acquire with this_t, it will always acquire.
				if constexpr ((referenceStrength2 == reference_strength::weak) || (referenceStrength == reference_strength::strong))
					continue;
			}
			break;
		}
		return acquired;
	}

	bool guarded_begin_read(read_token& rt) const volatile // returns false if it's a released weak reference.  May be treated as NULL.
	{
		bool result = true;
		if constexpr (referenceStrength == reference_strength::strong)
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
				p.bind_unacquired(h, descPtr);
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

	rc_container()
	{
		m_contents->m_obj = nullptr;
		m_contents->m_desc = nullptr;
	}

	rc_container(this_t&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_obj = nullptr;
		src.m_contents->m_desc = nullptr;
	}

	rc_container(const this_t& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj;
		m_contents->m_desc = src.m_contents->m_desc;
		acquire_inner();
	}

	rc_container(const volatile this_t& src)
	{
		read_token rt;
		if (!!src.template guarded_acquire<referenceStrength>(rt))
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
	rc_container(rc_container<type2, referenceStrength>&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
	}

	template <typename type2, reference_strength referenceStrength2>
	rc_container(const rc_container<type2, referenceStrength2>& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = src.m_contents->m_desc;
		acquire_inner();
	}

	template <typename type2, reference_strength referenceStrength2>
	rc_container(const volatile rc_container<type2, referenceStrength2>& src)
	{
		typename rc_container<type2, referenceStrength2>::read_token rt;
		if (!!src.template guarded_acquire<referenceStrength>(rt))
		{
			COGS_ASSERT(rt->m_obj || !rt->m_desc);
			m_contents->m_obj = rt->m_obj; // <- A failure here means type conversion (caller) error.
			m_contents->m_desc = rt->m_desc;
			//acquire_inner();

		}
		else
		{
			m_contents->m_obj = nullptr;
			m_contents->m_desc = nullptr;
		}
	}

	template <typename type2>
	explicit rc_container(const ref<type2>& obj)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	explicit rc_container(const ptr<type2>& obj)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	explicit rc_container(type2* const& obj)
	{
		m_contents->m_obj = obj; // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	explicit rc_container(const volatile ref<type2>& obj)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	explicit rc_container(const volatile ptr<type2>& obj)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	explicit rc_container(type2* const volatile& obj)
	{
		m_contents->m_obj = atomic::load(obj); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = nullptr;
	}

	template <typename type2>
	rc_container(const ref<type2>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	template <typename type2>
	rc_container(const ptr<type2>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	template <typename type2>
	rc_container(type2* const& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj; // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	template <typename type2>
	rc_container(const volatile ref<type2>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	template <typename type2>
	rc_container(const volatile ptr<type2>& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = obj.get_ptr(); // <- A failure here means type conversion (caller) error.
		m_contents->m_desc = desc.get_ptr();
		COGS_ASSERT(m_contents->m_obj || !m_contents->m_desc);
	}

	template <typename type2>
	rc_container(type2* const volatile& obj, const ptr<rc_obj_base>& desc)
	{
		m_contents->m_obj = atomic::load(obj); // <- A failure here means type conversion (caller) error.
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
				m_contents->m_obj = src.m_contents->m_obj; // if desc are the same, no need to acquire/release, just copy obj
			else
			{
				rc_obj_base* oldDesc = m_contents->m_desc; // to release later (in case it's the same one we haven't acquired yet)
				if (!src.m_contents->m_desc) // If no new desc, just copy obj and clear desc.
				{
					m_contents->m_obj = src.m_contents->m_obj;
					m_contents->m_desc = nullptr;
				}
				else if (!!src.m_contents->m_desc->acquire(referenceStrength))
				{
					COGS_ASSERT(!!src.m_contents->m_obj);
					m_contents->m_obj = src.m_contents->m_obj;
					m_contents->m_desc = src.m_contents->m_desc;
					//acquire_inner();
				}
				else
					clear();// failure to acquire

				if (!!oldDesc)
					oldDesc->release(referenceStrength);
			}
		}
		return *this;
	}

	this_t& operator=(const volatile this_t& src)
	{
		if (this != &src)
		{
			rc_obj_base* oldDesc = m_contents->m_desc; // to release later (in case it's the same one we haven't acquired yet)

			read_token rt;
			if (!!src.template guarded_acquire<referenceStrength>(rt))
			{
				COGS_ASSERT(rt->m_obj || !rt->m_desc);
				m_contents->m_obj = rt->m_obj;
				m_contents->m_desc = rt->m_desc;
				//acquire_inner();
			}
			else
				clear();

			if (!!oldDesc)
				oldDesc->release(referenceStrength);
		}
		return *this;
	}

	volatile this_t& operator=(const this_t& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		if (this != &src)
		{
			if (!!src.m_contents->m_desc && !src.m_contents->m_desc->acquire(referenceStrength)) // couldn't acquire, treat as null
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
	this_t& operator=(rc_container<type2, referenceStrength>&& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		release_inner();
		m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (user) error.
		m_contents->m_desc = src.m_contents->m_desc;
		src.m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(rc_container<type2, referenceStrength>&& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		swap(src);
		src.release_inner();
		src.m_contents->m_desc = nullptr;
		return *this;
	}


	template <typename type2, reference_strength referenceStrength2>
	this_t& operator=(const rc_container<type2, referenceStrength2>& src)
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		if (m_contents->m_desc == src.m_contents->m_desc) // If desc are the same, no need to acquire/release, just copy obj
			m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (user) error.
		else
		{
			rc_obj_base* oldDesc = m_contents->m_desc; // to release later (in case it's the same one we haven't acquired yet)
			if (!src.m_contents->m_desc) // If no new desc, just copy obj and clear desc.
			{
				m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (user) error.
				m_contents->m_desc = nullptr;
			}
			else if (!src.m_contents->m_desc->acquire(referenceStrength)) // if failure to acquire
				clear();
			else // acquired desc, copy contents
			{
				m_contents->m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (user) error.
				m_contents->m_desc = src.m_contents->m_desc;
			}

			if (!!oldDesc)
				oldDesc->release(referenceStrength);
		}
		return *this;
	}

	template <typename type2, reference_strength referenceStrength2>
	this_t& operator=(const volatile rc_container<type2, referenceStrength2>& src)
	{
		rc_obj_base* oldDesc = m_contents->m_desc; // to release later (in case it's the same one we haven't acquired yet)

		typename rc_container<type2, referenceStrength2>::read_token rt;
		if (!src.template guarded_acquire<referenceStrength>(rt))
			clear();
		else
		{
			COGS_ASSERT(rt->m_obj || !rt->m_desc);
			m_contents->m_obj = rt->m_obj; // <- A failure here means type conversion (user) error.
			m_contents->m_desc = rt->m_desc;
		}

		if (!!oldDesc)
			oldDesc->release(referenceStrength);

		return *this;
	}

	template <typename type2, reference_strength referenceStrength2>
	volatile this_t& operator=(const rc_container<type2, referenceStrength2>& src) volatile
	{
		COGS_ASSERT(src.m_contents->m_obj || !src.m_contents->m_desc);
		typename rc_container<type2, referenceStrength2>::read_token rt;
		if (!src.template guarded_acquire<referenceStrength>(rt))
			release();
		else
		{
			this_t tmp; // need a matching content_t to atomically swap
			tmp.m_contents->m_obj = rt->m_obj; // <- A failure here means type conversion (user) error.
			tmp.m_contents->m_desc = rt->m_desc;
			m_contents.swap(tmp.m_contents);
		}
		return *this;
	}

	template <typename type2>
	this_t& operator=(const ref<type2>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr(); // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(const ptr<type2>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr(); // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(type2* const& src)
	{
		release_inner();
		m_contents->m_obj = src; // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(const volatile ref<type2>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr(); // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(const volatile ptr<type2>& src)
	{
		release_inner();
		m_contents->m_obj = src.get_ptr(); // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}

	template <typename type2>
	this_t& operator=(type2* const volatile& src)
	{
		release_inner();
		m_contents->m_obj = atomic::load(src); // <- A failure here means type conversion (user) error.
		m_contents->m_desc = nullptr;
		return *this;
	}


	template <typename type2>
	volatile this_t& operator=(const ref<type2>& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(const ptr<type2>& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(type2* const& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(const volatile ref<type2>& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(const volatile ptr<type2>& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}

	template <typename type2>
	volatile this_t& operator=(type2* const volatile& src) volatile
	{
		this_t tmp(src); // need a matching content_t to atomically swap
		m_contents.swap(tmp.m_contents);
		return *this;
	}


	//template <typename type2, reference_strength referenceStrength2>
	//static content_t make_content(const rc_container<type2, referenceStrength2>& src)
	//{
	//	content_t c;
	//	c.m_obj = src.m_contents->m_obj; // <- A failure here means type conversion (caller) error.
	//	c.m_desc = src.m_contents->m_desc;
	//	return c;
	//}

	//template <typename type2, reference_strength referenceStrength2>
	//static content_t make_content(const volatile rc_container<type2, referenceStrength2>& src)
	//{
	//	content_t c;
	//	read_token rt = src.m_contents->begin_read();
	//	c.m_obj = rt->m_obj; // <- A failure here means type conversion (caller) error.
	//	c.m_desc = rt->m_desc;
	//	return c;
	//}

	//template <typename type2>
	//static content_t make_content(const ptr<type2>& src)
	//{
	//	content_t c;
	//	c.m_obj = src.get_ptr(); // <- A failure here means type conversion (caller) error.
	//	c.m_desc = nullptr;
	//	return c;
	//}


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
		if constexpr (referenceStrength != reference_strength::strong)
		{
			if (!!m_contents->m_desc && m_contents->m_desc->is_released()) // No need to acquire a guard.  A weak ref retains the alloc, if not the object.
				return nullptr;
		}
		return m_contents->m_obj;
	}

	// Not safe to call on a weak reference unless you know it's still in scope somewhere.
	// Otherwise, the return value may be invalidated before use.
	type* get_obj() const volatile
	{
		read_token rt;
		return guarded_begin_read(rt) ? rt->m_obj : nullptr;
	}

	type* peek_obj() const { return m_contents->m_obj; }

	type* peek_obj() const volatile { return get_obj(); }

	static size_t mark_bits() { return ptr<type>::mark_bits(); }
	static size_t mark_mask() { return ptr<type>::mark_mask(); }

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

	void clear_to_mark() { ptr<type> p = m_contents->m_obj; p.clear_to_mark(); m_contents->m_obj = p.get_ptr(); }
	void clear_to_mark() volatile
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

			p.clear_to_mark();
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

			ptr<type> p = wt->m_obj;
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
		this_t tmp; // need a matching content_t to atomically swap
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
		this_t tmp; // need a matching content_t to atomically swap
		tmp.set_marked(p, mark);
		m_contents.swap(tmp.m_contents);
	}


	bool is_empty() const { return !get_obj(); }
	bool is_empty() const volatile { return !get_obj(); }

	bool operator!() const { return !get_obj(); }
	bool operator!() const volatile { return !get_obj(); }

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

	template <typename type2, reference_strength referenceStrength2>
	bool operator==(const rc_container<type2, referenceStrength2>& cmp) const
	{
		return (m_contents->m_obj == cmp.m_contents->m_obj) && (m_contents->m_desc == cmp.m_contents->m_desc);
	}

	template <typename type2, reference_strength referenceStrength2>
	bool operator==(const rc_container<type2, referenceStrength2>& cmp) const volatile
	{
		bool result = true;
		read_token rt;
		begin_read(rt);
		result = (rt->m_obj == cmp.m_contents->m_obj) && (rt->m_desc == cmp.m_contents->m_desc);
		return result;
	}

	template <typename type2, reference_strength referenceStrength2>
	bool operator==(const volatile rc_container<type2, referenceStrength2>& cmp) const
	{ return cmp.operator==(*this); }

	bool operator!=(const this_t& cmp) const
	{ return !operator==(cmp); }

	template <typename type2, reference_strength referenceStrength2>
	bool operator!=(const rc_container<type2, referenceStrength2>& cmp) const
	{ return !operator==(cmp); }

	bool operator!=(const this_t& cmp) const volatile
	{ return !operator==(cmp); }

	template <typename type2, reference_strength referenceStrength2>
	bool operator!=(const rc_container<type2, referenceStrength2>& cmp) const volatile
	{ return !operator==(cmp); }

	bool operator!=(const volatile this_t& cmp) const
	{ return !operator==(cmp); }

	template <typename type2, reference_strength referenceStrength2>
	bool operator!=(const volatile rc_container<type2, referenceStrength2>& cmp) const
	{ return !operator==(cmp); }

	template <typename type2> bool operator==(const ptr<type2>& src) const { return src == get_obj(); }
	template <typename type2> bool operator==(const ptr<type2>& src) const volatile { return src == get_obj(); }
	template <typename type2> bool operator!=(const ptr<type2>& src) const { return src != get_obj(); }
	template <typename type2> bool operator!=(const ptr<type2>& src) const volatile { return src != get_obj(); }

	template <typename type2> bool operator==(const ref<type2>& src) const { return src == get_obj(); }
	template <typename type2> bool operator==(const ref<type2>& src) const volatile { return src == get_obj(); }
	template <typename type2> bool operator!=(const ref<type2>& src) const { return src != get_obj(); }
	template <typename type2> bool operator!=(const ref<type2>& src) const volatile { return src != get_obj(); }

	template <typename type2> bool operator==(const volatile ptr<type2>& src) const { return src == get_obj(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& src) const volatile { return src == get_obj(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& src) const { return src != get_obj(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& src) const volatile { return src != get_obj(); }

	template <typename type2> bool operator==(const volatile ref<type2>& src) const { return src == get_obj(); }
	template <typename type2> bool operator==(const volatile ref<type2>& src) const volatile { return src == get_obj(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& src) const { return src != get_obj(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& src) const volatile { return src != get_obj(); }

	template <typename type2> bool operator==(type2* const& src) const { return src == get_obj(); }
	template <typename type2> bool operator==(type2* const& src) const volatile { return src == get_obj(); }
	template <typename type2> bool operator!=(type2* const& src) const { return src != get_obj(); }
	template <typename type2> bool operator!=(type2* const& src) const volatile { return src != get_obj(); }

	template <typename type2> bool operator==(type2* const volatile& src) const { return atomic::load(src) == get_obj(); }
	template <typename type2> bool operator==(type2* const volatile& src) const volatile { return atomic::load(src) == get_obj(); }
	template <typename type2> bool operator!=(type2* const volatile& src) const { return atomic::load(src) != get_obj(); }
	template <typename type2> bool operator!=(type2* const volatile& src) const volatile { return atomic::load(src) != get_obj(); }

	// swap

	// It's caller error (and nonsensical) to pass *this as wth.  The behavior of doing so is undefined.

	void swap(this_t& wth)
	{
		m_contents.swap(wth.m_contents);
	}

	template <typename type2, reference_strength referenceStrength2>
	void swap(rc_container<type2, referenceStrength2>& wth)
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = std::move(tmp);
	}

	template <typename type2>
	void swap(ptr<type2>& wth)
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}

	template <typename type2>
	void swap(ref<type2>& wth)
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}

	template <typename type2>
	void swap(type2*& wth)
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}


	template <typename type2, reference_strength referenceStrength2>
	void swap(volatile rc_container<type2, referenceStrength2>& wth)
	{
		wth.swap(*this);
	}

	template <typename type2>
	void swap(volatile ptr<type2>& wth)
	{
		ptr<type2> tmp = m_contents->m_obj;
		cogs::swap(wth, tmp);
		*this = tmp;
	}

	template <typename type2>
	void swap(volatile ref<type2>& wth)
	{
		ref<type2> tmp;
		tmp.get_ptr_ref() = m_contents->m_obj;
		cogs::swap(wth, tmp);
		*this = tmp;
	}

	template <typename type2>
	void swap(type2* volatile& wth)
	{
		type2* tmp = m_contents->m_obj;
		cogs::swap(wth, tmp);
		*this = tmp;
	}


	void swap(this_t& wth) volatile
	{
		m_contents.swap(wth.m_contents);
	}

	template <typename type2, reference_strength referenceStrength2>
	void swap(rc_container<type2, referenceStrength2>& wth) volatile
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = std::move(tmp);
	}

	template <typename type2>
	void swap(ptr<type2>& wth) volatile
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}

	template <typename type2>
	void swap(ref<type2>& wth) volatile
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}

	template <typename type2>
	void swap(type2*& wth) volatile
	{
		this_t tmp(wth);
		m_contents.swap(tmp.m_contents);
		wth = tmp->m_obj;
	}


	// exchange

	// It's caller error (and nonsensical) to pass *this as src.  The behavior of doing so is undefined.

	template <typename T2>
	this_t exchange(T2&& src)
	{
		this_t tmp(std::forward<T2>(src));
		m_contents.swap(tmp.m_contents);
		return tmp;
	}

	template <typename T2>
	this_t exchange(T2&& src) volatile
	{
		this_t tmp(std::forward<T2>(src));
		m_contents.swap(tmp.m_contents);
		return tmp;
	}


	// If src and rtn are references to the same volatile instance, it will not be both read and written in the same atomic operation.
	// It's caller error (and nonsensical) to pass *this as rtn, or src.  The behavior of doing so is undefined.

	template <typename T2, typename T3>
	void exchange(T2&& src, T3& rtn)
	{
		this_t tmp(std::forward<T2>(src));
		m_contents.swap(tmp.m_contents);
		set_return(rtn, std::move(tmp));
	}

	template <typename T2, typename T3>
	void exchange(T2&& src, T3& rtn) volatile
	{
		this_t tmp(std::forward<T2>(src));
		m_contents.swap(tmp.m_contents);
		set_return(rtn, std::move(tmp));
	}


	// compare_exchange

	// It's caller error (and nonsensical) to pass *this as src or cmp, or to pass the same instance for both src and cmp.  The behavior of doing so is undefined.

	template <typename T2, typename T3>
	bool compare_exchange(T2&& src, T3&& cmp)
	{
		bool b = (*this == std::forward<T3>(cmp));
		if (b)
			*this = std::forward<T2>(src);
		return b;
	}

	template <typename T2, typename T3>
	bool compare_exchange(T2&& src, T3&& cmp) volatile
	{
		this_t tmpSrc(std::forward<T2>(src));
		content_t tmpCmp = make_content(cmp);
		content_t tmpRtn;
		bool b = m_contents.compare_exchange_contents(*tmpSrc.m_contents, tmpCmp, tmpRtn);
		if (b)
		{
			tmpSrc.disown(); // tmpSrc has been swapped into this, so tmpSrc needs to disown
			if (!!tmpRtn.m_desc) // release the reference we just removed from this
				tmpRtn.m_desc->release(referenceStrength);
		}
		return b;
	}


	// If src and rtn are references to the same volatile instance, it will not be both read and written in the same atomic operation.
	// If cmp and rtn are references to the same volatile instance, it will not be both read and written in the same atomic operation.
	// It's caller error (and nonsensical) to pass *this as src, cmp, or rtn, or to pass the same instance for both src and cmp.  The behavior of doing so is undefined.

	template <typename T2, typename T3, typename T4>
	bool compare_exchange(T2&& src, T3&& cmp, T4& rtn)
	{
		bool b = (*this == cmp);
		if (!b)
		{
			this_t tmp(std::forward<T2>(src));
			m_contents.swap(tmp.m_contents);
			set_return(rtn, std::move(tmp));
		}
		return b;
	}

	template <typename T2, typename T3, typename T4>
	bool compare_exchange(T2&& src, T3&& cmp, T4& rtn) volatile
	{
		this_t tmpSrc(std::forward<T2>(src));
		read_token rt;
		this_t tmp; // contain, to handle hand-off of ownership.  or not.
		bool result;
		for (;;)
		{
			guarded_acquire(rt);
			*tmp.m_contents = *rt;

			if (!compare_content(*rt, cmp))
			{
				result = false;
				break;
			}

			if (end_write(rt, *tmpSrc.m_contents))
			{
				tmpSrc.disown(); // tmpSrc has been swapped into this, so tmpSrc needs to disown
				tmp.release_inner(); // release the reference we just removed from this
				result = true;
				break;
			}
			tmp.release_inner();
		}
		set_return(rtn, std::move(tmp));
		return result;
	}

private:
	template <typename type2, bool unused = true>
	class on_released_helper
	{
	public:
		template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<bool, F, type&, rc_obj_base&, bool> > >
		static void on_released2(F&& f, const this_t& rcb)
		{
			rc_obj_base* desc = rcb.get_desc();
			if (!!desc)
			{
				desc->on_released([f, obj{ rcb.get_obj() }](rc_obj_base& desc, bool releaseNow)
				{
					return f(*obj, desc, releaseNow);
				});
			}
		}
	};

	template <bool unused>
	class on_released_helper<void, unused>
	{
	public:
		template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<bool, F, rc_obj_base&, bool> > >
		static void on_released2(F&& f, const this_t& rcb)
		{
			rc_obj_base* desc = rcb.get_desc();
			if (!!desc)
				desc->on_released(std::move(f));
		}
	};

public:
	template <typename F>
	void on_released(F&& f) const
	{
		on_released_helper<type> ::on_released2(std::move(f), *this);
	}
};


}


#include "cogs/mem/weak_rcptr.hpp"


#endif
