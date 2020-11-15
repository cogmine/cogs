//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_RC_OBJ_BASE
#define COGS_HEADER_MEM_RC_OBJ_BASE


#include <type_traits>

#include "cogs/collections/no_aba_stack.hpp"
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


/// @ingroup Mem
/// @brief The strength type of a reference.  Weak references are conditional to strong references remaining in scope.
///
/// Generally, when circular references may be present, strong references should
/// be used only to explicitly control the scope of an object, and weak references used
/// otherwise.
enum class reference_strength
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

	void deallocate_released_handlers();

	struct counts_t
	{
		// Destructed  when m_reference[reference_strength::strong] == 0
		// Deallocated when m_reference[reference_strength::weak] == 0
		alignas(atomic::get_alignment_v<size_t>) size_t m_references[2];

		volatile size_t& operator[](reference_strength referenceStrength) volatile { return m_references[(int)referenceStrength]; }
		const volatile size_t& operator[](reference_strength referenceStrength) const volatile { return m_references[(int)referenceStrength]; }

		bool operator==(const counts_t& c) const { return (m_references[(int)reference_strength::strong] == c.m_references[(int)reference_strength::strong]) && (m_references[(int)reference_strength::weak] == c.m_references[(int)reference_strength::weak]); }
		bool operator!=(const counts_t& c) const { return !operator==(c); }

		int compare(const counts_t& c) const
		{
			if (m_references[(int)reference_strength::strong] > c.m_references[(int)reference_strength::strong])
				return 1;
			if (m_references[(int)reference_strength::strong] < c.m_references[(int)reference_strength::strong])
				return -1;
			if (m_references[(int)reference_strength::weak] > c.m_references[(int)reference_strength::weak])
				return 1;
			if (m_references[(int)reference_strength::weak] < c.m_references[(int)reference_strength::weak])
				return -1;
			return 0;
		}

		bool operator<(const counts_t& c) const
		{
			if (m_references[(int)reference_strength::strong] < c.m_references[(int)reference_strength::strong])
				return true;
			if (m_references[(int)reference_strength::strong] > c.m_references[(int)reference_strength::strong])
				return false;
			if (m_references[(int)reference_strength::weak] < c.m_references[(int)reference_strength::weak])
				return true;
			return false;
		}

		bool operator>(const counts_t& c) const { return c < *this; }

		bool operator<=(const counts_t& c) const { return !operator>(c); }
		bool operator>=(const counts_t& c) const { return !operator<(c); }
	};

	alignas(atomic::get_alignment_v<counts_t>) counts_t m_counts;

	class released_handlers;
	released_handlers mutable* m_releasedHandlers;

	released_handlers* initialize_released_handlers() const;
	bool run_released_handlers();

	class link : public slink
	{
	public:
		rc_obj_base* m_desc;

		explicit link(rc_obj_base& desc)
			: m_desc(&desc)
		{ }
	};

	inline static placement<hazard> s_hazard;

