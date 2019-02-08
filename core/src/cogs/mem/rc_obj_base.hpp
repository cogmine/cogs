//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_RC_OBJ_BASE
#define COGS_RC_OBJ_BASE


#include "cogs/collections/slink.hpp"
#include "cogs/collections/rbtree.hpp"
#include "cogs/debug.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/hazard.hpp"





namespace cogs {


class rc_obj_base;

class rc_object_base;	// seems ambiguous, need better names for these different objects!  TBD



/// @ingroup Mem
/// @brief The strength type of a reference.  Weak references are conditional to strong references remaining in scope.
///
/// Generally, when circular references may be present, strong references should
/// be used only to explicitly control the scope of an object, and weak references used
/// otherwise.
enum reference_strength_type
{
	/// @brief A strong reference is basically a reference-counting smart-pointer.
	///
	/// It extends/contains the scope/lifetime of the object it refers to.
	/// When the last strong reference to an object goes out of scope, the
	/// referenced object is released.
	strong = 1,

	/// @brief A weak reference is a like a pointer to a dynamically allocated object, that
	/// automatically becomes NULL when the object is deleted elsewhere.
	///
	/// It does not extend the scope/lifetime of object it refers to.  It is still
	/// possible to retrieve a (strong) reference to the object, as long as it is still
	/// in scope somewhere.  Weak references break circular reference chains.
	weak = 0
};


#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
void assert_not_deallocated(const ptr<void>& p);
#define ASSERT_NOT_DEALLOCATED(p) cogs::assert_not_deallocated(p)
#else
#define ASSERT_NOT_DEALLOCATED(p)	
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
void assert_no_overflow(const ptr<void>& p);
#define ASSERT_NO_OVERFLOW(p) cogs::assert_no_overflow(p)
#else
#define ASSERT_NO_OVERFLOW(p)	
#endif

#if COGS_DEBUG_RC_LOGGING 
extern volatile alignas (atomic::get_alignment_v<unsigned long>) unsigned long g_rcLogCount;
#endif 

#if COGS_DEBUG_ALLOC_LOGGING
extern volatile alignas (atomic::get_alignment_v<unsigned long>) unsigned long g_allocLogCount;
#endif 


/// @brief A base class for reference-counted objects
/// 
/// rc_obj_base is a base class for reference counted objects, not a container such as rc_obj or a
/// reference container such as rcptr.  The reference count itself is contain within rc_obj_base.
/// Counting is not automatic.  The caller must invoke acquire()/release() to add/remove references.
/// 
/// Instead of just 1 count, rc_obj_base manages 2 separate reference counts, strong and weak.
/// Weak references allow circular dependencies to be broken.  released() is called
/// when the strong reference count reaches 0, and disposed when the reference count reaches 0.
class rc_obj_base
{
private:
	rc_obj_base(const rc_obj_base&) = delete;
	rc_obj_base& operator=(const rc_obj_base&) = delete;

	struct counts_t		// Destructed  when m_reference[strong] == 0
	{					// Deallocated when m_reference[weak] == 0
		alignas (atomic::get_alignment_v<size_t>) size_t m_references[2];

		      volatile size_t& operator[](reference_strength_type refStrengthType) volatile			{ return m_references[refStrengthType]; }
		const volatile size_t& operator[](reference_strength_type refStrengthType) const volatile	{ return m_references[refStrengthType]; }

		bool operator==(const counts_t& c) const { return (m_references[strong] == c.m_references[strong]) && (m_references[weak] == c.m_references[weak]); }
		bool operator!=(const counts_t& c) const { return !operator==(c); }

		int compare(const counts_t& c) const
		{
			if (m_references[strong] > c.m_references[strong])
				return 1;
			if (m_references[strong] < c.m_references[strong])
				return -1;
			if (m_references[weak] > c.m_references[weak])
				return 1;
			if (m_references[weak] < c.m_references[weak])
				return -1;
			return 0;
		}

		bool operator<(const counts_t& c) const
		{
			if (m_references[strong] < c.m_references[strong])
				return true;
			if (m_references[strong] > c.m_references[strong])
				return false;
			if (m_references[weak] < c.m_references[weak])
				return true;
			return false;
		} 
		
