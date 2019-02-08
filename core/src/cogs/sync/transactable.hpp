//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_TRANSACT
#define COGS_TRANSACT

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/atomic_exchange.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified

template <typename T>
class transactable;


template <typename T>
class thread_safe_transactable
{
public:
	typedef thread_safe_transactable<T> this_t;
	typedef T type;

private:
	static_assert(std::is_copy_constructible_v<T>);
	static_assert(std::is_destructible_v<T>);

	template <typename>
	friend class transactable;

	class construct_embedded_t { };
	class construct_embedded_volatile_t { };
	class unconstructed_embedded_t { };
	class unconstructed_embedded_volatile_t { };

	typedef rc_obj<type> descriptor_t;

	ptr<descriptor_t>			m_desc;
	typename placement<type>	m_embedded;

#if COGS_DEBUG_TRANSACTABLE
	type*						m_embeddedPtrDebug;
#endif

	// Normally we require const-correctness, so we don't need volatility just for read access to memory.
	// But it's OK to make an exception for something that is always volatile.
	mutable alignas (atomic::get_alignment_v<size_t>) size_t m_embeddedRefCount;

	      type& get_embedded()			{ return m_embedded.get(); }
	const type& get_embedded() const	{ return m_embedded.get(); }

	// Normally we require const-correctness, so we don't need volatility just for read access to memory.
	// But it's OK to cast away the volatility of something that will not be written to.
	// When object is promoted to volatile, all legal concurrent access must be volatile.  Embedded contents
	// will not be changed through volatile access.
	const type&	get_embedded() const volatile	{ return *const_cast<const type*>(&m_embedded.get()); }

	template <typename... args_t>
	static descriptor_t* allocate(args_t&&... src)
	{
		ref<descriptor_t> desc = descriptor_t::allocate();
		type* ptr = desc->get_obj();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		desc->set_type_name(typeid(type).name());
		desc->set_debug_str("thread_safe_transactable");
		desc->set_obj_ptr(ptr);
#endif

#if COGS_DEBUG_RC_LOGGING
		unsigned long rcCount = pre_assign_next(g_rcLogCount);
		printf("(%lu) RC_NEW(transactable): %p (desc) %p (ptr) %s\n", rcCount, (rc_obj_base*)(desc.get_ptr()), ptr, typeid(descriptor_t).name());
#endif

		new (ptr) type(std::forward<args_t>(src)...);
		return desc.get_ptr();
	}

	static descriptor_t* allocate_unconstructed()
	{
		ref<descriptor_t> desc = descriptor_t::allocate();
		type* ptr = desc->get_obj();

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		desc->set_type_name(typeid(type).name());
		desc->set_debug_str("thread_safe_transactable");
		desc->set_obj_ptr(ptr);
#endif

#if COGS_DEBUG_RC_LOGGING
		unsigned long rcCount = pre_assign_next(g_rcLogCount);
		printf("(%lu) RC_NEW(transactable): %p (desc) %p (ptr) %s\n", rcCount, (rc_obj_base*)(desc.get_ptr()), ptr, typeid(descriptor_t).name());
#endif

		return desc.get_ptr();
	}

	void acquire_embedded() const volatile
	{
		assign_next(m_embeddedRefCount);
	}

	void release_embedded() const volatile
	{
		size_t curCount = atomic::load(m_embeddedRefCount);
		for (;;)
		{
			if (curCount == 0)
			{
				bool b = atomic::compare_exchange(m_embeddedRefCount, (size_t)1, (size_t)0, curCount);
				if (b)
				{
					get_embedded().~type();
					break;
				}
			}
			else if (atomic::compare_exchange(m_embeddedRefCount, curCount - 1, curCount, curCount))
				break;
		}	
	}

	void shed_embedded()
	{
		if (!m_desc)
		{
			m_desc = allocate(get_embedded());
			release_embedded();
		}
	}

	void shed_embedded() volatile
	{
		if (!m_desc)
		{
			acquire_embedded();
			descriptor_t* newDesc = allocate(get_embedded());
			if (!m_desc.compare_exchange(newDesc, 0))
				newDesc->release();
			else
				release_embedded();
			release_embedded();
		}
	}

	template <typename... args_t>
	thread_safe_transactable(const construct_embedded_t&, args_t&&... src)
		: m_desc(0), m_embeddedRefCount(0) 
	{
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
		new (&get_embedded()) type(std::forward<args_t>(src)...);
	}

	template <typename... args_t>
	thread_safe_transactable(const construct_embedded_volatile_t&, args_t&&... src)	// Allocates block, does not initialize embedded
	{
		m_desc = allocate(std::forward<args_t>(src)...);
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug = 0;
#endif
		m_embeddedRefCount = 1;
	}

	thread_safe_transactable(const unconstructed_embedded_t&)	// Allocates block, does not initialize embedded
		: m_desc(0)
	{
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug = &get_embedded();
#endif
		m_embeddedRefCount = 0;
	}

	thread_safe_transactable(const unconstructed_embedded_volatile_t&)	// Allocates block, does not initialize embedded
	{
		m_desc = allocate_unconstructed();
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug = 0;
#endif
		m_embeddedRefCount = 1;
	}
	template <typename... args_t>
	thread_safe_transactable(descriptor_t* srcDesc, args_t&&... src)	// takes ownership of srcDesc if not null
		: m_desc(srcDesc)
	{
		if (!!srcDesc)
		{
#if COGS_DEBUG_TRANSACTABLE
			m_embeddedPtrDebug = 0;
#endif
			m_embeddedRefCount = 1;
		}
		else
		{
#if COGS_DEBUG_TRANSACTABLE
			m_embeddedPtrDebug =
#endif
				new (&get_embedded()) type(std::forward<args_t>(src)...);
			m_embeddedRefCount = 0;
		}
	}

public:
	class read_token
	{	
	protected:
		ptr<descriptor_t>		m_compare;
		ptr<descriptor_t>		m_read;

		friend class thread_safe_transactable;

		explicit read_token(const ptr<descriptor_t>& compare, const ptr<descriptor_t>& read)	// read must already be acquired
			:	m_compare(compare), m_read(read)
		{ }

		void acquire_inner()
		{
			if (!!m_read)
				m_read->acquire();
		}
		
		void release_inner()
		{
			if (!!m_read)
				m_read->release();
		}

		void set(const ptr<descriptor_t>& compare, const ptr<descriptor_t>& read)	// read must already be acquired
		{
			release_inner();
			m_compare = compare;
			m_read = read;
		}

	public:
		read_token()
		{ }

		read_token(read_token& src) = delete;
		read_token(const read_token& src) = delete;
		read_token& operator=(read_token& src) = delete;
		read_token& operator=(const read_token& src) = delete;

		read_token(read_token&& src)
			: m_compare(src.m_compare),
			m_read(src.m_read)
		{
			src.m_read.release();
		}

		read_token& operator=(read_token&& src)
		{
			release_inner();
			m_read = src.m_read;
			if (!!m_read)
			{
				m_compare = src.m_compare;
				src.m_read.release();
			}
			return *this;
		}

		void release()
		{
			if (!!m_read)
			{
				m_read->release();
				m_read.release();
			}
		}

		~read_token()
		{
			release_inner();
		}

		const type* get() const			{ return m_read->get_obj(); }
		const type& operator*() const	{ return *get(); }
		const type* operator->() const	{ return get(); }
	};

	class write_token
	{	
	protected:
		// Unlike read_token, the write_token will acquire m_compare.
		// This is to ensure there is a remaining reference to the original
		// contents for the duration of the write, to avoid an ABA Problem.

		ptr<descriptor_t>		m_compare;
		ptr<descriptor_t>		m_write;

		friend class thread_safe_transactable;

		explicit write_token(const ptr<descriptor_t>& compare, const ptr<descriptor_t>& write)	// both must already be acquired
			:	m_compare(compare),
				m_write(write)
		{ }

		void acquire_inner()
		{
			if (!!m_write)
			{
				m_write->acquire();
				if (!!m_compare)
					m_compare->acquire();
			}
		}

		void release_inner()
		{
			if (!!m_write)
			{
				m_write->release();
				if (!!m_compare)
					m_compare->release();
			}
		}

		void set(const ptr<descriptor_t>& compare, const ptr<descriptor_t>& write)	// both must already be acquired
		{
			release_inner();
			m_compare = compare;
			m_write = write;
		}

	public:
		write_token()
		{ }

		write_token(write_token& src) = delete;
		write_token(const write_token& src) = delete;
		write_token& operator=(write_token& src) = delete;
		write_token& operator=(const write_token& src) = delete;

		write_token(write_token&& src)
			: m_compare(src.m_compare),
			m_write(src.m_write)
		{
			src.m_compare.release();
			src.m_write.release();
		}

