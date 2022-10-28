//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_REF
#define COGS_HEADER_MEM_REF

#include <type_traits>

#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/operators.hpp"


namespace cogs {


template <typename type>
class ptr;


// forward declared for to_string().
template <typename T>
class string_t;

template <typename T>
class composite_string_t;


/// @defgroup ReferenceContainerTypes Reference Container Types
/// @{
/// @ingroup Mem
/// @brief Reference container types
/// @}

/// @ingroup ReferenceContainerTypes
/// @brief Wraps a reference (a non-nullable pointer) and adds some additional capabilities.
/// @tparam T Data type referenced
template <typename T>
class ref
{
public:
	/// @brief Alias to the type pointed to
	typedef T type;

	/// @brief Alias to this type.
	typedef ref<type> this_t;

	/// @brief Alias to this type.  If this reference container required locking (such as a 'relocatable handle'), lock_t would be the appropriate type to contain the lock.
	typedef ref<type> lock_t;

	/// @brief Alias to the non_nullable equivalent of this reference type
	typedef ref<type> non_nullable;

	/// @brief Alias to the nullable equivalent of this reference type
	typedef ptr<type> nullable;

	/// @brief Provides a ref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A ref with a different referenced type.
		typedef ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	type* m_value alignas(atomic::get_alignment_v<type*>);

	template <typename>
	friend class ptr;

	void set(type* p) { m_value = p; }
	void set(type* p) volatile { atomic::store(m_value, p); }

	void set(std::nullptr_t) { m_value = nullptr; }
	void set(std::nullptr_t) volatile { atomic::store(m_value, (type*)0); }
	ref(std::nullptr_t) : m_value(nullptr) { }

	ref() : m_value(0) { }

	template <typename type2>
	ref(type2* r) : m_value(r) { }

	/// @{
	/// @brief Sets the value to only the specified marked bit, clearing the original pointer value.
	/// @param mark Bitmask of bits to set.
	void set_to_mark(size_t mark) { set((type*)(mark & mark_mask())); }
	/// @brief Thread-safe versions of set_to_mark()
	void set_to_mark(size_t mark) volatile { set((type*)(mark & mark_mask())); }
	/// @}

public:
	/// @{
	/// @brief Initializes reference to specified value.
	/// @param src Initial value
	template <typename type2>
	ref(type2& src) : m_value(&src) { }

	ref(const this_t& src) : m_value(src.get_ptr()) { }

	ref(const volatile this_t& src) : m_value(src.get_ptr()) { }

	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ref(ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ref(const ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ref(volatile ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ref(const volatile ref<type2>& src) : m_value(src.get_ptr()) { }
	/// @}

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ref(ref<type2>&& src) : m_value(src.m_value)
	{
		src.m_value = 0;
	}

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(ref<type2>&& src)
	{
		m_value = src.m_value;
		src.m_value = 0;
		return *this;
	}


	/// @{
	/// @brief Sets this ref to the specified reference value
	/// @param r Value to set
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void set(type2& r) { set(&r); }
	/// @brief Thread-safe implementation of set().
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void set(type2& r) volatile { set(&r); }
	/// @}

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(type& src) { set(src); return *this; }
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	void operator=(type& src) volatile { set(src); }
	/// @brief Thread-safe implementation of operator=()
	void operator=(const this_t& src) volatile { set(src.get_ptr()); }
	/// @brief Thread-safe implementation of operator=()
	void operator=(const volatile this_t& src) volatile { set(src.get_ptr()); }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const ref<type2>& src) volatile { set(src.get_ptr()); }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); }
	/// @}

	/// @{
	/// @brief Gets the pointer value
	/// @return A native pointer to the encapsulated value
	type* get_ptr() const { return m_value; }
	/// @brief Thread-safe implementation of get_ptr()
	type* get_ptr() const volatile { type* rtn; atomic::load(m_value, rtn); return rtn; }
	/// @}

