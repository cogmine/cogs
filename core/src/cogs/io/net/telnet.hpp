//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, NeedsTesting

#ifndef COGS_HEADER_IO_NET_TELNET
#define COGS_HEADER_IO_NET_TELNET


#include "cogs/collections/container_queue.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/env.hpp"
#include "cogs/io/datastream_protocol.hpp"


namespace cogs {
namespace io {
namespace net {


// Adapts a datastream, usually TCP at remote port 23

/// @ingroup Net
/// @brief Implements the telnet protocol.
class telnet : public datastream_protocol
{
public:
	// Interface for terminal emulation that supports telnet options
	class terminal
	{
	private:
		weak_rcptr<telnet>	m_telnet;

		friend class telnet;

	public:
		// notifications/requests
		// NOP by default
		virtual void telnet_are_you_there()		{}	// Called when remote party sends an AYT req.
													// This party needs to respond with something to prove they are there.
		virtual void telnet_interrupt_process() {}	// Received an Interrupt Process (IP) message from remote party
		virtual void telnet_abort_output()		{}	// Received an Abort Output (AO) message from remote party
		virtual void telnet_erase_char()		{}	// Received an Erase Character (EC) message from remote party
		virtual void telnet_erase_line()		{}	// Received an Erase Line (EL) message from remote party
		virtual void telnet_break()				{}	// Received a Break message from remote party

		virtual bool telnet_request_echo(bool echoOn)	{ return false; }	// received request to set echo state
		virtual bool telnet_notify_echo(bool echoOn)	{ return echoOn; }	// received request to set echo state

		virtual cstring get_telnet_terminal_type()		{ return cstring::literal("UNKNOWN"); }

		virtual void get_window_size(uint16_t& width, uint16_t& height)	{ }

		// Terminal utils
		void send_window_size(uint16_t width, uint16_t height)
		{
			rcptr<telnet> t = m_telnet;
			if (!!t)
				t->send_window_size(width, height);
		}
	};

private:
	weak_rcptr<terminal>	m_terminal;
	volatile buffer			m_recvBuffer;

	static constexpr uint8_t IAC =		255;
	static constexpr uint8_t DONT =		254;
	static constexpr uint8_t DO =		253;
	static constexpr uint8_t WONT =		252;
	static constexpr uint8_t WILL =		251;
	static constexpr uint8_t SB =		250;
	static constexpr uint8_t GA =		249;
	static constexpr uint8_t EL =		248;
	static constexpr uint8_t EC =		247;
	static constexpr uint8_t AYT =		246;
	static constexpr uint8_t AO =		245;
	static constexpr uint8_t IP =		244;
	static constexpr uint8_t BRK =		243;
	static constexpr uint8_t DATAMARK =	242;
	static constexpr uint8_t NOP =		241;
	static constexpr uint8_t SE =		240;
	static constexpr uint8_t SEND =		1;
	static constexpr uint8_t IS =		0;

