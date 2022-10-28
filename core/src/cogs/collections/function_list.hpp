//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_FUNCTION_LIST
#define COGS_HEADER_COLLECTION_FUNCTION_LIST


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

	function_list(const base_t&) = delete;
	function_list(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) = delete;
	this_t& operator=(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) volatile = delete;
	this_t& operator=(const volatile base_t&) volatile = delete;

public:
	//function_list() { }

	template <typename... args_t2>
	function_list(args_t2&&... args)
		: base_t(std::forward<args_t2>(args)...)
	{ }

	//function_list(this_t&& src)
	//	: base_t(std::move(src))
	//{ }

	//function_list(this_t& src)
	//	: base_t(src)
	//{ }

	//function_list(const this_t& src)
	//	: base_t(src)
	//{ }

	function_list(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	void operator=(this_t&&) volatile = delete;
	void operator=(const this_t&) volatile = delete;
	void operator=(const volatile this_t&) volatile = delete;

	return_t operator()(args_t... a)
	{
		return_t result{};
		for (auto& f : *this)
		{
			result = f(a...);
			if (!!result)
				break;
		}
		return result;
	}

	// Safe to call in parallel with move operations, but may continue to
	// iterate over nodes moved to destination container.
	return_t operator()(args_t... a) volatile
	{
		return_t result{};
		for (auto& f : *this)
		{
			result = f(a...);
			if (!!result)
				break;
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

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//this_t exchange(base_t&& src) volatile
	//{
	//	this_t tmp(std::move(src));
	//	swap(tmp);
	//	return tmp;
	//}

	void exchange(base_t&& src, base_t& rtn) { base_t::exchange(std::move(src), std::move(rtn)); }

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//void exchange(base_t&& src, base_t& rtn) volatile { base_t::exchange(std::move(src), rtn); }
};


template <typename... args_t>
class function_list<void(args_t...)> : public container_dlist<function<void(args_t...)> >
{
private:
	typedef function_list<void(args_t...)> this_t;
	typedef container_dlist<function<void(args_t...)> > base_t;

	function_list(const base_t&) = delete;
	function_list(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) = delete;
	this_t& operator=(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) volatile = delete;
	this_t& operator=(const volatile base_t&) volatile = delete;

public:
	//function_list() { }

	template <typename... args_t2>
	function_list(args_t2&&... args)
		: base_t(std::forward<args_t2>(args)...)
	{ }

	//function_list(this_t&& src)
	//	: base_t(std::move(src))
	//{ }

	//function_list(this_t& src)
	//	: base_t(src)
	//{ }

	//function_list(const this_t& src)
	//	: base_t(src)
	//{ }

	function_list(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	void operator=(this_t&&) volatile = delete;
	void operator=(const this_t&) volatile = delete;
	void operator=(const volatile this_t&) volatile = delete;

	void operator()(args_t... a)
	{
		for (auto& f : *this)
			f(a...);
	}

	// Safe to call in parallel with move operations, but may continue to
	// iterate over nodes moved to destination container.
	void operator()(args_t... a) volatile
	{
		for (auto& f : *this)
			f(a...);
	}

	// Redefined to provide redirect type for return type
	this_t exchange(base_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//this_t exchange(base_t&& src) volatile
	//{
	//	this_t tmp(std::move(src));
	//	swap(tmp);
	//	return tmp;
	//}

	void exchange(base_t&& src, base_t& rtn) { base_t::exchange(std::move(src), std::move(rtn)); }

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//void exchange(base_t&& src, base_t& rtn) volatile { base_t::exchange(std::move(src), rtn); }
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

	async_function_list(const base_t&) = delete;
	async_function_list(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) = delete;
	this_t& operator=(const volatile base_t&) = delete;

	this_t& operator=(const base_t&) volatile = delete;
	this_t& operator=(const volatile base_t&) volatile = delete;

public:
	//async_function_list() { }

	template <typename... args_t2>
	async_function_list(args_t2&&... args)
		: base_t(std::forward<args_t2>(args)...)
	{ }

	//async_function_list(this_t&& src)
	//	: base_t(std::move(src))
	//{ }

	//async_function_list(this_t& src)
	//	: base_t(src)
	//{ }

	//async_function_list(const this_t& src)
	//	: base_t(src)
	//{ }

	async_function_list(const volatile this_t&) = delete;

	this_t& operator=(const this_t& src)
	{
		base_t::operator=(src);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		base_t::operator=(std::move(src));
		return *this;
	}

	this_t& operator=(const volatile this_t&) = delete;
	void operator=(this_t&&) volatile = delete;
	void operator=(const this_t&) volatile = delete;
	void operator=(const volatile this_t&) volatile = delete;

	// It's caller error to allow the object to go out of scope while an invoke is still in progress.

	// Safe to call in parallel with move operations, but may continue to
	// iterate over nodes moved to destination container.
	rcref<task<return_t> > operator()(args_t... a) volatile
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

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//this_t exchange(base_t&& src) volatile
	//{
	//	this_t tmp(std::move(src));
	//	swap(tmp);
	//	return tmp;
	//}

	void exchange(base_t&& src, base_t& rtn) { base_t::exchange(std::move(src), std::move(rtn)); }

	//template <typename enable = std::enable_if_t<base_t::allocator_type::is_static> >
	//void exchange(base_t&& src, base_t& rtn) volatile { base_t::exchange(std::move(src), rtn); }
};


}


#endif
