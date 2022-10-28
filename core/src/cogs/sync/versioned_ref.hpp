//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_VERSIONED_REF
#define COGS_HEADER_SYNC_VERSIONED_REF


#include "cogs/env.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/versioned.hpp"


namespace cogs {


template <typename T>
class versioned_ptr;


/// @ingroup ReferenceContainerTypes
/// @ingroup Synchronization
/// @brief Provides an atomic ABA solution for pointer types.  Non-nullable.
/// @tparam T Type pointed to
template <typename T>
class versioned_ref
{
public:
	typedef T type;

	typedef versioned_ref<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef ref<type> lock_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A versioned_ref with a different referenced type.
		typedef versioned_ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	versioned_t<type*> m_versioned;

	template <typename>
	friend class versioned_ptr;

	versioned_t<type*>& get_versioned() { return m_versioned; }
	const versioned_t<type*>& get_versioned() const { return m_versioned; }
	volatile versioned_t<type*>& get_versioned() volatile { return m_versioned; }
	const volatile versioned_t<type*>& get_versioned() const volatile { return m_versioned; }

	version_t set(type* src) { return m_versioned.set(src); }
	void unversioned_set(type* src) { m_versioned.unversioned_set(src); }
	version_t set(type* src) volatile { return m_versioned.set(src); }

	versioned_ref() : m_versioned(0) { }

	template <typename type2>
	versioned_ref(type2* r) : m_versioned(r) { }

	template <typename type2>
	versioned_ref(const ptr<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ptr<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }

public:
	versioned_ref(const ref<type>& t)
		: m_versioned(t.get_ptr())
	{ }

	versioned_ref(const versioned_ref<type>& vp)
		: m_versioned(vp.get_ptr())
	{ }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ref<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ref<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

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

