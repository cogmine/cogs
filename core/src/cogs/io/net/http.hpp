//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_HEADER_IO_NET_HTTP
#define COGS_HEADER_IO_NET_HTTP


#include "cogs/env.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/function.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/limiter.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/io/net/ip/tcp.hpp"
#include "cogs/io/net/server.hpp"
#include "cogs/math/chars.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/datetime.hpp"


namespace cogs {
namespace io {
namespace net {


// A URI is immutable.  It's basically used to convert a URI string into its components.
class url
{
private:
	composite_cstring m_scheme;
	composite_cstring m_query;
	composite_cstring m_fragment;
	composite_cstring m_path;
	composite_cstring m_host;
	composite_cstring m_user;
	composite_cstring m_password;
	bool m_useDefaultPort;
	unsigned short m_port;

public:
	url(const composite_cstring& s)
	{
		m_useDefaultPort = true;
		m_fragment = s;
		m_scheme = m_fragment.split_off_before(m_fragment.index_of(':'));
		m_fragment.advance();
		m_path = m_fragment.split_off_before(m_fragment.index_of('?'));
		m_fragment.advance();
		m_query = m_fragment.split_off_before(m_fragment.index_of('#'));
		m_fragment.advance();
		if (m_path.starts_with(cstring::literal("//")))
		{
			m_path.advance(2);
			composite_cstring portString = m_path.split_off_before(m_path.index_of('/'));
			m_path.advance(1);

			m_password = portString.split_off_before(portString.index_of('@'));
			portString.advance();
			m_host = portString.split_off_before(portString.index_of(':'));
			portString.advance();
			if (!!portString)
			{
				m_useDefaultPort = false;
				m_port = portString.to_int<unsigned short>();
			}

			if (!!m_password)
			{
				m_user = m_password.split_off_before(portString.index_of(':'));
				m_password.advance();
			}
		}
	}

	const composite_cstring& get_scheme() const { return m_scheme; }
	const composite_cstring& get_query() const { return m_query; }
	const composite_cstring& get_fragment() const { return m_fragment; }

	const composite_cstring& get_path() const { return m_path; }

	const composite_cstring& get_host() const { return m_host; }
	bool uses_default_port() const { return m_useDefaultPort; }
	unsigned short get_port() const { return m_port; }

	const composite_cstring& get_user() const { return m_user; }
	const composite_cstring& get_password() const { return m_password; }
};


/// @ingroup Net
/// @brief Namespace for HTTP
namespace http {


class chunk_source;
class chunk_sink;

class client;
//class client::request; // client request
//class client::response; // client response

class server;
//class server::connection;
//class server::request;
//class server::response;


// chunk_source is a helper class used by both the client and server http components,
// to parse a datastream with a Transfer-Encoding of 'chunked'.

// chunk_source and chuck_sink are not combined into a single chunk_protocol
// as chunking only occurs in one direction or the other, genernally not in both directions simultaneously.

/// @ingroup Net
/// @brief A datasource filter that decodes the 'chunked' Transfer-Encoding.
class chunk_source : public filter
{
public:
	typedef nonvolatile_map<composite_cstring, composite_cstring, false> trailer_map_t;

private:
	static constexpr size_t max_chunk_size_line_length = 4 * 1024; // 4K should be more than enough
	static constexpr size_t max_trailer_items = 500;

