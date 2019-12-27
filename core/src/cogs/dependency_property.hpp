//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting, MayNeedCleanup

#ifndef COGS_HEADER_BINDABLE_PROPERTY
#define COGS_HEADER_BINDABLE_PROPERTY


#include "cogs/function.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/io/permission.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


// dependency_property combines the concept of a property (supporting get() and set() functions that can be 
// customized) with the concept of databinding (automatically notifying dependent properties when a property
// changes).
//
// dependency_property<> is the interface class, however all instantiated properties are derived from virtual_dependency_property<>,
// which handles processing of change notifications.
//
// Certain constraints exist to handle parallelism issues.
//	- A call to get() always occurs synchronously, in the invoking thread.
//		- If an asychronous get is required, consider binding a dependency_property, as its set() will be invoked asynchronously.
//	- Parallel calls to set(), and changed() will be serialized.
//		All direct calls to set() will be queued/processed.  Updates resulting from a changed() will be coalesced, or omitted
//		entirely if a subsequent direct call to set() occurs.


// Interface class for dependency_property.
//    Most references should be to rcptr<dependency_property<type, readWriteType> >
template <typename type, io::permission readWriteType = io::read_write>
class dependency_property;

// Base class to use when deriving dependency_property types (implementing virtual setting() and/or getting()
//    Calls to set() are asynchronous and serialized.  User implementation of setting() must call set_complete() to indicate completion.
template <typename type, io::permission readWriteType = io::read_write>
class virtual_dependency_property;

// A concrete class for delegate-based properties.
template <typename type, io::permission readWriteType = io::read_write>
class delegated_dependency_property;

// A concrete class for a backed dependency_property.
//    Simply wraps a variable in a dependency_property.
template <typename type, io::permission readWriteType = io::read_write>
class backed_dependency_property;

template <typename type>
class virtual_dependency_property_base;

// dependency_property<> is used as an interface only.
// Derived properties should be derived from virtual_dependency_property.
template <typename type, io::permission readWriteType> // read_write
class dependency_property : public dependency_property<type, io::read_only>, public dependency_property<type, io::write_only>
{
public:
	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() = 0;

	virtual void bind(const rcref<dependency_property<type, io::read_write> >& src, bool useThisValue = false) = 0;
	virtual void bind_to(const rcref<dependency_property<type, io::write_only> >& snk) = 0;
	virtual void bind_from(const rcref<dependency_property<type, io::read_only> >& src) = 0;

	type get() const { return getting(); } // inline for API symmetry with set/setting

	virtual type getting() const = 0; // User override, if virtual dependency_property

	virtual void changed() = 0;

	virtual void set(const type& t) = 0; // Overloaded by virtual_dependency_property to defer setting() to appropriate thread

	virtual void setting(const type& t) = 0; // User override, if virtual_dependency_property
};

template <typename type>
class dependency_property<type, io::read_only>
{
public:
	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() = 0;

	virtual void bind_to(const rcref<dependency_property<type, io::write_only> >& snk) = 0;

	type get() const { return getting(); } // inline for API symmetry with set/setting

	virtual type getting() const = 0; // User override, if virtual dependency_property

	virtual void changed() = 0;

	template <typename F>
	rcref<dependency_property<type, io::write_only> > on_changed(const rcref<volatile dispatcher>& d, F&& f);
};

template <typename type>
class dependency_property<type, io::write_only>
{
public:
	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() = 0;

	virtual void bind_from(const rcref<dependency_property<type, io::read_only> >& src) = 0;

	virtual void set(const type& t) = 0; // Overloaded by virtual_dependency_property to defer setting() to appropriate thread

	virtual void setting(const type& t) = 0; // User override, if virtual_dependency_property
};


template <typename type>
class virtual_dependency_property_base : public object
{
private:
	typedef virtual_dependency_property_base<type> this_t;

	class binding
	{
	public:
		weak_rcptr<this_t> m_sink;

		binding(const rcref<this_t>& snk)
			: m_sink(snk)
		{ }
	};

	class notification_context
	{
	public:
		type m_cachedValue;
		weak_rcptr<this_t> m_updateSource;

		notification_context(const type& t, const rcptr<this_t>& src)
			: m_cachedValue(t),
			m_updateSource(src)
		{ }
	};