	static constexpr uint8_t TELOPT_BINARY =		0;
	static constexpr uint8_t TELOPT_ECHO =			1;
	static constexpr uint8_t TELOPT_SGA =			3;	// Suppress Go Ahead. 
	static constexpr uint8_t TELOPT_STATUS =		5;
	static constexpr uint8_t TELOPT_TIMING =		6;
	static constexpr uint8_t TELOPT_RCTE =			7;	
	static constexpr uint8_t TELOPT_NAOCRD =		10;
	static constexpr uint8_t TELOPT_NAOHTS =		11;
	static constexpr uint8_t TELOPT_NAOHTD =		12;
	static constexpr uint8_t TELOPT_NAOFFD =		13;
	static constexpr uint8_t TELOPT_NAOVTS =		14;
	static constexpr uint8_t TELOPT_NAOVTD =		15;
	static constexpr uint8_t TELOPT_NAOLFD =		16;
	static constexpr uint8_t TELOPT_EXTEND_ASCII =	17;	// WILL, DO
	static constexpr uint8_t TELOPT_LOGOUT =		18;	// 
	static constexpr uint8_t TELOPT_BM =			19;	// Byte Macro
	static constexpr uint8_t TELOPT_DET =			20;	// Data Entry Terminal
	static constexpr uint8_t TELOPT_SUPDUP =		21;	// SUPDUP terminal? RFC734
	static constexpr uint8_t TELOPT_SUPDUPOUTPUT =	22;	// SUPDUP terminal within existing term? RFC749
	static constexpr uint8_t TELOPT_SENDLOCATION =	23;	// Send location string
	static constexpr uint8_t TELOPT_TTYPE =			24;	// Terminal Type - RFC1091
	static constexpr uint8_t TELOPT_EOR =			25;	// Necessary?
	static constexpr uint8_t TELOPT_TUID =			26;	// TAC?  - Anyone still use this?
	static constexpr uint8_t TELOPT_OUTMRK =		27;	// RFC933
	static constexpr uint8_t TELOPT_TTYLOC =		28;	// Terminal ID number
	static constexpr uint8_t TELOPT_3270REGIME =	29;	// 3270 terminal?
	static constexpr uint8_t TELOPT_X3PAD =			30;	// Support X.3-PAD
	static constexpr uint8_t TELOPT_NAWS =			31;	// Negotiate about window size.
	static constexpr uint8_t TELOPT_TERMSPEED =		32;	// Not meaningful anymore
	static constexpr uint8_t TELOPT_FLOWCONTROL =	33;	// Not meaningful anymore
	static constexpr uint8_t TELOPT_LINEMODE =		34;	// Line edit mode - Lots to do there
	static constexpr uint8_t TELOPT_XDISPLOC =		35;	// X-Windows display addr
	static constexpr uint8_t TELOPT_AUTHENTICATION =37;	// RFC2941
	static constexpr uint8_t TELOPT_ENCRYPT =		38;	// RFC2946
	static constexpr uint8_t TELOPT_NEWENVIRON =	39;	// Environment options
	static constexpr uint8_t TELOPT_TN3270E =		40;	// TN3270 Enchancements	RFC2355
	static constexpr uint8_t TELOPT_XAUTH =			41;	// XAUTH?
	static constexpr uint8_t TELOPT_CHARSET =		42;	// RFC2066
	static constexpr uint8_t TELOPT_RSP =			42;	// Remote Serial Port
	static constexpr uint8_t TELOPT_COMPORTOPTION =	44;	// Not meaningful anymore
	static constexpr uint8_t TELOPT_SLE =			45;	// Suppress Local Echo
	static constexpr uint8_t TELOPT_STARTTLS =		46;	// Start TLS
	static constexpr uint8_t TELOPT_KERMIT =		47;	// Kermit
	static constexpr uint8_t TELOPT_SENDURL =		48;	// Send URL
	static constexpr uint8_t TELOPT_FORWARDX =		49;	// Forward X?
	static constexpr uint8_t TELOPT_EXOPL =			255;

	int		m_parserState;
	uint8_t m_optionVerb;

	uint8_t	m_myNegotiationState[256];		// A negotiation state per option
	uint8_t	m_theirNegotiationState[256];	// A negotiation state per option

	// negotiation state values:
	//
	// 0 = news to me, I assume they wouldn't use it (6)
	// 1 = just sent DO/WILL   without provocation, waiting
	// 2 = just sent DONT/WONT without provocation, waiting
	// 3 = just sent DONT/WONT response to WILL/DO, waiting
	// 4 = just sent DO/WILL   response to WONT/DONT, waiting
	// 5 = We've decided that I/they WILL/DO
	// 6 = We've decided that I/they WONT/DONT

	cstring	m_incomingSB;
	uint8_t	m_option;

	bool	m_sendNAWS;