	size_t m_remainingChunk = 0; // 0 if not in a chunk.  Otherwise, number of bytes remaining to be read in the chunk
	composite_cstring m_curLine;
	bool m_lastChunkReceived = false;
	bool m_lastCharWasLF = false;
	trailer_map_t m_trailers;

public:
	virtual rcref<task<composite_buffer> > filtering(composite_buffer& src)
	{
		composite_buffer result;
		COGS_ASSERT(!!src);
		do {
			size_t copySize = m_remainingChunk;
			if (copySize > src.get_length())
				copySize = src.get_length();
			if (copySize) // Some data left in this chunk.  Shovel it.
			{
				result.append(src.split_off_before(copySize));

				m_remainingChunk -= copySize;
				src.advance(copySize);

				if (!src)
					break;

				// otherwise, m_remainingChunk should be 0
				COGS_ASSERT(!m_remainingChunk);
				break;
			}

			// reading a chunk header
			for (;!!src;src.advance(1))
			{
				buffer buf = src.get_inner(0);
				const char c = *(const char*)(buf.get_const_ptr());
				if (c == special_characters<char>::CR) // Ignore CRs
					continue;
				if (m_lastCharWasLF)
				{
					m_lastCharWasLF = false;
					if (c == special_characters<char>::LF) // Terminating blank link.
						break; // Refuse to process this char.  Tells the caller we're done reading.

					if (!m_lastChunkReceived) // haven't seen an empty/last chunk yet.
					{
						// The format of a chunk size line is: chunk-size [ chunk-extension ] CRLF
						// chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )

						vector<composite_cstring> parts = m_curLine.split_on(';');
						m_curLine.clear();

						// For now, just ignore extensions.  TBD

						parts.get_first().trim();
						m_remainingChunk = parts[0].to_int<size_t>(16);
						if (!m_remainingChunk) // 0 byte final chunk indicates the end.
							m_lastChunkReceived = true; // next are trailing headers and a final blank line.
					}
					else // if (!!m_lastChunkReceived)
					{
						if (!cstring::is_white_space(c))
						{ // Only start a new one, if it didn't start with a blank space.  Otherwise, add the blank space.
							if (m_trailers.size() == max_trailer_items)
								break; // Too many options! Refuse to process this char.  Tells the caller we're done reading.

							composite_cstring::position_t pos = m_curLine.position_of(composite_cstring::position_t(0, 0), ':');
							if (pos == m_curLine.get_end_position()) // Bogus header?
								break;

							composite_cstring optionName(m_curLine.subrange(pos));
							composite_cstring optionValue(m_curLine.subrange(m_curLine.get_next_position(pos), m_curLine.get_length()));
							optionName.trim();
							optionValue.trim();
							m_trailers.insert_replace(optionName, optionValue);
							m_curLine.clear();
						}
					}
				}
				if (c == special_characters<char>::LF)
				{
					m_lastCharWasLF = true;
					continue;
				}
				if (m_curLine.get_length() == max_chunk_size_line_length)
					break; // Give up at this character.  The base class will get the message and close.
			}
			// Only continue if there is more data, and m_remainingChunk changed to non-zero
		} while (!!src && !!m_remainingChunk);

		return signaled(result);
	}