	weak_rcptr<volatile dispatcher> m_dispatcher;

	volatile container_dlist<rcref<binding> > m_bindings;
	volatile container_queue<rcref<notification_context> > m_setQueue;
	volatile boolean m_pendingChange;

	rcptr<notification_context> m_lastBoundSet;

	// 0 = not dispatching, 1 = processing dispatch, 2 = pending dispatch
	alignas (atomic::get_alignment_v<int>) volatile int m_pumpSerializer = 0;

	void set_changed_pump()
	{
		atomic::store(m_pumpSerializer, 1);
		for (;;)
		{
			rcptr<notification_context> ctx;
			m_setQueue.pop_first(ctx.dereference());
			if (!!ctx)
			{
				m_lastBoundSet.release();
				if (!!ctx->m_updateSource)
				{
					if (!m_setQueue.is_empty()) // Ignore change()-triggered set if not the most recent set operation
						continue;
					m_lastBoundSet = ctx;
				}
				setting(ctx->m_cachedValue);
				break;
			}

			if (!!m_pendingChange)
			{
				m_pendingChange = false;
				type value = getting();
				typename container_dlist<rcref<binding> >::volatile_iterator itor = m_bindings.get_first();
				while (!!itor)
				{
					rcptr<this_t> snk = (*itor)->m_sink;
					if (!!snk)
					{
						if (!m_lastBoundSet || (m_lastBoundSet->m_updateSource != snk) || (m_lastBoundSet->m_cachedValue != value))
							snk->bound_set(value, this_rcref);
						++itor;
					}
					else
					{
						typename container_dlist<rcref<binding> >::volatile_iterator itor2 = itor;
						++itor;
						m_bindings.remove(itor2);
					}
				}
			}

			m_lastBoundSet.release();

			int newPumpSerializer = pre_assign_prev(m_pumpSerializer);
			if (newPumpSerializer == 0)
				break;
		}
	}

	void start_pump()
	{
		int oldPumpSerializer;
		atomic::exchange(m_pumpSerializer, 2, oldPumpSerializer);
		if (oldPumpSerializer == 0)
		{
			dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<this_t> r2 = r;
				if (!!r2)
					r2->set_changed_pump();
			});
		}
	}

	void set_inner(const type& t, const rcptr<this_t>& src)
	{
		rcref<notification_context> ctx = rcnew(notification_context, t, src);
		m_setQueue.append(ctx);
		start_pump();
	}

	void dispatch(const function<void()>& d)
	{
		rcptr<volatile dispatcher> dsptch = m_dispatcher;
		if (!dsptch)
			thread_pool::get_default_or_immediate()->dispatch(d);
		else
			dsptch->dispatch(d);
	}

protected:

	explicit virtual_dependency_property_base(rc_obj_base& desc)
		: object(desc)
	{ }

	virtual_dependency_property_base(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: object(desc),
		m_dispatcher(d)
	{ }

	void changed()
	{
		if (!!m_bindings) // NOP if nothing bound
		{
			m_pendingChange = true;
			start_pump();
		}
	}

	void bound_set(const type& t, const rcref<this_t>& src) { return set_inner(t, src); }

	void set(const type& t) { return set_inner(t, 0); }

	void set_complete()
	{
		dispatch([r{ this_weak_rcptr }]()
		{
			rcptr<this_t> r2 = r;
			if (!!r2)
				r2->set_changed_pump();
		});
	}

	void set_complete(bool hasChanged)
	{
		if (hasChanged)
			changed();
		set_complete();
	}

	virtual void setting(const type& t) = 0;
	virtual type getting() const = 0;

	// If both are read_write, the value of src will be used
	static void bind2(const rcref<this_t>& src, const rcref<this_t>& snk, bool update)
	{
		rcref<binding> bindingObj = rcnew(binding, snk);
		src->m_bindings.append(bindingObj);

		if (update) // defer an update of just this binding
			snk->bound_set(src->getting(), src);
	}

	void bind_to(const rcref<this_t>& snk, bool update = true)
	{
		bind2(this_rcref, snk, update);
	}

	void bind_from(const rcref<this_t>& src, bool update = true)
	{
		bind2(src, this_rcref, update);
	}

	void bind(const rcref<this_t>& prop, bool useThisValue = false)
	{
		bind_from(prop, !useThisValue);
		bind_to(prop, useThisValue);
	}
};


