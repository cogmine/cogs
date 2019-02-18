//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_SLINK
#define COGS_HEADER_COLLECTION_SLINK

#include <type_traits>

#include "cogs/mem/ptr.hpp"

namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


template <class link_t, template <typename> class ref_type = ptr>
class default_slink_iterator
{
public:
	typedef ref_type<link_t>	ref_t;

	static const          ref_t& get_next(const          link_t& l)	{ return l.get_next_link(); }
	static const volatile ref_t& get_next(const volatile link_t& l)	{ return l.get_next_link(); }

	static void set_next(         link_t& l, const ref_t& src)		{ l.set_next_link(src); }
	static void set_next(volatile link_t& l, const ref_t& src)		{ l.set_next_link(src); }
};


template <class link_t, template <typename> class ref_type = ptr, class link_iterator = default_slink_iterator<link_t, ref_type> >
class slink_methods
{
public:
	typedef ref_type<link_t>	ref_t;

	static const          ref_t& get_next(const          link_t& l)	{ return link_iterator::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l)	{ return link_iterator::get_next(l); }

	static void set_next(         link_t& l, const ref_t& src)		{ link_iterator::set_next_link(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src)		{ link_iterator::set_next_link(l, src); }

	static void insert_next(link_t& ths, const ref_t& l)
	{
		set_next(*l, get_next(ths));
		set_next(ths, l);
	}

	static ref_t remove_next(link_t& ths)
	{
		ref_t l(get_next(ths));
		set_next(ths, get_next(*l));
		return l;
	}

	static void insert_segment(link_t& ths, const ref_t& seq_start, const ref_t& seq_end)
	{
		set_next(*seq_end, get_next(ths));
		set_next(ths, seq_start);
	}

	static ref_t insert_terminated_list(link_t& ths, const ref_t& l, const ref_t& terminator = ref_t())
	{
		ref_t last(find_last_terminated(ths, l, terminator));
		set_next(*last, get_next(ths));
		set_next(ths, l);
		return last;
	}

	static ref_t insert_circular_list(link_t& ths, const ref_t& l)
	{
		ref_t last(find_last_circular(l));
		set_next(*last, get_next(ths));
		set_next(ths, l);
		return last;
	}

	static ref_t insert_list(link_t& ths, const ref_t& l, const ref_t& terminator = ref_t())
	{
		ref_t last(find_last(l, terminator));	// supports terminator or full circular
		set_next(*last, get_next(ths));
		set_next(ths, l);
		return last;
	}

	static bool is_circular(const link_t& ths, const ref_t& terminator = ref_t())
	{
		bool result = false;
		size_t count = 0;
		size_t loop_at = 1;
		ref_t loop = ths;
		ref_t cur(get_next(ths));
		for(;;)
		{
			if (terminator != cur)
			{
				if (loop != cur)
				{
					if (++count == loop_at)
					{
						loop_at <<= 1;
						count = 0;
						loop = cur;
					}
					cur = get_next(*cur);
					continue;
				}
				result = true;
			}
			break;
		}
		return result;
	}

	static bool is_full_circular(const link_t& ths, const ref_t& terminator = ref_t())
	{
		bool result = false;
		size_t count = 0;
		size_t loop_at = 1;
		ref_t loop;
		ref_t cur(get_next(ths));
		for(;;)
		{
			if (terminator != cur)
			{
				if (cur != ths)
				{
					if (++count == loop_at)
					{
						loop_at <<= 1;
						count = 0;
						loop = cur;
					}
					cur = get_next(*cur);
					if (loop == cur)
						break;
					continue;
				}
				result = true;
			}
			break;
		}
		return result;
	}

	static bool is_tail_circular(const link_t& ths, const ref_t& terminator = ref_t())
	{
		bool result = false;
		size_t count = 0;
		size_t loop_at = 1;
		ref_t loop;
		ref_t cur(get_next(ths));
		for(;;)
		{
			if (terminator != cur)
			{
				if (ths != cur)
				{
					if (++count == loop_at)
					{
						loop_at <<= 1;
						count = 0;
						loop = cur;
					}
					cur = get_next(*cur);
					if (loop != cur)
						continue;
					result = true;
				}
			}
			break;
		}
		return result;
	}

	static ref_t find_last(const ref_t& l, const ref_t& terminator = ref_t())
	{
		ref_t last(l);
		for (;;)
		{
			ref_t cur(get_next(*last));
			if ((cur != l) && (cur != terminator))
			{
				last = cur;
				continue;
			}
			break;
		}
		return last;
	}

	static ref_t find_last_terminated(const ref_t& l, const ref_t& terminator = ref_t())
	{
		ref_t last(l);
		for (;;)
		{
			ref_t cur(get_next(*last));
			if (cur != terminator)
			{
				last = cur;
				continue;
			}
			break;
		}
		return last;
	}

	static ref_t find_last_circular(const ref_t& l)
	{
		ref_t last(l);
		for (;;)
		{
			ref_t cur(get_next(*last));
			if (cur != l)
			{
				last = cur;
				continue;
			}
			break;
		}
		return last;
	}

	static ref_t reverse(const ref_t& l)
	{
		if (!l)
			return l;

		ref_t cur(l);
		ref_t prev;
		ref_t prevprev;
		do {
			prevprev = prev;
			prev = cur;
			cur = get_next(*cur);
			get_next(*prev) = prevprev;
		} while (!!cur);
		return prev;

	}
};


template <class derived_t, template <typename> class ref_type = ptr, class link_iterator = default_slink_iterator<derived_t, ref_type> >
class slink_base
{
public:
	typedef slink_base<derived_t, ref_type, link_iterator>						this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t>	link_t;
	typedef ref_type<link_t>													ref_t;
	typedef slink_methods<link_t, ref_type, link_iterator>						slink_methods_t;

