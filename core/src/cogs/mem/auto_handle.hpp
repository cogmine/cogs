//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_AUTO_HANDLE
#define COGS_HEADER_MEM_AUTO_HANDLE


#include <utility>
#include "cogs/operators.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/sync/atomic_load.hpp"


namespace cogs {


template <typename value_t, value_t invalidValue, void(*dispose_func)(value_t), typename enable = void>
class auto_handle;

template <typename value_t, value_t invalidValue, void(*dispose_func)(value_t)>
class auto_handle<value_t, invalidValue, dispose_func, std::enable_if_t<!can_atomic_v<value_t> > >
{
private:
	typedef auto_handle<value_t, invalidValue, dispose_func> this_t;

	value_t m_value;

public:
	auto_handle()
		: m_value(invalidValue)
	{ }

	auto_handle(const value_t& value)
		: m_value(value)
	{ }

	auto_handle(value_t&& value)
		: m_value(std::move(value))
	{ }

	auto_handle(this_t&& src)
		: m_value(std::move(src.m_value))
	{ }

	~auto_handle()
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
	}

	this_t& operator=(const value_t& value)
	{
		set(value);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
		m_value = std::move(src.m_value);
		return *this;
	}

	void set(const value_t& value)
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
		m_value = value;
	}

	value_t& get() { return m_value; }
	const value_t& get() const { return m_value; }

	value_t& operator*() { return m_value; }
	const value_t& operator*() const { return m_value; }

	value_t* operator->() { return &m_value; }
	const value_t* operator->() const { return &m_value; }

	void release()
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
	}

	bool operator!() const { return m_value == invalidValue; }

	void swap(this_t& wth) { cogs::swap(m_value, wth.m_value); }

	this_t exchange(const this_t& src) { return cogs::exchange(m_value, src.m_value); }

	void exchange(const this_t& src, this_t& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }

	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }

	bool compare_exchange(const this_t& src, const this_t& cmp, this_t& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
};


template <typename value_t, value_t invalidValue, void(*dispose_func)(value_t)>
class auto_handle<value_t, invalidValue, dispose_func, std::enable_if_t<can_atomic_v<value_t> > >
{
private:
	typedef auto_handle<value_t, invalidValue, dispose_func> this_t;

	alignas (cogs::atomic::get_alignment_v<value_t>) value_t m_value;

public:
	auto_handle()
		: m_value(invalidValue)
	{ }

	auto_handle(const value_t& value)
		: m_value(value)
	{ }

	auto_handle(value_t&& value)
		: m_value(std::move(value))
	{ }

	auto_handle(this_t&& src)
		: m_value(std::move(src.m_value))
	{ }

	~auto_handle()
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
	}

	this_t& operator=(const value_t& value)
	{
		set(value);
		return *this;
	}

	volatile this_t& operator=(const value_t& value) volatile
	{
		set(value);
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		set(src.m_value);
		return *this;
	}

	volatile this_t& operator=(this_t&& src) volatile
	{
		set(src.m_value);
		return *this;
	}

	void set(const value_t& value)
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
		m_value = value;
	}


	void set(const value_t& value) volatile
	{
		value_t tmp = cogs::exchange(m_value, value);
		if (tmp != invalidValue)
			dispose_func(tmp);
	}

	value_t& get() { return m_value; }
	const value_t& get() const { return m_value; }

	value_t get() const volatile { return atomic::load(m_value); }

	value_t& operator*() { return m_value; }
	const value_t& operator*() const { return m_value; }

	value_t operator*() const volatile { return atomic::load(m_value); }

	value_t* operator->() { return &m_value; }
	const value_t* operator->() const { return &m_value; }

	void release()
	{
		if (m_value != invalidValue)
			dispose_func(m_value);
	}

	void release() volatile
	{
		value_t tmp = cogs::exchange(m_value, invalidValue);
		if (tmp != invalidValue)
			dispose_func(tmp);
	}

	bool operator!() const { return m_value == invalidValue; }
	bool operator!() const volatile { return get() == invalidValue; }

	void swap(this_t& wth) { cogs::swap(m_value, wth.m_value); }
	void swap(volatile this_t& wth) { cogs::swap(m_value, wth.m_value); }
	void swap(this_t& wth) volatile { cogs::swap(m_value, wth.m_value); }

	this_t exchange(const this_t& src) { return cogs::exchange(m_value, src.m_value); }
	this_t exchange(const volatile this_t& src) { return cogs::exchange(m_value, src.m_value); }
	this_t exchange(const this_t& src) volatile { return cogs::exchange(m_value, src.m_value); }
	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(m_value, src.m_value); }

	void exchange(const this_t& src, this_t& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const this_t& src, this_t& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile this_t& src, this_t& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile this_t& src, this_t& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const this_t& src, this_t volatile& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const this_t& src, this_t volatile& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile this_t& src, this_t volatile& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile this_t& src, this_t volatile& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }

	bool compare_exchange(const this_t& src, const this_t& cmp, this_t& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const this_t& cmp, this_t& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, this_t& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, this_t& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, this_t& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, this_t& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, this_t& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, this_t& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }

	bool compare_exchange(const this_t& src, const this_t& cmp, this_t volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const this_t& cmp, this_t volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, this_t volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, this_t volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, this_t volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, this_t volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, this_t volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, this_t volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
};


}


#endif
