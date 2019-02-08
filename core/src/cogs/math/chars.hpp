//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_CHARS
#define COGS_CHARS


namespace cogs {


template <typename char_t>
class special_characters;


template <>
class special_characters<char>
{
public:
	// Control codes
	static const char BELL		=	(char)0x07;
	static const char BS		=	(char)0x08;
	static const char TAB		=	(char)0x09;
	static const char LF		=	(char)0x0A;
	static const char VT		=	(char)0x0B;
	static const char FF		=	(char)0x0C;
	static const char HOME		=	(char)0x0C;
	static const char CR		=	(char)0x0D;
	static const char ESC		=	(char)0x1B;
	static const char ESCAPE	=	(char)0x1B;
	static const char SPACE		=	(char)0x20;
	static const char DEL		=	(char)0x7F;
	static const char IND		=	(char)0x84;
	static const char NEL		=	(char)0x85;
	static const char RI		=	(char)0x8D;
	static const char SS3		=	(char)0x8F;
	static const char HTS		=	(char)0x88;
	static const char CSI		=	(char)0x9B;
	static const char DCS		=	(char)0x90;
	static const char ST		=	(char)0x9C;

	// Symbols
	static const char MU	=	(char)0xB5;
};


template <>
class special_characters<unsigned char>
{
public:
	// Control codes
	static const unsigned char BELL		=	(unsigned char)0x07;
	static const unsigned char BS		=	(unsigned char)0x08;
	static const unsigned char TAB		=	(unsigned char)0x09;
	static const unsigned char LF		=	(unsigned char)0x0A;
	static const unsigned char VT		=	(unsigned char)0x0B;
	static const unsigned char FF		=	(unsigned char)0x0C;
	static const unsigned char HOME		=	(unsigned char)0x0C;
	static const unsigned char CR		=	(unsigned char)0x0D;
	static const unsigned char ESC		=	(unsigned char)0x1B;
	static const unsigned char ESCAPE	=	(unsigned char)0x1B;
	static const unsigned char SPACE	=	(unsigned char)0x20;
	static const unsigned char DEL		=	(unsigned char)0x7F;
	static const unsigned char IND		=	(unsigned char)0x84;
	static const unsigned char NEL		=	(unsigned char)0x85;
	static const unsigned char RI		=	(unsigned char)0x8D;
	static const unsigned char SS3		=	(unsigned char)0x8F;
	static const unsigned char HTS		=	(unsigned char)0x88;
	static const unsigned char CSI		=	(unsigned char)0x9B;
	static const unsigned char DCS		=	(unsigned char)0x90;
	static const unsigned char ST		=	(unsigned char)0x9C;

	// Symbols
	static const unsigned char MU	=	(unsigned char)0xB5;
};


template <>
class special_characters<wchar_t>
{
public:
	// Control codes
	static const wchar_t BELL	=	(wchar_t)0x07;
	static const wchar_t BS		=	(wchar_t)0x08;
	static const wchar_t TAB	=	(wchar_t)0x09;
	static const wchar_t LF		=	(wchar_t)0x0A;
	static const wchar_t VT		=	(wchar_t)0x0B;
	static const wchar_t FF		=	(wchar_t)0x0C;
	static const wchar_t HOME	=	(wchar_t)0x0C;
	static const wchar_t CR		=	(wchar_t)0x0D;
	static const wchar_t ESC	=	(wchar_t)0x1B;
	static const wchar_t ESCAPE	=	(wchar_t)0x1B;
	static const wchar_t SPACE	=	(wchar_t)0x20;
	static const wchar_t DEL	=	(wchar_t)0x7F;
	static const wchar_t IND	=	(wchar_t)0x84;
	static const wchar_t NEL	=	(wchar_t)0x85;
	static const wchar_t RI		=	(wchar_t)0x8D;
	static const wchar_t SS3	=	(wchar_t)0x8F;
	static const wchar_t HTS	=	(wchar_t)0x88;
	static const wchar_t CSI	=	(wchar_t)0x9B;
	static const wchar_t DCS	=	(wchar_t)0x90;
	static const wchar_t ST		=	(wchar_t)0x9C;

	// Symbols
	static const wchar_t MU	=	(wchar_t)0x03BC;
};



}


#endif