		write_token& operator=(write_token&& src)
		{
			release_inner();
			m_write = src.m_write;
			m_compare = src.m_compare;
			src.m_write.release();
			src.m_compare.release();
			return *this;
		}

		void release()
		{
			if (!!m_write)
			{
				m_write->release();
				m_write.release();
				if (!!m_compare)
				{
					m_compare->release();
					m_compare.release();
				}
			}
		}

		~write_token()
		{
			release_inner();
		}

		      type* get()				{ return m_write->get_obj(); }
		const type* get() const			{ return m_write->get_obj(); }
		      type& operator*()			{ return *get(); }
		const type& operator*() const	{ return *get(); }
		      type* operator->()		{ return get(); }
		const type* operator->() const	{ return get(); }
	};

	thread_safe_transactable()
		:	m_desc(0), m_embeddedRefCount(0)
	{
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
		new (&get_embedded()) type;
	}

	// Need superfluous non-const copy constructors to avoid this_t& matching T&& instead of const this_t&

	thread_safe_transactable(this_t& src)
		: m_desc(0), m_embeddedRefCount(0)
	{
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
			new (&get_embedded()) type(*src.get());
	}

	thread_safe_transactable(const this_t& src)
		: m_desc(0), m_embeddedRefCount(0)
	{
#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
			new (&get_embedded()) type(*src.get_contents());
	}

	thread_safe_transactable(volatile this_t& src)
	{
		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
		new (&get_embedded()) type(*srcPtr);

		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();

	}

	thread_safe_transactable(const volatile this_t& src)
	{
		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

#if COGS_DEBUG_TRANSACTABLE
		m_embeddedPtrDebug =
#endif
			new (&get_embedded()) type(*srcPtr);

		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();
	}

	thread_safe_transactable(this_t&& src)
		: m_desc(0), m_embeddedRefCount(0)
	{
		new (&get_embedded()) type(std::move(*src.get()));
	}


	~thread_safe_transactable()
	{
		if (!!m_desc)
			m_desc->release();
		else if (m_embeddedRefCount == 0)
			get_embedded().~type();
	}

	this_t& operator=(const this_t& src)
	{
		cogs::assign(*get(), *(src.get()));
		return *this;
	}

	this_t& operator=(const volatile this_t& src)
	{
		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

		cogs::assign(*get(), *srcPtr);

		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();

		return *this;
	}

	volatile this_t& operator=(const this_t& src) volatile
	{
		ptr<descriptor_t> newDesc = allocate(*src.get());
		ptr<descriptor_t> oldDesc = m_desc.exchange(newDesc);
		if (!!oldDesc)
			oldDesc->release();
		else
			release_embedded();
		return *this;
	}

	volatile this_t& operator=(const volatile this_t& src) volatile
	{
		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

		descriptor_t* newDesc = allocate(*srcPtr);
		descriptor_t* oldDesc = m_desc.exchange(newDesc);
		if (!!oldDesc)
			oldDesc->release();
		else
			release_embedded();

		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();

		return *this;
	}

	// Move constructors and move assignment operators will leave the original object in a state invalid for any futher use
	// other than destruction.

	this_t& operator=(this_t&& src)
	{
		*get() = std::move(*src.get());
		return *this;
	}


	bool operator==(const this_t& cmp) const { return cogs::equals(*get(), *(cmp.get())); }
	bool operator==(const this_t& cmp) const volatile
	{
		acquire_embedded();
		descriptor_t* d = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
		type* p = !d ? get_embedded() : d->get_obj();

		bool b = cogs::equals(*p, *cmp.get());

		release_embedded();
		if (!!d)
			d->release();
		return b;
	}

	bool operator==(const volatile this_t& cmp) const { return cmp.operator==(*this); }
	bool operator==(const volatile this_t& cmp) const volatile
	{
		acquire_embedded();
		descriptor_t* d = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
		type* p = !d ? get_embedded() : d->get_obj();

		cmp.acquire_embedded();
		descriptor_t* cmpDesc = (descriptor_t*)rc_obj_base::guarded_acquire(cmp.m_desc.get_ptr_ref());
		type* cmpPtr = !cmpDesc ? cmp.get_embedded() : cmpDesc->get_obj();

		bool b = cogs::equals(*p, *cmpPtr);

		cmp.release_embedded();
		if (!!cmpDesc)
			cmpDesc->release();

		release_embedded();
		if (!!d)
			d->release();
		return b;
	}

	bool operator!=(const this_t& cmp) const { return !operator==(cmp); }
	bool operator!=(const this_t& cmp) const volatile { return !operator==(cmp); }
	bool operator!=(const volatile this_t& cmp) const { return !operator==(cmp); }
	bool operator!=(const volatile this_t& cmp) const volatile { return !operator==(cmp); }

	type* get()				{ return !!m_desc ? m_desc->get_obj() : &(get_embedded()); }
	const type* get() const	{ return !!m_desc ? m_desc->get_obj() : &(get_embedded()); }

	type& operator*()					{ return *get(); }
	const type& operator*() const		{ return *get(); }

	type* operator->()					{ return get(); }
	const type* operator->() const		{ return get(); }

	template <typename... args_t>
	void set(args_t&&... src) { placement_reconstruct(get(), std::forward<args_t>(src)...); }

