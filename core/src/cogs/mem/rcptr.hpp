//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCPTR
#define COGS_HEADER_MEM_RCPTR


#include "cogs/env.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/mem/weak_rcptr.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

/// @ingroup ReferenceCountedReferenceContainerTypes
/// @brief A nullable reference-counted reference container.
/// @tparam T Data type pointed to
template <typename T>
class rcptr
{
private:
	rcref<T> m_ref;

public:
	/// @brief Alias to the type pointed to
	typedef T type;

	/// @brief Alias to this type.
	typedef rcptr<T> this_t;

	/// @brief Alias to this type.  If this reference container required locking (such as a 'relocatable handle'), locked_t would be the appropriate type to contain the lock.
	typedef ref<type> locked_t;

	/// @brief Alias to the nullable equivalent of this reference type
	typedef rcptr<T> nullable;

	/// @brief Alias to the non_nullable equivalent of this reference type
	typedef rcref<T> non_nullable;

protected:
	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

public:
	/// @brief Provides a rcptr with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A rcptr with a different referenced type.
		typedef rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	/// @{
	/// @brief Constructor.  Initializes rcptr to NULL
	rcptr() { }
	/// @}

	rcptr(this_t&& src) : m_ref(std::move(src.m_ref)) { }

	/// @{
	/// @brief Initializes rcptr to specified value.
	/// @param src Initial value
	rcptr(const this_t& src) : m_ref(src.m_ref) { }
	rcptr(const volatile this_t& src) : m_ref(src.m_ref) { }


	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(rcref<type2>&& src) : m_ref(std::move(src)) { }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(rcptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(weak_rcptr<type2>&& src) : m_ref(std::move(src)) { }



	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const rcref<type2>& src) : m_ref(src) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const rcptr<type2>& src) : m_ref(src.m_ref) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const weak_rcptr<type2>& src) : m_ref(src) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const volatile rcref<type2>& src) : m_ref(src) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const volatile rcptr<type2>& src) : m_ref(src) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcptr(const volatile weak_rcptr<type2>& src) : m_ref(src) { }

	/// @brief Initializes rcptr to specified value.
	///
	/// A rcptr may contain a non reference-counted pointer.  By specifying a pointer value where a rcptr
	/// is expected, the caller is gauranteeing the resource pointed to will remain in scope until no longer referenced
	/// by the rcptr.
	/// @param src Initial value
	rcptr(type* src) : m_ref(*src) { }

	/// @brief Initializes a rcptr to contain the specified object and descriptor, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_ref(obj, desc) { }
	/// @}

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(type* src) { m_ref.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }
	this_t& operator=(const this_t& src) { m_ref.operator=(src.m_ref); return *this; }
	this_t& operator=(const volatile this_t& src) { m_ref.operator=(src.m_ref); return *this; }


	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(rcref<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(rcptr<type2>&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(weak_rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }



	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const rcref<type2>& src) { m_ref.operator=(src); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile rcref<type2>& src) { m_ref.operator=(src); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile rcptr<type2>& src) { m_ref.operator=(src); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }
	/// @brief Thread-safe implementation of operator=()
	void operator=(type* src) volatile { m_ref.operator=(src); }
	/// @brief Thread-safe implementation of operator=()
	void operator=(const this_t& src) volatile { m_ref.operator=(src); }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const rcref<type2>& src) volatile { m_ref.operator=(src); }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const rcptr<type2>& src) volatile { m_ref.operator=(src.m_ref); }
	/// @brief Thread-safe implementation of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const weak_rcptr<type2>& src) volatile { m_ref.operator=(src); }
	/// @}


	void operator=(this_t&& src) volatile { m_ref.operator=(std::move(src)); }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(rcref<type2>&& src) volatile { m_ref.operator=(std::move(src)); }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src.m_ref)); }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(weak_rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src)); }



	/// @{
	/// @brief Sets to the specified object, without incrementing its reference count.
	/// @param obj Object to set value to
	void set(const ptr<type>& obj) { m_ref.set(obj); }

	/// @brief Thread-safe implementation of set()
	void set(const ptr<type>& obj) volatile { m_ref.set(obj); }

	/// @brief Sets to the specified object, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_ref.set(obj, desc); }

	/// @brief Thread-safe implementation of set()
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_ref.set(obj, desc); }
	/// @}

	/// @{
	/// @brief Releases the contained object, decrementing the reference count.
	/// @return True if the reference count reached zero and the object was destructed and/or disposed.
	bool release() { return m_ref.release(); }
	/// @brief Thread-safe implementation of release()
	bool release() volatile { return m_ref.release(); }
	/// @}

	/// @{
	/// @brief Releases the contained object without decrementing the reference count
	void disown() { m_ref.disown(); }
	/// @brief Thread-safe version of disown()
	void disown() volatile { m_ref.disown(); }
	/// @}

	/// @{
	/// @brief Get a rcref from a rcptr
	///
	/// It is caller error to invoke dereference() on an empty rcptr.  The result is undefined.
	/// @return A rcref from a rcptr
	non_nullable& dereference() & { return m_ref; }
	/// @brief Const implementation of dereference()
	const non_nullable& dereference() const & { return m_ref; }
	/// @brief rvalue qualified implementation of dereference(), which returns rcref as rvalue.
	non_nullable&& dereference() && { return std::move(m_ref); }
	/// @}

	type* get_obj() const { return m_ref.get_obj(); }
	type* get_obj() const volatile { return m_ref.get_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object
	/// @return A pointer to the encapsulated object
	type* get_ptr() const { return get_obj(); }
	/// @brief Thread-safe version of get_ptr()
	type* get_ptr() const volatile { return get_obj(); }
	/// @}

	type* peek_obj() const { return m_ref.peek_obj(); }
	type* peek_obj() const volatile { return m_ref.peek_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object.
	///
	/// This function is provided for parity with weak_rcptr.
	type* peek_ptr() const { return peek_obj(); }
	/// @brief Thread-safe version of peek_ptr()
	type* peek_ptr() const volatile { return peek_obj(); }
	/// @}

	/// @{
	/// @brief Gets the associated reference-counted descriptor, if any
	/// @return Descriptor associated with this reference-counted object, if any
	rc_obj_base* get_desc() const { return m_ref.get_desc(); }
	/// @brief Thread-safe version of get_desc()
	rc_obj_base* get_desc() const volatile { return m_ref.get_desc(); }
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value, dereferenced
	type* operator->() const
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return result;
	}

	/// @brief Thread-safe version of operator->()
	type* operator->() const volatile
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return result;
	}
	/// @}

	/// @{
	/// @brief Dereference operator
	/// @return The encapsulated value, dereferenced
	type& operator*() const
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return *result;
	}

	/// @brief Thread-safe version of operator*()
	type& operator*() const volatile
	{
		type* result = get_ptr();
		COGS_ASSERT(!!result);
		return *result;
	}
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL
	/// @return True if the pointer value is NULL.
	bool is_empty() const { return !get_ptr(); }
	/// @brief Thread-safe implementation of is_empty()
	bool is_empty() const volatile { return !get_ptr(); }
	/// @}

	/// @{
	/// @brief Tests if the pointer value is NULL.  An alias for is_empty()
	/// @return True if the pointer value is NULL.
	bool operator!() const { return is_empty(); }
	/// @brief Thread-safe implementation of operator!()
	bool operator!() const volatile { return is_empty(); }
	/// @}



	template <typename type2>
	const rcptr<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> static_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template static_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();;
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template dynamic_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> const_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template const_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(&member, get_desc());
		return storage;
	}


	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile { return *this; }



