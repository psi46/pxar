// file.h

#pragma once

#include <stdio.h>


class CFile
{
	FILE *f;
	char lastChar;
	unsigned int pos, n;
	char *buffer;
	char FillBuffer();
public:
	CFile() : f(0), lastChar(0), pos(0), n(0) {}
	~CFile() { Close(); }
	void Open(const char *fileName);
	bool IsOpen() { return f != 0; }
	void Close();
	char Get() { return lastChar; }
	char GetNext();
};