	const trailer_map_t& get_trailers() const { return m_trailers; }
};


// chunk_sink is a helper class used by both the client and server http components,
// to output a datastream with a Transfer-Encoding of 'chunked'.

/// @ingroup Net
/// @brief A datasink filter that encodes the 'chunked' Transfer-Encoding.
class chunk_sink : public filter
{
public:
	chunk_sink()
		: filter([](composite_buffer & src)
		{
			composite_buffer result;
			static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
			buffer crlfBuf = buffer::contain(CRLF, 2);
			size_type bufLength = src.get_length();
			result.append(composite_buffer::from_composite_cstring(bufLength.to_cstring(16))); // Chunk length
			result.append(crlfBuf); // CRLF
			result.append(src);     // DATA
			result.append(crlfBuf); // CRLF
			src.clear();
			return signaled(result);
		},
		[]()
		{
			// send empty chunk
			composite_buffer compBuf;
			static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
			buffer crlfBuf = buffer::contain(CRLF, 2);
			buffer zeroBuf = buffer::contain("0", 1);
			compBuf.append(zeroBuf);
			compBuf.append(crlfBuf);
			compBuf.append(crlfBuf);

			// TBD: Send trailers?

			return signaled(compBuf);
		})
	{ }
};


composite_cstring url_encode(const composite_cstring& s);

composite_cstring url_decode(const composite_cstring& s);

//
//class client
//{
//private:
//	composite_cstring m_host;
//	unsigned short m_port;
//	bool m_useSSL;
//	size_t m_maxConnections;
//
//	class connection : public object
//	{
//	private:
//		const rcref<client> m_client;
//		rcref<net::connection> m_netConnection;
//		bool m_reuse;
//
//	public:
//		connection(const rcref<client>& cl, const rcref<net::connection>& c)
//			: m_client(cl),
//			m_netConnection(c)
//		{ }
//
//		const rcref<net::connection>& get_net_connection() const { return m_netConnection; }
//
//		void close_on_recycle() { m_reuse = false; }
//		bool is_connection_reusable() const { return m_reuse; }
//	};
//
//	//rcref<connection> get_connection()
//	//{
//	//}
//
//public:
//	class request_parameters
//	{
//	public:
//		// headers
//		//........
//		// Accept:
//		// Connection: (Close/Keep-Alive)
//		// Content-Length: <numeric>
//		// Content-Type:
//		// Expect:
//		// Date: <date>
//		// Host:
//		// If-Modified-Since:
//		// Range:
//		// Referer:
//		// Transfer-Encoding:
//		// User-Agent:
//
//		// Protocol version (1.0, 1.1)
//		// AllowAutoRedirect: bool
//		// Num automatic redirections before abort
//		// max response header length
//		// max error response length
//		// read/write timeout
//
//		composite_cstring m_path;
//		composite_cstring m_method; // GET, POST, PUT, etc.
//		composite_cstring m_contentType; // Content-Type
//		size_t m_contentLength; // Content-Length: <numeric>
//		boolean m_sendChunked; // Transfer-encoding: chunked
//		boolean m_keepAlive;
//
//		composite_cstring m_accept;
//		composite_cstring m_ifModifiedSince;
//		composite_cstring m_refered;
//		composite_cstring m_userAgent;
//		datetime m_date;
//		boolean m_hasDate;
//		boolean m_hasRange;
//		dynamic_integer m_startRange;
//		dynamic_integer m_endRange;
//
//		request_parameters()
//		{
//			m_path = cstring::literal("/");
//			m_method = cstring::literal("GET");
//			m_keepAlive = true;
//		}
//
//		request_parameters(const request_parameters& src)
//		{
//			m_path = src.m_path;
//			m_method = src.m_method;
//			m_contentType = src.m_contentType;
//			m_contentLength = src.m_contentLength;
//			m_sendChunked = src.m_sendChunked;
//			m_keepAlive = src.m_keepAlive;
//			m_accept = src.m_accept;
//			m_ifModifiedSince = src.m_ifModifiedSince;
//			m_refered = src.m_refered;
//			m_userAgent = src.m_userAgent;
//			m_date = src.m_date;
//			m_hasDate = src.m_hasDate;
//			m_hasRange = src.m_hasRange;
//			m_startRange = src.m_startRange;
//			m_endRange = src.m_endRange;
//		}
//
//		request_parameters& operator=(const request_parameters& src)
//		{
//			m_path = src.m_path;
//			m_method = src.m_method;
//			m_contentType = src.m_contentType;
//			m_contentLength = src.m_contentLength;
//			m_sendChunked = src.m_sendChunked;
//			m_keepAlive = src.m_keepAlive;
//			m_accept = src.m_accept;
//			m_ifModifiedSince = src.m_ifModifiedSince;
//			m_refered = src.m_refered;
//			m_userAgent = src.m_userAgent;
//			m_date = src.m_date;
//			m_hasDate = src.m_hasDate;
//			m_hasRange = src.m_hasRange;
//			m_startRange = src.m_startRange;
//			m_endRange = src.m_endRange;
//			return *this;
//		}
//	};
//
//	class response //: public datasource_proxy
//	{
//	public:
//		//rcref<datasink> get_datasource()
//		//{
//		//}
//	};
//
//	class request
//	{
//	private:
//		request_parameters	m_parameters;
//
//	public:
//		const request_parameters& get_request_parameters() const { return m_parameters; }
//
//		explicit request(const request_parameters& parameters)
//			: m_parameters(parameters)
//		{
//		}
//
//		//rcref<datasink> get_datasink()
//		//{
//		//}
//
//		//rcref<response> commit()
//		//{
//		//	// TODO: prevent double-call
//		//}
//	};
//
//
//	//rcref<request> begin_request(const request_parameters& r)
//	//{
//	//}
//
//	client(composite_cstring host = cstring::literal("localhost"), unsigned short port = 80, bool useSSL = false, size_t maxConnections = 2)
//	{
//		m_host = host;
//		m_port = port;
//		m_useSSL = useSSL;
//		m_maxConnections = maxConnections;
//	}
//
//	static rcref<request> create_request(const composite_cstring& urlString) { return create_request(url(urlString)); }
//
//	static rcref<request> create_request(const url& u)
//	{
//		unsigned short port = 80;
//		bool useSSL = u.get_scheme().equals("https", case_sensitive::no);
//		if (useSSL)
//			port = 443;
//
//		if (!u.uses_default_port())
//			port = u.get_port();
//
//		rcref<client> c = rcnew(client)(u.get_host(), port, useSSL);
//		//return c->begin_request();
//	}
//};


/// @ingroup Net
/// @brief A HTTP server
class server : public net::request_response_server
{
private:
	class connection;
	class request;
	class response;

public:
	typedef function<void(const rcref<request>&)> verb_delegate_t;

	typedef nonvolatile_map<composite_cstring, composite_cstring, false, case_insensitive_comparator<composite_cstring> > header_map_t;
	typedef nonvolatile_map<composite_cstring, verb_delegate_t, true, case_insensitive_comparator<composite_cstring> > verb_handler_map_t;

private:
	static cstring get_version_cstring() { return cstring::literal("cogs::io::net::http::server/1.0"); }

	class connection : public net::request_response_server::connection
	{
	private:
		friend class server;
		friend class request;
		friend class response;

		connection() = delete;
		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

		virtual rcref<net::request_response_server::request> create_request()
		{
			return server::default_create_request(this_rcref);
		}

		connection(const rcref<server>& srvr, const rcref<net::connection>& c, const timeout_t::period_t& inactivityTimeout = timeout_t::period_t(0))
			: net::request_response_server::connection(srvr, c, true, inactivityTimeout)
		{ }
	};

	class response : public net::request_response_server::response
	{
	public:
		enum class status
		{
			continue_status = 100,
			switching_protocols = 101,
			processing = 102, // WebDAV, RFC2518