#ifdef DOXYGEN

	/// @{
	/// @brief static_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcptr<type2>& static_cast_to() const;

	/// @brief Thread-safe implementation of static_cast_to()
	template <typename type2>
	rcptr<type2> static_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief dynamic_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcptr<type2>& dynamic_cast_to() const;

	/// @brief Thread-safe implementation of dynamic_cast_to()
	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief const_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcptr<type2>& const_cast_to() const;

	/// @brief Thread-safe implementation of const_cast_to()
	template <typename type2>
	rcptr<type2> const_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief Gets a rcptr to a data member of the contained reference-counted object
	/// @tparam type2 Secondary data type
	/// @param member Reference to the member to create a rcptr to.  This can be anything known to
	/// be within the allocated block managed by the same descriptor.
	/// @return rcptr to the member.
	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member) const;
	/// @}

	/// @{
	/// @brief Gets a weak reference to this rcptr.
	///
	/// The non-volatile implementation of weak() avoids a reference count, otherwise this is equivalent to  constructing a weak_rcptr.
	/// @return A weak reference to this rcptr.
	const weak_rcptr<type>& weak() const;
	weak_rcptr<type> weak() const volatile;
	/// @}

#endif


	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
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
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
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
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
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
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }

	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
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
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
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
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @}


	/// @{
	/// @brief Swap the pointer value
	/// @param[in,out] wth Value to swap
	template <typename type2>
	void swap(rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(rcptr<type2>& wth) volatile { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(volatile rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @}

	template <typename type2> this_t exchange(const rcref<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const rcref<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> this_t exchange(const rcptr<type2>& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(const rcptr<type2>& src) volatile { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) volatile { return m_ref.exchange(src.m_ref); }

	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }

	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(std::move(src)), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
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
	static size_t mark_bits() { return ptr<type>::mark_bits(); }

	/// @brief Gets a mask with all available mark bits sets.
	/// @return A mask containing all available mark bits set.
	static size_t mark_mask() { return ptr<type>::mark_mask(); }

	/// @{
	/// @brief Gets marked bits on the pointer, if any
	/// @return The marked bits, if any
	size_t get_mark() const { return m_ref.get_mark(); }
	/// @brief Thread-safe version of get_ptr()
	size_t get_mark() const volatile { return m_ref.get_mark(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with marked bits removed.
	///
	/// If marked bits are set, direct use of the pointer value will include the marked bits.
	/// get_unmarked() must be used instead.
	/// @return The unmarked pointer value
	type* get_unmarked() const { return m_ref.get_unmarked(); }
	/// @brief Thread-safe version of get_ptr()
	type* get_unmarked() const volatile { return m_ref.get_unmarked(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with the specified bitmask applied
	/// @param mark Bitmask of bits to mark
	/// @return The pointer value with the mark applied.
	type* get_marked(size_t mark) const { return m_ref.get_marked(mark); }
	/// @brief Thread-safe version of get_ptr()
	type* get_marked(size_t mark) const volatile { return m_ref.get_marked(mark); }
	/// @}

	/// @{
	/// @brief Clears any marked bits.
	void clear_mark() { m_ref.clear_mark(); }
	/// @brief Thread-safe version of get_ptr()
	void clear_mark() volatile { m_ref.clear_mark(); }
	/// @}

	/// @{
	/// @brief Sets marked bits.
	/// @param mark Bitmask of bits to set
	void set_mark(size_t mark) { m_ref.set_mark(mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_mark(size_t mark) volatile { m_ref.set_mark(mark); }
	/// @}

	/// @{
	/// @brief Sets the value to only the specified marked bit, clearing the original pointer value.
	/// @param mark Bitmask of bits to set.
	void set_to_mark(size_t mark) { m_ref.set_to_mark(mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_to_mark(size_t mark) volatile { m_ref.set_to_mark(mark); }
	/// @}

	/// @{
	/// @brief Set to the pointer value specified, and applied the speficied marked bits.
	/// @param p Value to set.
	/// @param mark Bitmask of bits to set.
	void set_marked(type* p, size_t mark) { m_ref.set_marked(p, mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_marked(type* p, size_t mark) volatile { m_ref.set_marked(p, mark); }
	/// @}
};


template <>
class rcptr<void>
{
private:
	rcref<void> m_ref;

public:
	typedef void type;
	typedef rcptr<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename>
	friend class rcref;

	template <typename>
	friend class rcptr;

	template <typename>
	friend class weak_rcptr;

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcptr() { }
	rcptr(this_t&& src) : m_ref(std::move(src.m_ref)) { }
	rcptr(const this_t& src) : m_ref(src.m_ref) { }
	rcptr(const volatile this_t& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const weak_rcptr<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const volatile weak_rcptr<type2>& src) : m_ref(src) { }
	rcptr(type* obj) : m_ref(obj) { }
	rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc): m_ref(obj, desc) { }

	template <typename type2> rcptr(rcref<type2>&& src) : m_ref(std::move(src)) { }
	template <typename type2> rcptr(rcptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> rcptr(weak_rcptr<type2>&& src) : m_ref(std::move(src)) { }


	this_t& operator=(void* src) { m_ref.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }
	this_t& operator=(const this_t& src) { m_ref.operator=(src.m_ref); return *this; }
	this_t& operator=(const volatile this_t& src) { m_ref.operator=(src.m_ref); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }


	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }
	template <typename type2> this_t& operator=(rcptr<type2>&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }
	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }



	void operator=(type* src) volatile { m_ref.operator=(src); }

	void operator=(this_t&& src) volatile { m_ref.operator=(std::move(src)); }
	void operator=(const this_t& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_ref.operator=(std::move(src)); }
	template <typename type2> void operator=(rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src.m_ref)); }
	template <typename type2> void operator=(weak_rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src)); }


	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_ref.operator=(src); }
	template <typename type2> void operator=(const rcptr<type2>& src) volatile { m_ref.operator=(src.m_ref); }
	template <typename type2> void operator=(const weak_rcptr<type2>& src) volatile { m_ref.operator=(src); }

	void set(const ptr<type>& obj) { m_ref.set(obj); }
	void set(const ptr<type>& obj) volatile { m_ref.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_ref.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_ref.set(obj, desc); }

	bool release() { return m_ref.release(); }
	bool release() volatile { return m_ref.release(); }

	void disown() { m_ref.disown(); }
	void disown() volatile { m_ref.disown(); }

	non_nullable& dereference() & { return m_ref; }
	const non_nullable& dereference() const & { return m_ref; }
	non_nullable&& dereference() && { return std::move(m_ref); }

	type* get_obj() const { return m_ref.get_obj(); }
	type* get_obj() const volatile { return m_ref.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_ref.peek_obj(); }
	type* peek_obj() const volatile { return m_ref.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	rc_obj_base* get_desc() const { return m_ref.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_ref.get_desc(); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }



	template <typename type2>
	const rcptr<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> static_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template static_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();;
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template dynamic_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> const_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template const_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(&member, get_desc());
		return storage;
	}


	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile { return *this; }




	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
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
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
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
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
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
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }

	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
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
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
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
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @}

		/// @{
	/// @brief Swap the pointer value
	/// @param[in,out] wth Value to swap
	template <typename type2>
	void swap(rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(rcptr<type2>& wth) volatile { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(volatile rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @}


	template <typename type2> this_t exchange(const rcref<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const rcref<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> this_t exchange(const rcptr<type2>& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(const rcptr<type2>& src) volatile { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) volatile { return m_ref.exchange(src.m_ref); }

	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }

	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }


	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(std::move(src)), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	/// @}
};

template <>
class rcptr<const void>
{
private:
	rcref<const void> m_ref;

public:
	typedef const void type;
	typedef rcptr<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename>
	friend class rcref;

	template <typename>
	friend class rcptr;

	template <typename>
	friend class weak_rcptr;

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcptr() { }
	rcptr(this_t&& src) : m_ref(std::move(src.m_ref)) { }
	rcptr(const this_t& src) : m_ref(src.m_ref) { }
	rcptr(const volatile this_t& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const weak_rcptr<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const volatile weak_rcptr<type2>& src) : m_ref(src) { }
	rcptr(type* obj) : m_ref(obj) { }
	rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_ref(obj, desc) { }

	template <typename type2> rcptr(rcref<type2>&& src) : m_ref(std::move(src)) { }
	template <typename type2> rcptr(rcptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> rcptr(weak_rcptr<type2>&& src) : m_ref(std::move(src)) { }


	this_t& operator=(void* src) { m_ref.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_ref.operator=(std::move(src)); return *this; }
	this_t& operator=(const this_t& src) { m_ref.operator=(src); return *this; }
	this_t& operator=(const volatile this_t& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }
	template <typename type2> this_t& operator=(rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }
	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }


	void operator=(type* src) volatile { m_ref.operator=(src); }
	void operator=(this_t&& src) volatile { m_ref.operator=(std::move(src)); }
	void operator=(const this_t& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_ref.operator=(src); }
	template <typename type2> void operator=(const rcptr<type2>& src) volatile { m_ref.operator=(src.m_ref); }
	template <typename type2> void operator=(const weak_rcptr<type2>& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_ref.operator=(std::move(src)); }
	template <typename type2> void operator=(rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src.m_ref)); }
	template <typename type2> void operator=(weak_rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src)); }

	void set(const ptr<type>& obj) { m_ref.set(obj); }
	void set(const ptr<type>& obj) volatile { m_ref.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_ref.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_ref.set(obj, desc); }

	bool release() { return m_ref.release(); }
	bool release() volatile { return m_ref.release(); }

	void disown() { m_ref.disown(); }
	void disown() volatile { m_ref.disown(); }

	non_nullable& dereference() & { return m_ref; }
	const non_nullable& dereference() const & { return m_ref; }
	non_nullable&& dereference() && { return std::move(m_ref); }

	type* get_obj() const { return m_ref.get_obj(); }
	type* get_obj() const volatile { return m_ref.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_ref.peek_obj(); }
	type* peek_obj() const volatile { return m_ref.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	rc_obj_base* get_desc() const { return m_ref.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_ref.get_desc(); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }



	template <typename type2>
	const rcptr<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> static_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template static_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();;
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template dynamic_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> const_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template const_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(&member, get_desc());
		return storage;
	}


	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile { return *this; }




	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
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
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
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
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
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
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }

	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
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
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
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
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @}


	/// @{
	/// @brief Swap the pointer value
	/// @param[in,out] wth Value to swap
	template <typename type2>
	void swap(rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(rcptr<type2>& wth) volatile { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(volatile rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @}


	template <typename type2> this_t exchange(const rcref<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const rcref<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> this_t exchange(const rcptr<type2>& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(const rcptr<type2>& src) volatile { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) volatile { return m_ref.exchange(src.m_ref); }

	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }

	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }


	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(std::move(src)), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	/// @}
};

template <>
class rcptr<volatile void>
{
private:
	rcref<volatile void> m_ref;

public:
	typedef volatile void type;
	typedef rcptr<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename>
	friend class rcref;

	template <typename>
	friend class rcptr;

	template <typename>
	friend class weak_rcptr;

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcptr() { }
	rcptr(this_t&& src) : m_ref(std::move(src.m_ref)) { }
	rcptr(const this_t& src) : m_ref(src.m_ref) { }
	rcptr(const volatile this_t& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const weak_rcptr<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const volatile weak_rcptr<type2>& src) : m_ref(src) { }
	rcptr(type* obj) : m_ref(obj) { }
	rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_ref(obj, desc) { }

	template <typename type2> rcptr(rcref<type2>&& src) : m_ref(std::move(src)) { }
	template <typename type2> rcptr(rcptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> rcptr(weak_rcptr<type2>&& src) : m_ref(std::move(src)) { }

	this_t& operator=(void* src) { m_ref.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_ref.operator=(std::move(src)); return *this; }
	this_t& operator=(const this_t& src) { m_ref.operator=(src); return *this; }
	this_t& operator=(const volatile this_t& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }
	template <typename type2> this_t& operator=(rcptr<type2>&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }
	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }


	void operator=(type* src) volatile { m_ref.operator=(src); }
	void operator=(this_t&& src) volatile { m_ref.operator=(std::move(src)); }
	void operator=(const this_t& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_ref.operator=(src); }
	template <typename type2> void operator=(const rcptr<type2>& src) volatile { m_ref.operator=(src.m_ref); }
	template <typename type2> void operator=(const weak_rcptr<type2>& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_ref.operator=(std::move(src)); }
	template <typename type2> void operator=(rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src.m_ref)); }
	template <typename type2> void operator=(weak_rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src)); }

	void set(const ptr<type>& obj) { m_ref.set(obj); }
	void set(const ptr<type>& obj) volatile { m_ref.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_ref.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_ref.set(obj, desc); }

	bool release() { return m_ref.release(); }
	bool release() volatile { return m_ref.release(); }

	void disown() { m_ref.disown(); }
	void disown() volatile { m_ref.disown(); }

	non_nullable& dereference() & { return m_ref; }
	const non_nullable& dereference() const & { return m_ref; }
	non_nullable&& dereference() && { return std::move(m_ref); }

	type* get_obj() const { return m_ref.get_obj(); }
	type* get_obj() const volatile { return m_ref.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_ref.peek_obj(); }
	type* peek_obj() const volatile { return m_ref.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	rc_obj_base* get_desc() const { return m_ref.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_ref.get_desc(); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	const rcptr<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> static_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template static_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();;
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template dynamic_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> const_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template const_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(&member, get_desc());
		return storage;
	}


	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile { return *this; }



	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
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
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
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
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
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
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }

	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
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
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
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
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @}



	/// @{
	/// @brief Swap the pointer value
	/// @param[in,out] wth Value to swap
	template <typename type2>
	void swap(rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(rcptr<type2>& wth) volatile { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(volatile rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @}


	template <typename type2> this_t exchange(const rcref<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const rcref<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> this_t exchange(const rcptr<type2>& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(const rcptr<type2>& src) volatile { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) volatile { return m_ref.exchange(src.m_ref); }

	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }

	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }


	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(std::move(src)), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	/// @}
};

template <>
class rcptr<const volatile void>
{
private:
	rcref<const volatile void> m_ref;

public:
	typedef const volatile void type;
	typedef rcptr<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename>
	friend class rcref;

	template <typename>
	friend class rcptr;

	template <typename>
	friend class weak_rcptr;

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcptr() { }
	rcptr(this_t&& src) : m_ref(std::move(src.m_ref)) { }
	rcptr(const this_t& src) : m_ref(src.m_ref) { }
	rcptr(const volatile this_t& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const weak_rcptr<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcref<type2>& src) : m_ref(src) { }
	template <typename type2> rcptr(const volatile rcptr<type2>& src) : m_ref(src.m_ref) { }
	template <typename type2> rcptr(const volatile weak_rcptr<type2>& src) : m_ref(src) { }
	rcptr(type* obj) : m_ref(obj) { }
	rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_ref(obj, desc) { }

	template <typename type2> rcptr(rcref<type2>&& src) : m_ref(std::move(src)) { }
	template <typename type2> rcptr(rcptr<type2>&& src) : m_ref(std::move(src.m_ref)) { }
	template <typename type2> rcptr(weak_rcptr<type2>&& src) : m_ref(std::move(src)) { }


	this_t& operator=(void* src) { m_ref.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_ref.operator=(std::move(src)); return *this; }
	this_t& operator=(const this_t& src) { m_ref.operator=(src); return *this; }
	this_t& operator=(const volatile this_t& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_ref.operator=(src.m_ref); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_ref.operator=(src); return *this; }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }
	template <typename type2> this_t& operator=(rcptr<type2>&& src) { m_ref.operator=(std::move(src.m_ref)); return *this; }
	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_ref.operator=(std::move(src)); return *this; }

	void operator=(type* src) volatile { m_ref.operator=(src); }
	void operator=(this_t&& src) volatile { m_ref.operator=(std::move(src)); }
	void operator=(const this_t& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_ref.operator=(src); }
	template <typename type2> void operator=(const rcptr<type2>& src) volatile { m_ref.operator=(src.m_ref); }
	template <typename type2> void operator=(const weak_rcptr<type2>& src) volatile { m_ref.operator=(src); }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_ref.operator=(std::move(src)); }
	template <typename type2> void operator=(rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src.m_ref)); }
	template <typename type2> void operator=(weak_rcptr<type2>&& src) volatile { m_ref.operator=(std::move(src)); }

	void set(const ptr<type>& obj) { m_ref.set(obj); }
	void set(const ptr<type>& obj) volatile { m_ref.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_ref.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_ref.set(obj, desc); }

	bool release() { return m_ref.release(); }
	bool release() volatile { return m_ref.release(); }

	void disown() { m_ref.disown(); }
	void disown() volatile { m_ref.disown(); }

	non_nullable& dereference() & { return m_ref; }
	const non_nullable& dereference() const & { return m_ref; }
	non_nullable&& dereference() && { return std::move(m_ref); }

	type* get_obj() const { return m_ref.get_obj(); }
	type* get_obj() const volatile { return m_ref.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_ref.peek_obj(); }
	type* peek_obj() const volatile { return m_ref.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	rc_obj_base* get_desc() const { return m_ref.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_ref.get_desc(); }

	bool is_empty() const { return !get_ptr(); }
	bool is_empty() const volatile { return !get_ptr(); }

	bool operator!() const { return !get_ptr(); }
	bool operator!() const volatile { return !get_ptr(); }


	template <typename type2>
	const rcptr<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> static_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template static_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();;
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	rcptr<type2> dynamic_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template dynamic_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	rcptr<type2> const_cast_to() const volatile
	{
		this_t tmp(*this);
		return tmp.template const_cast_to<type2>();
	}

	template <typename type2>
	const rcptr<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const
	{
		storage.set(&member, get_desc());
		return storage;
	}


	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile { return *this; }




	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(type2* cmp) const volatile { return get_ptr() == cmp; }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_ref.operator==(cmp); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_ref.operator==(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator==()
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator==(cmp); }
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
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(type2* cmp) const volatile { return get_ptr() != cmp; }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_ref.operator!=(cmp); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_ref.operator!=(cmp.m_ref); }
	/// @brief Thread-safe implementation of operator!=()
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_ref.operator!=(cmp); }
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
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(type2* cmp) const volatile { return get_ptr() > cmp; }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>()
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
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
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }

	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(type2* cmp) const volatile { return get_ptr() < cmp; }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<()
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
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
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(type2* cmp) const volatile { return get_ptr() >= cmp; }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator>=()
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
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
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(type2* cmp) const volatile { return get_ptr() <= cmp; }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @brief Thread-safe implementation of operator<=()
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	/// @}



	/// @{
	/// @brief Swap the pointer value
	/// @param[in,out] wth Value to swap
	template <typename type2>
	void swap(rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(rcptr<type2>& wth) volatile { m_ref.swap(wth.m_ref); }
	/// @brief Thread-safe implementation of swap()
	template <typename type2>
	void swap(volatile rcptr<type2>& wth) { m_ref.swap(wth.m_ref); }
	/// @}


	template <typename type2> this_t exchange(const rcref<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const rcref<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(rcref<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> this_t exchange(const rcptr<type2>& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(const rcptr<type2>& src) volatile { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) { return m_ref.exchange(src.m_ref); }
	template <typename type2> this_t exchange(rcptr<type2>&& src) volatile { return m_ref.exchange(src.m_ref); }

	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(const weak_rcptr<type2>& src) volatile { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) { return m_ref.exchange(src); }
	template <typename type2> this_t exchange(weak_rcptr<type2>&& src) volatile { return m_ref.exchange(src); }

	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const rcref<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(rcref<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }

	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(const rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src.m_ref, rtn.m_ref); }
	template <typename type2> void exchange(rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src.m_ref, rtn.m_ref); }

	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(const weak_rcptr<type2>& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) { m_ref.exchange(src, rtn.m_ref); }
	template <typename type2> void exchange(weak_rcptr<type2>&& src, this_t& rtn) volatile { m_ref.exchange(src, rtn.m_ref); }


	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(std::move(src)), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src.m_ref, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(src, cmp, rtn); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) { return m_ref.compare_exchange(std::move(src), cmp, rtn); }

	/// @brief Thread-safe implementation of compare_exchange()
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src.m_ref, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(src, cmp); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp) volatile { return m_ref.compare_exchange(std::move(src), cmp); }


	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src.m_ref, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(const weak_rcptr<type2>& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(src, cmp, rtn.m_ref); }

	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcref<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src.m_ref), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, type3* cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcref<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	template <typename type2, typename type3> bool compare_exchange(weak_rcptr<type2>&& src, const weak_rcptr<type3>& cmp, this_t& rtn) volatile { return m_ref.compare_exchange(std::move(src), cmp, rtn.m_ref); }
	/// @}
};




template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, type3* cmp, rcptr<type>& rtn) { return base_t::compare_exchange(src, cmp, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, rcptr<type>& rtn) { return base_t::compare_exchange(src, cmp, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, rcptr<type>& rtn) { return base_t::compare_exchange(src, cmp.m_ref, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, rcptr<type>& rtn) { return base_t::compare_exchange(src, cmp, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, type3* cmp, rcptr<type>& rtn) volatile { return base_t::compare_exchange(src, cmp, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const rcref<type3>& cmp, rcptr<type>& rtn) volatile { return base_t::compare_exchange(src, cmp, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const rcptr<type3>& cmp, rcptr<type>& rtn) volatile { return base_t::compare_exchange(src, cmp.m_ref, rtn.m_ref); }

template <typename type>
template <typename type2, typename type3>
inline bool rcref<type>::compare_exchange(const rcref<type2>& src, const weak_rcptr<type3>& cmp, rcptr<type>& rtn) volatile { return base_t::compare_exchange(src, cmp, rtn.m_ref); }



}


#endif