	/// @{
	/// @brief Gets a const reference to the internal encapsulated pointer value.
	/// @return A const reference to the internal encapsulated pointer value
	type* const& get_ptr_ref() const { return m_value; }
	/// @brief Thread-safe implementation of get_ptr_ref()
	type* const volatile& get_ptr_ref() const volatile { return m_value; }
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value
	type* operator->() const { type* result = get_ptr(); COGS_ASSERT(!!result); return result; }
	/// @brief Thread-safe implementation of operator*()
	type* operator->() const volatile { type* result = get_ptr(); COGS_ASSERT(!!result); return result; }
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value
	type& operator*() const { type* result = get_ptr(); COGS_ASSERT(!!result); return *result; }
	/// @brief Thread-safe implementation of operator*()
	type& operator*() const volatile { type* result = get_ptr(); COGS_ASSERT(!!result); return *result; }
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL
	/// @return True if the pointer value is NULL.
	bool is_empty() const { return false; }
	/// @brief Thread-safe implementation of operator!()
	bool is_empty() const volatile { return false; }
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL.  An alias for is_empty()
	/// @return True if the pointer value is NULL.
	bool operator!() const { return is_empty(); }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return is_empty(); }
	/// @}

	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @}

	/// @{
	/// @brief static_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ref
	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of static_cast_to()
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief dynamic_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ref
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of dynamic_cast_to()
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief reinterpret_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ref
	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of reinterpret_cast_to()
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief const_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ref
	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of const_cast_to()
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }
	/// @}


	template <typename type2>
	void swap(ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(volatile ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(ref<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }


	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	/// @}

	/// @{
	/// @brief Convert the pointer value to a string_t
	/// @tparam char_t Unit type for string_t
	/// @return The pointer value converted to a string_t\<char_t\>
	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	/// @brief Convert the pointer value to a string
	/// @return The pointer value converted to a string
	composite_string_t<wchar_t> to_string() const;

	/// @brief Convert the pointer value to a cstring
	/// @return The pointer value converted to a cstring
	composite_string_t<char> to_cstring() const;

	/// @brief Thread-safe versions of to_string_t()
	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	/// @brief Thread-safe versions of to_string()
	composite_string_t<wchar_t> to_string() const volatile;
	/// @brief Thread-safe versions of to_cstring()
	composite_string_t<char> to_cstring() const volatile;
	/// @}

	/// @brief Gets the number of bits available to be marked on the pointer.
	///
	/// Depending on the size and alignment of what is pointed to, there may be some insignificant bits
	/// that will always be zero.  This allows those bits to be used to temporarily
	/// store something.
	///
	/// - Alignment of 1 indicates there are no spare bits.
	/// - Alignment of 2 indicates there is 1.
	/// - Alignment of 4 indicates there is 2.
	/// - Alignment of 8 indicates there is 3.
	/// @return The number of bits available to be marked on the pointer.
	static constexpr size_t mark_bits() { return range_to_bits_v<0, alignof(type) - 1>; }

	/// @brief Gets a mask with all available mark bits sets.
	/// @return A mask containing all available mark bits set.
	static constexpr size_t mark_mask() { return (1 << mark_bits()) - 1; }

	/// @{
	/// @brief Gets marked bits on the pointer, if any
	/// @return The marked bits, if any
	size_t get_mark() const { return ((size_t)(get_ptr()) & mark_mask()); }
	/// @brief Thread-safe versions of get_mark()
	size_t get_mark() const volatile { return ((size_t)(get_ptr()) & mark_mask()); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with marked bits removed.
	///
	/// If marked bits are set, direct use of the pointer value will include the marked bits.
	/// get_unmarked() must be used instead.
	/// @return The unmarked pointer value
	type* get_unmarked() const { return (type*)((size_t)get_ptr() & (size_t)~mark_mask()); }
	/// @brief Thread-safe versions of get_unmarked()
	type* get_unmarked() const volatile { return (type*)((size_t)get_ptr() & (size_t)~mark_mask()); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with the specified bitmask applied
	/// @param mark Bitmask of bits to mark
	/// @return The pointer value with the mark applied.
	type* get_marked(size_t mark) const { return (type*)((size_t)get_unmarked() | (mark & mark_mask())); }
	/// @brief Thread-safe versions of get_marked()
	type* get_marked(size_t mark) const volatile { return (type*)((size_t)get_unmarked() | (mark & mark_mask())); }
	/// @}

	/// @{
	/// @brief Clears any marked bits.
	void clear_mark() { set(get_unmarked()); }
	/// @brief Thread-safe versions of clear_mark()
	void clear_mark() volatile
	{
		ref<type> oldValue = *this;
		ref<type> newValue = oldValue;
		for (;;)
		{
			newValue.clear_mark();
			if ((oldValue == newValue) || compare_exchange(newValue, oldValue, oldValue))
				break;
			newValue = oldValue;
		}
	}
	/// @}

	/// @{
	/// @brief Clears the pointer value, but keeps marked bits.
	void clear_to_mark() { set((type*)(get_mark())); }
	/// @brief Thread-safe versions of clear_to_mark()
	void clear_to_mark() volatile
	{
		ref<type> oldValue = *this;
		ref<type> newValue = oldValue;
		for (;;)
		{
			newValue.clear_to_mark();
			if ((oldValue == newValue) || compare_exchange(newValue, oldValue, oldValue))
				break;
			newValue = oldValue;
		}
	}
	/// @}

	/// @{
	/// @brief Sets marked bits.
	/// @param mark Bitmask of bits to set
	void set_mark(size_t mark) { set((type*)((size_t)get_unmarked() | (mark & mark_mask()))); }
	/// @brief Thread-safe versions of set_mark()
	void set_mark(size_t mark) volatile
	{
		ref<type> oldValue = *this;
		ref<type> newValue = oldValue;
		for (;;)
		{
			newValue.set_mark(mark);
			if ((oldValue == newValue) || compare_exchange(newValue, oldValue, oldValue))
				break;
			newValue = oldValue;
		}
	}
	/// @}

	/// @{
	/// @brief Set to the pointer value specified, and apply the speficied marked bits.
	/// @param p Value to set.
	/// @param mark Bitmask of bits to set.
	void set_marked(type* p, size_t mark) { set((type*)((size_t)p | (mark & mark_mask()))); }
	/// @brief Thread-safe versions of set_marked()
	void set_marked(type* p, size_t mark) volatile { set((type*)((size_t)p | (mark & mark_mask()))); }
	/// @}

	/// @{
	/// @brief Index operator
	/// @param i Index value.
	/// @return a reference to the element at that index
	type& operator[](size_t i) const { return get_ptr()[i]; }
	/// @brief Thread-safe versions of type& operator[]()
	type& operator[](size_t i) const volatile { return get_ptr()[i]; }
	/// @}

private:
	this_t next() const { return cogs::next(get_ptr_ref()); }
	this_t next() const volatile { return cogs::next(get_ptr_ref()); }
	void assign_next() { cogs::assign_next(get_ptr_ref()); }
	void assign_next() volatile { cogs::assign_next(get_ptr_ref()); }
	this_t& operator++() { cogs::assign_next(get_ptr_ref()); return *this; }
	this_t operator++() volatile { return cogs::pre_assign_next(get_ptr_ref()); }
	this_t operator++(int) { return cogs::post_assign_next(get_ptr_ref()); }
	this_t operator++(int) volatile { return cogs::post_assign_next(get_ptr_ref()); }

	this_t prev() const { return cogs::prev(get_ptr_ref()); }
	this_t prev() const volatile { return cogs::prev(get_ptr_ref()); }
	void assign_prev() { cogs::assign_prev(get_ptr_ref()); }
	void assign_prev() volatile { cogs::assign_prev(get_ptr_ref()); }
	this_t& operator--() { cogs::assign_prev(get_ptr_ref()); return *this; }
	this_t operator--() volatile { return cogs::pre_assign_prev(get_ptr_ref()); }
	this_t operator--(int) { return cogs::post_assign_prev(get_ptr_ref()); }
	this_t operator--(int) volatile { return cogs::post_assign_prev(get_ptr_ref()); }


	/// @{
	/// @brief Gets a reference to the internal encapsulated pointer value.
	/// @return A reference to the internal encapsulated pointer value
	type*& get_ptr_ref() { return m_value; }
	/// @brief Thread-safe version of get_ptr_ref()
	type* volatile& get_ptr_ref() volatile { return m_value; }
	/// @}
};


template <>
class ref<void>
{
public:
	typedef void type;
	typedef ref<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	type* m_value alignas(atomic::get_alignment_v<type*>);

	template <typename>
	friend class ptr;

	void set(type* p) { m_value = p; }
	void set(type* p) volatile { atomic::store(m_value, p); }

	void set(std::nullptr_t) { m_value = nullptr; }
	void set(std::nullptr_t) volatile { atomic::store(m_value, (type*)0); }
	ref(std::nullptr_t) : m_value(nullptr) { }

	type*& get_ptr_ref() { return m_value; }
	type* volatile& get_ptr_ref() volatile { return m_value; }

	ref() : m_value(0) { }
	ref(type* r) : m_value(r) { }

public:
	ref(const this_t& r) : m_value(r.get_ptr()) { }
	ref(const volatile this_t& r) : m_value(r.get_ptr()) { }

	template <typename type2> ref(const ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2> ref(const volatile ref<type2>& src) : m_value(src.get_ptr()) { }


	ref(ref<type>&& src) : m_value(src.m_value) { src.m_value = 0; }
	template <typename type2> ref(ref<type2>&& src) : m_value(src.m_value) { src.m_value = 0; }
	this_t& operator=(ref<type>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }


	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }

	void operator=(const this_t& src) volatile { set(src.get_ptr()); }
	void operator=(const volatile this_t& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const ref<type2>& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); }

	type* get_ptr() const { return m_value; }
	type* get_ptr() const volatile { type* rtn; atomic::load(m_value, rtn); return rtn; }

	type* const& get_ptr_ref() const { return m_value; }
	type* const volatile& get_ptr_ref() const volatile { return m_value; }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* p) const { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator==(type2* p) const volatile { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const volatile { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const volatile { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const volatile { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const volatile { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const volatile { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }


	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	void swap(ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(volatile ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(ref<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }


	template <typename type2>
	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
class ref<const void>
{
public:
	typedef const void type;
	typedef ref<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	type* m_value alignas(atomic::get_alignment_v<type*>);

	template <typename>
	friend class ptr;

	void set(type* p) { m_value = p; }
	void set(type* p) volatile { atomic::store(m_value, p); }

	void set(std::nullptr_t) { m_value = nullptr; }
	void set(std::nullptr_t) volatile { atomic::store(m_value, (type*)0); }
	ref(std::nullptr_t) : m_value(nullptr) { }

	type*& get_ptr_ref() { return m_value; }
	type* volatile& get_ptr_ref() volatile { return m_value; }

	ref() : m_value(0) { }
	ref(type* r) : m_value(r) { }

public:
	ref(const this_t& r) : m_value(r.get_ptr()) { }
	ref(const volatile this_t& r) : m_value(r.get_ptr()) { }

	template <typename type2> ref(const ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2> ref(const volatile ref<type2>& src) : m_value(src.get_ptr()) { }

	ref(ref<type>&& src) : m_value(src.m_value) { src.m_value = 0; }
	template <typename type2> ref(ref<type2>&& src) : m_value(src.m_value) { src.m_value = 0; }
	this_t& operator=(ref<type>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }


	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }

	void operator=(const this_t& src) volatile { set(src.get_ptr()); }
	void operator=(const volatile this_t& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const ref<type2>& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); }

	type* get_ptr() const { return m_value; }
	type* get_ptr() const volatile { type* rtn; atomic::load(m_value, rtn); return rtn; }

	type* const& get_ptr_ref() const { return m_value; }
	type* const volatile& get_ptr_ref() const volatile { return m_value; }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* p) const { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator==(type2* p) const volatile { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const volatile { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const volatile { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const volatile { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const volatile { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const volatile { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }


	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	void swap(ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(volatile ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(ref<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }


	template <typename type2>
	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
class ref<volatile void>
{
public:
	typedef volatile void type;
	typedef ref<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	type* m_value alignas(atomic::get_alignment_v<type*>);

	template <typename>
	friend class ptr;

	void set(type* p) { m_value = p; }
	void set(type* p) volatile { atomic::store(m_value, p); }

	void set(std::nullptr_t) { m_value = nullptr; }
	void set(std::nullptr_t) volatile { atomic::store(m_value, (type*)0); }
	ref(std::nullptr_t) : m_value(nullptr) { }

	type*& get_ptr_ref() { return m_value; }
	type* volatile& get_ptr_ref() volatile { return m_value; }

	ref() : m_value(0) { }
	ref(type* r) : m_value(r) { }

public:
	ref(const this_t& r) : m_value(r.get_ptr()) { }
	ref(const volatile this_t& r) : m_value(r.get_ptr()) { }

	template <typename type2> ref(const ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2> ref(const volatile ref<type2>& src) : m_value(src.get_ptr()) { }

	ref(ref<type>&& src) : m_value(src.m_value) { src.m_value = 0; }
	template <typename type2> ref(ref<type2>&& src) : m_value(src.m_value) { src.m_value = 0; }
	this_t& operator=(ref<type>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }


	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }

	void operator=(const this_t& src) volatile { set(src.get_ptr()); }
	void operator=(const volatile this_t& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const ref<type2>& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); }

	type* get_ptr() const { return m_value; }
	type* get_ptr() const volatile { type* rtn; atomic::load(m_value, rtn); return rtn; }

	type* const& get_ptr_ref() const { return m_value; }
	type* const volatile& get_ptr_ref() const volatile { return m_value; }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* p) const { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator==(type2* p) const volatile { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const volatile { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const volatile { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const volatile { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const volatile { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const volatile { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }


	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	void swap(ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(volatile ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(ref<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }


	template <typename type2>
	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;

	// void references don't support mark bits, because they could be using all of their bits to point to something.
	// Provided to maintain the interface.
};


template <>
class ref<const volatile void>
{
public:
	typedef const volatile void type;
	typedef ref<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

private:
	type* m_value alignas(atomic::get_alignment_v<type*>);

	template <typename>
	friend class ptr;

	void set(type* p) { m_value = p; }
	void set(type* p) volatile { atomic::store(m_value, p); }

	void set(std::nullptr_t) { m_value = nullptr; }
	void set(std::nullptr_t) volatile { atomic::store(m_value, (type*)0); }
	ref(std::nullptr_t) : m_value(nullptr) { }

	type*& get_ptr_ref() { return m_value; }
	type* volatile& get_ptr_ref() volatile { return m_value; }

	ref() : m_value(0) { }
	ref(type* r) : m_value(r) { }

public:
	ref(const this_t& r) : m_value(r.get_ptr()) { }
	ref(const volatile this_t& r) : m_value(r.get_ptr()) { }

	template <typename type2> ref(const ref<type2>& src) : m_value(src.get_ptr()) { }
	template <typename type2> ref(const volatile ref<type2>& src) : m_value(src.get_ptr()) { }

	ref(ref<type>&& src) : m_value(src.m_value) { src.m_value = 0; }
	template <typename type2> ref(ref<type2>&& src) : m_value(src.m_value) { src.m_value = 0; }
	this_t& operator=(ref<type>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_value = src.m_value; src.m_value = 0; return *this; }


	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }

	void operator=(const this_t& src) volatile { set(src.get_ptr()); }
	void operator=(const volatile this_t& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const ref<type2>& src) volatile { set(src.get_ptr()); }
	template <typename type2> void operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); }

	type* get_ptr() const { return m_value; }
	type* get_ptr() const volatile { type* rtn; atomic::load(m_value, rtn); return rtn; }

	type* const& get_ptr_ref() const { return m_value; }
	type* const volatile& get_ptr_ref() const volatile { return m_value; }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }

	template <typename type2> bool operator==(type2* p) const { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator==(type2* p) const volatile { return get_ptr() == p; }
	template <typename type2> bool operator==(const ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& p) const volatile { return get_ptr() == p.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& r) const volatile { return get_ptr() == r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator!=(type2* p) const volatile { return get_ptr() != p; }
	template <typename type2> bool operator!=(const ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& p) const volatile { return get_ptr() != p.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& r) const volatile { return get_ptr() != r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator>(type2* p) const volatile { return get_ptr() > p; }
	template <typename type2> bool operator>(const ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }
	template <typename type2> bool operator>(const volatile ptr<type2>& p) const volatile { return get_ptr() > p.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& r) const volatile { return get_ptr() > r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator<(type2* p) const volatile { return get_ptr() < p; }
	template <typename type2> bool operator<(const ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }
	template <typename type2> bool operator<(const volatile ptr<type2>& p) const volatile { return get_ptr() < p.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& r) const volatile { return get_ptr() < r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator>=(type2* p) const volatile { return get_ptr() >= p; }
	template <typename type2> bool operator>=(const ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& p) const volatile { return get_ptr() >= p.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& r) const volatile { return get_ptr() >= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const { return get_ptr() <= r.get_ptr(); }

	template <typename type2> bool operator<=(type2* p) const volatile { return get_ptr() <= p; }
	template <typename type2> bool operator<=(const ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& p) const volatile { return get_ptr() <= p.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& r) const volatile { return get_ptr() <= r.get_ptr(); }


	template <typename type2> ref<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ref<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }


	template <typename type2>
	void swap(ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(volatile ref<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(ref<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }


	template <typename type2>
	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	template <typename type2>
	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ref<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	bool compare_exchange(const this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }

	bool compare_exchange(const this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr()); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ref<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename char_t>
	composite_string_t<char_t> to_string_t() const;

	composite_string_t<wchar_t> to_string() const;
	composite_string_t<char> to_cstring() const;

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const volatile;

	composite_string_t<wchar_t> to_string() const volatile;
	composite_string_t<char> to_cstring() const volatile;

	// void references don't support mark bits, because they could be using all of their bits to point to something.
	// Provided to maintain the interface.
};


}


#endif
