//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_DLIST
#define COGS_HEADER_COLLECTION_DLIST


#include "cogs/collections/dlink.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @ingroup Collections
/// @brief An intrusive double-link list.
/// @tparam link_t A double-link list element type
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_dlink_accessor
template <class link_t = dlink, template <typename> class ref_type = ptr, class link_accessor = default_dlink_accessor<link_t, ref_type> >
class dlist_t
{
public:
	typedef dlist_t<link_t, ref_type, link_accessor> this_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_first;
	ref_t m_last;

	static const ref_t& get_next(const link_t& l) { return link_accessor::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l) { return link_accessor::get_next(l); }

	static const ref_t& get_prev(const link_t& l) { return link_accessor::get_prev(l); }
	static const volatile ref_t& get_prev(const volatile link_t& l) { return link_accessor::get_prev(l); }

	static void set_next(link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }

	static void set_prev(link_t& l, const ref_t& src) { return link_accessor::set_prev(l, src); }
	static void set_prev(volatile link_t& l, const ref_t& src) { return link_accessor::set_prev(l, src); }

	dlist_t(const ref_t& first, const ref_t& last)
		: m_first(first),
		m_last(last)
	{ }

	dlist_t(ref_t&& first, const ref_t& last)
		: m_first(std::move(first)),
		m_last(last)
	{ }

	dlist_t(const ref_t& first, ref_t&& last)
		: m_first(first),
		m_last(std::move(last))
	{ }

	dlist_t(ref_t&& first, ref_t&& last)
		: m_first(std::move(first)),
		m_last(std::move(last))
	{ }

	dlist_t(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;

public:
	dlist_t()
	{ }

	dlist_t(this_t&& src)
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

	const ref_t& get_first() const { return m_first; }
	ref_t get_first() const volatile { return m_first; }

	const ref_t& get_last() const { return m_last; }
	ref_t get_last() const volatile { return m_last; }

	void append(const ref_t& l)
	{
		set_prev(*l, m_last);
		if (!m_first)
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
		else
			set_prev(*m_first, l);
		m_first = l;
	}

	void remove(const ref_t& l)
	{
		if (m_first == l)
		{
			if (m_last == l)
			{
				m_first.release();
				m_last.release();
				return;
			}
			m_first = get_next(*m_first);
		}
		else if (m_last == l)
			m_last = get_prev(*m_last);
		else
		{
			ref_t next = get_next(*l);
			ref_t prev = get_prev(*l);
			set_prev(*next, prev);
			set_next(*prev, next);
		}
	}

	ref_t pop_first()
	{
		ref_t result = m_first;
		if (!!result)
		{
			if (result == m_last)
			{
				m_first.release();
				m_last.release();
				return;
			}
			m_first = get_next(*result);
		}
		return result;
	}

	ref_t pop_last()
	{
		ref_t result = m_last;
		if (!!result)
		{
			if (result == m_first)
			{
				m_first.release();
				m_last.release();
				return;
			}
			m_last = get_prev(*result);
		}
		return result;
	}

	this_t clear()
	{
		this_t result(std::move(*this));
		m_first.release();
		m_last.release();
		return result;
	}
};

/// @ingroup Collections
/// @brief An intrusive double-link circular list.
/// @tparam link_t A double-link list element type
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_dlink_accessor
template <class link_t = dlink, template <typename> class ref_type = ptr, class link_accessor = default_dlink_accessor<link_t, ref_type> >
class circular_dlist_t
{
public:
	typedef circular_dlist_t<link_t, ref_type, link_accessor> this_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_first;

	static const ref_t& get_next(const link_t& l) { return link_accessor::get_next(l); }
	static const volatile ref_t& get_next(const volatile link_t& l) { return link_accessor::get_next(l); }

	static const ref_t& get_prev(const link_t& l) { return link_accessor::get_prev(l); }
	static const volatile ref_t& get_prev(const volatile link_t& l) { return link_accessor::get_prev(l); }

	static void set_next(link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }
	static void set_next(volatile link_t& l, const ref_t& src) { return link_accessor::set_next(l, src); }

	static void set_prev(link_t& l, const ref_t& src) { return link_accessor::set_prev(l, src); }
	static void set_prev(volatile link_t& l, const ref_t& src) { return link_accessor::set_prev(l, src); }

	circular_dlist_t(const ref_t& setTo)
		: m_first(setTo)
	{ }

	circular_dlist_t(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;

public:
	circular_dlist_t()
	{ }

	circular_dlist_t(this_t&& src)
		: m_first(std::move(src.m_first))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_first = std::move(src.m_first);
		return *this;
	}

	bool operator!() const { return !m_first; }
	bool operator!() const volatile { return !m_first; }
	bool is_empty() const { return !m_first; }
	bool is_empty() const volatile { return !m_first; }

	const ref_t& get_first() const { return m_first; }
	ref_t get_first() const volatile { return m_first; }

	const ref_t& get_last() const
	{
		if (!!m_first)
			return get_prev(*m_first);
		return m_first;
	}

	void advance()
	{
		if (!!m_first)
			m_first = get_next(*m_first);
	}

	void append(const ref_t& l) // put before first
	{
		if (!m_first)
		{
			set_prev(*l, l);
			set_next(*l, l);
			m_first = l;
		}
		else
		{
			ref_t last = get_prev(*m_first);
			set_next(*l, m_first);
			set_prev(*l, last);
			set_next(*last, l);
			set_prev(*m_first, l);
		}
	}

	void prepend(const ref_t& l) // put before first, update first.
	{
		if (!m_first)
		{
			set_prev(*l, l);
			set_next(*l, l);
		}
		else
		{
			ref_t last = get_prev(*m_first);
			set_next(*l, m_first);
			set_prev(*l, last);
			set_next(*last, l);
			set_prev(*m_first, l);
		}

		m_first = l;
	}

	void remove(const ref_t& l)
	{
		if (m_first == l)
		{
			m_first = get_next(*m_first);
			if (m_first == l)
			{
				m_first.release();
				return;
			}
		}

		ref_t next = get_next(*l);
		ref_t prev = get_prev(*l);
		set_prev(*next, prev);
		set_next(*prev, next);
	}

	ref_t pop_first()
	{
		ref_t result = m_first;
		if (!!result)
		{
			m_first = get_next(*m_first);
			if (m_first == result)
				m_first.release();
			else
			{
				ref_t next = get_next(*result);
				ref_t prev = get_prev(*result);
				set_prev(*next, prev);
				set_next(*prev, next);
			}
		}

		return result;
	}

	ref_t pop_last()
	{
		ref_t result;
		if (!!m_first)
		{
			result = get_prev(*m_first);
			if (m_first == result)
				m_first.release();
			else
			{
				ref_t next = get_next(*result);
				ref_t prev = get_prev(*result);
				set_prev(*next, prev);
				set_next(*prev, next);
			}
		}

		return result;
	}

	this_t clear()
	{
		this_t result(std::move(*this));
		m_first.release();
		return result;
	}

	this_t clear() volatile
	{
		ref_t oldFirst;
		m_first.exchange(oldFirst, oldFirst);
		return this_t(std::move(oldFirst));
	}
};


typedef dlist_t<dlink, ptr, default_dlink_accessor<dlink> > dlist;
typedef circular_dlist_t<dlink, ptr, default_dlink_accessor<slink> > circular_dlist;


}


#endif
