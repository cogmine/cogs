//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_OBJECT
#define COGS_HEADER_MEM_OBJECT


#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/unowned.hpp"


namespace cogs {


template <typename T>
class rcptr;

template <typename T>
class rcref;

template <typename T>
class weak_rcptr;

struct rcnew_glue_obj_t
{
	mutable void* m_obj;
	mutable rc_obj_base* m_desc;
	mutable const rcnew_glue_obj_t* m_saved;
};

/// @ingroup Mem
/// @brief A base class for objects intended to be allocated with rcnew.  Provides access to this_rcref, etc.
class object
{
protected:
	rc_obj_base* m_desc;

public:
	inline static thread_local const rcnew_glue_obj_t* rcnew_glue_obj = nullptr;

	object()
	{
		COGS_ASSERT(object::rcnew_glue_obj != nullptr);
		COGS_ASSERT(object::rcnew_glue_obj->m_desc != nullptr);
		COGS_ASSERT(object::rcnew_glue_obj->m_desc->contains(this));
		m_desc = object::rcnew_glue_obj->m_desc;
	}

	rc_obj_base* get_desc() const { return m_desc; }
	rc_obj_base* get_desc() const volatile { return const_cast<const object*>(this)->m_desc; } // Set on construction and not modified.

	rc_obj_base* self_acquire(reference_strength referenceStrength = reference_strength::strong) const
	{
		m_desc->acquire(referenceStrength);
		return m_desc;
	}

	rc_obj_base* self_release(reference_strength referenceStrength = reference_strength::strong) const
	{
		m_desc->release(referenceStrength);
		return m_desc;
	}

	rc_obj_base* self_acquire(reference_strength referenceStrength = reference_strength::strong) const volatile { return const_cast<const object*>(this)->self_acquire(referenceStrength); }
	rc_obj_base* self_release(reference_strength referenceStrength = reference_strength::strong) const volatile { return const_cast<const object*>(this)->self_release(referenceStrength); }

	template <class type> const rcref<type>& get_self_rcref(type* obj,
		unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage.dereference(); }

	template <class type> const rcref<const type>& get_self_rcref(const type* obj,
		unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage.dereference(); }

	template <class type> const rcref<volatile type>& get_self_rcref(volatile type* obj,
		unowned_t<rcptr<volatile type> >& storage = unowned_t<rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage.dereference(); }

	template <class type> const rcref<const volatile type>& get_self_rcref(const volatile type* obj,
		unowned_t<rcptr<const volatile type> >& storage = unowned_t<rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage.dereference(); }


	template <class type> const rcptr<type>& get_self_rcptr(type* obj,
		unowned_t<rcptr<type> >& storage = unowned_t<rcptr<type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const rcptr<const type>& get_self_rcptr(const type* obj,
		unowned_t<rcptr<const type> >& storage = unowned_t<rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const rcptr<volatile type>& get_self_rcptr(volatile type* obj,
		unowned_t<rcptr<volatile type> >& storage = unowned_t<rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const rcptr<const volatile type>& get_self_rcptr(const volatile type* obj,
		unowned_t<rcptr<const volatile type> >& storage = unowned_t<rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage; }


	template <class type> const weak_rcptr<type>& get_self_weak_rcptr(type* obj,
		unowned_t<weak_rcptr<type> >& storage = unowned_t<weak_rcptr<type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const weak_rcptr<const type>& get_self_weak_rcptr(const type* obj,
		unowned_t<weak_rcptr<const type> >& storage = unowned_t<weak_rcptr<const type> >().get_unowned()) const
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const weak_rcptr<volatile type>& get_self_weak_rcptr(volatile type* obj,
		unowned_t<weak_rcptr<volatile type> >& storage = unowned_t<weak_rcptr<volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage; }

	template <class type> const weak_rcptr<const volatile type>& get_self_weak_rcptr(const volatile type* obj,
		unowned_t<weak_rcptr<const volatile type> >& storage = unowned_t<weak_rcptr<const volatile type> >().get_unowned()) const volatile
	{ storage.set(obj, get_desc()); return storage; }
};

/// @brief When used in a class derived from object, returns a rcref referring to itself
#define this_rcref (this->get_self_rcref(this))

/// @brief When used in a class derived from object, returns a rcptr referring to itself
#define this_rcptr (this->get_self_rcptr(this))

/// @brief When used in a class derived from object, returns a this_weak referring to itself
#define this_weak_rcptr (this->get_self_weak_rcptr(this))

}


#include "cogs/sync/transactable.hpp"


#endif
