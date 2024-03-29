//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MATH_CHARS
#define COGS_HEADER_MATH_CHARS


namespace cogs {


template <typename char_t>
class special_characters;


template <>
class special_characters<char>
{
public:
	// Control codes
	static constexpr char BELL   = '\x07';
	static constexpr char BS     = '\x08';
	static constexpr char TAB    = '\x09';
	static constexpr char LF     = '\x0A';
	static constexpr char VT     = '\x0B';
	static constexpr char FF     = '\x0C';
	static constexpr char HOME   = '\x0C';
	static constexpr char CR     = '\x0D';
	static constexpr char ESC    = '\x1B';
	static constexpr char ESCAPE = '\x1B';
	static constexpr char SPACE  = '\x20';
	static constexpr char DEL    = '\x7F';
	static constexpr char IND    = '\x84';
	static constexpr char NEL    = '\x85';
	static constexpr char RI     = '\x8D';
	static constexpr char SS3    = '\x8F';
	static constexpr char HTS    = '\x88';
	static constexpr char CSI    = '\x9B';
	static constexpr char DCS    = '\x90';
	static constexpr char ST     = '\x9C';

	// Symbols
	static constexpr char MU     = '\xB5';
};


template <>
class special_characters<unsigned char>
{
public:
	// Control codes
	static constexpr unsigned char BELL   = (unsigned char)0x07;
	static constexpr unsigned char BS     = (unsigned char)0x08;
	static constexpr unsigned char TAB    = (unsigned char)0x09;
	static constexpr unsigned char LF     = (unsigned char)0x0A;
	static constexpr unsigned char VT     = (unsigned char)0x0B;
	static constexpr unsigned char FF     = (unsigned char)0x0C;
	static constexpr unsigned char HOME   = (unsigned char)0x0C;
	static constexpr unsigned char CR     = (unsigned char)0x0D;
	static constexpr unsigned char ESC    = (unsigned char)0x1B;
	static constexpr unsigned char ESCAPE = (unsigned char)0x1B;
	static constexpr unsigned char SPACE  = (unsigned char)0x20;
	static constexpr unsigned char DEL    = (unsigned char)0x7F;
	static constexpr unsigned char IND    = (unsigned char)0x84;
	static constexpr unsigned char NEL    = (unsigned char)0x85;
	static constexpr unsigned char RI     = (unsigned char)0x8D;
	static constexpr unsigned char SS3    = (unsigned char)0x8F;
	static constexpr unsigned char HTS    = (unsigned char)0x88;
	static constexpr unsigned char CSI    = (unsigned char)0x9B;
	static constexpr unsigned char DCS    = (unsigned char)0x90;
	static constexpr unsigned char ST     = (unsigned char)0x9C;

	// Symbols
	static constexpr unsigned char MU     = (unsigned char)0xB5;
};


template <>
class special_characters<wchar_t>
{
public:
	// Control codes
	static constexpr wchar_t BELL   = (wchar_t)0x07;
	static constexpr wchar_t BS     = (wchar_t)0x08;
	static constexpr wchar_t TAB    = (wchar_t)0x09;
	static constexpr wchar_t LF     = (wchar_t)0x0A;
	static constexpr wchar_t VT     = (wchar_t)0x0B;
	static constexpr wchar_t FF     = (wchar_t)0x0C;
	static constexpr wchar_t HOME   = (wchar_t)0x0C;
	static constexpr wchar_t CR     = (wchar_t)0x0D;
	static constexpr wchar_t ESC    = (wchar_t)0x1B;
	static constexpr wchar_t ESCAPE = (wchar_t)0x1B;
	static constexpr wchar_t SPACE  = (wchar_t)0x20;
	static constexpr wchar_t DEL    = (wchar_t)0x7F;
	static constexpr wchar_t IND    = (wchar_t)0x84;
	static constexpr wchar_t NEL    = (wchar_t)0x85;
	static constexpr wchar_t RI     = (wchar_t)0x8D;
	static constexpr wchar_t SS3    = (wchar_t)0x8F;
	static constexpr wchar_t HTS    = (wchar_t)0x88;
	static constexpr wchar_t CSI    = (wchar_t)0x9B;
	static constexpr wchar_t DCS    = (wchar_t)0x90;
	static constexpr wchar_t ST     = (wchar_t)0x9C;

	// Symbols
	static constexpr wchar_t MU     = (wchar_t)0x03BC;
};


}


#endif