	template <typename... args_t>
	void set(args_t&&... src) volatile
	{
		descriptor_t* newDesc = allocate(std::forward<args_t>(src)...);
		descriptor_t* oldDesc;

		m_desc.exchange(newDesc, oldDesc);

		if (!!oldDesc)
			oldDesc->release();
		else
			release_embedded();
	}

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src)
	{
		type* ptr = get();
		cogs::assign(rtn, *ptr);
		ptr->~type();
		new (ptr) type(std::forward<args_t>(src)...);
	}

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src) volatile
	{
		descriptor_t* newDesc = allocate(std::forward<args_t>(src)...);
		descriptor_t* oldDesc;
		m_desc.exchange(newDesc, oldDesc);
		if (!oldDesc)
		{
			cogs::assign(rtn, get_embedded());
			release_embedded();
		}
		else
		{
			cogs::assign(rtn, *(oldDesc->get_obj()));
			oldDesc->release();
		}
	}

	void swap(this_t& wth)
	{
		if (this != &wth)
		{
			if (!!m_desc)
			{
				if (!!wth.m_desc)
					m_desc.swap(wth.m_desc);	// both have descriptors, so swapping those is preferred
				else
					cogs::swap(wth.get_embedded(), *(m_desc->get_obj()));
			}
			else
				cogs::swap(get_embedded(), *(wth.get()));
		}
	}

	void swap(this_t& wth) volatile
	{
		acquire_embedded();
		if (!!wth.m_desc)
		{
			m_desc.swap(wth.m_desc);
			if (!wth.m_desc)
				wth.m_desc = allocate(get_embedded());
		}
		else
		{
			descriptor_t* tmpDesc = allocate(wth.get_embedded());
			m_desc.swap(tmpDesc);
			if (!tmpDesc)
				wth.get_embedded() = get_embedded();
			else
			{
				wth.m_desc = tmpDesc;
				wth.release_embedded();
			}
		}
		release_embedded();
	}

	void swap(volatile this_t& wth)			{ wth.swap(*this); }

	void swap_contents(type& wth)			{ cogs::swap(*get(), wth); }

	void swap_contents(type& wth) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc = allocate(wth);
		acquire_embedded();
		m_desc.exchange(newDesc, oldDesc);
		if (!oldDesc)
		{
			wth = get_embedded();
			release_embedded();
		}
		else
		{
			wth = *(oldDesc->get_obj());
			oldDesc->release();
		}
		release_embedded();
	}

	// exchange
	this_t exchange(const this_t& src)
	{
		if (this == &src)
			return *this;

		// Better to exchange embedded contents of rtn than to try to preserve m_desc,
		// because it incurs an allocation.

		this_t rtn;
		cogs::exchange(*get(), *(src.get()), *(rtn.get()));
		return rtn;
	}

	this_t exchange(const this_t& src) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc = allocate(*(src.get()));
		acquire_embedded();
		m_desc.exchange(newDesc, oldDesc);
		this_t rtn(oldDesc, std::move(get_embedded()));
		release_embedded();
		return rtn;
	}

	this_t exchange(const volatile this_t& src)
	{
		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

		// Better to assign embedded contents of rtn than to try to preserve m_desc,
		// because if we did, we would need to allocate a new buffer for this value.

		this_t rtn;
		cogs::exchange(*get(), *srcPtr, *(rtn.get()));
		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();
		return rtn;
	}

	this_t exchange(const volatile this_t& src) volatile
	{
		if (this == &src)
			return *this;

		src.acquire_embedded();
		descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
		type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

		descriptor_t* oldDesc;
		descriptor_t* newDesc = allocate(*srcPtr);
		acquire_embedded();
		m_desc.exchange(newDesc, oldDesc);
		this_t rtn(oldDesc, get_embedded());
		release_embedded();
		src.release_embedded();
		if (!!srcDesc)
			srcDesc->release();
		return rtn;
	}

	this_t exchange(this_t&& src)
	{
		type* p;
		type* srcPtr;
		if (!!m_desc)
		{
			if (!!src.m_desc)
			{
				// Only if both have desc's can we shuffle them.  Otherwise, we would need to allocate one.
				this_t rtn(m_desc);
				m_desc = src.m_desc;
				src.m_desc = 0;
				return rtn;
			}

			srcPtr = &src.get_embedded();
			p = m_desc->get_obj();
		}
		else
		{
			if (!!src.m_desc)
				srcPtr = src.m_desc->get_obj();
			else
				srcPtr = &src.get_embedded();
			p = &get_embedded();
		}

		// Instead of allocating, fall back to calling exchange on the contained type.
		this_t rtn(std::move(cogs::exchange(*p, std::move(*srcPtr))));
		if (!!src.m_desc)
		{
			src.m_desc->release();
			src.m_desc = 0;
		}
		else
			src.release_embedded();
		return rtn;
	}

	this_t exchange(this_t&& src) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc;
		if (!!src.m_desc)
		{
			newDesc = src.m_desc;
			src.m_desc = 0;
		}
		else
		{
			newDesc = allocate(std::move(src.get_embedded()));
			src.release_embedded();
		}
		acquire_embedded();
		m_desc.exchange(newDesc, oldDesc);
		this_t rtn(oldDesc, std::move(get_embedded()));
		if (!oldDesc)
			release_embedded();
		release_embedded();
		return rtn;
	}


	void exchange(const this_t& src, this_t& rtn)
	{
		if (this == &src)
			rtn = *this;
		else if (this == &rtn)
			*this = src;
		else if (&src == &rtn)
		{
			if (!!m_desc)
			{
				if (!!rtn.m_desc)
					m_desc.swap(rtn.m_desc);	// both have descriptors, so swapping those is preferred
				else
					cogs::swap(*(m_desc->get_obj()), rtn.get_embedded());
			}
			else
				cogs::swap(get_embedded(), *(rtn.get()));
		}
		else
			cogs::exchange(*get(), *(src.get()), *(rtn.get()));
	}
	
	void exchange(const this_t& src, volatile this_t& rtn)
	{
		if (this == &src)
			rtn = *this;
		else
		{
			descriptor_t* newDesc = allocate(std::move(cogs::exchange(*get(), *(src.get()))));
			descriptor_t* oldRtnDesc = rtn.m_desc.exchange(newDesc);
			if (!!oldRtnDesc)
				oldRtnDesc->release();
			else
				rtn.release_embedded();
		}
	}

	void exchange(const volatile this_t& src, this_t& rtn)
	{
		if (this == &rtn)
			*this = src;
		else
		{
			src.acquire_embedded();
			descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
			type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();
			cogs::exchange(*get(), *srcPtr, *(rtn.get()));
			src.release_embedded();
			if (!!srcDesc)
				srcDesc->release();
		}
	}

	void exchange(const volatile this_t& src, volatile this_t& rtn)
	{
		if (src == &rtn)
			*this = src;
		else if (&src == &rtn)
			rtn.swap(*this);
		else
		{
			src.acquire_embedded();
			descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
			type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();
			descriptor_t* newDesc = allocate(std::move(cogs::exchange(*get(), *srcPtr)));
			descriptor_t* oldRtnDesc = rtn.m_desc.exchange(newDesc);
			if (!!oldRtnDesc)
				oldRtnDesc->release();
			else
				rtn.release_embedded();
			src.release_embedded();
			if (!!srcDesc)
				srcDesc->release();
		}
	}

	void exchange(const this_t& src, this_t& rtn) volatile
	{
		if (&src == &rtn)
			swap(rtn);
		else
		{
			descriptor_t* oldDesc;
			descriptor_t* newDesc = allocate(*(src.get()));
			acquire_embedded();
			m_desc.exchange(newDesc, oldDesc);
			if (!oldDesc)
				*(rtn.get()) = get_embedded();
			else if (!!rtn.m_desc)
				rtn.m_desc->release();
			else
				rtn.release_embedded();
			rtn.m_desc = oldDesc;
			release_embedded();
		}
	}

	void exchange(const volatile this_t& src, this_t& rtn) volatile
	{
		if (this == &src)
			rtn = *this;
		else
		{
			src.acquire_embedded();
			descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
			type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();
			descriptor_t* oldDesc;
			descriptor_t* newDesc = allocate(*srcPtr);
			acquire_embedded();
			m_desc.exchange(newDesc, oldDesc);
			if (!oldDesc)
				*(rtn.get()) = get_embedded();
			else if (!!rtn.m_desc)
				rtn.m_desc->release();
			else
				rtn.release_embedded();
			rtn.m_desc = oldDesc;
			release_embedded();
			src.release_embedded();
			if (!!srcDesc)
				srcDesc->release();
		}
	}

	void exchange(const this_t& src, volatile this_t& rtn) volatile
	{
		if (this == &rtn)
			*this = src;
		else
		{
			descriptor_t* oldDesc;
			descriptor_t* newDesc = allocate(*(src.get()));
			acquire_embedded();
			m_desc.exchange(newDesc, oldDesc);
			if (!oldDesc)
			{
				oldDesc = allocate(get_embedded());
				release_embedded();
			}
			release_embedded();
			descriptor_t* oldRtnDesc = rtn.m_desc.exchange(oldDesc);
			if (!!oldRtnDesc)
				oldRtnDesc->release();
			else
				rtn.release_embedded();
		}
	}

	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile
	{
		if (this == &src)
			rtn = *this;
		else if (this == &rtn)
			*this = src;
		else
		{
			src.acquire_embedded();
			descriptor_t* srcDesc = (descriptor_t*)rc_obj_base::guarded_acquire(src.m_desc.get_ptr_ref());
			type* srcPtr = !srcDesc ? src.get_embedded() : srcDesc->get_obj();

			descriptor_t* oldDesc;
			descriptor_t* newDesc = allocate(*srcPtr);
			acquire_embedded();
			m_desc.exchange(newDesc, oldDesc);
			if (!oldDesc)
			{
				oldDesc = allocate(get_embedded());
				release_embedded();
			}
			release_embedded();
			descriptor_t* oldRtnDesc = rtn.m_desc.exchange(oldDesc);
			if (!!oldRtnDesc)
				oldRtnDesc->release();
			else
				rtn.release_embedded();

			src.release_embedded();
			if (!!srcDesc)
				srcDesc->release();
		}
	}

	void exchange(this_t&& src, this_t& rtn)
	{
		if (this == &rtn)
			*this = std::move(src);
		else
		{
			type* p;
			type* srcPtr;
			if (!!m_desc)
			{
				if (!!src.m_desc)
				{
					// Only if both have desc's can we shuffle them.  Otherwise, we would need to allocate one.
					if (!!rtn.m_desc)
						rtn.m_desc->release();
					else
						rtn.release_embedded();
					rtn.m_desc = m_desc;
					m_desc = src.m_desc;
					src.m_desc = 0;
					return;
				}

				srcPtr = &src.get_embedded();
				p = m_desc->get_obj();
			}
			else
			{
				if (!!src.m_desc())
					srcPtr = src.m_desc->get_obj();
				else
					srcPtr = &src.get_embedded();
				p = &get_embedded();
			}

			// Instead of allocating, fall back to calling exchange on the contained type.
			cogs::exchange(*p, std::move(*srcPtr), *rtn.get());
			if (!!src.m_desc)
				src.m_desc->release();
			else
				src.release_embedded();
		}
	}

	void exchange(this_t&& src, volatile this_t& rtn)
	{
		descriptor_t* newRtnDesc;
		if (!!src.m_desc)
		{
			if (!!m_desc)
				newRtnDesc = m_desc;
			else
			{
				newRtnDesc = allocate(std::move(get_embedded()));
				release_embedded();
			}
			m_desc = src.m_desc;
			src.m_desc = 0;
		}
		else			// If we gave m_desc to rtn, we would need to create another anyway.  So, copy to rtn.
		{
			type* p = !!m_desc ? m_desc->get_obj() : &get_embedded();
			newRtnDesc = allocate(*p);
			*p = src.get_embedded();
			src.release_embedded();
		}

		descriptor_t* oldRtnDesc = rtn.m_desc.exchange(newRtnDesc);
		if (!!oldRtnDesc)
			oldRtnDesc->release();
		else
			rtn.release_embedded();
	}

	void exchange(this_t&& src, this_t& rtn) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc;
		if (!!src.m_desc)
		{
			newDesc = src.m_desc;
			src.m_desc = 0;
		}
		else
		{
			newDesc = allocate(std::move(src.get_embedded()));
			src.release_embedded();
		}

		m_desc.exchange(newDesc, oldDesc);
		if (!oldDesc)
		{
			*(rtn.get()) = std::move(get_embedded());
			release_embedded();
		}
		else if (!!rtn.m_desc)
			rtn.m_desc->release();
		else
			rtn.release_embedded();
		rtn.m_desc = oldDesc;
	}

	void exchange(this_t&& src, volatile this_t& rtn) volatile
	{
		if (this == &rtn)
			*this = std::move(src);
		else
		{
			descriptor_t* oldDesc;
			descriptor_t* newDesc;
			if (!!src.m_desc)
			{
				newDesc = src.m_desc;
				src.m_desc = 0;
			}
			else
			{
				newDesc = allocate(std::move(src.get_embedded()));
				src.release_embedded();
			}

			m_desc.exchange(newDesc, oldDesc);
			if (!oldDesc)
			{
				oldDesc = allocate(std::move(get_embedded()));
				release_embedded();
			}
			descriptor_t* oldRtnDesc = rtn.m_desc.exchange(oldDesc);
			if (!!oldRtnDesc)
				oldRtnDesc->release();
			else
				rtn.release_embedded();
		}
	}

	// exchange_contents
	template <typename type2>
	type exchange_contents(type2&& src) { return cogs::exchange(*get(), std::forward<type2>(src)); }

	template <typename type2>
	type exchange_contents(type2&& src) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc = allocate(std::forward<type2>(src));
		m_desc.exchange(newDesc, oldDesc);
		if (!oldDesc)
			release_embedded();
		else
			oldDesc->release();
	}

	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) { cogs::exchange(*get_content(), std::forward<type2>(src), rtn); }

	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) volatile
	{
		descriptor_t* oldDesc;
		descriptor_t* newDesc = allocate(std::forward<type2>(src));
		m_desc.exchange(newDesc, oldDesc);
		if (!oldDesc)
		{
			cogs::assign(rtn, std::move(get_embedded()));
			release_embedded();
		}
		else
		{
			cogs::assign(rtn, std::move(*(oldDesc->get_obj())));
			oldDesc->release();
		}
	}

	// compare_exchange
	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) { return cogs::compare_exchange(std::forward<type2>(src), cmp); }

	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) volatile
	{
		bool b = false;
		descriptor_t* newDesc = 0;
		descriptor_t* oldDesc;
		acquire_embedded();
		for (;;)
		{
			oldDesc = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
			type* objPtr = (!!oldDesc) ? (oldDesc->get_obj()) : &get_embedded();
			if (cogs::equals(*objPtr, cmp))
			{
				if (!newDesc)
					newDesc = allocate(std::forward<type2>(src));
				b = m_desc.compare_exchange(newDesc, oldDesc);

				// Dispose the temporary reference we acquired for the sake of comparing against it.
				// If compare_exchange was successful, we need to release it an additional time.
				if (!!oldDesc)
					oldDesc->release();

				if (b)
				{
					// compare_exchange was successful.  m_desc now contains newDesc.
					// Dispose of the original reference.
					if (!!oldDesc)
						oldDesc->release();
					else
						release_embedded();
					break;
				}
				continue;
			}

			if (!!newDesc)
				newDesc->release();
			if (!!oldDesc)
				oldDesc->release();
			break;
		}

		release_embedded();
		return b;
	}

	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) { return cogs::compare_exchange(std::forward<type2>(src), cmp, rtn); }

	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) volatile
	{
		bool b = false;
		ptr<descriptor_t> newDesc = 0;
		ptr<descriptor_t> oldDesc;
		acquire_embedded();
		for (;;)
		{
			oldDesc = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
			type* objPtr = (!!oldDesc) ? (oldDesc->get_obj()) : &get_embedded();
			if (cogs::equals(*objPtr, cmp))
			{
				if (!newDesc)
					newDesc = allocate(std::forward<type2>(src));
				b = m_desc.compare_exchange(newDesc, oldDesc);

				if (b)
					cogs::assign(rtn, *objPtr);

				// Dispose the temporary reference we acquired for the sake of comparing against it.
				// If compare_exchange was successful, we need to release it an additional time.
				if (!!oldDesc)
					oldDesc->release();

				if (b)
				{
					// compare_exchange was successful.  m_desc now contains newDesc.
					// Dispose of the original reference.
					if (!!oldDesc)
						oldDesc->release();
					else
						release_embedded();
					break;
				}
				continue;
			}

			if (!!newDesc)
				newDesc->release();
			cogs::assign(rtn, *objPtr);
			if (!!oldDesc)
				oldDesc->release();
			break;
		}

		release_embedded();
		return b;
	}


	read_token begin_read() const volatile
	{
		read_token rt;
		begin_read(rt);
		return rt;
	}

	void begin_read(read_token& rt) const volatile
	{
		acquire_embedded();

		ptr<descriptor_t> readDesc;
		ptr<descriptor_t> compareDesc = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
		if (!compareDesc)
		{
			// If embedded, we need to make a copy.
			// We can't give out owned references to our internal payload
			// Also, can't self-update to use dynamic payload, to preserve const-correctness
			readDesc = allocate(get_embedded());
		}
		else
			readDesc = compareDesc;	// readDesc now owns, compareDesc does not own it

		release_embedded();
		rt.set(compareDesc, readDesc);
	}

	bool end_read(read_token& t) const volatile
	{
		bool result = false;
		if (!!t.m_read)
		{
			ptr<descriptor_t> p = m_desc;
			result = (t.m_compare == p);
			t.m_read->release();
			t.m_read = 0;
		}
		return result;
	}

	void begin_write(write_token& wt) volatile
	{
		acquire_embedded();
		ptr<descriptor_t> compareDesc = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
		ptr<descriptor_t> writeDesc = allocate((!compareDesc) ? get_embedded() : *(compareDesc->get_obj()));
		release_embedded();
		wt.set(compareDesc, writeDesc);
	}

	template <typename... args_t>
	void begin_write(write_token& wt, args_t&&... src) volatile
	{
		ptr<descriptor_t> newDesc = allocate(std::forward<args_t>(src)...);
		ptr<descriptor_t> oldDesc = (descriptor_t*)rc_obj_base::guarded_acquire(m_desc.get_ptr_ref());
		wt.set(oldDesc, newDesc);
	}
	
	bool promote_read_token(read_token& rt, write_token& wt) volatile	// ends read_token
	{
		bool result = false;
		if (!!rt.m_read)
		{
			ptr<descriptor_t> desc = m_desc;
			result = (rt.m_compare == desc);
			for (;;)
			{
				if (result)
				{
					if (!desc)
					{							// If read_token's m_compare is null, then the m_read was a one-off.  Just use it.
						wt.set(nullptr, rt.m_read);
						break;					// don't release read token
					}
					if (!desc->is_owned())		// Read token would be the same as compare, and read token was acquired...
					{							// Take ownership from the read token, and allocate a new writeable copy
						wt.set(desc, allocate(*(desc->get_obj())));
						break;
					}
					COGS_ASSERT(m_desc != desc);
					result = false;				// If we own the only copy of that block, then we KNOW a write would fail anyway
				}
				rt.m_read->release();
				break;
			}
			rt.m_read = 0;
		}
		return result;
	}

	template <typename... args_t>
	bool promote_read_token(read_token& rt, write_token& wt, args_t&&... src) volatile
	{
		bool result = false;
		if (!!rt.m_read)
		{
			ptr<descriptor_t> desc = m_desc;
			result = (rt.m_compare == desc);
			for (;;)
			{
				if (result)
				{
					if (!desc)
					{							// If read_token's m_compare is null, then the m_read was a one-off.  Just use it.
						wt.set(0, rt.m_read);
						break;					// don't release read token
					}
					if (!desc->is_owned())		// Read token would be the same as compare, and read token was acquired...
					{							// Take ownership from the read token, and allocate a new writeable copy
						wt.set(desc, allocate(std::forward<args_t>(src)...));
						break;
					}
					COGS_ASSERT(m_desc != desc);
					result = false;				// If we own the only copy of that block, then we KNOW a write would fail anyway
				}
				COGS_ASSERT(!!desc);
				rt.m_read->release();
				break;
			}
			rt.m_read = 0;
		}
		return result;
	}
	
	bool end_write(write_token& t) volatile
	{
		COGS_ASSERT(t.m_compare != t.m_write);

		bool result = false;
		if (!!t.m_write)
		{
			result = m_desc.compare_exchange(t.m_write, t.m_compare);
			if (!result)
				t.m_write->release();	// Clean out write token
			else if (!!t.m_compare)
				t.m_compare->release();	// If t.m_compare was not null, it now contains the original value, and needs to be released on its behalf.
			else
				release_embedded();		// Otherwise, embedded needs to be released
			if (!!t.m_compare)
				t.m_compare->release();	// Clean out write token
			t.m_write = 0;
		}
		return result;
	}

	template <typename... args_t>
	bool end_write(read_token& t, args_t&&... src) volatile
	{
		bool result = false;
		if (!!t.m_read)
		{
			ptr<descriptor_t> newDesc = allocate(std::forward<args_t>(src)...);
			result = m_desc.compare_exchange(newDesc, t.m_compare);
			if (!result)
				newDesc->release();			// newDesc won't be used, so needs to be released.
			else if (!!t.m_compare)
				t.m_compare->release();		// If t.m_compare was not null, it now contains the original value, and needs to be released on its behalf.
			else
				release_embedded();			// Otherwise, embedded needs to be released
			t.m_read->release();			// Clean out read token
			t.m_read = 0;
		}
		return result;
	}

	template <typename... args_t>
	bool end_write(write_token& t, args_t&&... src) volatile
	{
		bool result = false;
		if (!!t.m_write)
		{
			ptr<descriptor_t> newDesc = allocate(std::forward<args_t>(src)...);
			result = m_desc.compare_exchange(newDesc, t.m_compare);
			if (!result)
				newDesc->release();			// newDesc won't be used, so needs to be released.
			else if (!!t.m_compare)
				t.m_compare->release();		// If t.m_compare was not null, it now contains the original value, and needs to be released on its behalf.
			else
				release_embedded();			// Otherwise, embedded needs to be released
			t.m_write->release();		// Clean out write token.  We didn't use it at all, we used src args
			if (!!t.m_compare)
				t.m_compare->release();	// Clean out write token
			t.m_write = 0;
		}
		return result;
	}

	bool is_current(const read_token& rt) const volatile		{ return (m_desc == rt.m_compare) && !!rt.m_read; }
	bool is_current(const write_token& wt) const volatile		{ return (m_desc == wt.m_compare) && !!wt.m_write; }
};


