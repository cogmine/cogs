//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_DLINK
#define COGS_HEADER_COLLECTION_DLINK

#include <type_traits>

#include "cogs/collections/slink.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


template <class link_t, template <typename> class ref_type = ptr>
class default_dlink_iterator : public default_slink_iterator<link_t, ref_type>
{
public:
	typedef ref_type<link_t>	ref_t;

	static const          ref_t& get_prev(const          link_t& l)	{ return l.get_prev_link(); }
	static const volatile ref_t& get_prev(const volatile link_t& l)	{ return l.get_prev_link(); }

	static void set_prev(         link_t& l, const link_t& src)		{ l.set_prev_link(src); }
	static void set_prev(volatile link_t& l, const link_t& src)		{ l.set_prev_link(src); }
};


template <class link_t, template <typename> class ref_type = ptr, class link_iterator = default_dlink_iterator<link_t, ref_type> >
class dlink_methods
{
public:
	typedef ref_type<link_t>	ref_t;

	static const          ref_t& get_next(const          link_t& l)	{ return link_iterator::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l)	{ return link_iterator::get_next(l); }
	static const          ref_t& get_prev(const          link_t& l)	{ return link_iterator::get_prev(l); }
	static const volatile ref_t& get_prev(const volatile link_t& l)	{ return link_iterator::get_prev(l); }

	static void set_next(         link_t& l, const ref_t& src)		{ link_iterator::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src)		{ link_iterator::set_next(l, src); }
	static void set_prev(         link_t& l, const ref_t& src)		{ link_iterator::set_prev(l, src); }
	static void set_prev(volatile link_t& l, const ref_t& src)		{ link_iterator::set_prev(l, src); }

	static void remove(link_t& ths, const ref_t& terminator = ref_t())
	{
		if (get_next(ths) != terminator)
			get_next(ths)->set_prev(get_prev(ths));
		if (get_prev(ths) != terminator)
			get_prev(ths)->set_next(get_next(ths));
	}

	static ref_t remove_next(link_t& ths, const ref_t& terminator = ref_t())
	{
		ref_t next(get_next(ths));
		if (next != terminator)
		{
			ref_t nn = get_next(*next);
			set_next(ths, nn);
			if (nn != terminator)
				set_prev(*nn, get_prev(*next));	// get proper ref_t to ths
		}
		return next;
	}

	static ref_t remove_prev(link_t& ths, const ref_t& terminator = ref_t())
	{
		ref_t prev(get_prev(ths));
		if (prev != terminator)
		{
			ref_t pp = get_prev(*prev);
			set_prev(ths, pp);
			if (pp != terminator)
				set_next(*pp, get_next(*prev));	// get proper ref_t to ths
		}
		return prev;
	}

	static ref_t remove(link_t& ths, bool next, const ref_t& terminator = ref_t())
	{
		return next ? remove_next(ths, terminator) : remove_prev(ths, terminator);
	}

	static ref_t remove_next(const ref_t& cur, const ref_t& terminator = ref_t())
	{
		ref_t next(get_next(*cur));
		if (next != terminator)
		{
			ref_t nn = get_next(*next);
			set_next(*cur, nn);
			if (nn != terminator)
				set_prev(*nn, cur);
		}
		return next;
	}

	static ref_t remove_prev(const ref_t& cur, const ref_t& terminator = ref_t())
	{
		ref_t prev(get_prev(*cur));
		if (prev != terminator)
		{
			ref_t pp = get_prev(*prev);
			set_prev(*cur, pp);
			if (pp != terminator)
				set_next(*pp, cur);	// get proper ref_t to ths
		}
		return prev;
	}

	static ref_t remove(const ref_t& cur, bool next, const ref_t& terminator = ref_t())
	{
		return next ? remove_next(cur, terminator) : remove_prev(cur, terminator);
	}

	static ref_t find_last_terminated(const ref_t& cur, const ref_t& terminator = ref_t())	
	{
		ref_t last(cur);
		for (;;)
		{
			ref_t e = get_next(*last);
			if (e == terminator)
				break;
			last = e;
		}
		return last;
	}

	static ref_t find_first_terminated(const ref_t& cur, const ref_t& terminator = ref_t())	
	{
		ref_t last(cur);
		for (;;)
		{
			ref_t e = get_prev(*last);
			if (e == terminator)
				break;
			last = e;
		}
		return last;
	}

	static ref_t find_terminated(bool after, const ref_t& cur, const ref_t& terminator = ref_t())
	{
		return after ? find_last_terminated(cur, terminator) : find_first_terminator(cur, terminator);
	}

