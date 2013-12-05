// cpp_error.h

#pragma once

#include <stdio.h>


class CPError
{
public:
	enum ERROR
	{
		OK,
		FILE_OPEN,
		FILE_NOT_OPEN_READ,
		END_OF_FILE,
		FILE_READ_FAILED,
		UNKNOWN_CHARACTER,
		WRONG_NUMBER_FORMAT,
		NAME_TOO_LONG,
		TYPE_EXPECTED,
		REFERENCE_EXPECTED,
		VALUE_EXPECTED,
		BRACKET_EXPECTED,
		FUNCTION_NAME_EXPECTED,
		MISSING_CGRP,
		WRONG_CGRP
	} error;
	CPError(ERROR e) : error(e) {}
	const char* GetString();
	void What() { printf("ERROR: %s!\n", GetString()); }
};