template <typename T>
class empty_transactable
{
public:
	typedef empty_transactable<T>	this_t;
	typedef T type;

private:
	template <typename>
	friend class transactable;

	class construct_embedded_t { };
	class construct_embedded_volatile_t { };
	class unconstructed_embedded_t { };
	class unconstructed_embedded_volatile_t { };

	template <typename... args_t>
	empty_transactable(const construct_embedded_t&, args_t&&... src) { }

	template <typename... args_t>
	empty_transactable(const construct_embedded_volatile_t&, args_t&&... src) { }

	empty_transactable(const unconstructed_embedded_t&) { }
	empty_transactable(const unconstructed_embedded_volatile_t&) { }

public:
	class write_token
	{
	public:
		write_token()	// leave contents uninitialized
		{ }

		write_token(write_token& wt) = delete;// { }
		write_token(write_token&& wt) { }
		write_token& operator=(write_token& wt) = delete;// { return *this; }
		write_token& operator=(write_token&& wt) { return *this; }
		void release() { }

		type* get() { return nullptr; }
		const type* get() const { return nullptr; }
		type& operator*() { return *get(); }
		const type& operator*() const { *get(); }
		type* operator->() { return nullptr; }
		const type* operator->() const { return nullptr; }
	};

	class read_token
	{
	public:
		read_token() { }
		read_token(const read_token& rt) = delete;// { }
		read_token(const read_token&& rt) { }
		read_token& operator=(read_token& rt) = delete;// { return *this; }
		read_token& operator=(read_token&& rt) { return *this; }
		void release() { }

