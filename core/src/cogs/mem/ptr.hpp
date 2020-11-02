//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_PTR
#define COGS_HEADER_MEM_PTR

#include <type_traits>


#include "cogs/assert.hpp"
#include "cogs/mem/ref.hpp"


namespace cogs {


/// @ingroup ReferenceContainerTypes
/// @brief Wraps a pointer and adds some additional capabilities.
/// @tparam T Data type pointed to
template <typename T>
class ptr
{
private:
	ref<T> m_ref;

public:
	/// @brief Alias to the type pointed to
	typedef T type;

	/// @brief Alias to this type.
	typedef ptr<type> this_t;

	/// @brief Alias to this type.  If this reference container required locking (such as a 'relocatable handle'), locked_t would be the appropriate type to contain the lock.
	typedef ptr<type> locked_t;

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
		typedef ptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;


	/// @{
	/// @brief Constructor.  Initializes pointer to 0.
	ptr() : m_ref(nullptr) { }
	ptr(std::nullptr_t) : m_ref(nullptr) { }
	/// @}


	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(type2* src) : m_ref(src) { }

	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(const ptr<type2>& src) : m_ref(src.get_ptr()) { }

	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(const ref<type2>& src) : m_ref(src.get_ptr()) { }

	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(const volatile ptr<type2>& src) : m_ref(src.get_ptr()) { }