	// non-volatile misc
	void insert_next(const ref_t& l)														{ slink_methods_t::insert_next(*(derived_t*)this, l); }
	ref_t remove_next()																		{ return  slink_methods_t::remove_next(*(derived_t*)this); }
	void insert_segment(const ref_t& seq_start, const ref_t& seq_end)						{ slink_methods_t::insert_segment(*(derived_t*)this, seq_start, seq_end); }
	ref_t insert_terminated_list(const ref_t& l, const ref_t& terminator = ref_t())			{ return slink_methods_t::insert_terminated_list(*(derived_t*)this, l, terminator); }
	ref_t insert_circular_list(const ref_t& l)												{ return slink_methods_t::insert_circular_list(*(derived_t*)this, l); }
	ref_t insert_list(const ref_t& l, const ref_t& terminator = ref_t())					{ return slink_methods_t::insert_list(*(derived_t*)this, l, terminator); }
	
	bool is_circular(const ref_t& terminator = ref_t()) const								{ return slink_methods_t::is_circular(*(derived_t*)this, terminator); }
	bool is_full_circular(const ref_t& terminator = ref_t()) const							{ return slink_methods_t::is_full_circular(*(derived_t*)this, terminator); }
	bool is_tail_circular(const ref_t& terminator = ref_t()) const							{ return slink_methods_t::is_tail_circular(*(derived_t*)this, terminator); }

	static ref_t find_last(const ref_t& l, const ref_t& terminator = ref_t())				{ return slink_methods_t::find_last(l, terminator); }
	static ref_t find_last_terminated(const ref_t& l, const ref_t& terminator = ref_t())	{ return slink_methods_t::find_last_terminated(l, terminator); }
	static ref_t find_last_circular(const ref_t& l)											{ return slink_methods_t::find_last_circular(l); }
	static ref_t reverse(const ref_t& l)													{ return slink_methods_t::reverse(l); }
};


/// @ingroup Collections
/// @brief A single-link list element
/// @tparam derived_t Derived type of this class.  Allows links to be returned as references to the derived type, without requiring a cast.
/// If void is specified, links will point to slink_t<void, ref_type, link_iterator>.  Default: void
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_iterator Helper type providing functions to get and set links.  Default: default_slink_iterator<derived_t, ref_type>
template <class derived_t = void, template <typename> class ref_type = ptr, class link_iterator = default_slink_iterator<derived_t, ref_type> >
class slink_t : public slink_base<derived_t, ref_type, link_iterator>
{
public:
	typedef slink_t<derived_t, ref_type, link_iterator>							this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t>	link_t;
	typedef ref_type<link_t>													ref_t;

	typedef default_slink_iterator<this_t, ref_type> default_link_iterator;

private:
	ref_t m_next;

	slink_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	slink_t()
	{ }

	slink_t(this_t&& t)
	{
		m_next = std::move(t.m_next);
	}

	this_t& operator=(this_t&& t)
	{
		m_next = std::move(t.m_next);
		return *this;
	}

	               ref_t& get_next_link()					{ return m_next; }
	const          ref_t& get_next_link() const				{ return m_next; }
	      volatile ref_t& get_next_link()       volatile	{ return m_next; }
	const volatile ref_t& get_next_link() const volatile	{ return m_next; }

	void set_next_link(const          ref_t& n)				{ m_next = n; }
	void set_next_link(const volatile ref_t& n)				{ m_next = n; }
	void set_next_link(const          ref_t& n) volatile	{ m_next = n; }
	void set_next_link(const volatile ref_t& n) volatile	{ m_next = n; }
};


template <template <typename> class ref_type>
class default_slink_iterator<void, ref_type> : public default_slink_iterator<slink_t<void, ref_type, default_slink_iterator<void, ref_type> > >
{
};


typedef slink_t<void> slink;


#pragma warning(pop)


}


#endif