		const type* get() const { return nullptr; }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return nullptr; }
	};

	empty_transactable() { }

	empty_transactable(const this_t& src) { }
	empty_transactable(const volatile this_t& src) { }

	empty_transactable(this_t& src) { }
	empty_transactable(volatile this_t& src) { }

	empty_transactable(this_t&& src) { }

	//template <typename... args_t>
	//empty_transactable(args_t&&... src) { }

	this_t& operator=(const volatile this_t& src) { return *this; }
	volatile this_t& operator=(const volatile this_t& src) volatile { return *this; }

	this_t& operator=(this_t&& src) { return *this; }


	bool operator==(const this_t& cmp) const volatile  { return true; }
	bool operator==(const volatile this_t& cmp) const volatile  { return true; }

	bool operator!=(const this_t& cmp) const volatile  { return false; }
	bool operator!=(const volatile this_t& cmp) const volatile  { return false; }

	type* get() { return nullptr; }
	const type* get() const { return nullptr; }
	volatile type* get() volatile { return nullptr; }
	const volatile type* get() const volatile { return nullptr; }

	type& operator*() { return *get(); }
	const type& operator*() const { return *get(); }
	volatile type& operator*() volatile { return *get(); }
	const volatile type& operator*() const volatile { return *get(); }

	type* operator->() { return nullptr; }
	const type* operator->() const { return nullptr; }
	volatile type* operator->() volatile { return nullptr; }
	const volatile type* operator->() const volatile { return nullptr; }

	template <typename... args_t>
	void set(args_t&&... src) volatile { }

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src) volatile { }

	void swap(this_t& wth) volatile { }

	void swap(volatile this_t& wth) volatile { }

	void swap_contents(type& wth) volatile { }


	this_t exchange(const volatile this_t& src) volatile { }

	this_t exchange(this_t&& src) volatile { }

	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { }

	void exchange(this_t&& src, volatile this_t& rtn) volatile { }


	template <typename type2>
	type exchange_contents(type2&& src) volatile { }

	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) volatile { }

	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) volatile { return true; }

	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) volatile
	{
		cogs::assign(rtn, src);
		return cogs::equals(src, cmp);
	}

	read_token begin_read() const volatile { read_token rt; return rt; }

	void begin_read(read_token& rt) const volatile { }

	bool end_read(read_token& t) const volatile { return true; }

	void begin_write(write_token& wt) volatile { }

	template <typename... args_t>
	void begin_write(write_token& wt, args_t&&... src) volatile { }

	bool promote_read_token(read_token& rt, write_token& wt) volatile { return true; }

	template <typename... args_t>
	bool promote_read_token(read_token& rt, write_token& wt, args_t&&... src) volatile { return true; }

	bool end_write(write_token& t) volatile { return true; }

	template <typename... args_t>
	bool end_write(read_token& t, args_t&&... src) volatile { return true; }

	template <typename... args_t>
	bool end_write(write_token& t, args_t&&... src) volatile { return true; }

	bool is_current(const read_token& rt) const volatile { return true; }
	bool is_current(const write_token& wt) const volatile { return true; }
};


template <typename T>
class cas_transactable
{
public:
	typedef cas_transactable<T>	this_t;
	typedef T type;

private:
	static_assert(can_atomic_v<T>);

	alignas (atomic::get_alignment_v<type>) type m_contents;

	template <typename>
	friend class transactable;

	class construct_embedded_t { };
	class construct_embedded_volatile_t { };
	class unconstructed_embedded_t { };
	class unconstructed_embedded_volatile_t { };

	template <typename... args_t>
	cas_transactable(const construct_embedded_t&, args_t&&... src)
		: m_contents(std::forward<T>(src)...)
	{ }

	template <typename... args_t>
	cas_transactable(const construct_embedded_volatile_t&, args_t&&... src)
		: m_contents(std::forward<T>(src)...)
	{ }

	cas_transactable(const unconstructed_embedded_t&)
	{ }

	cas_transactable(const unconstructed_embedded_volatile_t&)
	{ }

public:
	class write_token
	{
	protected:
		typename placement<type>	m_old;
		typename placement<type>	m_new;

		friend class cas_transactable;

		write_token(const type& t)
		{
			memcpy(&m_old, &t, sizeof(type));
			memcpy(&m_new, &t, sizeof(type));
		}

		void set(const type& t)
		{
			memcpy(&m_old, &t, sizeof(type));
			memcpy(&m_new, &t, sizeof(type));
		}	

		void set(const type& t, const type& newT)
		{
			memcpy(&m_old, &t, sizeof(type));
			memcpy(&m_new, &newT, sizeof(type));
		}

	public:
		write_token()	// leave contents uninitialized
		{ }

		write_token(write_token& wt) = delete;

		write_token(write_token&& wt)
		{
			memcpy(&m_old, &wt.m_old, sizeof(type));
			memcpy(&m_new, &wt.m_new, sizeof(type));
		}

		write_token& operator=(write_token& wt) = delete;