			ok = 200,
			created = 201,
			accepted = 202,
			nonauthoritative_information = 203,
			no_content = 204,
			reset_content = 205,
			multi_status = 207, // WebDAV, RFC4918
			im_used = 226, // WebDAV, RFC3229

			multiple_choices = 300,
			moved_permanently = 301,
			found = 302,
			see_other = 303,
			not_modified = 304,
			use_proxy = 305,
			temporary_redirect = 307,

			bad_request = 400,
			unauthorized = 401,
			payment_required = 402,
			forbidden = 403,
			not_found = 404,
			method_not_allowed = 405,
			not_acceptable = 406,
			proxy_authentication_required = 407,
			request_timeout = 408,
			conflict = 409,
			gone = 410,
			length_required = 411,
			precondition_failed = 412,
			request_entity_too_large = 413,
			request_uri_too_large = 414,
			unsupported_media_type = 415,
			request_range_not_satisfiable = 416,
			expectation_failed = 417,
			iam_a_teapot = 418, // joke
			unprocessable_entity = 422, // WebDAV, RFC4918
			locked = 423, // WebDAV, RFC4918
			failed_dependency = 424, // WebDAV, RFC4918
			unordered_collection = 425, // RFC3649
			upgrade_required = 426, // RFC2817
			no_response = 444, // Nginx
			retry_with = 449,
			blocked_by_windows_parental_controls = 450,
			client_closed_request = 499,

			internal_server_error = 500,
			not_implemented = 501,
			bad_gateway = 502,
			service_unavailable = 503,
			gateway_timeout = 504,
			http_version_not_supported = 505,
			variant_also_negotiates = 506, // RFC2295
			insufficient_storage = 507, // WebDAV, RFC4918
			bandwidth_limit_exceeded = 509, // Apache bw/limited extension
			not_extended = 510 // RFC2274
		};

		enum class mode
		{
			// A response of a known fixed size allows the connection to be
			// reused for additional requests.  The size is sent in header
			// as Content-Length
			fixed_size = 0,

			// A response of an unknown size can be 'chunked', into
			// chunks of known sizes, with a terminating empty chunk.
			// After the empty chunk, the connection can be reused for
			// additional requests.
			// Note: If the last request indicated HTTP/1.0, then chunking is
			// not supported.  "Connection: close" will be sent.
			chunked = 1,

			// A response may leave the connection open indefinitely, such as
			// to stream dynamic content.  These connections actually
			// indicate a "Connection: close" header, to tell the client
			// that additional requests will not be possible on this connection.
			raw = 2
		};

	private:
		mode m_mode;
		size_t m_contentLength;
		bool m_chunkOutgoing;

		rcptr<chunk_sink> m_chunkSink;
		rcptr<io::limiter> m_sinkContentLengthLimiter;
		rcptr<datasink> m_sink; // Will vary depending on whether chunking filter is used
		const status m_statusCode;
		const composite_cstring m_statusPhrase;
		const bool m_reuseConnection;

	protected:
		friend class server;
		friend class connection;
		friend class request;

		response(const rcref<request>& r, status code, const composite_cstring& statusPhrase, mode m, size_t contentLength, bool reuseConnection)
			: net::request_response_server::response(r),
			m_mode(m),
			m_contentLength(contentLength),
			m_statusCode(code),
			m_statusPhrase(statusPhrase),
			m_reuseConnection(reuseConnection)
		{ }

