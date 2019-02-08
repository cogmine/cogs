//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_VERSIONED_PTR
#define COGS_VERSIONED_PTR


#include "cogs/sync/versioned_ref.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment oeprators specified


/// @ingroup ReferenceContainerTypes
/// @ingroup Synchronization
/// @brief Provides an atomic ABA solution for pointer types.  Nullable.
/// @tparam T Type pointed to
template <typename T>
class versioned_ptr
{
public:
	typedef T type;

	typedef versioned_ptr<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef versioned_ptr<type> locked_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ptr with a different referenced type.
	/// @tparam type Data type referenced
	template <typename type2>
	class cast_type
	{
	public:
		/// @brief A versioned_ptr with a different referenced type.
		typedef versioned_ptr<type2> type;
	};

private:
	versioned_ref<type> m_ref;

public:	
	versioned_ptr()
	{ }
	
	template <typename type2>
	versioned_ptr(type2* t)
		: m_ref(t)
	{ }

	template <typename type2>
	versioned_ptr(const ptr<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const ref<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ptr<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ref<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	version_t release()				{ return m_ref.set(0); }
	void unversioned_release()		{ m_ref.unversioned_set(0); }
	version_t release() volatile	{ return m_ref.set(0); }

	version_t set(type* src)			{ return m_ref.set(src); }
	void unversioned_set(type* src)		{ m_ref.unversioned_set(src); }
	version_t set(type* src) volatile	{ return m_ref.set(src); }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(type2* p) { set(p); return *this; }
	template <typename type2> this_t& operator=(const versioned_ptr<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& p) { set(p.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ptr<type> operator=(type2* p) volatile { set(p); return p; }
	template <typename type2> ptr<type> operator=(const versioned_ptr<type2>& vp) volatile { set(vp.get_ptr()); return ptr<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const ptr<type2>& p) volatile { set(p.get_ptr()); return p; }
	template <typename type2> ptr<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { return m_ref.get_ptr(); } 
	type* get_ptr() const volatile { return m_ref.get_ptr(); }
	
	versioned_ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const versioned_ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	volatile versioned_ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const volatile versioned_ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }

	type* operator->() const
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return result;
	}

	type* operator->() const volatile
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return result;
	}

