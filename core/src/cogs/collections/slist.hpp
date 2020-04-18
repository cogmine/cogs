//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_SLIST
#define COGS_HEADER_COLLECTION_SLIST


#include "cogs/collections/slink.hpp"


namespace cogs {


/// @ingroup Collections
/// @brief An intrusive single-link list.
/// @tparam link_t A single-link list element type
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_slink_accessor<link_t, ref_type>
template <class link_t = slink, template <typename> class ref_type = ptr, class link_accessor = default_slink_accessor<link_t, ref_type> >
class slist_t
{
public:
	typedef slist_t<link_t, ref_type, link_accessor> this_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_first;
	ref_t m_last;

	static const ref_t& get_next(const link_t& l) { return link_accessor::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l) { return link_accessor::get_next(l); }

	static void set_next(link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }

	slist_t(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;

public:
	slist_t()
	{ }

	slist_t(this_t&& src)
		: m_first(std::move(src.m_first)),
		m_last(std::move(src.m_last))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_first = std::move(src.m_first);
		m_last = std::move(src.m_last);
		return *this;
	}

	bool operator!() const { return !m_first; }
	bool operator!() const volatile { return !m_first; }
	bool is_empty() const { return !m_first; }
	bool is_empty() const volatile { return !m_first; }

	void append(const ref_t& l)
	{
		if (!m_last)
			m_first = l;
		else
			set_next(*m_last, l);
		m_last = l;
	}

	void prepend(const ref_t& l)
	{
		set_next(*l, m_first);
		if (!m_first)
			m_last = l;
		m_first = l;
	}

	ref_t pop_first()
	{
		ref_t result = m_first;
		if (!!result)
		{
			m_first = get_next(*result);
			if (!m_first)
				m_last.clear();
		}

		return result;
	}
};

/// @ingroup Collections
/// @brief An intrusive single-link circular list.
/// @tparam link_t A single-link list element type
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_slink_accessor<link_t, ref_type>
template <class link_t = slink, template <typename> class ref_type = ptr, class link_accessor = default_slink_accessor<link_t, ref_type> >
class circular_slist_t
{
public:
	typedef circular_slist_t<link_t, ref_type, link_accessor> this_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_last;

	static const ref_t& get_next(const link_t& l) { return link_accessor::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l) { return link_accessor::get_next(l); }

	static void set_next(link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }

	circular_slist_t(const ref_t& setTo)
		: m_last(setTo)
	{ }

	circular_slist_t(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;

public:
	circular_slist_t()
	{ }

	circular_slist_t(this_t&& src)
		: m_last(std::move(src.m_last))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_last = std::move(src.m_last);
		return *this;
	}

	ref_t peek_last() const { return m_last; }
	ref_t peek_last() const volatile { return m_last; }

	ref_t peek() const
	{
		ref_t result;
		if (!!m_last)
			result = get_next(*m_last);
		return result;
	}

	bool operator!() const { return !m_last; }
	bool operator!() const volatile { return !m_last; }
	bool is_empty() const { return !m_last; }
	bool is_empty() const volatile { return !m_last; }

	void advance()
	{
		if (!!m_last)
			m_last = get_next(*m_last);
	}

	void append(const ref_t& e)
	{
		set_next(*e, !m_last ? e : get_next(*m_last));
		m_last = e;
	}

	void prepend(const ref_t& e)
	{
		set_next(*e, !m_last ? (m_last = e) : get_next(*m_last));
	}

	ref_t remove() // remove's first
	{
		if (!m_last)
		{
			ref_t emptyRef;
			return emptyRef;
		}

		ref_t result = get_next(*m_last);
		if (result == m_last)
			m_last.clear();
		else
			set_next(*m_last, get_next(*result));
		return result;
	}

	ref_t remove_if_equals(const ref_t& e, ref_t& oldHead)
	{
		if (!m_last)
			oldHead.clear();
		else
		{
			oldHead = get_next(*m_last);
			if (oldHead == e)
			{
				if (e == m_last)
					m_last.clear();
				else
					set_next(*m_last, get_next(*e));
				return e;
			}
		}
		return ref_t();
	}

	ref_t remove_if_equals(const ref_t& e)
	{
		if (!!m_last)
		{
			if (get_next(*m_last) == e)
			{
				if (e == m_last)
					m_last.clear();
				else
					set_next(*m_last, get_next(*e));
				return e;
			}
		}
		return ref_t();
	}

	this_t clear()
	{
		ref_t oldLast = m_last;
		m_last.clear();
		return this_t(oldLast);
	}

	this_t clear() volatile
	{
		ref_t oldLast;
		m_last.exchange(oldLast, oldLast);
		return this_t(oldLast);
	}
};


typedef slist_t<slink, ptr, default_slink_accessor<slink> > slist;
typedef circular_slist_t<slink, ptr, default_slink_accessor<slink> > circular_slist;


};


#endif