		bool operator>(const counts_t& c) const { return c < *this; }

		bool operator<=(const counts_t& c) const { return !operator>(c); }
		bool operator>=(const counts_t& c) const { return !operator<(c); }
	};

	volatile alignas (atomic::get_alignment_v<counts_t>) counts_t m_counts;

	class link : public slink
	{
	public:
		rc_obj_base*	m_desc;

		link(rc_obj_base* desc)
			:	m_desc(desc)
		{ }
	};

	static placement<hazard> s_hazard;

#if COGS_DEBUG_LEAKED_REF_DETECTION
public:
	class tracking_header : public slink_t<tracking_header>//, public rbtree_node_t<tracking_header>
	{
	public:
		volatile boolean	m_destructed;
		const char*			m_debugStr;
		const char*			m_typeName;
		rc_obj_base*		m_desc;
		void*				m_objPtr;

		tracking_header(rc_obj_base* desc)
			:	m_desc(desc),
				m_destructed(false),
				m_debugStr(0),
				m_typeName(0),
				m_objPtr(0)
		{ }
	};

	void set_debug_str(const char* s)
	{
		m_tracker->m_debugStr = s;
#if COGS_DEBUG_RC_LOGGING
		m_debugStr = s;
#endif
	}
	void set_type_name(const char* s)
	{
		m_tracker->m_typeName = s;
#if COGS_DEBUG_RC_LOGGING
		m_typeName = s;
#endif
	}
	void set_obj_ptr(void* obj)		
	{
		m_tracker->m_objPtr = obj;
#if COGS_DEBUG_RC_LOGGING
		m_objPtr = obj;
#endif
	}

private:
	tracking_header* m_tracker;


	void install_tracker();

#if COGS_DEBUG_RC_LOGGING
	const char* m_typeName;
	const char* m_debugStr;
	void* m_objPtr;
#endif

public:

	static void log_active_references();

	rc_obj_base()
		: m_counts{ 1, 1 }
	{
		COGS_ASSERT(((size_t)&m_counts % atomic::get_alignment_v<counts_t>) == 0);

#if COGS_DEBUG_RC_LOGGING
		m_typeName = 0;
		m_debugStr = 0;
		m_objPtr = 0;
#endif
		install_tracker();
	}

	~rc_obj_base()
	{
		m_tracker->m_destructed = true;

#if COGS_DEBUG_RC_LOGGING
		if (!!m_objPtr)
			printf("RC_DELETE: %p (desc) %p (ptr) %s @ %s\n", this, m_objPtr, m_typeName, m_debugStr);
#endif
	}

#else
	
public:

#if COGS_DEBUG_RC_LOGGING
	const char* m_typeName;
	const char* m_debugStr;
	void* m_objPtr;

	void set_debug_str(const char* s)	{ m_debugStr = s; }
	void set_type_name(const char* s)	{ m_typeName = s; }
	void set_obj_ptr(void* obj)			{ m_objPtr = obj; }
#endif

	rc_obj_base()
		: m_counts{ 1, 1 }
	{
		COGS_ASSERT(((size_t)&m_counts % atomic::get_alignment_v<counts_t>) == 0);

#if COGS_DEBUG_RC_LOGGING
		m_typeName = 0;
		m_debugStr = 0;
		m_objPtr = 0;
#endif
	}

	~rc_obj_base()
	{
#if COGS_DEBUG_RC_LOGGING
		printf("RC_DELETE: %p %s @ %s\n", this, m_typeName, m_debugStr);
#endif
	}

#endif

	bool is_released() const
	{
		size_t tmp = atomic::load(m_counts[strong]);
		return !tmp;
	}

	bool is_stale() const
	{
		counts_t oldCounts;
		atomic::load(m_counts, oldCounts);
		return (oldCounts.m_references[strong] == 0) && (oldCounts.m_references[weak] == 0);
	}

	bool is_owned() const
	{
		counts_t oldCounts;
		atomic::load(m_counts, oldCounts);
		return (oldCounts.m_references[strong] == 1) && (oldCounts.m_references[weak] == 1);
	}