	void handle_option(bool response)
	{
		uint8_t msg[3];
		msg[0] = IAC;
		msg[2] = m_option;
		bool pos = (m_optionVerb == DO) || (m_optionVerb == WILL);
		bool asking = (m_optionVerb == DO) || (m_optionVerb == DONT);

		uint8_t* state;
		if (asking)
		{
			state = m_myNegotiationState;
			msg[1] = WONT;
			if (response)
				msg[1] = WILL;
		}
		else
		{
			state = m_theirNegotiationState;
			msg[1] = DONT;
			if (response)
				msg[1] = DO;
		}

		if (pos)
		{
			switch (*state)
			{
			case 0:	// unsolicited
			case 6:	// unsolicited, but already discussed
			case 2:	// I just said no, and they are disagreeing.
			case 3:	// I just said no, and they are disagreeing.
				get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
			case 1:	// Said I would, this must be the response
			case 4:	// They came around.
				*state = response ? 6 : 3;
			case 5:	// must not respond when already in this mode
				break;
			default:
				COGS_ASSERT(false);
				break;
			}
		}
		else
		{
			switch (*state)
			{
			case 0:	// unsolicitied
			case 1:	// Was going to, but was just told not to. Confirm.
			case 4:	// I just said I would, and they are disagreeing.
			case 5:	// Already agreed I would, they changed their mind.
				get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
			case 2:	// Already said I wont, this was a response.
			case 3:	// They came around
				*state = response ? 5 : 4;
			case 6:	// We already discussed this, no change, ignored.
				break;
			default:
				COGS_ASSERT(false);
				break;
			}
		}
	}

	virtual composite_buffer filtering_source(composite_buffer& src)
	{
		composite_buffer result;
		composite_buffer::const_iterator itor = src.get_first_const_iterator();
		while (!!itor)
		{
			uint8_t c = *itor;
			switch (m_parserState)
			{
			case 0:	// Normal state
			{
				if (c == IAC)
				{
					// always eat the first IAC.
					result.append(src.split_off_before(itor.get_position()));
					itor = src.get_first_const_iterator();
					m_parserState = 1;
				}
				else
					++itor;
				break;
			}
			case 5:	// SB content
			{
				if (c == IAC)	// Two in a row means the value itself
				{
					m_incomingSB.append(IAC);
					m_parserState = 4;
					++itor;
					break;
				}
				if (c == SE)	// sub neg is over
				{
					// I suppose once we support some options that have SBs, that could go here

					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				// Looks like SB was interrupt with a new IAC.
				// Fall through to state 1
			}
			case 1:	// Got an IAC
			{
				switch (c)
				{
				case IAC:	// second IAC indicates it's intended to be passed through.
				{
					m_parserState = 0;
					++itor;
					break;
				}
				case DO:
				case DONT:
				case WILL:
				case WONT:
				{
					m_optionVerb = c;
					m_parserState = 2;
					++itor;
					break;
				}
				case SB:
				{
					m_optionVerb = c;
					m_parserState = 3;
					++itor;
					break;
				}
				case AYT:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_are_you_there();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case AO:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_abort_output();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case EC:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_erase_char();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case EL:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_erase_line();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case BRK:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_break();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case IP:
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
						term->telnet_interrupt_process();
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
					break;
				}
				case GA:		// Go-Ahead is unnecessary. We won't ever be using any half-duplex connections.
				case NOP:
				default:
				{
					m_parserState = 0;
					++itor;
					src.set_to_subrange(itor.get_position());
					itor = src.get_first_const_iterator();
				}
				}
				break;
			}
			case 2:	// Receive option name - Pass option on to handler
			{
				m_option = c;
				switch (m_option)
				{
				case TELOPT_BINARY:		// always binary on
				case TELOPT_SGA:		// always SGA
				{
					handle_option(true);
					break;
				}
				case TELOPT_ECHO:	// ask terminal
				{
					rcptr<terminal> term = m_terminal;
					if (!!term)
					{
						bool b;
						if ((m_optionVerb == DO) || (m_optionVerb == DONT))
							b = term->telnet_request_echo(m_optionVerb == DO);
						else
							b = term->telnet_notify_echo(m_optionVerb == WILL);
						handle_option(b);
					}
					break;
				}
				case TELOPT_TTYPE:
				{
					if (m_optionVerb == DO)
					{
						handle_option(true);
						rcptr<terminal> term = m_terminal;
						cstring termType = (!!term) ? term->get_telnet_terminal_type() : cstring::literal("UNKNOWN");
						uint8_t msg[4] = { IAC, SB, TELOPT_TTYPE, IS };
						get_sink_filter()->bypass(buffer((char*)&msg[0], 4));
						get_sink_filter()->bypass(encode_buffer_const(buffer(&termType[0], termType.get_length())));
						//msg[0] = IAC;
						msg[1] = SE;
						get_sink_filter()->bypass(buffer((char*)&msg[0], 2));
					}
					else
						handle_option(m_optionVerb == WILL);
					break;
				}
				case TELOPT_NAWS:
				{
					if (m_optionVerb == DONT)
					{
						m_sendNAWS = false;
						handle_option(false);
					}
					else if (m_optionVerb == DO)
					{
						m_sendNAWS = true;
						handle_option(true);
						rcptr<terminal> term = m_terminal;
						if (!!term)
						{
							uint16_t width = 80;
							uint16_t height = 24;
							term->get_window_size(width, height);
							send_window_size(width, height);
						}
					}
					else
						handle_option(m_optionVerb == WILL);
					break;
				}
				default:
					handle_option(false);
					break;
				}

				m_parserState = 0;
				src.set_to_subrange(itor.get_position());
				itor = src.get_first_const_iterator();
				break;
			}
			case 3:	// Receive option name, then wait for content, plus IAC SE
			{
				m_option = c;
				m_incomingSB.clear();
				m_parserState = 4;
				++itor;
				break;
			}
			case 4:
			{
				// Do IAC's need escaping in an SB sequence?
				// We're supposed to receive one right before an SE.
				// But, what if we get one and something other than SE if after it?

				if (c == IAC)
					m_parserState = 5;
				else
					m_incomingSB.append(c);
				++itor;
				break;
			}
			default:
			{
				COGS_ASSERT(false);
				break;
			}
			}
		}
		return result;
	}

