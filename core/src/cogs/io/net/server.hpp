//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_NET_SERVER
#define COGS_NET_SERVER


#include "cogs/collections/container_dlist.hpp"
#include "cogs/function.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/delegated_datasink.hpp"
#include "cogs/io/net/connection.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/measure.hpp" //
#include "cogs/mem/object.hpp"
#include "cogs/sync/single_fire_timer.hpp"


namespace cogs {
namespace io {
namespace net {


/// @ingroup Net
/// @brief A simple server base class.
class server : public object
{
private:
	server(const server&) = delete;
	server& operator=(const server&) = delete;

public:
	class connection;

protected:
	// Server owns primary references to all active connections.
	typedef container_dlist<rcref<connection> > connection_list_t;
	volatile connection_list_t m_connections;

public:
	class connection : public object
	{
	private:
		connection() = delete;
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

		const weak_rcptr<server>		m_server;
		const rcref<net::connection>	m_netConnection;
		volatile boolean				m_closed;
		const rcref<single_fire_timer>	m_inactivityTimer;
		volatile timeout_t::period_t	m_inactivityTimeoutPeriod;

		connection_list_t::volatile_remove_token m_removeToken;

	protected:
		friend class server;

		void abort()	// immediate close - does not wait for data to be completely written (though underlying protocol might).
		{
			if (m_closed.compare_exchange(true, false))
			{
				rcptr<server> srvr = m_server;
				if (!!srvr)
					srvr->m_connections.remove(m_removeToken);	// closes datastream by releasing it from scope
				m_netConnection->abort();
			}
		}

		virtual void start()
		{
			restart_inactivity_timer();
		}

		virtual void inactivity_timeout_expired()		{ abort(); }

	public:		
		const rcref<net::connection>& get_net_connection() const	{ return m_netConnection; }

		virtual rcref<datasource> get_datasource() const	{ return m_netConnection; }
		virtual rcref<datasink> get_datasink() const		{ return m_netConnection; }

		connection(const rcref<server>& srvr, const rcref<net::connection>& c, const timeout_t::period_t& inactivityTimeoutPeriod = make_measure<timeout_t::period_unitbase>(0))
			:	m_netConnection(c),
			m_server(srvr),
			m_inactivityTimeoutPeriod(inactivityTimeoutPeriod),
			m_inactivityTimer(single_fire_timer::create(timeout_t::infinite()))
		{
			m_removeToken = srvr->m_connections.append(this_rcref);
			m_inactivityTimer->dispatch([c{ this_weak_rcptr }]()
			{
				rcptr<connection> c2 = c;
				if (!!c2)
					c2->inactivity_timeout_expired();
			});
		}

		bool set_inactivity_timout_period(const timeout_t::period_t& t) volatile	{ m_inactivityTimeoutPeriod = t; return restart_inactivity_timer(); }
		bool cancel_inactivity_timer() volatile										{ return m_inactivityTimer->abort(); }
		
		bool restart_inactivity_timer() volatile
		{
			timeout_t::period_t t1 = m_inactivityTimeoutPeriod;
			timeout_t newTimeout;
			if (!t1)
				newTimeout = timeout_t::infinite();
			else
				newTimeout = t1;
			bool notAborted = m_inactivityTimer->reschedule(newTimeout);
			if (!notAborted)
				return false;
			timeout_t::period_t t2 = m_inactivityTimeoutPeriod;	// If changed since what we set it to, one more try should ensure reasonable ordering.
			if (t1 == t2)
				return true;
			if (!t2)
				newTimeout = timeout_t::infinite();
			else
				newTimeout = t2;
			return m_inactivityTimer->reschedule(newTimeout);
		}

		const weak_rcptr<server>& get_server() const					{ return m_server; }

		void release()
		{
			if (m_closed.compare_exchange(true, false))
			{
				rcptr<server> srvr = m_server;
				if (!!srvr)
					srvr->m_connections.remove(m_removeToken);	// closes datastream by releasing it from scope
			}
		}
	};

public:
	server()
	{ }

	virtual void connecting(const rcref<net::connection>& c)
	{
		create_connection(c)->start();	// scope added to server in connection base constructor
	}	

protected:
	virtual rcref<connection> create_connection(const rcref<net::connection>& c) = 0;
};


/// @ingroup Net
/// @brief A simple request-response server base class.
class request_response_server : public server
{
private:
	request_response_server(const request_response_server&) = delete;
	request_response_server& operator=(const request_response_server&) = delete;

public:
	class connection : public server::connection
	{
	public:
		class request;
		class response;