	static void insert_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())
	{
		ref_t next = get_next(*cur);
		set_next(*dl, next);
		set_prev(*dl, cur);
		if (next != terminator)
			set_prev(*next, dl);
		set_next(*cur, dl);
	}

	static void insert_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())
	{
		ref_t prev = get_prev(*cur);
		set_prev(*dl, prev);
		set_next(*dl, cur);
		if (prev != terminator)
			set_next(*prev, dl);
		set_prev(*cur, dl);
	}

	static void insert(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())
	{
		if (after)
			insert_after(cur, dl, terminator);
		else
			insert_before(cur, dl, terminator);
	}

	static void insert_list_after(const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())
	{
		set_prev(*first, cur);
		ref_t next = get_next(*cur);
		set_next(*cur, first);
		set_next(*last, next);
		if (next != terminator)
			set_prev(*next, last);
	}

	static void insert_list_before(const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())
	{
		set_next(*last, cur);
		ref_t prev = get_prev(*cur);
		set_prev(*cur, last);
		set_prev(*first, prev);
		if (prev != terminator)
			set_next(*prev, first);
	}

	static void insert_list(bool after, const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())
	{
		if (after)
			insert_list_after(cur, first, last, terminator);
		else
			insert_list_before(cur, first, last, terminator);
	}

	static void insert_terminated_list_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())			{ insert_list_after(cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }
	static void insert_terminated_list_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())			{ insert_list_before(cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }
	static void insert_terminated_list(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ insert_list(after, cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }

	static void insert_circular_list_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())		{ insert_list_after(cur, dl, dl->get_prev_link(), terminator); }
	static void insert_circular_list_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())		{ insert_list_before(cur, dl, dl->get_prev_link(), terminator); }
	static void insert_circular_list(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ insert_list(after, cur, dl, dl->get_prev_link(), terminator); }

	static void insert_into_circular_after(const ref_t& cur, const ref_t& dl)
	{
		ref_t next(get_next(*cur));
		set_next(*dl, next);
		set_next(*cur, dl);
		set_prev(*dl, cur);
		set_prev(*next, dl);
	}

	static void insert_into_circular_before(const ref_t& cur, const ref_t& dl)
	{
		ref_t prev(get_prev(*cur));
		set_prev(*dl, prev);
		set_prev(*cur, dl);
		set_next(*dl, cur);
		set_next(*prev, dl);
	}

	static void insert_into_circular(bool after, const ref_t& cur, const ref_t& dl)
	{
		if (after)
			insert_into_circular_after(cur, dl);
		else
			insert_into_circular_before(cur, dl);
	}

	static void insert_list_into_circular_after(const ref_t& cur, const ref_t& new_first, const ref_t& new_last)
	{
		ref_t next(get_next(*cur));
		set_prev(*next, new_last);
		set_next(*cur, new_first);
		set_next(*new_last, next);
		set_prev(*new_first, cur);
	}

	static void insert_list_into_circular_before(const ref_t& cur, const ref_t& new_first, const ref_t& new_last)
	{
		ref_t prev(get_prev(*cur));
		set_next(*prev, new_first);
		set_prev(*cur, new_last);
		set_prev(*new_first, prev);
		set_next(*new_last, cur);
	}

	static void insert_list_into_circular(bool after, const ref_t& cur, const ref_t& new_first, const ref_t& new_last)
	{
		if (after)
			insert_list_into_circular_after(cur, new_first, new_last);
		else
			insert_list_into_circular_before(cur, new_first, new_last);
	}

	static void insert_terminated_list_into_circular(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ insert_list_into_circular(after, cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }
	static void insert_terminated_list_into_circular_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ insert_list_into_circular_after(cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }
	static void insert_terminated_list_into_circular_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ insert_list_into_circular_before(cur, cur, find_first_terminated(dl, terminator), find_last_terminated(dl, terminator)); }

	static void insert_circular_list_into_circular(bool after, const ref_t& cur, const ref_t& dl)	{ insert_list_into_circular(after, cur, dl, dl->get_prev_link()); }
	static void insert_circular_list_into_circular_after(const ref_t& cur, const ref_t& dl)			{ insert_list_into_circular_after(cur, dl, dl->get_prev_link()); }
	static void insert_circular_list_into_circular_before(const ref_t& cur, const ref_t& dl)		{ insert_list_into_circular_before(cur, dl, dl->get_prev_link()); }
};


template <class derived_t, template <typename> class ref_type = ptr, class link_iterator = default_dlink_iterator<derived_t, ref_type> >
class dlink_base
{
public:
	typedef dlink_base<derived_t, ref_type, link_iterator>						this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t>	link_t;
	typedef ref_type<link_t>													ref_t;
	typedef dlink_methods<link_t, ref_type, link_iterator>						dlink_methods_t;