	/// @brief Initializes reference to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(const volatile ref<type2>& src) : m_ref(src.get_ptr()) { }
	/// @}


	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(ptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	ptr(ref<type2>&& src) : m_ref(std::move(src)) { }


	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(ptr<type2>&& src)
	{
		m_ref = std::move(src.m_ref);
		return *this;
	}

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(ref<type2>&& src)
	{
		m_ref = std::move(src);
		return *this;
	}


	/// @{
	/// @brief Sets the pointer value to NULL
	void release() { set(nullptr); }
	/// @brief Thread-safe implementation of release().
	void release() volatile { set(nullptr); }
	/// @}

	/// @{
	/// @brief Sets this ref to the specified reference value
	/// @param p Value to set
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void set(type2* p) { m_ref.set(p); }
	/// @brief Thread-safe implementation of set().
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void set(type2* p) volatile { m_ref.set(p); }
	/// @}

	void set(std::nullptr_t) { m_ref.set(nullptr); }
	void set(std::nullptr_t) volatile { m_ref.set(nullptr); }

	this_t& operator=(std::nullptr_t) { set(nullptr); return *this; }
	volatile this_t& operator=(std::nullptr_t) volatile { set(nullptr); return *this; }


	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(type2* src) { set(src); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(type* src) volatile { set(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const volatile this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const volatile ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @}

	/// @{
	/// @brief Gets the pointer value
	/// @return A native pointer to the encapsulated value
	type* get_ptr() const { return m_ref.get_ptr(); }
	/// @brief Thread-safe implementation of get_ptr()
	type* get_ptr() const volatile { return m_ref.get_ptr(); }
	/// @}

	//operator type*() const { return get_ptr(); }
	//operator type*() const volatile { return get_ptr(); }


	/// @{
	/// @brief Gets a reference to this object's pointer value, as a ref
	///
	/// It is caller error to invoke dereference() on a NULL pointer.  The result is undefined.
	/// @return A reference to this object's pointer value, as a ref
	ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Const implementation of dereference().
	const ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	volatile ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	const volatile ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @}

	/// @{
	/// @brief Gets a reference to the internal encapsulated pointer value.
	/// @return A reference to the internal encapsulated pointer value
	type*& get_ptr_ref() { return m_ref.get_ptr_ref(); }
	/// @brief Const implementation of get_ptr_ref()
	type* const& get_ptr_ref() const { return m_ref.get_ptr_ref(); }
	/// @brief Thread-safe implementation of get_ptr_ref()
	type* volatile& get_ptr_ref() volatile { return m_ref.get_ptr_ref(); }
	/// @brief Thread-safe implementation of get_ptr_ref()
	type* const volatile& get_ptr_ref() const volatile { return m_ref.get_ptr_ref(); }
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value, dereferenced
	type* operator->() const { type* result = get_ptr(); COGS_ASSERT(!!result); return result; }
	/// @brief Thread-safe implementation of operator*()
	type* operator->() const volatile { type* result = get_ptr(); COGS_ASSERT(!!result); return result; }
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value, dereferenced
	type& operator*() const { type* result = get_ptr(); COGS_ASSERT(!!result); return *result; }
	/// @brief Thread-safe implementation of operator*()
	type& operator*() const volatile { type* result = get_ptr(); COGS_ASSERT(!!result); return *result; }
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL
	/// @return True if the pointer value is NULL.
	bool is_empty() const { return !get_ptr(); }
	/// @brief Thread-safe implementation of operator!()
	bool is_empty() const volatile { return !get_ptr(); }
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL.  An alias for is_empty()
	/// @return True if the pointer value is NULL.
	bool operator!() const { return is_empty(); }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return is_empty(); }
	/// @}


	template <typename T2>
	auto operator+(T2&& src) const { return cogs::add(get_ptr(), std::forward<T2>(src)); }
	template <typename T2>
	auto operator+(T2&& src) const volatile { return cogs::add(get_ptr(), std::forward<T2>(src)); }

	template <typename T2>
	this_t& operator+=(T2&& src) { cogs::assign_add(get_ptr_ref(), std::forward<T2>(src)); return *this; }
	template <typename T2>
	volatile this_t& operator+=(T2&& src) volatile { cogs::assign_add(get_ptr_ref(), std::forward<T2>(src)); return *this; }

	template <typename T2>
	const this_t& pre_assign_add(T2&& src) { cogs::assign_add(get_ptr_ref(), std::forward<T2>(src)); return *this; }
	template <typename T2>
	this_t pre_assign_add(T2&& src) volatile { return cogs::pre_assign_add(get_ptr_ref(), std::forward<T2>(src)); }

	template <typename T2>
	this_t post_assign_add(T2&& src) { return cogs::post_assign_add(get_ptr_ref(), std::forward<T2>(src)); }
	template <typename T2>
	this_t post_assign_add(T2&& src) volatile { return cogs::post_assign_add(get_ptr_ref(), std::forward<T2>(src)); }


	template <typename T2>
	auto operator-(T2&& src) const { return cogs::subtract(get_ptr(), std::forward<T2>(src)); }

	template <typename T2>
	auto operator-(ref<T2>&& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(ref<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const ref<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(volatile ref<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const volatile ref<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }

	template <typename T2>
	auto operator-(ptr<T2>&& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(ptr<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const ptr<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(volatile ptr<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const volatile ptr<T2>& src) const { return cogs::subtract(get_ptr(), src.get_ptr()); }


	template <typename T2>
	auto operator-(T2&& src) const volatile { return cogs::subtract(get_ptr(), std::forward<T2>(src)); }

	template <typename T2>
	auto operator-(ref<T2>&& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(ref<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const ref<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(volatile ref<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const volatile ref<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }

	template <typename T2>
	auto operator-(ptr<T2>&& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(ptr<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const ptr<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(volatile ptr<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }
	template <typename T2>
	auto operator-(const volatile ptr<T2>& src) const volatile { return cogs::subtract(get_ptr(), src.get_ptr()); }


	template <typename T2>
	this_t& operator-=(T2&& src) { cogs::assign_subtract(get_ptr_ref(), std::forward<T2>(src)); return *this; }
	template <typename T2>
	volatile this_t& operator-=(T2&& src) volatile { cogs::assign_subtract(get_ptr_ref(), std::forward<T2>(src)); return *this; }

	template <typename T2>
	const this_t& pre_assign_subtract(T2&& src) { cogs::assign_subtract(get_ptr_ref(), std::forward<T2>(src)); return *this; }
	template <typename T2>
	this_t pre_assign_subtract(T2&& src) volatile { return cogs::pre_assign_subtract(get_ptr_ref(), std::forward<T2>(src)); }

	template <typename T2>
	this_t post_assign_subtract(T2&& src) { return cogs::post_assign_subtract(get_ptr_ref(), std::forward<T2>(src)); }
	template <typename T2>
	this_t post_assign_subtract(T2&& src) volatile { return cogs::post_assign_subtract(get_ptr_ref(), std::forward<T2>(src)); }


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
	/// @return Result of the cast, as a ptr
	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of static_cast_to()
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief dynamic_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ptr
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of dynamic_cast_to()
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief reinterpret_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ptr
	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of reinterpret_cast_to()
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	/// @}

	/// @{
	/// @brief const_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast, as a ptr
	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	/// @brief Thread-safe implementation of const_cast_to()
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }
	/// @}


	template <typename type2>
	void swap(ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(volatile ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2* volatile& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(ptr<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) volatile { cogs::swap(get_ptr_ref(), wth); }


	this_t exchange(const this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const volatile this_t& src) { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }

	this_t exchange(const volatile this_t& src) volatile { return cogs::exchange(get_ptr_ref(), src.get_ptr()); }


	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


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
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
	static size_t mark_bits() { return ref<T>::mark_bits(); }

	/// @brief Gets a mask with all available mark bits sets.
	/// @return A mask containing all available mark bits set.
	static size_t mark_mask() { return ref<T>::mark_mask(); }

	/// @{
	/// @brief Gets marked bits on the pointer, if any
	/// @return The marked bits, if any
	size_t get_mark() const { return m_ref.get_mark(); }
	/// @brief Thread-safe versions of get_mark()
	size_t get_mark() const volatile { return m_ref.get_mark(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with marked bits removed.
	///
	/// If marked bits are set, direct use of the pointer value will include the marked bits.
	/// get_unmarked() must be used instead.
	/// @return The unmarked pointer value
	type* get_unmarked() const { return m_ref.get_unmarked(); }
	/// @brief Thread-safe versions of get_mark()
	type* get_unmarked() const volatile { return m_ref.get_unmarked(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with the specified bitmask applied
	/// @param mark Bitmask of bits to mark
	/// @return The pointer value with the mark applied.
	type* get_marked(size_t mark) const { return m_ref.get_marked(mark); }
	/// @brief Thread-safe versions of get_mark()
	type* get_marked(size_t mark) const volatile { return m_ref.get_marked(mark); }
	/// @}

	/// @{
	/// @brief Clears any marked bits.
	void clear_mark() { m_ref.clear_mark(); }
	/// @brief Thread-safe versions of get_mark()
	void clear_mark() volatile { m_ref.clear_mark(); }
	/// @}

	/// @{
	/// @brief Clears the pointer value, but keeps marked bits.
	void clear_to_mark() { m_ref.clear_to_mark(); }
	/// @brief Thread-safe versions of clear_to_mark()
	void clear_to_mark() volatile { m_ref.clear_to_mark(); }
	/// @}

	/// @{
	/// @brief Sets marked bits.
	/// @param mark Bitmask of bits to set
	void set_mark(size_t mark) { m_ref.set_mark(mark); }
	/// @brief Thread-safe versions of get_mark()
	void set_mark(size_t mark) volatile { m_ref.set_mark(mark); }
	/// @}

	/// @{
	/// @brief Sets the value to only the specified marked bit, clearing the original pointer value.
	/// @param mark Bitmask of bits to set.
	void set_to_mark(size_t mark) { m_ref.set_to_mark(mark); }
	/// @brief Thread-safe versions of get_mark()
	void set_to_mark(size_t mark) volatile { m_ref.set_to_mark(mark); }
	/// @}

	/// @{
	/// @brief Set to the pointer value specified, and apply the speficied marked bits.
	/// @param p Value to set.
	/// @param mark Bitmask of bits to set.
	void set_marked(type* p, size_t mark) { m_ref.set_marked(p, mark); }
	/// @brief Thread-safe versions of get_mark()
	void set_marked(type* p, size_t mark) volatile { m_ref.set_marked(p, mark); }
	/// @}

	/// @{
	/// @brief Index operator
	/// @param i Index value.
	/// @return A reference to the element at that index
	type& operator[](size_t i) const { return get_ptr()[i]; }
	/// @brief Thread-safe versions of operator[]()
	type& operator[](size_t i) const volatile { return get_ptr()[i]; }
	/// @}
};


template <>
class ptr<void>
{
private:
	ref<void> m_ref;

public:
	typedef void type;
	typedef ptr<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	ptr() : m_ref(nullptr) { }
	ptr(std::nullptr_t) : m_ref(nullptr) { }
	ptr(type* src) : m_ref(src) { }
	ptr(const ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const ref<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ref<type>& src) : m_ref(src.get_ptr()) { }


	template <typename type2> ptr(type2* src) : m_ref(src) { }
	template <typename type2> ptr(const ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const ref<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ref<type2>& src) : m_ref(src.get_ptr()) { }


	ptr(ptr<type>&& src) : m_ref(std::move(src.m_ref)) { }
	ptr(ref<type>&& src) : m_ref(std::move(src)) { }
	template <typename type2> ptr(ptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> ptr(ref<type2>&& src) : m_ref(std::move(src)) { }
	this_t& operator=(ptr<type>&& src) { m_ref = std::move(src.m_ref); return *this; }
	this_t& operator=(ref<type>&& src) { m_ref = std::move(src); return *this; }
	template <typename type2> this_t& operator=(ptr<type2>&& src) { m_ref = std::move(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_ref = std::move(src); return *this; }


	void release() { set(nullptr); }
	void release() volatile { set(nullptr); }

	void set(type* p) { m_ref.set(p); }
	void set(type* p) volatile { m_ref.set(p); }

	void set(std::nullptr_t) { m_ref.set(nullptr); }
	void set(std::nullptr_t) volatile { m_ref.set(nullptr); }

	this_t& operator=(std::nullptr_t) { set(nullptr); return *this; }
	volatile this_t& operator=(std::nullptr_t) volatile { set(nullptr); return *this; }

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(type* src) { set(src); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(type* src) volatile { set(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const volatile this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @}

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }


	/// @{
	/// @brief Gets a reference to this object's pointer value, as a ref
	///
	/// It is caller error to invoke dereference() on a NULL pointer.  The result is undefined.
	/// @return A reference to this object's pointer value, as a ref
	ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Const implementation of dereference().
	const ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	volatile ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	const volatile ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @}

	type*& get_ptr_ref() { return m_ref.get_ptr_ref(); }
	type* const& get_ptr_ref() const { return m_ref.get_ptr_ref(); }
	type* volatile& get_ptr_ref() volatile { return m_ref.get_ptr_ref(); }
	type* const volatile& get_ptr_ref() const volatile { return m_ref.get_ptr_ref(); }

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

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	void swap(ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(volatile ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2* volatile& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(ptr<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) volatile { cogs::swap(get_ptr_ref(), wth); }


	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


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
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
class ptr<const void>
{
private:
	ref<const void> m_ref;

public:
	typedef const void type;
	typedef ptr<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	ptr() : m_ref(nullptr) { }
	ptr(std::nullptr_t) : m_ref(nullptr) { }
	ptr(type* src) : m_ref(src) { }
	ptr(const ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const ref<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ref<type>& src) : m_ref(src.get_ptr()) { }

	template <typename type2> ptr(type2* src) : m_ref(src) { }
	template <typename type2> ptr(const ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const ref<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ref<type2>& src) : m_ref(src.get_ptr()) { }

	ptr(ptr<type>&& src) : m_ref(std::move(src.m_ref)) { }
	ptr(ref<type>&& src) : m_ref(std::move(src)) { }
	template <typename type2> ptr(ptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> ptr(ref<type2>&& src) : m_ref(std::move(src)) { }
	this_t& operator=(ptr<type>&& src) { m_ref = std::move(src.m_ref); return *this; }
	this_t& operator=(ref<type>&& src) { m_ref = std::move(src); return *this; }
	template <typename type2> this_t& operator=(ptr<type2>&& src) { m_ref = std::move(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_ref = std::move(src); return *this; }

	void release() { set(nullptr); }
	void release() volatile { set(nullptr); }

	void set(type* p) { m_ref.set(p); }
	void set(type* p) volatile { m_ref.set(p); }

	void set(std::nullptr_t) { m_ref.set(nullptr); }
	void set(std::nullptr_t) volatile { m_ref.set(nullptr); }

	this_t& operator=(std::nullptr_t) { set(nullptr); return *this; }
	volatile this_t& operator=(std::nullptr_t) volatile { set(nullptr); return *this; }


	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(type* src) { set(src); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(type* src) volatile { set(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const volatile this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @}

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	/// @{
	/// @brief Gets a reference to this object's pointer value, as a ref
	///
	/// It is caller error to invoke dereference() on a NULL pointer.  The result is undefined.
	/// @return A reference to this object's pointer value, as a ref
	ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Const implementation of dereference().
	const ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	volatile ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	const volatile ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @}

	type*& get_ptr_ref() { return m_ref.get_ptr_ref(); }
	type* const& get_ptr_ref() const { return m_ref.get_ptr_ref(); }
	type* volatile& get_ptr_ref() volatile { return m_ref.get_ptr_ref(); }
	type* const volatile& get_ptr_ref() const volatile { return m_ref.get_ptr_ref(); }

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

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	void swap(ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(volatile ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2* volatile& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(ptr<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) volatile { cogs::swap(get_ptr_ref(), wth); }


	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


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
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
class ptr<volatile void>
{
private:
	ref<volatile void> m_ref;

public:
	typedef volatile void type;
	typedef ptr<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	ptr() : m_ref(nullptr) { }
	ptr(std::nullptr_t) : m_ref(nullptr) { }
	ptr(type* src) : m_ref(src) { }
	ptr(const ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const ref<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ref<type>& src) : m_ref(src.get_ptr()) { }

	template <typename type2> ptr(type2* src) : m_ref(src) { }
	template <typename type2> ptr(const ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const ref<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ref<type2>& src) : m_ref(src.get_ptr()) { }

	ptr(ptr<type>&& src) : m_ref(std::move(src.m_ref)) { }
	ptr(ref<type>&& src) : m_ref(std::move(src)) { }
	template <typename type2> ptr(ptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> ptr(ref<type2>&& src) : m_ref(std::move(src)) { }
	this_t& operator=(ptr<type>&& src) { m_ref = std::move(src.m_ref); return *this; }
	this_t& operator=(ref<type>&& src) { m_ref = std::move(src); return *this; }
	template <typename type2> this_t& operator=(ptr<type2>&& src) { m_ref = std::move(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_ref = std::move(src); return *this; }

	void release() { set(nullptr); }
	void release() volatile { set(nullptr); }

	void set(type* p) { m_ref.set(p); }
	void set(type* p) volatile { m_ref.set(p); }

	void set(std::nullptr_t) { m_ref.set(nullptr); }
	void set(std::nullptr_t) volatile { m_ref.set(nullptr); }

	this_t& operator=(std::nullptr_t) { set(nullptr); return *this; }
	volatile this_t& operator=(std::nullptr_t) volatile { set(nullptr); return *this; }


	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(type* src) { set(src); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(type* src) volatile { set(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const volatile this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @}

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	/// @{
	/// @brief Gets a reference to this object's pointer value, as a ref
	///
	/// It is caller error to invoke dereference() on a NULL pointer.  The result is undefined.
	/// @return A reference to this object's pointer value, as a ref
	ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Const implementation of dereference().
	const ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	volatile ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	const volatile ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @}

	type*& get_ptr_ref() { return m_ref.get_ptr_ref(); }
	type* const& get_ptr_ref() const { return m_ref.get_ptr_ref(); }
	type* volatile& get_ptr_ref() volatile { return m_ref.get_ptr_ref(); }
	type* const volatile& get_ptr_ref() const volatile { return m_ref.get_ptr_ref(); }

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

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	void swap(ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(volatile ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2* volatile& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(ptr<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) volatile { cogs::swap(get_ptr_ref(), wth); }


	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


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
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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
class ptr<const volatile void>
{
private:
	ref<const volatile void> m_ref;

public:
	typedef const volatile void type;
	typedef ptr<type> this_t;
	typedef ref<type> non_nullable;
	typedef ptr<type> nullable;

	template <typename type2>
	class cast
	{
	public:
		typedef ptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	ptr() : m_ref(nullptr) { }
	ptr(std::nullptr_t) : m_ref(nullptr) { }
	ptr(type* src) : m_ref(src) { }
	ptr(const ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const ref<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ptr<type>& src) : m_ref(src.get_ptr()) { }
	ptr(const volatile ref<type>& src) : m_ref(src.get_ptr()) { }

	template <typename type2> ptr(type2* src) : m_ref(src) { }
	template <typename type2> ptr(const ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const ref<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ptr<type2>& src) : m_ref(src.get_ptr()) { }
	template <typename type2> ptr(const volatile ref<type2>& src) : m_ref(src.get_ptr()) { }

	ptr(ptr<type>&& src) : m_ref(std::move(src.m_ref)) { }
	ptr(ref<type>&& src) : m_ref(std::move(src)) { }
	template <typename type2> ptr(ptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> ptr(ref<type2>&& src) : m_ref(std::move(src)) { }
	this_t& operator=(ptr<type>&& src) { m_ref = std::move(src.m_ref); return *this; }
	this_t& operator=(ref<type>&& src) { m_ref = std::move(src); return *this; }
	template <typename type2> this_t& operator=(ptr<type2>&& src) { m_ref = std::move(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(ref<type2>&& src) { m_ref = std::move(src); return *this; }

	void release() { set(nullptr); }
	void release() volatile { set(nullptr); }

	void set(type* p) { m_ref.set(p); }
	void set(type* p) volatile { m_ref.set(p); }

	void set(std::nullptr_t) { m_ref.set(nullptr); }
	void set(std::nullptr_t) volatile { m_ref.set(nullptr); }

	this_t& operator=(std::nullptr_t) { set(nullptr); return *this; }
	volatile this_t& operator=(std::nullptr_t) volatile { set(nullptr); return *this; }

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(const volatile this_t& src) { set(src.get_ptr()); return *this; }
	this_t& operator=(type* src) { set(src); return *this; }
	template <typename type2> this_t& operator=(const ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const ref<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ptr<type2>& src) { set(src.get_ptr()); return *this; }
	template <typename type2> this_t& operator=(const volatile ref<type2>& src) { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(type* src) volatile { set(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	volatile this_t& operator=(const volatile this_t& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ptr<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2> volatile this_t& operator=(const volatile ref<type2>& src) volatile { set(src.get_ptr()); return *this; }
	/// @}

	type* get_ptr() const { return m_ref.get_ptr(); }
	type* get_ptr() const volatile { return m_ref.get_ptr(); }

	/// @{
	/// @brief Gets a reference to this object's pointer value, as a ref
	///
	/// It is caller error to invoke dereference() on a NULL pointer.  The result is undefined.
	/// @return A reference to this object's pointer value, as a ref
	ref<type>& dereference() { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Const implementation of dereference().
	const ref<type>& dereference() const { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	volatile ref<type>& dereference() volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @brief Thread-safe implementation of dereference().
	const volatile ref<type>& dereference() const volatile { COGS_ASSERT(get_ptr() != 0); return m_ref; }
	/// @}

	type*& get_ptr_ref() { return m_ref.get_ptr_ref(); }
	type* const& get_ptr_ref() const { return m_ref.get_ptr_ref(); }
	type* volatile& get_ptr_ref() volatile { return m_ref.get_ptr_ref(); }
	type* const volatile& get_ptr_ref() const volatile { return m_ref.get_ptr_ref(); }

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

	template <typename type2> ptr<type2> static_cast_to() const { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> static_cast_to() const volatile { return static_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> dynamic_cast_to() const volatile { return dynamic_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> reinterpret_cast_to() const volatile { return reinterpret_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const { return const_cast<type2*>(get_ptr()); }
	template <typename type2> ptr<type2> const_cast_to() const volatile { return const_cast<type2*>(get_ptr()); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	void swap(ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(volatile ptr<type2>& wth) { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2* volatile& wth) { cogs::swap(get_ptr_ref(), wth); }

	template <typename type2>
	void swap(ptr<type2>& wth) volatile { cogs::swap(get_ptr_ref(), wth.get_ptr_ref()); }

	template <typename type2>
	void swap(type2*& wth) volatile { cogs::swap(get_ptr_ref(), wth); }


	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }

	template <typename type2>
	void exchange(const this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2*& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const this_t& src, volatile ptr<type2>& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const this_t& src, type2* volatile& rtn) volatile { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


	template <typename type2>
	void exchange(const volatile this_t& src, volatile ptr<type2>& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	void exchange(const volatile this_t& src, type2* volatile& rtn) { cogs::exchange(get_ptr_ref(), src.get_ptr(), rtn); }


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
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2*& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const this_t& src, const volatile this_t& cmp, type2* volatile& rtn) volatile { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, volatile ptr<type2>& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn.get_ptr_ref()); }

	template <typename type2>
	bool compare_exchange(const volatile this_t& src, const volatile this_t& cmp, type2* volatile& rtn) { return cogs::compare_exchange(get_ptr_ref(), src.get_ptr(), cmp.get_ptr(), rtn); }


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


}


// placement operator new/delete for ptr<>
template <typename type>
inline void* operator new(size_t sz, const cogs::ptr<type>& p) { return ::operator new(sz, p.get_ptr()); }

template <typename type>
inline void* operator new(size_t sz, const cogs::ptr<type>& p, const std::nothrow_t& nt) throw () { return ::operator new(sz, p.get_ptr(), nt); }


template <typename type>
inline void operator delete(void*, const cogs::ptr<type>&) throw () { }

template <typename type>
inline void operator delete(void*, const cogs::ptr<type>&, const std::nothrow_t&) throw () { }


#endif
