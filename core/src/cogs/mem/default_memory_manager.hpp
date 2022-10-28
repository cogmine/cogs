//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_DEFAULT_MEMORY_MANAGER
#define COGS_HEADER_MEM_DEFAULT_MEMORY_MANAGER


#include "cogs/collections/no_aba_stack.hpp"
#include "cogs/debug.hpp"
#include "cogs/env.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/mem/bballoc.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
void assert_not_deallocated(void* p);
#else
inline void assert_not_deallocated(void*) { }
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
void assert_no_overflow(void* p);
#else
inline void assert_no_overflow(void*) { }
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
void assert_no_overflows();
#else
inline void assert_no_overflows() { }
#endif

#if COGS_DEBUG_RC_LOGGING || COGS_DEBUG_ALLOC_LOGGING || (COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION)
inline volatile unsigned long g_allocLogCount alignas(atomic::get_alignment_v<unsigned long>);
#endif

#if COGS_USE_DEBUG_DEFAULT_ALLOCATOR
#ifdef COGS_USE_NATIVE_MEMORY_MANAGER
typedef env::memory_manager default_memory_manager_inner;
#else
typedef buddy_block_memory_manager<sizeof(void*), 1024 * 1024 * 4> default_memory_manager_inner;
#endif

class debug_default_memory_manager : public default_memory_manager_inner
{
public:

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
	volatile size_type m_recordCount;

	debug_default_memory_manager() { m_recordCount = 0; }

