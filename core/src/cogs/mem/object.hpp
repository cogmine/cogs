//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_OBJECT
#define COGS_HEADER_MEM_OBJECT


#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/rc_container_base.hpp"
#include "cogs/mem/rcptr.hpp"


namespace cogs {


/// @ingroup Mem
/// @brief A base class for objects intended to be allocated with rcnew.  Provides access to this_rcref, etc.
class object : public rc_object_base
{
protected:
	rc_obj_base*	m_desc;

	object() = delete;

public:
	object(const ptr<rc_obj_base>& desc) : m_desc(desc.get_ptr()) { }

	rc_obj_base* get_desc() const			{ return m_desc; }
	rc_obj_base* get_desc() const volatile	{ return m_desc; }	// Set on construction and not modified.

	void set_desc(const ptr<rc_obj_base>& desc) { m_desc = desc.get_ptr(); }
	void set_desc(const ptr<rc_obj_base>& desc) volatile { m_desc = desc.get_ptr(); }

	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const
	{
		m_desc->acquire(refStrengthType);
		return m_desc;
	}

	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const
	{
		m_desc->release(refStrengthType);
		return m_desc;
	}

	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const volatile	{ return const_cast<const object*>(this)->self_acquire(refStrengthType); }
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const volatile	{ return const_cast<const object*>(this)->self_release(refStrengthType); }


	template <class type> const rcref<type>& get_self_rcref(type* obj,
		unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage.dereference(); }
	
	template <class type> const rcref<const type>& get_self_rcref(const type* obj,
		unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage.dereference(); }
	
	template <class type> const rcref<volatile type>& get_self_rcref(volatile type* obj, 
		unowned_t<rcptr<volatile type> >& storage = unowned_t<rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage.dereference(); }
	
