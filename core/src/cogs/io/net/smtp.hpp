//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_HEADER_IO_NET_SMTP
#define COGS_HEADER_IO_NET_SMTP


#include "cogs/collections/map.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/function.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/io/net/ip/tcp.hpp"
#include "cogs/io/net/server.hpp"
#include "cogs/math/chars.hpp"
#include "cogs/math/datetime.hpp"


namespace cogs {
namespace io {
namespace net {

/// @ingroup Net
/// @brief Namespace for SMTP
namespace smtp {


class client;
//class client::request; // client request
//class client::response; // client response

class server;
//class server::connection;
//class server::request;
//class server::response;


/// @ingroup Net
/// @brief A SMTP server
class server : public net::request_response_server
{
public:
	class connection;
	class request;
	class response;

	typedef function<void(const rcref<request>&)> command_delegate_t;

	typedef nonvolatile_map<composite_cstring, command_delegate_t, true> command_handler_map_t;

private:
	server(const server&) = delete;
	server& operator=(const server&) = delete;

public:
	class connection : public net::request_response_server::connection
	{
	private:
		friend class server;
		friend class request;
		friend class response;

		// Since SMTP is state-based, the connection holds the current email state
		composite_cstring m_reversePath;
		vector<composite_cstring> m_recipientList;

		connection() = delete;
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

		connection(const rcref<server>& srvr, const rcref<net::connection>& c, const timeout_t::period_t& inactivityTimeout = timeout_t::period_t(0))
			: net::request_response_server::connection(srvr, c, true, inactivityTimeout)
		{ }

		virtual void start()
		{
			static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
			cstring crlfStr = cstring::contain(CRLF, 2);
			composite_cstring connectStr = cstring::literal("220 ");
			connectStr += get_host_name_cstring();
			connectStr += cstring::literal(" cogs::io::net::smtp::server/1.0; ");
			connectStr += datetime::now().to_cstring();
			connectStr += crlfStr;
			get_net_connection()->write(composite_buffer::from_composite_cstring(connectStr));
			net::request_response_server::connection::start();
		}

		virtual rcref<net::request_response_server::request> create_request() { return server::default_create_request(this_rcref); }

	public:
		// Since SMTP is state-based, the connection holds the current email state
		void clear_mail_state()
		{
			m_reversePath.clear();
			m_recipientList.clear();
		}
	};

	class response : public net::request_response_server::response
	{
	public:
		enum reply
		{
			system_status_of_help = 211,
			help_message = 214,

			service_ready = 220,
			closing_channel = 221,
			action_completed = 250,
			non_local_user_forwarded = 251,
			unverified_user_accepted = 252,

			start_mail_input = 354,

			service_not_available = 421,
			mailbox_busy = 450,
			local_error = 451,
			insufficient_storage = 452,
			parameter_refused = 455,

			syntax_error = 500,
			syntax_error_in_parameters = 501,
			command_not_implemented = 502,
			bad_command_sequence = 503,
			parameter_not_implemented = 504,

			mailbox_unavailable = 550,
			user_not_local = 551,
			exceeded_storage_limit = 552,
			mailbox_name_not_allowed = 553,
			transaction_failed = 554,
			parameter_error = 555 // MAIL FROM/RCPT TO parameters not recognized or not implemented
		};

	protected:
		friend class server;
		friend class request;
		friend class connection;

		reply m_replyCode;
		const composite_cstring m_text;

		response(const rcref<request>& r, reply replyCode, const composite_cstring& text = cstring())
			: net::request_response_server::response(r),
			m_replyCode(replyCode),
			m_text(text)
		{ }

		virtual void start()
		{
			net::request_response_server::response::start();

			static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
			cstring crlfStr = cstring::contain(CRLF, 2);
			composite_cstring str = size_type((int)m_replyCode).to_cstring();
			str += cstring::literal(" ");
			str += m_text;
			str += crlfStr;
			get_datasink()->write(composite_buffer::from_composite_cstring(str));
		}
	};

	class request : public net::request_response_server::request
	{
	protected:
		friend class response;
		friend class connection;
		friend class server;

		static constexpr size_t max_request_length = 64 * 1024; // 64K

		composite_buffer m_bufferedWrite;
		bool m_gotCR = false;
		composite_cstring m_currentCommand;
		composite_cstring m_commandParams;
		rcref<datasink> m_sink;
		rcptr<task<void> > m_coupler;

