//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_OS_IO_NET_IP_TCP
#define COGS_HEADER_OS_IO_NET_IP_TCP


#include "cogs/io/datastream.hpp"
#include "cogs/io/net/connection.hpp"
#include "cogs/os/io/net/ip/socket.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class tcp : public connection
{
private:
	const rcref<socket>	m_socket;

	class tcp_reader : public reader
	{
	private:
		const weak_rcptr<tcp>		m_tcp;
		const rcref<socket>			m_socket;
		buffer						m_currentBuffer;
		size_t						m_progress;
		size_t						m_adjustedRequestedSize;
		bool						m_aborted;
		bool						m_complete;
		os::io::epoll_pool::remove_token			m_waiterRemoveToken;

		enum task_type
		{
			read_task = 0,
			abort_task = 1
		};

		volatile container_queue<task_type>	m_completionSerializer;

		bool immediate_read(int s, size_t n)	// for now just returns false on failure, to trigger generic socket close.  More error handling later.
		{
			int i = recv(s, (char*)(m_currentBuffer.get_ptr()) + m_progress, (int)n, 0);
			if (i == -1)
			{
				if (errno == EAGAIN)
					return true;
				return false;
			}
			m_progress += i;
			return true;
		}

	public:
		tcp_reader(const ptr<rc_obj_base>& desc, const rcref<datasource>& proxy, const rcref<tcp>& t)
			:	reader(desc, proxy),
				m_tcp(t),
				m_socket(t->m_socket),
				m_progress(0),
				m_aborted(false),
				m_complete(false)
		{
			m_socket->get_dup_read_fd();
		}

		~tcp_reader()
		{ }
		
		virtual void reading()
		{
			m_adjustedRequestedSize = get_unread_size();
			m_currentBuffer = allocate_buffer(m_adjustedRequestedSize);
			read_more();
		}

		void read_more()					{ read_more_or_abort(read_task); }

		virtual void aborting()				{ read_more_or_abort(abort_task); }

		// Some socket implementations (Winsock, being one, not sure about sockets with epoll)
		// may incur significant overhead just checking how many bytes of data are available,
		// so it's more efficient to issue an arbitrary size read, and see how much is retrieved.
		// We go ahead and allocate a single buffer the size of the request, which is most efficient
		// if all data is available now, and only inefficient due to RAM use otherwise.  IO buffers
		// are usually very short-lived.
		void read_inner()
		{
			if (!!m_tcp && !!immediate_read(m_socket->m_dupReadFd.get(), m_adjustedRequestedSize - m_progress))
			{
				if ((m_progress != m_adjustedRequestedSize) && ((get_read_mode() != read_some) || !m_progress) && (get_read_mode() != read_now))
				{
					m_waiterRemoveToken = m_socket->m_epollPool->wait_readable(m_socket->m_dupReadFd.get(), [r{ this_weak_rcptr }]()
					{
						rcptr<tcp_reader> r2 = r;
						if (!!r2)
							r2->read_more();
					});
				}
				else
				{
					m_currentBuffer.truncate_to(m_progress);
					get_buffer().append(m_currentBuffer);
					m_currentBuffer.release();
					m_complete = true;
					complete();
				}
			}
			else
			{
				m_currentBuffer.truncate_to(m_progress);
				get_buffer().append(m_currentBuffer);
				m_currentBuffer.release();
				m_complete = true;
				complete(true);	// closes the read channel
			}
		}

		void read_more_or_abort(task_type taskType)
		{
			if (m_completionSerializer.append(taskType))
			{
				for (;;)
				{
					if (taskType == read_task)
					{
						if (!m_aborted)
							read_inner();
					}
					else // if (taskType == abort_task)
					{
						if (!m_complete)
						{
							m_aborted = true;
							m_socket->m_epollPool->abort_waiter(m_waiterRemoveToken);
							complete();
						}
					}
					bool wasLast;
					m_completionSerializer.remove_first(wasLast);
					if (wasLast)
						break;
					m_completionSerializer.peek_first(taskType);
				}
			}
		}
	};

	class tcp_writer : public writer
	{
	public:
		const weak_rcptr<tcp>		m_tcp;
		const rcref<socket>			m_socket;
		size_t						m_progress;
		bool						m_aborted;
		bool						m_complete;
		os::io::epoll_pool::remove_token	m_waiterRemoveToken;

		enum task_type
		{
			write_task = 0,
			abort_task = 1
		};

		volatile container_queue<task_type>	m_completionSerializer;

		tcp_writer(const ptr<rc_obj_base>& desc, const rcref<datasink>& proxy, const rcref<tcp>& t)
			:	writer(desc, proxy),
				m_tcp(t),
				m_socket(t->m_socket),
				m_progress(0),
				m_aborted(false),
				m_complete(false)
		{ }
		
		virtual void writing()	{ write_more(); }

		void write_more()		{ write_more_or_abort(write_task); }

		virtual void aborting()	{ write_more_or_abort(abort_task); }

		void write_more_or_abort(task_type taskType)
		{
			if (m_completionSerializer.append(taskType))
			{
				for (;;)
				{
					if (taskType == write_task)
					{
						if (!m_aborted)
							write_inner();
					}
					else // if (taskType == abort_task)
					{
						if (!m_complete)
						{
							m_aborted = true;
							m_socket->m_epollPool->abort_waiter(m_waiterRemoveToken);
							complete();
						}
					}
					bool wasLast;
					m_completionSerializer.remove_first(wasLast);
					if (wasLast)
						break;
					m_completionSerializer.peek_first(taskType);
				}
			}
		}

		void write_inner()
		{
			if (!m_tcp)
			{
				m_complete = true;
				complete(true);
			}
			else
			{
				for (;;)
				{
					if (!get_buffer().get_length())
					{
						m_complete = true;
						complete();
					}
					else
					{
						buffer b = get_buffer().get_inner(0);
						size_t sent = send(m_socket->m_fd.get(), (uint8_t*)(b.get_const_ptr()), b.get_length(), 0);
						if (sent == b.get_length())	// complete buffer, write again immediately
						{
							get_buffer().advance_array();
							continue;
						}
						if (sent > 0)
						{
							get_buffer().advance(sent);
							continue;	// Go ahead and try a write again immediately.  Maybe the underlying layer is breaking it up, but is ready now.
						}
						else
						{
							COGS_ASSERT(sent == -1);	// Don't think we'll ever get an actual zero here.
							if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
							{
								m_complete = true;
								complete(true);
								break;
							}
						}
						m_waiterRemoveToken = m_socket->m_epollPool->wait_writable(m_socket->m_fd.get(), [r{ this_weak_rcptr }]()
						{
							rcptr<tcp_writer> r2 = r;
							if (!!r2)
								r2->write_more();
						});
					}
					break;
				}
			}
		}
	};

	virtual rcref<reader> create_reader(const rcref<datasource>& proxy)
	{
		return rcnew(tcp_reader, proxy, this_rcref);
	}

	virtual rcref<writer> create_writer(const rcref<datasink>& proxy)
	{
		return rcnew(tcp_writer, proxy, this_rcref);
	}

protected:
	tcp(const ptr<rc_obj_base>& desc, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		: connection(desc),
		m_socket(rcnew(bypass_constructor_permission<socket>, SOCK_STREAM, IPPROTO_TCP, addressFamily, epp))
	{ }

	tcp(const ptr<rc_obj_base>& desc, int sckt, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
		: connection(desc),
		m_socket(rcnew(bypass_constructor_permission<socket>, sckt, SOCK_STREAM, IPPROTO_TCP, addressFamily, epp))
	{ }

public:
	~tcp()
	{
		abort();
	}

	class connecter : public signallable_task_base<connecter>
	{
	protected:
		rcref<os::io::epoll_pool>			m_epollPool;

		rcptr<tcp>							m_tcp;
		vector<address>						m_addresses;
		unsigned short						m_remotePort;
		os::io::epoll_pool::remove_token	m_waiterRemoveToken;
		bool								m_aborted;
		
		enum task_type
		{
			complete_task = 0,
			abort_task = 1
		};

		volatile container_queue<task_type> m_serializer;

		friend class tcp;

		connecter(const ptr<rc_obj_base>& desc, const vector<address>& addresses, unsigned short port, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
			: signallable_task_base<connecter>(desc),
			m_epollPool(epp),
			m_addresses(addresses),
			m_remotePort(port),
			m_aborted(false)
		{
			connect();
		}

		void connect()
		{
			for (;;)
			{
				if (m_addresses.is_empty())
				{
					m_tcp.release();	// failure to connect is indicated by complete connecter with null tcp object
					signal();
					self_release();
					break;
				}

				// Start with address first in the list.
				// If the connection fails, we'll remove it from the list.
				const address& addr = m_addresses[0];

				// Set port in address structure.
				sockaddr_in sa;
				memcpy(&sa, addr.get_sockaddr(), sizeof(sockaddr_in));
				sa.sin_port = htons(m_remotePort);

				// Allocate tcp obj to use
				m_tcp = rcnew(bypass_constructor_permission<tcp>, addr.get_address_family(), m_epollPool);

				// Bind local port to any address.
				int i = m_tcp->m_socket->bind_any();
				if (i != 0)	// bind failed?
				{
					m_tcp.release();	// failure to connect is indicated by complete connecter with null tcp object
					signal();
					self_release();
					break;
				}

				i = ::connect(m_tcp->m_socket->m_fd.get(), (sockaddr*)&sa, addr.get_sockaddr_size());
				if (i == 0)	// immediately connected??
				{
					signal();
					self_release();
					break;
				}
				COGS_ASSERT(i == -1);
				if (errno == EINPROGRESS)
				{
					m_waiterRemoveToken = m_epollPool->wait_writable(m_tcp->m_socket->m_fd.get(), [r{ this_weak_rcptr }]()
					{
						rcptr<connecter> r2 = r;
						if (!!r2)
							r2->complete();
					});
					break;
				}
				m_addresses.erase(0, 1);
				//continue;
			}
		}

		void complete_or_abort(task_type taskType)
		{
			if (m_serializer.append(taskType))
			{
				for (;;)
				{
					if (taskType == complete_task)
					{
						if (!m_aborted)
						{
							int errnum;
							socklen_t len = sizeof(errnum);
							int i = getsockopt(m_tcp->m_socket->m_fd.get(), SOL_SOCKET, SO_ERROR, &errnum, &len);
							COGS_ASSERT(i != -1);
							if (errnum == 0)	 //connected
							{
								m_tcp->m_socket->read_endpoints();
								signal();
								self_release();
							}
							else
							{
								m_addresses.erase(0, 1);
								connect();
							}
						}
					}
					else // if (taskType == abort_task)
					{
						if (!m_aborted)
						{
							m_aborted = true;
							m_epollPool->abort_waiter(m_waiterRemoveToken);
							m_tcp.release();	// failure to connect is indicated by complete connecter with null tcp object
							signal();
							self_release();
						}
					}
					bool wasLast;
					m_serializer.remove_first(wasLast);
					if (wasLast)
						break;
					m_serializer.peek_first(taskType);
				}
			}
		}

		void complete()	{ complete_or_abort(complete_task); }

		virtual const connecter& get() const volatile { return *(const connecter*)this; }

	public:
		~connecter() { cancel(); }

		const rcptr<tcp>& get_tcp() const { return m_tcp; }
		unsigned short get_remote_port() const { return m_remotePort; }
	
		virtual rcref<task<bool> > cancel() volatile
		{
			auto t = signallable_task_base<connecter>::cancel();	// will complete immediately
			if (t->get())
				((connecter*)this)->complete_or_abort(abort_task);
			return t;
		}
	};

	static rcref<connecter> connect(const vector<address>& addresses, unsigned short port, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
	{
		return rcnew(bypass_constructor_permission<connecter>, addresses, port, epp);
	}

	static rcref<connecter> connect(const address& addr, unsigned short port, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
	{
		vector<address> addresses;
		addresses.append(1, addr);
		return connect(addresses, port, epp);
	}

	virtual void abort()			{ datasource::abort_source(); datasink::abort_sink(); m_socket->close(); }
	virtual void abort_source()		{ datasource::abort_source(); m_socket->close_source(); }
	virtual void abort_sink()		{ datasink::abort_sink(); m_socket->close_sink(); }

	typedef function<void(const rcref<tcp>&)> accept_delegate_t;

	class listener : public object
	{
	private:
		class accept_helper : public object
		{
		public:
			rcref<os::io::epoll_pool>			m_epollPool;
			os::io::epoll_pool::remove_token	m_listenerRemoveToken;

			accept_delegate_t		m_acceptDelegate;
			address_family			m_addressFamily;

			weak_rcptr<listener>	m_listener;
			rcptr<tcp>				m_listenSocket;

			bool				m_closed;

			enum task_type
			{
				accept_task = 0,
				close_task = 1
			};

			volatile container_queue<task_type> m_serializer;

			accept_helper(const ptr<rc_obj_base>& desc, const rcref<listener>& l, const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
				: object(desc),
				m_listener(l),
				m_acceptDelegate(acceptDelegate),
				m_listenSocket(rcnew(bypass_constructor_permission<tcp>, addressFamily, epp)),
				m_addressFamily(addressFamily),
				m_epollPool(epp),
				m_closed(false)
			{
				l->m_acceptHelper = this_rcref;
				for (;;)
				{
					int i = m_listenSocket->m_socket->bind_any(port);
					if (i != -1)
					{
						int i = ::listen(m_listenSocket->m_socket->m_fd.get(), SOMAXCONN);
						if (i != -1)
						{
							m_listenerRemoveToken = m_epollPool->register_listener(m_listenSocket->m_socket->m_fd.get(), [r{ this_weak_rcptr }]()
							{
								rcptr<accept_helper> r2 = r;
								if (!!r2)
									r2->accept_connection();
							});
							break;
						}
					}
					l->close();
					break;
				}
			}

			void close()	{ close_or_accept_connection(close_task); }

			void accept_connection()	{ close_or_accept_connection(accept_task); }

			void close_or_accept_connection(task_type taskType)
			{
				if (m_serializer.append(taskType))
				{
					for (;;)
					{
						if (taskType == accept_task)
						{
							if (!m_closed)
							{
								rcptr<listener> l = m_listener;
								for (;;)
								{
									if (!!l)
									{
										int s = accept(m_listenSocket->m_socket->m_fd.get(), 0, 0);
										if (s != -1)
										{
											rcref<tcp> ds = rcnew(bypass_constructor_permission<tcp>, s, m_addressFamily, m_epollPool);
											ds->m_socket->read_endpoints();
											thread_pool::get_default_or_immediate()->dispatch([r{ this_rcref }, ds{ std::move(ds) }]()
											{
												r->m_acceptDelegate(ds);
											});
										}
										m_listenerRemoveToken = m_epollPool->register_listener(m_listenSocket->m_socket->m_fd.get(), [r{ this_weak_rcptr }]()
										{
											rcptr<accept_helper> r2 = r;
											if (!!r2)
												r2->accept_connection();
										});
									}
									// closed
									break;
								}
							}
						}
						else // if (taskType == close_task)
						{
							m_closed = true;
							m_epollPool->deregister_listener(m_listenerRemoveToken);
							if (!!m_listenSocket)
							{
								m_listenSocket->close();
								m_listenSocket = 0;
							}
						}
						bool wasLast;
						m_serializer.remove_first(wasLast);
						if (wasLast)
							break;
						m_serializer.peek_first(taskType);
					}
				}
			}
		};

		unsigned short			m_port;
		rcptr<accept_helper>	m_acceptHelper;
		single_fire_event		m_closeEvent;
				
		listener() = delete;
		listener(listener&&) = delete;
		listener(const listener&) = delete;
		listener& operator=(listener&&) = delete;
		listener& operator=(const listener&) = delete;

	protected:
		friend class tcp;

		listener(const ptr<rc_obj_base>& desc, const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
			: object(desc),
			m_closeEvent(desc),
			m_port(port)
		{
			rcnew(accept_helper, this_rcref, acceptDelegate, port, addressFamily, epp);
		}

		void close()
		{
			if (!!m_acceptHelper)
			{
				m_acceptHelper->close();
				m_acceptHelper = 0;
				m_closeEvent.signal();
			}
		}

	public:
		~listener()									{ close(); }

		const waitable& get_close_event() const		{ return m_closeEvent; }
	};

	static rcref<listener> listen(const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4, const rcref<os::io::epoll_pool>& epp = os::io::epoll_pool::get())
	{
		return rcnew(bypass_constructor_permission<listener>, acceptDelegate, port, addressFamily, epp);
	}

	static rcref<listener> server_listen(const rcref<net::server>& srvr, unsigned short port, address_family addressFamily = inetv4)
	{
		return listen([srvr](const rcref<tcp>& r)
		{
			srvr->connecting(r.template static_cast_to<net::connection>());
		}, port, addressFamily);
	}

	virtual rcref<const net::endpoint> get_local_endpoint()	const	{ return this_rcref.member_cast_to(m_socket->m_localEndpoint); }
	virtual rcref<const net::endpoint> get_remote_endpoint() const	{ return this_rcref.member_cast_to(m_socket->m_remoteEndpoint); }
};


}
}
}
}


#endif