	private:
		friend class request;
		friend class response;

		connection() = delete;
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

		volatile boolean m_reuse;
		volatile rcptr<request> m_currentRequest;
		volatile rcptr<response> m_currentResponse;

	protected:
		virtual rcref<request> create_request() = 0;

		void start_inner()
		{
			m_currentRequest.release();
			m_currentResponse.release();
			rcref<request> r = create_request();
			m_currentRequest = r;
			r->start();
		}

	public:
		connection(const rcref<server>& srvr, const rcref<net::connection>& c, bool supportMultipleRequests = true, const timeout_t::period_t& inactivityTimeout = make_measure<timeout_t::period_unitbase>(0))
			: server::connection(srvr, c, inactivityTimeout),
			m_reuse(supportMultipleRequests)
		{ }

		void set_connection_reuse(bool reuse)	{ m_reuse = reuse; }
		bool is_connection_reusable() const		{ return !!m_reuse; }

		virtual void recycle()
		{
			rcptr<datasink> ds;
			rcptr<response> r = m_currentResponse;
			if (!!r)
				ds = r->get_datasink();
			if (!ds)
				ds = get_datasink();
			if (is_connection_reusable())
			{
				ds->flush_sink()->dispatch([c{ this_weak_rcptr }]()
				{
					rcptr<connection> c2 = c;
					if (!!c2)
						c2->start_inner();
				});
			}
			else
			{
				ds->close_sink()->dispatch([c{ this_weak_rcptr }]()
				{
					rcptr<connection> c2 = c;
					if (!!c2)
						c2->release();
				});
			}
		}

		// A derived request should know just enough to parse the request.
		class request : public object
		{
		protected:
			friend class server;
			friend class connection;

			const weak_rcptr<connection>			m_connection;
			const rcref<datasource::transaction>	m_transaction;
			volatile boolean						m_completed;

			request(const rcref<connection>& c)
				: m_connection(c),
				m_transaction(datasource::transaction::create(c->get_datasource(), false, datasource::transaction::propagate_abort_only))
			{
			}

			virtual void start()
			{
				m_transaction->start();
			}

		public:
			const weak_rcptr<connection> get_connection() const	{ return m_connection; }

			virtual rcref<datasource> get_datasource() const	{ return m_transaction; }

			bool abort()
			{
				if (!m_completed)
				{
					// Consider destruction without completing an abort
					rcptr<connection> c = m_connection;
					if (!!c)
						c->abort();
					return true;
				}
				return false;
			}

			bool complete()	// If a response object is used, complete it instead of the request.
			{
				if (m_completed.compare_exchange(true, false))
				{
					rcptr<connection> c = m_connection;
					if (!!c)
						c->recycle();
					return true;
				}
				return false;
			}

			virtual void completing()	{ }
		};

		class response : public object
		{
		private:
			const weak_rcptr<connection>		m_connection;
			const rcref<datasink::transaction>	m_transaction;
			const rcref<request>				m_request;	// response extends scope of request. response is gone before request.

		protected:
			friend class request;

			response(const rcref<request>& r)
				: m_request(r),
				m_connection(r->get_connection()),
				m_transaction(datasink::transaction::create(r->get_connection()->get_datasink(), false, datasink::transaction::propagate_abort_only))
			{ }

		public:
			virtual void start()
			{
				m_transaction->start();
			}

			const weak_rcptr<connection> get_connection() const	{ return m_connection; }

			virtual rcref<datasink> get_datasink() const		{ return m_transaction; }

			const rcref<request>& get_request() const			{ return m_request; }

			bool complete()
			{
				if (m_request->m_completed.compare_exchange(true, false))
				{
					rcptr<connection> c = m_connection;
					m_request->completing();
					completing();
					if (!!c)
					{
						c->m_currentResponse = this_rcref;
						c->recycle();
					}
					return true;
				}
				return false;
			}

			virtual void completing() { }

			~response()
			{
				m_request->abort();	// aborts if not completed
			}
		};

		virtual void start()
		{
			start_inner();
		}
	};

	typedef connection::request request;
	typedef connection::response response;

public:
	request_response_server()
	{ }
};


}
}
}


#endif
