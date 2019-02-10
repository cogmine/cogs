//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_SMTP
#define COGS_SMTP


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
#include "cogs/math/int_types.hpp"


namespace cogs {
namespace io {
namespace net {

/// @ingroup Net
/// @brief Namespace for SMTP
namespace smtp {


class client;
//class client::request;	// client request	
//class client::response;	// client response

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

	typedef nonvolatile_map<composite_cstring, command_delegate_t, true>	command_handler_map_t;

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
		composite_cstring				m_reversePath;
		vector<composite_cstring>		m_recipientList;
		
		connection() = delete;
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

	protected:
		connection(const rcref<server>& srvr, const rcref<net::connection>& c, const timeout_t::period_t& inactivityTimeout = timeout_t::period_t(0))
			: net::request_response_server::connection(srvr, c, true, inactivityTimeout)
		{ }

	public:
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

		virtual rcref<net::request_response_server::request> create_request()	{ return server::default_create_request(this_rcref); }

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
		enum reply_code
		{
			reply_system_status_of_help = 211,
			reply_help_message = 214,

			reply_service_ready = 220,
			reply_closing_channel = 221,
			reply_action_completed = 250,
			reply_non_local_user_forwarded = 251,
			reply_unverified_user_accepted = 252,

			reply_start_mail_input = 354,

			reply_service_not_available = 421,
			reply_mailbox_busy = 450,
			reply_local_error = 451,
			reply_insufficient_storage = 452,
			reply_parameter_refused = 455,

			reply_syntax_error = 500,
			reply_syntax_error_in_parameters = 501,
			reply_command_not_implemented = 502,
			reply_bad_command_sequence = 503,
			reply_parameter_not_implemented = 504,

			reply_mailbox_unavailable = 550,
			reply_user_not_local = 551,
			reply_exceeded_storage_limit = 552,
			reply_mailbox_name_not_allowed = 553,
			reply_transaction_failed = 554,
			reply_parameter_error = 555				// MAIL FROM/RCPT TO parameters not recognized or not implemented
		};

	protected:
		friend class server;
		friend class request;
		friend class connection;

		reply_code m_replyCode;
		const composite_cstring m_text;

		response(const rcref<request>& r, reply_code replyCode, const composite_cstring& text = cstring())
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

		static constexpr size_t max_request_length = 64 * 1024;	// 64K

		composite_buffer			m_bufferedWrite;
		bool						gotCR;
		composite_cstring			m_currentCommand;
		composite_cstring			m_commandParams;
		rcref<delegated_datasink>	m_delegatedSink;
		rcptr<task<void> >	m_coupler;

	private:
		bool process_write(composite_buffer& compBuf)
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
					gotCR = true;
				else if (c == special_characters<char>::LF)
				{
					if (gotCR)
					{
						completeRequest = true;
						break;
					}
				}
				else
				{
					gotCR = false;
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
							begin_response(response::reply_syntax_error, cstring::literal("Command too long"))->complete();
							closing = true;
							break;
						}
					}
				}
			}

			if (completeRequest)
			{
				rcptr<connection> c = m_connection.static_cast_to<connection>();
				if (!c)
					closing = true;
				else
				{
					rcptr<server> srvr = c->get_server().static_cast_to<server>();
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
							(*commandItor)(this_rcref);
						else
							begin_response(response::reply_command_not_implemented, cstring::literal("Command not recognized: \"") + m_currentCommand + cstring::literal("\""))->complete();
					}
				}
				m_currentCommand.clear();
			}

			return !closing;
		}

	public:
		rcref<response> begin_response(response::reply_code replyCode, const composite_cstring& text)
		{
			m_coupler->cancel();
			rcref<response> r = rcnew(bypass_constructor_permission<response>, this_rcref, replyCode, text);
			r->start();
			return r;
		}

		request(const rcref<connection>& c)
			: net::request_response_server::request(c),
			gotCR(false),
			m_delegatedSink(rcnew(delegated_datasink, [r{ this_weak_rcptr }](composite_buffer& b)
			{
				rcptr<request> r2 = r;
				if (!!r2)
					return r2->process_write(b);
				return false;
			}))
		{ }

		virtual void start()
		{
			net::request_response_server::request::start();

			m_coupler = couple(get_datasource(), m_delegatedSink, true, true);
			m_delegatedSink->get_sink_close_event()->dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<request> r2 = r;
				if (!!r2)
					r2->abort();
			});
		}

		composite_cstring get_command_params() const	{ return m_commandParams; }
	};

private:
	static rcref<net::request_response_server::request> default_create_request(const rcref<connection>& c)	{ return rcnew(request, c); }

	rcref<command_handler_map_t>	m_commandHandlerMap;

	virtual rcref<net::server::connection> create_connection(const rcref<net::connection>& ds)
	{
		return rcnew(bypass_constructor_permission<connection>, this_rcref, ds);
	}