#if COGS_DEBUG_LEAKED_REF_DETECTION
public:
	class tracking_header : public slink_t<tracking_header>
	{
	public:
		volatile boolean m_destructed;
		const char* m_debugStr = "<unknown>";
		const char* m_typeName = "<unknown>";
		rc_obj_base* m_desc;
		void* m_objPtr = 0;

		explicit tracking_header(rc_obj_base& desc)
			: m_desc(&desc)
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
	tracking_header* m_tracker = nullptr;

	inline static placement<no_aba_stack<rc_obj_base::tracking_header> > s_allocRecord;
	inline static placement<size_type> s_totalTrackers;

	void install_tracker()
	{
		COGS_ASSERT(m_tracker == nullptr);
		m_tracker = (rc_obj_base::tracking_header*)malloc(sizeof(rc_obj_base::tracking_header));
		new (m_tracker) tracking_header(*this); // placement new

		volatile no_aba_stack<rc_obj_base::tracking_header>& allocRecord = s_allocRecord.get();
		allocRecord.push(m_tracker);

		volatile size_type& totalTrackers = s_totalTrackers.get();
		++totalTrackers;
	}

#if COGS_DEBUG_RC_LOGGING
	const char* m_typeName;
	const char* m_debugStr;
	void* m_objPtr;
#endif

public:

	static void shutdown()
	{
		volatile no_aba_stack<rc_obj_base::tracking_header>& allocRecord = s_allocRecord.get();
		no_aba_stack<rc_obj_base::tracking_header> allocs;
		no_aba_stack<rc_obj_base::tracking_header> weakAllocs;
		allocRecord.swap(allocs);
		size_t numStrongLeaks = 0;
		size_t numWeakLeaks = 0;

		ptr<rc_obj_base::tracking_header> tracker;

		size_t numAllocations = allocs.count();
		size_t index = numAllocations;

		// strip out all blocks successfully deallocated.  Sort the rest
		bool started = false;
		for (;;)
		{
			tracker = allocs.pop();
			if (!tracker)
				break;
			if (!tracker->m_destructed)
			{
				rc_obj_base* desc = tracker->m_desc;
				size_t strongReferences = desc->m_counts.m_references[(int)reference_strength::strong];
				if (strongReferences == 0)
				{
					weakAllocs.push(tracker);
					++numWeakLeaks;
				}
				else
				{
					if (!started)
					{
						started = true;
						printf("RC LEAKS:\n");
						printf("Index|Strong|Weak|rc_obj_base*|ptr|Type|Location\n");
					}
					size_t weakReferences = desc->m_counts.m_references[(int)reference_strength::weak];
					printf("%zd|%zd|%zd|%p|%p|\"%s\"|\"%s\"\n", index, strongReferences, weakReferences, desc, tracker->m_objPtr, tracker->m_typeName, tracker->m_debugStr);

					++numStrongLeaks;
				}
			}
			--index;
		}

#if COGS_DEBUG_LEAKED_WEAK_REF_DETECTION
		if (numWeakLeaks > 0)
		{
			if (started)
				printf("\n");
			else
			{
				printf("RC LEAKS:\n");
				printf("Index|Strong|Weak|rc_obj_base*|ptr|Type|Location\n");
			}
			for (;;)
			{
				tracker = weakAllocs.pop();
				if (!tracker)
					break;
				rc_obj_base* desc = tracker->m_desc;
				constexpr size_t strongReferences = 0;
				size_t weakReferences = desc->m_counts.m_references[(int)reference_strength::weak];
				printf("%zd|%zd|%zd|%p|%p|\"%s\"|\"%s\"\n", index, strongReferences, weakReferences, desc, tracker->m_objPtr, tracker->m_typeName, tracker->m_debugStr);
			}
		}
#endif

		if (numStrongLeaks > 0 || numWeakLeaks != 0)
			printf("RC LEAKS: %d of %d RC allocation(s) leaked. %d stong, %d weak.\n", (int)(numStrongLeaks + numWeakLeaks), (int)numAllocations, (int)numStrongLeaks, (int)numWeakLeaks);
		COGS_ASSERT(numStrongLeaks == 0 && numWeakLeaks == 0);
	}

	rc_obj_base()
		: m_counts{ 1, 1 },
		m_releasedHandlers(0)
	{
		COGS_ASSERT(((size_t)&m_counts % atomic::get_alignment_v<counts_t>) == 0);

#if COGS_DEBUG_RC_LOGGING
		m_typeName = "<unknown>";
		m_debugStr = "<unknown>";
		m_objPtr = 0;
#endif
		install_tracker();
	}

	~rc_obj_base()
	{
		deallocate_released_handlers();

		m_tracker->m_destructed = true;

#if COGS_DEBUG_RC_LOGGING
		if (!!m_objPtr)
			printf("RC_DELETE: %p (desc) %p (ptr) %s @ %s\n", this, m_objPtr, m_typeName, m_debugStr);
#endif
	}

#else

public:
	static void shutdown() {}


#if COGS_DEBUG_RC_LOGGING
	const char* m_typeName;
	const char* m_debugStr;
	void* m_objPtr;

	void set_debug_str(const char* s) { m_debugStr = s; }
	void set_type_name(const char* s) { m_typeName = s; }
	void set_obj_ptr(void* obj) { m_objPtr = obj; }
#endif

	rc_obj_base()
		: m_counts{ 1, 1 },
		m_releasedHandlers(0)
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
		deallocate_released_handlers();

#if COGS_DEBUG_RC_LOGGING
		printf("RC_DELETE: %p %s @ %s\n", this, m_typeName, m_debugStr);
#endif
	}