		virtual void start()
		{
			net::request_response_server::response::start();
			m_sink = net::request_response_server::response::get_datasink();
			rcptr<connection> c = get_connection().template static_cast_to<connection>();
			if (!!c)
			{
				static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
				buffer crlfBuf = buffer::contain(CRLF, 2);

				// Start of HTTP Server Response.

				// Send Status-Line.  (rfc2616,  6.1 Status-Line)
				// Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

				cstring httpVersionStr = cstring::literal("HTTP/1.1 ");
				m_sink->write(buffer(httpVersionStr.get_const_ptr(), httpVersionStr.get_length()));

				int_type statusCode = (int)m_statusCode;
				composite_cstring statusCodeString = statusCode.to_cstring();
				statusCodeString.append(' ');
				m_sink->write(composite_buffer::from_composite_cstring(statusCodeString));
				m_sink->write(composite_buffer::from_composite_cstring(m_statusPhrase));
				m_sink->write(crlfBuf);

				const rcref<request>& r = get_request().template static_cast_to<request>();
				rcref<header_map_t> headers = r->m_responseHeaders;

				// Date header.  (rfc2616,  14.18 Date)
				headers->insert_replace(cstring::literal("Date"), datetime::now().to_cstring(datetime::format::RFC1123DateTime));

				if (!m_reuseConnection || m_mode == mode::raw)
					c->set_connection_reuse(false);
				if (!c->is_connection_reusable())
				{ // If connection is not persistent, indicate Connection: close
					// Connection header.  (rfc2616,  14.10 Connection)
					headers->insert_replace(cstring::literal("Connection"), cstring::literal("close"));
				}

				m_chunkOutgoing = (m_mode == mode::chunked);
				if (m_chunkOutgoing)
				{
					// Transfer-Encoding header.  (rfc2616,  14.41 Transfer-Encoding)
					headers->insert_replace(cstring::literal("Transfer-Encoding"), cstring::literal("chunked"));
					m_chunkSink = rcnew(chunk_sink);
				}

				if (m_mode == mode::fixed_size)
				{
					if (m_contentLength != 0)
					{
						size_type contentLengthNum = m_contentLength;

						// Content-Length header.  (rfc2616,  14.13 Content-Length)
						headers->insert_replace(cstring::literal("Content-Length"), contentLengthNum.to_cstring());
						m_sinkContentLengthLimiter = rcnew(limiter)(m_contentLength);
					}
				}

				static constexpr char colonSpace[2] = { ':', ' ' };
				buffer colonBuf = buffer::contain(colonSpace, 2);

				// Write all headers to the stream.

				header_map_t::iterator itor = headers->get_first();
				while (!!itor)
				{
					const composite_cstring& fieldName = itor.get_key();
					const composite_cstring& fieldValue = *itor;
					m_sink->write(composite_buffer::from_composite_cstring(fieldName));
					m_sink->write(colonBuf);
					m_sink->write(composite_buffer::from_composite_cstring(fieldValue));
					m_sink->write(crlfBuf);
					++itor;
				}
				m_sink->write(crlfBuf);
				headers->clear();

				if (!!m_chunkSink)
				{
					couple(m_chunkSink.dereference(), m_sink.dereference(), true);
					m_sink = m_chunkSink.dereference();
				}
				else if (!!m_sinkContentLengthLimiter)
				{
					couple(m_sinkContentLengthLimiter.dereference(), m_sink.dereference(), true);
					m_sink = m_sinkContentLengthLimiter.dereference();
				}
			}
		}

		virtual rcref<datasink> get_datasink() const { return m_sink.dereference(); }

		virtual void completing()
		{
			if (!!m_chunkSink)
				m_chunkSink->close_sink();
		}
	};

	class request : public net::request_response_server::request
	{
	protected:
		friend class response;
		friend class connection;
		friend class server;

		static constexpr size_t max_request_length = 64 * 1024; // 64K
		static constexpr size_t max_header_item_length = 8 * 1024; //  8K

		// Max number of header option items
		static constexpr size_t max_header_items = 500;

		composite_cstring m_requestLine;
		composite_cstring m_method;
		composite_cstring m_requestURI;
		composite_cstring m_httpVersionString;
		uint16_t m_httpVersionMajor;
		uint16_t m_httpVersionMinor;
		bool m_supportOutgoingChunking;

		rcref<header_map_t> m_requestHeaders;
		rcref<header_map_t> m_responseHeaders;

		size_t m_contentLength;

		composite_buffer m_bufferedWrite;
		int m_CRLFs; // 0 = none found, 1 = CR, 2 = CRLF, 3 = CRLFCR, (until CRLFCRLF)

		rcref<datasink> m_sink;

		rcptr<chunk_source> m_chunkSource;
		rcptr<io::limiter> m_sourceContentLengthLimiter;
		rcptr<datasource> m_source;

		rcptr<task<void> > m_coupler;