	static const          ref_t& get_next(const          link_t& l)	{ return link_iterator::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l)	{ return link_iterator::get_next(l); }
	static const          ref_t& get_prev(const          link_t& l)	{ return link_iterator::get_prev(l); }
	static const volatile ref_t& get_prev(const volatile link_t& l)	{ return link_iterator::get_prev(l); }

	static void set_next(         link_t& l, const ref_t& src)		{ link_iterator::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src)		{ link_iterator::set_next(l, src); }
	static void set_prev(         link_t& l, const ref_t& src)		{ link_iterator::set_prev(l, src); }
	static void set_prev(volatile link_t& l, const ref_t& src)		{ link_iterator::set_prev(l, src); }

	static void remove(link_t& ths, const ref_t& terminator = ref_t())					{ dlink_methods_t::remove(ths, terminator); }

	static ref_t remove(link_t& ths, bool next, const ref_t& terminator = ref_t())		{ return dlink_methods_t::remove(ths, next, terminator); }
	static ref_t remove_next(link_t& ths, const ref_t& terminator = ref_t())			{ return dlink_methods_t::remove_next(ths, terminator); }
	static ref_t remove_prev(link_t& ths, const ref_t& terminator = ref_t())			{ return dlink_methods_t::remove_prev(ths. terminator); }

	static ref_t remove(const ref_t& cur, bool next, const ref_t& terminator = ref_t())				{ return dlink_methods_t::remove(cur, next, terminator); }
	static ref_t remove_next(const ref_t& cur, const ref_t& terminator = ref_t())					{ return dlink_methods_t::remove_next(cur, terminator); }
	static ref_t remove_prev(const ref_t& cur, const ref_t& terminator = ref_t())					{ return dlink_methods_t::remove_prev(cur, terminator); }
	static ref_t find_terminated(bool after, const ref_t& cur, const ref_t& terminator = ref_t())	{ return dlink_methods_t::find_terminated(after, cur, terminator); }
	static ref_t find_last_terminated(const ref_t& cur, const ref_t& terminator = ref_t())			{ return dlink_methods_t::find_last_terminated(cur, terminator); }
	static ref_t find_first_terminated(const ref_t& cur, const ref_t& terminator = ref_t())			{ return dlink_methods_t::find_first_terminated(cur, terminator); }

	static void insert(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ dlink_methods_t::insert(after, cur, dl, terminator); }
	static void insert_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())			{ dlink_methods_t::insert_after(cur, dl, terminator); }
	static void insert_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())			{ dlink_methods_t::insert_before(cur, dl, terminator); }
	static void insert_list(bool after, const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())	{ dlink_methods_t::insert_list(after, cur, first, last, terminator); }
	static void insert_list_after(const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())		{ dlink_methods_t::insert_list_after(cur, first, last, terminator); }
	static void insert_list_before(const ref_t& cur, const ref_t& first, const ref_t& last, const ref_t& terminator = ref_t())		{ dlink_methods_t::insert_list_before(cur, first, last, terminator); }
	static void insert_terminated_list(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())			{ dlink_methods_t::insert_terminated_list(after, cur, dl, terminator); }
	static void insert_terminated_list_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())					{ dlink_methods_t::insert_terminated_list_after(cur, dl, terminator); }
	static void insert_terminated_list_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())					{ dlink_methods_t::insert_terminated_list_before(cur, dl, terminator); }
	static void insert_circular_list(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())				{ dlink_methods_t::insert_circular_list(after, cur, dl, terminator); }
	static void insert_circular_list_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())					{ dlink_methods_t::insert_circular_list_after(cur, dl, terminator); }
	static void insert_circular_list_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())					{ dlink_methods_t::insert_circular_list_before(cur, dl, terminator); }
	static void insert_into_circular(bool after, const ref_t& cur, const ref_t& dl)													{ dlink_methods_t::insert_into_circular(after, cur, dl); }
	static void insert_into_circular_after(const ref_t& cur, const ref_t& dl)														{ dlink_methods_t::insert_into_circular_after(cur, dl); }
	static void insert_into_circular_before(const ref_t& cur, const ref_t& dl)														{ dlink_methods_t::insert_into_circular_before(cur, dl); }
	static void insert_list_into_circular(bool after, const ref_t& cur, const ref_t& new_first, const ref_t& new_last)				{ dlink_methods_t::insert_list_into_circular(after, cur, new_first, new_last); }
	static void insert_list_into_circular_after(const ref_t& cur, const ref_t& new_first, const ref_t& new_last)					{ dlink_methods_t::insert_list_into_circular_after(cur, new_first, new_last); }
	static void insert_list_into_circular_before(const ref_t& cur, const ref_t& new_first, const ref_t& new_last)					{ dlink_methods_t::insert_list_into_circular_before(cur, new_first, new_last); }
	static void insert_terminated_list_into_circular(bool after, const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ dlink_methods_t::insert_terminated_list_into_circular(after, cur, dl, terminator); }
	static void insert_terminated_list_into_circular_after(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ dlink_methods_t::insert_terminated_list_into_circular_after(cur, dl, terminator); }
	static void insert_terminated_list_into_circular_before(const ref_t& cur, const ref_t& dl, const ref_t& terminator = ref_t())	{ dlink_methods_t::insert_terminated_list_into_circular_before(cur, dl, terminator); }
	static void insert_circular_list_into_circular(bool after, const ref_t& cur, const ref_t& dl)									{ dlink_methods_t::insert_circular_list_into_circular(after, cur, dl); }
	static void insert_circular_list_into_circular_after(const ref_t& cur, const ref_t& dl)											{ dlink_methods_t::insert_circular_list_into_circular_after(cur, dl); }
	static void insert_circular_list_into_circular_before(const ref_t& cur, const ref_t& dl)										{ dlink_methods_t::insert_circular_list_into_circular_before(cur, dl); }
};


