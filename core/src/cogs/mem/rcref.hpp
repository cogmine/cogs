//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RCREF
#define COGS_HEADER_MEM_RCREF


#include "cogs/env.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/is_pointer_type.hpp"
#include "cogs/mem/is_reference_type.hpp"
#include "cogs/mem/is_rc_type.hpp"
#include "cogs/mem/rc_container.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/mem/weak_rcptr.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {

/// @defgroup ReferenceCountedReferenceContainerTypes Reference-Counted Reference Container Types
/// @{
/// @ingroup ReferenceContainerTypes
/// @brief Reference-counted, reference container types.
/// @}

/// @ingroup ReferenceCountedReferenceContainerTypes
/// @brief A non-nullable reference-counted reference container.
/// @tparam T Data type pointed to
template <typename T>
class rcref
{
public:
	/// @brief Alias to the type pointed to
	typedef T type;

	/// @brief Alias to this type.
	typedef rcref<type> this_t;

	/// @brief Alias to this type.  If this reference container required locking (such as a 'relocatable handle'), locked_t would be the appropriate type to contain the lock.
	typedef ref<type> locked_t;

	/// @brief Alias to the nullable equivalent of this reference type
	typedef rcptr<type> nullable;

	/// @brief Alias to the non_nullable equivalent of this reference type
	typedef rcref<type> non_nullable;

private:
	typedef rc_container<type, strong> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	rcref() { }

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	rcref(const ptr<type>& obj) : m_container(obj) { }
	this_t& operator=(const ptr<type>& src) { m_container.operator=(src); return *this; }
	void operator=(const ptr<type>& src) volatile { m_container.operator=(src); }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(rcptr<type2>&& src) : m_container(std::move(src.m_ref.m_container)) { }


	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

public:
	/// @brief Provides a rcref with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A rcref with a different referenced type.
		typedef rcref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcref(this_t&& src) : m_container(std::move(src.m_container)) { }


	/// @{
	/// @brief Initializes rcref to the specified value.
	/// @param src Initial value
	rcref(const this_t& src) : m_container(src.m_container) { }
	rcref(const volatile this_t& src) : m_container(src.m_container) { }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(rcref<type2>&& src) : m_container(std::move(src.m_container)) { }

	/// @brief Initializes rcref to the specified value.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const rcref<type2>& src) : m_container(src.m_container) { }

	/// @brief Initializes rcref to the specified value.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	rcref(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	/// @brief Initializes rcref to the specified value.
	///
	/// A rcref may contain a non reference-counted pointer.  By specifying a reference by address where a rcref
	/// is expected, the caller is gauranteeing the resource pointed to will remain in scope until no longer referenced
	/// by the rcref.
	/// @param src Initial value
	rcref(type& src) : m_container(&src) { }

	/// @brief Initializes a rcref to contain the specified object and descriptor, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	rcref(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }
	/// @}

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(rcref<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(rcref<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); }


	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	/// @brief Thread-safe version of operator=()
	void operator=(const this_t& src) volatile { m_container.operator=(src.m_container); }
	/// @brief Thread-safe version of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	void operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); }
	/// @}