		write_token& operator=(write_token&& wt)
		{
			memcpy(&m_old, &wt.m_old, sizeof(type));
			memcpy(&m_new, &wt.m_new, sizeof(type));
			return *this;
		}

		void release() { }

		      type* get()				{ return &m_new.get(); }
		const type* get() const			{ return &m_new.get(); }
		      type& operator*()			{ return m_new.get(); }
		const type& operator*() const	{ return m_new.get(); }
		      type* operator->()		{ return get(); }
		const type* operator->() const	{ return get(); }
	};

	class read_token
	{	
	protected:
		typename placement<type>	m_obj;

		friend class cas_transactable;
		
		read_token(const type& t)
		{
			memcpy(&m_obj, &t, sizeof(type));
		}

		void set(const type& t)			
		{
			memcpy(&m_obj, &t, sizeof(type));
		}

	public:
		read_token()	
		{ }

		read_token(const read_token& rt) = delete;

		read_token(const read_token&& rt)
		{
			memcpy(&m_obj, &rt.m_obj, sizeof(type));
		}

		read_token& operator=(read_token& rt) = delete;

		read_token& operator=(read_token&& rt)
		{
			memcpy(&m_obj, &rt.m_obj, sizeof(type));
			return *this;
		}

		void release() { }

		const type* get() const			{ return &m_obj.get(); }
		const type& operator*() const	{ return m_obj.get(); }
		const type* operator->() const	{ return get(); }
	};

	cas_transactable()
	{ }

	cas_transactable(const this_t& src)
		: m_contents(src.m_contents)
	{ }

	cas_transactable(const volatile this_t& src)
	{
		cogs::assign(m_contents, src.m_contents);
	}

	// Need a non-const version of this constructor to prevent variadic constructor from being select for non-const arg
	
	cas_transactable(this_t& src)
		: m_contents(src.m_contents)
	{ }

	cas_transactable(volatile this_t& src)
	{
		cogs::assign(m_contents, src.m_contents);
	}

	cas_transactable(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(const this_t& src) { cogs::assign(m_contents, src.m_contents); return *this; }
	this_t& operator=(const volatile this_t& src) { cogs::assign(m_contents, src.m_contents);; return *this; }
	volatile this_t& operator=(const this_t& src) volatile { cogs::assign(m_contents, src.m_contents);; return *this; }
	volatile this_t& operator=(const volatile this_t& src) volatile { cogs::assign(m_contents, src.m_contents); return *this; }

	this_t& operator=(this_t&& src) { m_contents = std::move(src.m_contents); return *this; }


	bool operator==(const this_t& cmp) const { return cogs::equals(*get(), *(cmp.get())); }
	bool operator==(const this_t& cmp) const volatile { return cogs::equals(*get(), *(cmp.get())); }
	bool operator==(const volatile this_t& cmp) const { return cogs::equals(*get(), *(cmp.get())); }
	bool operator==(const volatile this_t& cmp) const volatile { return cogs::equals(*get(), *(cmp.get())); }

	bool operator!=(const this_t& cmp) const { return !cogs::equals(*get(), *(cmp.get())); }
	bool operator!=(const this_t& cmp) const volatile { return !cogs::equals(*get(), *(cmp.get())); }
	bool operator!=(const volatile this_t& cmp) const { return !cogs::equals(*get(), *(cmp.get())); }
	bool operator!=(const volatile this_t& cmp) const volatile { return !cogs::equals(*get(), *(cmp.get())); }

	type* get() { return &(m_contents); }
	const type* get() const { return &(m_contents); }
	volatile type* get() volatile { return &(m_contents); }
	const volatile type* get() const volatile { return &(m_contents); }

	type& operator*() { return *get(); }
	const type& operator*() const { return *get(); }
	volatile type& operator*() volatile	{ return *get(); }
	const volatile type& operator*() const volatile	{ return *get(); }

	type* operator->() { return get(); }
	const type* operator->() const { return get(); }
	volatile type* operator->() volatile { return get(); }
	const volatile type* operator->() const volatile { return get(); }

	template <typename... args_t>
	void set(args_t&&... src) { placement_reconstruct(get(), std::forward<args_t>(src)...); }

	template <typename... args_t>
	void set(args_t&&... src) volatile
	{
		type tmp(std::forward<args_t>(src)...);
		atomic::store(m_contents, tmp);
	}

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src)
	{
		type* ptr = get();
		cogs::assign(rtn, *ptr);
		ptr->~type();
		new (ptr) type(std::forward<args_t>(src)...);
	}

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src) volatile
	{
		type tmp(std::forward<args_t>(src)...);
		exchange_contents(tmp, rtn);
	}

	void swap(this_t& wth) { cogs::swap(m_contents, wth.m_contents); }
	void swap(this_t& wth) volatile { atomic::exchange(m_contents, wth.m_contents, wth.m_contents); }
	void swap(volatile this_t& wth) { atomic::exchange(wth.m_contents, m_contents, m_contents); }

	void swap_contents(type& wth) { cogs::swap(m_contents, wth); }
	void swap_contents(type& wth) volatile { atomic::exchange(m_contents, wth, wth); }
	void swap_contents(volatile type& wth) { atomic::exchange(wth, m_contents, m_contents); }

	// exchange
	this_t exchange(const this_t& src) { return cogs::exchange(m_contents, src.m_contents); }

	this_t exchange(const this_t& src) volatile { return atomic::exchange(m_contents, src.m_contents); }

	this_t exchange(const volatile this_t& src) { return cogs::exchange(m_contents, src.m_contents); }

	this_t exchange(const volatile this_t& src) volatile { return atomic::exchange(m_contents, src.m_contents); }

	this_t exchange(this_t&& src) { return cogs::exchange(m_contents, std::move(src.m_contents)); }

	this_t exchange(this_t&& src) volatile { return atomic::exchange(m_contents, std::move(src.m_contents)); }


	void exchange(const this_t& src, this_t& rtn) { cogs::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const this_t& src, volatile this_t& rtn) { cogs::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, this_t& rtn) { cogs::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, volatile this_t& rtn) { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const this_t& src, this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const this_t& src, volatile this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) { cogs::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, volatile this_t& rtn) { cogs::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, volatile this_t& rtn) volatile { atomic::exchange(m_contents, src.m_contents, rtn.m_contents); }

	// exchange_contents
	template <typename type2>
	type exchange_contents(type2&& src) { return cogs::exchange(m_contents, std::forward<type2>(src)); }

	template <typename type2>
	type exchange_contents(type2&& src) volatile { return atomic::exchange(m_contents, std::forward<type2>(src)); }

	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) { cogs::exchange(m_contents, std::forward<type2>(src), rtn); }

	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) volatile { return atomic::exchange(m_contents, std::forward<type2>(src), rtn); }


	// compare_exchange
	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) { return cogs::compare_exchange(m_contents, src, cmp); }

	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) volatile { return atomic::compare_exchange(m_contents, src, cmp); }

	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) { return cogs::compare_exchange(m_contents, src, cmp, rtn); }

	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) volatile { return atomic::compare_exchange(m_contents, src, cmp, rtn); }



	read_token begin_read() const volatile			{ read_token rt; begin_read(rt); return rt; }
	
	void begin_read(read_token& rt) const volatile	{ type tmp; atomic::load(m_contents, tmp); rt.set(tmp); }

	bool end_read(read_token& t) volatile			{ type tmp; atomic::load(m_contents, tmp); return tmp == t.m_obj; }

	void begin_write(write_token& wt) volatile		{ type tmp; atomic::load(m_contents, tmp); wt.set(tmp); }

	template <typename... args_t>
	void begin_write(write_token& wt, args_t&&... src) volatile
	{
		type tmp;
		atomic::load(m_contents, tmp);
		wt.set(tmp, std::forward<args_t>(src)...);
	}

	bool promote_read_token(read_token& rt, write_token& wt) volatile	// ends read_token
	{
		type tmp;
		atomic::load(m_contents, tmp);
		if (*(rt.get()) != tmp)
			return false;
		wt.set(*(rt.get()));
		return true;
	}

	template <typename... args_t>
	bool promote_read_token(read_token& rt, write_token& wt, args_t&&... src) volatile	// ends read_token
	{
		type tmp;
		atomic::load(m_contents, tmp);
		if (*(rt.get()) != tmp)
			return false;
		type tmp2(std::forward<args_t>(src)...);
		wt.set(*(rt.get()), tmp2);
		return true;
	}

	bool end_write(write_token& t) volatile
	{
		return atomic::compare_exchange(m_contents, *(t.get()), t.m_old.get());
	}
	
	template <typename... args_t>
	bool end_write(read_token& t, args_t&&... src) volatile
	{
		type tmp(std::forward<args_t>(src)...);
		return atomic::compare_exchange(m_contents, tmp, *(t.get()));
	}
	
	template <typename... args_t>
	bool end_write(write_token& t, args_t&&... src) volatile
	{
		type tmp(std::forward<args_t>(src)...);
		return atomic::compare_exchange(m_contents, tmp, t.m_old.get());
	}

	bool is_current(const read_token& rt) const volatile
	{
		type tmp;
		atomic::load(m_contents, tmp);
		return (*(rt.get()) == tmp);
	}

	bool is_current(const write_token& wt) const volatile
	{
		type tmp;
		atomic::load(m_contents, tmp);
		return (*(wt.get()) == tmp);
	}
};


