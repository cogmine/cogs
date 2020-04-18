//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_FUNCTION_LIST
#define COGS_HEADER_FUNCTION_LIST


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/sync/dispatcher.hpp"


namespace cogs {


template <typename signature>
class function_list;

template <typename signature>
class async_function_list;


template <typename return_t, typename... args_t>
class function_list<return_t(args_t...)> : public container_dlist<function<return_t(args_t...)> >
{
private:
	typedef function_list<return_t(args_t...)> this_t;
	typedef container_dlist<function<return_t(args_t...)> > base_t;

public:
	using base_t::base_t;
	using base_t::operator=;

	return_t invoke(args_t... a)
	{
		return_t result;
		typename base_t::iterator itor = base_t::get_first();
		while (!!itor)
		{
			result = (*itor)(std::forward<args_t>(a)...);
			if (!!result)
				break;
			++itor;
		}
		return result;
	}

	// Safe to call in parallel with move operations, but may continue to
	// iterate over nodes moved to destination container.
	return_t invoke(args_t... a) volatile
	{
		return_t result;
		typename base_t::volatile_iterator itor = base_t::get_first();
		while (!!itor)
		{
			result = (*itor)(std::forward<args_t>(a)...);
			if (!!result)
				break;
			++itor;
		}
		return result;
	}

	// Redefined to provide redirect type for return type
	this_t exchange(base_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	this_t exchange(base_t&& src) volatile
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	void exchange(base_t&& src, base_t& rtn) { base_t::exchange(std::move(src), std::move(rtn)); }

	template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	void exchange(base_t&& src, base_t& rtn) volatile { base_t::exchange(std::move(src), rtn); }
};


template <typename return_t, typename... args_t>
class async_function_list<rcref<task<return_t> >(args_t...)> : public container_dlist<function<rcref<task<return_t> >(args_t...)> >
{
private:
	typedef async_function_list<rcref<task<return_t> >(args_t...)> this_t;
	typedef container_dlist<function<rcref<task<return_t> >(args_t...)> > base_t;

	static rcref<task<return_t> > continue_invoke(const typename base_t::volatile_iterator& itor, args_t... a)
	{
		return (*itor)(a...)->dispatch([itor, a...](const rcref<task<return_t> >& t)
		{
			if (!t->get())
			{
				typename base_t::volatile_iterator nextItor = itor;
				if (!!++nextItor)
					return continue_invoke(nextItor, a...);
			}
			return t;
		});
	}

public:
	using base_t::base_t;

	// It's caller error to allow the object to go out of scope while an invoke is still in progress.

	// Safe to call in parallel with move operations, but may continue to
	// iterate over nodes moved to destination container.
	rcref<task<return_t> > invoke(args_t... a) volatile
	{
		typename base_t::volatile_iterator itor = base_t::get_first();
		if (!itor)
		{
			return_t result{};
			return signaled(std::move(result));
		}

		return continue_invoke(itor, a...);
	}

	// Redefined to provide redirect type for return type
	this_t exchange(base_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	this_t exchange(base_t&& src) volatile
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	void exchange(base_t&& src, base_t& rtn) { base_t::exchange(std::move(src), std::move(rtn)); }

	template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	void exchange(base_t&& src, base_t& rtn) volatile { base_t::exchange(std::move(src), rtn); }
};


}


#endif
