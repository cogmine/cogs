//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#ifdef COGS_COMPILE_SOURCE


// Status: Good


#include "cogs/function.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/math/fixed_integer_extended.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/collections/abastack.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/mem/bballoc.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/hazard.hpp"


namespace cogs {

placement<ptr<allocator> >				default_allocator::s_defaultAllocator;
placement<hazard::list_freelist_t>		hazard::s_listFreeList;				// zero-initialize as bss, leaked
placement<hazard::token_freelist_t>		hazard::s_tokenFreeList;			// zero-initialize as bss, leaked

typedef buddy_block_allocator< sizeof(void*), 1024 * 1024 * 4 > default_allocator_t; 


#if COGS_DEBUG_RC_LOGGING 
volatile alignas (atomic::get_alignment_v<unsigned long>) unsigned long cogs::g_rcLogCount = 0;
#endif 

#if COGS_DEBUG_ALLOC_LOGGING
volatile alignas (atomic::get_alignment_v<unsigned long>) unsigned long cogs::g_allocLogCount = 0;
#endif 


class default_allocator_derived_wrapper : public default_allocator_t
{
public:

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
	volatile size_type m_numRecords;

	default_allocator_derived_wrapper()		{ m_numRecords = 0; }

	~default_allocator_derived_wrapper()	{ COGS_ASSERT(m_allocRecord.is_empty()); }
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	class footer
	{
	public:
		unsigned char checker[OVERFLOW_CHECK_SIZE];

		void check_overflow()
		{
			for (int i = 0; i < OVERFLOW_CHECK_SIZE; i++)
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


		unsigned char checker[OVERFLOW_CHECK_SIZE];

		void check_overflow()
		{
			for (int i = 0; i < OVERFLOW_CHECK_SIZE; i++)
				COGS_ASSERT(checker[i] == COGS_DEBUG_ALLOC_OVERFLOW_CHECKING_VALUE);
		}

		footer* m_footer;
#endif
	};

	static const size_t header_size = least_multiple_of<sizeof(header), largest_alignment>::value;

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
	static const size_t overhead = header_size + least_multiple_of<sizeof(footer), largest_alignment>::value;
#else
	static const size_t overhead = header_size;
#endif

	volatile aba_stack<header>	m_allocRecord;
	volatile ptr<header>			m_firstAdded;
	volatile ptr<header>			m_lastAdded;
	
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

		header* hdr = (header*)ptr;

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
		footer* ftr =(footer*)((unsigned char*)ptr + header_size + n);
		hdr->m_footer = ftr;
		for (int i = 0; i < OVERFLOW_CHECK_SIZE; i++)
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
		header* hdr = (header*)((unsigned char*)ptr - header_size);

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		bool b = hdr->m_deallocated.compare_exchange(int_type(1), zero_t());
		COGS_ASSERT(b);	// double-delete detection
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
	virtual bool try_reallocate(const ptr<void>& p, size_t newSize)	volatile
	{
		return false;//(newSize <= get_allocation_size(p));	// disable try_reallocate() to make things simpler for debugging.
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
				printf("(%lu) MEM LEAK: %p\n", tracker->m_allocIndex, (unsigned char*)tracker.get_ptr() +header_size);
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
		aba_stack<header> allocs;
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
void assert_not_deallocated(const ptr<void>& p)
{
	if (!!p)
	{
		void* ptr = p.get_ptr();
		default_allocator_derived_wrapper::header* hdr = (default_allocator_derived_wrapper::header*)((unsigned char*)ptr - default_allocator_derived_wrapper::header_size);
		COGS_ASSERT(hdr->m_deallocated != 0);
	}
}
#endif

#if COGS_DEBUG_ALLOC_OVERFLOW_CHECKING
void assert_no_overflow(const ptr<void>& p)
{
	if (!!p)
	{
		void* ptr = p.get_ptr();
		default_allocator_derived_wrapper::header* hdr = (default_allocator_derived_wrapper::header*)((unsigned char*)ptr - default_allocator_derived_wrapper::header_size);
		hdr->check_overflow();
		hdr->m_footer->check_overflow();
	}
}
#endif


allocator* default_allocator::create_default_allocator()
{
	ptr<default_allocator_derived_wrapper> al = env::allocator::allocate(sizeof(default_allocator_derived_wrapper), std::alignment_of<default_allocator_derived_wrapper>::value).reinterpret_cast_to<default_allocator_derived_wrapper>();
	new (al) default_allocator_derived_wrapper;
	return al.get_ptr();
}


void default_allocator::dispose_default_allocator(allocator* al)
{
	ptr<default_allocator_derived_wrapper> al2 = static_cast<default_allocator_derived_wrapper*>(al);
	al2->~default_allocator_derived_wrapper();
	env::allocator::deallocate(al);
}


void default_allocator::shutdown()
{
	volatile ptr<allocator>& defaultAllocator = s_defaultAllocator.get();
	default_allocator_derived_wrapper* al = (default_allocator_derived_wrapper*)defaultAllocator.get_ptr();
	if (!!al)
	{
#if COGS_DEBUG_LEAKED_REF_DETECTION
		rc_obj_base::log_active_references();
#endif

#if COGS_DEBUG_LEAKED_BLOCK_DETECTION
		al->log_active_allocations();
		al->shutdown();
#endif
		al->~default_allocator_derived_wrapper();
		env::allocator::deallocate(al);
	}
}

}


#endif