	~debug_default_memory_manager() { COGS_ASSERT(m_allocRecord.is_empty()); }
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	class footer
	{
	public:
		unsigned char checker[COGS_OVERFLOW_CHECK_SIZE];

		bool check_overflow()
		{
			for (size_t i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
			{
				if (checker[i] != COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE)
					return false;
			}
			return true;
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

#if (COGS_DEBUG_ALLOC_LOGGING && COGS_DEBUG_LEAKED_BLOCK_DETECTION) || (COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION)
		unsigned long m_allocIndex;
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		footer* m_footer;
		unsigned char checker[COGS_OVERFLOW_CHECK_SIZE];

		void check_overflow()
		{
			for (size_t i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
			{
				if (checker[i] != COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE)
				{
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
					printf("(%lu) MEM HEADER OVERWRITE: %p\n", m_allocIndex, (unsigned char*)this + header_size);
#else
					printf("MEM HEADER OVERWRITE: %p\n", (unsigned char*)this + header_size);
#endif
					COGS_ASSERT(false);
				}
			}
			if (!m_footer->check_overflow())
			{
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
				printf("(%lu) MEM FOOTER OVERWRITE: %p\n", m_allocIndex, (unsigned char*)this + header_size);
#else
				printf("MEM FOOTER OVERWRITE: %p\n", (unsigned char*)this + header_size);
#endif
				COGS_ASSERT(false);
			}
		}
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

	void* allocate(size_t n, size_t, size_t* usableSize = nullptr) volatile
	{
#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		// Pad request size to proper alignment for footer, before adding footer size.
		// No additional usableSize is provided to the caller, to encourage overflow detection.
		size_t n_minus_one = n - 1;
		n = n_minus_one + (largest_alignment - (n_minus_one % largest_alignment));
#endif
		if (!!usableSize)
			*usableSize = n;

		void* p = default_memory_manager_inner::allocate(overhead + n, largest_alignment);
		COGS_ASSERT(((size_t)p % largest_alignment) == 0);

		header * hdr = (header*)p;

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		footer * ftr = (footer*)((unsigned char*)p + header_size + n);
		hdr->m_footer = ftr;
		for (size_t i = 0; i < COGS_OVERFLOW_CHECK_SIZE; i++)
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
		++m_recordCount;
#endif

		p = (unsigned char*)p + header_size;

#if COGS_DEBUG_ALLOC_BUFFER_DEINIT
		hdr->m_usableBlockSize = n;
#endif

#if COGS_DEBUG_ALLOC_BUFFER_INIT
		memset(p, COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE, n);
#endif

#if COGS_DEBUG_ALLOC_LOGGING || (COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION)
		unsigned long allocCount = pre_assign_next(g_allocLogCount);
#endif

#if COGS_DEBUG_ALLOC_LOGGING
		printf("(%lu) ALLOC: %p\n", allocCount, p);
#endif

#if (COGS_DEBUG_ALLOC_LOGGING && COGS_DEBUG_LEAKED_BLOCK_DETECTION) || (COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION)
		hdr->m_allocIndex = allocCount;
#endif

		return p;
	}

#elif COGS_DEBUG_ALLOC_BUFFER_INIT || COGS_DEBUG_ALLOC_LOGGING

	void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr) volatile
	{
#if COGS_DEBUG_ALLOC_BUFFER_INIT
		size_t usableSize2;
		size_t* usableSize3 = (usableSize != nullptr) ? usableSize : &usableSize2;
		void* p = default_memory_manager_inner::allocate(n, align, usableSize3);
		memset(p, COGS_DEBUG_ALLOC_BUFFER_INIT_VALUE, *usableSize3);
#else
		void* p = default_memory_manager_inner::allocate(n, align, usableSize);
#endif

#if COGS_DEBUG_ALLOC_LOGGING || (COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION)
		unsigned long allocCount = pre_assign_next(g_allocLogCount);
#endif

#if COGS_DEBUG_ALLOC_LOGGING
		printf("(%lu) ALLOC: %p\n", allocCount, p);
#endif

		return p;
	}

#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT || COGS_DEBUG_ALLOC_LOGGING

	void deallocate(void* p) volatile
	{
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT
		header * hdr = (header*)((unsigned char*)p - header_size);

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		bool b = hdr->m_deallocated.compare_exchange(int_type(1), 0);
		COGS_ASSERT(b); // double-delete detection
#endif

#if COGS_DEBUG_ALLOC_LOGGING
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		printf("(%lu) DEALLOC: %p\n", hdr->m_allocIndex, p);
#else
		printf("DEALLOC: %p\n", p);
#endif
#endif

#if COGS_DEBUG_ALLOC_BUFFER_DEINIT
		memset(p, COGS_DEBUG_ALLOC_BUFFER_DEINIT_VALUE, hdr->m_usableBlockSize);
#endif
		p = hdr;
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		hdr->check_overflow();
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
#else
		default_memory_manager_inner::deallocate(p);
#endif
	}
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION || COGS_DEBUG_ALLOC_OVERFLOW_CHECKING || COGS_DEBUG_ALLOC_BUFFER_DEINIT
	bool try_reallocate(void*, size_t, size_t = cogs::largest_alignment, size_t* = nullptr) volatile
	{
		return false; // disable try_reallocate() to make things simpler for debugging.
	}
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
	void log_active_allocations() volatile
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
		COGS_ASSERT(numLeaks == 0);
	}

	void shutdown() volatile
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
			default_memory_manager_inner::deallocate(tracker.get_ptr());
			--m_recordCount;
		}
		COGS_ASSERT(m_recordCount == 0);
		COGS_ASSERT(m_allocRecord.is_empty());
		printf("All allocations deallocated.\n");
	}
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	void assert_no_overflows()
	{
		for (ptr<header> tracker = m_allocRecord.peek(); !!tracker; tracker = (header*)tracker->get_next_link().get_ptr())
		{
			tracker->check_overflow();
		}
	}
#endif
};


typedef debug_default_memory_manager default_memory_manager_impl;
#else
#ifdef COGS_USE_NATIVE_MEMORY_MANAGER
typedef env::memory_manager default_memory_manager_impl;
#else
typedef buddy_block_memory_manager<sizeof(void*), 1024 * 1024 * 4> default_memory_manager_impl;
#endif
#endif


/// @ingroup Mem
/// @brief Provides access to the default memory management algorithm
class default_memory_manager : public memory_manager_base<default_memory_manager>
{
private:
	template <bool is_static, bool unused = true>
	struct inner
	{
		static void shutdown() {}

		template <typename default_memory_manager_impl_t = default_memory_manager_impl>
		static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr) { return default_memory_manager_impl_t::allocate(n, align, usableSize); }

		template <typename default_memory_manager_impl_t = default_memory_manager_impl>
		static void deallocate(void* p) { default_memory_manager_impl_t::deallocate(p); }

