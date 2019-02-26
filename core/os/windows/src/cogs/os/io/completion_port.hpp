//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_COMPLETION_PORT
#define COGS_HEADER_IO_COMPLETION_PORT


#include "cogs/function.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/os/handle.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace os {
namespace io {


class completion_port : public object
{
public:
	class overlapped_t : public OVERLAPPED
	{
	protected:
		function<void()>	m_delegate;

	public:
		size_t		m_numTransferred;
		bool		m_success;

		explicit overlapped_t(const function<void()>& d)
			:	m_delegate(d)
		{
			clear();
		}

		explicit overlapped_t(const function<void()>& d, DWORD offset, DWORD offsetHigh)
			:	m_delegate(d)
		{
			set(offset, offsetHigh);
			Internal = 0;
			InternalHigh = 0;
		}

		void set(DWORD offset, DWORD offsetHigh)
		{
			hEvent = 0;
			Offset = offset;
			OffsetHigh = offsetHigh;
		}

		void clear()
		{
			set(0, 0);
			Internal = 0;
			InternalHigh = 0;
		}

		OVERLAPPED* get()	{ return this; }

		virtual void run()
		{
			m_delegate();
		}
	};
	
	class self_destructing_overlapped_t : public overlapped_t
	{
	public:
		self_destructing_overlapped_t(const function<void()>& r)
			:	overlapped_t(r)
		{ }

		virtual void run()
		{
			overlapped_t::run();
			default_allocator::destruct_deallocate_type(this);
		}
	};

private:
	thread_pool			m_pool;
	rcref<auto_HANDLE>	m_handle;

	class task
	{
	private:
		rcref<auto_HANDLE>	m_handle;

	public:
		task(const rcref<auto_HANDLE>& h)
			:	m_handle(h)
		{ }

		void run()
		{
			DWORD n;
			ULONG_PTR key;
			OVERLAPPED* ovrlppd;
			for (;;)
			{
				ovrlppd = 0;
				BOOL b = GetQueuedCompletionStatus(m_handle->get(), &n, &key, &ovrlppd, INFINITE);
				if (!ovrlppd)
					break;
				overlapped_t* o = static_cast<overlapped_t*>(ovrlppd);
				o->m_numTransferred = n;
				o->m_success = !!b;
				o->run();
			}
		}
	};

	void start()
	{
		m_pool.start();
		rcref<task> r = rcnew(task, m_handle);
		m_pool.dispatch_parallel(m_pool.get_num_threads(), [r{ std::move(r) }]()
		{
			r->run();
		});
	}

protected:
	explicit completion_port(const ptr<rc_obj_base>& desc)
		: object(desc),
		m_handle(rcnew(auto_HANDLE))
	{
		HANDLE h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		m_handle->set(h);
	}


public:
	~completion_port()
	{
		for (size_t n = m_pool.get_num_threads(); n != 0; n--)
			PostQueuedCompletionStatus(m_handle->get(), 0, 0, 0);
		m_pool.shutdown();
		m_pool.join();
	}

	static rcref<completion_port> get()
	{
		bool isNew;
		rcref<completion_port> result = singleton<completion_port>::get(isNew);
		if (isNew)
			result->start();
		return result;
	}

	void register_handle(HANDLE h)
	{
		if (h != INVALID_HANDLE_VALUE)
			CreateIoCompletionPort(h, m_handle->get(), 0, 0);
	}

	// Since in Win32, all overlapped IO is cancelled if the calling thread is canceled,
	// let's defer the issuing of all overlapped IO to the worker threads.
	void dispatch(const function<void()>& d) const volatile
	{
		self_destructing_overlapped_t* o = new (default_allocator::get()) self_destructing_overlapped_t(d);
		PostQueuedCompletionStatus(m_handle->get(), 0, 0, o->get());
	}
};


}
}
}


#endif