	private:
		rcref<task<bool> > process_write(composite_buffer& compBuf)
		{
			bool completeRequest = false;
			bool closing = false;

			// Buffer all input until the pattern <CR><LF> is found.
			// Also check for max request size.
			for (; !!compBuf;)
			{
				buffer buf = compBuf.get_inner(0);
				char c = ((char*)(buf.get_const_ptr()))[0];
				compBuf.advance(1);
				if (c == special_characters<char>::CR)
					m_gotCR = true;
				else if (c == special_characters<char>::LF)
				{
					if (m_gotCR)
					{
						completeRequest = true;
						break;
					}
				}
				else
				{
					m_gotCR = false;
					if ((c == special_characters<char>::DEL) || (c == special_characters<char>::BS))
					{
						if (m_currentCommand.get_length() > 0)
							m_currentCommand.truncate(1);
					}
					else
					{
						m_currentCommand.append(c);
						if (m_currentCommand.get_length() > max_request_length)
						{
							begin_response(response::reply::syntax_error, cstring::literal("Command too long"))->complete();
							closing = true;
							break;
						}
					}
				}
			}

			if (completeRequest)
			{
				rcptr<connection> c = m_connection.template static_cast_to<connection>();
				if (!c)
					closing = true;
				else
				{
					rcptr<server> srvr = c->get_server().template static_cast_to<server>();
					if (!srvr)
						closing = true;
					else
					{
						m_commandParams.clear();
						composite_cstring command = m_currentCommand;
						composite_cstring::position_t pos = command.position_of(composite_cstring::position_t(0, 0), ' ');
						if (pos != command.get_end_position())
						{
							command.truncate_to(pos);
							m_commandParams = m_currentCommand;
							m_commandParams.advance_to(m_commandParams.get_next_position(pos));
						}
						cstring commandStr = command.composite();
						commandStr.to_uppercase();
						command_handler_map_t::iterator commandItor = srvr->m_commandHandlerMap->find(commandStr);
						if (!!commandItor)
							commandItor->value(this_rcref);
						else
							begin_response(response::reply::command_not_implemented, cstring::literal("Command not recognized: \"") + m_currentCommand + cstring::literal("\""))->complete();
					}
				}
				m_currentCommand.clear();
			}

			return signaled(closing);
		}

	public:
		rcref<response> begin_response(response::reply replyCode, const composite_cstring& text)
		{
			m_coupler->cancel();
			rcref<response> r = rcnew(response)(this_rcref, replyCode, text);
			r->start();
			return r;
		}

		explicit request(const rcref<connection>& c)
			: net::request_response_server::request(c),
			m_sink(rcnew(datasink)([r{ this_weak_rcptr }](composite_buffer& b)
			{
				rcptr<request> r2 = r;
				if (!r2)
					return signaled(true);
				return r2->process_write(b);
			}))
		{ }

		virtual void start()
		{
			net::request_response_server::request::start();

			m_coupler = couple(get_datasource(), m_sink, true, true);
			m_sink->get_sink_close_event().dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<request> r2 = r;
				if (!!r2)
					r2->abort();
			});
		}

		composite_cstring get_command_params() const { return m_commandParams; }
	};

private:
	static rcref<net::request_response_server::request> default_create_request(const rcref<connection>& c) { return rcnew(request)(c); }

	rcref<command_handler_map_t> m_commandHandlerMap;

	virtual rcref<net::server::connection> create_connection(const rcref<net::connection>& ds)
	{
		return rcnew(connection)(this_rcref, ds);// , make_measure<seconds>(inactivity_timeout_in_seconds));
	}

public:
	static void default_HELO_handler(const rcref<request>& r)
	{
		composite_cstring params = r->get_command_params();
		if (!params)
			r->begin_response(response::reply::syntax_error_in_parameters, cstring::literal("HELO/EHLO requires domain address"))->complete();
		else
		{
			rcptr<net::server::connection> c = r->get_connection();
			if (!!c)
			{
				composite_cstring str = get_host_name_cstring();
				str += cstring::literal(" Hello ");
				str += params;
				str += cstring::literal(" [");
				rcptr<const endpoint> e = c->get_net_connection()->get_remote_endpoint();
				const io::net::address& a = e->get_address();
				str += a.to_cstring();
				str += cstring::literal("], pleased to meet you");

				r->begin_response(response::reply::action_completed, str)->complete();
			}
		}
	}

