//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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

class rc_object_base; // seems ambiguous, need better names for these different objects!  TBD


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
inline alignas (atomic::get_alignment_v<unsigned long>) volatile unsigned long g_rcLogCount;
#endif

#if COGS_DEBUG_ALLOC_LOGGING
inline alignas (atomic::get_alignment_v<unsigned long>) volatile unsigned long g_allocLogCount;
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

	struct counts_t
	{
		// Destructed  when m_reference[strong] == 0
		// Deallocated when m_reference[weak] == 0
		alignas (atomic::get_alignment_v<size_t>) size_t m_references[2];

		volatile size_t& operator[](reference_strength_type refStrengthType) volatile { return m_references[refStrengthType]; }
		const volatile size_t& operator[](reference_strength_type refStrengthType) const volatile { return m_references[refStrengthType]; }

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

	alignas (atomic::get_alignment_v<counts_t>) counts_t m_counts;

	class released_handlers;
	released_handlers mutable* m_releasedHandlers;

	released_handlers* initialize_released_handlers() const;
	void run_released_handlers();

	class link : public slink
	{
	public:
		rc_obj_base* m_desc;

		link(rc_obj_base& desc)
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
		const char* m_debugStr = 0;
		const char* m_typeName = 0;
		rc_obj_base* m_desc;
		void* m_objPtr = 0;

		tracking_header(rc_obj_base& desc)
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
	tracking_header* m_tracker;

	inline static placement<no_aba_stack<rc_obj_base::tracking_header> > s_allocRecord;
	inline static placement<size_type> s_totalTrackers;

	void install_tracker()
	{
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

	static void log_active_references()
	{
		volatile no_aba_stack<rc_obj_base::tracking_header>& allocRecord = s_allocRecord.get();
		no_aba_stack<rc_obj_base::tracking_header> allocs;
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
				size_t strongReferences = desc->m_counts.m_references[1];
				if (strongReferences == 0)
					++numWeakLeaks;
				else
				{
					if (!started)
					{
						started = true;
						printf("RC LEAKS:\n");
						printf("Index|Strong|Weak|rc_obj_base*|ptr|Type|Location\n");
					}
					size_t weakReferences = desc->m_counts.m_references[0];
					printf("%zd|%zd|%zd|%p|%p|\"%s\"|\"%s\"\n", index, strongReferences, weakReferences, desc, tracker->m_objPtr, tracker->m_typeName, tracker->m_debugStr);

					++numStrongLeaks;
				}
			}
			--index;
		}

		printf("RC LEAKS: %d of %d RC object(s) leaked.\n", (int)numStrongLeaks, (int)numAllocations);
		if (numWeakLeaks > 0)
			printf("RC LEAKS: %d lingering weak reference(s).\n", (int)numWeakLeaks);
		COGS_ASSERT(numStrongLeaks == 0);
	}

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
		install_tracker();
	}

	~rc_obj_base()
	{
		default_allocator::destruct_deallocate_type(m_releasedHandlers);

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
		default_allocator::destruct_deallocate_type(m_releasedHandlers);

#if COGS_DEBUG_RC_LOGGING
		printf("RC_DELETE: %p %s @ %s\n", this, m_typeName, m_debugStr);
#endif
	}

#endif

	class released_handler_remove_token;

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

	bool acquire_strong() { return acquire(strong); }
	bool acquire_weak() { return acquire(weak); }

	bool acquire(reference_strength_type refStrengthType = strong, size_t n = 1)
	{
		bool result = true;
		size_t oldCount = atomic::load(m_counts[refStrengthType]);
		if (!n)
			result = oldCount != 0;
		else
		{
			// This should NOT be changed to disallow weak acquires if all strong references have been released.
			// These weak acquires are necessary to support making compariable copies of weak references, and
			// to prevent ABA issues with those comparisons.
			size_t newCount;
			do {
				COGS_ASSERT((refStrengthType == strong) || !!oldCount);
				if (!oldCount)
				{
					result = false;
					break;
				}
				newCount = oldCount + n;
			} while (!atomic::compare_exchange(m_counts[refStrengthType], newCount, oldCount, oldCount));
		}
		return result;
	}

	static volatile hazard& get_hazard() { return s_hazard.get(); }

	template <class derived_t>
	static derived_t* guarded_acquire(derived_t* const volatile& srcDesc, reference_strength_type refStrengthType = strong, size_t n = 1)
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
				run_released_handlers();
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

	bool release(reference_strength_type refStrengthType = strong, size_t i = 1)
	{
		if (refStrengthType == strong)
			return release_strong(i);
		return release_weak(i);
	}

	// Can be useful if reusing a descriptor to reset it to the initialized state.
	void reset_counts()
	{
		m_counts[strong] = 1;
		m_counts[weak] = 1;
	}

	virtual void released() = 0;
	virtual void dispose() = 0;

	template <typename F, typename enable = std::enable_if_t<std::is_invocable_v<F, rc_obj_base&> > >
	released_handler_remove_token on_released(F&& f) const;

	bool uninstall_released_handler(const released_handler_remove_token& removeToken) const;
};