public:
	static void default_HELO_handler(const rcref<request>& r)
	{
		composite_cstring params = r->get_command_params();
		if (!params)
			r->begin_response(response::reply_syntax_error_in_parameters, cstring::literal("HELO/EHLO requires domain address"))->complete();
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

				r->begin_response(response::reply_action_completed, str)->complete();
			}
		}
	}

	static void default_NOOP_handler(const rcref<request>& r)
	{
		r->begin_response(response::reply_action_completed, cstring::literal("OK"))->complete();
	}

	static void default_QUIT_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().static_cast_to<connection>();
		if (!!c)
			c->set_connection_reuse(false);
		r->begin_response(response::reply_action_completed, cstring::literal("OK"))->complete();
	}

	static void default_MAIL_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().static_cast_to<connection>();
		if (!!c)	// otherwise, connection is shutting down
		{
			if (!!c->m_reversePath)
				r->begin_response(response::reply_bad_command_sequence, cstring::literal("Sender already specified"))->complete();
			else
			{
				const vector<composite_cstring> params = r->get_command_params().split_on(' ');
				if (!params[0] || !params[0].starts_with(cstring::literal("FROM:<")) || !params[0].ends_with(">", 1))
					r->begin_response(response::reply_parameter_error, cstring::literal("MAIL FROM parameters not recognized or not implemented"))->complete();
				else
				{
					composite_cstring reversePath = params[1];
					reversePath.advance(6);
					reversePath.truncate(1);
					c->m_reversePath = reversePath;
					r->begin_response(response::reply_action_completed, cstring::literal("OK"))->complete();
				}
			}
		}
	}

	static void default_RCPT_handler(const rcref<request>& r)
	{
		rcptr<connection> c = r->get_connection().static_cast_to<connection>();
		if (!!c)	// otherwise, connection is shutting down
		{
			if (!c->m_reversePath)
				r->begin_response(response::reply_bad_command_sequence, cstring::literal("Need MAIL before RCPT"))->complete();
			else
			{
				const vector<composite_cstring> params = r->get_command_params().split_on(' ');
				if (!params[0] || !params[0].starts_with(cstring::literal("TO:<")) || !params[0].ends_with(">", 1))
					r->begin_response(response::reply_parameter_error, cstring::literal("RCPT TO parameters not recognized or not implemented"))->complete();
				else
				{
					composite_cstring recipient = params[1];
					recipient.advance(4);
					recipient.truncate(1);

					// Now to decide if we want to accept the recipient or not
					vector<composite_cstring> parts = recipient.split_on('@');


					c->m_recipientList.append(1, recipient);
					r->begin_response(response::reply_action_completed, cstring::literal("OK"))->complete();
				}
			}
		}
	}

	static rcref<command_handler_map_t> get_default_command_handlers()
	{
		rcref<command_handler_map_t> mapRef = rcnew(command_handler_map_t);
		mapRef->try_insert(cstring::literal("HELO"), &default_HELO_handler);
		mapRef->try_insert(cstring::literal("EHLO"), &default_HELO_handler);
		mapRef->try_insert(cstring::literal("NOOP"), &default_NOOP_handler);
		mapRef->try_insert(cstring::literal("QUIT"), &default_QUIT_handler);
		mapRef->try_insert(cstring::literal("MAIL"), &default_MAIL_handler);
		// RCPT
		// SIZE
		// DATA
		// VRFY
		// EXPN
		// RSET
		return mapRef;
	}

	server()
		:	m_commandHandlerMap(get_default_command_handlers())
	{ } 

	explicit server(const rcref<command_handler_map_t>& commandHandlers)
		:	m_commandHandlerMap(commandHandlers)
	{ }

	~server()
	{ }

	//static constexpr uint16_t inactivity_timeout_in_seconds = 60 * 2;	// 2 minute inactivity timeout
	// The inactivity timeout must be extended by a handler that does something appropriate to extend
	// the lifetime of the connection.  Care should be taken to prevent denial-of-service attacks,
	// such as clients establishing unnecessary connections and leaving them connected, to try to
	// deprive the server of memory or network resources.
};


/*
class client
{
private:
	rcptr<ip::tcp>	m_tcpSocket;
	unsigned short	m_port;

public:
	explicit client(unsigned short port = 25)
		:	m_port(port)
	{ }


//	static rcref<connecter> connect(const vector<address>& addresses, unsigned short port, const rcref<os::io::completion_port>& cp = os::io::completion_port::get())
//	{
//		return rcnew(connecter, addresses, port, cp);
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

