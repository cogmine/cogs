//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_OS_IP_TCP
#define COGS_OS_IP_TCP


#include "cogs/io/datastream.hpp"
#include "cogs/io/net/connection.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/os/io/net/ip/socket.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace io {
namespace net {
namespace ip {


class tcp : public connection
{
private:
	const rcref<socket> m_socket;

	class tcp_reader : public reader
	{
	private:
		const rcref<socket>						m_socket;
		const weak_rcptr<tcp>					m_tcp;
		os::io::completion_port::overlapped_t*	m_overlapped;
		buffer									m_currentBuffer;
		size_t									m_progress;
		size_t									m_adjustedRequestedSize;
		volatile fixed_integer<false, 2>		m_abortStateBits;	// bit 0=aborted, bit 1=started
		
		enum task_type
		{
			read_task = 0,
			complete_task = 1
		};

		volatile container_queue<task_type> m_completionSerializer;

		bool immediate_read(SOCKET s, size_t n)	// for now just returns false on failure, to trigger generic socket close.  More error handling later.
		{
			int i = recv(s, (char*)(m_currentBuffer.get_ptr()) + m_progress, (int)n, 0);
			if (i == SOCKET_ERROR)
			{
				int i = WSAGetLastError();
				if (i == WSAEWOULDBLOCK)
					return true;
				return false;
			}
			m_progress += i;
			return true;
		}

	public:
		virtual void aborting()
		{
			fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_set_bit(0);
			if (resultingValue.test_bit(1))		// if it has already been started			
				CancelIoEx((HANDLE)(m_socket->m_socket), m_overlapped);
		}

		tcp_reader(const rcref<datasource>& proxy, const rcref<tcp>& t)
			:	reader(proxy),
				m_tcp(t),
				m_socket(t->m_socket),
				m_overlapped(0),
				m_progress(0),
				m_abortStateBits(0)
		{
		}
		
		void execute_in_completion_port_thread()
		{
			read_or_complete(read_task);
		}

		void read_done()
		{
			read_or_complete(complete_task);
		}

		// Some socket implementations (particularly Winsock)
		// may incur significant overhead just checking how many bytes of data are available,
		// so it's more efficient to issue an arbitrary size read, and see how much is retrieved.
		// We go ahead and allocate a single buffer the size of the request, which is most efficient
		// if all data is available now, and only inefficient due to RAM use otherwise.  IO buffers
		// are usually very short-lived.
		virtual void reading()
		{
			for (;;)
			{
				rcptr<tcp> ds = m_tcp;
				bool closing = false;
				if (!!ds)
				{
					m_currentBuffer = allocate_buffer(get_unread_size());
					m_adjustedRequestedSize = m_currentBuffer.get_length();
					if (!immediate_read(m_socket->m_socket, m_adjustedRequestedSize))
					{
						m_currentBuffer.release();
						closing = true;
						//ds->close_source();
					}
					else if ((m_progress != m_adjustedRequestedSize) && ((get_read_mode() != read_some) || !m_progress) && (get_read_mode() != read_now))
					{
						// Defer initiating the actual read to the completion port thread.
						// This is because, on Windows, if the thread initiating asynchronous IO
						// terminates, so does the IO request.
						m_socket->m_completionPort->dispatch([r{ this_rcref }]()
						{
							//COGS_ASSERT(r->m_progress != (size_t)0xdededededededede);
							r->execute_in_completion_port_thread();
						});
						break;
					}
				}
				m_currentBuffer.truncate_to(m_progress);
				get_buffer().append(m_currentBuffer);
				m_currentBuffer.release();
				default_allocator::destruct_deallocate_type(m_overlapped);
				complete(closing);
				break;
			}
		}

