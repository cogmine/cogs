//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_KQUEUE_POOL
#define COGS_KQUEUE_POOL


#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "cogs/env.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/io/auto_fd.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace os {
namespace io {

class kqueue_pool : public object
{
private:
	static placement<rcptr<kqueue_pool> >		s_defaultKqueuePool;

	static void cleanup_globals()
	{
		volatile rcptr<kqueue_pool>& defaultKqueuePool = s_defaultKqueuePool.get();
		defaultKqueuePool = 0;
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
		volatile map_t			m_writeTasks;
		volatile map_t			m_readOrListenTasks;
		weak_rcptr<kqueue_pool>	m_kqueuePool;

		task(const rcref<auto_fd>& fd, const rcref<kqueue_pool> kq)
			:	m_fd(fd),
				m_kqueuePool(kq)
		{ }

		void run()
		{
			for (;;)
			{
				struct kevent kevt;
				int i = kevent(m_fd->m_fd, NULL, 0, &kevt, 1, 0);
				COGS_ASSERT(i != -1);
				if (kevt.udata == (void*)-1)
				{
					close(m_shutdownSocket[0]);
					close(m_shutdownSocket[1]);
					if (--m_numThreadsLeft > 0)	// if more threads waiting, signal another
					{
						i = pipe(m_shutdownSocket);	// Use a pipe to tell kqueue threads to shut down
						COGS_ASSERT(i != -1);
						EV_SET(&kevt, m_shutdownSocket[1], EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, (void*)-1);
						i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
						COGS_ASSERT(i != -1);
					}
					break;
				}
				int fd = (int)kevt.ident;
				short filter = kevt.filter;
				volatile map_t* taskMap;
				if (filter == EVFILT_WRITE)
					taskMap = &m_writeTasks;
				else // if (filter == EVFILT_READ) ... or a listen operation
				{
					COGS_ASSERT(filter == EVFILT_READ);
					taskMap = &m_readOrListenTasks;
				}
				map_t::volatile_iterator itor = taskMap->find(fd);
				if (!!itor)
				{
					if (taskMap->remove(itor))
					{
						rcptr<kqueue_pool> kq = m_kqueuePool;
						COGS_ASSERT(!!kq);
						kq->self_release();
					}
					(*itor)();
				}
			}
		}
	};

	rcptr<task> m_func;

	kqueue_pool()
		: m_fd(rcnew(auto_fd)),
		m_pool(false)
	{
		int fd = kqueue();
		m_fd->m_fd = fd;
		m_func = rcnew(task, m_fd, this_rcref);
	}

	void start()
	{
		m_pool.start();
		size_t numThreads = m_pool.get_num_threads();
		m_func->m_numThreadsLeft = numThreads;
		m_pool.dispatch_parallel(numThreads, [r{ m_func.dereferenc() }]()
		{
			r->run();
		});
	}

public:
	class remove_token
	{
	protected:
		friend class kqueue_pool;

		map_t::volatile_remove_token m_removeToken;
		int m_fd;

		remove_token(int fd, const map_t::volatile_remove_token& rt)
			: m_fd(fd),
			m_removeToken(rt)
		{ }

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

	~kqueue_pool()
	{
		if (!!m_func)
		{
			struct kevent kevt;
			int i = pipe(&(m_func->m_shutdownSocket[0]));	// Use a pipe to tell kqueue threads to shut down
			COGS_ASSERT(i != -1);
			EV_SET(&kevt, m_func->m_shutdownSocket[1], EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, (void*)-1);
			i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
			COGS_ASSERT(i != -1);
			m_pool.shutdown();
			m_pool.join();
		}
	}

	static rcref<kqueue_pool> get()
	{
		volatile rcptr<kqueue_pool>& defaultKqueuePool = s_defaultKqueuePool.get();
		rcptr<kqueue_pool> myDefaultKqueuePool = defaultKqueuePool;
		if (!myDefaultKqueuePool)
		{
			rcptr<kqueue_pool> newKqueuePool = rcnew(kqueue_pool);
			if (defaultKqueuePool.compare_exchange(newKqueuePool, myDefaultKqueuePool, myDefaultKqueuePool))
			{
				newKqueuePool->start();
				myDefaultKqueuePool = newKqueuePool;
				cleanup_queue::get_default()->dispatch(&cleanup_globals);
			}
		}
		return myDefaultKqueuePool.dereference();
	}

	// size_t passed to callback indicates available buffer.  Will be 0 on connection close.
	remove_token wait_writable(int fd, const function<void()>& d)
	{
		self_acquire();
		struct kevent kevt;
		EV_SET(&kevt, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
		map_t::volatile_iterator itor = m_func->m_writeTasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}

	// size_t passed to callback indicates number of bytes available.  Will be 0 on connection close.
	remove_token wait_readable(int fd, const function<void()>& d, size_t minBytes = 1)	// size
	{
		self_acquire();
		struct kevent kevt;
		EV_SET(&kevt, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_LOWAT, minBytes, 0);
		map_t::volatile_iterator itor = m_func->m_readOrListenTasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}

	void abort_write_waiter(const remove_token& rt)
	{
		if (m_func->m_writeTasks.remove(rt.m_removeToken))
		{
			self_release();
			struct kevent kevt;
			EV_SET(&kevt, rt.m_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
			int i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
			COGS_ASSERT(i != -1);
		}
	}

	void abort_read_waiter(const remove_token& rt)
	{
		if (m_func->m_readOrListenTasks.remove(rt.m_removeToken))
		{
			self_release();
			struct kevent kevt;
			EV_SET(&kevt, rt.m_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
			int i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
			COGS_ASSERT(i != -1);
		}
	}

	// size_t passed to callback indicates number of bytes available.  Will be 0 on connection close.
	remove_token register_listener(int fd, const function<void()>& d)
	{
		self_acquire();
		struct kevent kevt;
		EV_SET(&kevt, fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
		map_t::volatile_iterator itor = m_func->m_readOrListenTasks.try_insert(fd, d);
		COGS_ASSERT(!!itor);	// shouldn't fail
		int i = kevent(m_fd->m_fd, &kevt, 1, 0, 0, 0);
		COGS_ASSERT(i != -1);
		remove_token result(fd, itor);
		return result;
	}

	void deregister_listener(const remove_token& rt)
	{
		if (m_func->m_readOrListenTasks.remove(rt.m_removeToken))
			self_release();
	}

};


}
}
}


#endif
