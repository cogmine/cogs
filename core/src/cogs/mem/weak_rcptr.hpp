//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_WEAK_RCPTR
#define COGS_HEADER_MEM_WEAK_RCPTR


#include "cogs/env.hpp"
#include "cogs/mem/is_pointer_type.hpp"
#include "cogs/mem/is_reference_type.hpp"
#include "cogs/mem/is_rc_type.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rc_container.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


/// @ingroup ReferenceCountedReferenceContainerTypes
/// @brief A weak nullable reference-counted reference container.
///
/// A weak_rcptr will retain a reference to a reference-counted object only if there are also 'strong' (rcref/rcptr) references
/// elsewhere.  Once all strong references are released, a weak_rcptr can no longer be used to get a pointer to the object.
///
/// weak_rcptr is useful for preventing circular references, which would prevent reference-counted objects from being released.
/// A good approach is to use weak_rcptr for all references except for references specifically intended to extend the scope of the object.
///
/// @tparam T Data type pointed to
template <typename T>
class weak_rcptr
{
public:
	/// @brief Alias to the type pointed to
	typedef T type;

	/// @brief Alias to this type.
	typedef weak_rcptr<type> this_t;

	/// @brief A type constructible using this type that will prevent the encapsulated object from becoming deallocated or relocated
	typedef rcptr<type> locked_t;

private:
	typedef rc_container<type, weak> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

public:
	/// @brief Provides a weak_rcptr with a different referenced type.
	/// @tparam type2 Data type referenced
	template <typename type2>
	class cast
	{
	public:
		/// @brief A weak_rcptr with a different referenced type.
		typedef weak_rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	/// @{
	/// @brief Constructor.  Initializes weak_rcptr to NULL.
	weak_rcptr() { }
	/// @}

	weak_rcptr(this_t&& src) : m_container(std::move(src.m_container)) { }