/// @ingroup Synchronization
/// @brief Provides atomic transactions for a type
/// 
/// Encapsulates a type and provides access to it using simple atomic read/write transactions.
/// Acquiring a transactable::read_token provides a read-only copy.  
/// Acquiring a transactable::write_token creates a read/write copy, and
/// allows changes to be committed conditionally if no other changes have been committed since read.
///
/// @tparam T Type to encapsulate
template <typename T>
class transactable
{
public:
	typedef T	type;
	typedef transactable<T>	this_t;

private:
	typedef std::conditional_t<std::is_empty_v<T>, empty_transactable<type>,
				std::conditional_t<can_atomic_v<T>, cas_transactable<type>, thread_safe_transactable<type> > > content_t;

	content_t m_contents;

public:
	class construct_embedded_t { };
	class construct_embedded_volatile_t { };
	class unconstructed_embedded_t { };
	class unconstructed_embedded_volatile_t { };

	template <typename... args_t>
	transactable(const construct_embedded_t&, args_t&&... src)
		: m_contents(typename content_t::construct_embedded_t(), std::forward<args_t>(src)...)
	{ }

	template <typename... args_t>
	transactable(const construct_embedded_volatile_t&, args_t&&... src)
		: m_contents(typename content_t::construct_embedded_volatile_t(), std::forward<args_t>(src)...)
	{ }

	transactable(const unconstructed_embedded_t&)
		: m_contents(typename content_t::unconstructed_embedded_t())
	{ }

	transactable(const unconstructed_embedded_volatile_t&)
		: m_contents(typename content_t::unconstructed_embedded_volatile_t())
	{ }

	/// @brief A read transaction token
	class read_token
	{
	private:
		typename content_t::read_token	m_readToken;

		template <typename>
		friend class transactable;

		read_token(typename content_t::read_token& rt)
			:	m_readToken(rt)
		{ }

	public:
		/// @{
		/// @brief Constructor
		read_token()															{ }
		/// @}

		/// @{
		/// @brief Move-Constructor
		/// @param src Initial value
		read_token(read_token& src) = delete;
		read_token(read_token&& src) : m_readToken(std::move(src.m_readToken)) { }
		/// @}
		
		/// @{
		/// @brief Move-Assignment
		/// @param src Value to copy
		/// @return A reference to this
		read_token& operator=(read_token& src) = delete;
		read_token& operator=(read_token&& src) { m_readToken = std::move(src.m_readToken); return *this; }
		/// @}

		/// @{
		/// @brief Releases this read_token
		void release()															{ m_readToken.release(); }
		/// @}

		/// @{
		/// @brief Gets the value referenced by this read_token
		/// @return The value referenced by this read_token
		const type* get() const													{ return m_readToken.get(); }
		const type& operator*() const											{ return *get(); }
		const type* operator->() const											{ return get(); }
		/// @}
	};

	/// @brief A (read and) write transaction token
	class write_token
	{
	private:
		typename content_t::write_token	m_writeToken;

		template <typename>
		friend class transactable;

		write_token(typename content_t::write_token& wt)
			: m_writeToken(wt)
		{ }

	public:
		/// @{
		/// @brief Constructor
		write_token()															{ }
		/// @}

		/// @{
		/// @brief Move-Constructor
		/// @param src Initial value
		write_token(write_token&& src) : m_writeToken(std::move(src.m_writeToken)) { }
		/// @}

		/// @{
		/// @brief Move-Assignment
		/// @param src Value to move
		/// @return A reference to this
		write_token& operator=(write_token&& src)							{ m_writeToken = std::move(src.m_writeToken); return *this; }
		/// @}

		/// @{
		/// @brief Releases this write_token
		void release()														{ m_writeToken.release(); }
		/// @}

		/// @{
		/// @brief Gets the value referenced by this write_token
		      type* get()				{ return m_writeToken.get(); }
		const type* get() const			{ return m_writeToken.get(); }
		      type& operator*()			{ return *get(); }
		const type& operator*() const	{ return *get(); }
		      type* operator->()		{ return get(); }
		const type* operator->() const	{ return get(); }
		/// @}
	};

	transactable() { }

	transactable(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	transactable(const this_t& src) : m_contents(*(src.get())) { }
	transactable(const volatile this_t& src) : m_contents(*(src.begin_read())) { }

	transactable(this_t& src) : m_contents(*(src.get())) { }
	transactable(volatile this_t& src) : m_contents(*(src.begin_read())) { }

	template <typename type2>
	transactable(const transactable<type2>& src) : m_contents(*(src.get())) { }

	template <typename type2>
	transactable(transactable<type2>& src) : m_contents(*(src.get())) { }

	template <typename type2>
	transactable(const volatile transactable<type2>& src) : m_contents(*(src.begin_read())) { }

	template <typename type2>
	transactable(volatile transactable<type2>& src) : m_contents(*(src.begin_read())) { }
	/// @}

	/// @{
	/// @brief Assignment
	/// @param src Value to set to
	/// @return Reference to this
	this_t& operator=(const this_t& src) { m_contents = src.m_contents; return *this; }

	this_t& operator=(const volatile this_t& src) { m_contents = src.m_contents; return *this; }

	template <typename type2>
	this_t& operator=(const transactable<type2>& src)		{ *get() = *src.get(); return *this; }

	template <typename type2>
	this_t& operator=(const volatile transactable<type2>& src)	{ *get() = *(src.begin_read()); return *this; }

	/// @brief Thread-safe implementation of operator=()
	/// @param src Value to set to
	volatile this_t& operator=(const this_t& src) volatile { m_contents = src.m_contents; return *this; }

	template <typename type2>
	volatile this_t& operator=(const transactable<type2>& src) volatile	{ m_contents.set(*src); return *this; }

	template <typename type2>
	volatile this_t& operator=(const volatile transactable<type2>& src) volatile { m_contents.set(*src.begin_read());  return *this; }
	/// @}


	bool operator==(const this_t& cmp) const { return cogs::equals(m_contents, cmp.m_contents); }
	bool operator==(const this_t& cmp) const volatile { return cogs::equals(*begin_read(), *cmp); }
	bool operator==(const volatile this_t& cmp) const { return cogs::equals(*get(), *cmp.begin_read()); }
	bool operator==(const volatile this_t& cmp) const volatile { return cogs::equals(*begin_read(), *cmp.begin_read()); }

	bool operator!=(const this_t& cmp) const { return !operator==(cmp); }
	bool operator!=(const this_t& cmp) const volatile { return !operator==(cmp); }
	bool operator!=(const volatile this_t& cmp) const { return !operator==(cmp); }
	bool operator!=(const volatile this_t& cmp) const volatile { return !operator==(cmp); }


	/// @{
	/// @brief Gets the encapsulated value.
	///
	/// Direct access to the encapsulated value bypasses thread safety, and should
	/// not be done if there is potential parallel access.
	/// @return A pointer to the encapsulated object
	               type* get()					{ return m_contents.get(); }
	const          type* get() const			{ return m_contents.get(); }

	               type* operator->()					{ return get(); }
	const          type* operator->() const				{ return get(); }

	/// @brief Gets the encapsulated value.
	///
	/// Direct access to the encapsulated value bypasses thread safety, and should
	/// not be done if there is potential parallel access.
	/// @return A reference to the encapsulated object
	               type& operator*()					{ return *get(); }
	const          type& operator*() const				{ return *get(); }
	/// @}

	/// @{
	/// @brief Sets the encapsulated value to the specified value
	/// @param src Value to set to
	template <typename... args_t>
	void set(args_t&&... src) { m_contents.set(std::forward<args_t>(src)...); }

	/// @brief Thread-safe implementation of set()
	template <typename... args_t>
	void set(args_t&&... src) volatile { m_contents.set(std::forward<args_t>(src)...); }
	/// @}

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src)			{ m_contents.exchange_set(rtn, std::forward<args_t>(src)...); }

	template <typename type2, typename... args_t>
	void exchange_set(type2& rtn, args_t&&... src) volatile	{ m_contents.exchange_set(rtn, std::forward<args_t>(src)...); }