#endif

	class released_handler_remove_token;

	bool is_released() const
	{
		size_t tmp = atomic::load(m_counts[reference_strength::strong]);
		return !tmp;
	}

	bool is_stale() const
	{
		counts_t oldCounts;
		atomic::load(m_counts, oldCounts);
		return (oldCounts.m_references[(int)reference_strength::strong] == 0) && (oldCounts.m_references[(int)reference_strength::weak] == 0);
	}

	bool is_owned() const
	{
		counts_t oldCounts;
		atomic::load(m_counts, oldCounts);
		return (oldCounts.m_references[(int)reference_strength::strong] == 1) && (oldCounts.m_references[(int)reference_strength::weak] == 1);
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
		atomic::load(m_counts[reference_strength::strong], n);
		return n;
	}

	size_t get_weak_count()
	{
		size_t n;
		atomic::load(m_counts[reference_strength::weak], n);
		return n;
	}

	bool acquire_strong() { return acquire(reference_strength::strong); }
	bool acquire_weak() { return acquire(reference_strength::weak); }

	bool acquire(reference_strength referenceStrength = reference_strength::strong, size_t n = 1)
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		COGS_ASSERT(m_tracker->m_objPtr != nullptr);
#endif
		bool result = true;
		size_t oldCount = atomic::load(m_counts[referenceStrength]);
		if (!n)
			result = oldCount != 0;
		else
		{
			// This should NOT be changed to disallow weak acquires if all strong references have been released.
			// These weak acquires are necessary to support making compariable copies of weak references, and
			// to prevent ABA issues with those comparisons.
			size_t newCount;
			do {
				COGS_ASSERT((referenceStrength == reference_strength::strong) || !!oldCount);
				if (!oldCount)
				{
					result = false;
					break;
				}
				newCount = oldCount + n;
			} while (!atomic::compare_exchange(m_counts[referenceStrength], newCount, oldCount, oldCount));
		}
		return result;
	}

	static volatile hazard& get_hazard() { return s_hazard.get(); }

	template <class derived_t>
	static derived_t* guarded_acquire(derived_t* const volatile& srcDesc, reference_strength referenceStrength = reference_strength::strong, size_t n = 1)
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

			p.bind_unacquired(h, oldDesc);
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

			bool acquired = oldDesc->acquire(referenceStrength, n);

			if (p.release())
			{
				rc_obj_base* obj = oldDesc;
				obj->dispose();
			}

			if (!acquired)
			{
				cmpDesc = atomic::load(srcDesc);
				if (cmpDesc != oldDesc)
				{
					oldDesc = cmpDesc;
					continue;
				}

				// There is no reason for a failure to
				// acquire a weak reference, if srcDesc is still set to it.
				COGS_ASSERT(referenceStrength != reference_strength::weak);
				oldDesc = 0;
			}

			break;
		}

		COGS_ASSERT(!oldDesc || !oldDesc->is_stale());
		return oldDesc;
	}

	bool release_strong(size_t i = 1)
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		COGS_ASSERT(m_tracker->m_objPtr != nullptr);
#endif
		if (i > 0)
		{
			size_t oldCount = atomic::load(m_counts[reference_strength::strong]);
			do {
				COGS_ASSERT(oldCount >= i);
			} while (!atomic::compare_exchange(m_counts[reference_strength::strong], oldCount - i, oldCount, oldCount));
			if (oldCount == i)
			{
				if (run_released_handlers())
				{
					released();
					release_weak();
				}
				return true;
			}
		}
		return false;
	}

	bool release_weak(size_t i = 1)
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		COGS_ASSERT(m_tracker->m_objPtr != nullptr);
#endif
		if (i > 0)
		{
			size_t oldCount = atomic::load(m_counts[reference_strength::weak]);
			do {
				COGS_ASSERT(oldCount >= i);
			} while (!atomic::compare_exchange(m_counts[reference_strength::weak], oldCount - i, oldCount, oldCount));
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

	bool release(reference_strength referenceStrength = reference_strength::strong, size_t i = 1)
	{
		if (referenceStrength == reference_strength::strong)
			return release_strong(i);
		return release_weak(i);
	}

	// Can be useful if reusing a descriptor to reset it to the initialized state.
	void reset_counts()
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		COGS_ASSERT(m_tracker->m_objPtr != nullptr);
#endif
		m_counts[reference_strength::strong] = 1;
		m_counts[reference_strength::weak] = 1;
	}

	virtual void released() = 0;
	virtual void dispose() = 0;

	virtual bool contains(void* obj) const = 0;

	template <typename F, typename enable = std::enable_if_t<std::is_invocable_r_v<bool, F, rc_obj_base&, bool> > >
	released_handler_remove_token on_released(F&& f) const;

	bool uninstall_released_handler(const released_handler_remove_token& removeToken) const;
};


}


#include "cogs/mem/rc_obj.hpp"


#endif