template <typename type, io::permission readWriteType>
class virtual_dependency_property : public dependency_property<type, readWriteType>, public virtual_dependency_property_base<type>
{
private:
	typedef virtual_dependency_property<type, readWriteType> this_t;
	typedef virtual_dependency_property_base<type> base_t;

	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() { return this_rcref; }

public:
	explicit virtual_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	virtual_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc, d)
	{ }

	virtual void bind(const rcref<dependency_property<type, io::read_write> >& srcSnk, bool useThisValue = false)
	{
		base_t::bind(srcSnk->get_virtual_dependency_property_base(), useThisValue);
	}

	virtual void bind_to(const rcref<dependency_property<type, io::write_only> >& snk)
	{
		base_t::bind_to(snk->get_virtual_dependency_property_base());
	}

	virtual void bind_from(const rcref<dependency_property<type, io::read_only> >& src)
	{
		base_t::bind_from(src->get_virtual_dependency_property_base());
	}

	virtual type getting() const = 0;

	virtual void changed() { base_t::changed(); }

	virtual void set(const type& t) { base_t::set(t); }

	virtual void setting(const type& t) { base_t::set_complete(); }

	void set_complete() { base_t::set_complete(); }
	void set_complete(bool hasChanged) { base_t::set_complete(hasChanged); }
};

template <typename type>
class virtual_dependency_property<type, io::read_only> : public dependency_property<type, io::read_only>, public virtual_dependency_property_base<type>
{
private:
	typedef virtual_dependency_property<type, io::read_only> this_t;
	typedef virtual_dependency_property_base<type> base_t;

	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() { return this_rcref; }

	virtual void setting(const type& t) { }

public:
	explicit virtual_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	virtual_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc, d)
	{ }

	virtual void bind_to(const rcref<dependency_property<type, io::write_only> >& snk)
	{
		base_t::bind_to(snk->get_virtual_dependency_property_base());
	}

	virtual type getting() const = 0;

	virtual void changed() { base_t::changed(); }
};

template <typename type>
class virtual_dependency_property<type, io::write_only> : public dependency_property<type, io::write_only>, public virtual_dependency_property_base<type>
{
private:
	typedef virtual_dependency_property<type, io::write_only> this_t;
	typedef virtual_dependency_property_base<type> base_t;

	virtual rcref<virtual_dependency_property_base<type> > get_virtual_dependency_property_base() { return this_rcref; }

	virtual type getting() const
	{
		// Placeholder.  It's an error to try to get the value of a write-only property
		COGS_ASSERT(false);
		type* unused = 0;
		return *unused;
	}

public:
	explicit virtual_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	virtual_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc, d)
	{ }

	virtual void bind_from(const rcref<dependency_property<type, io::read_only> >& src)
	{
		base_t::bind_from(src->get_virtual_dependency_property_base());
	}

	virtual void set(const type& t) { base_t::set(t); }

	virtual void setting(const type& t) { base_t::changed(); }

	void set_complete() { base_t::set_complete(); }
	void set_complete(bool hasChanged) { base_t::set_complete(hasChanged); }
};


template <typename type>
class delegated_dependency_property<type, io::read_write> : public virtual_dependency_property<type, io::read_write>
{
private:
	typedef delegated_dependency_property<type, io::read_write> this_t;
	typedef virtual_dependency_property<type, io::read_write> base_t;

public:
	typedef function<void(const type&,const rcref<dependency_property<type, io::write_only> >&)> set_delegate_type;
	typedef function<type()> get_delegate_type;

private:
	get_delegate_type m_getDelegate;
	set_delegate_type m_setDelegate;

public:
	delegated_dependency_property(rc_obj_base& desc, const get_delegate_type& g, const set_delegate_type& s)
		: base_t(desc),
		m_getDelegate(g),
		m_setDelegate(s)
	{ }

	delegated_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const get_delegate_type& g, const set_delegate_type& s)
		: base_t(desc, d),
		m_getDelegate(g),
		m_setDelegate(s)
	{ }

	virtual type getting() const { return m_getDelegate(); }

	virtual void setting(const type& t)
	{
		m_setDelegate(t, this_rcref);
	}
};