	/// @{
	/// @brief Swaps contents with another transactable
	/// @param[in,out] wth Transactable to exchange with
	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth) { m_contents.swap(wth.m_contents); }
	/// @brief Thread-safe implementation of swap();
	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }
	/// @}

	/// @{
	/// @brief Exchanges the encapsulated value
	/// @param wth Value to exchange
	template <typename type2>
	void swap_contents(type2& wth) { m_contents.swap_contents(wth); }
	/// @brief Thread-safe implementation of swap_contents()
	template <typename type2>
	void swap_contents(type2& wth) volatile { m_contents.swap_contents(wth); }

	this_t exchange(this_t&& src) { return cogs::exchange(std::move(src)); }
	this_t exchange(this_t&& src) volatile { return cogs::exchange(std::move(src)); }

	/// @{
	/// @brief Exchanges contents with another transactable
	/// @param[in] src Transactable to copy
	/// @param[out] rtn Receives a copy of the original transactable
	void exchange(const this_t& src, this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }
	void exchange(this_t&& src, volatile this_t& rtn) { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }

	/// @brief Thread-safe implementation of exchange()
	void exchange(const this_t& src, this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, volatile this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) volatile { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }
	void exchange(this_t&& src, volatile this_t& rtn) volatile { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }
	/// @}

	template <typename type2>
	type exchange_contents(type2&& src) { return m_contents.exchange_contents(std::forward<type2>(src)); }

	template <typename type2>
	type exchange_contents(type2&& src) volatile { return m_contents.exchange_contents(std::forward<type2>(src)); }


	/// @brief Exchanges the encapsulated value
	/// @param[in] src Value to set
	/// @param[out] rtn Receives the original value
	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) { m_contents.exchange_contents(std::forward<type2>(src), rtn); }

	/// @brief Thread-safe implementation of exchange_contents()
	template <typename type2, typename type3>
	void exchange_contents(type2&& src, type3& rtn) volatile { m_contents.exchange_contents(std::forward<type2>(src), rtn); }
	/// @}

	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated value
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) { return m_contents.compare_exchange_contents(std::forward<type2>(src), cmp); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3>
	bool compare_exchange_contents(type2&& src, const type3& cmp) volatile { return m_contents.compare_exchange_contents(std::forward<type2>(src), cmp); }

	/// @brief Based on a comparison, conditionally exchange the encapsulated value
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) { return m_contents.compare_exchange_contents(std::forward<type2>(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3, typename type4>
	bool compare_exchange_contents(type2&& src, const type3& cmp, type4& rtn) volatile { return m_contents.compare_exchange_contents(std::forward<type2>(src), cmp, rtn); }
	/// @}



	/// @{
	/// @brief Gets an object that provides read-only access to the encapsulated value
	/// @return An object that provides read-only access to the encapsulated value
	read_token begin_read() const volatile									{ read_token rt; m_contents.begin_read(rt.m_readToken); return rt; }
	/// @brief Gets an object that provides read-only access to the encapsulated value
	/// @param[out] rt An object that provides read-only access to the encapsulated value
	void begin_read(read_token& rt) const volatile							{ m_contents.begin_read(rt.m_readToken); }
	/// @}

	/// @{
	/// @brief Releases a read_token and checks if the value has remained the same since the read token was acquired.
	/// 
	/// It is not necessary to use end_read() to release a read_token, but it provides a way of checking if the read
	/// state has remained current at the end of a read operation.
	/// @param[in,out] rt An object that provides read-only access to the encapsulated value
	/// @return True if the value has remained the same since the read token was acquired
	bool end_read(read_token& rt) const volatile							{ return m_contents.end_read(rt.m_readToken); }
	/// @}

	/// @{
	/// @brief Gets an object that provides (read and) write access to the encapsulated value
	/// @param[out] wt An object that provides (read and) write access to the encapsulated value
	void begin_write(write_token& wt) volatile								{ m_contents.begin_write(wt.m_writeToken); }
	/// @brief Gets an object that provides write access to the encapsulated value
	/// @param[out] wt An object that provides write access to the encapsulated value
	/// @param src New value to initialize the write token to.
	template <typename... args_t>
	void begin_write(write_token& wt, args_t&&... src) volatile				{ m_contents.begin_write(wt.m_writeToken, std::forward<args_t>(src)...); }
	/// @}

	/// @{
	/// @brief Converts a read_token to a write_token, if the value has not yet changed
	/// @param[in,out] rt read_token to convert to a write token
	/// @param[out] wt An object that provides (read and) write access to the encapsulated value
	/// @return True if the promotion was successful
	bool promote_read_token(read_token& rt, write_token& wt) volatile		{ return m_contents.promote_read_token(rt.m_readToken, wt.m_writeToken); }
	/// @brief Converts a read_token to a write_token, if the value has not yet changed
	/// @param[in,out] rt read_token to convert to a write token
	/// @param[out] wt An object that provides (read and) write access to the encapsulated value
	/// @param src New value to initialize the write token to.
	/// @return True if the promotion was successful
	template <typename... args_t>
	bool promote_read_token(read_token& rt, write_token& wt, args_t&&... src) volatile	{ return m_contents.promote_read_token(rt.m_readToken, wt.m_writeToken, std::forward<args_t>(src)...); }
	/// @}

	/// @{
	/// @brief Commits a write token, if the value has not changed since read.
	/// @param[in,out] wt write_token to write
	/// @return True if value was successfully committed
	bool end_write(write_token& wt) volatile								{ return m_contents.end_write(wt.m_writeToken); }
	/// @brief Commits a write token, if the value has not changed since read.
	/// @param[in,out] rt read_token associated with a write to perform
	/// @param src Value to write
	/// @return True if value was successfully committed
	template <typename... args_t>
	bool end_write(read_token& rt, args_t&&... src) volatile				{ return m_contents.end_write(rt.m_readToken, std::forward<args_t>(src)...); }
	/// @}

	/// @{
	/// @brief Checks if a read_token reflects the current state of the transactable.
	bool is_current(const read_token& rt) const volatile					{ return m_contents.is_current(rt.m_readToken); }
	/// @brief Checks if a write_token reflects the current state of the transactable.
	bool is_current(const write_token& wt) const volatile					{ return m_contents.is_current(wt.m_writeToken); }
	/// @}

	template <class functor_t>
	void write_retry_loop(functor_t&& fctr) volatile
	{
		write_token wt;
		do {
			begin_write(wt);
			fctr(*wt);
		} while (!end_write(wt));
	}

	template <class functor_t, class on_fail_t>
	void write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				fctr(*wt);
				if (end_write(wt))
					break;
			}
			onFail();
		}
	}

	template <class functor_t>
	auto write_retry_loop_pre(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			begin_write(wt);
			fctr(*wt);
			auto result = *wt;
			if (end_write(wt))
				return result;
		}
	}

	template <class functor_t, class on_fail_t>
	auto write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				fctr(*wt);
				auto result = *wt;
				if (end_write(wt))
					return result;
			}
			onFail();
		}
	}

	template <class functor_t>
	auto write_retry_loop_post(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			begin_write(wt);
			auto result = *wt;
			fctr(*wt);
			if (end_write(wt))
				return result;
		}
	}

	template <class functor_t, class on_fail_t>
	auto write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				auto result = *wt;
				fctr(*wt);
				if (end_write(wt))
					return result;
			}
			onFail();
		}
	}


	template <class functor_t>
	void try_write_retry_loop(functor_t&& fctr) volatile
	{
		write_token wt;
		do {
			begin_write(wt);
			if (!fctr(*wt))
				break;
		} while (!end_write(wt));
	}

	template <class functor_t, class on_fail_t>
	void try_write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				if (!fctr(*wt) || end_write(wt))
					break;
			}
			onFail();
		}
	}

	template <class functor_t>
	auto try_write_retry_loop_pre(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			begin_write(wt);
			bool b = fctr(*wt);
			auto result = *wt;
			if (!b || end_write(wt))
				return result;
		}
	}

	template <class functor_t, class on_fail_t>
	auto try_write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				bool b = fctr(*wt);
				auto result = *wt;
				if (!b || end_write(wt))
					return result;
			}
			onFail();
		}
	}

	template <class functor_t>
	auto try_write_retry_loop_post(functor_t&& fctr) volatile
	{
		write_token wt;
		for (;;)
		{
			begin_write(wt);
			auto result = *wt;
			if (!fctr(*wt) || end_write(wt))
				return result;
		}
	}

	template <class functor_t, class on_fail_t>
	auto try_write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile
	{
		for (;;)
		{
			{
				write_token wt;
				begin_write(wt);
				auto result = *wt;
				if (!fctr(*wt) || end_write(wt))
					return result;
			}
			onFail();
		}
	}


};


#pragma warning(pop)


}



#endif