		template <typename default_memory_manager_impl_t = default_memory_manager_impl>
		static bool try_reallocate(void* p, size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
		{
			return default_memory_manager_impl_t::try_reallocate(p, n, align, usableSize);
		}

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
		static void assert_no_overflows() { }
#endif
	};

	template <bool unused>
	struct inner<false, unused>
	{
		inline static placement<ptr<default_memory_manager_impl> > s_defaultMemoryManager;

		static default_memory_manager_impl* create_default_memory_manager()
		{
			ptr<default_memory_manager_impl> al = reinterpret_cast<default_memory_manager_impl*>(env::memory_manager::allocate(sizeof(default_memory_manager_impl), alignof(default_memory_manager_impl)));
			new (al) default_memory_manager_impl;
			return al.get_ptr();
		}

		static void dispose_default_memory_manager(default_memory_manager_impl* al)
		{
			al->default_memory_manager_impl::~default_memory_manager_impl();
			env::memory_manager::deallocate(al);
		}

		static default_memory_manager_impl& get()
		{
			// The default memory manager will outlive all allocations.
			// It's not shut down until after all threads have exited, and main is about to return.
			// Note: We impose a rule that globals should not be used to store objects
			// that outlive the memory manager.  Globals and static data should be placed in placement
			// blocks and either be valid when zero-initialized or handled with an atomic lazy-initializer.
			// if cleanup is necessary, the cleanup API should be used.

			volatile ptr<default_memory_manager_impl>& defaultMemoryManager = s_defaultMemoryManager.get();
			ptr<default_memory_manager_impl> al = defaultMemoryManager;
			if (!al)
			{
				default_memory_manager_impl* newAllocator = create_default_memory_manager();
				if (defaultMemoryManager.compare_exchange(newAllocator, al, al))
					al = newAllocator;
				else
					dispose_default_memory_manager(al.get_ptr());
			}
			return *al;
		}

		static void shutdown()
		{
			volatile ptr<default_memory_manager_impl>& defaultMemoryManager = s_defaultMemoryManager.get();
			default_memory_manager_impl* al = (default_memory_manager_impl*)defaultMemoryManager.get_ptr();
			if (!!al)
			{
#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
				al->log_active_allocations();
				al->shutdown();
#endif
				al->default_memory_manager_impl::~default_memory_manager_impl();
				env::memory_manager::deallocate(al);
			}
		}

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
		static void assert_no_overflows()
		{
			volatile ptr<default_memory_manager_impl>& defaultMemoryManager = s_defaultMemoryManager.get();
			default_memory_manager_impl* al = (default_memory_manager_impl*)defaultMemoryManager.get_ptr();
			if (!!al)
				al->assert_no_overflows();
		}
#endif

		static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
		{ return get().allocate(n, align, usableSize); }

		static void deallocate(void* p) { get().deallocate(p); }

		static bool try_reallocate(void* p, size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
		{
			return get().try_reallocate(p, n, align, usableSize);
		}
	};

	typedef inner<default_memory_manager_impl::is_static> inner_t;

public:
	static void shutdown()
	{
		inner_t::shutdown();
	}

	static void* allocate(size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
	{
		return inner_t::allocate(n, align, usableSize);
	}

	static void deallocate(void* p)
	{
		inner_t::deallocate(p);
	}

	static bool try_reallocate(void* p, size_t n, size_t align = cogs::largest_alignment, size_t* usableSize = nullptr)
	{
		return inner_t::try_reallocate(p, n, align, usableSize);
	}

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
	static void assert_no_overflows()
	{
		inner_t::assert_no_overflows();
	}
#endif
};


#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
inline void assert_not_deallocated(void* p)
{
	if (!!p)
	{
		debug_default_memory_manager::header* hdr = (debug_default_memory_manager::header*)((unsigned char*)p - debug_default_memory_manager::header_size);
		COGS_ASSERT(hdr->m_deallocated != 0);
	}
}
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
inline void assert_no_overflow(void* p)
{
	if (!!p)
	{
		debug_default_memory_manager::header* hdr = (debug_default_memory_manager::header*)((unsigned char*)p - debug_default_memory_manager::header_size);
		hdr->check_overflow();
	}
}
#endif


#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING && COGS_DEBUG_LEAKED_BLOCK_DETECTION
inline void assert_no_overflows()
{
	default_memory_manager::assert_no_overflows();
}
#endif


}


#endif