	/// @{
	/// @brief Sets to the specified object, without incrementing its reference count.
	/// @param obj Object to set value to
	void set(const ref<type>& obj) { m_container.set(obj); }
	/// @brief Thread-safe implementation of set()
	void set(const ref<type>& obj) volatile { m_container.set(obj); }
	/// @brief Sets to the specified object, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	/// @brief Thread-safe implementation of set()
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }
	/// @}

	/// @{
	/// @brief Releases the contained object without decrementing the reference count
	rc_obj_base* disown() { return m_container.disown(); }
	/// @brief Thread-safe version of disown()
	rc_obj_base* disown() volatile { return m_container.disown(); }
	/// @}

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object
	/// @return A pointer to the encapsulated object
	type* get_ptr() const { return get_obj(); }
	/// @brief Thread-safe version of get_ptr()
	type* get_ptr() const volatile { return get_obj(); }
	/// @}

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object.
	///
	/// This function is provided for parity with weak_rcptr.
	type* peek_ptr() const { return peek_obj(); }
	/// @brief Thread-safe version of peek_ptr()
	type* peek_ptr() const volatile { return peek_obj(); }
	/// @}

	/// @{
	this_t& dereference()& { return *this; }
	/// @brief Thread-safe version of dereference()
	const this_t& dereference() const& { return *this; }
	/// @brief rvalue qualified implementation of dereference()
	this_t&& dereference()&& { return std::move(*this); }
	/// @}

	/// @{
	/// @brief Gets the associated reference-counted descriptor, if any
	/// @return Descriptor associated with this reference-counted object, if any
	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	/// @brief Thread-safe version of get_desc()
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }
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



	template <typename type2>
	const rcref<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> static_cast_to()&&
	{
		rcref<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();
		storage.set(tmp, desc);
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2>&& dynamic_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned())&&
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : disown();
		storage.set(tmp, desc);
		return std::move(storage).dereference();
	}

	template <typename type2>
	rcref<type2> dynamic_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template dynamic_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> const_cast_to()&&
	{
		rcref<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& reinterpret_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to()&&
	{
		rcref<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(&member, get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> member_cast_to(type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned())&&
	{
		storage.set(&member, disown());
		return std::move(storage).dereference();
	}

	template <typename type2>
	const rcref<type2>& member_cast_to(volatile type2& member, unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const volatile&
	{
		storage.set(&member, get_desc());
		return storage.dereference();
	}

	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const&
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile
	{
		weak_rcptr<type> result(*this);
		return result;
	}



#ifdef DOXYGEN
	/// @{
	/// @brief static_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcref<type2>& static_cast_to() const;

	/// @brief Thread-safe implementation of static_cast_to()
	template <typename type2>
	const rcref<type2>& static_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief dynamic_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcref<type2>& dynamic_cast_to() const;

	/// @brief Thread-safe implementation of dynamic_cast_to()
	template <typename type2>
	const rcref<type2>& dynamic_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief const_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const rcref<type2>& const_cast_to() const;

	/// @brief Thread-safe implementation of const_cast_to()
	template <typename type2>
	const rcref<type2>& const_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief Gets a rcref to a data member of the contained reference-counted object
	/// @tparam type2 Secondary data type
	/// @param member Reference to the member to create a rcref to.  This can be anything known to
	/// be within the allocated block managed by the same descriptor.
	/// @return rcref to the member.
	template <typename type2>
	const rcref<type2>& member_cast_to(type2& member) const;
	/// @}

	/// @{
	/// @brief Gets a weak reference to this rcptr.
	///
	/// The non-volatile implementation of weak() avoids a reference count, otherwise this is equivalent to  constructing a weak_rcptr.
	/// @return A weak reference to this rcptr.
	const weak_rcptr<type>& weak() const;
	weak_rcptr<type> weak() const volatile;
	/// @

#endif



	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	bool operator==(type& cmp) const { return get_ptr() == &cmp; }
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }

	bool operator==(type& cmp) const volatile { return get_ptr() == &cmp; }
	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	bool operator!=(type& cmp) const { return get_ptr() != &cmp; }
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }

	bool operator!=(type& cmp) const volatile { return get_ptr() != &cmp; }
	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	bool operator>(type& cmp) const { return get_ptr() > &cmp; }
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }

	bool operator>(type& cmp) const volatile { return get_ptr() > &cmp; }
	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	bool operator<(type& cmp) const { return get_ptr() < &cmp; }
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }

	bool operator<(type& cmp) const volatile { return get_ptr() < &cmp; }
	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	bool operator>=(type& cmp) const { return get_ptr() >= &cmp; }
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }

	bool operator>=(type& cmp) const volatile { return get_ptr() >= &cmp; }
	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	bool operator<=(type& cmp) const { return get_ptr() <= &cmp; }
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }

	bool operator<=(type& cmp) const volatile { return get_ptr() <= &cmp; }
	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	/// @}

	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}



	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}



	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

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
	size_t get_mark() const { return m_container.get_mark(); }
	/// @brief Thread-safe version of get_mark()
	size_t get_mark() const volatile { return m_container.get_mark(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with marked bits removed.
	///
	/// If marked bits are set, direct use of the pointer value will include the marked bits.
	/// get_unmarked() must be used instead.
	/// @return The unmarked pointer value
	type* get_unmarked() const { return m_container.get_unmarked(); }
	/// @brief Thread-safe version of get_unmarked()
	type* get_unmarked() const volatile { return m_container.get_unmarked(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with the specified bitmask applied
	/// @param mark Bitmask of bits to mark
	/// @return The pointer value with the mark applied.
	type* get_marked(size_t mark) const { return m_container.get_marked(mark); }
	/// @brief Thread-safe version of get_marked()
	type* get_marked(size_t mark) const volatile { return m_container.get_marked(mark); }
	/// @}

	/// @{
	/// @brief Clears any marked bits.
	void clear_mark() { m_container.clear_mark(); }
	/// @brief Thread-safe version of clear_mark()
	void clear_mark() volatile { m_container.clear_mark(); }
	/// @}

	/// @{
	/// @brief Sets marked bits.
	/// @param mark Bitmask of bits to set
	void set_mark(size_t mark) { m_container.set_mark(mark); }
	/// @brief Thread-safe version of set_mark()
	void set_mark(size_t mark) volatile { m_container.set_mark(mark); }
	/// @}

	/// @{
	/// @brief Set to the pointer value specified, and applied the speficied marked bits.
	/// @param p Value to set.
	/// @param mark Bitmask of bits to set.
	void set_marked(type& p, size_t mark) { m_container.set_marked(&p, mark); }
	/// @brief Thread-safe version of set_marked()
	void set_marked(type& p, size_t mark) volatile { m_container.set_marked(&p, mark); }
	/// @}

	template <typename F>
	void on_released(F&& f) const
	{
		m_container.on_released(std::forward<F>(f));
	}
};

template <>
class rcref<void>
{
public:
	typedef void type;

	typedef rcref<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	typedef rc_container<type, strong> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	rcref() { }

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	rcref(const ptr<type>& obj) : m_container(obj) { }
	this_t& operator=(const ptr<type>& src) { m_container.operator=(src); return *this; }
	void operator=(const ptr<type>& src) volatile { m_container.operator=(src); }

	template <typename type2> rcref(rcptr<type2>&& src) : m_container(std::move(src.m_ref.m_container)) { }

	template <typename type2> rcref(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

	//template <typename type2, class allocator_type>
	//rcref(const rcnew_nullable_return_glue_t<type2, allocator_type>& glue) : m_container(glue) { }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcref(this_t&& src) : m_container(std::move(src.m_container)) { }
	rcref(const this_t& src) : m_container(src.m_container) { }
	rcref(const volatile this_t& src) : m_container(src.m_container) { }

	template <typename type2> rcref(rcref<type2>&& src) : m_container(std::move(src.m_container)) { }
	template <typename type2> rcref(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	rcref(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	void operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	void operator=(const this_t& src) volatile { m_container.operator=(src.m_container); }


	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); }


	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); }

	void set(const ref<type>& obj) { m_container.set(obj); }
	void set(const ref<type>& obj) volatile { m_container.set(obj); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	rc_obj_base* disown() { return m_container.disown(); }
	rc_obj_base* disown() volatile { return m_container.disown(); }

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	this_t& dereference()& { return *this; }
	const this_t& dereference() const& { return *this; }
	this_t&& dereference()&& { return std::move(*this); }

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return false; }
	bool operator!() const volatile { return false; }


	template <typename type2>
	const rcref<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> static_cast_to()&&
	{
		rcref<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> const_cast_to()&&
	{
		rcref<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& reinterpret_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to()&&
	{
		rcref<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const&
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile
	{
		weak_rcptr<type> result(*this);
		return result;
	}




	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	/// @}


	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}



	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}



	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}
	/// @}

	template <typename F>
	void on_released(F&& f) const
	{
		m_container.on_released(std::forward<F>(f));
	}
};

template <>
class rcref<const void>
{
public:
	typedef const void type;

	typedef rcref<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	typedef rc_container<type, strong> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	rcref() { }

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	rcref(const ptr<type>& obj) : m_container(obj) { }
	this_t& operator=(const ptr<type>& src) { m_container.operator=(src); return *this; }
	void operator=(const ptr<type>& src) volatile { m_container.operator=(src); }

	template <typename type2> rcref(rcptr<type2>&& src) : m_container(std::move(src.m_ref.m_container)) { }

	template <typename type2> rcref(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcref(this_t&& src) : m_container(std::move(src.m_container)) { }
	rcref(const this_t& src) : m_container(src.m_container) { }
	rcref(const volatile this_t& src) : m_container(src.m_container) { }

	template <typename type2> rcref(rcref<type2>&& src) : m_container(std::move(src.m_container)) { }
	template <typename type2> rcref(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	rcref(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	void operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	void operator=(const this_t& src) volatile { m_container.operator=(src.m_container); }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); }

	void set(const ref<type>& obj) { m_container.set(obj); }
	void set(const ref<type>& obj) volatile { m_container.set(obj); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	rc_obj_base* disown() { return m_container.disown(); }
	rc_obj_base* disown() volatile { return m_container.disown(); }

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	this_t& dereference()& { return *this; }
	const this_t& dereference() const& { return *this; }
	this_t&& dereference()&& { return std::move(*this); }

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return false; }
	bool operator!() const volatile { return false; }


	template <typename type2>
	const rcref<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> static_cast_to()&&
	{
		rcref<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> const_cast_to()&&
	{
		rcref<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& reinterpret_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to()&&
	{
		rcref<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const&
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile
	{
		weak_rcptr<type> result(*this);
		return result;
	}



	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	/// @}


	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}



	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}



	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}
	/// @}

	template <typename F>
	void on_released(F&& f) const
	{
		m_container.on_released(std::forward<F>(f));
	}
};

template <>
class rcref<volatile void>
{
public:
	typedef volatile void type;

	typedef rcref<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	typedef rc_container<type, strong> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	rcref() { }

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	rcref(const ptr<type>& obj) : m_container(obj) { }
	this_t& operator=(const ptr<type>& src) { m_container.operator=(src); return *this; }
	void operator=(const ptr<type>& src) volatile { m_container.operator=(src); }

	template <typename type2> rcref(rcptr<type2>&& src) : m_container(std::move(src.m_ref.m_container)) { }

	template <typename type2> rcref(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcref(this_t&& src) : m_container(std::move(src.m_container)) { }
	rcref(const this_t& src) : m_container(src.m_container) { }
	rcref(const volatile this_t& src) : m_container(src.m_container) { }

	template <typename type2> rcref(rcref<type2>&& src) : m_container(std::move(src.m_container)) { }
	template <typename type2> rcref(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	rcref(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	void operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	void operator=(const this_t& src) volatile { m_container.operator=(src.m_container); }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); }

	void set(const ref<type>& obj) { m_container.set(obj); }
	void set(const ref<type>& obj) volatile { m_container.set(obj); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	rc_obj_base* disown() { return m_container.disown(); }
	rc_obj_base* disown() volatile { return m_container.disown(); }

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	this_t& dereference()& { return *this; }
	const this_t& dereference() const& { return *this; }
	this_t&& dereference()&& { return std::move(*this); }

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return false; }
	bool operator!() const volatile { return false; }



	template <typename type2>
	const rcref<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> static_cast_to()&&
	{
		rcref<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> const_cast_to()&&
	{
		rcref<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& reinterpret_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to()&&
	{
		rcref<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const&
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile
	{
		weak_rcptr<type> result(*this);
		return result;
	}





	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	/// @}




	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}



	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}



	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	/// @}

	template <typename F>
	void on_released(F&& f) const
	{
		m_container.on_released(std::forward<F>(f));
	}
};

template <>
class rcref<const volatile void>
{
public:
	typedef const volatile void type;

	typedef rcref<type> this_t;
	typedef rcptr<type> nullable;
	typedef rcref<type> non_nullable;

private:
	typedef rc_container<type, strong> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	rcref() { }

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	rcref(const ptr<type>& obj) : m_container(obj) { }
	this_t& operator=(const ptr<type>& src) { m_container.operator=(src); return *this; }
	void operator=(const ptr<type>& src) volatile { m_container.operator=(src); }

	template <typename type2> rcref(rcptr<type2>&& src) : m_container(std::move(src.m_ref.m_container)) { }

	template <typename type2> rcref(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> rcref(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

public:
	template <typename type2>
	class cast
	{
	public:
		typedef rcref<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	rcref(this_t&& src) : m_container(std::move(src.m_container)) { }
	rcref(const this_t& src) : m_container(src.m_container) { }
	rcref(const volatile this_t& src) : m_container(src.m_container) { }

	template <typename type2> rcref(rcref<type2>&& src) : m_container(std::move(src.m_container)) { }
	template <typename type2> rcref(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> rcref(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	rcref(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	void operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	void operator=(const this_t& src) volatile { m_container.operator=(src.m_container); }

	template <typename type2> this_t& operator=(rcref<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> void operator=(rcref<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); }
	template <typename type2> void operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); }

	void set(const ref<type>& obj) { m_container.set(obj); }
	void set(const ref<type>& obj) volatile { m_container.set(obj); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ref<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	rc_obj_base* disown() { return m_container.disown(); }
	rc_obj_base* disown() volatile { return m_container.disown(); }

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	type* get_ptr() const { return get_obj(); }
	type* get_ptr() const volatile { return get_obj(); }

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	type* peek_ptr() const { return peek_obj(); }
	type* peek_ptr() const volatile { return peek_obj(); }

	this_t& dereference()& { return *this; }
	const this_t& dereference() const& { return *this; }
	this_t&& dereference()&& { return std::move(*this); }

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return false; }
	bool is_empty() const volatile { return false; }

	bool operator!() const { return false; }
	bool operator!() const volatile { return false; }


	template <typename type2>
	const rcref<type2>& static_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> static_cast_to()&&
	{
		rcref<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& const_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> const_cast_to()&&
	{
		rcref<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const rcref<type2>& reinterpret_cast_to(unowned_t<rcptr<type2> >& storage = unowned_t<rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage.dereference();
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to()&&
	{
		rcref<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	rcref<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		rcref<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	const weak_rcptr<type>& weak(unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const&
	{
		storage.set(
			peek_obj(),
			get_desc());
		return storage;
	}

	weak_rcptr<type> weak() const volatile
	{
		weak_rcptr<type> result(*this);
		return result;
	}




	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator==(cmp.m_container); }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_ref.m_container); }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator==(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator!=(cmp.m_container); }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_ref.m_container); }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator!=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>(cmp.m_container); }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_ref.m_container); }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<(cmp.m_container); }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_ref.m_container); }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator>=(cmp.m_container); }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_ref.m_container); }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator>=(cmp.m_container); }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container.operator<=(cmp.m_container); }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_ref.m_container); }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container.operator<=(cmp.m_container); }
	/// @}



	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && !std::is_const_v<T3>> >
	void exchange(T2&& src, T3& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				m_container.exchange(std::forward<T2>(src).get_container(), rtn.get_container());
			else
				m_container.exchange(std::forward<T2>(src).get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T3>)
			m_container.exchange(std::forward<T2>(src), rtn.get_container());
		else
			m_container.exchange(std::forward<T2>(src), rtn);
	}



	/// @{
	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @return True if the comparison was equal and the value set.

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}

	template <typename T2, typename T3, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
	bool compare_exchange(T2&& src, const T3& cmp) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp);
		}
		else if constexpr (is_rc_type_v<T3>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp);
	}



	/// @brief Based on a comparison, conditionally exchange the encapsulated object
	/// @param[in] src Value to set if comparison is equal
	/// @param[in] cmp Value to compare against
	/// @param[out] rtn Returns the original pointer value
	/// @return True if the comparison was equal and the value set.
	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<is_reference_type_v<std::remove_reference_t<T2> > && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
	bool compare_exchange(T2&& src, const T3& cmp, T4& rtn) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
		{
			if constexpr (is_rc_type_v<T3>)
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp.get_container(), rtn);
			}
			else
			{
				if constexpr (is_rc_type_v<T4>)
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn.get_container());
				else
					return m_container.compare_exchange(std::forward<T2>(src).get_container(), cmp, rtn);
			}
		}
		else if constexpr (is_rc_type_v<T3>)
		{
			if constexpr (is_rc_type_v<T4>)
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn.get_container());
			else
				return m_container.compare_exchange(std::forward<T2>(src), cmp.get_container(), rtn);
		}
		else if constexpr (is_rc_type_v<T4>)
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn.get_container());
		else
			return m_container.compare_exchange(std::forward<T2>(src), cmp, rtn);
	}
	/// @}

	template <typename F>
	void on_released(F&& f) const
	{
		m_container.on_released(std::forward<F>(f));
	}
};


}


#endif
