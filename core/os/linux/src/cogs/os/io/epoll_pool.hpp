//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_EPOLL_POOL
#define COGS_EPOLL_POOL


#include <sys/epoll.h>

#include "cogs/env.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/function.hpp"
#include "cogs/io/auto_fd.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace os {
namespace io {

class epoll_pool : public object
{
private:
	static placement<rcptr<epoll_pool> >		s_defaultEPollPool;

	static void cleanup_globals()
	{
		volatile rcptr<epoll_pool>& defaultEPollPool = s_defaultEPollPool.get();
		defaultEPollPool = 0;
	}

	thread_pool		m_pool;
	rcref<auto_fd>	m_fd;

	// Need to keep track of listenings in a less than ideal mannger, since kqueue does not tell us,
	// in a race-condition-free manner, when listening sockets close.
	typedef map<int, function<void()> > map_t;

	class task
	{
	public:
		rcref<auto_fd>			m_fd;
		int						m_shutdownSocket[2];
		size_t					m_numThreadsLeft;
		volatile map_t			m_tasks;
		weak_rcptr<epoll_pool>	m_epollPool;

		task(const rcref<auto_fd>& fd, const rcref<epoll_pool>& epp)
			:	m_fd(fd),
				m_epollPool(epp)
		{
		}

		void run()
		{
			for (;;)
			{
				struct epoll_event ev;
				int i = epoll_wait(m_fd->m_fd, &ev, 1, -1);
				if ((i == -1) && (errno == EINTR))
					continue;
				COGS_ASSERT(i != -1);
				if (i > 0)
				{
					int fd = ev.data.fd;
					if (fd == m_fd->m_fd)	// must have been triggered by m_shutdownSocket
					{
						close(m_shutdownSocket[0]);
						close(m_shutdownSocket[1]);
						if (--m_numThreadsLeft > 0)	// if more threads waiting, signal another
						{
							i = pipe(m_shutdownSocket);	// Use a pipe to tell epoll threads to shut down
							COGS_ASSERT(i != -1);
							ev.events = EPOLLONESHOT | EPOLLOUT | EPOLLERR | EPOLLHUP;
							ev.data.fd = fd;
							i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_ADD, m_shutdownSocket[1], &ev);
							COGS_ASSERT(i != -1);
						}
						break;
					}
					map_t::volatile_iterator itor = m_tasks.find(fd);
					if (!!itor)
					{
						if (m_tasks.remove(itor))
						{
							rcptr<epoll_pool> epp = m_epollPool;
							COGS_ASSERT(!!epp);
							epp->self_release();
						}
						(*itor)();
					}
				}
			}
		}
	};

	rcptr<task> m_func;

	epoll_pool()
		: m_fd(rcnew(auto_fd)),
		m_pool(false)
	{
		int fd = epoll_create1(0);
		m_fd->m_fd = fd;
		m_func = rcnew(task, m_fd, this_rcref);
	}

	void start()
	{
		m_pool.start();
		size_t numThreads = m_pool.get_num_threads();
		m_func->m_numThreadsLeft = numThreads;
		m_pool.dispatch_parallel(numThreads, [r{ m_func.dereference() }]()
		{
			r->run();
		});
	}

public:
	~epoll_pool()
	{
		int i = pipe(m_func->m_shutdownSocket);	// Use a pipe to tell epoll threads to shut down
		COGS_ASSERT(i != -1);
		struct epoll_event ev;
		ev.events = EPOLLONESHOT | EPOLLOUT | EPOLLERR | EPOLLHUP;
		ev.data.fd = m_fd->m_fd;
		i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_ADD, m_func->m_shutdownSocket[1], &ev);
		COGS_ASSERT(i != -1);
		m_pool.shutdown();
		m_pool.join();
	}

	static rcref<epoll_pool> get()
	{
		volatile rcptr<epoll_pool>& defaultEPollPool = s_defaultEPollPool.get();
		rcptr<epoll_pool> oldDefaultEPollPool = defaultEPollPool;
		if (!oldDefaultEPollPool)
		{
			rcptr<epoll_pool> newDefaultEPollPool = rcnew(epoll_pool);
			if (defaultEPollPool.compare_exchange(newDefaultEPollPool, oldDefaultEPollPool, oldDefaultEPollPool))
			{
				newDefaultEPollPool->start();
				oldDefaultEPollPool = newDefaultEPollPool;		// Return the one we just created.
				cleanup_queue::get_default()->dispatch(&cleanup_globals);
			}
		}
		return oldDefaultEPollPool.dereference();
	}

	void register_fd(int fd)
	{
		struct epoll_event ev;
		ev.events = EPOLLERR | EPOLLHUP | EPOLLET;
		ev.data.fd = fd;
		int i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_ADD, fd, &ev);
		COGS_ASSERT(i != -1);
	}

	class remove_token
	{
	protected:
		friend class epoll_pool;

		map_t::volatile_remove_token m_removeToken;
		int m_fd;

		remove_token(int fd, const map_t::volatile_remove_token& rt)
			: m_fd(fd),
			m_removeToken(rt)
		{
		}

	public:
		remove_token()											{ }
		remove_token(const remove_token& rt) : m_fd(rt.m_fd), m_removeToken(rt.m_removeToken)	{ }
		remove_token& operator=(const remove_token& rt)			{ m_fd = rt.m_fd;  m_removeToken = rt.m_removeToken; return *this; }
		bool is_active() const									{ return m_removeToken.is_active(); }
		void release()											{ m_removeToken.release(); }
		bool operator!() const									{ return !m_removeToken; }
		bool operator==(const remove_token& rt) const			{ return m_removeToken == rt.m_removeToken; }
		bool operator!=(const remove_token& rt) const			{ return !operator==(rt); }
	};


	remove_token wait_writable(int fd, const function<void()>& d)
	{
		self_acquire();
		struct epoll_event ev;
		ev.events = EPOLLONESHOT | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET;
		ev.data.fd = fd;
		map_t::volatile_iterator itor = m_func->m_tasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_MOD, fd, &ev);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}

	remove_token wait_readable(int fd, const function<void()>& d)
	{
		self_acquire();
		struct epoll_event ev;
		ev.events = EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
		ev.data.fd = fd;
		map_t::volatile_iterator itor = m_func->m_tasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_MOD, fd, &ev);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}
	
	void abort_waiter(const remove_token& rt)
	{
		if (m_func->m_tasks.remove(rt.m_removeToken))
		{
			self_release();
			struct epoll_event ev;
			ev.events = EPOLLERR | EPOLLHUP | EPOLLET;
			int i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_MOD, rt.m_fd, &ev);
			COGS_ASSERT(i != -1);
		}
	}

	remove_token register_listener(int fd, const function<void()>& d)
	{
		self_acquire();
		struct epoll_event ev;
		ev.events = EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
		ev.data.fd = fd;	// mark lsb to clue other thread in that this is a listener FD
		map_t::volatile_iterator itor = m_func->m_tasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = epoll_ctl(m_fd->m_fd, EPOLL_CTL_MOD, fd, &ev);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}

	void deregister_listener(const remove_token& rt)
	{
		if (m_func->m_tasks.remove(rt.m_removeToken))
			self_release();
	}
};


}
}
}


#endif