template <typename type>
class delegated_dependency_property<type, io::read_only> : public virtual_dependency_property<type, io::read_only>
{
private:
	typedef delegated_dependency_property<type, io::read_only> this_t;
	typedef virtual_dependency_property<type, io::read_only> base_t;

public:
	typedef function<type()> get_delegate_type;

private:
	get_delegate_type m_getDelegate;

public:
	delegated_dependency_property(rc_obj_base& desc, const get_delegate_type& g)
		: base_t(desc),
		m_getDelegate(g)
	{ }

	delegated_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const get_delegate_type& g)
		: base_t(desc, d),
		m_getDelegate(g)
	{ }

	virtual type getting() const { return m_getDelegate(); }
};

template <typename type>
class delegated_dependency_property<type, io::write_only> : public virtual_dependency_property<type, io::write_only>
{
private:
	typedef delegated_dependency_property<type, io::write_only> this_t;
	typedef virtual_dependency_property<type, io::write_only> base_t;

public:
	typedef function<void(const type&, const rcref<dependency_property<type, io::write_only> >&)> set_delegate_type;

private:
	set_delegate_type m_setDelegate;

public:
	delegated_dependency_property(rc_obj_base& desc, const set_delegate_type& s)
		: base_t(desc),
		m_setDelegate(s)
	{ }

	delegated_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const set_delegate_type& s)
		: base_t(desc, d),
		m_setDelegate(s)
	{ }

	virtual void setting(const type& t)
	{
		m_setDelegate(t, this_rcref);
	}
};

template <typename type>
class backed_dependency_property<type, io::read_write> : public virtual_dependency_property<type, io::read_write>
{
private:
	typedef backed_dependency_property<type, io::read_write> this_t;
	typedef virtual_dependency_property<type, io::read_write> base_t;

	typedef transactable<type> transactable_t;
	volatile transactable_t m_contents;

public:
	explicit backed_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	backed_dependency_property(rc_obj_base& desc, const type& t)
		: base_t(desc),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc, d)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const type& t)
		: base_t(desc, d),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	virtual type getting() const { return *(m_contents.begin_read()); }

	virtual void setting(const type& t)
	{
		m_contents.set(t);
		virtual_dependency_property<type, io::read_write>::set_complete(true);
	}
};

template <typename type>
class backed_dependency_property<type, io::read_only> : public virtual_dependency_property<type, io::read_only>
{
private:
	typedef backed_dependency_property<type, io::read_only> this_t;
	typedef virtual_dependency_property<type, io::read_only> base_t;

	typedef transactable<type> transactable_t;
	volatile transactable_t m_contents;

public:
	explicit backed_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	backed_dependency_property(rc_obj_base& desc, const type& t)
		: base_t(desc),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const type& t)
		: base_t(desc),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	virtual type getting() const { return m_contents; }
};

template <typename type>
class backed_dependency_property<type, io::write_only> : public virtual_dependency_property<type, io::write_only>
{
private:
	typedef backed_dependency_property<type, io::write_only> this_t;
	typedef virtual_dependency_property<type, io::write_only> base_t;

	typedef transactable<type> transactable_t;
	volatile transactable_t m_contents;

public:
	explicit backed_dependency_property(rc_obj_base& desc)
		: base_t(desc)
	{ }

	backed_dependency_property(rc_obj_base& desc, const type& t)
		: base_t(desc),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d)
		: base_t(desc, d)
	{ }

	backed_dependency_property(rc_obj_base& desc, const rcref<volatile dispatcher>& d, const type& t)
		: base_t(desc, d),
		m_contents(typename transactable_t::construct_embedded_t(), t)
	{ }

	virtual void setting(const type& t)
	{
		m_contents.set(t);
		virtual_dependency_property<type, io::write_only>::set_complete(true);
	}
};

template <typename type>
template <typename F>
inline rcref<dependency_property<type, io::write_only> > dependency_property<type, io::read_only>::on_changed(const rcref<volatile dispatcher>& d, F&& f)
{
	typedef delegated_dependency_property<type, io::write_only> property_t;
	rcref<dependency_property<type, io::write_only> > result = rcnew(property_t, d, [f{ std::move(f) }](const type& t, const rcref<dependency_property<type, io::write_only> >& p)
	{
		f(t, p);
	}).template static_cast_to<dependency_property<type, io::write_only> >();
	bind_to(result);
	return result;
}


}


#endif