	/// @{
	/// @brief Initializes rcptr to specified value.
	/// @param src Initial value
	weak_rcptr(const this_t& src) : m_container(src.m_container) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const rcref<type2>& src) : m_container(src.m_container) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const weak_rcptr<type2>& src) : m_container(src.m_container) { }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(weak_rcptr<type2>&& src) : m_container(std::move(src.m_container)) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const volatile rcref<type2>& src) : m_container(src.m_container) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const volatile rcptr<type2>& src) : m_container(src.m_re.m_containerf) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Initial value
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	weak_rcptr(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }

	/// @brief Initializes weak_rcptr to specified value.
	///
	/// A weak_rcptr may contain a non reference-counted pointer.  By specifying a pointer value where a weak_rcptr
	/// is expected, the caller is gauranteeing the resource pointed to will remain in scope until no longer referenced
	/// by the weak_rcptr.
	/// @param src Initial value
	weak_rcptr(type* src) : m_container(src) { }

	/// @brief Initializes a rcptr to contain the specified object and descriptor, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	weak_rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }
	/// @}

	/// @{
	/// @brief Assignment
	/// @param src Value to set
	/// @return A reference to this
	this_t& operator=(type* src) { m_container.operator=(src); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

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

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(weak_rcptr<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }

	/// @brief Assignment
	///
	/// Implicit conversion is allowed between compatible pointer types.
	/// @param src Value to set
	/// @return A reference to this
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	this_t& operator=(const volatile weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }
	/// @brief Thread-safe version of operator=()
	volatile this_t& operator=(type* src) volatile { m_container.operator=(src); return *this; }
	/// @brief Thread-safe version of operator=()
	volatile this_t& operator=(const this_t& src) volatile { m_container.operator=(src.m_container); return *this; }
	/// @brief Thread-safe version of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	/// @brief Thread-safe version of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const rcptr<type2>& src) volatile { m_container.operator=(src.m_ref.m_container); return *this; }
	/// @brief Thread-safe version of operator=()
	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(const weak_rcptr<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	/// @}

	volatile this_t& operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); }

	template <typename type2, typename enable = std::enable_if_t<std::is_convertible_v<type2*, type*> > >
	volatile this_t& operator=(weak_rcptr<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }


	/// @{
	/// @brief Sets to the specified object, without incrementing its reference count.
	void set(const ptr<type>& obj) { m_container.set(obj); }

	/// @brief Thread-safe implementation of set()
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }

	/// @brief Sets to the specified object, without incrementing its reference count.
	/// @param obj Object to set value to
	/// @param desc Descriptor of object to set value to
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }

	/// @brief Thread-safe implementation of set()
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }
	/// @}

	/// @{
	/// @brief Releases the contained object, decrementing the reference count.
	/// @return True if the reference count reached zero and the object was destructed and/or disposed.
	bool release() { return m_container.release(); }
	/// @brief Thread-safe implementation of release()
	bool release() volatile { return m_container.release(); }
	/// @}

	/// @{
	/// @brief Releases the contained object without decrementing the reference count
	///
	/// Although a weak reference does not extend the scope of the contained object, it
	/// does extend the scope of a descriptor in which it retains a 'weak' count.  When
	/// all weak references are released, the descriptor is released.  disown() will release
	/// the contained objects without decrementing the weak reference count.
	rc_obj_base* disown() { return m_container.disown(); }
	/// @brief Thread-safe version of disown()
	rc_obj_base* disown() volatile { return m_container.disown(); }
	/// @}

	type* get_obj() const { return m_container.get_obj(); }
	type* get_obj() const volatile { return m_container.get_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object, after checking if still in scope.
	/// @return A pointer to the encapsulated object, or NULL if no longer in scope.
	type* get_ptr() const { return get_obj(); }
	/// @brief Thread-safe version of get_ptr()
	type* get_ptr() const volatile { return get_obj(); }
	/// @}

	type* peek_obj() const { return m_container.peek_obj(); }
	type* peek_obj() const volatile { return m_container.peek_obj(); }

	/// @{
	/// @brief Gets a pointer to the encapsulated object without checking if it's still in scope.
	/// @return A pointer to the encapsulated object, which may or may not still be in scope.
	type* peek_ptr() const { return peek_obj(); }
	/// @brief Thread-safe version of peek_ptr()
	type* peek_ptr() const volatile { return peek_obj(); }
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
	const weak_rcptr<type2>& static_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to()&&
	{
		weak_rcptr<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& dynamic_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : get_desc();
		storage.set(tmp, desc);
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2>&& dynamic_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned())&&
	{
		type2* tmp = dynamic_cast<type2*>(peek_obj()); // A failure here means type conversion (user) error.
		rc_obj_base* desc = !tmp ? nullptr : disown();
		storage.set(tmp, desc);
		return std::move(storage);
	}

	template <typename type2>
	weak_rcptr<type2> dynamic_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template dynamic_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& const_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to()&&
	{
		weak_rcptr<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& reinterpret_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to()&&
	{
		weak_rcptr<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template reinterpret_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& member_cast_to(type2& member, unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(&member, get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> member_cast_to(type2& member, unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned())&&
	{
		storage.set(&member, disown());
		return std::move(storage);
	}

	template <typename type2>
	const weak_rcptr<type2>& member_cast_to(volatile type2& member, unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const volatile&
	{
		storage.set(&member, get_desc());
		return storage;
	}


#ifdef DOXYGEN
	/// @{
	/// @brief static_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const weak_rcptr<type2>& static_cast_to() const;

	/// @brief Thread-safe implementation of static_cast_to()
	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief dynamic_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const weak_rcptr<type2>& dynamic_cast_to() const;

	/// @brief Thread-safe implementation of dynamic_cast_to()
	template <typename type2>
	weak_rcptr<type2> dynamic_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief const_cast the encapsulated pointer
	/// @tparam type2 Secondary data type
	/// @return Result of the cast
	template <typename type2>
	const weak_rcptr<type2>& const_cast_to() const;

	/// @brief Thread-safe implementation of const_cast_to()
	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile;
	/// @}

	/// @{
	/// @brief Gets a weak_rcptr to a data member of the contained reference-counted object
	/// @tparam type2 Secondary data type
	/// @param member Reference to the member to create a weak_rcptr to.  This can be anything known to
	/// be within the allocated block managed by the same descriptor.
	/// @return weak_rcptr to the member.
	template <typename type2>
	const weak_rcptr<type2>& member_cast_to(type2& member) const;
	/// @}
#endif

	/// @{
	/// @brief Equality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are equal
	template <typename type2> bool operator==(type2* const& cmp) const { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	/// @}



	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}




	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}


	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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
	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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



	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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
	/// @brief Thread-safe version of get_ptr()
	size_t get_mark() const volatile { return m_container.get_mark(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with marked bits removed.
	///
	/// If marked bits are set, direct use of the pointer value will include the marked bits.
	/// get_unmarked() must be used instead.
	/// @return The unmarked pointer value
	type* get_unmarked() const { return m_container.get_unmarked(); }
	/// @brief Thread-safe version of get_ptr()
	type* get_unmarked() const volatile { return m_container.get_unmarked(); }
	/// @}

	/// @{
	/// @brief Gets the pointer value with the specified bitmask applied
	/// @param mark Bitmask of bits to mark
	/// @return The pointer value with the mark applied.
	type* get_marked(size_t mark) const { return m_container.get_marked(mark); }
	/// @brief Thread-safe version of get_ptr()
	type* get_marked(size_t mark) const volatile { return m_container.get_marked(mark); }
	/// @}

	/// @{
	/// @brief Clears any marked bits.
	void clear_mark() { m_container.clear_mark(); }
	/// @brief Thread-safe version of get_ptr()
	void clear_mark() volatile { m_container.clear_mark(); }
	/// @}

	/// @{
	/// @brief Sets marked bits.
	/// @param mark Bitmask of bits to set
	void set_mark(size_t mark) { m_container.set_mark(mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_mark(size_t mark) volatile { m_container.set_mark(mark); }
	/// @}

	/// @{
	/// @brief Sets the value to only the specified marked bit, clearing the original pointer value.
	/// @param mark Bitmask of bits to set.
	void set_to_mark(size_t mark) { m_container.set_to_mark(mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_to_mark(size_t mark) volatile { m_container.set_to_mark(mark); }
	/// @}

	/// @{
	/// @brief Set to the pointer value specified, and applied the speficied marked bits.
	/// @param p Value to set.
	/// @param mark Bitmask of bits to set.
	void set_marked(type* p, size_t mark) { m_container.set_marked(p, mark); }
	/// @brief Thread-safe version of get_ptr()
	void set_marked(type* p, size_t mark) volatile { m_container.set_marked(p, mark); }
	/// @}
};


template <>
class weak_rcptr<void>
{
public:
	typedef void type;
	typedef weak_rcptr<type> this_t;

private:
	typedef rc_container<type, weak> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

public:

	template <typename type2>
	class cast
	{
	public:
		typedef weak_rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	weak_rcptr() { }
	weak_rcptr(const this_t& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const weak_rcptr<type2>& src) : m_container(src.m_container) { }

	weak_rcptr(this_t&& src) : m_container(std::move(src.m_container)) { }
	template <typename type2> weak_rcptr(weak_rcptr<type2>&& src) : m_container(std::move(src.m_container)) { }

	weak_rcptr(type* obj) : m_container(obj) { }

	weak_rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(type* src) { m_container.operator=(src); return *this; }

	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(rcref<type2>& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	template <typename type2> this_t& operator=(rcptr<type2>& src) { m_container.operator=(std::move(src.m_ref.m_container)); return *this; }
	template <typename type2> this_t& operator=(weak_rcptr<type2>& src) { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }

	volatile this_t& operator=(type* src) volatile { m_container.operator=(src); return *this; }

	volatile this_t& operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { m_container.operator=(src.m_container); return *this; }

	template <typename type2> volatile this_t& operator=(weak_rcptr<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2> volatile this_t& operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const rcptr<type2>& src) volatile { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const weak_rcptr<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

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

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return m_container.is_empty(); }
	bool is_empty() const volatile { return m_container.is_empty(); }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


	template <typename type2>
	const weak_rcptr<type2>& static_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to()&&
	{
		weak_rcptr<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& const_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to()&&
	{
		weak_rcptr<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& reinterpret_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to()&&
	{
		weak_rcptr<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template reinterpret_cast_to<type2>();
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
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	/// @}




	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}




	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}


	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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
	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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



	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

};


template <>
class weak_rcptr<const void>
{
public:
	typedef const void type;
	typedef weak_rcptr<type> this_t;

private:
	typedef rc_container<type, weak> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

public:

	template <typename type2>
	class cast
	{
	public:
		typedef weak_rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	weak_rcptr() { }

	weak_rcptr(this_t&& src) : m_container(std::move(src.m_container)) { }
	weak_rcptr(const this_t& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const weak_rcptr<type2>& src) : m_container(src.m_container) { }

	template <typename type2> weak_rcptr(weak_rcptr<type2>&& src) : m_container(std::move(src.m_container)) { }

	weak_rcptr(type* obj) : m_container(obj) { }

	weak_rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(type* src) { m_container.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }


	volatile this_t& operator=(type* src) volatile { m_container.operator=(src); return *this; }
	volatile this_t& operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { m_container.operator=(src.m_container); return *this; }

	template <typename type2> volatile this_t& operator=(weak_rcptr<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2> volatile this_t& operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const rcptr<type2>& src) volatile { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const weak_rcptr<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

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

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return m_container.is_empty(); }
	bool is_empty() const volatile { return m_container.is_empty(); }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


	template <typename type2>
	const weak_rcptr<type2>& static_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to()&&
	{
		weak_rcptr<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& const_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to()&&
	{
		weak_rcptr<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& reinterpret_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to()&&
	{
		weak_rcptr<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template reinterpret_cast_to<type2>();
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
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	/// @}




	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}




	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}


	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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
	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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



	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

};


template <>
class weak_rcptr<volatile void>
{
public:
	typedef volatile void type;
	typedef weak_rcptr<type> this_t;

private:
	typedef rc_container<type, weak> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

public:

	template <typename type2>
	class cast
	{
	public:
		typedef weak_rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	weak_rcptr() { }
	weak_rcptr(this_t&& src) : m_container(std::move(src.m_container)) { }
	weak_rcptr(const this_t& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const weak_rcptr<type2>& src) : m_container(src.m_container) { }

	template <typename type2> weak_rcptr(weak_rcptr<type2>&& src) : m_container(std::move(src.m_container)) { }

	weak_rcptr(type* obj) : m_container(obj) { }

	weak_rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(type* src) { m_container.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }


	volatile this_t& operator=(type* src) volatile { m_container.operator=(src); return *this; }
	volatile this_t& operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { m_container.operator=(src.m_container); return *this; }

	template <typename type2> volatile this_t& operator=(weak_rcptr<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2> volatile this_t& operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const rcptr<type2>& src) volatile { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const weak_rcptr<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

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

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return m_container.is_empty(); }
	bool is_empty() const volatile { return m_container.is_empty(); }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


	template <typename type2>
	const weak_rcptr<type2>& static_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to()&&
	{
		weak_rcptr<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& const_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to()&&
	{
		weak_rcptr<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& reinterpret_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to()&&
	{
		weak_rcptr<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template reinterpret_cast_to<type2>();
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
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	/// @}




	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}




	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}


	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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
	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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



	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

};


template <>
class weak_rcptr<const volatile void>
{
public:
	typedef const volatile void type;
	typedef weak_rcptr<type> this_t;

private:
	typedef rc_container<type, weak> container_t;
	container_t m_container;

	container_t&& get_container()&& { return std::move(m_container); }
	container_t& get_container()& { return m_container; }
	const container_t& get_container() const& { return m_container; }
	volatile container_t& get_container() volatile& { return m_container; }
	const volatile container_t& get_container() const volatile& { return m_container; }

	template <typename type2>
	friend class rcref;

	template <typename type2>
	friend class rcptr;

	template <typename type2>
	friend class weak_rcptr;

	template <template <typename> class, template <typename> class, template <typename> class>
	friend class rc_object_t;

public:

	template <typename type2>
	class cast
	{
	public:
		typedef weak_rcptr<type2> type;
	};
	template <typename type2>
	using cast_t = typename cast<type2>::type;

	weak_rcptr() { }
	weak_rcptr(this_t&& src) : m_container(std::move(src.m_container)) { }
	weak_rcptr(const this_t& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const volatile rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const volatile weak_rcptr<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcref<type2>& src) : m_container(src.m_container) { }
	template <typename type2> weak_rcptr(const rcptr<type2>& src) : m_container(src.m_ref.m_container) { }
	template <typename type2> weak_rcptr(const weak_rcptr<type2>& src) : m_container(src.m_container) { }

	template <typename type2> weak_rcptr(weak_rcptr<type2>&& src) : m_container(std::move(src.m_container)) { }

	weak_rcptr(type* obj) : m_container(obj) { }

	weak_rcptr(const ptr<type>& obj, const ptr<rc_obj_base>& desc) : m_container(obj, desc) { }

	this_t& operator=(type* src) { m_container.operator=(src); return *this; }
	this_t& operator=(this_t&& src) { m_container.operator=(std::move(src.m_container)); return *this; }
	this_t& operator=(const this_t& src) { m_container.operator=(src.m_container); return *this; }
	this_t& operator=(const volatile this_t& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(const rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcref<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile rcptr<type2>& src) { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> this_t& operator=(const weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }
	template <typename type2> this_t& operator=(const volatile weak_rcptr<type2>& src) { m_container.operator=(src.m_container); return *this; }

	template <typename type2> this_t& operator=(weak_rcptr<type2>&& src) { m_container.operator=(std::move(src.m_container)); return *this; }

	volatile this_t& operator=(type* src) volatile { m_container.operator=(src); return *this; }
	volatile this_t& operator=(this_t&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { m_container.operator=(src.m_container); return *this; }

	template <typename type2> volatile this_t& operator=(weak_rcptr<type2>&& src) volatile { m_container.operator=(std::move(src.m_container)); return *this; }

	template <typename type2> volatile this_t& operator=(const rcref<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const rcptr<type2>& src) volatile { m_container.operator=(src.m_ref.m_container); return *this; }
	template <typename type2> volatile this_t& operator=(const weak_rcptr<type2>& src) volatile { m_container.operator=(src.m_container); return *this; }

	void set(const ptr<type>& obj) { m_container.set(obj); }
	void set(const ptr<type>& obj) volatile { m_container.set(obj); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) { m_container.set(obj, desc); }
	void set(const ptr<type>& obj, const ptr<rc_obj_base>& desc) volatile { m_container.set(obj, desc); }

	bool release() { return m_container.release(); }
	bool release() volatile { return m_container.release(); }

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

	rc_obj_base* get_desc() const { return m_container.get_desc(); }
	rc_obj_base* get_desc() const volatile { return m_container.get_desc(); }

	bool is_empty() const { return m_container.is_empty(); }
	bool is_empty() const volatile { return m_container.is_empty(); }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


	template <typename type2>
	const weak_rcptr<type2>& static_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to()&&
	{
		weak_rcptr<type2> result(
			static_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> static_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template static_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& const_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to()&&
	{
		weak_rcptr<type2> result(
			const_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> const_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template const_cast_to<type2>();
		return result;
	}

	template <typename type2>
	const weak_rcptr<type2>& reinterpret_cast_to(unowned_t<weak_rcptr<type2> >& storage = unowned_t<weak_rcptr<type2> >().get_unowned()) const&
	{
		storage.set(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			get_desc());
		return storage;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to()&&
	{
		weak_rcptr<type2> result(
			reinterpret_cast<type2*>(peek_obj()), // A failure here means type conversion (user) error.
			disown());
		return result;
	}

	template <typename type2>
	weak_rcptr<type2> reinterpret_cast_to() const volatile&
	{
		this_t tmp(*this);
		weak_rcptr<type2> result = tmp.template reinterpret_cast_to<type2>();
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
	template <typename type2> bool operator==(const rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const { return m_container == cmp.m_container; }

	template <typename type2> bool operator==(type2* const& cmp) const volatile { return get_ptr() == cmp; }
	template <typename type2> bool operator==(type2* const volatile& cmp) const volatile { return get_ptr() == atomic::load(cmp); }
	template <typename type2> bool operator==(const ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile ptr<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile ref<type2>& cmp) const volatile { return get_ptr() == cmp.get_ptr(); }
	template <typename type2> bool operator==(const volatile rcref<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	template <typename type2> bool operator==(const volatile rcptr<type2>& cmp) const volatile { return m_container == cmp.m_ref.m_container; }
	template <typename type2> bool operator==(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container == cmp.m_container; }
	/// @}

	/// @{
	/// @brief Inequality operator
	/// @param cmp Pointer to test against
	/// @return True if the values are inequal
	template <typename type2> bool operator!=(type2* const& cmp) const { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const { return m_container != cmp.m_container; }

	template <typename type2> bool operator!=(type2* const& cmp) const volatile { return get_ptr() != cmp; }
	template <typename type2> bool operator!=(type2* const volatile& cmp) const volatile { return get_ptr() != atomic::load(cmp); }
	template <typename type2> bool operator!=(const ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile ref<type2>& cmp) const volatile { return get_ptr() != cmp.get_ptr(); }
	template <typename type2> bool operator!=(const volatile rcref<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	template <typename type2> bool operator!=(const volatile rcptr<type2>& cmp) const volatile { return m_container != cmp.m_ref.m_container; }
	template <typename type2> bool operator!=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container != cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greater-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than the parameter
	template <typename type2> bool operator>(type2* const& cmp) const { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const { return m_container > cmp.m_container; }

	template <typename type2> bool operator>(type2* const& cmp) const volatile { return get_ptr() > cmp; }
	template <typename type2> bool operator>(type2* const volatile& cmp) const volatile { return get_ptr() > atomic::load(cmp); }
	template <typename type2> bool operator>(const ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile ptr<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile ref<type2>& cmp) const volatile { return get_ptr() > cmp.get_ptr(); }
	template <typename type2> bool operator>(const volatile rcref<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	template <typename type2> bool operator>(const volatile rcptr<type2>& cmp) const volatile { return m_container > cmp.m_ref.m_container; }
	template <typename type2> bool operator>(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container > cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than the parameter
	template <typename type2> bool operator<(type2* const& cmp) const { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const { return m_container < cmp.m_container; }

	template <typename type2> bool operator<(type2* const& cmp) const volatile { return get_ptr() < cmp; }
	template <typename type2> bool operator<(type2* const volatile& cmp) const volatile { return get_ptr() < atomic::load(cmp); }
	template <typename type2> bool operator<(const ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile ptr<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile ref<type2>& cmp) const volatile { return get_ptr() < cmp.get_ptr(); }
	template <typename type2> bool operator<(const volatile rcref<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	template <typename type2> bool operator<(const volatile rcptr<type2>& cmp) const volatile { return m_container < cmp.m_ref.m_container; }
	template <typename type2> bool operator<(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container < cmp.m_container; }
	/// @}

	/// @{
	/// @brief Greather-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is greater than or equal to the parameter
	template <typename type2> bool operator>=(type2* const& cmp) const { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const { return m_container >= cmp.m_container; }

	template <typename type2> bool operator>=(type2* const& cmp) const volatile { return get_ptr() >= cmp; }
	template <typename type2> bool operator>=(type2* const volatile& cmp) const volatile { return get_ptr() >= atomic::load(cmp); }
	template <typename type2> bool operator>=(const ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile ref<type2>& cmp) const volatile { return get_ptr() >= cmp.get_ptr(); }
	template <typename type2> bool operator>=(const volatile rcref<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	template <typename type2> bool operator>=(const volatile rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_ref.m_container; }
	template <typename type2> bool operator>=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container >= cmp.m_container; }
	/// @}

	/// @{
	/// @brief Less-than-or-equal operator
	/// @param cmp Pointer to test against
	/// @return True if this value is less than or equal to the parameter
	template <typename type2> bool operator<=(type2* const& cmp) const { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const { return m_container <= cmp.m_container; }

	template <typename type2> bool operator<=(type2* const& cmp) const volatile { return get_ptr() <= cmp; }
	template <typename type2> bool operator<=(type2* const volatile& cmp) const volatile { return get_ptr() <= atomic::load(cmp); }
	template <typename type2> bool operator<=(const ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile ptr<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile ref<type2>& cmp) const volatile { return get_ptr() <= cmp.get_ptr(); }
	template <typename type2> bool operator<=(const volatile rcref<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	template <typename type2> bool operator<=(const volatile rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_ref.m_container; }
	template <typename type2> bool operator<=(const volatile weak_rcptr<type2>& cmp) const volatile { return m_container <= cmp.m_container; }
	/// @}



	///// @{
	///// @brief Swap the pointer value
	///// @param[in,out] wth Value to swap
	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> && !std::is_volatile_v<T2> > >
	void swap(T2& wth) volatile
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<T2> && !std::is_const_v<T2> > >
	void swap(volatile T2& wth)
	{
		if constexpr (is_rc_type_v<T2>)
			m_container.swap(wth.get_container());
		else
			m_container.swap(wth);
	}
	///// @}




	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src)
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}

	template <typename T2, typename = std::enable_if_t<is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> > > >
	this_t exchange(T2&& src) volatile
	{
		if constexpr (is_rc_type_v<std::remove_reference_t<T2> >)
			return m_container.exchange(std::forward<T2>(src).get_container());
		else
			return m_container.exchange(std::forward<T2>(src));
	}


	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && is_pointer_type_v<T3> && !std::is_const_v<T3> > >
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
	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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

	template <typename T2, typename T3, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) > >
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



	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

	template <typename T2, typename T3, typename T4, typename = std::enable_if_t<(is_pointer_type_v<std::remove_reference_t<T2> > || is_reference_type_v<std::remove_reference_t<T2> >) && (is_reference_type_v<T3> || is_pointer_type_v<T3>) && (is_reference_type_v<T4> || is_pointer_type_v<T4>) && !std::is_const_v<T4> > >
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

};


}


#include "cogs/mem/rcnew.hpp"


#endif