	private:
		rcref<task<bool> > process_write(composite_buffer& compBuf)
		{
			bool closing = false;

			// Buffer all input until the pattern <CR><LF><CR><LF> is found.
			// Also check for max request size.
			bool completeRequest = false;
			composite_buffer::position_t pos = compBuf.get_first_position();
			composite_buffer::position_t endPos = compBuf.get_end_position();
			size_t requestSize = m_bufferedWrite.get_length();
			for (;;)
			{
				if (++requestSize > max_request_length)
				{
					error_reply(response::status::bad_request, cstring::literal("Request Too Large"))->complete();
					closing = true; // Don't bother catching up with the buffer, we're closing it before it finishes.
					break;
				}

				unsigned char c = compBuf[pos];

				if (c == special_characters<char>::CR)
				{
					if (m_CRLFs == 2)
						m_CRLFs = 3;
					else
						m_CRLFs = 1;
				}
				else if (c != special_characters<char>::LF)
					m_CRLFs = 0;
				else if (m_CRLFs == 1)
					m_CRLFs = 2;
				else if (m_CRLFs != 3)
					m_CRLFs = 0;
				else
					completeRequest = true;

				pos = compBuf.get_next_position(pos);
				if (completeRequest || pos == endPos)
					break;
			}

			m_bufferedWrite.append(compBuf.split_off_before(pos));

			if (completeRequest)
			{
				enum class state
				{
					process_request_line = 0,
					process_header = 1,
					process_message_body = 2
				};

				bool lastCharWasLF = false;
				state curState = state::process_request_line;
				for (;;)
				{
					if (curState == state::process_request_line)
					{
						for (; !!m_bufferedWrite; m_bufferedWrite.advance(1))
						{
							buffer buf = m_bufferedWrite.get_inner(0);
							const char c = ((const char*)(buf.get_const_ptr()))[0];
							if (c == special_characters<char>::CR) // Ignore CR's.  No CR or LF is allowed except in the final CRLF sequence.
								continue;
							if (c != special_characters<char>::LF)
							{
								if ((c != special_characters<char>::DEL) && (c != special_characters<char>::BS))
									m_requestLine.append(c);
								else
								{
									if (m_requestLine.get_length() > 0)
										m_requestLine.truncate(1);
								}
								continue;
							}
							else // (c == special_characters<char>::LF)
							{
								if (m_requestLine.is_empty()) // ignore leading blank lines
									continue;

								m_bufferedWrite.advance(1);
								vector<composite_cstring> parts = m_requestLine.split_on(' ');
								if (parts.get_length() == 3)
								{
									m_method = parts[0];
									m_requestURI = parts[1];
									m_httpVersionString = parts[2];
									m_httpVersionString.advance(5); // Skip past "HTTP/"
									parts = m_httpVersionString.split_on('.');
									if (parts.get_length() == 2)
									{
										m_httpVersionMajor = parts[0].to_int<uint16_t>();
										m_httpVersionMinor = parts[1].to_int<uint16_t>();
										curState = state::process_header;
										break;
									}
								}
								error_reply()->complete();
							}
							break;
						}
					}

					lastCharWasLF = true;
					if (curState == state::process_header)
					{
						composite_cstring currentHeader;
						for (; !!m_bufferedWrite; m_bufferedWrite.advance(1))
						{
							buffer buf = m_bufferedWrite.get_inner(0);
							const char c = ((const char*)(buf.get_const_ptr()))[0];
							if (c == special_characters<char>::CR) // Ignore CR's.  No CR or LF is allowed except in the final CRLF sequence.
								continue;
							if (lastCharWasLF)
							{
								lastCharWasLF = false;
								if (c == special_characters<char>::LF)
								{
									// A blank line indicates the end of headers.  Message body is next.
									curState = state::process_message_body;
									m_bufferedWrite.advance(1);
									break;
								}
								if (!cstring::is_white_space(c))
								{
									if (!!currentHeader)
									{
										// Only start a new one, if it didn't start with a blank space.  Otherwise, add the blank space.
										if (m_requestHeaders->size() == max_header_items)
										{ // Too many options!
											error_reply()->complete();
											break;
										}

										size_t i2 = currentHeader.index_of(':');
										if (i2 == (size_t)-1) // Bogus header?
										{
											error_reply()->complete();
											break;
										}
										composite_cstring optionName(currentHeader.subrange(i2));
										composite_cstring optionValue(currentHeader.subrange(i2 + 1, currentHeader.get_length()));
										optionName.trim();
										optionValue.trim();
										m_requestHeaders->insert_replace(optionName, optionValue);
										currentHeader.clear();
									}
								}
							}
							if (c == special_characters<char>::LF)
								lastCharWasLF = true;
							else
							{
								if (currentHeader.get_length() != max_header_item_length)
								{
									currentHeader.append(c);
									continue;
								}
								error_reply()->complete();
								break;
							}
						}
					}

					if (curState == state::process_message_body) // Done reading.  Validate the request.
					{
						// Before we receive a message body, we need to validate the request.
						if (m_httpVersionMajor != 1)
							error_reply(response::status::http_version_not_supported, cstring::literal("HTTP Version Not Supported"))->complete(); // TDB: bad_request
						else
						{
							bool useIncomingChunking = false;
							header_map_t::iterator itor = m_requestHeaders->find(cstring::literal("Transfer-Encoding"));
							bool abort = false;
							if (!!itor)
							{
								if (*itor == cstring::literal("chunked"))
									useIncomingChunking = true;
								else
									abort = true;
							}

							if (abort)
								error_reply(response::status::not_implemented, cstring::literal("Transfer-Encoding \"") + *itor + cstring::literal("\" not implemented"))->complete();
							else
							{
								rcptr<connection> c = m_connection.template static_cast_to<connection>();
								if (!c)
								{
									closing = true;
									break;
								}

								if ((m_httpVersionMajor == 1) && (m_httpVersionMinor == 0))
									m_supportOutgoingChunking = false;

								itor = m_requestHeaders->find(cstring::literal("Connection"));
								if (!!itor)
								{
									if (itor->starts_with(cstring::literal("close"), case_sensitive::no))
										c->set_connection_reuse(false);

									// Repeat Keep-Alive back to the client.
									if (itor->starts_with(cstring::literal("Keep-Alive"), case_sensitive::no))
										m_responseHeaders->insert_replace(cstring::literal("Connection"), cstring::literal("Keep-Alive"));
								}

								if (useIncomingChunking)
								{
									m_chunkSource = rcnew(chunk_source);
									couple(m_source.dereference(), m_chunkSource.dereference(), true);
									m_source = m_chunkSource;
								}
								else
								{
									itor = m_requestHeaders->find(cstring::literal("Content-Length"));
									if (!!itor) // if known content length, add a limiter.
									{
										m_contentLength = itor->to_int<size_t>();
										m_sourceContentLengthLimiter = rcnew(limiter)(m_contentLength);

										couple(m_source.dereference(), m_sourceContentLengthLimiter.dereference(), true);
										m_source = m_sourceContentLengthLimiter;
									}
								}

								// Pass the request on to handlers for each HTTP Verb
								rcptr<io::net::server> netSrvr = c->get_server();
								if (!netSrvr)
								{
									closing = true;
									break;
								}
								rcref<server> srvr = netSrvr.template static_cast_to<server>().dereference();

								verb_handler_map_t::iterator verbItor = srvr->m_verbHandlerMap->find(m_method);
								if (!!verbItor)
									(*verbItor)(this_rcref);
								else
								{
									m_responseHeaders->insert_replace(cstring::literal("Allow"), get_allow_string());
									error_reply()->complete();
								}
							}
						}
					}
					break;
				}
			}

			return signaled(closing);
		}

