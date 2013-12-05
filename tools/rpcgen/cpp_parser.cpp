// cpp_parser.cpp

#include "stdafx.h"

#define RPC_CGRP_ID       0
#define RPC_CGRP_SERVICE  1
#define RPC_CGRP_DTB1     2
#define RPC_CGRP_DTB2     3
#define RPC_CGRP_DTB3     4
#define RPC_CGRP_DTB4     5
#define RPC_CGRP_ROC      8
#define RPC_CGRP_TBM      9
#define RPC_CGRP_ATB1    10
#define RPC_CGRP_USER1   16    
#define RPC_CGRP_USER2   17
#define RPC_CGRP_USER3   18
#define RPC_CGRP_USER4   19
#define RPC_CGRP_ERROR   31


void CppParser::GetIntegerType()
{
	if (f.IsSymbol()) switch (f.Get()->symbol)
	{
		case CToken::SYMB_CHAR: typeChar   = 'c'; break;
		case CToken::SYMB_SHORT: typeChar  = 's'; break;
		case CToken::SYMB_INT: typeChar    = 'i'; break;
		case CToken::SYMB_LONG: typeChar   = 'l'; break;
		case CToken::SYMB_UCHAR: typeChar  = 'C'; break;
		case CToken::SYMB_USHORT: typeChar = 'S'; break;
		case CToken::SYMB_UINT: typeChar   = 'I'; break;
		case CToken::SYMB_ULONG: typeChar  = 'L'; break;
		default: throw CPError(CPError::TYPE_EXPECTED);
	}
	else throw CPError(CPError::TYPE_EXPECTED);

	f.GetNext();
}


void CppParser::GetType()
{
	if (f.IsSymbol(CToken::SYMB_BOOL))
	{
		typeChar = 'b';
		f.GetNext();
	}
	else
	{
		GetIntegerType();
	}
}


void CppParser::GetComplexType()
{
	ctypeChar = 0;
	bool ret = false;
	if (f.IsSymbol()) switch (f.Get()->symbol)
	{
	case CToken::SYMB_VECTORR:
		ret = true;
	case CToken::SYMB_VECTOR:
		ctypeChar = ret ? '2' : '1';
		f.GetNext();
		if (!f.IsDelimiter('<')) throw CPError(CPError::TYPE_EXPECTED);
		f.GetNext();
		GetType();
		if (!f.IsDelimiter('>')) throw CPError(CPError::TYPE_EXPECTED);
		f.GetNext();
		if (!f.IsDelimiter('&')) throw CPError(CPError::REFERENCE_EXPECTED);
		f.GetNext();
		break;
	case CToken::SYMB_STRINGR:
		ret = true;
	case CToken::SYMB_STRING:
		ctypeChar = ret ? '4' : '3';
		typeChar = 'c';
		f.GetNext();
		if (!f.IsDelimiter('&')) throw CPError(CPError::REFERENCE_EXPECTED);
		f.GetNext();
		break;
	default: 
		GetType();
		if (f.IsDelimiter('&'))
		{
			ctypeChar = '0';
			f.GetNext();
		}
	}
}


void CppParser::GetParameter()
{
	GetComplexType();
	if (ctypeChar) fparam += ctypeChar;
	fparam += typeChar;
	if (f.IsName())
	{
		f.GetNext();
		if (f.IsDelimiter('='))
		{
			f.GetNext();
			if (!(f.IsValue() || f.IsSymbol(CToken::SYMB_FALSE) || f.IsSymbol(CToken::SYMB_TRUE) || f.IsName()))
				throw CPError(CPError::VALUE_EXPECTED);
			f.GetNext();
		}
	}
}


void CppParser::GetParList()
{
	GetParameter();
	while (f.IsDelimiter(',')) { f.GetNext(); GetParameter(); }
}


void CppParser::GetModifier()
{
	if (f.IsSymbol(CToken::SYMB_STATIC)) f.GetNext();
}


void CppParser::GetFunctionDecl()
{
	fparam = "";

	GetModifier();

	if (f.IsSymbol(CToken::SYMB_VOID))
	{
		fparam += 'v';
		f.GetNext(); 
	}
	else // type
	{
		GetType();
		fparam += typeChar;
	}

	if (f.IsName())
	{
		fname = f.Get()->name;
		f.GetNext();
	} else throw CPError(CPError::FUNCTION_NAME_EXPECTED);

	if (f.IsDelimiter('(')) f.GetNext(); else throw CPError(CPError::BRACKET_EXPECTED);

	if (f.IsDelimiter(')')) f.GetNext();
	else
	{
		if (f.IsSymbol(CToken::SYMB_VOID)) f.GetNext();
		else GetParList();
		if (f.IsDelimiter(')')) f.GetNext(); else throw CPError(CPError::BRACKET_EXPECTED);
	}

	if (!f.IsDelimiter(';')) throw CPError(CPError::BRACKET_EXPECTED);
}


unsigned char CppParser::GetRpcExport()
{
	f.SkipTo("RPC_EXPORT");
	f.GetNext();
	return -1;
}
