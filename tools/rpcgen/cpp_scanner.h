// cpp_scanner.h

#pragma once

#include <stdio.h>


class CToken
{
public:
	enum { EMPTY, VALUE, NAME, SYMBOL, DELIMITER } type;
	enum symType
	{
		SYMB_VOID, SYMB_BOOL, SYMB_CHAR, SYMB_SHORT, SYMB_INT, SYMB_LONG,
		SYMB_UCHAR, SYMB_USHORT, SYMB_UINT, SYMB_ULONG,
		SYMB_VECTOR, SYMB_VECTORR, SYMB_STRING,	SYMB_STRINGR,
		SYMB_FALSE, SYMB_TRUE, SYMB_STATIC
	};
	union
	{
		long value;
		char *name;
		symType symbol;
		char delimiter;
	};

	void SetEmpty() { if (type == NAME) delete[] name; type = EMPTY; }

	CToken() : type(EMPTY) {}
	~CToken() { if (type == NAME) delete[] name; }

	bool IsValue()  { return type == VALUE; }
	bool IsName() { return type == NAME; }
	bool IsSymbol() { return type == SYMBOL; }
	bool IsSymbol(symType sym) { return IsSymbol() && symbol == sym; }
	bool IsDelimiter() { return type == DELIMITER; }
	bool IsDelimiter(char ch) { return type == DELIMITER && delimiter == ch; }
};


class CppScanner : public CToken
{
	CFile f;
	bool IsAlpha();
	static bool IsNumber(char ch) { return '0' <= ch && ch <= '9'; }
	static bool IsAlpha(char ch) { return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || (ch == '_'); }
	static bool IsAlphaNum(char ch) { return IsAlpha(ch) || IsNumber(ch); }
	static bool IsWhitespace(char ch) { return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'); }

	void SkipWhitespace();
	void SkipComment();
	void GetNumber();
	void GetName();
public:
	CppScanner() {}
	void Open(const char *fileName) { f.Open(fileName); SetEmpty(); }
	void Close() { f.Close(); }
	const CToken* Get() { return this; }
	void GetNext();
	void SkipTo(const char *name);
};
