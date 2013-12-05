// cpp_scanner.cpp

#include "stdafx.h"


void CppScanner::SkipWhitespace()
{
	while (IsWhitespace(f.Get())) f.GetNext();
}


void CppScanner::SkipComment()
{
	char ch = f.GetNext();
	if (ch == '*')
	{ // skip to string end
		do { while (f.GetNext() != '*'); } while(f.GetNext() != '/');
		f.GetNext();
	}
	else if (ch == '/')
	{ // skip to line end
		f.GetNext();
		while (f.Get() != '\n' && f.Get() != '\r') f.GetNext(); 
	}
}


void CppScanner::SkipTo(const char *name)
{
	if (name[0] == 0) return;

	while (true)
	{
		while (name[0] != f.Get())
		{
			if (f.Get() == '/') SkipComment();
			else f.GetNext();
		}

		unsigned int i = 0;
		while(true)
		{
			if (name[i] == 0) return;
			if (f.Get() == '/') { SkipComment(); break; }
			if (f.Get() != name[i]) break;
			f.GetNext();
			i++;
		}
	}
}


void CppScanner::GetNumber()
{
	long x;
	bool negative = false;

	if (f.Get() == '+') f.GetNext();
	else if (f.Get() == '-') { negative = true; f.GetNext(); }

	if (IsNumber(f.Get())) { x = f.Get() - '0'; f.GetNext(); }
	else throw CPError(CPError::WRONG_NUMBER_FORMAT);

	while (IsNumber(f.Get())) { x = x*10 + f.Get() - '0'; f.GetNext(); }
	if (f.Get() == 'u') f.GetNext();
	if (f.Get() == 'l') f.GetNext();
	if (f.Get() == 'l') f.GetNext();

	type = CToken::VALUE;
	value = x;
}


void CppScanner::GetName()
{
	char s[512];
	int i = 0;
	do
	{
		s[i++] = f.Get();
		if (i > 511) throw CPError(CPError::NAME_TOO_LONG);
		f.GetNext();
	} while (IsAlphaNum(f.Get()));
	s[i] = 0;

	type = CToken::SYMBOL;
	if      (!strcmp(s, "void"))     symbol = CToken::SYMB_VOID;
	else if (!strcmp(s, "bool"))     symbol = CToken::SYMB_BOOL;
	else if (!strcmp(s, "int8_t"))   symbol = CToken::SYMB_CHAR;
	else if (!strcmp(s, "int16_t"))  symbol = CToken::SYMB_SHORT;
	else if (!strcmp(s, "int32_t"))  symbol = CToken::SYMB_INT;
	else if (!strcmp(s, "int64_t"))  symbol = CToken::SYMB_LONG;
	else if (!strcmp(s, "uint8_t"))  symbol = CToken::SYMB_UCHAR;
	else if (!strcmp(s, "uint16_t")) symbol = CToken::SYMB_USHORT;
	else if (!strcmp(s, "uint32_t")) symbol = CToken::SYMB_UINT;
	else if (!strcmp(s, "uint64_t")) symbol = CToken::SYMB_ULONG;
	else if (!strcmp(s, "vector"))   symbol = CToken::SYMB_VECTOR;
	else if (!strcmp(s, "vectorR"))  symbol = CToken::SYMB_VECTORR;
	else if (!strcmp(s, "string"))   symbol = CToken::SYMB_STRING;
	else if (!strcmp(s, "stringR"))  symbol = CToken::SYMB_STRINGR;
	else if (!strcmp(s, "stringR"))  symbol = CToken::SYMB_STRINGR;
	else if (!strcmp(s, "false"))    symbol = CToken::SYMB_FALSE;
	else if (!strcmp(s, "true"))     symbol = CToken::SYMB_TRUE;
	else if (!strcmp(s, "static"))   symbol = CToken::SYMB_STATIC;
	else
	{
		type = CToken::NAME;
		name = new char[i+1];
		strcpy(name, s);
	}
}


void CppScanner::GetNext()
{
	SetEmpty();
	SkipWhitespace();
	if (f.Get() == '/') { SkipComment(); SkipWhitespace(); }

	char ch = f.Get();
	if (IsNumber(ch) || (ch == '+') || (ch == '-')) GetNumber();
	else if (IsAlpha(ch)) GetName();
	else if (ch == '(' || ch == ')' || ch == '<' || ch == '>' || ch == '=' || ch == ',' || ch == ';' || ch == '&')
	{ type = CToken::DELIMITER; delimiter = ch; f.GetNext(); }
	else throw CPError(CPError::UNKNOWN_CHARACTER);
}
