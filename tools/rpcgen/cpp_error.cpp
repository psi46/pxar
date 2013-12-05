// cpp_error.cpp

#include "stdafx.h"


const char* CPError::GetString()
{
	switch (error)
	{
		case OK:                  return "Ok";
		case FILE_OPEN:           return "Could not open file";
		case FILE_NOT_OPEN_READ:  return "Acces of closed file";
		case END_OF_FILE:         return "End of file";
		case FILE_READ_FAILED:    return "Read error";
		case UNKNOWN_CHARACTER:   return "Unknown character";
		case WRONG_NUMBER_FORMAT: return "Wrong number format";
		case NAME_TOO_LONG:       return "name too long";
		case TYPE_EXPECTED:       return "type specifier expected";
		case REFERENCE_EXPECTED:  return "Must be a reference parameter";
		case VALUE_EXPECTED:      return "Value expected";
		case BRACKET_EXPECTED:    return "Missing bracket";
		case FUNCTION_NAME_EXPECTED: return "Function name expected";
		case MISSING_CGRP:        return "Missing cgrp name";
		case WRONG_CGRP:          return "Wrong cgrp name";
	}
	return "";
}
