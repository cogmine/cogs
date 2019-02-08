//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifdef COGS_COMPILE_SOURCE


#include "cogs/collections/abastack.hpp"
#include "cogs/env.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/sync/hazard.hpp"


namespace cogs {

placement<hazard> rc_obj_base::s_hazard;

#if COGS_DEBUG_LEAKED_REF_DETECTION


static placement<aba_stack<rc_obj_base::tracking_header> > s_allocRecord;
static placement<size_type> s_totalTrackers;

void rc_obj_base::install_tracker()
{
	m_tracker = (rc_obj_base::tracking_header*)malloc(sizeof(rc_obj_base::tracking_header));
	new (m_tracker) tracking_header(this);	// placement new

	volatile aba_stack<rc_obj_base::tracking_header>& allocRecord = s_allocRecord.get();
	allocRecord.push(m_tracker);

	volatile size_type& totalTrackers = s_totalTrackers.get();
	++totalTrackers;
}

void rc_obj_base::log_active_references()
{
	volatile aba_stack<rc_obj_base::tracking_header>& allocRecord = s_allocRecord.get();
	aba_stack<rc_obj_base::tracking_header> allocs;
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


#endif


}

#endif
