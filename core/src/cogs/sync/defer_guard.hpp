//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Obsolete

#ifndef COGS_HEADER_SYNC_DEFER_GUARD
#define COGS_HEADER_SYNC_DEFER_GUARD


#include "cogs/collections/abastack.hpp"
#include "cogs/collections/slink.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"


namespace cogs {


// A defer_guard defers work (such as the release of an object) until all locks on the
// guard have been released.  Once the last lock is released, all deferrables are returned
// to the caller to be processed.  If the lock must remain acquired until all deferrables
// are processed, use a serial_defer_guard instead.
//class defer_guard;


template <class link_t, class link_iterator = default_slink_iterator<link_t> >
class defer_guard_t
{
private:
	struct content_t
	{
	public:
		ptr<link_t> m_head;

		alignas (atomic::get_alignment_v<size_t>) size_t m_guardCount;

		content_t() { m_guardCount = 0; }
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

	void prepend(link_t& e)
	{
		link_iterator::set_next(e, m_contents.m_head);
		m_contents.m_head = &e;
	}

	// No ABA problem since the only removal is wholesale.
	void prepend(link_t& e) volatile
	{
		ptr<link_t> ePtr = &e;
		volatile link_t* vol = &e;
		ptr<link_t> oldHead;
		atomic::load(m_contents.m_head, oldHead);
		do {
			link_iterator::set_next(*vol, oldHead.get_ptr());
		} while (!atomic::compare_exchange(m_contents.m_head, ePtr, oldHead, oldHead)); 
	}

public:
	defer_guard_t()
	{ }

	// If someone wants to add or process work, they need to first use begin_guard()
	bool begin_guard() { return !m_contents.m_guardCount++; }
	bool begin_guard() volatile { return !post_assign_next(m_contents.m_guardCount); }

	bool is_free() const { return !m_contents.m_guardCount; }
	bool is_free() const volatile { return !atomic::load(m_contents.m_guardCount); }

	// Anyone can 'add' work if the guard is acquired
	void add(link_t& sl) { prepend(sl); }
	void add(link_t& sl) volatile { prepend(sl); }

	link_t* release()
	{
		ptr<link_t> result = 0;
		if (!--(*(m_contents.m_guardCount)))
		{
			result = m_contents.m_head;
			m_contents.m_head = 0;
		}
		return result;
	}

	link_t* release() volatile
	{
		link_t* result;
		content_t oldContent;
		atomic::load(m_contents, oldContent);
		for (;;)
		{
			if (oldContent.m_guardCount == 1)
			{
				content_t newContent;
				if (atomic::compare_exchange(m_contents, newContent, oldContent, oldContent))
				{
					result = oldContent.m_head.get_ptr();
					break;
				}
				continue;
			}
			if (atomic::compare_exchange(m_contents.m_guardCount, oldContent.m_guardCount - 1, oldContent.m_guardCount, oldContent.m_guardCount))
			{
				result = 0;
				break;
			}
			atomic::load(m_contents, oldContent);
			//continue;
		}
		return result;
	}
};


typedef defer_guard_t<slink> defer_guard;


}


#endif
