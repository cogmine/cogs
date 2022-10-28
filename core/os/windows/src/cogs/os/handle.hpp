//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_HANDLE
#define COGS_HEADER_OS_HANDLE


#include "cogs/env/mem/alignment.hpp"
#include "cogs/os.hpp"
#include "cogs/operators.hpp"
#include "cogs/mem/auto_handle.hpp"


namespace cogs {
namespace os {


// Not using auto_handle, due to clang complaining about use of void* (a HANDLE) as a compile time constant

//inline void auto_handle_impl_CloseHandle(HANDLE h) { CloseHandle(h); }
//typedef auto_handle<HANDLE, INVALID_HANDLE_VALUE, auto_handle_impl_CloseHandle> auto_HANDLE;


class auto_HANDLE
{
private:
	HANDLE m_value alignas(cogs::atomic::get_alignment_v<HANDLE>);

public:
	auto_HANDLE()
		: m_value(INVALID_HANDLE_VALUE)
	{ }

	auto_HANDLE(const HANDLE& value)
		: m_value(value)
	{ }

	auto_HANDLE(HANDLE&& value)
		: m_value(std::move(value))
	{ }

	auto_HANDLE(auto_HANDLE&& src)
		: m_value(std::move(src.m_value))
	{ }

	~auto_HANDLE()
	{
		if (m_value != INVALID_HANDLE_VALUE)
			CloseHandle(m_value);
	}

	auto_HANDLE& operator=(const HANDLE& value)
	{
		set(value);
		return *this;
	}

	void operator=(const HANDLE& value) volatile
	{
		set(value);
	}

	auto_HANDLE& operator=(auto_HANDLE&& src)
	{
		set(src.m_value);
		return *this;
	}

	void operator=(auto_HANDLE&& src) volatile
	{
		set(src.m_value);
	}

	void set(const HANDLE& value)
	{
		if (m_value != INVALID_HANDLE_VALUE)
			CloseHandle(m_value);
		m_value = value;
	}


	void set(const HANDLE& value) volatile
	{
		HANDLE tmp = cogs::exchange(m_value, value);
		if (tmp != INVALID_HANDLE_VALUE)
			CloseHandle(tmp);
	}

	HANDLE& get() { return m_value; }
	const HANDLE& get() const { return m_value; }

	HANDLE get() const volatile { return cogs::atomic::load(m_value); }

	HANDLE& operator*() { return m_value; }
	const HANDLE& operator*() const { return m_value; }

	HANDLE operator*() const volatile { return cogs::atomic::load(m_value); }

	HANDLE* operator->() { return &m_value; }
	const HANDLE* operator->() const { return &m_value; }

	void release()
	{
		if (m_value != INVALID_HANDLE_VALUE)
			CloseHandle(m_value);
	}

	void release() volatile
	{
		HANDLE tmp = cogs::exchange(m_value, INVALID_HANDLE_VALUE);
		if (tmp != INVALID_HANDLE_VALUE)
			CloseHandle(tmp);
	}

	bool operator!() const { return m_value == INVALID_HANDLE_VALUE; }
	bool operator!() const volatile { return get() == INVALID_HANDLE_VALUE; }

	void swap(auto_HANDLE& wth) { cogs::swap(m_value, wth.m_value); }
	void swap(volatile auto_HANDLE& wth) { cogs::swap(m_value, wth.m_value); }
	void swap(auto_HANDLE& wth) volatile { cogs::swap(m_value, wth.m_value); }

	auto_HANDLE exchange(const auto_HANDLE& src) { return cogs::exchange(m_value, src.m_value); }
	auto_HANDLE exchange(const volatile auto_HANDLE& src) { return cogs::exchange(m_value, src.m_value); }
	auto_HANDLE exchange(const auto_HANDLE& src) volatile { return cogs::exchange(m_value, src.m_value); }
	auto_HANDLE exchange(const volatile auto_HANDLE& src) volatile { return cogs::exchange(m_value, src.m_value); }

	void exchange(const auto_HANDLE& src, auto_HANDLE& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const auto_HANDLE& src, auto_HANDLE& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile auto_HANDLE& src, auto_HANDLE& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile auto_HANDLE& src, auto_HANDLE& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const auto_HANDLE& src, auto_HANDLE volatile& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const auto_HANDLE& src, auto_HANDLE volatile& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile auto_HANDLE& src, auto_HANDLE volatile& rtn) { cogs::exchange(m_value, src.m_value, rtn.m_value); }
	void exchange(const volatile auto_HANDLE& src, auto_HANDLE volatile& rtn) volatile { cogs::exchange(m_value, src.m_value, rtn.m_value); }


	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }

	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value); }

	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }

	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const auto_HANDLE& cmp, auto_HANDLE volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE volatile& rtn) { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
	bool compare_exchange(const volatile auto_HANDLE& src, const volatile auto_HANDLE& cmp, auto_HANDLE volatile& rtn) volatile { return cogs::compare_exchange(m_value, src.m_value, cmp.m_value, rtn.m_value); }
};


}
}


#endif