	type& operator*() const
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return *result;
	}

	type& operator*() const volatile
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return *result;
	}

	void get(type*& t, version_t& v) const				{ m_ref.get(t, v); }
	void get(type*& t, version_t& v) const volatile		{ m_ref.get(t, v); }

	void get(ptr<type>& t, version_t& v) const			{ m_ref.get(t, v); }
	void get(ptr<type>& t, version_t& v) const volatile	{ m_ref.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const				{ m_ref.get_version(v); }
	void get_version(version_t& v) const volatile		{ m_ref.get_version(v); }
	version_t get_version() const						{ return m_ref.get_version(); }
	version_t get_version() const volatile				{ return m_ref.get_version(); }

	void touch()			{ m_ref.touch(); }
	void touch() volatile	{ m_ref.touch(); }

	bool is_empty() const { return get_ptr() == 0; }
	bool is_empty() const volatile { return get_ptr() == 0; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	this_t&	operator++() { return *this += 1; }
	ptr<type> operator++() volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v;
		get(oldValue, v);
		do {
			newValue = oldValue + 1;
		} while (!versioned_exchange(newValue, v, oldValue));
		return newValue;
	}

	ptr<type> operator++(int i) { type* p = get_ptr(); set(p + 1); return p; }
	ptr<type> operator++(int i) volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v;
		get(oldValue, v);
		do {
			newValue = oldValue + 1;
		} while (!versioned_exchange(newValue, v, oldValue));
		return oldValue;
	}

	this_t&	operator--() { return *this -= 1; }
	ptr<type> operator--() volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v;
		get(oldValue, v);
		do {
			newValue = oldValue - 1;
		} while (!versioned_exchange(newValue, v, oldValue));
		return newValue;
	}

	ptr<type> operator--(int i) { type* p = get_ptr(); set(p - 1); return p; }
	ptr<type> operator--(int i) volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v = get(oldValue, v);
		do {
			newValue = oldValue - 1;
		} while (!versioned_exchange(newValue, v, oldValue));
		return oldValue;
	}

	this_t&	operator+=(size_t i) { set(get_ptr() + i); return *this; }
	ptr<type> operator+=(size_t i) volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v;
		get(oldValue, v);
		do {
			newValue = oldValue + i;
		} while (!versioned_exchange(newValue, v, oldValue));
		return newValue;
	}

	this_t&	operator-=(size_t i) { set(get_ptr() - i); return *this; }
	ptr<type> operator-=(size_t i) volatile
	{
		ptr<type> oldValue;
		ptr<type> newValue;
		version_t v;
		get(oldValue, v);
		do {
			newValue = oldValue + i;
		} while (!versioned_exchange(newValue, v, oldValue));
		return newValue;
	}

	ptr<type> operator+(ptrdiff_t n) { return get_ptr() + n; }
	ptr<type> operator+(ptrdiff_t n) volatile { return get_ptr() + n; }

	ptr<type> operator-(ptrdiff_t n) { return get_ptr() - n; }
	ptr<type> operator-(ptrdiff_t n) volatile { return get_ptr() - n; }

	ptrdiff_t operator-(type* src) const { return get_ptr() - src; }
	ptrdiff_t operator-(const ptr<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const ref<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const versioned_ptr<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const versioned_ref<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile ptr<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile ref<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile versioned_ptr<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile versioned_ref<type>& src) const { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(type* src) const volatile { return get_ptr() - src; }
	ptrdiff_t operator-(const ptr<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const ref<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const versioned_ptr<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const versioned_ref<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile ptr<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile ref<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile versioned_ptr<type>& src) const volatile { return get_ptr() - src.get_ptr(); }
	ptrdiff_t operator-(const volatile versioned_ref<type>& src) const volatile { return get_ptr() - src.get_ptr(); }

	template <typename type2>	bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2>	bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2>	bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2>	bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2>	bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2>	bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }




	template <typename type2>
	version_t exchange(type2*& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(ptr<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, ptr<type2>& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, type2*& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ptr<type>& src, version_t& version) { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ptr<type>& src, version_t& version) volatile { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) volatile { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }







	static size_t mark_bits() { return range_to_bits<0, std::alignment_of<type>::value - 1>::value; }
	static size_t mark_mask() { return (1 << mark_bits()) - 1; }

	size_t get_mark() const { return ((size_t)(get_ptr()) & mark_mask()); }
	size_t get_mark() const volatile { return ((size_t)(get_ptr()) & mark_mask()); }

	type* get_unmarked() const { return (type*)((size_t)get_ptr() & (size_t)~mark_mask()); }
	type* get_unmarked() const volatile { return (type*)((size_t)get_ptr() & (size_t)~mark_mask()); }

	type* get_marked(size_t mark) const { return (type*)((size_t)get_unmarked() | (mark & mark_mask())); }
	type* get_marked(size_t mark) const volatile { return (type*)((size_t)get_unmarked() | (mark & mark_mask())); }

	version_t clear_mark() { return set(get_unmarked()); }
	void unversioned_clear_mark() { unversioned_set(get_unmarked()); }

	version_t clear_mark() volatile
	{
		ptr<type> oldValue;
		version_t v = get(oldValue, v);
		while ((oldValue.get_mark() != 0) && (!versioned_exchange(oldValue.get_unmarked(), v, oldValue)))
		{
		}

		return v;
	}

	version_t set_mark(size_t mark) { return set((type*)((size_t)get_unmarked() | (mark & mark_mask()))); }

	version_t set_to_mark(size_t mark) { return set((type*)(mark & mark_mask())); }
	void unversioned_set_to_mark(size_t mark) { unversioned_set((type*)(mark & mark_mask())); }
	version_t set_to_mark(size_t mark) volatile { return set((type*)(mark & mark_mask())); }

	version_t set_marked(type* p, size_t mark) { return set((type*)((size_t)p | (mark & mark_mask()))); }
	void unversioned_set_marked(type* p, size_t mark) { unversioned_set((type*)((size_t)p | (mark & mark_mask()))); }
	version_t set_marked(type* p, size_t mark) volatile { return set((type*)((size_t)p | (mark & mark_mask()))); }


	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


template <>
class versioned_ptr<void>
{
public:
	typedef void type;

	typedef versioned_ptr<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef versioned_ptr<type> locked_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ptr with a different referenced type.
	/// @tparam type Data type referenced
	template <typename type2>
	class cast_type
	{
	public:
		/// @brief A versioned_ptr with a different referenced type.
		typedef versioned_ptr<type2> type;
	};

private:
	versioned_ref<type> m_ref;

public:
	versioned_ptr()
		: m_ref((type*)0)
	{ }

	template <typename type2>
	versioned_ptr(type2* t)
		: m_ref(t)
	{ }

	template <typename type2>
	versioned_ptr(const ptr<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const ref<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ptr<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ref<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	version_t release() { return m_ref.set(0); }
	void unversioned_release() { m_ref.unversioned_set(0); }
	version_t release() volatile { return m_ref.set(0); }

	version_t set(type* src) { return m_ref.set(src); }
	void unversioned_set(type* src) { m_ref.unversioned_set(src); }
	version_t set(type* src) volatile { return m_ref.set(src); }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(type2* p) { set(p); return *this; }
	template <typename type2> this_t& operator=(const versioned_ptr<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& p) { set(p.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ptr<type> operator=(type2* p) volatile { set(p); return p; }
	template <typename type2> ptr<type> operator=(const versioned_ptr<type2>& vp) volatile { set(vp.get_ptr()); return ptr<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const ptr<type2>& p) volatile { set(p.get_ptr()); return p; }
	template <typename type2> ptr<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	versioned_ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const versioned_ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	volatile versioned_ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const volatile versioned_ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }

	void get(type*& t, version_t& v) const { m_ref.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_ref.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_ref.get(t, v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_ref.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_ref.get_version(v); }
	void get_version(version_t& v) const volatile { m_ref.get_version(v); }
	version_t get_version() const { return m_ref.get_version(); }
	version_t get_version() const volatile { return m_ref.get_version(); }

	void touch() { m_ref.touch(); }
	void touch() volatile { m_ref.touch(); }

	bool is_empty() const { return get_ptr() == 0; }
	bool is_empty() const volatile { return get_ptr() == 0; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2>	bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2>	bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2>	bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2>	bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2>	bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2>	bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }



	template <typename type2>
	version_t exchange(type2*& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(ptr<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, ptr<type2>& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, type2*& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ptr<type>& src, version_t& version) { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ptr<type>& src, version_t& version) volatile { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) volatile { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }






	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


template <>
class versioned_ptr<const void>
{
public:
	typedef const void type;

	typedef versioned_ptr<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef versioned_ptr<type> locked_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ptr with a different referenced type.
	/// @tparam type Data type referenced
	template <typename type2>
	class cast_type
	{
	public:
		/// @brief A versioned_ptr with a different referenced type.
		typedef versioned_ptr<type2> type;
	};

private:
	versioned_ref<type> m_ref;

public:
	versioned_ptr()
		: m_ref((type*)0)
	{ }

	template <typename type2>
	versioned_ptr(type2* t)
		: m_ref(t)
	{ }

	template <typename type2>
	versioned_ptr(const ptr<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const ref<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ptr<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ref<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	version_t release() { return m_ref.set(0); }
	void unversioned_release() { m_ref.unversioned_set(0); }
	version_t release() volatile { return m_ref.set(0); }

	version_t set(type* src) { return m_ref.set(src); }
	void unversioned_set(type* src) { m_ref.unversioned_set(src); }
	version_t set(type* src) volatile { return m_ref.set(src); }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(type2* p) { set(p); return *this; }
	template <typename type2> this_t& operator=(const versioned_ptr<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& p) { set(p.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ptr<type> operator=(type2* p) volatile { set(p); return p; }
	template <typename type2> ptr<type> operator=(const versioned_ptr<type2>& vp) volatile { set(vp.get_ptr()); return ptr<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const ptr<type2>& p) volatile { set(p.get_ptr()); return p; }
	template <typename type2> ptr<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	versioned_ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const versioned_ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	volatile versioned_ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const volatile versioned_ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }

	void get(type*& t, version_t& v) const { m_ref.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_ref.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_ref.get(t, v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_ref.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_ref.get_version(v); }
	void get_version(version_t& v) const volatile { m_ref.get_version(v); }
	version_t get_version() const { return m_ref.get_version(); }
	version_t get_version() const volatile { return m_ref.get_version(); }

	void touch() { m_ref.touch(); }
	void touch() volatile { m_ref.touch(); }

	bool is_empty() const { return get_ptr() == 0; }
	bool is_empty() const volatile { return get_ptr() == 0; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2>	bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2>	bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2>	bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2>	bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2>	bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2>	bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }





	template <typename type2>
	version_t exchange(type2*& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(ptr<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, ptr<type2>& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, type2*& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ptr<type>& src, version_t& version) { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ptr<type>& src, version_t& version) volatile { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) volatile { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }







	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


template <>
class versioned_ptr<volatile void>
{
public:
	typedef volatile void type;

	typedef versioned_ptr<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef versioned_ptr<type> locked_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ptr with a different referenced type.
	/// @tparam type Data type referenced
	template <typename type2>
	class cast_type
	{
	public:
		/// @brief A versioned_ptr with a different referenced type.
		typedef versioned_ptr<type2> type;
	};

private:
	versioned_ref<type> m_ref;

public:
	versioned_ptr()
		: m_ref((type*)0)
	{ }

	template <typename type2>
	versioned_ptr(type2* t)
		: m_ref(t)
	{ }

	template <typename type2>
	versioned_ptr(const ptr<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const ref<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ptr<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ref<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	version_t release() { return m_ref.set(0); }
	void unversioned_release() { m_ref.unversioned_set(0); }
	version_t release() volatile { return m_ref.set(0); }

	version_t set(type* src) { return m_ref.set(src); }
	void unversioned_set(type* src) { m_ref.unversioned_set(src); }
	version_t set(type* src) volatile { return m_ref.set(src); }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(type2* p) { set(p); return *this; }
	template <typename type2> this_t& operator=(const versioned_ptr<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& p) { set(p.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ptr<type> operator=(type2* p) volatile { set(p); return p; }
	template <typename type2> ptr<type> operator=(const versioned_ptr<type2>& vp) volatile { set(vp.get_ptr()); return ptr<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const ptr<type2>& p) volatile { set(p.get_ptr()); return p; }
	template <typename type2> ptr<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	versioned_ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const versioned_ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	volatile versioned_ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const volatile versioned_ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }

	void get(type*& t, version_t& v) const { m_ref.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_ref.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_ref.get(t, v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_ref.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_ref.get_version(v); }
	void get_version(version_t& v) const volatile { m_ref.get_version(v); }
	version_t get_version() const { return m_ref.get_version(); }
	version_t get_version() const volatile { return m_ref.get_version(); }

	void touch() { m_ref.touch(); }
	void touch() volatile { m_ref.touch(); }

	bool is_empty() const { return get_ptr() == 0; }
	bool is_empty() const volatile { return get_ptr() == 0; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2>	bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2>	bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2>	bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2>	bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2>	bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2>	bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }





	template <typename type2>
	version_t exchange(type2*& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(ptr<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, ptr<type2>& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, type2*& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ptr<type>& src, version_t& version) { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ptr<type>& src, version_t& version) volatile { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) volatile { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }







	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


template <>
class versioned_ptr<const volatile void>
{
public:
	typedef const volatile void type;

	typedef versioned_ptr<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef versioned_ptr<type> locked_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ptr with a different referenced type.
	/// @tparam type Data type referenced
	template <typename type2>
	class cast_type
	{
	public:
		/// @brief A versioned_ptr with a different referenced type.
		typedef versioned_ptr<type2> type;
	};

private:
	versioned_ref<type> m_ref;

public:
	versioned_ptr()
		: m_ref((type*)0)
	{ }

	template <typename type2>
	versioned_ptr(type2* t)
		: m_ref(t)
	{ }

	template <typename type2>
	versioned_ptr(const ptr<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const ref<type2>& t)
		: m_ref(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ptr<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	template <typename type2>
	versioned_ptr(const versioned_ref<type2>& vp)
		: m_ref(vp.get_ptr())
	{ }

	version_t release() { return m_ref.set(0); }
	void unversioned_release() { m_ref.unversioned_set(0); }
	version_t release() volatile { return m_ref.set(0); }

	version_t set(type* src) { return m_ref.set(src); }
	void unversioned_set(type* src) { m_ref.unversioned_set(src); }
	version_t set(type* src) volatile { return m_ref.set(src); }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(type2* p) { set(p); return *this; }
	template <typename type2> this_t& operator=(const versioned_ptr<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& p) { set(p.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ptr<type> operator=(type2* p) volatile { set(p); return p; }
	template <typename type2> ptr<type> operator=(const versioned_ptr<type2>& vp) volatile { set(vp.get_ptr()); return ptr<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ptr<type> operator=(const ptr<type2>& p) volatile { set(p.get_ptr()); return p; }
	template <typename type2> ptr<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	versioned_ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const versioned_ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	volatile versioned_ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	const volatile versioned_ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }

	void get(type*& t, version_t& v) const { m_ref.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_ref.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_ref.get(t, v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_ref.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_ref.get_version(v); }
	void get_version(version_t& v) const volatile { m_ref.get_version(v); }
	version_t get_version() const { return m_ref.get_version(); }
	version_t get_version() const volatile { return m_ref.get_version(); }

	void touch() { m_ref.touch(); }
	void touch() volatile { m_ref.touch(); }

	bool is_empty() const { return get_ptr() == 0; }
	bool is_empty() const volatile { return get_ptr() == 0; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2>	bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2>	bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2>	bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2>	bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2>	bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2>	bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2>	bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2>	bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2>	bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2>	bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2>	bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2>	bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2>	bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2>	bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2>	bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2>	bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2>	bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2>	bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }





	template <typename type2>
	version_t exchange(type2*& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(ptr<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ptr<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_ref.get_versioned().exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, ptr<type2>& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ptr<type>& src, type2*& rtn) { m_ref.get_versioned().unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ptr<type>& src, version_t& version) { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ptr<type>& src, version_t& version) volatile { return m_ref.get_versioned().versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ptr<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp) volatile { return m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_ref.get_versioned().compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp) { return m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ptr<type>& src, const ptr<type>& cmp, type2*& rtn) { type* tmp; bool result = m_ref.get_versioned().unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }







	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


#pragma warning(pop)


}


// placement operator new/delete for versioned_ptr<>
template <typename type>
inline void* operator new(size_t sz, const cogs::versioned_ptr<type>& p)	{ return ::operator new(sz, p.get_ptr()); }

template <typename type>
inline void* operator new(size_t sz, const cogs::versioned_ptr<type>& p, const std::nothrow_t& nt) throw ()	{ return ::operator new(sz, p.get_ptr(), nt); }



template <typename type>
inline void operator delete(void*, const cogs::versioned_ptr<type>& p) throw ()	{ }

template <typename type>
inline void operator delete(void*, const cogs::versioned_ptr<type>& p, const std::nothrow_t&) throw ()	{ }


#endif