	const buffer iacBuf;

	virtual composite_buffer filtering_sink(composite_buffer& src)
	{
		return encode_buffer(src);
	}

	composite_buffer encode_buffer_const(const composite_buffer& src)
	{
		composite_buffer src2(src);
		return encode_buffer(src2);
	}

	composite_buffer encode_buffer(composite_buffer& src)
	{
		composite_buffer result;
		composite_buffer::const_iterator itor = src.get_first_const_iterator();
		while (!!itor)
		{
			bool foundIAC = (*itor == IAC);
			++itor;
			if (!foundIAC)
				continue;
			result.append(src.split_off_before(itor.get_position()));
			result.append(iacBuf);
			itor = src.get_first_const_iterator();
			continue;
		}
		return result;
	}

public:
	telnet(const ptr<rc_obj_base>& desc, const rcref<datastream>& ds, const rcptr<terminal>& term = 0)
		:	datastream_protocol(desc, ds),
			m_terminal(term),
			m_parserState(0),
			m_sendNAWS(true),
			iacBuf(1, IAC)
	{
		if (!!term)
			term->m_telnet = this_rcref;

		memset(m_myNegotiationState, 0, 256);
		memset(m_theirNegotiationState, 0, 256);

		uint8_t msg[3];
		msg[0] = IAC;
		msg[1] = WILL;
		msg[2] = TELOPT_SGA;
		get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
		m_myNegotiationState[TELOPT_SGA] = 1;

		//msg[0] = IAC;
		msg[1] = DO;
		//msg[2] = TELOPT_SGA;
		get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
		m_theirNegotiationState[TELOPT_SGA] = 1;

		//msg[0] = IAC;
		//msg[1] = DO;
		msg[2] = TELOPT_BINARY;
		get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
		m_theirNegotiationState[TELOPT_BINARY] = 1;

		//msg[0] = IAC;
		msg[1] = WILL;
		//msg[2] = TELOPT_BINARY;
		get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
		m_myNegotiationState[TELOPT_BINARY] = 1;

		//msg[0] = IAC;
		//msg[1] = WILL;
		msg[2] = TELOPT_TTYPE;
		get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
		m_myNegotiationState[TELOPT_TTYPE] = 1;
	}

	void send_window_size(uint16_t width, uint16_t height)
	{
		if (m_sendNAWS)
		{
			uint8_t msg[4] = { IAC, SB, TELOPT_NAWS };
			get_sink_filter()->bypass(buffer((char*)&msg[0], 3));
			msg[0] = (uint8_t)(width >> 8);
			msg[1] = (uint8_t)width;
			msg[2] = (uint8_t)(height >> 8);
			msg[3] = (uint8_t)height;
			get_sink_filter()->bypass(encode_buffer_const(buffer((char*)&msg[0], 4)));
			msg[0] = IAC;
			msg[1] = SE;
			get_sink_filter()->bypass(buffer((char*)&msg[0], 2));
		}
	}

};


}
}
}


#endif