	counts_t get_counts()
	{
		counts_t n;
		atomic::load(m_counts, n);
		return n;
	}

	size_t get_strong_count()
	{
		size_t n;
		atomic::load(m_counts[strong], n);
		return n;
	}

	size_t get_weak_count()
	{
		size_t n;
		atomic::load(m_counts[weak], n);
		return n;
	}



	bool acquire_strong()	{ return acquire(strong); }
	bool acquire_weak()		{ return acquire(weak); }

	bool acquire(reference_strength_type refStrengthType = strong, size_t n = 1)
	{
		if (!n)
			return false;

		bool result = true;

		// This should NOT be optimized to only allow weak acquires if currently strongly acquired.
		// Weak re-acquires are necessary to support making compariable copies of weak references, and
		// to prevent ABA issues with those comparisons.

		size_t newCount;
		size_t oldCount = atomic::load(m_counts[refStrengthType]);
		do {
			COGS_ASSERT((refStrengthType == strong) || !!oldCount);
			if (!oldCount)
			{
				result = false;
				break;
			}
			newCount = oldCount + n;
		} while (!atomic::compare_exchange(m_counts[refStrengthType], newCount, oldCount, oldCount));
		return result;
	}

	static volatile hazard& get_hazard()	{ return s_hazard.get(); }

	template <class derived_t>
	static derived_t* guarded_acquire(derived_t* const volatile & srcDesc, reference_strength_type refStrengthType = strong, size_t n = 1)
	{
		if (!n)
			return 0;

		volatile hazard& h = get_hazard();
		hazard::pointer p;
		derived_t* oldDesc = atomic::load(srcDesc);
		for (;;)
		{
			if (!oldDesc)
				break;

			p.bind(h, oldDesc);
			derived_t* cmpDesc = atomic::load(srcDesc);
			if (oldDesc != cmpDesc)
			{
				oldDesc = cmpDesc;
				continue;
			}

			if (!p.validate())
			{
				oldDesc = atomic::load(srcDesc);
				continue;
			}

			bool acquired = oldDesc->acquire(refStrengthType, n);

			if (p.release())
			{
				rc_obj_base* obj = oldDesc;
				obj->dispose();
			}

			if (!acquired)
			{
				derived_t* cmpDesc = atomic::load(srcDesc);
				if (cmpDesc != oldDesc)
				{
					oldDesc = cmpDesc;
					continue;
				}

				// There is no reason for a failure to
				// acquire a weak reference, if srcDesc is still set to it.
				COGS_ASSERT(refStrengthType != weak);
				oldDesc = 0;
			}
			
			break;
		}

		COGS_ASSERT(!oldDesc || !oldDesc->is_stale());
		return oldDesc;
	}

	bool release_strong(size_t i = 1)
	{
		if (i > 0)
		{
			size_t oldCount = atomic::load(m_counts[strong]);
			do {
				COGS_ASSERT(oldCount >= i);
			} while (!atomic::compare_exchange(m_counts[strong], oldCount - i, oldCount, oldCount));
			if (oldCount == i)
			{
				released();
				release_weak();
				return true;
			}
		}
		return false;
	}

	bool release_weak(size_t i = 1)
	{
		if (i > 0)
		{
			size_t oldCount = atomic::load(m_counts[weak]);
			do {
				COGS_ASSERT(oldCount >= i);
			} while (!atomic::compare_exchange(m_counts[weak], oldCount - i, oldCount, oldCount));
			if (oldCount == i)
			{
				volatile hazard& h = get_hazard();
				if (h.release(this))
					dispose();
				return true;
			}
		}
		return false;
	}

	// Putting the comparison in its own function seems to encourage the compiler to optimize
	// it out completely.
	bool release(reference_strength_type refStrengthType = strong, size_t i = 1)
	{
		if (refStrengthType == strong)
			return release_strong(i);
		return release_weak(i);
	}

	// Can be useful if reusing a descriptor to reset it to the initialized stated.
	void reset_counts()
	{
		m_counts[strong] = 1;
		m_counts[weak] = 1;
	}

	virtual void released() = 0;
	virtual void dispose() = 0;
};


}

#endif