	template <class type> const rcref<const volatile type>& get_self_rcref(const volatile type* obj, 
		unowned_t<rcptr<const volatile type> >& storage = unowned_t<rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage.dereference(); }

	
	template <class type> const rcptr<type>& get_self_rcptr(type* obj,
		unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const rcptr<const type>& get_self_rcptr(const type* obj,
		unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const rcptr<volatile type>& get_self_rcptr(volatile type* obj,
		unowned_t<rcptr<volatile type> >& storage = unowned_t<rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const rcptr<const volatile type>& get_self_rcptr(const volatile type* obj,
		unowned_t<rcptr<const volatile type> >& storage = unowned_t<rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage; }

	
	template <class type> const weak_rcptr<type>& get_self_weak_rcptr(type* obj,
		unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const weak_rcptr<const type>& get_self_weak_rcptr(const type* obj,
		unowned_t<weak_rcptr<const type> >& storage = unowned_t<weak_rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const weak_rcptr<volatile type>& get_self_weak_rcptr(volatile type* obj,
		unowned_t<weak_rcptr<volatile type> >& storage = unowned_t<weak_rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage; }
	
	template <class type> const weak_rcptr<const volatile type>& get_self_weak_rcptr(const volatile type* obj,
		unowned_t<weak_rcptr<const volatile type> >& storage = unowned_t<weak_rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, m_desc); return storage; }

};


// As an optimization, use of this_xxxx does not acquire a reference.
// As a member function is executing, the reference is assumed to already be in scope.

/// @brief When used in a class derived from object, returns the object's reference-counting descriptor
#define this_desc									(this->get_desc())

/// @brief When used in a class derived from object, returns a rcref referring to itself
#define this_rcref									(this->get_self_rcref(this))

/// @brief When used in a class derived from object, returns a rcptr referring to itself
#define this_rcptr									(this->get_self_rcptr(this))

/// @brief When used in a class derived from object, returns a this_weak referring to itself
#define this_weak_rcptr								(this->get_self_weak_rcptr(this))


#define COGS_IMPLEMENT_MULTIPLY_DERIVED_OBJECT_GLUE2(derived_type, base1, base2)												\
	rc_obj_base* get_desc() const			{ return base1::get_desc(); }												\
	rc_obj_base* get_desc() const volatile	{ return base1::get_desc(); }												\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_release(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_release(refStrengthType); }																	\
	template <typename type> rcref<type> get_self_rcref(type* obj) const													\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const type> get_self_rcref(const type* obj) const										\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<volatile type> get_self_rcref(volatile type* obj) const volatile						\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const volatile type> get_self_rcref(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcptr<type> get_self_rcptr(type* obj) const													\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const type> get_self_rcptr(const type* obj) const										\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<volatile type> get_self_rcptr(volatile type* obj) const volatile						\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const volatile type> get_self_rcptr(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> weak_rcptr<type> get_self_weak_rcptr(type* obj) const											\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const type> get_self_weak_rcptr(const type* obj) const								\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<volatile type> get_self_weak_rcptr(volatile type* obj) const volatile				\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const volatile type> get_self_weak_rcptr(const volatile type* obj) const volatile	\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	void set_desc(const ptr<rc_obj_base>& desc) { base1::set_desc(desc); base2::set_desc(desc); }											\
	void set_desc(const ptr<rc_obj_base>& desc) volatile { base1::set_desc(desc); base2::set_desc(desc); }	



#define COGS_IMPLEMENT_MULTIPLY_DERIVED_OBJECT_GLUE3(derived_type, base1, base2, base3)											\
	rc_obj_base* get_desc() const			{ return base1::get_desc(); }												\
	rc_obj_base* get_desc() const volatile	{ return base1::get_desc(); }												\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_release(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_release(refStrengthType); }																	\
	template <typename type> rcref<type> get_self_rcref(type* obj) const													\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const type> get_self_rcref(const type* obj) const										\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<volatile type> get_self_rcref(volatile type* obj) const volatile						\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const volatile type> get_self_rcref(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcptr<type> get_self_rcptr(type* obj) const													\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const type> get_self_rcptr(const type* obj) const										\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<volatile type> get_self_rcptr(volatile type* obj) const volatile						\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const volatile type> get_self_rcptr(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> weak_rcptr<type> get_self_weak_rcptr(type* obj) const											\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const type> get_self_weak_rcptr(const type* obj) const								\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<volatile type> get_self_weak_rcptr(volatile type* obj) const volatile				\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const volatile type> get_self_weak_rcptr(const volatile type* obj) const volatile	\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	void set_desc(const ptr<rc_obj_base>& desc) { base1::set_desc(desc); base2::set_desc(desc); }											\
	void set_desc(const ptr<rc_obj_base>& desc) volatile { base1::set_desc(desc); base2::set_desc(desc); }	


#define COGS_IMPLEMENT_MULTIPLY_DERIVED_OBJECT_GLUE4(derived_type, base1, base2, base3, base4)									\
	rc_obj_base* get_desc() const			{ return base1::get_desc(); }												\
	rc_obj_base* get_desc() const volatile	{ return base1::get_desc(); }												\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_acquire(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_acquire(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const									\
	{ return base1::self_release(refStrengthType); }																	\
	rc_obj_base* self_release(reference_strength_type refStrengthType = strong) const volatile							\
	{ return base1::self_release(refStrengthType); }																	\
	template <typename type> rcref<type> get_self_rcref(type* obj) const													\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const type> get_self_rcref(const type* obj) const										\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<volatile type> get_self_rcref(volatile type* obj) const volatile						\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcref<const volatile type> get_self_rcref(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcref(obj); }																				\
	template <typename type> rcptr<type> get_self_rcptr(type* obj) const													\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const type> get_self_rcptr(const type* obj) const										\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<volatile type> get_self_rcptr(volatile type* obj) const volatile						\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> rcptr<const volatile type> get_self_rcptr(const volatile type* obj) const volatile			\
	{ return base1::get_self_rcptr(obj); }																				\
	template <typename type> weak_rcptr<type> get_self_weak_rcptr(type* obj) const											\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const type> get_self_weak_rcptr(const type* obj) const								\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<volatile type> get_self_weak_rcptr(volatile type* obj) const volatile				\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	template <typename type> weak_rcptr<const volatile type> get_self_weak_rcptr(const volatile type* obj) const volatile	\
	{ return base1::get_self_weak_rcptr(obj); }																			\
	void set_desc(const ptr<rc_obj_base>& desc) { base1::set_desc(desc); base2::set_desc(desc); }											\
	void set_desc(const ptr<rc_obj_base>& desc) volatile { base1::set_desc(desc); base2::set_desc(desc); }	

}


#endif