		virtual void start()
		{
			net::request_response_server::request::start();
			m_coupler = couple(get_datasource(), m_sink, true);
			m_sink->get_sink_close_event().dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<request> r2 = r;
				if (!!r2)
					r2->abort();
			});
		}

	protected:
		explicit request(const rcref<connection>& c)
			: net::request_response_server::request(c),
			m_supportOutgoingChunking(true),
			m_requestHeaders(rcnew(header_map_t)),
			m_responseHeaders(rcnew(header_map_t)),
			m_contentLength(0),
			m_CRLFs(0),
			m_sink(rcnew(datasink)([r{ this_weak_rcptr }](composite_buffer& b)
			{
				rcptr<request> r2 = r;
				if (!r2)
					return signaled(true);
				return r2->process_write(b);
			}))
		{
			// setup default response headers
			m_responseHeaders->insert_replace(cstring::literal("Server"), server::get_version_cstring());
			m_responseHeaders->insert_replace(cstring::literal("Content-Type"), cstring::literal("text/html; charset=iso-8859-1"));
			m_source = net::request_response_server::request::get_datasource();
		}

	public:
		virtual rcref<datasource> get_datasource() const { return m_source.dereference(); }

		void send_100_continue()
		{
			rcptr<connection> c = get_connection().template static_cast_to<connection>();
			if (!!c)
			{
				if (expects_continue())
				{
					response::status code = response::status::continue_status;
					cstring statusPhrase = cstring::literal("Continue");

					rcptr<datasink> snk = c->get_net_connection();

					// Start of HTTP Server Response.

					// Send Status-Line.  (rfc2616,  6.1 Status-Line)
					// Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

					cstring httpVersionStr = cstring::literal("HTTP/1.1 ");
					snk->write(buffer(httpVersionStr.get_const_ptr(), httpVersionStr.get_length()));

					int_type statusCode = (int)code;
					composite_cstring statusCodeString = statusCode.to_cstring();
					statusCodeString.append(' ');
					snk->write(composite_buffer::from_composite_cstring(statusCodeString));
					snk->write(composite_buffer::from_composite_cstring(statusPhrase));

					static constexpr char CRLF[2] = { special_characters<char>::CR, special_characters<char>::LF };
					buffer crlfBuf = buffer::contain(CRLF, 2);
					snk->write(crlfBuf);
				}
			}
		}

		rcref<response> simple_reply(const composite_cstring& body = composite_cstring(), response::status code = response::status::ok, const composite_cstring& statusPhrase = cstring::literal("OK"), bool reuseConnection = true)
		{
			rcref<response> r = begin_response(response::mode::fixed_size, body.get_length(), code, statusPhrase, reuseConnection);
			r->get_datasink()->write(composite_buffer::from_composite_cstring(body));
			return r;
		}

		rcref<response> simple_reply(response::status code, const composite_cstring& statusPhrase = cstring::literal("OK"), bool reuseConnection = true)
		{
			return simple_reply(composite_cstring(), code, statusPhrase, reuseConnection);
		}

		rcref<response> error_reply(response::status code = response::status::bad_request, const composite_cstring& statusPhrase = cstring::literal("Bad Request"))
		{
			int_type statusCode = (int)code;
			composite_cstring statusCodeString = statusCode.to_cstring();
			statusCodeString.append(' ');

			cstring body1 = cstring::literal("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>");
			cstring body2 = cstring::literal("</title></head><body><h1>");
			cstring body3 = cstring::literal("</h1><p>Your browser sent a request this server could not understand.<br /></p><hr><address>");
			cstring body4 = cstring::literal("</address></body></html>");

			composite_cstring body = body1;
			body += statusCodeString;
			body += statusPhrase;
			body += body2;
			body += statusPhrase;
			body += body3;
			body += server::get_version_cstring();
			body += body4;
			return simple_reply(body, code, statusPhrase, false);
		}

