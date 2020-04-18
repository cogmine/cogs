////
////  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: Obsolete, WorkInProgress
//
//#ifndef COGS_HEADER_GUI_ANSITERM
//#define COGS_HEADER_GUI_ANSITERM
//
//
//#include "cogs/env.hpp"
//#include "cogs/collections/simple_vector.hpp"
//#include "cogs/collections/string.hpp"
//#include "cogs/dependency_property.hpp"
//#include "cogs/gui/pane.hpp"
//#include "cogs/io/buffer.hpp"
//#include "cogs/io/composite_buffer.hpp"
//#include "cogs/io/datastream.hpp"
//#include "cogs/io/net/telnet.hpp"
//#include "cogs/math/measure.hpp"
//#include "cogs/math/time.hpp"
//#include "cogs/mem/rcnew.hpp"
//#include "cogs/sync/refireable_timer.hpp"
//
//
//namespace cogs {
//namespace gui {
//
//
//class ansiterm :
//	public pane,
//	public io::net::telnet::terminal, // provide telnet class with access to terminal info and controls
//	public io::datasource_facade
//{
//private:
//	volatile transactable<scroll_bar_state> m_vScrollBarState;
//	alignas (atomic::get_alignment_v<double>) volatile double m_vScrollBarPosition;
//
//	delayed_construction<delegated_dependency_property<scroll_bar_state, io::permission::read> > m_vScrollBarStateProperty;
//	delayed_construction<delegated_dependency_property<double> > m_vScrollBarPositionProperty;
//
//	// "Screen" refers to the (bottom) terminal emulation portion of the buffer
//	// "View" refers to the currently visible portion, which may have been scrolled up to
//	// "Buffer" refers to the entire (circular) buffer contents (and may extend past the right boundary)
//
//	// Character brush
//	// Pack down these bits, since one is stored per buffer char
//	typedef union {
//		struct {
//			uint16_t padding : 3;
//			uint16_t blinking : 2; // 1= blinking, 3=rapid blinking
//			uint16_t foreFlag : 3; //3 bits - 8 foreground colors
//
//			uint16_t bold : 1; // aka 'intense'
//			uint16_t underlined : 1;
//			uint16_t selected : 1; // If currently hilited/selected
//			uint16_t inverse : 1; // Not sure why not just inverse colors instead
//			uint16_t invisible : 1; // ??
//			uint16_t backFlag : 3; //3 bits - 8 background colors
//		};
//		uint16_t value;
//	} brush;
//
//	static constexpr uint16_t BackBlack = 0;
//	static constexpr uint16_t ForeWhite = 0;
//	static constexpr uint16_t Red = 1;
//	static constexpr uint16_t Green = 2;
//	static constexpr uint16_t Cyan = 3;
//	static constexpr uint16_t Blue = 4;
//	static constexpr uint16_t Yellow = 5;
//	static constexpr uint16_t Magenta = 6;
//	static constexpr uint16_t BackWhite = 7;
//	static constexpr uint16_t ForeBlack = 7;
//
//	// settings
//	rcptr<bitmask> m_fontBitMap;
//	unsigned int m_fontSize;
//	color m_forePallete[8];
//	color m_forePalleteBold[8];
//	color m_backPallete[8];
//	size_t m_bufSize;
//	uint8_t m_stripMode; // 0xFF for no strip, 0x7F to strip high bit
//	uint8_t m_tabSpaces; // Number of spaces for tab code
//	bool m_localEcho;
//	bool m_cursorMode; // true = block, false = underline
//	bool m_rawMode; // Just raw-insert everything
//	bool m_whiteBackground; // Reverses default back/fore
//	bool m_insertMode;
//	bool m_wrapMode;
//	bool m_addCRonLF;
//	bool m_vt52Compat;
//	bool m_vt100Compat;
//
//	// state
//	bool m_parseANSISeq;
//	bool m_hadESC;
//	bool m_isMidSeq;
//	bool m_gotQmark;
//	bool m_seqVarValid;
//	bool m_hasBufferLooped;
//	bool m_cursorVis;
//	uint8_t* m_charBuffer;
//	size_t m_bufPos; // Position in buffer of current cursor pos
//	unsigned int m_charWidthInPixels;
//	unsigned int m_charHeightInPixels;
//	unsigned int m_screenWidthInChars; // For both the view and screen size
//	unsigned int m_screenHeightInChars;
//	unsigned int m_bufferHeightInChars;
//	unsigned int m_bufferWidthInChars;
//	unsigned int m_scrollPos; // line number of top left pixel in view
//	unsigned int m_screenTop;
//	brush* m_brushBuffer;
//	brush m_curBrush;
//	brush m_savedBrush;
//	brush m_savedBrush78;
//	unsigned int m_savedPosX;
//	unsigned int m_savedPosY;
//	unsigned int m_savedPos78X;
//	unsigned int m_savedPos78Y;
//	unsigned int m_cursorX;
//	unsigned int m_cursorY;
//	unsigned int m_topScrollLine;
//	unsigned int m_bottomScrollLine;
//	simple_vector<unsigned int> m_seqVars;
//
//	rcptr<refireable_timer> m_blinkTimer;
//	bool m_blinkTimerRunning;
//	unsigned int m_blinkState; // incremented every 1/4 second - fast blank is 1/4 second, regular blink is 1/2 second
//
//	//	//volatile rcptr<resettable_timer> m_blinkTimer;
//
//	void inc_blink_state()
//	{
//		m_blinkState++;
//
//		// Scan through present screen, invalidating anything with blinking contents
//		invalidate_cursor();
//		unsigned int screenHeightInChars = m_screenHeightInChars;
//		unsigned int screenWidthInChars = m_screenWidthInChars;
//		unsigned int scrollPos = m_scrollPos;
//		for (unsigned int y = 0; y < screenHeightInChars; y++)
//		{
//			for (unsigned int x = 0; x < screenWidthInChars; x++)
//			{
//				unsigned int i = (((y + scrollPos) * m_bufferWidthInChars) + x) % m_bufSize;
//
//				if (m_brushBuffer[i].blinking != 0)
//				{
//					invalidate(bounds(point(x*m_charWidthInPixels, y*m_charHeightInPixels), size(m_charWidthInPixels, m_charHeightInPixels)));
//				}
//			}
//		}
//	}
//
//	void start_sequence()
//	{
//		m_isMidSeq = true;
//		m_gotQmark = false;
//		m_seqVarValid = false;
//		m_seqVars.clear();
//	}
//
//	bounds get_screen_rect(unsigned int onRow, unsigned int fromColumn, unsigned int toColumn) // return empty if cursor not on screen
//	{
//		if (fromColumn > toColumn)
//			return bounds(point(0, 0), size(0, 0));
//
//		onRow += m_screenTop; // Map from screen row to buffer row
//		onRow %= m_bufferHeightInChars; // fixup buffer loop.
//
//		bool onPage = false;
//		unsigned int pageRow;
//		unsigned int screenHeightInChars = m_screenHeightInChars;
//		uint32_t bottomOfPageLine = m_scrollPos + screenHeightInChars;
//		bool pageRowsAreNormal = (bottomOfPageLine <= m_bufferHeightInChars);
//		if (pageRowsAreNormal)
//		{ // if on page
//			if ((onRow >= m_scrollPos) && (onRow < m_scrollPos + screenHeightInChars))
//			{
//				onPage = true;
//				pageRow = onRow - m_scrollPos;
//			}
//		}
//		else
//		{
//			bottomOfPageLine %= m_bufferHeightInChars;
//			if (onRow >= m_scrollPos)
//			{
//				onPage = true;
//				pageRow = onRow - m_scrollPos;
//			}
//			else if (onRow < bottomOfPageLine)
//			{
//				onPage = true;
//				pageRow = onRow + (m_bufferHeightInChars - m_scrollPos);
//			}
//		}
//		if (!onPage)
//			return bounds(point(0, 0), size(0, 0));
//
//		return bounds(point(fromColumn * m_charWidthInPixels, pageRow * m_charHeightInPixels), size(((toColumn + 1) - fromColumn) * m_charWidthInPixels, m_charHeightInPixels));
//	}
//
//	void invalidate_cursor()
//	{
//		unsigned int tmpCursorColumn = m_cursorX;
//		unsigned int screenWidthInChars = m_screenWidthInChars;
//		if (m_cursorX >= screenWidthInChars)
//			tmpCursorColumn = screenWidthInChars - 1;
//		invalidate(get_screen_rect(m_cursorY, tmpCursorColumn, tmpCursorColumn));
//	}
//
//	void toggle_cursor()
//	{
//		m_cursorVis = !m_cursorVis;
//		invalidate_cursor();
//	}
//
//	// line is screen-based
//	void invalidate_line(unsigned int line, unsigned int colStart, unsigned int colEnd)
//	{
//		invalidate(get_screen_rect(line, colStart, colEnd));
//	}
//
//	// line is screen-based, and range will never extend past the end of the screen.
//	void invalidate_lines(unsigned int startLine, uint32_t n)
//	{
//		if (n)
//		{
//			unsigned int screenWidthInChars = m_screenWidthInChars;
//			unsigned int screenHeightInChars = m_screenHeightInChars;
//
//			// Start with the size of the screen
//			bounds screenRect(size(m_charWidthInPixels * screenWidthInChars, m_charHeightInPixels * screenHeightInChars));
//			bounds r(screenRect);
//
//			// trim off top and bottom
//			r.inset(startLine * m_charHeightInPixels, 0, (screenHeightInChars - n) * m_charHeightInPixels, 0);
//
//			// shift to scroll position
//			r += point(0, -(int)(((m_screenTop - m_scrollPos) + m_bufferHeightInChars) % m_bufferHeightInChars));
//
//			// clip to visable area
//			r &= screenRect;
//			invalidate(r);
//		}
//	}
//
//
//	// line is screen-based
//	void clean_line(unsigned int line, unsigned int colStart, unsigned int colEnd)
//	{
//		uint32_t pos = (((m_screenTop + line) * m_bufferWidthInChars) + colStart) % m_bufSize;
//		memset((void*)(m_brushBuffer + pos), 0, (colEnd - colStart) * sizeof(brush));
//		memset((void*)(m_charBuffer + pos), 0, (colEnd - colStart));
//	}
//
//	// line is screen-based
//	void clean_line(unsigned int line) { clean_line(line, 0, m_bufferWidthInChars); }
//
//
//	// line is screen-based
//	void clean_lines(unsigned int startLine, uint32_t n)
//	{
//		if (n)
//		{
//			startLine += m_screenTop;
//			startLine %= m_bufferHeightInChars;
//			uint32_t pos = startLine * m_bufferWidthInChars;
//			uint32_t pastEndLine = startLine + n;
//			if (pastEndLine <= m_bufferHeightInChars)
//			{
//				memset((void*)(m_brushBuffer + pos), 0, n * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer + pos), 0, n * m_bufferWidthInChars);
//			}
//			else
//			{
//				uint32_t linesAfterBufferWrap = pastEndLine - m_bufferHeightInChars;
//				uint32_t linesUntilBufferWrap = n - linesAfterBufferWrap;
//
//				// Clear from startLine through last line
//				memset((void*)(m_brushBuffer + pos), 0, linesUntilBufferWrap * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer + pos), 0, linesUntilBufferWrap * m_bufferWidthInChars);
//
//				// Clear from first line, to ((startLine + n) % m_bufferHeightInChars)
//				memset((void*)(m_brushBuffer), 0, linesAfterBufferWrap * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer), 0, linesAfterBufferWrap * m_bufferWidthInChars);
//			}
//		}
//	}
//
//
//	void fix_scrollbar()
//	{
//		//uint32_t topOfAllLine;
//		//uint32_t beforePage;
//		//uint32_t afterPage;
//		//unsigned int screenHeightInChars = m_screenHeightInChars;
//
//		//if (!m_hasBufferLooped)
//		//{
//		//	topOfAllLine = 0;
//		//	beforePage = m_scrollPos;
//		//	afterPage = m_screenTop - m_scrollPos;
//		//}
//		//else
//		//{
//		//	topOfAllLine = m_screenTop + screenHeightInChars;
//		//	topOfAllLine %= m_bufferHeightInChars;
//		//
//		//	beforePage = m_scrollPos - topOfAllLine;
//		//	if (beforePage >= m_bufferHeightInChars)
//		//		beforePage += m_bufferHeightInChars;
//		//
//		//	if (m_screenTop >= m_scrollPos)
//		//		afterPage = m_screenTop - m_scrollPos;
//		//	else
//		//		afterPage = (m_screenTop + m_bufferHeightInChars) - m_scrollPos;
//		//}
//		//
//		//update_scroller(1, beforePage, screenHeightInChars, beforePage + screenHeightInChars + afterPage);
//	}
//
//	void advance_screen(uint32_t n) // Advances the screen (bottom portion of buffer) n lines, possibly reclaiming from the top
//	{
//		// First, check if our advance steps past the end of the storage buffer
//		bool tracking = m_screenTop == m_scrollPos;
//		unsigned int screenHeightInChars = m_screenHeightInChars;
//		if (m_screenTop + screenHeightInChars + n >= m_bufferHeightInChars)
//			m_hasBufferLooped = true;
//
//		uint32_t oldScreenTop = m_screenTop;
//		m_screenTop += n;
//		m_screenTop %= m_bufferHeightInChars;
//		if (tracking)
//			m_scrollPos = m_screenTop;
//		else if (m_hasBufferLooped)
//		{ // If scrollPos is being reclaimed, push it forward... Which is tricky to compute, due to buffer looping.
//
//			uint32_t topOfAllLine = oldScreenTop + screenHeightInChars;
//			topOfAllLine %= m_bufferHeightInChars;
//
//			uint32_t oldScrollPos = m_scrollPos;
//			if (oldScrollPos < topOfAllLine)
//				oldScrollPos += m_bufferHeightInChars;
//
//			uint32_t relativeScrollPos = oldScrollPos - topOfAllLine;
//			if (relativeScrollPos < n) // need to advance scroll pos
//			{
//				m_scrollPos += n - relativeScrollPos;
//				m_scrollPos %= m_bufferHeightInChars;
//			}
//		}
//
//		uint32_t firstLineToClean = screenHeightInChars - n;
//		clean_lines(firstLineToClean, n);
//
//		fix_scrollbar();
//		invalidate(get_size());
//	}
//
//
//	void calc_buf_pos() // after change to m_cursorY and/or m_cursorX, or end-line causing page screen
//	{
//		m_bufPos = (((m_screenTop + m_cursorY) * m_bufferWidthInChars) + m_cursorX) % m_bufSize;
//		invalidate_cursor(); // if we needed to recompute the cursor, then it needs to be draw
//	}
//
//	void inc_buf_pos()
//	{
//		if (++m_bufPos == m_bufSize)
//			m_bufPos = 0;
//		invalidate_cursor();
//	}
//
//	void back_off_verge()
//	{
//		unsigned int screenWidthInChars = m_screenWidthInChars;
//		if (m_cursorX >= screenWidthInChars)
//		{
//			unsigned int d = (m_cursorX - screenWidthInChars) + 1;
//			m_cursorX -= d;
//			m_bufPos -= d;
//			if (m_bufPos >= m_bufSize)
//				m_bufPos += m_bufSize;
//			else
//				m_bufPos -= d;
//		}
//	}
//
//	void step_off_verge()
//	{
//		unsigned int screenWidthInChars = m_screenWidthInChars;
//		unsigned int screenHeightInChars = m_screenHeightInChars;
//		if (m_cursorX >= screenWidthInChars)
//		{
//			invalidate_cursor();
//			if (m_wrapMode)
//			{
//				if (m_cursorY == screenHeightInChars - 1)
//					advance_screen(1);
//				else
//					m_cursorY++;
//				m_cursorX = 0;
//				calc_buf_pos();
//			}
//			else // terminal set to a mode where it refuses to advance lines automatically
//			{ // So, keep overwriting last character
//				m_cursorX--;
//				m_bufPos--;
//			}
//		}
//	}
//
//	void raw_insert(char c)
//	{
//		step_off_verge();
//		if (m_curBrush.invisible) // ignore text, just insert spaces.  Overrides all other brush attributes
//		{
//			if (m_insertMode)
//			{
//				// move chars and brushes to the right
//			}
//			m_charBuffer[m_bufPos] = ' ';
//		}
//		else
//		{
//			if (m_insertMode)
//			{
//				// move chars and brushes to the right
//			}
//			m_charBuffer[m_bufPos] = c;
//		}
//		m_brushBuffer[m_bufPos].value = m_curBrush.value;
//		invalidate_cursor();
//		m_cursorX++;
//		inc_buf_pos();
//	}
//
//
//	void parse(char c)
//	{
//		switch (c)
//		{
//		case CSI:
//			if (m_parseANSISeq)
//				start_sequence();
//			else
//				raw_insert(c);
//			break;
//		case 0: // Ignore NULL
//			break;
//		case DCS: // Device Control String
//					// ignored
//			break;
//		case ST: // String Terminator
//			// ignored
//			break;
//		case ESC:
//			if (m_parseANSISeq)
//				m_hadESC = true;
//			else
//				raw_insert(c);
//			break;
//		case HOME:
//		{
//			invalidate_cursor();
//			m_cursorX = 0;
//			m_cursorY = 0;
//			unsigned int screenHeightInChars = m_screenHeightInChars;
//			advance_screen(screenHeightInChars);
//			calc_buf_pos();
//		}
//		break;
//		case TAB:
//		{
//			unsigned int screenWidthInChars = m_screenWidthInChars;
//			invalidate_cursor();
//			uint8_t tabSpaces = m_tabSpaces - (m_cursorX % m_tabSpaces);
//			if (m_cursorX > screenWidthInChars - m_tabSpaces)
//				m_cursorX = screenWidthInChars;
//			else
//				m_cursorX += tabSpaces;
//			calc_buf_pos();
//		}
//		break;
//		case BELL:
//			os::beep();
//			break;
//		case BS:
//		case DEL:
//			invalidate_cursor();
//			if (m_cursorX != 0)
//			{
//				m_cursorX--;
//				calc_buf_pos();
//			}
//			break;
//		case LF: // Line Feed - Same as ESC D
//			if (m_addCRonLF)
//				m_cursorX = 0;
//		case IND: // Index - same as ESC D
//		// fall through
//		case VT: // Vertical tab (treated same as NExt Line)
//		case NEL: // NExt Line - Same as ESC E
//		{
//			invalidate_cursor();
//
//			// TBD - arbitrary scroll regions!
//
//			unsigned int screenHeightInChars = m_screenHeightInChars;
//			if (m_cursorY == screenHeightInChars - 1)
//				advance_screen(1);
//			else
//				m_cursorY++;
//			calc_buf_pos();
//		}
//		break;
//		case RI: // Reverse Index - same as Esc M
//			invalidate_cursor();
//
//			// TBD - arbitrary scroll regions!
//
//			if (m_cursorY != 0)
//				m_cursorY--;
//			calc_buf_pos();
//			break;
//		case CR:
//			invalidate_cursor();
//			m_cursorX = 0;
//			calc_buf_pos();
//			break;
//			//		case MUSICNOTE: // Marks end of ANSI music sequence?
//			//			break;
//		default:
//			raw_insert(c);
//			break;
//		}
//	}
//
//
//	void parse_esc_code(char c)
//	{
//		switch (c) // ANSI escape sequences
//		{
//		case 0: // ignore NULL
//			break;
//		case ESC: // Previous escape ignored
//			// leave m_hadESC intact
//			break;
//		case 'P': // Device Control String start NOP
//			m_hadESC = false;
//			parse(DCS);
//			break;
//		case '\\': // String Terminated / ignored
//			m_hadESC = false;
//			parse(ST);
//			break;
//		case 'E': // NEL - Next Line
//			m_hadESC = false;
//			parse(NEL);
//			break;
//		case 'M': // Move up
//			m_hadESC = false;
//			parse(RI);
//			break;
//		case 'H':
//		{
//			m_hadESC = false;
//			if (!m_vt52Compat)
//			{
//				// HTS - Horizontal Tab Set
//				break;
//			}
//			// VT52 set cursor position
//			start_sequence();
//			parse_mid_sequence(c);
//		}
//		break;
//		case 'D':
//			if (!m_vt52Compat)
//			{
//				// IND - Index - Move down (scroll if necessary)
//				m_hadESC = false;
//				parse(IND);
//				break;
//			}
//			// else fallthrough, as these VT52 ESC_ is identical to ESC[_
//		case 'A': // Some VT52 support
//		case 'B':
//		case 'C':
//		case 'J':
//		case 'K':
//			m_hadESC = false;
//			// Same as ESC[ versions
//			start_sequence();
//			parse_mid_sequence(c);
//			break;
//		case CSI:
//		case '[':
//			m_hadESC = false;
//			start_sequence();
//			break;
//		case '7': // DECSC - Save Cursor
//			m_hadESC = false;
//			m_savedBrush78 = m_curBrush;
//			m_savedPos78X = m_cursorX;
//			m_savedPos78Y = m_cursorY;
//			break;
//		case '8': // DECRC - Restore Cursor
//		{
//			m_hadESC = false;
//			invalidate_cursor();
//			m_curBrush = m_savedBrush78;
//			m_cursorX = m_savedPos78X;
//			m_cursorY = m_savedPos78Y;
//			unsigned int screenHeightInChars = m_screenHeightInChars;
//			if (m_cursorY >= screenHeightInChars)
//			{
//				m_cursorY = screenHeightInChars - 1;
//				COGS_ASSERT(m_cursorY < 999);
//			}
//			calc_buf_pos();
//		}
//		break;
//		case '<': // Exit vt52 mode, enter VT100 mode
//			m_hadESC = false;
//			m_vt52Compat = false;
//			break;
//		case 'I': // Reverse Line Feed - vt52
//		case '6': // DECBI — Back Index
//			m_hadESC = false;
//			break;
//
//		case 'c': // RIS - Reset to Initial State
//			m_hadESC = false;
//			break;
//
//		case 'g': // visual bell // ignored
//		case 'n': // Lock shift G2 // ignored
//		case 'o': // Lock shift G3 // ignored
//		case '!': // Global message string // ignored
//		case ']': // Operating system command // ignored
//		case '1': // ignored
//		case '2': // ignored
//			m_hadESC = false;
//			break;
//			//case 'q': // DECLL - Load LEDs
//			//case '9': // DECFI - Forward Index
//			//case 'F': // Enter graphics mode - VT52
//			//case 'G': // Exit graphics mode - VT52
//			//case ']': // Print screen - vt52
//			//case 'V': // Print current line - vt52
//			//case 'W': // Enter printer controller mode - vt52
//			//case 'X': // Exit printer controller mode - vt52
//			//case '^': // Enter autoprint mode ? - vt52
//			//case '_': // Exit autoprint mode ? - vt52
//			//case '=': // DECKPAM - Keypad Application Mode - Alt keypad mode
//			//case '>': // DECKPNM — Keypad Numeric Mode - Exit alt keypad mode
//			//case 'Z': // identify (host to terminal)
//			//case '/Z': // identify (terminal to host)
//			//	break
//			// ESC Y# ?? - Move cursor to column # - VT52
//			// ESC Q # <string> ?? - SCODFK - Define Function Key
//			// ESC #3 - DECDHL - Double-Width, Double-Height Line - top half
//			// ESC #4 - DECDHL - Double-Width, Double-Height Line - bottom half
//			// ESC #5 - DECSWL - Single-Width, Single-Height Line
//			// ESC #6 - DECDWL - Double-Width, Single-Height Line
//			// ESC #8 - DECALN - Screen Alignment Pattern
//			// ESC SP F - S7C1T - Send C1 Control Character to the Host
//			// ESC SP G - S8C1T - Send C1 Control Character to the Host
//		default:
//			m_hadESC = false; // Ignores potential control character
//			break;
//		}
//	}
//
//	void parse_mid_sequence(char c)
//	{
//		if ((c >= '0') && (c <= '9'))
//		{
//			if (!m_seqVarValid)
//			{
//				m_seqVarValid = true;
//				unsigned int i = (unsigned int)(uint8_t)c - 48;
//				m_seqVars.append(&i, 1);
//			}
//			else
//			{
//				unsigned int* v = &(m_seqVars.get_ptr()[m_seqVars.get_length() - 1]);
//				*v *= 10;
//				*v += ((uint8_t)c - 48);
//			}
//		}
//		else
//		{
//			// in-sequence and not a number
//			switch (c)
//			{
//			case 0: // ignore NULL
//				break;
//			case '?': // Mode control ?
//				m_gotQmark = true;
//				break;
//			case ';': // break between seq vars
//				if (!m_seqVarValid)
//					m_seqVars.append((size_t)1, (unsigned int)0);
//				else
//					m_seqVarValid = false;
//				break;
//			case ESC: // Start over
//				m_isMidSeq = false;
//				m_hadESC = true;
//				break;
//			case CSI: // Start over
//				start_sequence();
//				break;
//			case 'A': // CUU - Cursor Up - move up
//			{
//				m_isMidSeq = false;
//				// old code would keep the cursor on a line until it wrapped due to text entered.
//				invalidate_cursor();
//				step_off_verge();
//				unsigned int n = 1;
//				if (!m_seqVars.is_empty())
//					n = m_seqVars[0];
//				if (n < 1)
//					n = 1;
//				if (m_cursorY <= n)
//					m_cursorY = 0;
//				else
//				{
//					m_cursorY -= n;
//					COGS_ASSERT(m_cursorY < 999);
//				}
//				calc_buf_pos();
//			}
//			break;
//			case 'B': // CUD - Cursor Down - move down
//			{
//				m_isMidSeq = false;
//				// old code would keep the cursor on a line until it wrapped due to text entered.
//				invalidate_cursor();
//				step_off_verge();
//				unsigned int n = 1;
//				if (!m_seqVars.is_empty())
//					n = m_seqVars[0];
//				if (n < 1)
//					n = 1;
//				m_cursorY += n;
//				COGS_ASSERT(m_cursorY < 999);
//				unsigned int screenHeightInChars = m_screenHeightInChars;
//				if (m_cursorY >= screenHeightInChars)
//				{
//					m_cursorY = screenHeightInChars - 1; // Old code did this.  Maybe more accurate to retain 'verge' effect?  Could be repro'ing bad behavior of other terminals.
//					COGS_ASSERT(m_cursorY < 999);
//				}
//				calc_buf_pos();
//			}
//			break;
//			case 'C': // CUF - Cursor Forward - Move right
//			{
//				m_isMidSeq = false;
//				step_off_verge();
//				// old code would keep the cursor on a line until it wrapped due to text entered.
//				invalidate_cursor();
//				unsigned int n = 1;
//				if (!m_seqVars.is_empty())
//					n = m_seqVars[0];
//				if (n < 1)
//					n = 1;
//				m_cursorX += n;
//				unsigned int screenWidthInChars = m_screenWidthInChars;
//				if (m_cursorX >= screenWidthInChars)
//					m_cursorX = screenWidthInChars - 1; // Old code did this.  Maybe more accurate to retain 'verge' effect?  Could be repro'ing bad behavior of other terminals.
//				calc_buf_pos();
//			}
//			break;
//			case 'D': // CUB - Cursor Backward - move left
//			{
//				m_isMidSeq = false;
//				// old code would keep the cursor on a line until it wrapped due to text entered.
//				invalidate_cursor();
//				unsigned int n = 1;
//				if (!m_seqVars.is_empty())
//					n = m_seqVars[0];
//				if (n < 1)
//					n = 1;
//				if (m_cursorX <= n)
//					m_cursorX = 0;
//				else
//					m_cursorX -= n;
//				calc_buf_pos();
//			}
//			break;
//			case 'f': // HVP - Horizontal and Vertical Position
//			case 'H': // CUP - Cursor Position
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				unsigned int newPosX = 0;
//				unsigned int newPosY = 0;
//				if (m_seqVars.get_length() > 0)
//				{
//					newPosY = m_seqVars[0];
//					unsigned int screenHeightInChars = m_screenHeightInChars;
//					if (newPosY > screenHeightInChars)
//						newPosY = screenHeightInChars;
//					if (newPosY > 0)
//						newPosY--;
//				}
//				if (m_seqVars.get_length() > 1)
//				{
//					newPosX = m_seqVars[1];
//					if (newPosX > 0)
//						newPosX--;
//				}
//				m_cursorX = newPosX;
//				m_cursorY = newPosY;
//				COGS_ASSERT(m_cursorY < 999);
//				calc_buf_pos();
//			}
//			break;
//			case 'd': //	VPA - Vertical Line Position Absolute - Go to line #
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				unsigned int n = 0;
//				if (!m_seqVars.is_empty())
//					n = m_seqVars[0];
//				unsigned int screenHeightInChars = m_screenHeightInChars;
//				if (n > screenHeightInChars)
//					n = screenHeightInChars;
//				if (n > 0)
//					n--;
//				m_cursorY = n;
//				COGS_ASSERT(m_cursorY < 999);
//				calc_buf_pos();
//			}
//			break;
//			//case 's': // ESC[#;#s - DECSLRM - Set Left and Right Margins ????
//			case 's': // SCOSC - Save Current Cursor Position
//				m_isMidSeq = false;
//				m_savedBrush = m_curBrush;
//				m_savedPosX = m_cursorX;
//				m_savedPosY = m_cursorY;
//				break;
//			case 'u': // SCORC - Restore Saved Cursor Position
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				m_cursorX = m_savedPosX;
//				m_cursorY = m_savedPosY;
//				m_curBrush = m_savedBrush;
//				unsigned int screenHeightInChars = m_screenHeightInChars;
//				if (m_cursorY >= screenHeightInChars)
//				{
//					m_cursorY = screenHeightInChars - 1;
//					COGS_ASSERT(m_cursorY < 999);
//				}
//				calc_buf_pos();
//			}
//			break;
//			case 'm': // SGR - Select Graphic Rendition
//				m_isMidSeq = false;
//				do {
//					unsigned int selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					m_seqVars.erase(0, 1);
//					switch (selector)
//					{
//					case 0:
//						m_curBrush.value = 0;
//						break;
//					case 1:
//						m_curBrush.bold = 1;
//						break;
//					case 2:
//						// faint
//						break;
//					case 3:
//						// italic
//						break;
//					case 4:
//						m_curBrush.underlined = 1;
//						break;
//					case 5:
//						m_curBrush.blinking = 1;
//						break;
//					case 6:
//						m_curBrush.blinking = 3;
//						break;
//					case 7:
//						m_curBrush.inverse = 1;
//						break;
//					case 8:
//						m_curBrush.invisible = 1;
//						break;
//					case 10: // Default ASCII mapping of 7-bit set
//						break;
//					case 11: // 00-7F PC character set to 7-bit set
//						break;
//					case 12: // 80-FF of current character set to 7-bit set
//						break;
//					case 22:
//						m_curBrush.bold = 0;
//						break;
//					case 24:
//						m_curBrush.underlined = 0;
//						break;
//					case 25:
//						m_curBrush.blinking = 0;
//						break;
//					case 27:
//						m_curBrush.inverse = 0;
//						break;
//					case 28:
//						m_curBrush.invisible = 0;
//						break;
//					case 30:
//						m_curBrush.foreFlag = ForeBlack;
//						break;
//					case 31:
//						m_curBrush.foreFlag = Red;
//						break;
//					case 32:
//						m_curBrush.foreFlag = Green;
//						break;
//					case 33:
//						m_curBrush.foreFlag = Yellow;
//						break;
//					case 34:
//						m_curBrush.foreFlag = Blue;
//						break;
//					case 35:
//						m_curBrush.foreFlag = Magenta;
//						break;
//					case 36:
//						m_curBrush.foreFlag = Cyan;
//						break;
//					case 37:
//					case 39: // default
//						m_curBrush.foreFlag = ForeWhite;
//						break;
//					case 40:
//					case 49: // default or superscript ??
//						m_curBrush.backFlag = BackBlack;
//						break;
//					case 41:
//						m_curBrush.backFlag = Red;
//						break;
//					case 42:
//						m_curBrush.backFlag = Green;
//						break;
//					case 43:
//						m_curBrush.backFlag = Yellow;
//						break;
//					case 44:
//						m_curBrush.backFlag = Blue;
//						break;
//					case 45:
//						m_curBrush.backFlag = Magenta;
//						break;
//					case 46:
//						m_curBrush.backFlag = Cyan;
//						break;
//					case 47:
//						m_curBrush.backFlag = BackWhite;
//						break;
//					case 48: // subscript
//						break;
//					default:
//						break;
//					}
//				} while (!m_seqVars.is_empty());
//
//				break;
//			case 'J':
//			{
//				m_isMidSeq = false;
//				if (m_gotQmark)
//				{
//					// DECSED - Selective Erase in Display
//				}
//				else
//				{
//					// ED — Erase in Display
//					invalidate_cursor();
//					unsigned int selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					switch (selector)
//					{
//					case 0: // Clear to bottom of screen
//					{
//						clean_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//						invalidate_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//						unsigned int nextLine = m_cursorY + 1;
//						unsigned int screenHeightInChars = m_screenHeightInChars;
//						unsigned int linesLeft = screenHeightInChars - nextLine;
//						if (linesLeft)
//						{
//							clean_lines(nextLine, linesLeft);
//							invalidate_lines(nextLine, linesLeft);
//						}
//						break;
//					}
//					case 1: // Clear to top of screen
//					{
//						clean_line(m_cursorY, 0, m_cursorX);
//						invalidate_line(m_cursorY, 0, m_cursorX);
//						if (m_cursorY > 0)
//						{
//							clean_lines(0, m_cursorY);
//							invalidate_lines(0, m_cursorY);
//						}
//						break;
//					}
//					case 2: // Clear entire screen
//					{
//						unsigned int screenHeightInChars = m_screenHeightInChars;
//						advance_screen(screenHeightInChars);
//
//						clean_lines(0, screenHeightInChars - 1);
//						invalidate_lines(0, screenHeightInChars - 1);
//
//						// ANSI resets cursor pos, VT100 does not?
//						if (!m_vt100Compat)
//						{
//							m_cursorX = 0;
//							m_cursorY = 0;
//						}
//						break;
//					}
//					}
//				}
//			}
//			break;
//			case 'K':
//			{
//				m_isMidSeq = false;
//				if (m_gotQmark)
//				{
//					// DECSEL - Selective Erase in Line
//				}
//				else
//				{
//					// EL - Erase in Line
//					invalidate_cursor();
//					unsigned int selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					switch (selector)
//					{
//					case 0: // Clear to end of current line
//						clean_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//						invalidate_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//						break;
//					case 1: // Clear backward to begining of current line
//						clean_line(m_cursorY, 0, m_cursorX);
//						invalidate_line(m_cursorY, 0, m_cursorX);
//						break;
//					case 2: // Clear entire line
//						clean_line(m_cursorY, 0, m_bufferWidthInChars);
//						invalidate_line(m_cursorY, 0, m_bufferWidthInChars);
//						break;
//					}
//				}
//			}
//			break;
//			case '@': // ICH - Insert Character - Cursor does not move.  Pushes line forward as if in an insert
//			{
//				m_isMidSeq = false;
//				unsigned int screenWidthInChars = m_screenWidthInChars;
//				if (m_cursorX <= screenWidthInChars)
//				{
//					unsigned int selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (selector < 1)
//						selector = 1;
//					if (selector + m_cursorX > screenWidthInChars)
//						selector = screenWidthInChars - m_cursorX;
//					unsigned int n = screenWidthInChars - (selector + m_cursorX);
//
//					memcpy(m_charBuffer + m_bufPos + selector, m_charBuffer + m_bufPos, n);
//					memcpy(m_brushBuffer + m_bufPos + selector, m_brushBuffer + m_bufPos, n * sizeof(brush));
//
//					invalidate_line(m_cursorY, m_cursorX, screenWidthInChars);
//				}
//			}
//			break;
//			case 'P': // DCH - Delete Character - Cursor does not move.  Shifts line to left, deleting at cursor.
//			{
//				m_isMidSeq = false;
//				unsigned int screenWidthInChars = m_screenWidthInChars;
//				if (m_cursorX <= screenWidthInChars)
//				{
//					unsigned int selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (selector < 1)
//						selector = 1;
//					if (selector + m_cursorX > screenWidthInChars)
//						selector = screenWidthInChars - m_cursorX;
//					unsigned int n = screenWidthInChars - (selector + m_cursorX);
//
//					memcpy(m_charBuffer + m_bufPos, m_charBuffer + m_bufPos + selector, n);
//					memcpy(m_brushBuffer + m_bufPos, m_brushBuffer + m_bufPos + selector, n * sizeof(brush));
//
//					invalidate_line(m_cursorY, m_cursorX, screenWidthInChars);
//				}
//			}
//			break;
//			case 'L': // IL - Insert Line (or # lines)
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (selector < 1)
//					selector = 1;
//				unsigned int screenHeightInChars = m_screenHeightInChars;
//				if (selector + m_cursorY > screenHeightInChars)
//					selector = screenHeightInChars - m_cursorY;
//				m_cursorX = 0;
//				calc_buf_pos();
//				unsigned int scrollHeight = screenHeightInChars - m_cursorY;
//				unsigned int linesToMove = scrollHeight - selector;
//				if (linesToMove)
//				{
//					memcpy(m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), m_charBuffer + m_bufPos, linesToMove * m_bufferWidthInChars);
//					memcpy(m_brushBuffer + m_bufPos + (selector * m_bufferWidthInChars), m_brushBuffer + m_bufPos, linesToMove * m_bufferWidthInChars * sizeof(brush));
//				}
//				memset(m_charBuffer + m_bufPos, 0, selector * m_bufferWidthInChars);
//				for (unsigned int i = 0; i < selector * m_bufferWidthInChars; i++)
//					m_brushBuffer[m_bufPos + i].value = m_curBrush.value;
//				invalidate_lines(m_cursorY, scrollHeight);
//			}
//			break;
//			case 'M': // DL — Delete Line (or # lines)
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (selector < 1)
//					selector = 1;
//				unsigned int screenHeightInChars = m_screenHeightInChars;
//				if (selector + m_cursorY > screenHeightInChars)
//					selector = screenHeightInChars - m_cursorY;
//				m_cursorX = 0;
//				calc_buf_pos();
//				unsigned int scrollHeight = screenHeightInChars - m_cursorY;
//				unsigned int linesToMove = scrollHeight - selector;
//				if (linesToMove)
//				{
//					memcpy(m_charBuffer + m_bufPos, m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), linesToMove * m_bufferWidthInChars);
//					memcpy(m_brushBuffer + m_bufPos, m_brushBuffer + m_bufPos + (selector * m_bufferWidthInChars), linesToMove * m_bufferWidthInChars * sizeof(brush));
//				}
//				memset(m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), 0, selector * m_bufferWidthInChars);
//				for (unsigned int i = 0; i < selector * m_bufferWidthInChars; i++)
//					m_brushBuffer[m_bufPos + i].value = m_curBrush.value;
//				invalidate_lines(m_cursorY, scrollHeight);
//			}
//			break;
//			case 'R': // Position report?  Are we a server?
//				m_isMidSeq = false;
//
//				// TBD - This isn't useful data, as the position may have changed by now.
//				//			But, this does indicate that the report party did parse our request.
//				//			Commonly used to detect if the report party has an ANSI terminal.
//
//				break;
//			case 'n': // DSR - Device Status Reports
//			{
//				m_isMidSeq = false;
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (m_gotQmark)
//				{
//					//?15n Q: What's the printer status
//					//		?13n A: There is no printer
//					//?26n Q: What's the keyboard type?
//					//		?27;1n A: North American
//					switch (selector)
//					{
//					case 6: //	DECXCPR - Extended Cursor Position
//					{ //?6n Q; Send extended cursor position report.  Respond with ESC [ #;#;# R
//						cstring s;
//						s += ESC;
//						s += '[';
//						s += uint_type(m_cursorY).to_cstring();
//						s += ';';
//						s += uint_type(m_cursorX).to_cstring();
//						s += cstring::literal(";0R"); // last is 'page' ?  zero
//						queue_to_datasource(io::buffer::from_cstring(s));
//					}
//					break;
//					case 15: // DSR - Printer Port (request)
//					{
//						uint8_t msg[6] = { ESC, '[', '?', '1', '3', 'n' };
//						queue_to_datasource(io::buffer((char*)&msg[0], 6));
//					}
//					break;
//					case 26: // DSR - Keyboard - (request)
//					{
//						uint8_t msg[8] = { ESC, '[', '?', '2', '7', ';', '1', 'n' };
//						queue_to_datasource(io::buffer((char*)&msg[0], 8));
//					}
//					break;
//					//case 10: // DSR - Printer Port - (response) - printer ready
//					//case 11: // DSR - Printer Port - (response) - printer not ready
//					//case 13: // DSR - Printer Port - (response) - no printer
//					//case 18: // DSR - Printer Port - (response) - printer busy
//					//case 19: // DSR - Printer Port - (response) - printer assigned to another session
//					//case 20: // DSR - User-Defined Keys - (response) - unlocked
//					//case 21: // DSR - User-Defined Keys - (response) - locked
//					//case 25: // DSR - User-Defined Keys - (request)
//					//case 27: // DSR - Keyboard - (response)
//					//case 62: // DSR - Macro Space Report (request)
//					//case 63: // DSR - Memory Checksum (DECCKSR)
//					//case 70: // DSR — Data Integrity Report - (response) Ready
//					//case 71: // DSR — Data Integrity Report - (response) Malfunction
//					//case 73: // DSR — Data Integrity Report - (response) No report
//					//case 75: // DSR — Data Integrity Report - (request) integrity flag status
//					default:
//						break;
//					}
//				}
//				else
//				{
//					switch (selector)
//					{
//					case 6: // CPR - Cursor Position Report
//					{ //6n Q; Send cursor position report.  Respond with ESC [ #;#R
//						cstring s;
//						s += ESC;
//						s += '[';
//						s += uint_type(m_cursorY).to_cstring();
//						s += ';';
//						s += uint_type(m_cursorX).to_cstring();
//						s += 'R';
//						queue_to_datasource(io::buffer::from_cstring(s));
//					}
//					break;
//					case 5: // DSR - Operating Status - (request)
//					{ //5n Q: What is your status?
//						uint8_t msg[4] = { ESC, '[', '0', 'n' };
//						queue_to_datasource(io::buffer((char*)&msg[0], 4));
//					}
//					break;
//					//case 0: // DSR - Operating Status - (response) good
//					//case 3: // DSR - Operating Status - (response) malfunction
//					default:
//						break;
//					}
//				}
//			}
//			break;
//			case 'G': // CHA - Cursor Horizontal Absolute - Move cursor to column n
//			{
//				m_isMidSeq = false;
//				invalidate_cursor();
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (selector > 0)
//					selector--;
//				m_cursorX = selector;
//			}
//			break;
//			case 'h':
//			{
//				m_isMidSeq = false;
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (m_gotQmark)
//				{
//					switch (selector)
//					{
//					case 25: // DECTCEM - Text Cursor Enable Mode - show cursor
//								// TBD
//						break;
//						//case 1: // DECCKM - Cursor Keys Mode - set
//						//case 3: // 132 column mode
//						//case 4: // DECSCLM - Scrolling Mode - smooth scroll
//						//case 5: // DECSCNM - Screen Mode: Light or Dark Screen - set/reverse video
//						//case 6: // DECOM - Origin Mode - set/within margins
//						//case 7: // DECAWM — Autowrap Mode - set/enable
//						//case 8: // DECARM - Autorepeat Mode - enable auto-repeat
//						//case 18: // DECPFF - Print Form Feed Mode - use FF
//						//case 19: // DECPEX - Printer Extent Mode - prints whole page
//						//case 20: // NL mode
//						//case 34: // DECRLM - Cursor Right to Left Mode - set
//						//case 42: // DECNRCM - National Replacement Character Set Mode - set/7-bit characters
//						//case 47: //?47 ignored (XTERM) switch to alternate screen
//						//case 58: // DECIPEM - Enter/Return from IBM ProPrinter Emulation Mode - set/enter
//						//case 60: // DECHCCM - Horizontal Cursor-Coupling Mode - set/couples cursor to display
//						//case 61: // DECVCCM - Vertical Cursor-Coupling Mode - set/couples cursor to display
//						//case 64: // DECPCCM - Page Cursor-Coupling Mode - set/couples cursor to display
//						//case 66: // DECNKM - Numeric Keypad Mode - Set/application sequences
//						//case 67: // DECBKM - Backarrow Key Mode - backspace mode
//						//case 68: // DECKBUM - Typewriter or Data Processing Keys - set/data keys
//						//case 69: // DECLRMM - Left Right Margin Mode - Set: DECSLRM can set margins.
//						//case 73: // DECXRLM - Transmit Rate Limiting - set/limit transmit rate
//						//case 81: // DECKPM - Key Position Mode - set/send key position reports
//						//case 96: // DECRLCM - Right-to-Left Copy - Enable/right-to-left copy
//						//case 97: // DECCRTSM - Set/Reset CRT Save Mode - set/enable
//						//case 98: // DECARSM — Set/Reset Auto Resize Mode - set/enable
//						//case 99: // DECMCM - Set/Reset Modem Control Mode - enable modem control
//						//case 100: // DECAAM - Set/Reset Auto Answerback Mode - Set/Enable auto answerback
//						//case 101: // DECCANSM - Conceal Answerback Message Mode
//						//case 102: // DECNULM - Set/Reset Ignoring Null Mode - Set/ignore NULL - default
//						//case 103: // DECHDPXM - Set/Reset Half-Duplex Mode - half-duplex mode
//						//case 104: // DECESKM - Enable Secondary Keyboard Language Mode - secondary mode
//						//case 106: // DECOSCNM - Set/Reset Overscan Mode - enable overscan
//						//case 108: // DECNUMLK - Num Lock Mode - set
//						//case 109: // DECCAPSLK - Caps Lock Mode - set
//						//case 110: // DECKLHIM - Keyboard LED's Host Indicator Mode - set
//					default:
//						break;
//					}
//				}
//				else
//				{
//					switch (selector)
//					{
//					case 12: // SRM - Local Echo: Send/Receive Mode - echo off
//						m_localEcho = false;
//						break;
//						//case 2: // KAM - Keyboard Action Mode - set/locks the keyboard
//						//case 3: // CRM - Show Control Character Mode - set/show control chars
//						//case 4: // IRM - Insert/Replace Mode - set/insert mode
//						//case 20: // LNM - Line Feed/New Line Mode - set new line mode
//					default:
//						break;
//					}
//				}
//			}
//			break;
//			case 'l':
//			{
//				m_isMidSeq = false;
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (m_gotQmark)
//				{
//					switch (selector)
//					{
//					case 2: // DECANM - ANSI Mode (actually VT52 mode) enable
//						m_vt52Compat = true;
//						break;
//					case 25: // DECTCEM - Text Cursor Enable Mode - hide cursor
//								// TBD
//						break;
//						//case 1: // DECCKM - Cursor Keys Mode - reset
//						//case 3: // 80 column mode
//						//case 4: // DECSCLM - Scrolling Mode - jump scroll
//						//case 5: // DECSCNM - Screen Mode: Light or Dark Screen - reset/normal video
//						//case 6: // DECOM - Origin Mode - reset/upper-left corner
//						//case 7: // DECAWM  Autowrap Mode - reset/disable
//						//case 8: // DECARM - Autorepeat Mode - disable auto-repeat
//						//case 18: // DECPFF - Print Form Feed Mode - no FF
//						//case 19: // DECPEX - Printer Extent Mode - prints scrolling region only
//						//case 20: // LF mode
//						//case 34: // DECRLM - Cursor Right to Left Mode - reset
//						//case 42: // DECNRCM - National Replacement Character Set Mode - reset/8-bit characters
//						//case 47: //?47 ignored (XTERM) switch to alternate screen
//						//case 58: // DECIPEM - Enter/Return from IBM ProPrinter Emulation Mode - reset/return
//						//case 60: // DECHCCM - Horizontal Cursor-Coupling Mode - reset/uncouples cursor
//						//case 61: // DECVCCM - Vertical Cursor-Coupling Mode - reset/uncouples cursor
//						//case 64: // DECPCCM - Page Cursor-Coupling Mode - reset/uncouples cursor
//						//case 66: // DECNKM - Numeric Keypad Mode - Reset/keypad characters
//						//case 67: // DECBKM - Backarrow Key Mode - delete mode
//						//case 68: // DECKBUM - Typewriter or Data Processing Keys - reset/typewritter
//						//case 69: // DECLRMM - Left Right Margin Mode - Reset: DECSLRM cannot set margins.
//						//case 73: // DECXRLM - Transmit Rate Limiting - reset/unlimited transmit rate
//						//case 81: // DECKPM - Key Position Mode - reset/send character codes
//						//case 96: // DECRLCM - Right-to-Left Copy - Disable/left-to-right copy
//						//case 97: // DECCRTSM - Set/Reset CRT Save Mode - disable
//						//case 98: // DECARSM — Set/Reset Auto Resize Mode - reset/disable
//						//case 99: // DECMCM - Set/Reset Modem Control Mode - disable modem control - default
//						//case 100: // DECAAM - Set/Reset Auto Answerback Mode - Reset/Disable auto answerback
//						//case 101: // DECCANSM - Conceal Answerback Message Mode
//						//case 102: // DECNULM - Set/Reset Ignoring Null Mode - Reset/accept NULL
//						//case 103: // DECHDPXM - Set/Reset Half-Duplex Mode - full-duplex mode
//						//case 104: // DECESKM - Enable Secondary Keyboard Language Mode - primary mode
//						//case 106: // DECOSCNM - Set/Reset Overscan Mode - disable overscan - default
//						//case 108: // DECNUMLK - Num Lock Mode - Reset
//						//case 109: // DECCAPSLK - Caps Lock Mode - reset
//						//case 110: // DECKLHIM - Keyboard LED's Host Indicator Mode - reset
//					default:
//						break;
//					}
//				}
//				else
//				{
//					switch (selector)
//					{
//					case 12: // SRM - Local Echo: Send/Receive Mode - echo on
//						m_localEcho = true;
//						break;
//						//case 2: // KAM - Keyboard Action Mode - reset/unlocks the keyboard
//						//case 3: // CRM - Show Control Character Mode - reset/interpret control chars
//						//case 4: // IRM - Insert/Replace Mode - replace mode
//						//case 20: // LNM - Line Feed/New Line Mode - reset/line feed mode
//					default:
//						break;
//					}
//				}
//			}
//			break;
//			case 't': // DECSLPP - Set num lines per page.
//				m_isMidSeq = false; // Ignored, we don't support multi-page mode
//				break;
//			case 'E': // CNL - Cursor Next Line - Move cursor to beginning of line n lines down
//				m_isMidSeq = false;
//				break;
//			case 'F': // CPL - Cursor Previous Line - Move cursor to beginning of line n lines up
//				m_isMidSeq = false;
//				break;
//			case 'S': // SU - Pan Down - Scroll up n lines
//				m_isMidSeq = false;
//				break;
//			case 'T': // SD - Pan Up - Scroll down n lines
//				m_isMidSeq = false;
//				break;
//			case 'r': // Set the scroll region
//				m_isMidSeq = false;
//				if (m_gotQmark)
//				{
//					// DECPCTERM - Enter/Exit PCTerm or Scancode Mode
//				}
//				else
//				{
//					// DECSTBM - Set Top and Bottom Margins
//					unsigned int screenHeightInChars = m_screenHeightInChars;
//					unsigned int top = 0;
//					if (!m_seqVars.is_empty())
//						top = m_seqVars[0];
//					if (top > screenHeightInChars)
//						top = screenHeightInChars;
//					if (top > 0)
//						top--;
//					unsigned int bottom = screenHeightInChars;
//					if (m_seqVars.get_length() > 1)
//						bottom = m_seqVars[1];
//					if (top > screenHeightInChars)
//						top = screenHeightInChars;
//					if (bottom > 0)
//						bottom--;
//					m_topScrollLine = top;
//					m_bottomScrollLine = bottom;
//					m_cursorX = 0;
//					m_cursorY = top;
//				}
//				break;
//			case 'X': // ECH - Erase Character - erase # chars
//			{
//				m_isMidSeq = false;
//				unsigned int screenWidthInChars = m_screenWidthInChars;
//				unsigned int pos = 1;
//				if (!m_seqVars.is_empty())
//					pos = m_seqVars[0];
//				unsigned int charsLeft = (screenWidthInChars - m_cursorX) + 1;
//				if (pos > charsLeft)
//					pos = charsLeft;
//				if (pos > 0)
//					pos--; // 0 = clear 1 char, 1 = clear 2 chars
//				clean_line(m_cursorY, m_cursorX, m_cursorX + pos);
//				invalidate_line(m_cursorY, m_cursorX, m_cursorX + pos);
//			}
//			break;
//			case 'c': // DA1 - Primary Device Attributes - Send identification string
//			{
//				m_isMidSeq = false;
//				unsigned int selector = 0;
//				if (!m_seqVars.is_empty())
//					selector = m_seqVars[0];
//				if (m_gotQmark)
//				{
//					// DA1 response
//					//switch (selector)
//					//{
//					//default:
//					//	break;
//					//}
//				}
//				else
//				{
//					switch (selector)
//					{
//					case 0: // Host Request - response: ESC[?<classcode>;<extentions>c
//					{
//						// respond with no options
//						uint8_t msg[7] = { ESC, '[', '?', '1', ';', '0', 'c' };
//						queue_to_datasource(io::buffer((char*)&msg[0], 7));
//					}
//					break;
//					// Class code:
//					//	 1 = VT100
//					// Extensions:
//					//	0 No options ESC
//					//	1 Processor option (STP) ESC
//					//	2 Advanced video option (AVO) ESC
//					//	3 AVO and STP ESC
//					//	4 Graphics option (GPO) ESC
//					//	5 GPO and STP ESC
//					//	6 GPO and AVO ESC
//					//	7 GPO, STP and AVO ESC
//					//
//					// Class code:
//					//	64 = VT510 class code
//					// Extensions:
//					//	1 132 columns
//					//	2 Printer port
//					//	4 Sixel
//					//	6 Selective erase
//					//	7 Soft character set (DRCS)
//					//	8 User-defined keys (UDKs)
//					//	9 National replacement character sets (NRCS) (International terminal only)
//					//	12 Yugoslavian (SCS)
//					//	15 Technical character set
//					//	18 Windowing capability
//					//	21 Horizontal scrolling
//					//	23 Greek
//					//	24 Turkish
//					//	42 ISO Latin-2 character set
//					//	44 PCTerm
//					//	45 Soft key map
//					//	46 ASCII emulation
//					default:
//						break;
//					}
//				}
//			}
//			break;
//			case '`': // HPA - Horizontal Position Absolute
//				m_isMidSeq = false;
//				break;
//			case 'a': // HPR - Horizontal Position Relative
//				m_isMidSeq = false;
//				break;
//			case 'e': // VPR - Vertical Position Relative - relative line #
//				m_isMidSeq = false;
//				break;
//			case 'I': // CHT - Cursor Horizontal Forward Tabulation
//				m_isMidSeq = false;
//				break;
//			case '~': // DECFNK - Function Key
//				m_isMidSeq = false;
//				break;
//				//case 'N': // ANSI Music?
//				//case '"': // ??
//				//case 'Z': // CBT - Cursor Backward Tabulation
//				//case 'U': // NP - Next Page
//				//case 'V': // PP - Preceding Page
//				//case '-': // deletes entire column?
//				//case '}': // insert column
//				//case 'X': // erase character
//				//case 'g': //	TBC - Tab Clear
//				//ESC [g  clear tab at current column
//				//ESC [3g  clear all tabs
//				//case 'i': //	?? printer?
//				//ESC [4i  ignored Stop relay to printer
//				//ESC [5i  ignored Start relay to printer
//				//case 'x': // ignored Send terminal parameter report - also req?
//				//case 'q': // keyboard LEDs?
//				//case ' q': // DECSCUSR — Set Cursor Style
//				// 0,1 - blinking block
//				//	2 - steady block
//				//	3 - blinking underline
//				//	4 - steady underline
//				//case 'y': // confidence test
//				//case 'p': // DECSSL - Select Set-Up Language
//				// ESC [>c or [>0c - DA2 - Secondary Device Attributes
//				// ESC [=c or [=0c - DA3 - Tertiary  Device Attributes
//				// ESC [=c or [=0c - DA3 - Tertiary  Device Attributes
//				// ESC [#'~ - DECDC - Delete Column
//				// ESC [#'} - DECIC - Insert Column
//				// ESC [#*x - DECSACE - Select Attribute Change Extent
//				// ESC [#*z - DECINVM - Invoke Macro
//				// ESC [#+z - DECPKA - Program Key Action
//				// ESC [#;# SP } - DECKBD - Keyboard Language Selection
//				// ESC [<hour>;<minute>,p - DECLTOD - Load Time of Day
//				// ESC [#*{ - DECMSR - Macro Space Report
//				// ESC [#;#+y - DECPKFMR - Program Key Free Memory Report
//				// ESC [#;#,v - DECRPKT - Report Key Type
//				// ESC [#;#$y - DECRPM - Report Mode - Terminal To Host
//				// ESC [?#;#$y - DECRPM - Report Mode - Terminal To Host
//				// ESC [#*p - DECSPPCS - Select ProPrinter Character Set
//				// ESC [#$p - DECRQM - Request Mode - Host To Terminal
//				// ESC [#$s - DECSPRTT - Select Printer Type
//				// ESC [?#$p - DECRQM - Request Mode - Host To Terminal
//				// ESC ["v - DECRQDE - Request Displayed Extent
//				// ESC [#;#,w - DECRQKD - Request Key Definition
//				// ESC [#,u - DECRQKT - Key Type Inquiry
//				// ESC [+x - DECRQPKFM - Program Key Free Memory Inquiry
//				// ESC [#$w - DECRQPSR - Request Presentation State Report
//				// ESC [#$u - DECRQTSR - Request Terminal State Report
//				// ESC [&u - DECRQUPSS - User-Preferred Supplemental Set
//				// ESC [#$} - DECSASD - Select Active Status Display
//				// ESC [#"q - DECSCA - Select Character Protection Attribute
//				// ESC [#$q - DECSDDT - Select Disconnect Delay Time
//				// ESC [#;#*u - DECSCP - Select Communication Port
//				// ESC [#;#*r - DECSCS - Select Communication Speed
//				// ESC [#$| - DECSCPP - Select 80 or 132 Columns per Page
//				// ESC [#)p - DECSDPT - Select Digital Printed Data Type
//				// ESC [#;#;#;#${ - DECSERA - Selective Erase Rectangular Area
//				// ESC [#;#;#;#*S - DECSFC - Select Flow Control
//				// ESC [ r - "SP r" - DECSKCV - Set Key Click Volume
//				// ESC [ v - "SP v" - DECSLCK - Set Lock Key Style
//				// ESC [# u - "# SP u" - DECSMBV - Set Margin Bell Volume
//				// ESC [#;#..;#+r - DECSMKR - Select Modifier Key Reporting
//				// ESC [#;#..;#+w - DECSPP - Set Port Parameter
//				// ESC [#*| - DECSNLS - Set Lines Per Screen
//				// ESC [#+p - DECSR - Secure Reset
//				// ESC [#*q - DECSRC - Secure Reset Confirmation
//				// ESC [#"t - DECSRFR - Select Refresh Rate
//				// ESC [# p - "SP p" - DECSSCLS - Set Scroll Speed
//				// ESC [#$~ - DECSSDT - Select Status Display (Line) Type
//				// ESC [?5W - DECST8C - Set Tab at Every 8 Columns
//				// ESC [!p - DECSTR - Soft Terminal Reset
//				// ESC [#;#"u - DECSTRL - Set Transmit Rate Limit
//				// ESC [# t - "SP t" - DECSWBV - Set Warning Bell Volume
//				// ESC [# ~ - "SP ~" - DECTME - Terminal Mode Emulation
//				// ESC [#,q - DECTID - Select Terminal ID
//				// ESC [#$u - DECTSR - Terminal State Report - Terminal to Host
//				// ESC [#i - MC - Media Copy - ANSI
//				// ESC [?#i - MC - Media Copy - VT mode
//				// ESC [# P - PPA - Page Position Absolute
//				// ESC [# R - PPB - Page Position Backward
//				// ESC [# Q - PPR - Page Position Relative
//				// ESC [?#;...;#l - RM - Reset Mode
//			default:
//				m_isMidSeq = false;
//				break;
//			}
//		}
//	}
//
//	void calc_screen(unsigned int& widthInChars, unsigned int& heightInChars)
//	{
//		unsigned int screenWidthInChars;
//		screenWidthInChars = (unsigned int)(get_size().get_width() / m_charWidthInPixels);
//		if (!screenWidthInChars)
//			screenWidthInChars = 1;
//		else if (screenWidthInChars > m_bufferWidthInChars)
//			screenWidthInChars = m_bufferWidthInChars;
//
//		unsigned int screenHeightInChars;
//		screenHeightInChars = (unsigned int)(get_size().get_height() / m_charHeightInPixels);
//		if (!screenHeightInChars)
//			screenHeightInChars = 1;
//		else if (screenHeightInChars > m_bufferHeightInChars)
//			screenHeightInChars = m_bufferHeightInChars;
//
//		heightInChars = screenHeightInChars;
//		widthInChars = screenWidthInChars;
//	}
//
//	void calc_screen()
//	{
//		unsigned int screenWidthInChars;
//		unsigned int screenHeightInChars;
//		calc_screen(screenWidthInChars, screenHeightInChars);
//		m_screenWidthInChars = screenWidthInChars;
//		m_screenHeightInChars = screenHeightInChars;
//	}
//
//	ansiterm(unsigned int fontSize, unsigned int bufHeight, unsigned int bufWidth, bool whiteBackground = false)
//		: m_bufferHeightInChars(bufHeight),
//		m_bufferWidthInChars(bufWidth),
//		m_charWidthInPixels(get_char_width(fontSize)),
//		m_charHeightInPixels(get_char_height(fontSize)),
//		m_whiteBackground(whiteBackground), // usual is black
//		m_scrollPos(0),
//		m_cursorX(0),
//		m_cursorY(0),
//		m_savedPosX(0),
//		m_savedPosY(0),
//		m_savedPos78X(0),
//		m_savedPos78Y(0),
//		m_screenTop(0),
//		m_stripMode(0xFF),
//		m_hadESC(false),
//		m_parseANSISeq(true),
//		m_gotQmark(false),
//		m_seqVarValid(false),
//		m_insertMode(false),
//		m_isMidSeq(false),
//		m_wrapMode(true), // wrapping behavior can be affected by certain ANSI sequences
//		m_addCRonLF(false),
//		m_tabSpaces(8),
//		m_hasBufferLooped(false),
//		m_cursorVis(true),
//		m_vt52Compat(false),
//		m_vt100Compat(false),
//		m_rawMode(false),
//		m_localEcho(false),
//		m_cursorMode(false),
//		m_fontSize(fontSize)
//		//m_blinkTimerExpireDelegate([r{ this_weak_rcptr }]()
//		//{
//		//	rcptr<ansiterm> r2 = r;
//		//	if (!!r2)
//		//		r2->blink_timer_expired();
//		//})
//	{
//		auto vStateGetter = [this]()
//		{
//			return *(m_vScrollBarState.begin_read());
//		};
//
//		placement_rcnew(&m_vScrollBarStateProperty.get(), this_desc)(*this, std::move(vStateGetter));
//
//		auto vPositionGetter = [this]()
//		{
//			return atomic::load(m_vScrollBarPosition);
//		};
//
//		auto vPositionSetter = [this](double d)
//		{
//			double newPos = d;
//			double oldPos = atomic::exchange(m_vScrollBarPosition, newPos);
//			if (oldPos != newPos)
//			{
//				m_vScrollBarPositionProperty->changed();
//				//scrolled();
//			}
//			m_vScrollBarPositionProperty->set_complete();
//		};
//
//		placement_rcnew(&m_vScrollBarPositionProperty.get(), this_desc)(*this, std::move(vPositionGetter), std::move(vPositionSetter));
//
//
//		// TODO TO DO, fix
//		//set_background_color(whiteBackground ? color::constant::white : color::constant::black);
//
//		m_forePallete[0] = color(0xB2, 0xB2, 0xB2);
//		m_forePallete[1] = color(0xB5, 0x00, 0x00);
//		m_forePallete[2] = color(0x00, 0xA5, 0x00);
//		m_forePallete[3] = color(0x00, 0xC0, 0xC0);
//		m_forePallete[4] = color(0x00, 0x00, 0xFF);
//		m_forePallete[5] = color(0xB1, 0xB1, 0x00);
//		m_forePallete[6] = color(0xB1, 0x00, 0xB1);
//		m_forePallete[7] = color(0x00, 0x00, 0x00);
//
//		m_forePalleteBold[0] = color(0xFF, 0xFF, 0xFF);
//		m_forePalleteBold[1] = color(0xFF, 0x00, 0x00);
//		m_forePalleteBold[2] = color(0x00, 0xFF, 0x00);
//		m_forePalleteBold[3] = color(0x00, 0xFF, 0xFF);
//		m_forePalleteBold[4] = color(0x36, 0x78, 0xFF);
//		m_forePalleteBold[5] = color(0xFF, 0xFF, 0x00);
//		m_forePalleteBold[6] = color(0xFF, 0x00, 0xFF);
//		m_forePalleteBold[7] = color(0x40, 0x40, 0x40);
//
//		m_backPallete[0] = color(0x00, 0x00, 0x00);
//		m_backPallete[1] = color(0xB5, 0x00, 0x00);
//		m_backPallete[2] = color(0x00, 0xA5, 0x00);
//		m_backPallete[3] = color(0x00, 0xC0, 0xC0);
//		m_backPallete[4] = color(0x00, 0x00, 0xFF);
//		m_backPallete[5] = color(0xB1, 0xB1, 0x00);
//		m_backPallete[6] = color(0xB1, 0x00, 0xB1);
//		m_backPallete[7] = color(0xFF, 0xFF, 0xFF);
//
//		calc_screen();
//
//		m_curBrush.value = 0;
//		m_savedBrush.value = 0;
//		m_savedBrush78.value = 0;
//		m_bufSize = (size_t)bufHeight * bufWidth;
//
//		m_charBuffer = default_allocator::allocate_type<uint8_t>(m_bufSize);
//		memset((void*)(m_charBuffer), 0, m_bufSize); // * sizeof(uint8_t)
//
//		m_brushBuffer = default_allocator::allocate_type<brush>(m_bufSize);
//		memset((void*)(m_brushBuffer), 0, m_bufSize * sizeof(brush));
//
////		size_t screenChars = m_screenWidthInChars * m_screenHeightInChars;
////		m_updateTable.append(false, screenChars); // started empty
//
//		m_bufPos = 0;
//	}
//
//	static constexpr unsigned char BELL = 0x07;
//	static constexpr unsigned char BS = 0x08;
//	static constexpr unsigned char TAB = 0x09;
//	static constexpr unsigned char LF = 0x0A;
//	static constexpr unsigned char VT = 0x0B;
//	static constexpr unsigned char FF = 0x0C;
//	static constexpr unsigned char HOME = 0x0C;
//	static constexpr unsigned char CR = 0x0D;
//	static constexpr unsigned char ESC = 0x1B;
//	static constexpr unsigned char SPACE = 0x20;
//	static constexpr unsigned char DEL = 0x7F;
//	//static constexpr unsigned char MUSICNOTE = 0x0E;
//	static constexpr unsigned char CSI = 0x9B;
//	static constexpr unsigned char DCS = 0x90;
//	static constexpr unsigned char ST = 0x9C;
//	static constexpr unsigned char SS3 = 0x8F;
//	static constexpr unsigned char IND = 0x84;
//	static constexpr unsigned char RI = 0x8D;
//	static constexpr unsigned char NEL = 0x85;
//	static constexpr unsigned char HTS = 0x88;
//
//public:
//	virtual rcref<dependency_property<scroll_bar_state, io::permission::read> > get_state_property()
//	{ return get_self_rcref(&m_vScrollBarStateProperty.get()).template static_cast_to<dependency_property<scroll_bar_state, io::permission::read>>(); }
//
//	virtual rcref<dependency_property<double> > get_position_property()
//	{ return get_self_rcref(&m_vScrollBarPositionProperty.get()).template static_cast_to<dependency_property<double>>(); }
//
//	static int get_char_width(unsigned int fontSize) // Only 0, 1, 2 supported
//	{
//		if (fontSize > 2)
//			return 0;
//		int widths[] = { 6, 7, 10 };
//		return widths[fontSize];
//	}
//
//	static int get_char_height(unsigned int fontSize) // Only 0, 1, 2 supported
//	{
//		if (fontSize > 2)
//			return 0;
//		int heights[] = { 11, 16, 18 };
//		return heights[fontSize];
//	}
//
//	static unsigned int get_default_width(unsigned int fontSize) { return get_char_width(fontSize) * 80; }
//	static unsigned int get_default_height(unsigned int fontSize) { return get_char_height(fontSize) * 24; }
//
//	~ansiterm()
//	{
//		//if (!!m_datastream)
//		//	m_datastream->close();
//
//		default_allocator::destruct_deallocate_type(m_charBuffer, m_bufSize);
//		default_allocator::destruct_deallocate_type(m_brushBuffer, m_bufSize);
//	}
//
//
//	virtual void drawing()
//	{
//		unsigned int blinkState = m_blinkState;
//		color backgroundColor;
//		if (m_whiteBackground)
//			backgroundColor = m_backPallete[BackWhite];
//		else
//			backgroundColor = m_backPallete[BackBlack];
//
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//
//		int  rightEdge = screenWidthInChars * m_charWidthInPixels;
//
//		uint32_t curRow = 0; // in coordinates of the visible portion
//		for (;;)
//		{
//			uint32_t curColumn = 0;
//			point dstPt;
//			dstPt.set_y(curRow * m_charHeightInPixels);
//			do {
//				dstPt.set_x(curColumn * m_charWidthInPixels);
//
//				bounds dstBounds(dstPt, size(m_charWidthInPixels, m_charHeightInPixels));
//				if (is_unclipped(dstBounds))
//				{
//					uint32_t curBufLine = curRow + m_scrollPos;
//					curBufLine %= m_bufferHeightInChars;
//
//					size_t i = (curBufLine * m_bufferWidthInChars) + curColumn;
//					uint8_t c = m_charBuffer[i];
//
//					bool doUnderline = m_brushBuffer[i].underlined;
//					uint16_t blinkMode = m_brushBuffer[i].blinking;
//					if (blinkMode != 0)
//					{
//						if (blinkMode == 1)
//						{
//							if ((blinkState >> 1) % 2)
//							{
//								c = ' ';
//								doUnderline = false;
//							}
//						}
//						else // if (blinkMode == 3)
//						{
//							if (blinkState % 2)
//							{
//								c = ' ';
//								doUnderline = false;
//							}
//						}
//
//					}
//
//					int fontRow = c / 16;
//					int fontColumn = c % 16;
//
//					int foreColorIndex = m_brushBuffer[i].foreFlag;
//					int backColorIndex = m_brushBuffer[i].backFlag;
//
//					if (m_brushBuffer[i].inverse)
//					{
//						int tmp = foreColorIndex;
//						foreColorIndex = backColorIndex;
//						backColorIndex = tmp;
//						if (!foreColorIndex)
//							foreColorIndex = 7;
//						if (!backColorIndex)
//							backColorIndex = 7;
//					}
//
//					color foreColor;
//					if (m_brushBuffer[i].bold)
//						foreColor = m_forePalleteBold[foreColorIndex];
//					else
//						foreColor = m_forePallete[foreColorIndex];
//					draw_bitmap(*m_fontBitMap,
//						bounds(point((fontColumn * m_charWidthInPixels) + 3, (fontRow * m_charHeightInPixels) + 3),
//							size(m_charWidthInPixels, m_charHeightInPixels)),
//						bounds(dstPt, size(m_charWidthInPixels, m_charHeightInPixels)),
//						foreColor,
//						m_backPallete[backColorIndex]);
//
//
//					//if (m_cursorMode == false) // underline mode
//					//{
//					//	if (m_cursorVis && ((blinkState >> 1) % 2))
//					//	{
//					//		uint32_t cursorRowBufLine = m_cursorY + m_screenTop;
//					//		cursorRowBufLine %= m_bufferHeightInChars;
//
//					//		if (curBufLine == cursorRowBufLine)
//					//		{
//					//			if ((curColumn == m_cursorX) || ((curColumn == screenWidthInChars-1) && (m_cursorX >= screenWidthInChars)))
//					//			{
//					//				doUnderline = !doUnderline;
//					//			}
//					//		}
//					//	}
//					//}
//
//					if (doUnderline)
//					{
//						point lineStart = dstPt;
//						lineStart.get_y() += (m_charHeightInPixels - 1);
//						point lineEnd = lineStart;
//						lineEnd.get_x() += m_charWidthInPixels;
//						draw_line(lineStart, lineEnd, foreColor);
//					}
//
//					//		if (m_cursorMode == true) // block mode
//					{
//						if (m_cursorVis && ((blinkState >> 1) % 2))
//						{
//							uint32_t cursorRowBufLine = m_cursorY + m_screenTop;
//							cursorRowBufLine %= m_bufferHeightInChars;
//
//							if (curBufLine == cursorRowBufLine)
//							{
//								if ((curColumn == m_cursorX) || ((curColumn == screenWidthInChars - 1) && (m_cursorX >= screenWidthInChars)))
//								{
//									point cursorPt = dstPt;
//									uint16_t cursorHeight = m_charHeightInPixels;
//									if (m_cursorMode == false)
//									{
//										cursorPt.get_y() += cursorHeight;
//										cursorHeight /= 5;
//										if (!cursorHeight)
//											cursorHeight = 1;
//										cursorPt.get_y() -= cursorHeight;
//									}
//									invert(bounds(cursorPt, size(m_charWidthInPixels, cursorHeight)));
//								}
//							}
//						}
//					}
//				}
//
//			} while (++curColumn != screenWidthInChars);
//			++curRow;
//			if (m_scrollPos == m_screenTop)
//			{
//				if (curRow == screenHeightInChars)
//				{
//					int bottomEdge = screenHeightInChars * m_charHeightInPixels;
//					bounds bottomBleedRect(point(0, bottomEdge), size(rightEdge, get_size().get_height() - bottomEdge));
//					fill(bottomBleedRect, backgroundColor);
//					break;
//				}
//			}
//			else
//			{
//				if (curRow > screenHeightInChars)
//					break;
//			}
//		}
//		bounds rightBleedRect(point(rightEdge, 0), size(get_size().get_width() - rightEdge, get_size().get_height()));
//		fill(rightBleedRect, backgroundColor);
//	}
//
//	void insert_inner(char c)
//	{
//		// Doesn't do any drawing, only 'invalidate' and change data.
//		char c2 = c & m_stripMode;
//		if (m_isMidSeq)
//			parse_mid_sequence(c2);
//		else if (m_hadESC) // if last character received was an ESC
//			parse_esc_code(c2);
//		else if (m_rawMode)
//			raw_insert(c2);
//		else
//			parse(c2);
//	}
//
//	void insert_inner(const io::composite_buffer& buf)
//	{
//		// Doesn't do any drawing, only 'invalidate' and change data.
//		io::composite_buffer::const_iterator itor = buf.get_first_const_iterator();
//		while (!!itor)
//		{
//			insert_inner(*itor);
//			++itor;
//		}
//	}
//
//	void insert_inner(const io::buffer& buf)
//	{
//		// Doesn't do any drawing, only 'invalidate' and change data.
//		io::buffer::const_iterator itor = buf.get_first_const_iterator();
//		while (!!itor)
//		{
//			insert_inner(*itor);
//			++itor;
//		}
//	}
//
//	void resized()
//	{
//		// handle resize
//		int32_t oldHeight = m_screenHeightInChars;
//		int32_t oldWidth = m_screenWidthInChars;
//
//		calc_screen();
//
//		int32_t screenHeightInChars = m_screenHeightInChars;
//		int32_t screenWidthInChars = m_screenWidthInChars;
//
//		if ((screenHeightInChars != oldHeight) || (screenWidthInChars != oldWidth))
//		{
//			size_t screenChars = screenHeightInChars * screenWidthInChars;
//			//					m_updateTable.resize(screenChars);
//			//					memset(&(m_updateTable[0]), 0, screenChars * sizeof(bool));
//
//			send_window_size(screenWidthInChars, screenHeightInChars); // Tell telnet server of new size
//
//			if (screenHeightInChars < oldHeight)
//			{
//				// height is shrinking
//				// Bottom line must be preserved, content pushes up into scroll buffer
//				uint32_t dif = oldHeight - screenHeightInChars;
//				if (m_cursorY >= dif)
//				{
//					m_cursorY -= dif;
//					COGS_ASSERT(m_cursorY < 999);
//				}
//				else
//					m_cursorY = 0;
//				bool tracking = m_screenTop == m_scrollPos;
//				m_screenTop += dif;
//				m_screenTop %= m_bufferHeightInChars;
//				if (tracking)
//					m_scrollPos = m_screenTop;
//				//else // no change?
//				calc_buf_pos();
//			}
//			else if (screenHeightInChars > oldHeight)
//			{
//				// height is being added
//				// Bottom line must remain the bottom line.
//				// Content is pulled down from scroll buffer.
//				// If no content in scroll buffer, new lines are added
//				uint32_t dif = screenHeightInChars - oldHeight;
//				m_cursorY += dif;
//				COGS_ASSERT(m_cursorY < 999);
//				uint32_t oldScreenTop = m_screenTop;
//
//				// If there is enough content above, just update screenPos
//				if (m_hasBufferLooped)
//				{
//					m_screenTop -= dif;
//					if (m_screenTop >= m_bufferHeightInChars)
//					{
//						m_screenTop += m_bufferHeightInChars;
//						if ((m_scrollPos <= oldScreenTop) || (m_scrollPos > m_screenTop))
//							m_scrollPos = m_screenTop;
//					}
//					else
//					{
//						if ((m_scrollPos <= oldScreenTop) && (m_scrollPos > m_screenTop))
//							m_scrollPos = m_screenTop;
//					}
//				}
//				else
//				{
//					if (m_screenTop >= dif)
//						m_screenTop -= dif;
//					else
//					{
//						uint32_t numLinesReclaimed = m_screenTop;
//						uint32_t numLinesNeeded = dif - numLinesReclaimed;
//						clean_lines(oldHeight, numLinesNeeded);
//						m_screenTop = 0;
//						m_cursorY -= numLinesNeeded;
//						COGS_ASSERT(m_cursorY < 999);
//					}
//					if (m_scrollPos > m_screenTop)
//						m_scrollPos = m_screenTop;
//				}
//				calc_buf_pos();
//			}
//			if (screenHeightInChars != oldHeight)
//				fix_scrollbar();
//			invalidate(get_size());
//		}
//	}
//
//	void scrolled(uint8_t dimension, uint32_t pos)
//	{
//		if (dimension == 1)
//		{
//			uint32_t topOfAllLine = 0;
//			if (m_hasBufferLooped)
//			{
//				uint16_t screenHeightInChars = m_screenHeightInChars;
//				topOfAllLine = m_screenTop + screenHeightInChars;
//				topOfAllLine %= m_bufferHeightInChars;
//			}
//			m_scrollPos = topOfAllLine + pos;
//			m_scrollPos %= m_bufferHeightInChars;
//
//			invalidate(get_size()); // TBD be more efficient with drawing
//		}
//	}
//
//	virtual cstring get_telnet_terminal_type() { return cstring::literal("ANSI"); }
//
//	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers)
//	{
//		COGS_ASSERT(m_cursorY < 999);
//		switch (c)
//		{
//		case ui::UpArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'A' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'A' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 3));
//			}
//			return true;
//		case ui::DownArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'B' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'B' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 3));
//			}
//			return true;
//		case ui::RightArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'C' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'C' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 3));
//			}
//			return true;
//		case ui::LeftArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'D' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'D' };
//				queue_to_datasource(io::buffer((char*)&msg[0], 3));
//			}
//			return true;
//		default:
//			;
//		}
//		COGS_ASSERT(m_cursorY < 999);
//		return false;
//	}
//
//	virtual bool key_releasing(wchar_t c)
//	{
//		COGS_ASSERT(m_cursorY < 999);
//		switch (c)
//		{
//		case ui::UpArrowKey:
//		case ui::DownArrowKey:
//		case ui::RightArrowKey:
//		case ui::LeftArrowKey:
//			return true;
//		default:
//			;
//		}
//		COGS_ASSERT(m_cursorY < 999);
//		return false;
//	}
//
//	virtual bool character_typing(wchar_t c, const ui::modifier_keys_state& modifiers)
//	{
//		if (m_scrollPos != m_screenTop)
//		{
//			m_scrollPos = m_screenTop;
//			invalidate(get_size());
//		}
//		if (m_localEcho)
//			insert_inner((char)c);
//
//		char c2 = (char)c;
//		queue_to_datasource(io::buffer(&c2, 1));
//	}
//
//	virtual bool process_write(io::composite_buffer& compBuf)
//	{
//		io::composite_buffer tmpBuf;
//		compBuf.swap(tmpBuf);
//		dispatch([r{ this_weak_rcptr }, tmpBuf]()
//		{
//			rcptr<ansiterm> r2 = r;
//			if (!!r2)
//				r2->insert_inner(tmpBuf);
//		});
//
//		return true;
//	}
//
//private:
//	virtual void installing()
//	{
//		pane::installing();
//		m_fontBitMap = load_bitmask(string::literal(L"ANSIFont") + int_to_fixed_integer_t<unsigned int>(m_fontSize).to_string(), true, get_dpi()),
//	}
//
//	virtual void uninstalling()
//	{
//		m_fontBitMap.release();
//		pane::uninstalling();
//	}
//
//#if 0
//
//public:
//	virtual bool process_write(io::composite_buffer& compBuf)
//	{
//		return false;
//	}
//	/*
//	// "Screen" refers to the (bottom) terminal emulation portion of the buffer
//	// "View" refers to the currently visible portion, which could be scrolled up
//	// "Buffer" refers to the entire (circular) buffer contents (and may extend past the right)
//
//	// 32-bits is probably enough.  Who needs a 4G buffer
//	const uint32_t m_charWidthInPixels;
//	const uint32_t m_charHeightInPixels;
//
//	volatile uint16_type m_screenWidthInChars; // Both the view and screen size
//	volatile uint16_type m_screenHeightInChars; // volatile, because resize and insert could occur parallel with draw
//
//	uint32_t m_bufferHeightInChars; // m_bufferNumLines?
//	uint32_t m_bufferWidthInChars; //
//	size_t m_bufSize;
//
//	uint32_t m_scrollPos; // line number on top left pixel in view
//	uint32_t m_screenTop;
//
//	typedef union {
//		struct {
//			uint16_t padding : 3;
//			uint16_t blinking : 2; // 1= blinking, 3=rapid blinking
//			uint16_t foreFlag : 3; //3 bits - 8 foreground colors
//
//			uint16_t bold : 1; // aka 'intense'
//			uint16_t underlined : 1;
//			uint16_t selected : 1; // If currently hilited/selected
//			uint16_t inverse : 1; // Not sure why not just inverse colors instead
//			uint16_t invisible : 1; // ??
//			uint16_t backFlag : 3; //3 bits - 8 background colors
//		};
//		uint16_t value;
//	} brush;
//
//	color m_forePallete[8];
//	color m_forePalleteBold[8];
//	color m_backPallete[8];
//
//	static constexpr uint16_t BackBlack = 0;
//	static constexpr uint16_t ForeWhite = 0;
//	static constexpr uint16_t Red = 1;
//	static constexpr uint16_t Green = 2;
//	static constexpr uint16_t Cyan = 3;
//	static constexpr uint16_t Blue = 4;
//	static constexpr uint16_t Yellow = 5;
//	static constexpr uint16_t Magenta = 6;
//	static constexpr uint16_t BackWhite = 7;
//	static constexpr uint16_t ForeBlack = 7;
//
//	uint8_t*	m_charBuffer;
//
//	// Pack down these bits, since one is stored per buffer char
//	brush*		m_brushBuffer;
//	brush m_curBrush;
//
//	brush m_savedBrush;
//	brush m_savedBrush78;
//
//	planar::point<uint16_t> m_savedPos;
//	planar::point<uint16_t> m_savedPos78;
//
//	planar::point<uint16_t> m_cursor;
//
//	bool m_whiteBackground; // reversed meaning of default back/fore
//	uint8_t m_tabSpaces;
//
//	bool m_hasBufferLooped;
//
//	bool m_insertMode;
//	bool m_wrapMode;
//	bool m_addCRonLF;
//
//	rcptr<gfx::canvas::bitmask> m_fontBitMap;
//
//	size_t m_bufPos; // Position in buffer of current cursor pos
//
//	uint8_t m_stripMode; // 0xFF for no strip, 0x7F to strip high bit
//
//	bool m_parseANSISeq;
//	bool m_hadESC;
//	bool m_isMidSeq;
//	bool m_gotQmark;
//	bool m_seqVarValid;
//
//	simple_vector<uint16_t> m_seqVars;
//
//	bool m_cursorVis;
//	bool m_vt52Compat;
//	bool m_vt100Compat;
//
//	bool m_rawMode; // Just raw-insert everything
//
//	bool m_localEcho;
//	bool m_cursorMode; // true = block, false = underline
//
//	uint16_t m_topScrollLine;
//	uint16_t m_bottomScrollLine;
//
//	volatile uint_type m_blinkState; // incremented every 1/4 second - fast blank is 1/4 second, regular blink is 1/2 second
//
//	volatile rcptr<resettable_timer> m_blinkTimer;
//
//
//	function<void()> m_blinkTimerExpireDelegate;
//
//
//#error defer update of blinking state to the main UI thread
//
//	void blink_timer_expired()
//	{
//		inc_blink_state();
//		rcptr<resettable_timer> blinkTimer = m_blinkTimer;
//		blinkTimer->refire();
//		blinkTimer->dispatch(m_blinkTimerExpireDelegate);
//	}
//
//	virtual void initializing()
//	{
//		rcptr<resettable_timer> blinkTimer = rcnew(resettable_timer)(measure<int_type, milliseconds>(250));
//		m_blinkTimer = blinkTimer;
//		blinkTimer->dispatch(m_blinkTimerExpireDelegate);
//	}
//
//	void inc_blink_state()
//	{
//		m_blinkState++;
//
////		defer_invalidates();
//
//		// Scan through present screen, invalidating anything with blinking contents
//		invalidate_cursor();
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		uint16_t scrollPos = m_scrollPos;
//		for (uint16_t y = 0; y < screenHeightInChars; y++)
//		{
//			for (uint16_t x = 0; x < screenWidthInChars; x++)
//			{
//				size_t i = (((y + scrollPos) * m_bufferWidthInChars) + x) % m_bufSize;
//
//				if (m_brushBuffer[i].blinking != 0)
//				{
//					invalidate(bounds(point(x*m_charWidthInPixels, y*m_charHeightInPixels), size(m_charWidthInPixels, m_charHeightInPixels)));
//				}
//			}
//		}
////		flush_invalidates();
//	}
//
//	void start_sequence()
//	{
//		m_isMidSeq = true;
//		m_gotQmark = false;
//		m_seqVarValid = false;
//		m_seqVars.clear();
//	}
//
//	bounds get_screen_rect(uint16_t onRow, uint16_t fromColumn, uint16_t toColumn) // return empty if cursor not on screen
//	{
//		if (fromColumn > toColumn)
//			return bounds(0, 0, 0, 0);
//
//		onRow += m_screenTop; // Map from screen row to buffer row
//		onRow %= m_bufferHeightInChars; // fixup buffer loop.
//
//		bool onPage = false;
//		uint16_t pageRow;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//		uint32_t bottomOfPageLine = m_scrollPos + screenHeightInChars;
//		bool pageRowsAreNormal = (bottomOfPageLine <= m_bufferHeightInChars);
//		if (pageRowsAreNormal)
//		{ // if on page
//			if ((onRow >= m_scrollPos) && (onRow < m_scrollPos + screenHeightInChars))
//			{
//				onPage = true;
//				pageRow = onRow - m_scrollPos;
//			}
//		}
//		else
//		{
//			bottomOfPageLine %= m_bufferHeightInChars;
//			if (onRow >= m_scrollPos)
//			{
//				onPage = true;
//				pageRow = onRow - m_scrollPos;
//			}
//			else if (onRow < bottomOfPageLine)
//			{
//				onPage = true;
//				pageRow = onRow + (m_bufferHeightInChars - m_scrollPos);
//			}
//		}
//		if (!onPage)
//			return bounds(0, 0, 0, 0);
//
//		return bounds(fromColumn * m_charWidthInPixels, pageRow * m_charHeightInPixels, ((toColumn + 1) - fromColumn) * m_charWidthInPixels, m_charHeightInPixels);
//	}
//
//	void invalidate_cursor()
//	{
//		uint16_t tmpCursorColumn = m_cursorX;
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		if (m_cursorX >= screenWidthInChars)
//			tmpCursorColumn = screenWidthInChars - 1;
//		invalidate(get_screen_rect(m_cursorY, tmpCursorColumn, tmpCursorColumn));
//	}
//
//	void toggle_cursor()
//	{
//		 m_cursorVis = !m_cursorVis;
//		 invalidate_cursor();
//	}
//
//	// line is screen-based
//	void invalidate_line(uint16_t line, uint16_t colStart, uint16_t colEnd)
//	{
//		invalidate(get_screen_rect(line, colStart, colEnd));
//	}
//
//	// line is screen-based, and range will never extend past the end of the screen.
//	void invalidate_lines(uint16_t startLine, uint32_t n)
//	{
//		if (n)
//		{
//			uint16_t screenWidthInChars = m_screenWidthInChars;
//			uint16_t screenHeightInChars = m_screenHeightInChars;
//
//			// Start with the size of the screen
//			bounds screenRect(size(m_charWidthInPixels * screenWidthInChars, m_charHeightInPixels * screenHeightInChars));
//			bounds r(screenRect);
//
//			// trim off top and bottom
//			r.inset(startLine * m_charHeightInPixels, 0, (screenHeightInChars - n) * m_charHeightInPixels, 0);
//
//			// shift to scroll position
//			r += point(0, -(int)(((m_screenTop - m_scrollPos) + m_bufferHeightInChars) % m_bufferHeightInChars));
//
//			// clip to visable area
//			r &= screenRect;
//			invalidate(r);
//		}
//	}
//
//
//	// line is screen-based
//	void clean_line(uint16_t line, uint16_t colStart, uint16_t colEnd)
//	{
//		uint32_t pos = (((m_screenTop + line) * m_bufferWidthInChars) + colStart) % m_bufSize;
//		memset((void*)(m_brushBuffer + pos), 0, (colEnd - colStart) * sizeof(brush));
//		memset((void*)(m_charBuffer + pos), 0, (colEnd - colStart));
//	}
//
//	// line is screen-based
//	void clean_line(uint16_t line) { clean_line(line, 0, m_bufferWidthInChars); }
//
//
//	// line is screen-based
//	void clean_lines(uint16_t startLine, uint32_t n)
//	{
//		if (n)
//		{
//			startLine += m_screenTop;
//			startLine %= m_bufferHeightInChars;
//			uint32_t pos = startLine * m_bufferWidthInChars;
//			uint32_t pastEndLine = startLine + n;
//			if (pastEndLine <= m_bufferHeightInChars)
//			{
//				memset((void*)(m_brushBuffer + pos), 0, n * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer + pos), 0, n * m_bufferWidthInChars);
//			}
//			else
//			{
//				uint32_t linesAfterBufferWrap = pastEndLine - m_bufferHeightInChars;
//				uint32_t linesUntilBufferWrap = n - linesAfterBufferWrap;
//
//				// Clear from startLine through last line
//				memset((void*)(m_brushBuffer + pos), 0, linesUntilBufferWrap * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer + pos), 0, linesUntilBufferWrap * m_bufferWidthInChars);
//
//				// Clear from first line, to ((startLine + n) % m_bufferHeightInChars)
//				memset((void*)(m_brushBuffer), 0, linesAfterBufferWrap * m_bufferWidthInChars * sizeof(brush));
//				memset((void*)(m_charBuffer), 0, linesAfterBufferWrap * m_bufferWidthInChars);
//			}
//		}
//	}
//
//
//	void fix_scrollbar()
//	{
//		uint32_t topOfAllLine;
//		uint32_t beforePage;
//		uint32_t afterPage;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//
//		if (!m_hasBufferLooped)
//		{
//			topOfAllLine = 0;
//			beforePage = m_scrollPos;
//			afterPage = m_screenTop - m_scrollPos;
//		}
//		else
//		{
//			topOfAllLine = m_screenTop + screenHeightInChars;
//			topOfAllLine %= m_bufferHeightInChars;
//
//			beforePage = m_scrollPos - topOfAllLine;
//			if (beforePage >= m_bufferHeightInChars)
//				beforePage += m_bufferHeightInChars;
//
//			if (m_screenTop >= m_scrollPos)
//				afterPage = m_screenTop - m_scrollPos;
//			else
//				afterPage = (m_screenTop + m_bufferHeightInChars) - m_scrollPos;
//		}
//
//		update_scroller(1, beforePage, screenHeightInChars, beforePage + screenHeightInChars + afterPage);
//	}
//
//	void advance_screen(uint32_t n) // Advances the screen (bottom portion of buffer) n lines, possibly reclaiming from the top
//	{
//		// First, check if our advance steps past the end of the storage buffer
//		bool tracking = m_screenTop == m_scrollPos;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//		if (m_screenTop + screenHeightInChars + n >= m_bufferHeightInChars)
//			m_hasBufferLooped = true;
//
//		uint32_t oldScreenTop = m_screenTop;
//		m_screenTop += n;
//		m_screenTop %= m_bufferHeightInChars;
//		if (tracking)
//			m_scrollPos = m_screenTop;
//		else if (m_hasBufferLooped)
//		{ // If scrollPos is being reclaimed, push it forward... Which is tricky to compute, due to buffer looping.
//
//			uint32_t topOfAllLine = oldScreenTop + screenHeightInChars;
//			topOfAllLine %= m_bufferHeightInChars;
//
//			uint32_t oldScrollPos = m_scrollPos;
//			if (oldScrollPos < topOfAllLine)
//				oldScrollPos += m_bufferHeightInChars;
//
//			uint32_t relativeScrollPos = oldScrollPos - topOfAllLine;
//			if (relativeScrollPos < n) // need to advance scroll pos
//			{
//				m_scrollPos += n - relativeScrollPos;
//				m_scrollPos %= m_bufferHeightInChars;
//			}
//		}
//
//		uint32_t firstLineToClean = screenHeightInChars - n;
//		clean_lines(firstLineToClean, n);
//
//		fix_scrollbar();
//		invalidate(get_size());
//	}
//
//
//	void calc_buf_pos() // after change to m_cursorY and/or m_cursorX, or end-line causing page screen
//	{
//		m_bufPos = (((m_screenTop + m_cursorY) * m_bufferWidthInChars) + m_cursorX) % m_bufSize;
//		invalidate_cursor(); // if we needed to recompute the cursor, then it needs to be draw
//	}
//
//	void inc_buf_pos()
//	{
//		if (++m_bufPos == m_bufSize)
//			m_bufPos = 0;
//		invalidate_cursor();
//	}
//
//	void back_off_verge()
//	{
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		if (m_cursorX >= screenWidthInChars)
//		{
//			size_t d = (m_cursorX - screenWidthInChars) + 1;
//			m_cursorX -= d;
//			m_bufPos -= d;
//			if (m_bufPos >= m_bufSize)
//				m_bufPos += m_bufSize;
//			else
//				m_bufPos -= d;
//		}
//	}
//
//	void step_off_verge()
//	{
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//		if (m_cursorX >= screenWidthInChars)
//		{
//			invalidate_cursor();
//			if (m_wrapMode)
//			{
//				if (m_cursorY == screenHeightInChars-1)
//					advance_screen(1);
//				else
//					m_cursorY++;
//				m_cursorX = 0;
//				calc_buf_pos();
//			}
//			else // terminal set to a mode where it refuses to advance lines automatically
//			{ // So, keep overwriting last character
//				m_cursorX--;
//				m_bufPos--;
//			}
//		}
//	}
//
//	void raw_insert(char c)
//	{
//		step_off_verge();
//		if (m_curBrush.invisible) // ignore text, just insert spaces.  Overrides all other brush attributes
//		{
//			if (m_insertMode)
//			{
//				// move chars and brushes to the right
//			}
//			m_charBuffer[m_bufPos] = ' ';
//		}
//		else
//		{
//			if (m_insertMode)
//			{
//				// move chars and brushes to the right
//			}
//			m_charBuffer[m_bufPos] = c;
//		}
//		m_brushBuffer[m_bufPos].value = m_curBrush.value;
//		invalidate_cursor();
//		m_cursorX++;
//		inc_buf_pos();
//	}
//
//
//	void parse(char c)
//	{
//		switch (c)
//		{
//		case CSI:
//			if (m_parseANSISeq)
//				start_sequence();
//			else
//				raw_insert(c);
//			break;
//		case 0: // Ignore NULL
//			break;
//		case DCS: // Device Control String
//			// ignored
//			break;
//		case ST: // String Terminator
//			// ignored
//			break;
//		case ESC:
//			if (m_parseANSISeq)
//				m_hadESC = true;
//			else
//				raw_insert(c);
//			break;
//		case HOME:
//			{
//				invalidate_cursor();
//				m_cursorX = 0;
//				m_cursorY = 0;
//				uint16_t screenHeightInChars = m_screenHeightInChars;
//				advance_screen(screenHeightInChars);
//				calc_buf_pos();
//			}
//			break;
//		case TAB:
//			{
//				uint16_t screenWidthInChars = m_screenWidthInChars;
//				invalidate_cursor();
//				uint8_t tabSpaces = m_tabSpaces - (m_cursorX % m_tabSpaces);
//				if (m_cursorX > screenWidthInChars - m_tabSpaces)
//					m_cursorX = screenWidthInChars;
//				else
//					m_cursorX += tabSpaces;
//				calc_buf_pos();
//			}
//			break;
//		case BELL:
//			os::beep();
//			break;
//		case BS:
//		case DEL:
//			invalidate_cursor();
//			if (m_cursorX != 0)
//			{
//				m_cursorX--;
//				calc_buf_pos();
//			}
//			break;
//		case LF: // Line Feed - Same as ESC D
//			if (m_addCRonLF)
//				m_cursorX = 0;
//		case IND: // Index - same as ESC D
//			// fall through
//		case VT: // Vertical tab (treated same as NExt Line)
//		case NEL: // NExt Line - Same as ESC E
//			{
//				invalidate_cursor();
//
//				// TBD - arbitrary scroll regions!
//
//				uint16_t screenHeightInChars = m_screenHeightInChars;
//				if (m_cursorY == screenHeightInChars - 1)
//					advance_screen(1);
//				else
//					m_cursorY++;
//				calc_buf_pos();
//			}
//			break;
//		case RI: // Reverse Index - same as Esc M
//			invalidate_cursor();
//
//			// TBD - arbitrary scroll regions!
//
//			if (m_cursorY!= 0)
//				m_cursorY--;
//			calc_buf_pos();
//			break;
//		case CR:
//			invalidate_cursor();
//			m_cursorX = 0;
//			calc_buf_pos();
//			break;
////		case MUSICNOTE: // Marks end of ANSI music sequence?
////			break;
//		default:
//			raw_insert(c);
//			break;
//		}
//	}
//
//
//	void parse_esc_code(char c)
//	{
//		switch (c) // ANSI escape sequences
//		{
//		case 0: // ignore NULL
//			break;
//		case ESC: // Previous escape ignored
//			// leave m_hadESC intact
//			break;
//		case 'P': // Device Control String start NOP
//			m_hadESC = false;
//			parse(DCS);
//			break;
//		case '\\': // String Terminated / ignored
//			m_hadESC = false;
//			parse(ST);
//			break;
//		case 'E': // NEL - Next Line
//			m_hadESC = false;
//			parse(NEL);
//			break;
//		case 'M': // Move up
//			m_hadESC = false;
//			parse(RI);
//			break;
//		case 'H':
//			{
//				m_hadESC = false;
//				if (!m_vt52Compat)
//				{
//					// HTS - Horizontal Tab Set
//					break;
//				}
//				// VT52 set cursor position
//				start_sequence();
//				parse_mid_sequence(c);
//			}
//			break;
//		case 'D':
//			if (!m_vt52Compat)
//			{
//				// IND - Index - Move down (scroll if necessary)
//				m_hadESC = false;
//				parse(IND);
//				break;
//			}
//			// else fallthrough, as these VT52 ESC_ is identical to ESC[_
//		case 'A': // Some VT52 support
//		case 'B':
//		case 'C':
//		case 'J':
//		case 'K':
//			m_hadESC = false;
//			// Same as ESC[ versions
//			start_sequence();
//			parse_mid_sequence(c);
//			break;
//		case CSI:
//		case '[':
//			m_hadESC = false;
//			start_sequence();
//			break;
//		case '7': // DECSC - Save Cursor
//			m_hadESC = false;
//			m_savedBrush78 = m_curBrush;
//			m_savedPos78 = m_cursor;
//			break;
//		case '8': // DECRC - Restore Cursor
//			{
//				m_hadESC = false;
//				invalidate_cursor();
//				m_curBrush = m_savedBrush78;
//				m_cursor = m_savedPos78;
//				uint16_t screenHeightInChars = m_screenHeightInChars;
//				if (m_cursorY >= screenHeightInChars)
//				{
//					m_cursor.set_y(screenHeightInChars - 1);
//					COGS_ASSERT(m_cursorY < 999);
//				}
//				calc_buf_pos();
//			}
//			break;
//		case '<': // Exit vt52 mode, enter VT100 mode
//			m_hadESC = false;
//			m_vt52Compat = false;
//			break;
//		case 'I': // Reverse Line Feed - vt52
//		case '6': // DECBI — Back Index
//			m_hadESC = false;
//			break;
//
//		case 'c': // RIS - Reset to Initial State
//			m_hadESC = false;
//			break;
//
//		case 'g': // visual bell // ignored
//		case 'n': // Lock shift G2 // ignored
//		case 'o': // Lock shift G3 // ignored
//		case '!': // Global message string // ignored
//		case ']': // Operating system command // ignored
//		case '1': // ignored
//		case '2': // ignored
//			m_hadESC = false;
//			break;
//		//case 'q': // DECLL - Load LEDs
//		//case '9': // DECFI - Forward Index
//		//case 'F': // Enter graphics mode - VT52
//		//case 'G': // Exit graphics mode - VT52
//		//case ']': // Print screen - vt52
//		//case 'V': // Print current line - vt52
//		//case 'W': // Enter printer controller mode - vt52
//		//case 'X': // Exit printer controller mode - vt52
//		//case '^': // Enter autoprint mode ? - vt52
//		//case '_': // Exit autoprint mode ? - vt52
//		//case '=': // DECKPAM - Keypad Application Mode - Alt keypad mode
//		//case '>': // DECKPNM — Keypad Numeric Mode - Exit alt keypad mode
//		//case 'Z': // identify (host to terminal)
//		//case '/Z': // identify (terminal to host)
//		//	break
//		// ESC Y# ?? - Move cursor to column # - VT52
//		// ESC Q # <string> ?? - SCODFK - Define Function Key
//		// ESC #3 - DECDHL - Double-Width, Double-Height Line - top half
//		// ESC #4 - DECDHL - Double-Width, Double-Height Line - bottom half
//		// ESC #5 - DECSWL - Single-Width, Single-Height Line
//		// ESC #6 - DECDWL - Double-Width, Single-Height Line
//		// ESC #8 - DECALN - Screen Alignment Pattern
//		// ESC SP F - S7C1T - Send C1 Control Character to the Host
//		// ESC SP G - S8C1T - Send C1 Control Character to the Host
//		default:
//			m_hadESC = false; // Ignores potential control character
//			break;
//		}
//	}
//
//	void parse_mid_sequence(char c)
//	{
//		if ((c >= '0') && (c <= '9'))
//		{
//			if (!m_seqVarValid)
//			{
//				m_seqVarValid = true;
//				m_seqVars.append((uint16_t)c - 48, 1);
//			}
//			else
//			{
//				uint16_t* v = &(m_seqVars[m_seqVars.get_length()-1]);
//				*v *= 10;
//				*v += ((uint8_t)c - 48);
//			}
//		}
//		else
//		{
//			// in-sequence and not a number
//			switch (c)
//			{
//			case 0: // ignore NULL
//				break;
//			case '?': // Mode control ?
//				m_gotQmark = true;
//				break;
//			case ';': // break between seq vars
//				if (!m_seqVarValid)
//					m_seqVars.append((uint16_t)0, 1);
//				else
//					m_seqVarValid = false;
//				break;
//			case ESC: // Start over
//				m_isMidSeq = false;
//				m_hadESC = true;
//				break;
//			case CSI: // Start over
//				start_sequence();
//				break;
//			case 'A': // CUU - Cursor Up - move up
//				{
//					m_isMidSeq = false;
//					// old code would keep the cursor on a line until it wrapped due to text entered.
//					invalidate_cursor();
//					step_off_verge();
//					uint16_t n = 1;
//					if (!m_seqVars.is_empty())
//						n = m_seqVars[0];
//					if (n < 1)
//						n = 1;
//					if (m_cursorY <= n)
//						m_cursorY = 0;
//					else
//					{
//						m_cursorY -= n;
//						COGS_ASSERT(m_cursorY < 999);
//					}
//					calc_buf_pos();
//				}
//				break;
//			case 'B': // CUD - Cursor Down - move down
//				{
//					m_isMidSeq = false;
//					// old code would keep the cursor on a line until it wrapped due to text entered.
//					invalidate_cursor();
//					step_off_verge();
//					uint16_t n = 1;
//					if (!m_seqVars.is_empty())
//						n = m_seqVars[0];
//					if (n < 1)
//						n = 1;
//					m_cursorY += n;
//					COGS_ASSERT(m_cursorY < 999);
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					if (m_cursorY >= screenHeightInChars)
//					{
//						m_cursorY = screenHeightInChars - 1; // Old code did this.  Maybe more accurate to retain 'verge' effect?  Could be repro'ing bad behavior of other terminals.
//						COGS_ASSERT(m_cursorY < 999);
//					}
//					calc_buf_pos();
//				}
//				break;
//			case 'C': // CUF - Cursor Forward - Move right
//				{
//					m_isMidSeq = false;
//					step_off_verge();
//					// old code would keep the cursor on a line until it wrapped due to text entered.
//					invalidate_cursor();
//					uint16_t n = 1;
//					if (!m_seqVars.is_empty())
//						n = m_seqVars[0];
//					if (n < 1)
//						n = 1;
//					m_cursorX += n;
//					uint16_t screenWidthInChars = m_screenWidthInChars;
//					if (m_cursorX >= screenWidthInChars)
//						m_cursorX = screenWidthInChars - 1; // Old code did this.  Maybe more accurate to retain 'verge' effect?  Could be repro'ing bad behavior of other terminals.
//					calc_buf_pos();
//				}
//				break;
//			case 'D': // CUB - Cursor Backward - move left
//				{
//					m_isMidSeq = false;
//					// old code would keep the cursor on a line until it wrapped due to text entered.
//					invalidate_cursor();
//					uint16_t n = 1;
//					if (!m_seqVars.is_empty())
//						n = m_seqVars[0];
//					if (n < 1)
//						n = 1;
//					if (m_cursorX <= n)
//						m_cursorX = 0;
//					else
//						m_cursorX -= n;
//					calc_buf_pos();
//				}
//				break;
//			case 'f': // HVP - Horizontal and Vertical Position
//			case 'H': // CUP - Cursor Position
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					linear::point<uint16_t> newPos(0,0);
//					if (m_seqVars.get_length() > 0)
//					{
//						newPosY = m_seqVars[0];
//						uint16_t screenHeightInChars = m_screenHeightInChars;
//						if (newPosY > screenHeightInChars)
//							newPosY = screenHeightInChars;
//						if (newPosY > 0)
//							newPosY--;
//					}
//					if (m_seqVars.get_length() > 1)
//					{
//						newPosX = m_seqVars[1];
//						if (newPosX > 0)
//							newPosX--;
//					}
//					m_cursor = newPos;
//					COGS_ASSERT(m_cursorY < 999);
//					calc_buf_pos();
//				}
//				break;
//			case 'd': // VPA - Vertical Line Position Absolute - Go to line #
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					uint16_t n = 0;
//					if (!m_seqVars.is_empty())
//						n = m_seqVars[0];
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					if (n > screenHeightInChars)
//						n = screenHeightInChars;
//					if (n > 0)
//						n--;
//					m_cursorY = n;
//					COGS_ASSERT(m_cursorY < 999);
//					calc_buf_pos();
//				}
//				break;
//			//case 's': // ESC[#;#s - DECSLRM - Set Left and Right Margins ????
//			case 's': // SCOSC - Save Current Cursor Position
//				m_isMidSeq = false;
//				m_savedBrush = m_curBrush;
//				m_savedPos = m_cursor;
//				break;
//			case 'u':  // SCORC - Restore Saved Cursor Position
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					m_cursor = m_savedPos;
//					m_curBrush = m_savedBrush;
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					if (m_cursorY >= screenHeightInChars)
//					{
//						m_cursor.set_y(screenHeightInChars - 1);
//						COGS_ASSERT(m_cursorY < 999);
//					}
//					calc_buf_pos();
//				}
//				break;
//			case 'm': // SGR - Select Graphic Rendition
//				m_isMidSeq = false;
//				do {
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					m_seqVars.erase(0, 1);
//					switch (selector)
//					{
//					case 0:
//						m_curBrush.value = 0;
//						break;
//					case 1:
//						m_curBrush.bold = 1;
//						break;
//					case 2:
//						// faint
//						break;
//					case 3:
//						// italic
//						break;
//					case 4:
//						m_curBrush.underlined = 1;
//						break;
//					case 5:
//						m_curBrush.blinking = 1;
//						break;
//					case 6:
//						m_curBrush.blinking = 3;
//						break;
//					case 7:
//						m_curBrush.inverse = 1;
//						break;
//					case 8:
//						m_curBrush.invisible = 1;
//						break;
//					case 10: // Default ASCII mapping of 7-bit set
//						break;
//					case 11: // 00-7F PC character set to 7-bit set
//						break;
//					case 12: // 80-FF of current character set to 7-bit set
//						break;
//					case 22:
//						m_curBrush.bold = 0;
//						break;
//					case 24:
//						m_curBrush.underlined = 0;
//						break;
//					case 25:
//						m_curBrush.blinking = 0;
//						break;
//					case 27:
//						m_curBrush.inverse = 0;
//						break;
//					case 28:
//						m_curBrush.invisible = 0;
//						break;
//					case 30:
//						m_curBrush.foreFlag = ForeBlack;
//						break;
//					case 31:
//						m_curBrush.foreFlag = Red;
//						break;
//					case 32:
//						m_curBrush.foreFlag = Green;
//						break;
//					case 33:
//						m_curBrush.foreFlag = Yellow;
//						break;
//					case 34:
//						m_curBrush.foreFlag = Blue;
//						break;
//					case 35:
//						m_curBrush.foreFlag = Magenta;
//						break;
//					case 36:
//						m_curBrush.foreFlag = Cyan;
//						break;
//					case 37:
//					case 39: // default
//						m_curBrush.foreFlag = ForeWhite;
//						break;
//					case 40:
//					case 49: // default or superscript ??
//						m_curBrush.backFlag = BackBlack;
//						break;
//					case 41:
//						m_curBrush.backFlag = Red;
//						break;
//					case 42:
//						m_curBrush.backFlag = Green;
//						break;
//					case 43:
//						m_curBrush.backFlag = Yellow;
//						break;
//					case 44:
//						m_curBrush.backFlag = Blue;
//						break;
//					case 45:
//						m_curBrush.backFlag = Magenta;
//						break;
//					case 46:
//						m_curBrush.backFlag = Cyan;
//						break;
//					case 47:
//						m_curBrush.backFlag = BackWhite;
//						break;
//					case 48: // subscript
//						break;
//					default:
//						break;
//					}
//				} while (!m_seqVars.is_empty());
//
//				break;
//			case 'J':
//				{
//					m_isMidSeq = false;
//					if (m_gotQmark)
//					{
//						// DECSED - Selective Erase in Display
//					}
//					else
//					{
//						// ED — Erase in Display
//						invalidate_cursor();
//						uint16_t selector = 0;
//						if (!m_seqVars.is_empty())
//							selector = m_seqVars[0];
//						switch (selector)
//						{
//						case 0: // Clear to bottom of screen
//							{
//								clean_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//								invalidate_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//								uint16_t nextLine = m_cursorY + 1;
//								uint16_t screenHeightInChars = m_screenHeightInChars;
//								uint16_t linesLeft = screenHeightInChars - nextLine;
//								if (linesLeft)
//								{
//									clean_lines(nextLine, linesLeft);
//									invalidate_lines(nextLine, linesLeft);
//								}
//								break;
//							}
//						case 1: // Clear to top of screen
//							{
//								clean_line(m_cursorY, 0, m_cursorX);
//								invalidate_line(m_cursorY, 0, m_cursorX);
//								if (m_cursorY > 0)
//								{
//									clean_lines(0, m_cursorY);
//									invalidate_lines(0, m_cursorY);
//								}
//								break;
//							}
//						case 2: // Clear entire screen
//							{
//								uint16_t screenHeightInChars = m_screenHeightInChars;
//								advance_screen(screenHeightInChars);
//
//								clean_lines(0, screenHeightInChars - 1);
//								invalidate_lines(0, screenHeightInChars - 1);
//
//								// ANSI resets cursor pos, VT100 does not?
//								if (!m_vt100Compat)
//									m_cursor.set(0,0);
//								break;
//							}
//						}
//					}
//				}
//				break;
//			case 'K':
//				{
//					m_isMidSeq = false;
//					if (m_gotQmark)
//					{
//						// DECSEL - Selective Erase in Line
//					}
//					else
//					{
//						// EL - Erase in Line
//						invalidate_cursor();
//						uint16_t selector = 0;
//						if (!m_seqVars.is_empty())
//							selector = m_seqVars[0];
//						switch (selector)
//						{
//						case 0: // Clear to end of current line
//							clean_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//							invalidate_line(m_cursorY, m_cursorX, m_bufferWidthInChars);
//							break;
//						case 1: // Clear backward to begining of current line
//							clean_line(m_cursorY, 0, m_cursorX);
//							invalidate_line(m_cursorY, 0, m_cursorX);
//							break;
//						case 2: // Clear entire line
//							clean_line(m_cursorY, 0, m_bufferWidthInChars);
//							invalidate_line(m_cursorY, 0, m_bufferWidthInChars);
//							break;
//						}
//					}
//				}
//				break;
//			case '@': // ICH - Insert Character - Cursor does not move.  Pushes line forward as if in an insert
//				{
//					m_isMidSeq = false;
//					uint16_t screenWidthInChars = m_screenWidthInChars;
//					if (m_cursorX <= screenWidthInChars)
//					{
//						uint16_t selector = 0;
//						if (!m_seqVars.is_empty())
//							selector = m_seqVars[0];
//						if (selector < 1)
//							selector = 1;
//						if (selector + m_cursorX > screenWidthInChars)
//							selector = screenWidthInChars - m_cursorX;
//						uint16_t n = screenWidthInChars - (selector + m_cursorX);
//
//						memcpy(m_charBuffer + m_bufPos + selector, m_charBuffer + m_bufPos, n);
//						memcpy(m_brushBuffer + m_bufPos + selector, m_brushBuffer + m_bufPos, n * sizeof(brush));
//
//						invalidate_line(m_cursorY, m_cursorX, screenWidthInChars);
//					}
//				}
//				break;
//			case 'P': // DCH - Delete Character - Cursor does not move.  Shifts line to left, deleting at cursor.
//				{
//					m_isMidSeq = false;
//					uint16_t screenWidthInChars = m_screenWidthInChars;
//					if (m_cursorX <= screenWidthInChars)
//					{
//						uint16_t selector = 0;
//						if (!m_seqVars.is_empty())
//							selector = m_seqVars[0];
//						if (selector < 1)
//							selector = 1;
//						if (selector + m_cursorX > screenWidthInChars)
//							selector = screenWidthInChars - m_cursorX;
//						uint16_t n = screenWidthInChars - (selector + m_cursorX);
//
//						memcpy(m_charBuffer + m_bufPos, m_charBuffer + m_bufPos + selector, n);
//						memcpy(m_brushBuffer + m_bufPos, m_brushBuffer + m_bufPos + selector, n * sizeof(brush));
//
//						invalidate_line(m_cursorY, m_cursorX, screenWidthInChars);
//					}
//				}
//				break;
//			case 'L': // IL - Insert Line (or # lines)
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (selector < 1)
//						selector = 1;
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					if (selector + m_cursorY > screenHeightInChars)
//						selector = screenHeightInChars - m_cursorY;
//					m_cursorX = 0;
//					calc_buf_pos();
//					uint16_t scrollHeight = screenHeightInChars - m_cursorY;
//					uint16_t linesToMove = scrollHeight - selector;
//					if (linesToMove)
//					{
//						memcpy(m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), m_charBuffer + m_bufPos, linesToMove * m_bufferWidthInChars);
//						memcpy(m_brushBuffer + m_bufPos + (selector * m_bufferWidthInChars), m_brushBuffer + m_bufPos, linesToMove * m_bufferWidthInChars * sizeof(brush));
//					}
//					memset(m_charBuffer + m_bufPos, 0, selector * m_bufferWidthInChars);
//					for (size_t i = 0; i < selector * m_bufferWidthInChars; i++)
//						m_brushBuffer[m_bufPos + i].value = m_curBrush.value;
//					invalidate_lines(m_cursorY, scrollHeight);
//				}
//				break;
//			case 'M': // DL — Delete Line (or # lines)
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (selector < 1)
//						selector = 1;
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					if (selector + m_cursorY > screenHeightInChars)
//						selector = screenHeightInChars - m_cursorY;
//					m_cursorX = 0;
//					calc_buf_pos();
//					uint16_t scrollHeight = screenHeightInChars - m_cursorY;
//					uint16_t linesToMove = scrollHeight - selector;
//					if (linesToMove)
//					{
//						memcpy(m_charBuffer + m_bufPos, m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), linesToMove * m_bufferWidthInChars);
//						memcpy(m_brushBuffer + m_bufPos, m_brushBuffer + m_bufPos + (selector * m_bufferWidthInChars), linesToMove * m_bufferWidthInChars * sizeof(brush));
//					}
//					memset(m_charBuffer + m_bufPos + (selector * m_bufferWidthInChars), 0, selector * m_bufferWidthInChars);
//					for (size_t i = 0; i < selector * m_bufferWidthInChars; i++)
//						m_brushBuffer[m_bufPos + i].value = m_curBrush.value;
//					invalidate_lines(m_cursorY, scrollHeight);
//				}
//				break;
//			case 'R': // Position report?  Are we a server?
//				m_isMidSeq = false;
//
//				// TBD - This isn't useful data, as the position may have changed by now.
//				//			But, this does indicate that the report party did parse our request.
//				//			Commonly used to detect if the report party has an ANSI terminal.
//
//				break;
//			case 'n': // DSR - Device Status Reports
//				{
//					m_isMidSeq = false;
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (m_gotQmark)
//					{
//						//?15n Q: What's the printer status
//						//		?13n A: There is no printer
//						//?26n Q: What's the keyboard type?
//						//		?27;1n A: North American
//						switch (selector)
//						{
//						case 6: //	DECXCPR - Extended Cursor Position
//							{ //?6n Q; Send extended cursor position report.  Respond with ESC [ #;#;# R
//								cstring s;
//								s += ESC;
//								s += '[';
//								s += cstring::from_int<uint16_t>(m_cursorY);
//								s += ';';
//								s += cstring::from_int<uint16_t>(m_cursorX);
//								s += cstring::literal(";0R"); // last is 'page' ?  zero
//								write(s);
//							}
//							break;
//						case 15: // DSR - Printer Port (request)
//							{
//								uint8_t msg[6] = { ESC, '[', '?', '1', '3', 'n' };
//								write(buffer(msg, 6));
//							}
//							break;
//						case 26: // DSR - Keyboard - (request)
//							{
//								uint8_t msg[8] = { ESC, '[', '?', '2', '7', ';', '1', 'n' };
//								write(buffer(msg, 8));
//							}
//							break;
//						//case 10: // DSR - Printer Port - (response) - printer ready
//						//case 11: // DSR - Printer Port - (response) - printer not ready
//						//case 13: // DSR - Printer Port - (response) - no printer
//						//case 18: // DSR - Printer Port - (response) - printer busy
//						//case 19: // DSR - Printer Port - (response) - printer assigned to another session
//						//case 20: // DSR - User-Defined Keys - (response) - unlocked
//						//case 21: // DSR - User-Defined Keys - (response) - locked
//						//case 25: // DSR - User-Defined Keys - (request)
//						//case 27: // DSR - Keyboard - (response)
//						//case 62: // DSR - Macro Space Report (request)
//						//case 63: // DSR - Memory Checksum (DECCKSR)
//						//case 70: // DSR — Data Integrity Report - (response) Ready
//						//case 71: // DSR — Data Integrity Report - (response) Malfunction
//						//case 73: // DSR — Data Integrity Report - (response) No report
//						//case 75: // DSR — Data Integrity Report - (request) integrity flag status
//						default:
//							break;
//						}
//					}
//					else
//					{
//						switch (selector)
//						{
//						case 6: // CPR - Cursor Position Report
//							{ //6n Q; Send cursor position report.  Respond with ESC [ #;#R
//								cstring s;
//								s += ESC;
//								s += '[';
//								s += cstring::from_int<uint16_t>(m_cursorY);
//								s += ';';
//								s += cstring::from_int<uint16_t>(m_cursorX);
//								s += 'R';
//								write(s);
//							}
//							break;
//						case 5: // DSR - Operating Status - (request)
//							{ //5n Q: What is your status?
//								uint8_t msg[4] = { ESC, '[', '0', 'n' };
//								write(buffer(msg, 4));
//							}
//							break;
//						//case 0: // DSR - Operating Status - (response) good
//						//case 3: // DSR - Operating Status - (response) malfunction
//						default:
//							break;
//						}
//					}
//				}
//				break;
//			case 'G': // CHA - Cursor Horizontal Absolute - Move cursor to column n
//				{
//					m_isMidSeq = false;
//					invalidate_cursor();
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (selector > 0)
//						selector--;
//					m_cursorX = selector;
//				}
//				break;
//			case 'h':
//				{
//					m_isMidSeq = false;
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (m_gotQmark)
//					{
//						switch (selector)
//						{
//						case 25: // DECTCEM - Text Cursor Enable Mode - show cursor
//							// TBD
//							break;
//						//case 1: // DECCKM - Cursor Keys Mode - set
//						//case 3: // 132 column mode
//						//case 4: // DECSCLM - Scrolling Mode - smooth scroll
//						//case 5: // DECSCNM - Screen Mode: Light or Dark Screen - set/reverse video
//						//case 6: // DECOM - Origin Mode - set/within margins
//						//case 7: // DECAWM — Autowrap Mode - set/enable
//						//case 8: // DECARM - Autorepeat Mode - enable auto-repeat
//						//case 18: // DECPFF - Print Form Feed Mode - use FF
//						//case 19: // DECPEX - Printer Extent Mode - prints whole page
//						//case 20: // NL mode
//						//case 34: // DECRLM - Cursor Right to Left Mode - set
//						//case 42: // DECNRCM - National Replacement Character Set Mode - set/7-bit characters
//						//case 47: //?47 ignored (XTERM) switch to alternate screen
//						//case 58: // DECIPEM - Enter/Return from IBM ProPrinter Emulation Mode - set/enter
//						//case 60: // DECHCCM - Horizontal Cursor-Coupling Mode - set/couples cursor to display
//						//case 61: // DECVCCM - Vertical Cursor-Coupling Mode - set/couples cursor to display
//						//case 64: // DECPCCM - Page Cursor-Coupling Mode - set/couples cursor to display
//						//case 66: // DECNKM - Numeric Keypad Mode - Set/application sequences
//						//case 67: // DECBKM - Backarrow Key Mode - backspace mode
//						//case 68: // DECKBUM - Typewriter or Data Processing Keys - set/data keys
//						//case 69: // DECLRMM - Left Right Margin Mode - Set: DECSLRM can set margins.
//						//case 73: // DECXRLM - Transmit Rate Limiting - set/limit transmit rate
//						//case 81: // DECKPM - Key Position Mode - set/send key position reports
//						//case 96: // DECRLCM - Right-to-Left Copy - Enable/right-to-left copy
//						//case 97: // DECCRTSM - Set/Reset CRT Save Mode - set/enable
//						//case 98: // DECARSM — Set/Reset Auto Resize Mode - set/enable
//						//case 99: // DECMCM - Set/Reset Modem Control Mode - enable modem control
//						//case 100: // DECAAM - Set/Reset Auto Answerback Mode - Set/Enable auto answerback
//						//case 101: // DECCANSM - Conceal Answerback Message Mode
//						//case 102: // DECNULM - Set/Reset Ignoring Null Mode - Set/ignore NULL - default
//						//case 103: // DECHDPXM - Set/Reset Half-Duplex Mode - half-duplex mode
//						//case 104: // DECESKM - Enable Secondary Keyboard Language Mode - secondary mode
//						//case 106: // DECOSCNM - Set/Reset Overscan Mode - enable overscan
//						//case 108: // DECNUMLK - Num Lock Mode - set
//						//case 109: // DECCAPSLK - Caps Lock Mode - set
//						//case 110: // DECKLHIM - Keyboard LED's Host Indicator Mode - set
//						default:
//							break;
//						}
//					}
//					else
//					{
//						switch (selector)
//						{
//						case 12: // SRM - Local Echo: Send/Receive Mode - echo off
//							m_localEcho = false;
//							break;
//						//case 2: // KAM - Keyboard Action Mode - set/locks the keyboard
//						//case 3: // CRM - Show Control Character Mode - set/show control chars
//						//case 4: // IRM - Insert/Replace Mode - set/insert mode
//						//case 20: // LNM - Line Feed/New Line Mode - set new line mode
//						default:
//							break;
//						}
//					}
//				}
//				break;
//			case 'l':
//				{
//					m_isMidSeq = false;
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (m_gotQmark)
//					{
//						switch (selector)
//						{
//						case 2: // DECANM - ANSI Mode (actually VT52 mode) enable
//							m_vt52Compat = true;
//							break;
//						case 25: // DECTCEM - Text Cursor Enable Mode - hide cursor
//							// TBD
//							break;
//						//case 1: // DECCKM - Cursor Keys Mode - reset
//						//case 3: // 80 column mode
//						//case 4: // DECSCLM - Scrolling Mode - jump scroll
//						//case 5: // DECSCNM - Screen Mode: Light or Dark Screen - reset/normal video
//						//case 6: // DECOM - Origin Mode - reset/upper-left corner
//						//case 7: // DECAWM  Autowrap Mode - reset/disable
//						//case 8: // DECARM - Autorepeat Mode - disable auto-repeat
//						//case 18: // DECPFF - Print Form Feed Mode - no FF
//						//case 19: // DECPEX - Printer Extent Mode - prints scrolling region only
//						//case 20: // LF mode
//						//case 34: // DECRLM - Cursor Right to Left Mode - reset
//						//case 42: // DECNRCM - National Replacement Character Set Mode - reset/8-bit characters
//						//case 47: //?47 ignored (XTERM) switch to alternate screen
//						//case 58: // DECIPEM - Enter/Return from IBM ProPrinter Emulation Mode - reset/return
//						//case 60: // DECHCCM - Horizontal Cursor-Coupling Mode - reset/uncouples cursor
//						//case 61: // DECVCCM - Vertical Cursor-Coupling Mode - reset/uncouples cursor
//						//case 64: // DECPCCM - Page Cursor-Coupling Mode - reset/uncouples cursor
//						//case 66: // DECNKM - Numeric Keypad Mode - Reset/keypad characters
//						//case 67: // DECBKM - Backarrow Key Mode - delete mode
//						//case 68: // DECKBUM - Typewriter or Data Processing Keys - reset/typewritter
//						//case 69: // DECLRMM - Left Right Margin Mode - Reset: DECSLRM cannot set margins.
//						//case 73: // DECXRLM - Transmit Rate Limiting - reset/unlimited transmit rate
//						//case 81: // DECKPM - Key Position Mode - reset/send character codes
//						//case 96: // DECRLCM - Right-to-Left Copy - Disable/left-to-right copy
//						//case 97: // DECCRTSM - Set/Reset CRT Save Mode - disable
//						//case 98: // DECARSM — Set/Reset Auto Resize Mode - reset/disable
//						//case 99: // DECMCM - Set/Reset Modem Control Mode - disable modem control - default
//						//case 100: // DECAAM - Set/Reset Auto Answerback Mode - Reset/Disable auto answerback
//						//case 101: // DECCANSM - Conceal Answerback Message Mode
//						//case 102: // DECNULM - Set/Reset Ignoring Null Mode - Reset/accept NULL
//						//case 103: // DECHDPXM - Set/Reset Half-Duplex Mode - full-duplex mode
//						//case 104: // DECESKM - Enable Secondary Keyboard Language Mode - primary mode
//						//case 106: // DECOSCNM - Set/Reset Overscan Mode - disable overscan - default
//						//case 108: // DECNUMLK - Num Lock Mode - Reset
//						//case 109: // DECCAPSLK - Caps Lock Mode - reset
//						//case 110: // DECKLHIM - Keyboard LED's Host Indicator Mode - reset
//						default:
//							break;
//						}
//					}
//					else
//					{
//						switch (selector)
//						{
//						case 12: // SRM - Local Echo: Send/Receive Mode - echo on
//							m_localEcho = true;
//							break;
//						//case 2: // KAM - Keyboard Action Mode - reset/unlocks the keyboard
//						//case 3: // CRM - Show Control Character Mode - reset/interpret control chars
//						//case 4: // IRM - Insert/Replace Mode - replace mode
//						//case 20: // LNM - Line Feed/New Line Mode - reset/line feed mode
//						default:
//							break;
//						}
//					}
//				}
//				break;
//			case 't': //  DECSLPP - Set num lines per page.
//				m_isMidSeq = false; // Ignored, we don't support multi-page mode
//				break;
//			case 'E': // CNL - Cursor Next Line - Move cursor to beginning of line n lines down
//				m_isMidSeq = false;
//				break;
//			case 'F': // CPL - Cursor Previous Line - Move cursor to beginning of line n lines up
//				m_isMidSeq = false;
//				break;
//			case 'S': // SU - Pan Down - Scroll up n lines
//				m_isMidSeq = false;
//				break;
//			case 'T': // SD - Pan Up - Scroll down n lines
//				m_isMidSeq = false;
//				break;
//			case 'r': // Set the scroll region
//				m_isMidSeq = false;
//				if (m_gotQmark)
//				{
//					// DECPCTERM - Enter/Exit PCTerm or Scancode Mode
//				}
//				else
//				{
//					// DECSTBM - Set Top and Bottom Margins
//					uint16_t screenHeightInChars = m_screenHeightInChars;
//					uint16_t top = 0;
//					if (!m_seqVars.is_empty())
//						top = m_seqVars[0];
//					if (top > screenHeightInChars)
//						top = screenHeightInChars;
//					if (top > 0)
//						top--;
//					uint16_t bottom = screenHeightInChars;
//					if (m_seqVars.get_length() > 1)
//						bottom = m_seqVars[1];
//					if (top > screenHeightInChars)
//						top = screenHeightInChars;
//					if (bottom > 0)
//						bottom--;
//					m_topScrollLine = top;
//					m_bottomScrollLine = bottom;
//					m_cursor.set_x(0);
//					m_cursor.set_y(top);
//				}
//				break;
//			case 'X': // ECH - Erase Character - erase # chars
//				{
//					m_isMidSeq = false;
//					uint16_t screenWidthInChars = m_screenWidthInChars;
//					uint16_t pos = 1;
//					if (!m_seqVars.is_empty())
//						pos = m_seqVars[0];
//					uint16_t charsLeft = (screenWidthInChars - m_cursorX) + 1;
//					if (pos > charsLeft)
//						pos = charsLeft;
//					if (pos > 0)
//						pos--; // 0 = clear 1 char, 1 = clear 2 chars
//					clean_line(m_cursorY, m_cursorX, m_cursorX + pos);
//					invalidate_line(m_cursorY, m_cursorX, m_cursorX + pos);
//				}
//				break;
//			case 'c': // DA1 - Primary Device Attributes - Send identification string
//				{
//					m_isMidSeq = false;
//					uint16_t selector = 0;
//					if (!m_seqVars.is_empty())
//						selector = m_seqVars[0];
//					if (m_gotQmark)
//					{
//						// DA1 response
//						//switch (selector)
//						//{
//						//default:
//						//	break;
//						//}
//					}
//					else
//					{
//						switch (selector)
//						{
//						case 0: // Host Request - response: ESC[?<classcode>;<extentions>c
//							{
//								// respond with no options
//								uint8_t msg[7] = { ESC, '[', '?', '1', ';', '0', 'c' };
//								write(buffer(msg, 7));
//							}
//							break;
//							// Class code:
//							//	 1 = VT100
//							// Extensions:
//							//	0 No options ESC
//							//	1 Processor option (STP) ESC
//							//	2 Advanced video option (AVO) ESC
//							//	3 AVO and STP ESC
//							//	4 Graphics option (GPO) ESC
//							//	5 GPO and STP ESC
//							//	6 GPO and AVO ESC
//							//	7 GPO, STP and AVO ESC
//							//
//							// Class code:
//							//	64 = VT510 class code
//							// Extensions:
//							//	1 132 columns
//							//	2 Printer port
//							//	4 Sixel
//							//	6 Selective erase
//							//	7 Soft character set (DRCS)
//							//	8 User-defined keys (UDKs)
//							//	9 National replacement character sets (NRCS) (International terminal only)
//							//	12 Yugoslavian (SCS)
//							//	15 Technical character set
//							//	18 Windowing capability
//							//	21 Horizontal scrolling
//							//	23 Greek
//							//	24 Turkish
//							//	42 ISO Latin-2 character set
//							//	44 PCTerm
//							//	45 Soft key map
//							//	46 ASCII emulation
//						default:
//							break;
//						}
//					}
//				}
//				break;
//			case '`': // HPA - Horizontal Position Absolute
//				m_isMidSeq = false;
//				break;
//			case 'a': // HPR - Horizontal Position Relative
//				m_isMidSeq = false;
//				break;
//			case 'e': // VPR - Vertical Position Relative - relative line #
//				m_isMidSeq = false;
//				break;
//			case 'I': // CHT - Cursor Horizontal Forward Tabulation
//				m_isMidSeq = false;
//				break;
//			case '~': // DECFNK - Function Key
//				m_isMidSeq = false;
//				break;
//			//case 'N': // ANSI Music?
//			//case '"': // ??
//			//case 'Z': // CBT - Cursor Backward Tabulation
//			//case 'U': // NP - Next Page
//			//case 'V': // PP - Preceding Page
//			//case '-': // deletes entire column?
//			//case '}': // insert column
//			//case 'X': // erase character
//			//case 'g': //	TBC - Tab Clear
//				//ESC [g  clear tab at current column
//				//ESC [3g  clear all tabs
//			//case 'i': //	?? printer?
//				//ESC [4i  ignored Stop relay to printer
//				//ESC [5i  ignored Start relay to printer
//			//case 'x': // ignored Send terminal parameter report - also req?
//			//case 'q': // keyboard LEDs?
//			//case ' q': // DECSCUSR — Set Cursor Style
//				// 0,1 - blinking block
//				//	2 - steady block
//				//	3 - blinking underline
//				//	4 - steady underline
//			//case 'y': // confidence test
//			//case 'p': // DECSSL - Select Set-Up Language
//			// ESC [>c or [>0c - DA2 - Secondary Device Attributes
//			// ESC [=c or [=0c - DA3 - Tertiary  Device Attributes
//			// ESC [=c or [=0c - DA3 - Tertiary  Device Attributes
//			// ESC [#'~ - DECDC - Delete Column
//			// ESC [#'} - DECIC - Insert Column
//			// ESC [#*x - DECSACE - Select Attribute Change Extent
//			// ESC [#*z - DECINVM - Invoke Macro
//			// ESC [#+z - DECPKA - Program Key Action
//			// ESC [#;# SP } - DECKBD - Keyboard Language Selection
//			// ESC [<hour>;<minute>,p - DECLTOD - Load Time of Day
//			// ESC [#*{ - DECMSR - Macro Space Report
//			// ESC [#;#+y - DECPKFMR - Program Key Free Memory Report
//			// ESC [#;#,v - DECRPKT - Report Key Type
//			// ESC [#;#$y - DECRPM - Report Mode - Terminal To Host
//			// ESC [?#;#$y - DECRPM - Report Mode - Terminal To Host
//			// ESC [#*p - DECSPPCS - Select ProPrinter Character Set
//			// ESC [#$p - DECRQM - Request Mode - Host To Terminal
//			// ESC [#$s - DECSPRTT - Select Printer Type
//			// ESC [?#$p - DECRQM - Request Mode - Host To Terminal
//			// ESC ["v - DECRQDE - Request Displayed Extent
//			// ESC [#;#,w - DECRQKD - Request Key Definition
//			// ESC [#,u - DECRQKT - Key Type Inquiry
//			// ESC [+x - DECRQPKFM - Program Key Free Memory Inquiry
//			// ESC [#$w - DECRQPSR - Request Presentation State Report
//			// ESC [#$u - DECRQTSR - Request Terminal State Report
//			// ESC [&u - DECRQUPSS - User-Preferred Supplemental Set
//			// ESC [#$} - DECSASD - Select Active Status Display
//			// ESC [#"q - DECSCA - Select Character Protection Attribute
//			// ESC [#$q - DECSDDT - Select Disconnect Delay Time
//			// ESC [#;#*u - DECSCP - Select Communication Port
//			// ESC [#;#*r - DECSCS - Select Communication Speed
//			// ESC [#$| - DECSCPP - Select 80 or 132 Columns per Page
//			// ESC [#)p - DECSDPT - Select Digital Printed Data Type
//			// ESC [#;#;#;#${ - DECSERA - Selective Erase Rectangular Area
//			// ESC [#;#;#;#*S - DECSFC - Select Flow Control
//			// ESC [ r - "SP r" - DECSKCV - Set Key Click Volume
//			// ESC [ v - "SP v" - DECSLCK - Set Lock Key Style
//			// ESC [# u - "# SP u" - DECSMBV - Set Margin Bell Volume
//			// ESC [#;#..;#+r - DECSMKR - Select Modifier Key Reporting
//			// ESC [#;#..;#+w - DECSPP - Set Port Parameter
//			// ESC [#*| - DECSNLS - Set Lines Per Screen
//			// ESC [#+p - DECSR - Secure Reset
//			// ESC [#*q - DECSRC - Secure Reset Confirmation
//			// ESC [#"t - DECSRFR - Select Refresh Rate
//			// ESC [# p - "SP p" - DECSSCLS - Set Scroll Speed
//			// ESC [#$~ - DECSSDT - Select Status Display (Line) Type
//			// ESC [?5W - DECST8C - Set Tab at Every 8 Columns
//			// ESC [!p - DECSTR - Soft Terminal Reset
//			// ESC [#;#"u - DECSTRL - Set Transmit Rate Limit
//			// ESC [# t - "SP t" - DECSWBV - Set Warning Bell Volume
//			// ESC [# ~ - "SP ~" - DECTME - Terminal Mode Emulation
//			// ESC [#,q - DECTID - Select Terminal ID
//			// ESC [#$u - DECTSR - Terminal State Report - Terminal to Host
//			// ESC [#i - MC - Media Copy - ANSI
//			// ESC [?#i - MC - Media Copy - VT mode
//			// ESC [# P - PPA - Page Position Absolute
//			// ESC [# R - PPB - Page Position Backward
//			// ESC [# Q - PPR - Page Position Relative
//			// ESC [?#;...;#l - RM - Reset Mode
//			default:
//				m_isMidSeq = false;
//				break;
//			}
//		}
//	}
//
//	void calc_screen(uint16_t& widthInChars, uint16_t& heightInChars)
//	{
//		uint16_t screenWidthInChars;
//		screenWidthInChars = get_size().get_width() / m_charWidthInPixels;
//		if (!screenWidthInChars)
//			screenWidthInChars = 1;
//		else if (screenWidthInChars > m_bufferWidthInChars)
//			screenWidthInChars = m_bufferWidthInChars;
//
//		uint16_t screenHeightInChars;
//		screenHeightInChars = get_size().get_height() / m_charHeightInPixels;
//		if (!screenHeightInChars)
//			screenHeightInChars = 1;
//		else if (screenHeightInChars > m_bufferHeightInChars)
//			screenHeightInChars = m_bufferHeightInChars;
//
//		heightInChars = screenHeightInChars;
//		widthInChars = screenWidthInChars;
//	}
//
//	void calc_screen()
//	{
//		uint16_t screenWidthInChars;
//		uint16_t screenHeightInChars;
//		calc_screen(screenWidthInChars, screenHeightInChars);
//		m_screenWidthInChars = screenWidthInChars;
//		m_screenHeightInChars = screenHeightInChars;
//	}
//
//public:
//	ansiterm(uint8_t fontSize, uint32_t bufHeight, uint32_t bufWidth, bool whiteBackground = false)
//			m_bufferHeightInChars(bufHeight),
//			m_bufferWidthInChars(bufWidth),
//			m_charWidthInPixels(get_char_width(fontSize)),
//			m_charHeightInPixels(get_char_height(fontSize)),
//			m_whiteBackground(whiteBackground), // usual is black
//			m_scrollPos(0),
//			m_fontBitMap(string::literal(L"ANSIFont") + integer<uint8_t>(fontSize).to_string(), true),
//			m_cursor(0,0),
//			m_screenTop(0),
//			m_stripMode(0xFF),
//			m_hadESC(false),
//			m_parseANSISeq(true),
//			m_gotQmark(false),
//			m_seqVarValid(false),
//			m_insertMode(false),
//			m_isMidSeq(false),
//			m_wrapMode(true), // wrapping behavior can be affected by certain ANSI sequences
//			m_addCRonLF(false),
//			m_tabSpaces(8),
//			m_hasBufferLooped(false),
//			m_cursorVis(true),
//			m_vt52Compat(false),
//			m_vt100Compat(false),
//			m_rawMode(false),
//			m_localEcho(false),
//			m_cursorMode(false),
//			m_blinkTimerExpireDelegate([r{ this_weak_rcptr }]()
//			{
//				rcptr<ansiterm> r2 = r;
//				if (!!r2)
//					r2->blink_timer_expired();
//			})
//		{
//		set_background_color(whiteBackground ? color::constant::white : color::constant::black);
//
//		m_forePallete[0] = color(0xB2, 0xB2, 0xB2);
//		m_forePallete[1] = color(0xB5, 0x00, 0x00);
//		m_forePallete[2] = color(0x00, 0xA5, 0x00);
//		m_forePallete[3] = color(0x00, 0xC0, 0xC0);
//		m_forePallete[4] = color(0x00, 0x00, 0xFF);
//		m_forePallete[5] = color(0xB1, 0xB1, 0x00);
//		m_forePallete[6] = color(0xB1, 0x00, 0xB1);
//		m_forePallete[7] = color(0x00, 0x00, 0x00);
//
//		m_forePalleteBold[0] = color(0xFF, 0xFF, 0xFF);
//		m_forePalleteBold[1] = color(0xFF, 0x00, 0x00);
//		m_forePalleteBold[2] = color(0x00, 0xFF, 0x00);
//		m_forePalleteBold[3] = color(0x00, 0xFF, 0xFF);
//		m_forePalleteBold[4] = color(0x36, 0x78, 0xFF);
//		m_forePalleteBold[5] = color(0xFF, 0xFF, 0x00);
//		m_forePalleteBold[6] = color(0xFF, 0x00, 0xFF);
//		m_forePalleteBold[7] = color(0x40, 0x40, 0x40);
//
//		m_backPallete[0] = color(0x00, 0x00, 0x00);
//		m_backPallete[1] = color(0xB5, 0x00, 0x00);
//		m_backPallete[2] = color(0x00, 0xA5, 0x00);
//		m_backPallete[3] = color(0x00, 0xC0, 0xC0);
//		m_backPallete[4] = color(0x00, 0x00, 0xFF);
//		m_backPallete[5] = color(0xB1, 0xB1, 0x00);
//		m_backPallete[6] = color(0xB1, 0x00, 0xB1);
//		m_backPallete[7] = color(0xFF, 0xFF, 0xFF);
//
//		calc_screen();
//
//		m_curBrush.value = 0;
//		m_savedBrush.value = 0;
//		m_savedBrush78.value = 0;
//		m_bufSize = bufHeight * bufWidth;
//
//		m_charBuffer = default_allocator::allocate_type<uint8_t>(m_bufSize);
//		memset((void*)(m_charBuffer), 0, m_bufSize); // * sizeof(uint8_t)
//
//		m_brushBuffer = default_allocator::allocate_type<brush>(m_bufSize);
//		memset((void*)(m_brushBuffer), 0, m_bufSize * sizeof(brush));
//
////		size_t screenChars = m_screenWidthInChars * m_screenHeightInChars;
////		m_updateTable.append(false, screenChars); // started empty
//
//		m_bufPos = 0;
//	}
//
//
//	static constexpr unsigned char BELL = 0x07;
//	static constexpr unsigned char BS = 0x08;
//	static constexpr unsigned char TAB = 0x09;
//	static constexpr unsigned char LF = 0x0A;
//	static constexpr unsigned char VT = 0x0B;
//	static constexpr unsigned char FF = 0x0C;
//	static constexpr unsigned char HOME = 0x0C;
//	static constexpr unsigned char CR = 0x0D;
//	static constexpr unsigned char ESC = 0x1B;
//	static constexpr unsigned char SPACE = 0x20;
//	static constexpr unsigned char DEL = 0x7F;
//	//static constexpr unsigned char MUSICNOTE = 0x0E;
//	static constexpr unsigned char CSI = 0x9B;
//	static constexpr unsigned char DCS = 0x90;
//	static constexpr unsigned char ST = 0x9C;
//	static constexpr unsigned char SS3 = 0x8F;
//	static constexpr unsigned char IND = 0x84;
//	static constexpr unsigned char RI = 0x8D;
//	static constexpr unsigned char NEL = 0x85;
//	static constexpr unsigned char HTS = 0x88;
//
//	static int get_char_width(uint8_t fontSize)
//	{
//		if (fontSize > 2)
//			return 0;
//		int widths[] = { 6, 7, 10 };
//		return widths[fontSize];
//	}
//
//	static int get_char_height(uint8_t fontSize)
//	{
//		if (fontSize > 2)
//			return 0;
//		int heights[] = { 11, 16, 18 };
//		return heights[fontSize];
//	}
//
//	static int get_default_width(uint8_t fontSize) { return get_char_width(fontSize) * 80; }
//	static int get_default_height(uint8_t fontSize) { return get_char_height(fontSize) * 24; }
//
//	~ansiterm()
//	{
//		if (!!m_datastream)
//			m_datastream->close();
//
//		default_allocator::destruct_deallocate_type(m_charBuffer, m_bufSize);
//		default_allocator::destruct_deallocate_type(m_brushBuffer, m_bufSize);
//	}
//
//	virtual void drawing()
//	{
//		unsigned int blinkState = m_blinkState;
//		color backgroundColor;
//		if (m_whiteBackground)
//			backgroundColor = m_backPallete[BackWhite];
//		else
//			backgroundColor = m_backPallete[BackBlack];
//
//		uint16_t screenWidthInChars = m_screenWidthInChars;
//		uint16_t screenHeightInChars = m_screenHeightInChars;
//
//		int  rightEdge = screenWidthInChars  * m_charWidthInPixels;
//
//		uint32_t curRow = 0; // in coordinates of the visible portion
//		for (;;)
//		{
//			uint32_t curColumn = 0;
//			point dstPt;
//			dstPt.set_y(curRow * m_charHeightInPixels);
//			do {
//				dstPt.set_x(curColumn * m_charWidthInPixels);
//
//				bounds dstBounds(dstPt, m_charWidthInPixels, m_charHeightInPixels);
//				if (is_unclipped(dstBounds))
//				{
//					uint32_t curBufLine = curRow + m_scrollPos;
//					curBufLine %= m_bufferHeightInChars;
//
//					size_t i = (curBufLine * m_bufferWidthInChars) + curColumn;
//					uint8_t c = m_charBuffer[i];
//
//					bool doUnderline = m_brushBuffer[i].underlined;
//					uint16_t blinkMode = m_brushBuffer[i].blinking;
//					if (blinkMode != 0)
//					{
//						if (blinkMode == 1)
//						{
//							if ((blinkState >> 1) % 2)
//							{
//								c = ' ';
//								doUnderline = false;
//							}
//						}
//						else // if (blinkMode == 3)
//						{
//							if (blinkState % 2)
//							{
//								c = ' ';
//								doUnderline = false;
//							}
//						}
//					}
//
//					int fontRow = c / 16;
//					int fontColumn = c % 16;
//
//					int foreColorIndex = m_brushBuffer[i].foreFlag;
//					int backColorIndex = m_brushBuffer[i].backFlag;
//
//					if (m_brushBuffer[i].inverse)
//					{
//						int tmp = foreColorIndex;
//						foreColorIndex = backColorIndex;
//						backColorIndex = tmp;
//						if (!foreColorIndex)
//							foreColorIndex = 7;
//						if (!backColorIndex)
//							backColorIndex = 7;
//					}
//
//					color foreColor;
//					if (m_brushBuffer[i].bold)
//						foreColor = m_forePalleteBold[foreColorIndex];
//					else
//						foreColor = m_forePallete[foreColorIndex];
//					draw_image_mask(m_fontBitMap,
//						bounds((fontColumn * m_charWidthInPixels) + 3, (fontRow * m_charHeightInPixels) + 3,
//							m_charWidthInPixels, m_charHeightInPixels),
//						dstPt,
//						foreColor,
//						m_backPallete[backColorIndex]);
//
//
//					//if (m_cursorMode == false) // underline mode
//					//{
//					//	if (m_cursorVis && ((blinkState >> 1) % 2))
//					//	{
//					//		uint32_t cursorRowBufLine = m_cursorY + m_screenTop;
//					//		cursorRowBufLine %= m_bufferHeightInChars;
//
//					//		if (curBufLine == cursorRowBufLine)
//					//		{
//					//			if ((curColumn == m_cursorX) || ((curColumn == screenWidthInChars-1) && (m_cursorX >= screenWidthInChars)))
//					//			{
//					//				doUnderline = !doUnderline;
//					//			}
//					//		}
//					//	}
//					//}
//
//					if (doUnderline)
//					{
//						point lineStart = dstPt;
//						lineStartY += (m_charHeightInPixels - 1);
//						point lineEnd = lineStart;
//						lineEndX += m_charWidthInPixels;
//						draw_line(lineStart, lineEnd, foreColor);
//					}
//
//			//		if (m_cursorMode == true) // block mode
//					{
//						if (m_cursorVis && ((blinkState >> 1) % 2))
//						{
//							uint32_t cursorRowBufLine = m_cursorY + m_screenTop;
//							cursorRowBufLine %= m_bufferHeightInChars;
//
//							if (curBufLine == cursorRowBufLine)
//							{
//								if ((curColumn == m_cursorX) || ((curColumn == screenWidthInChars-1) && (m_cursorX >= screenWidthInChars)))
//								{
//									point cursorPt = dstPt;
//									uint16_t cursorHeight = m_charHeightInPixels;
//									if (m_cursorMode == false)
//									{
//										cursorPtY += cursorHeight;
//										cursorHeight /= 5;
//										if (!cursorHeight)
//											cursorHeight = 1;
//										cursorPtY -= cursorHeight;
//									}
//									invert(bounds(cursorPt, size(m_charWidthInPixels, cursorHeight)));
//								}
//							}
//						}
//					}
//				}
//
//			} while (++curColumn != screenWidthInChars);
//			++curRow;
//			if (m_scrollPos == m_screenTop)
//			{
//				if (curRow == screenHeightInChars)
//				{
//					int bottomEdge = screenHeightInChars * m_charHeightInPixels;
//					bounds bottomBleedRect(0, bottomEdge, rightEdge, get_size().get_height() - bottomEdge);
//					fill(bottomBleedRect, backgroundColor);
//					break;
//				}
//			}
//			else
//			{
//				if (curRow > screenHeightInChars)
//					break;
//			}
//		}
//		bounds rightBleedRect(rightEdge, 0, get_size().get_width() - rightEdge, get_size().get_height());
//		fill(rightBleedRect, backgroundColor);
//
//		// How to handle blinking characters.  Timer?  Mod a known time to enforce synchronicity
//		// Timer to trigger updates, and check of mod tickCount to determine desired state vs. current state.
//	}
//
////	volatile container_serial_defer_guard<buffer> m_insertDeferGuard;
//
//
//	// Uses 8-bit type, as ANSI character set is 8-bit
//	void insert(const buffer& buf)
//	{
//		m_insertDeferGuard.begin_guard();
//		if (buf.get_length() > 0)
//			m_insertDeferGuard.add(buf);
//		release_insert_guard();
//	}
//
//	void insert(char c) { insert(buffer(&c, 1)); }
//	void insert(const cstring& s) { insert(buffer(s)); }
//	void insert(const string& s) { insert(string_to_cstring(s)); }
//	void insert(const char* buf, size_t n) { insert(buffer(buf, n)); }
//
//	virtual void resized(const size& oldSize)
//	{
//		m_insertDeferGuard.begin_guard();
//		m_insertDeferGuard.add(buffer(0));
//		release_insert_guard(); // resize handled by insert, only when none in progress
//	}
//
//	void release_insert_guard()
//	{
//		buffer buf;
//		bool released;
////		defer_invalidates();
//		while (m_insertDeferGuard.release(buf, &released))
//		{
//			size_t n = buf.get_length();
//			uint8_t* ptr = (uint8_t*)buf.get();
//
//			// Doesn't do any drawing, only 'invalidate' and change data.
//			if (n)
//			{
//				for (size_t i = 0; i < n; i++)
//				{
//					char c = ptr[i] & m_stripMode;
//					if (m_isMidSeq)
//						parse_mid_sequence(c);
//					else if (m_hadESC) // if last character received was an ESC
//						parse_esc_code(c);
//					else if (m_rawMode)
//						raw_insert(c);
//					else
//						parse(c);
//				}
//			}
//			else
//			{
//				// handle resize
//				int32_t oldHeight = m_screenHeightInChars;
//				int32_t oldWidth = m_screenWidthInChars;
//
//				calc_screen();
//
//				int32_t screenHeightInChars = m_screenHeightInChars;
//				int32_t screenWidthInChars = m_screenWidthInChars;
//
//				if ((screenHeightInChars != oldHeight) || (screenWidthInChars != oldWidth))
//				{
//					size_t screenChars = screenHeightInChars * screenWidthInChars;
////					m_updateTable.resize(screenChars);
////					memset(&(m_updateTable[0]), 0, screenChars * sizeof(bool));
//
//					send_window_size(screenWidthInChars, screenHeightInChars); // Tell telnet server of new size
//
//					if (screenHeightInChars < oldHeight)
//					{
//						// height is shrinking
//						// Bottom line must be preserved, content pushes up into scroll buffer
//						uint32_t dif = oldHeight - screenHeightInChars;
//						if (m_cursorY >= dif)
//						{
//							m_cursorY -= dif;
//							COGS_ASSERT(m_cursorY < 999);
//						}
//						else
//							m_cursorY = 0;
//						bool tracking = m_screenTop == m_scrollPos;
//						m_screenTop += dif;
//						m_screenTop %= m_bufferHeightInChars;
//						if (tracking)
//							m_scrollPos = m_screenTop;
//						//else // no change?
//						calc_buf_pos();
//					}
//					else if (screenHeightInChars > oldHeight)
//					{
//						// height is being added
//						// Bottom line must remain the bottom line.
//						// Content is pulled down from scroll buffer.
//						// If no content in scroll buffer, new lines are added
//						uint32_t dif = screenHeightInChars - oldHeight;
//						m_cursorY += dif;
//						COGS_ASSERT(m_cursorY < 999);
//						uint32_t oldScreenTop = m_screenTop;
//
//						// If there is enough content above, just update screenPos
//						if (m_hasBufferLooped)
//						{
//							m_screenTop -= dif;
//							if (m_screenTop >= m_bufferHeightInChars)
//							{
//								m_screenTop += m_bufferHeightInChars;
//								if ((m_scrollPos <= oldScreenTop) || (m_scrollPos > m_screenTop))
//									m_scrollPos = m_screenTop;
//							}
//							else
//							{
//								if ((m_scrollPos <= oldScreenTop) && (m_scrollPos > m_screenTop))
//									m_scrollPos = m_screenTop;
//							}
//						}
//						else
//						{
//							if (m_screenTop >= dif)
//								m_screenTop -= dif;
//							else
//							{
//								uint32_t numLinesReclaimed = m_screenTop;
//								uint32_t numLinesNeeded = dif - numLinesReclaimed;
//								clean_lines(oldHeight, numLinesNeeded);
//								m_screenTop = 0;
//								m_cursorY -= numLinesNeeded;
//								COGS_ASSERT(m_cursorY < 999);
//							}
//							if (m_scrollPos > m_screenTop)
//								m_scrollPos = m_screenTop;
//						}
//						calc_buf_pos();
//					}
//					if (screenHeightInChars != oldHeight)
//						fix_scrollbar();
//					invalidate();
//				}
//			}
//		}
////		flush_invalidates();
//	}
//
//
//	virtual void scrolled(uint8_t dimension, uint32_t pos)
//	{
//		if (dimension == 1)
//		{
//			uint32_t topOfAllLine = 0;
//			if (m_hasBufferLooped)
//			{
//				uint16_t screenHeightInChars = m_screenHeightInChars;
//				topOfAllLine = m_screenTop + screenHeightInChars;
//				topOfAllLine %= m_bufferHeightInChars;
//			}
//			m_scrollPos = topOfAllLine + pos;
//			m_scrollPos %= m_bufferHeightInChars;
//
//			invalidate(); // TBD be more efficient with drawing
//		}
//	}
//
//private:
//	//volatile integer<int> m_datastreamVersion;
//
//	class read_handler : public datasource::wait_result::notification, public datasource::read_result::notification
//	{
//	public:
//		weak_rcptr<read_handler> m_self;
//		weak_rcptr<ansiterm> m_ansiterm;
//		integer<int> m_datastreamVersion;
//
//		read_handler(const weak_rcptr<ansiterm>& ansiTerm, int datastreamVersion)
//			: m_ansiterm(ansiTerm),
//			m_datastreamVersion(datastreamVersion)
//		{ }
//
//		virtual void run(const rcref<datasource::wait_result>& rr)
//		{
//			rcptr<ansiterm> ansiTerm = m_ansiterm;
//			if (!!ansiTerm)
//			{
//				if (ansiTerm->m_datastreamVersion == m_datastreamVersion)
//				{
//					rcref<datasource> ds = rr->get_datasource();
//					size_t n = ds->get_count();
//					if (!n)
//					{
//						// TBD: Deal with connection closed
//					}
//					else
//					{
//						rcref<datasource::read_result> rr = ds->read(n);
//						rcptr<read_handler> self = m_self;
//						rr->wait(self.get_ref());
//					}
//				}
//			}
//		}
//
//		virtual void run(const rcref<datasource::read_result>& rr)
//		{
//			rcptr<ansiterm> ansiTerm = m_ansiterm;
//			if (!!ansiTerm)
//			{
//				if (ansiTerm->m_datastreamVersion == m_datastreamVersion)
//				{
//					rcref<datastream> ds = rr->get_datasource();
//					buffer buf = rr->get_buffer();
//					size_t n = buf.get_length();
//					if (!n)
//					{
//						// TBD: Deal with connection closed
//					}
//					else
//					{
//						ansiTerm->insert(buf);
//						rcref<datasource::wait_result> wr = ds->wait();
//						rcptr<read_handler> self = m_self;
//						wr->wait(self.get_ref());
//					}
//				}
//			}
//		}
//	};
//
//public:
//	rcptr<datastream> set_datastream(const rcptr<datastream>& ds, bool closeOldStream = true)
//	{
//		rcptr<datastream> tmp(ds);
//		m_datastream.swap(tmp);
//		if (tmp != ds)
//		{
//			if (closeOldStream)
//			{
//				if (!!tmp)
//					tmp->close();
//			}
//
//			// Sours the current read and prevents reuse if old datastream is set again.
//			int curDatastreamVersion = ++m_datastreamVersion;
//
//			if (!!ds)
//			{
//				rcref<datasource::wait_result> wr = ds->wait();
//				rcref<read_handler> readHandler = rcnew(read_handler)(m_self, curDatastreamVersion);
//				readHandler->m_self = readHandler;
//				wr->wait(readHandler);
//			}
//		}
//		return tmp;
//	}
//
//	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers)
//	{
//		COGS_ASSERT(m_cursorY < 999);
//		switch (c)
//		{
//		case UpArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'A' };
//				write(buffer(msg, 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'A' };
//				write(buffer(msg, 3));
//			}
//			break;
//		case DownArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'B' };
//				write(buffer(msg, 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'B' };
//				write(buffer(msg, 3));
//			}
//			break;
//		case RightArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'C' };
//				write(buffer(msg, 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'C' };
//				write(buffer(msg, 3));
//			}
//			break;
//		case LeftArrowKey:
//			if (m_vt52Compat)
//			{
//				uint8_t msg[2] = { ESC, 'D' };
//				write(buffer(msg, 2));
//			}
//			else
//			{
//				uint8_t msg[3] = { ESC, '[', 'D' };
//				write(buffer(msg, 3));
//			}
//			break;
//		default:
//		}
//		COGS_ASSERT(m_cursorY < 999);
//	}
//
//	virtual void character_typing(wchar_t c, const ui::modifier_keys_state& modifiers)
//	{
//		if (m_scrollPos != m_screenTop)
//		{
//			m_scrollPos = m_screenTop;
//			invalidate();
//		}
//		if (m_localEcho)
//			insert((char)c);
//		write(buffer(&c, 1));
//	}
//
//	virtual void write(const buffer& buf)
//	{
//		rcptr<datastream> ds = m_datastream;
//		if (!ds)
//			insert(buf);
//		else
//			ds->write(buf);
//	}
//
//	virtual cstring get_telnet_terminal_type() { return cstring::literal("ANSI"); }
//
//	virtual void focusing() // Start blinking, etc.
//	{
//	}
//
//	virtual void unfocusing() // Stop blinking, etc.
//	{
//	}
//
//	//virtual bool is_focusable()
//	//{
//	//	return true;
//	//}
//
//	*/
//#endif
//};
//
//
//}
//}
//
//
//#endif
//