#if COGS_USE_DEBUG_DEFAULT_ALLOCATOR

class debug_default_allocator : public default_allocator_t
{
public:

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
	volatile size_type m_numRecords;

	debug_default_allocator() { m_numRecords = 0; }

	~debug_default_allocator() { COGS_ASSERT(m_allocRecord.is_empty()); }
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	class footer
	{
	public:
		unsigned char checker[COGS_OVERFLOW_CHECK_SIZE];

		void check_overflow()
		{
			for (int i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
				COGS_ASSERT(checker[i] == COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE);
		}
	};
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT

	class header : public slink_t<header>
	{
	public:
#if COGS_DEBUG_ALLOC_BUFFER_DEINIT
		size_t m_usableBlockSize;
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		volatile int_type m_deallocated;
#endif

#if COGS_DEBUG_ALLOC_LOGGING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
		unsigned long m_allocIndex;
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING


		unsigned char checker[COGS_OVERFLOW_CHECK_SIZE];

		void check_overflow()
		{
			for (int i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
				COGS_ASSERT(checker[i] == COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE);
		}

		footer * m_footer;
#endif
	};

	static constexpr size_t header_size = least_multiple_of_v<sizeof(header), largest_alignment>;

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	static constexpr size_t overhead = header_size + least_multiple_of_v<sizeof(footer), largest_alignment>;
#else
	static constexpr size_t overhead = header_size;
#endif

	volatile no_aba_stack<header> m_allocRecord;
	volatile ptr<header> m_firstAdded;
	volatile ptr<header> m_lastAdded;

	virtual ptr<void> allocate(size_t n, size_t align) volatile
	{
#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		// Pad request size to proper alignment for footer, before adding footer size.
		// No additional usableSize is provided to the caller, to encourage overflow detection.
		size_t n_minus_one = n - 1;
		n = n_minus_one + (largest_alignment - (n_minus_one % largest_alignment));
#endif
		void* ptr = default_allocator_t::allocate(overhead + n, largest_alignment).get_ptr();
		COGS_ASSERT(((size_t)ptr % largest_alignment) == 0);

		header * hdr = (header*)ptr;

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		footer * ftr = (footer*)((unsigned char*)ptr + header_size + n);
		hdr->m_footer = ftr;
		for (int i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
		{
			ftr->checker[i] = COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE;
			hdr->checker[i] = COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE;
		}
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		hdr->m_deallocated = 0;
		if (m_allocRecord.push(hdr))
			m_firstAdded = hdr;
		m_lastAdded = hdr;
		++m_numRecords;
#endif

		ptr = (unsigned char*)ptr + header_size;

#if COGS_DEBUG_ALLOC_BUFFER_INIT || COGS_DEBUG_ALLOC_BUFFER_DEINIT

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		size_t usableSize = n;
#else
		size_t usableSize = default_allocator_t::get_allocation_size(hdr, largest_alignment, overhead + n) - overhead;
#endif

#if COGS_DEBUG_ALLOC_BUFFER_DEINIT
		hdr->m_usableBlockSize = usableSize;
#endif

#if COGS_DEBUG_ALLOC_BUFFER_INIT
		memset(ptr, COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE, usableSize);
#endif

#endif

#if COGS_DEBUG_ALLOC_LOGGING
		unsigned long allocCount = pre_assign_next(g_allocLogCount);
		printf("(%lu) ALLOC: %p\n", allocCount, ptr);

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		hdr->m_allocIndex = allocCount;
#endif

#endif
		return ptr;
	}

	virtual size_t get_allocation_size(const ptr<void>& p, size_t align, size_t knownSize) const volatile
	{
#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		return knownSize;
#else

		header* hdr = (header*)((unsigned char*)p.get_ptr() - header_size);
		return default_allocator_t::get_allocation_size(hdr, largest_alignment, overhead + knownSize) - overhead;
#endif
	}

#elif COGS_DEBUG_ALLOC_BUFFER_INIT || COGS_DEBUG_ALLOC_LOGGING

	virtual ptr<void> allocate(size_t n, size_t align) volatile
	{
		void* ptr = default_allocator_t::allocate(n, align).get_ptr();

#if COGS_DEBUG_ALLOC_BUFFER_INIT
		size_t usableSize = default_allocator_t::get_allocation_size(ptr, align, n);
		memset(ptr, COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE, usableSize);
#endif

#if COGS_DEBUG_ALLOC_LOGGING
		unsigned long allocCount = pre_assign_next(g_allocLogCount);
		printf("(%lu) ALLOC: %p\n", allocCount, ptr);
#endif

		return ptr;
	}

#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT || COGS_DEBUG_ALLOC_LOGGING

	virtual void deallocate(const ptr<void>& p) volatile
	{
		void* ptr = p.get_ptr();

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT
		header * hdr = (header*)((unsigned char*)ptr - header_size);

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		bool b = hdr->m_deallocated.compare_exchange(int_type(1), 0);
		COGS_ASSERT(b); // double-delete detection
#endif

#if COGS_DEBUG_ALLOC_BUFFER_DEINIT
		memset(ptr, COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE, hdr->m_usableBlockSize);
#endif
		ptr = hdr;
#endif

#if COGS_DEBUG_ALLOC_LOGGING
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		printf("(%lu) DEALLOC: %p\n", hdr->m_allocIndex, p.get_ptr());
#else
		printf("DEALLOC: %p\n", p.get_ptr());
#endif
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		hdr->check_overflow();
		hdr->m_footer->check_overflow();
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
#else
		default_allocator_t::deallocate(ptr);
#endif
	}
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT
	virtual bool try_reallocate(const ptr<void>& p, size_t newSize) volatile
	{
		return false;//(newSize <= get_allocation_size(p)); // disable try_reallocate() to make things simpler for debugging.
	}
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
	virtual void log_active_allocations() volatile
	{
		size_t numLeaks = 0;
		size_t numTotal = 0;
		for (ptr<header> tracker = m_allocRecord.peek(); !!tracker; tracker = (header*)tracker->get_next_link().get_ptr())
		{
#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
			tracker->check_overflow();
#endif
			++numTotal;
			if (tracker->m_deallocated == 0)
			{
				++numLeaks;
#if COGS_DEBUG_ALLOC_LOGGING
				printf("(%lu) MEM LEAK: %p\n", tracker->m_allocIndex, (unsigned char*)tracker.get_ptr() + header_size);
#else
				printf("MEM LEAK: %p\n", (unsigned char*)tracker.get_ptr() + header_size);
#endif

			}
		}
		printf("MEM LEAKS: %d of %d memory allocations leaked.\n", (int)numLeaks, (int)numTotal);
	}

	virtual void shutdown()
	{
		// We had been hording all blocks.  Abandon m_allocRecord and release the released blocks.
		printf("Attempting to deallocate all allocations...\n");
		no_aba_stack<header> allocs;
		allocs.swap(m_allocRecord);
		for (;;)
		{
			ptr<header> tracker = allocs.pop();
			if (!tracker)
				break;
			default_allocator_t::deallocate(tracker);
			--m_numRecords;
		}
		COGS_ASSERT(m_numRecords == 0);
		COGS_ASSERT(m_allocRecord.is_empty());
		printf("All allocations deallocated.\n");
	}
#endif

};


#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
inline void assert_not_deallocated(const ptr<void>& p)
{
	if (!!p)
	{
		void* ptr = p.get_ptr();
		debug_default_allocator::header* hdr = (debug_default_allocator::header*)((unsigned char*)ptr - debug_default_allocator::header_size);
		COGS_ASSERT(hdr->m_deallocated != 0);
	}
}
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
inline void assert_no_overflow(const ptr<void>& p)
{
	if (!!p)
	{
		void* ptr = p.get_ptr();
		debug_default_allocator::header* hdr = (debug_default_allocator::header*)((unsigned char*)ptr - debug_default_allocator::header_size);
		hdr->check_overflow();
		hdr->m_footer->check_overflow();
	}
}
#endif

#endif



inline allocator* default_allocator::create_default_allocator()
{
#if COGS_USE_DEBUG_DEFAULT_ALLOCATOR
	typedef debug_default_allocator default_allocator_type;
#else
	typedef default_allocator_t default_allocator_type;
#endif

	ptr<default_allocator_type> al = env::allocator::allocate(sizeof(default_allocator_type), std::alignment_of_v<default_allocator_type>).template reinterpret_cast_to<default_allocator_type>();
	new (al) default_allocator_type;
	return al.get_ptr();
}


inline void default_allocator::dispose_default_allocator(allocator * al)
{
#if COGS_USE_DEBUG_DEFAULT_ALLOCATOR
	typedef debug_default_allocator default_allocator_type;
#else
	typedef default_allocator_t default_allocator_type;
#endif

	ptr<default_allocator_type> al2 = static_cast<default_allocator_type*>(al);
	al2->default_allocator_type::~default_allocator_type();
	env::allocator::deallocate(al);
}


inline void default_allocator::shutdown()
{
#if COGS_USE_DEBUG_DEFAULT_ALLOCATOR
	typedef debug_default_allocator default_allocator_type;
#else
	typedef default_allocator_t default_allocator_type;
#endif

	volatile ptr<allocator>& defaultAllocator = s_defaultAllocator.get();
	default_allocator_type* al = (default_allocator_type*)defaultAllocator.get_ptr();
	if (!!al)
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		rc_obj_base::log_active_references();
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		al->log_active_allocations();
		al->shutdown();
#endif
		al->default_allocator_type::~default_allocator_type();
		env::allocator::deallocate(al);
	}
}



}

#endif