		rcref<response> begin_response(response::mode m, size_t contentLength, response::status code = response::status::ok, const composite_cstring& statusPhrase = cstring::literal("OK"), bool reuseConnection = true)
		{
			if ((m == response::mode::chunked) && !m_supportOutgoingChunking)
				m = response::mode::raw;

			m_coupler->cancel();
			rcref<response> r = rcnew(response)(this_rcref, code, statusPhrase, m, contentLength, reuseConnection);
			r->start();
			return r;
		}

		const composite_cstring& get_URI() const { return m_requestURI; }
		rcref<const header_map_t> get_request_headers() { return m_requestHeaders; }
		const rcref<header_map_t>& get_response_headers() { return m_responseHeaders; }

		composite_cstring get_allow_string() const
		{
			composite_cstring allVerbs;
			rcptr<connection> c = m_connection.template static_cast_to<connection>();
			if (!!c)
			{
				rcptr<io::net::server> netSrvr = c->get_server();
				if (!!netSrvr)
				{
					rcref<server> srvr = netSrvr.template static_cast_to<server>().dereference();
					verb_handler_map_t::iterator verbItor = srvr->m_verbHandlerMap->get_first();
					while (!!verbItor)
					{
						if (!allVerbs.is_empty())
							allVerbs.append(cstring::literal(", "));
						allVerbs.append(verbItor.get_key());
						++verbItor;
					}
				}
			}
			return allVerbs;
		}

		bool expects_continue() const
		{
			header_map_t::iterator itor = m_requestHeaders->find(cstring::literal("Expect"));
			return !!itor && itor->equals(cstring::literal("100-continue"), case_sensitive::no);
		}
	};

private:
	static rcref<net::request_response_server::request> default_create_request(const rcref<connection>& c)
	{
		return rcnew(request)(c);
	}

	rcref<verb_handler_map_t> m_verbHandlerMap;

	server(const server&) = delete;

	virtual rcref<net::server::connection> create_connection(const rcref<net::connection>& ds)
	{
		return rcnew(connection)(this_rcref, ds);// , make_measure<seconds>(inactivity_timeout_in_seconds));
	}

	static void default_get_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_post_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_head_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_put_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_delete_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_trace_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_options_handler(const rcref<request>& r)
	{
		r->get_response_headers()->insert_replace(cstring::literal("Allow"), r->get_allow_string());
		const composite_cstring& requestURI = r->get_URI();
		if (requestURI == cstring::literal("*"))
		{
			// OPTIONS *
			// Basically just an HTTP ping
			r->simple_reply()->complete();
		}
		else
		{
			// ??
			r->simple_reply()->complete();
		}
	}

	static void default_connect_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static void default_patch_handler(const rcref<request>& r)
	{
		r->error_reply(response::status::not_implemented, cstring::literal("Not Implemented"))->complete();
	}

	static rcref<verb_handler_map_t> get_default_verb_handlers()
	{
		rcref<verb_handler_map_t> mapRef = rcnew(verb_handler_map_t);
		mapRef->try_insert(cstring::literal("CONNECT"), &default_connect_handler);
		mapRef->try_insert(cstring::literal("DELETE"), &default_delete_handler);
		mapRef->try_insert(cstring::literal("GET"), &default_get_handler);
		mapRef->try_insert(cstring::literal("HEAD"), &default_head_handler);
		mapRef->try_insert(cstring::literal("OPTIONS"), &default_options_handler);
		mapRef->try_insert(cstring::literal("PATCH"), &default_patch_handler);
		mapRef->try_insert(cstring::literal("POST"), &default_post_handler);
		mapRef->try_insert(cstring::literal("PUT"), &default_put_handler);
		mapRef->try_insert(cstring::literal("TRACE"), &default_trace_handler);
		return mapRef;
	}

public:
	server()
		: m_verbHandlerMap(get_default_verb_handlers())
	{ }

	explicit server(const rcref<verb_handler_map_t>& verbHandlers)
		: m_verbHandlerMap(verbHandlers)
	{ }

	//static constexpr uint16_t inactivity_timeout_in_seconds = 60 * 2; // 2 minute inactivity timeout
	// The inactivity timeout must be extended by a handler that does something appropriate to extend
	// the lifetime of the connection.  Care should be taken to prevent denial-of-service attacks,
	// such as clients establishing unnecessary connections and leaving them connected, to try to
	// deprive the server of memory or network resources.
};


}
}
}
}


#endif