	static void default_NOOP_handler(const rcref<request>& r)
	{
		r->begin_response(response::reply::action_completed, cstring::literal("OK"))->complete();
	}

	static void default_QUIT_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().template static_cast_to<connection>();
		if (!!c)
			c->set_connection_reuse(false);
		r->begin_response(response::reply::action_completed, cstring::literal("OK"))->complete();
	}

	static void default_MAIL_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().template static_cast_to<connection>();
		if (!!c) // otherwise, connection is shutting down
		{
			if (!!c->m_reversePath)
				r->begin_response(response::reply::bad_command_sequence, cstring::literal("Sender already specified"))->complete();
			else
			{
				const vector<composite_cstring> params = r->get_command_params().split_on(' ');
				if (!params[0] || !params[0].starts_with(cstring::literal("FROM:<")) || !params[0].ends_with(">", 1))
					r->begin_response(response::reply::parameter_error, cstring::literal("MAIL FROM parameters not recognized or not implemented"))->complete();
				else
				{
					composite_cstring reversePath = params[1];
					reversePath.advance(6);
					reversePath.truncate(1);
					c->m_reversePath = reversePath;
					r->begin_response(response::reply::action_completed, cstring::literal("OK"))->complete();
				}
			}
		}
	}

	static void default_RCPT_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().template static_cast_to<connection>();
		if (!!c) // otherwise, connection is shutting down
		{
			if (!c->m_reversePath)
				r->begin_response(response::reply::bad_command_sequence, cstring::literal("Need MAIL before RCPT"))->complete();
			else
			{
				const vector<composite_cstring> params = r->get_command_params().split_on(' ');
				if (!params[0] || !params[0].starts_with(cstring::literal("TO:<")) || !params[0].ends_with(">", 1))
					r->begin_response(response::reply::parameter_error, cstring::literal("RCPT TO parameters not recognized or not implemented"))->complete();
				else
				{
					composite_cstring recipient = params[1];
					recipient.advance(4);
					recipient.truncate(1);

					// Now to decide if we want to accept the recipient or not
					vector<composite_cstring> parts = recipient.split_on('@');


					c->m_recipientList.append(1, recipient);
					r->begin_response(response::reply::action_completed, cstring::literal("OK"))->complete();
				}
			}
		}
	}

	static rcref<command_handler_map_t> get_default_command_handlers()
	{
		rcref<command_handler_map_t> mapRef = rcnew(command_handler_map_t);
		mapRef->insert_unique(cstring::literal("HELO"), &default_HELO_handler);
		mapRef->insert_unique(cstring::literal("EHLO"), &default_HELO_handler);
		mapRef->insert_unique(cstring::literal("NOOP"), &default_NOOP_handler);
		mapRef->insert_unique(cstring::literal("QUIT"), &default_QUIT_handler);
		mapRef->insert_unique(cstring::literal("MAIL"), &default_MAIL_handler);
		// RCPT
		// SIZE
		// DATA
		// VRFY
		// EXPN
		// RSET
		return mapRef;
	}

	server()
		: m_commandHandlerMap(get_default_command_handlers())
	{ }

	explicit server(const rcref<command_handler_map_t>& commandHandlers)
		: m_commandHandlerMap(commandHandlers)
	{ }

	//static constexpr uint16_t inactivity_timeout_in_seconds = 60 * 2; // 2 minute inactivity timeout
	// The inactivity timeout must be extended by a handler that does something appropriate to extend
	// the lifetime of the connection.  Care should be taken to prevent denial-of-service attacks,
	// such as clients establishing unnecessary connections and leaving them connected, to try to
	// deprive the server of memory or network resources.
};


/*
class client
{
private:
	rcptr<ip::tcp> m_tcpSocket;
	unsigned short m_port;

public:
	explicit client(unsigned short port = 25)
		: m_port(port)
	{ }


//	static rcref<connecter> connect(const vector<address>& addresses, unsigned short port, const rcref<os::io::completion_port>& cp = os::io::completion_port::get())
//	{
//		return rcnew(connecter)(addresses, port, cp);
//	}
//
//	static rcref<connecter> connect(const address& addr, unsigned short port, const rcref<os::io::completion_port>& cp = os::io::completion_port::get())
//	{
//		vector<address> addresses;
//		addresses.append(addr);
//		return connect(addresses, port, cp);
//	}

};
*/


}
}
}
}


#endif