	type* get_ptr() const { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr() const volatile { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr(version_t& v) const { type* t; m_versioned.get(t, v); return t; }
	type* get_ptr(version_t& v) const volatile { type* t; m_versioned.get(t, v); return t; }

	void get(type*& t, version_t& v) const { m_versioned.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_versioned.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_versioned.get(t.get_ptr_ref(), v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_versioned.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_versioned.get_version(v); }
	void get_version(version_t& v) const volatile { m_versioned.get_version(v); }
	version_t get_version() const { return m_versioned.get_version(); }
	version_t get_version() const volatile { return m_versioned.get_version(); }

	void touch() { m_versioned.touch(); }
	void touch() volatile { m_versioned.touch(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


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


	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	version_t exchange(ref<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ref<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ptr<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, type2*& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ref<type>& src, version_t& version) { return m_versioned.versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ref<type>& src, version_t& version) volatile { return m_versioned.versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) volatile { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	static constexpr size_t mark_bits() { return range_to_bits_v<0, alignof(type) - 1>; }
	static constexpr size_t mark_mask() { return (1 << mark_bits()) - 1; }

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

	version_t clear_to_mark() { return set((type*)(get_mark())); }
	void unversioned_clear_to_mark() { unversioned_set((type*)(get_mark())); }

	version_t clear_to_mark() volatile
	{
		ptr<type> oldValue;
		version_t v = get(oldValue, v);
		while ((oldValue.get_unmarked() != 0) && (!versioned_exchange((type*)oldValue.get_mark(), v, oldValue)))
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
class versioned_ref<void>
{
public:
	typedef void type;

	typedef versioned_ref<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef ref<type> lock_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A versioned_ref with a different referenced type.
		typedef versioned_ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	versioned_t<type*> m_versioned;

	template <typename>
	friend class versioned_ptr;

	versioned_t<type*>& get_versioned() { return m_versioned; }
	const versioned_t<type*>& get_versioned() const { return m_versioned; }
	volatile versioned_t<type*>& get_versioned() volatile { return m_versioned; }
	const volatile versioned_t<type*>& get_versioned() const volatile { return m_versioned; }

	version_t set(type* src) { return m_versioned.set(src); }
	void unversioned_set(type* src) { m_versioned.unversioned_set(src); }
	version_t set(type* src) volatile { return m_versioned.set(src); }

	versioned_ref() : m_versioned(0) { }

	template <typename type2>
	versioned_ref(type2* r) : m_versioned(r) { }

	template <typename type2>
	versioned_ref(const ptr<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ptr<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }


public:
	template <typename type2>
	versioned_ref(const ref<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ref<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ref<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ref<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr() const volatile { type* tmp; m_versioned.get(tmp); return tmp; }

	void get(type*& t, version_t& v) const { m_versioned.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_versioned.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_versioned.get(t.get_ptr_ref(), v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_versioned.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_versioned.get_version(v); }
	void get_version(version_t& v) const volatile { m_versioned.get_version(v); }
	version_t get_version() const { return m_versioned.get_version(); }
	version_t get_version() const volatile { return m_versioned.get_version(); }

	void touch() { m_versioned.touch(); }
	void touch() volatile { m_versioned.touch(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	version_t exchange(ref<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ref<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ptr<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, type2*& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ref<type>& src, version_t& version) { return m_versioned.versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ref<type>& src, version_t& version) volatile { return m_versioned.versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) volatile { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

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
class versioned_ref<const void>
{
public:
	typedef const void type;

	typedef versioned_ref<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef ref<type> lock_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A versioned_ref with a different referenced type.
		typedef versioned_ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	versioned_t<type*> m_versioned;

	template <typename>
	friend class versioned_ptr;

	versioned_t<type*>& get_versioned() { return m_versioned; }
	const versioned_t<type*>& get_versioned() const { return m_versioned; }
	volatile versioned_t<type*>& get_versioned() volatile { return m_versioned; }
	const volatile versioned_t<type*>& get_versioned() const volatile { return m_versioned; }

	version_t set(type* src) { return m_versioned.set(src); }
	void unversioned_set(type* src) { m_versioned.unversioned_set(src); }
	version_t set(type* src) volatile { return m_versioned.set(src); }

	versioned_ref() : m_versioned(0) { }

	template <typename type2>
	versioned_ref(type2* r) : m_versioned(r) { }

	template <typename type2>
	versioned_ref(const ptr<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ptr<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }


public:
	template <typename type2>
	versioned_ref(const ref<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ref<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ref<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ref<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr() const volatile { type* tmp; m_versioned.get(tmp); return tmp; }

	void get(type*& t, version_t& v) const { m_versioned.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_versioned.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_versioned.get(t.get_ptr_ref(), v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_versioned.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_versioned.get_version(v); }
	void get_version(version_t& v) const volatile { m_versioned.get_version(v); }
	version_t get_version() const { return m_versioned.get_version(); }
	version_t get_version() const volatile { return m_versioned.get_version(); }

	void touch() { m_versioned.touch(); }
	void touch() volatile { m_versioned.touch(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	version_t exchange(ref<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ref<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ptr<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, type2*& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ref<type>& src, version_t& version) { return m_versioned.versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ref<type>& src, version_t& version) volatile { return m_versioned.versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) volatile { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

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
class versioned_ref<volatile void>
{
public:
	typedef volatile void type;

	typedef versioned_ref<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef ref<type> lock_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A versioned_ref with a different referenced type.
		typedef versioned_ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	versioned_t<type*> m_versioned;

	template <typename>
	friend class versioned_ptr;

	versioned_t<type*>& get_versioned() { return m_versioned; }
	const versioned_t<type*>& get_versioned() const { return m_versioned; }
	volatile versioned_t<type*>& get_versioned() volatile { return m_versioned; }
	const volatile versioned_t<type*>& get_versioned() const volatile { return m_versioned; }

	version_t set(type* src) { return m_versioned.set(src); }
	void unversioned_set(type* src) { m_versioned.unversioned_set(src); }
	version_t set(type* src) volatile { return m_versioned.set(src); }

	versioned_ref() : m_versioned(0) { }

	template <typename type2>
	versioned_ref(type2* r) : m_versioned(r) { }

	template <typename type2>
	versioned_ref(const ptr<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ptr<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }


public:
	template <typename type2>
	versioned_ref(const ref<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ref<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ref<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ref<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr() const volatile { type* tmp; m_versioned.get(tmp); return tmp; }

	void get(type*& t, version_t& v) const { m_versioned.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_versioned.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_versioned.get(t.get_ptr_ref(), v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_versioned.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_versioned.get_version(v); }
	void get_version(version_t& v) const volatile { m_versioned.get_version(v); }
	version_t get_version() const { return m_versioned.get_version(); }
	version_t get_version() const volatile { return m_versioned.get_version(); }

	void touch() { m_versioned.touch(); }
	void touch() volatile { m_versioned.touch(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	version_t exchange(ref<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ref<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ptr<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, type2*& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ref<type>& src, version_t& version) { return m_versioned.versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ref<type>& src, version_t& version) volatile { return m_versioned.versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) volatile { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

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
class versioned_ref<const volatile void>
{
public:
	typedef const volatile void type;

	typedef versioned_ref<type> this_t;
	typedef versioned_ptr<type> nullable;
	typedef versioned_ref<type> non_nullable;
	typedef ref<type> lock_t;

	typedef typename versioned_t<type*>::version_t version_t;

	/// @brief Provides a versioned_ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A versioned_ref with a different referenced type.
		typedef versioned_ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	versioned_t<type*> m_versioned;

	template <typename>
	friend class versioned_ptr;

	versioned_t<type*>& get_versioned() { return m_versioned; }
	const versioned_t<type*>& get_versioned() const { return m_versioned; }
	volatile versioned_t<type*>& get_versioned() volatile { return m_versioned; }
	const volatile versioned_t<type*>& get_versioned() const volatile { return m_versioned; }

	version_t set(type* src) { return m_versioned.set(src); }
	void unversioned_set(type* src) { m_versioned.unversioned_set(src); }
	version_t set(type* src) volatile { return m_versioned.set(src); }

	versioned_ref() : m_versioned(0) { }

	template <typename type2>
	versioned_ref(type2* r) : m_versioned(r) { }

	template <typename type2>
	versioned_ref(const ptr<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ptr<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }


public:
	template <typename type2>
	versioned_ref(const ref<type2>& t)
		: m_versioned(t.get_ptr())
	{ }

	template <typename type2>
	versioned_ref(const versioned_ref<type2>& vp)
		: m_versioned(vp.get_ptr())
	{ }

	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2> this_t& operator=(const versioned_ref<type2>& vp) { set(vp.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& p) { set(p.get_ptr()); return *this; }

	template <typename type2> ref<type> operator=(const versioned_ref<type2>& vp) volatile { set(vp.get_ptr()); return ref<type>(*vp.get_ptr()); }
	template <typename type2> ref<type> operator=(const ref<type2>& p) volatile { set(p.get_ptr()); return p; }

	type* get_ptr() const { type* tmp; m_versioned.get(tmp); return tmp; }
	type* get_ptr() const volatile { type* tmp; m_versioned.get(tmp); return tmp; }

	void get(type*& t, version_t& v) const { m_versioned.get(t, v); }
	void get(type*& t, version_t& v) const volatile { m_versioned.get(t, v); }

	void get(ptr<type>& t, version_t& v) const { m_versioned.get(t.get_ptr_ref(), v); }
	void get(ptr<type>& t, version_t& v) const volatile { m_versioned.get(t.get_ptr_ref(), v); }

	void get_version(version_t& v) const { m_versioned.get_version(v); }
	void get_version(version_t& v) const volatile { m_versioned.get_version(v); }
	version_t get_version() const { return m_versioned.get_version(); }
	version_t get_version() const volatile { return m_versioned.get_version(); }

	void touch() { m_versioned.touch(); }
	void touch() volatile { m_versioned.touch(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }
	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile versioned_ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const versioned_ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }

	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }

	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile versioned_ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const versioned_ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }

	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile versioned_ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const versioned_ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }

	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }

	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile versioned_ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const versioned_ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }

	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }

	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	version_t exchange(ref<type2>& wth) volatile { return exchange(wth, wth); }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ref<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, ptr<type2>& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	version_t exchange(const ref<type>& src, type2*& rtn) volatile { type* tmp; version_t result = m_versioned.exchange(src.get_ptr(), tmp); rtn = tmp; return result; }


	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ref<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, ptr<type2>& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void unversioned_exchange(const ref<type>& src, type2*& rtn) { m_versioned.unversioned_exchange(src.get_ptr(), rtn); }


	bool versioned_exchange(const ref<type>& src, version_t& version) { return m_versioned.versioned_exchange(src.get_ptr(), version); }
	bool versioned_exchange(const ref<type>& src, version_t& version) volatile { return m_versioned.versioned_exchange(src.get_ptr(), version); }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }

	template <typename type2>
	bool versioned_exchange(const ref<type>& src, version_t& version, type2*& rtn) volatile { type* tmp; bool result = m_versioned.versioned_exchange(src.get_ptr(), version, tmp); rtn = tmp; return result; }


	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp) volatile { return m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) volatile { type* tmp; bool result = m_versioned.compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }


	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp) { return m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr()); }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ref<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, ptr<type2>& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename type2>
	bool unversioned_compare_exchange(const ref<type>& src, const ref<type>& cmp, type2*& rtn) { type* tmp; bool result = m_versioned.unversioned_compare_exchange(src.get_ptr(), cmp.get_ptr(), tmp); rtn = tmp; return result; }

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;
};


}


// placement operator new/delete for versioned_ref<>
template <typename type>
inline void* operator new(size_t sz, const cogs::versioned_ref<type>& p) { return ::operator new(sz, p.get_ptr()); }

template <typename type>
inline void* operator new(size_t sz, const cogs::versioned_ref<type>& p, const std::nothrow_t& nt) throw () { return ::operator new(sz, p.get_ptr(), nt); }


template <typename type>
inline void operator delete(void*, const cogs::versioned_ref<type>& p) throw () { }

template <typename type>
inline void operator delete(void*, const cogs::versioned_ref<type>& p, const std::nothrow_t&) throw () { }


#endif
