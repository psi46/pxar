// file.cpp

#include "stdafx.h"


#define FILE_BUFFER_SIZE 1024


void CFile::Open(const char *fileName)
{
	Close();
	f = fopen(fileName, "rt");
	if (f == 0) throw CPError(CPError::FILE_OPEN);
	buffer = new char[FILE_BUFFER_SIZE];
	pos = n = 0;
	lastChar = ' ';
}


void CFile::Close()
{
	if (IsOpen())
	{
		fclose(f);
		f = 0;
		delete[] buffer;
		lastChar = 0;
	}
}


char CFile::FillBuffer()
{
	if (!IsOpen()) throw CPError(CPError::FILE_NOT_OPEN_READ);
	n = fread(buffer, sizeof(char), FILE_BUFFER_SIZE, f); 
	pos = 0;
	if (n == 0)
	{
		if (feof(f)) throw CPError(CPError::END_OF_FILE);
		else throw CPError(CPError::FILE_READ_FAILED);
	}
	return buffer[pos++];
}


char CFile::GetNext()
{
	lastChar = (pos < n) ? buffer[pos++] : FillBuffer();
	return lastChar;
}