		// Need to serialize the async read and its completion to address race condition on abort
		// This is only needed for reads, because they may be restarted.  Writes do not need this synchronization.
		void read_or_complete(task_type taskType)	
		{
			rcptr<tcp> ds = m_tcp;
			if (m_completionSerializer.append(taskType))
			{
				for (;;)
				{
					for (;;)
					{
						bool closing = true;
						if (!!ds)
						{
							closing = false;
							if (taskType == read_task)
							{
								if (!m_abortStateBits.test_bit(0))
								{
									int i;
									int err = 0;
									if (!!m_overlapped)
										m_overlapped->clear();
									else
									{
										m_overlapped = new (default_allocator::get()) os::io::completion_port::overlapped_t([r{ this_weak_rcptr }]()
										{
											rcptr<tcp_reader> r2 = r;
											if (!!r2)
												r2->read_done();
										});
									}
									
									WSABUF buf;
									buf.buf = (char*)(m_currentBuffer.get_ptr()) + m_progress;
									if (get_read_mode() == read_some)
										buf.len = 1;		// Only wait on 1 byte if partial read
									else
										buf.len = (ULONG)(m_adjustedRequestedSize - m_progress);

									DWORD flags = 0;
									i = WSARecv(m_socket->m_socket, &buf, 1, 0, &flags, m_overlapped->get(), 0);
									DWORD err2 = WSAGetLastError();
									if ((i == SOCKET_ERROR) && (err2 != WSA_IO_PENDING))
										closing = true;
									else
									{
										fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_set_bit(1);
										if (resultingValue.test_bit(0))									// if it had already been aborted
											CancelIoEx((HANDLE)(m_socket->m_socket), m_overlapped);
										break;
									}
								}
							}
							else // if (taskType == complete_task)
							{
								size_t n = m_overlapped->m_numTransferred;
								m_progress += n;

								// unset flags
								fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_reset_bit(1);
								bool aborted = resultingValue.test_bit(0);
								if (!aborted)
								{
									if (!n || !m_overlapped->m_success)
										closing = true;
									else if (m_adjustedRequestedSize > m_progress)
									{
										immediate_read(m_socket->m_socket, m_adjustedRequestedSize - m_progress);
										if ((m_adjustedRequestedSize > m_progress) && (get_read_mode() == read_all))
										{
											m_socket->m_completionPort->dispatch([r{ this_rcref }]()
											{
												r->execute_in_completion_port_thread();
											});
											break;
										}
									}
								}
							}
						}
						m_currentBuffer.truncate_to(m_progress);
						get_buffer().append(m_currentBuffer);
						m_currentBuffer.release();
						default_allocator::destruct_deallocate_type(m_overlapped);
						complete(closing);
						break;
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
		const rcref<socket>						m_socket;
		const weak_rcptr<tcp>					m_tcp;
		os::io::completion_port::overlapped_t*	m_overlapped;
		vector<WSABUF>							m_wsaBuffers;
		volatile fixed_integer<false, 2>		m_abortStateBits;	// bit 0=aborted, bit 1=started
		
		tcp_writer(const rcref<datasink>& proxy, const rcref<tcp>& t)
			:	writer(proxy),
				m_tcp(t),
				m_socket(t->m_socket),
				m_overlapped(0),
				m_wsaBuffers(0)
		{ }

		virtual void aborting()
		{
			fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_set_bit(0);
			if (resultingValue.test_bit(1))	// if it has already been started
				CancelIoEx((HANDLE)(m_socket->m_socket), m_overlapped);
		}

		virtual void writing()
		{
			for (;;)
			{
				rcptr<tcp> ds = m_tcp;
				if (!!ds)
				{
					m_wsaBuffers.resize(get_buffer().get_inner_count());
					WSABUF* wsaBuf = m_wsaBuffers.get_ptr();
					for (size_t bufIndex = 0; bufIndex < m_wsaBuffers.get_length(); bufIndex++)
					{
						buffer b = get_buffer().get_inner(bufIndex);
						wsaBuf[bufIndex].buf = (char*)(b.get_const_ptr());
						wsaBuf[bufIndex].len = (ULONG)(b.get_length());
					}

					DWORD numSent = 0;

					int i = WSASend(m_socket->m_socket, wsaBuf, (DWORD)(m_wsaBuffers.get_length()), &numSent, 0, 0, 0);

					// TBD, abort connection here if error.  Currently handled later in completion port thread.  not much harm there.

					get_buffer().advance(numSent);
					if (!!get_buffer())
					{
						// try to complete an incomplete write
						// advance m_wsaBuffers
						for (;;)
						{
							ULONG len = wsaBuf->len;
							if (len > numSent)
							{
								wsaBuf->len = len - numSent;
								wsaBuf->buf += numSent;
								break;
							}
							//else // if (len <= numSent)					
							numSent -= len;
							m_wsaBuffers.advance(1);
						}

						// Defer initiating the actual write to the completion port thread.
						// This is because, on Windows, if the thread initiating asynchronous IO
						// terminates, so does the IO request.
						m_socket->m_completionPort->dispatch([r{ this_rcref }]()
						{
							r->execute_in_completion_port_thread();
						});
						break;
					}
				}
				default_allocator::destruct_deallocate_type(m_overlapped);
				complete();
				break;
			}
		}

		void execute_in_completion_port_thread()
		{
			for (;;)
			{
				bool closing = false;
				if (!m_abortStateBits.test_bit(0))
				{
					rcptr<tcp> ds = m_tcp;
					if (!!ds)
					{
						int i;
						int err = 0;
						m_overlapped = new (default_allocator::get()) os::io::completion_port::overlapped_t([r{ this_weak_rcptr }]()
						{
							rcptr<tcp_writer> r2 = r;
							if (!!r2)
								r2->write_done();
						});

						i = WSASend(m_socket->m_socket, (LPWSABUF)m_wsaBuffers.get_const_ptr(), (DWORD)(m_wsaBuffers.get_length()), NULL, 0, m_overlapped->get(), 0);
						if ((i == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
							closing = true;
						else
						{
							fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_set_bit(1);
							if (resultingValue.test_bit(0))									// if it has already been aborted
								CancelIoEx((HANDLE)(m_socket->m_socket), m_overlapped);
							break;
						}
					}
				}
				default_allocator::destruct_deallocate_type(m_overlapped);
				complete(closing);
				break;
			}
		}
		
		void write_done()
		{
			size_t n = m_overlapped->m_numTransferred;
			get_buffer().advance(n);

			bool closing = false;
			// unset flags
			fixed_integer<false, 2> resultingValue = m_abortStateBits.pre_reset_bit(1);
			bool aborted = resultingValue.test_bit(0);
			if (!aborted && (!n || (!m_overlapped->m_success)))
				closing = true;
			default_allocator::destruct_deallocate_type(m_overlapped);
			complete(closing);
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
	tcp(address_family addressFamily = inetv4, const rcref<os::io::completion_port>& cp = os::io::completion_port::get(), const rcref<network>& n = network::get_default())
		: m_socket(rcnew(bypass_constructor_permission<socket>, SOCK_STREAM, IPPROTO_TCP, addressFamily, cp, n))
	{ }

public:
	~tcp()
	{
		abort();
	}

	class connecter : public signallable_task<connecter>
	{
	protected:
		rcref<network>						m_network;
		rcref<os::io::completion_port>			m_completionPort;
		os::io::completion_port::overlapped_t*	m_overlapped;

		rcptr<tcp>								m_tcp;
		vector<address>							m_addresses;
		unsigned short							m_remotePort;
		//single_fire_event						m_event;

		friend class tcp;

		connecter(const vector<address>& addresses, unsigned short port, const rcref<os::io::completion_port>& cp, const rcref<network>& n = network::get_default())
			:	m_overlapped(0),
				m_addresses(addresses),
				m_network(n),
				m_remotePort(port),
				m_completionPort(cp)
		{
			self_acquire();

			// Defer initiating the actual connect attempt to the completion port thread.
			// This is because, on Windows, if the thread initiating asynchronous IO
			// terminates, so does the IO request.
			m_completionPort->dispatch([r{ this_rcref }]()
			{
				r->execute_in_completion_port_thread();
			});
		}

		void execute_in_completion_port_thread()
		{
			for (;;)
			{
				if (m_addresses.is_empty())
				{
					default_allocator::destruct_deallocate_type(m_overlapped);
					m_tcp.release();	// failure to connect is indicated by complete connecter with null tcp object
					signal();
					self_release();
					break;
				}

				// Start with address first in the list.
				// If the connection fails, we'll remove it from the list.
				const address& addr = *(m_addresses.get_const_ptr());

				// Set port in address structure.
				sockaddr_in sa = *(sockaddr_in*)(addr.get_sockaddr());
				sa.sin_port = htons(m_remotePort);

				// Allocate tcp obj to use
				m_tcp = rcnew(bypass_constructor_permission<tcp>, addr.get_address_family(), m_completionPort, m_network);

				// Bind local port to any address.
				int i = m_tcp->m_socket->bind_any();

				// Query for ptr to ConnectEx
				LPFN_CONNECTEX m_lpfnConnectEx = 0;
				DWORD dwBytes;
				GUID GuidConnectEx = WSAID_CONNECTEX;
				DWORD dwErr = WSAIoctl(m_tcp->m_socket->m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
									&GuidConnectEx,
									sizeof(GuidConnectEx),
									&m_lpfnConnectEx,
									sizeof(m_lpfnConnectEx),
									&dwBytes,
									NULL,
									NULL); 
				if (dwErr == SOCKET_ERROR)
				{							// Another address might actually have a different socket type, so try again
					m_addresses.erase(0, 1);
					continue;
				}

				if (!!m_overlapped)
					m_overlapped->clear();
				else
				{
					m_overlapped = new (default_allocator::get()) os::io::completion_port::overlapped_t([r{ this_rcref }]()
					{
						r->connect_done();
					});
				}

				if (!!m_lpfnConnectEx(m_tcp->m_socket->m_socket, (sockaddr*)&sa, (int)(addr.get_sockaddr_size()), NULL, 0, NULL, m_overlapped->get()))
					break;	// completed synchronously

				DWORD err = WSAGetLastError();
				if (err == WSA_IO_PENDING)
					break;	// completing asynchronously
					
				m_addresses.erase(0, 1);
			}
		}

		void connect_done()
		{
			if (!m_overlapped->m_success)	// on failure
			{
				m_addresses.erase(0, 1);
				m_completionPort->dispatch([r{ this_rcref }]()
				{
					r->execute_in_completion_port_thread();
				});
			}
			else
			{
				m_tcp->m_socket->read_endpoints();
				default_allocator::destruct_deallocate_type(m_overlapped);
				signal();
				self_release();
			}
		}

		virtual const connecter& get() const volatile { return *(const connecter*)this; }

	public:
		const rcptr<tcp>& get_tcp() const				{ return m_tcp; }
		
		unsigned short get_remote_port() const			{ return m_remotePort; }

		// TODO: cancel does not yet abort the connection operation, just prevent it from completing successfully
		virtual bool cancel() volatile
		{
			return signallable_task<connecter>::cancel();
		}
	};

	static rcref<connecter> connect(const vector<address>& addresses, unsigned short port, const rcref<os::io::completion_port>& cp = os::io::completion_port::get(), const rcref<network>& n = network::get_default())
	{
		return rcnew(bypass_constructor_permission<connecter>, addresses, port, cp);
	}

	static rcref<connecter> connect(const address& addr, unsigned short port, const rcref<os::io::completion_port>& cp = os::io::completion_port::get(), const rcref<network>& n = network::get_default())
	{
		vector<address> addresses;
		addresses.append(1, addr);
		return connect(addresses, port, cp);
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
			rcref<network>					m_network;
			rcref<os::io::completion_port>			m_completionPort;
			os::io::completion_port::overlapped_t*	m_overlapped;

			accept_delegate_t					m_acceptDelegate;
			address_family						m_addressFamily;

			weak_rcptr<listener>				m_listener;
			rcptr<tcp>							m_listenSocket;
			rcptr<tcp>							m_acceptSocket;
			LPFN_ACCEPTEX						m_lpfnAcceptEx;
			LPFN_GETACCEPTEXSOCKADDRS			m_lpfnGetAcceptExSockaddrs;
			char								m_acceptExBuffer[(16 + sizeof(SOCKADDR_STORAGE)) * 2];

			accept_helper(const rcref<listener>& l, const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4, const rcref<os::io::completion_port>& cp = os::io::completion_port::get(), const rcref<network>& n = network::get_default())
				:	m_listener(l),
					m_acceptDelegate(acceptDelegate),
					m_listenSocket(rcnew(bypass_constructor_permission<tcp>, addressFamily, cp, n)),
					m_acceptSocket(rcnew(bypass_constructor_permission<tcp>, addressFamily, cp, n)),
					m_addressFamily(addressFamily),
					m_completionPort(cp),
					m_network(n)
			{
				l->m_acceptHelper = this_rcref;
				for (;;)
				{
					int i = m_listenSocket->m_socket->bind_any(port);
					if (i != SOCKET_ERROR)
					{
						int i = ::listen(m_listenSocket->m_socket->m_socket, SOMAXCONN);
						if (i != SOCKET_ERROR)
						{
							// Query for ptr to AcceptEx
							m_lpfnAcceptEx = 0;
							DWORD dwBytes;
							GUID GuidAcceptEx = WSAID_ACCEPTEX;
							DWORD dwErr = WSAIoctl(m_listenSocket->m_socket->m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
												&GuidAcceptEx,
												sizeof(GuidAcceptEx),
												&m_lpfnAcceptEx,
												sizeof(m_lpfnAcceptEx),
												&dwBytes,
												NULL,
												NULL); 
							if (dwErr != SOCKET_ERROR)
							{
								// Query for ptr to GetAcceptExSockaddrs 
								m_lpfnGetAcceptExSockaddrs = 0;
								DWORD dwBytes;
								GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
								DWORD dwErr = WSAIoctl(m_listenSocket->m_socket->m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
													&GuidGetAcceptExSockaddrs,
													sizeof(GuidGetAcceptExSockaddrs),
													&m_lpfnGetAcceptExSockaddrs,
													sizeof(m_lpfnGetAcceptExSockaddrs),
													&dwBytes,
													NULL,
													NULL); 
								if (dwErr != SOCKET_ERROR)
								{
									m_overlapped = new (default_allocator::get()) os::io::completion_port::overlapped_t([r{ this_rcref }]()
									{
										r->connection_accepted();
									});
									
									m_listenSocket->m_socket->m_completionPort->dispatch([r{ this_rcref }]()
									{
										r->accept();
									});
									break;
								}
							}
						}
					}
					l->close();
					break;
				}
			}

			~accept_helper()	
			{
			}

			void close()
			{
				m_listenSocket.release();
				m_acceptSocket.release();
			}

			void accept()
			{
				rcptr<listener> l = m_listener;
				for (;;)
				{
					if (!!l)
					{
						DWORD unused;
						if (!(*m_lpfnAcceptEx)(m_listenSocket->m_socket->m_socket, m_acceptSocket->m_socket->m_socket, m_acceptExBuffer, 0, (16 + sizeof(SOCKADDR_STORAGE)), (16 + sizeof(SOCKADDR_STORAGE)), &unused, m_overlapped))
						{
							DWORD err = WSAGetLastError();
							if (err == WSA_IO_PENDING)
								break;
						}
						l->close();
					}
					default_allocator::destruct_deallocate_type(m_overlapped);
					break;
				}
			}

			void connection_accepted()
			{
				rcptr<listener> l = m_listener;
				for (;;)
				{
					if (!!l)
					{
						DWORD n;
						DWORD flags;
						BOOL b = WSAGetOverlappedResult(m_listenSocket->m_socket->m_socket, m_overlapped, &n, FALSE, &flags);
						if (!b)
							l->close();
						else
						{
							sockaddr* localAddr = 0;
							sockaddr* remoteAddr = 0;
							INT localAddrLength = 0;
							INT remoteAddrLength = 0;

							m_lpfnGetAcceptExSockaddrs(m_acceptExBuffer, 0, (16 + sizeof(SOCKADDR_STORAGE)), (16 + sizeof(SOCKADDR_STORAGE)),
								&localAddr, &localAddrLength, &remoteAddr, &remoteAddrLength);

							endpoint& localEndpoint = m_acceptSocket->m_socket->m_localEndpoint;
							endpoint& remoteEndpoint = m_acceptSocket->m_socket->m_remoteEndpoint;
							localEndpoint.set_sockaddr_size(localAddrLength);
							remoteEndpoint.set_sockaddr_size(remoteAddrLength);
							memcpy(localEndpoint.get_sockaddr(), localAddr, localAddrLength);
							memcpy(remoteEndpoint.get_sockaddr(), remoteAddr, remoteAddrLength);
							
							thread_pool::get_default_or_immediate()->dispatch([r{ this_rcref }, s{ m_acceptSocket.dereference() }]()
							{
								r->m_acceptDelegate(s);
							});
							m_acceptSocket = rcnew(bypass_constructor_permission<tcp>, m_addressFamily, m_completionPort, m_network);	// replace accept-socket
							accept();	// accepted, start another
							break;
						}
					}
					default_allocator::destruct_deallocate_type(m_overlapped);
					break;
				}
			}

		};

		unsigned short				m_port;
		rcptr<accept_helper>		m_acceptHelper;
		rcref<single_fire_event>	m_closeEvent;
				
		listener();
		listener(const listener&);

	protected:
		friend class tcp;

		listener(const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4)
			:	m_port(port),
				m_closeEvent(rcnew(single_fire_event))
		{
			rcnew(accept_helper, this_rcref, acceptDelegate, port, addressFamily);
		}

		void close()
		{
			if (!!m_acceptHelper)
			{
				m_acceptHelper->close();
				m_acceptHelper = 0;
				m_closeEvent->signal();
			}
		}

	public:

		~listener()									{ close(); }

		rcref<waitable> get_close_event() const		{ return m_closeEvent; }
	};

	static rcref<listener> listen(const accept_delegate_t& acceptDelegate, unsigned short port, address_family addressFamily = inetv4)
	{
		return rcnew(bypass_constructor_permission<listener>, acceptDelegate, port, addressFamily);
	}

	static rcref<listener> server_listen(const rcref<net::server>& srvr, unsigned short port, address_family addressFamily = inetv4)
	{
		return listen([srvr](const rcref<tcp>& r)
		{
			srvr->connecting(r.static_cast_to<net::connection>());
		}, port, addressFamily);
	}

	virtual rcref<const net::endpoint> get_local_endpoint() const	{ return this_rcref.member_cast_to(m_socket->m_localEndpoint); }
	virtual rcref<const net::endpoint> get_remote_endpoint() const	{ return this_rcref.member_cast_to(m_socket->m_remoteEndpoint); }
};


}
}
}
}


#endif