/// @ingroup Collections
/// @brief A double-link list element
/// @tparam derived_t Derived type of this class.  Allows links to be returned as references to the derived type, without requiring a cast.
/// If void is specified, links will point to dlink_t<void, ref_type, link_iterator>.  Default: void
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_iterator Helper type providing functions to get and set links.  Default: default_dlink_iterator<derived_t, ref_type>
template <class derived_t = void, template <typename> class ref_type = ptr, class link_iterator = default_dlink_iterator<derived_t, ref_type> >
class dlink_t : public dlink_base<derived_t, ref_type, link_iterator>
{
public:
	typedef dlink_t<derived_t, ref_type, link_iterator>							this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t>	link_t;
	typedef ref_type<link_t>													ref_t;

private:
	ref_t m_next;
	ref_t m_prev;

	dlink_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	dlink_t()
	{ }

	template <typename N, typename P>
	dlink_t(N&& n, P&& p)
		: m_next(std::forward<N>(n)),
		m_prev(std::forward<P>(p))
	{ }

	dlink_t(this_t&& t)
	{
		m_next = std::move(t.m_next);
		m_prev = std::move(t.m_prev);
	}

	dlink_t& operator=(this_t&& t)
	{
		m_next = std::move(t.m_next);
		m_prev = std::move(t.m_prev);
		return *this;
	}

	               ref_t& get_next_link()						{ return m_next; }
	const          ref_t& get_next_link() const					{ return m_next; }
	      volatile ref_t& get_next_link()       volatile		{ return m_next; }
	const volatile ref_t& get_next_link() const volatile		{ return m_next; }
	
		           ref_t& get_prev_link()						{ return m_prev; }
	const          ref_t& get_prev_link() const					{ return m_prev; }
	      volatile ref_t& get_prev_link() volatile				{ return m_prev; }
	const volatile ref_t& get_prev_link() const volatile		{ return m_prev; }

	void set_prev_link(const          ref_t& p)					{ m_prev = p; }
	void set_prev_link(const volatile ref_t& p)					{ m_prev = p; }
	void set_prev_link(const          ref_t& p) volatile		{ m_prev = p; }
	void set_prev_link(const volatile ref_t& p) volatile		{ m_prev = p; }

	void set_next_link(const          ref_t& n)					{ m_next = n; }
	void set_next_link(const volatile ref_t& n)					{ m_next = n; }
	void set_next_link(const          ref_t& n) volatile		{ m_next = n; }
	void set_next_link(const volatile ref_t& n) volatile		{ m_next = n; }

	void set_link(bool next, const          ref_t& n)			{ if (next) set_next_link(n); else set_prev_link(n); }
	void set_link(bool next, const volatile ref_t& n)			{ if (next) set_next_link(n); else set_prev_link(n); }
	void set_link(bool next, const          ref_t& n) volatile	{ if (next) set_next_link(n); else set_prev_link(n); }
	void set_link(bool next, const volatile ref_t& n) volatile	{ if (next) set_next_link(n); else set_prev_link(n); }

		           ref_t& get_link(bool next)					{ if (next) return get_next_link(); return get_prev_link(); }
	const          ref_t& get_link(bool next) const				{ if (next) return get_next_link(); return get_prev_link(); }
	      volatile ref_t& get_link(bool next) volatile			{ if (next) return get_next_link(); return get_prev_link(); }
	const volatile ref_t& get_link(bool next) const volatile	{ if (next) return get_next_link(); return get_prev_link(); }
};


template <template <typename> class ref_type>
class default_dlink_iterator<void, ref_type> : public default_dlink_iterator<dlink_t<void, ref_type, default_dlink_iterator<void, ref_type> > >
{
};


typedef dlink_t<void> dlink;


#pragma warning(pop)


}


#endif

