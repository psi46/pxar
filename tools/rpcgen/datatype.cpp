// datatype.cpp

#include "stdafx.h"


const char* CDataType::typeCNames[12] =
{
	"void", "bool", "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
	"int64_t", "uint64_t"
};


const char* CDataType::typeNames[] =
{
	"VOID", "BOOL", "INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32",
	"INT64", "UINT64"
};

const unsigned int CDataType::typeByteSize[12] = { 0, 1, 1, 1, 2, 2, 4, 4, 8, 8 };


bool CDataType::Read(const char*&s, int parNr)
{
	id = parNr;
	parByteCount = 0;
	retByteCount = 0;
	retValue = false;

	if (*s == 0) return false;
	switch (*s)
	{
		case '0': comp = REFERENCE; s++; break;
		case '1': comp = VECTOR;     s++; break;
		case '2': comp = VECTORR;    s++; break;
		case '3': comp = STRING;    s++; break;
		case '4': comp = STRINGR;   s++; break;
		default:  comp = SIMPLE;
	}

	switch (*(s++))
	{
		case 'v': type = VOID;   break;
		case 'b': type = BOOL;   break;
		case 'c': type = CHAR;   break;
		case 'C': type = UCHAR;  break;
		case 's': type = SHORT;  break;
		case 'S': type = USHORT; break;
		case 'i': type = INT;    break;
		case 'I': type = UINT;   break;
		case 'l': type = LONG;   break;
		case 'L': type = ULONG;  break;
		default: throw "unknow data type id";
	}

	if (IsReturn())
	{
		if (comp != SIMPLE)  throw "return value should be a simple type";
		retByteCount = typeByteSize[type];
	}
	else
	{
		if (type == VOID)  throw "void in parameter list not allowed";
		if (comp == REFERENCE)                    retByteCount = typeByteSize[type];
		if (comp == REFERENCE || comp == SIMPLE)  parByteCount = typeByteSize[type];
	}

	if (retByteCount || comp == VECTORR || comp == STRINGR) retValue = true;

	switch (comp)
	{
	case SIMPLE:
	case REFERENCE:
		cTypeName = string(typeCNames[type]);
		break;
	case VECTOR:
		cTypeName = string("vector<") + typeCNames[type] + ">";
		break;
	case VECTORR:
		cTypeName = string("vectorR<") + typeCNames[type] + ">";
		break;
	case STRING:
		cTypeName = string("string");
		break;
	case STRINGR:
		cTypeName = string("stringR");
		break;
	}

	return true;
}


void CDataType::WriteSendPar(FILE *f)
{
	if (parByteCount)
		fprintf(f, "\tmsg.Put_%s(rpc_par%u);\n", GetTypeName(), id);
}


void CDataType::WriteSendDat(FILE *f)
{
	if (comp == VECTOR || comp == STRING)
		fprintf(f, "\trpc_Send(*rpc_io, rpc_par%u);\n", id);
}

void CDataType::WriteRecvPar(FILE *f)
{
	if (retByteCount)
	{
		if (IsReturn())
			fprintf(f, "\trpc_par0 = msg.Get_%s();\n", GetTypeName());
		else
			fprintf(f, "\trpc_par%u = msg.Get_%s();\n", id, GetTypeName());
	}
}

void CDataType::WriteRecvDat(FILE *f)
{
	if (comp == VECTORR || comp == STRINGR)
		fprintf(f, "\trpc_Receive(*rpc_io, rpc_par%u);\n", id);
}


void CDataType::WriteDtbSendPar(FILE *f)
{
	if (parByteCount)
		fprintf(f, "\t%s rpc_par%u = msg.Get_%s();\n", GetCTypeName(), id, GetTypeName());
}


void CDataType::WriteDtbSendDat(FILE *f)
{
	switch (comp)
	{
	case VECTOR:
	case STRING:
		fprintf(f, "\t%s rpc_par%u; if (!rpc_Receive(rpc_io, rpc_par%u)) return false;\n", GetCTypeName(), id, id);
		break;
	case VECTORR:
	case STRINGR:
		fprintf(f, "\t%s rpc_par%u;\n", GetCTypeName(), id);
		break;
    default:
        break;
	}
}


void CDataType::WriteDtbRecvPar(FILE *f)
{
	if (retByteCount)
		fprintf(f, "\tmsg.Put_%s(rpc_par%u);\n", GetTypeName(), id);
}


void CDataType::WriteDtbRecvDat(FILE *f)
{
	if (comp == VECTORR || comp == STRINGR)
		fprintf(f, "\tif (!rpc_Send(rpc_io, rpc_par%u)) return false;\n", id);
}


// === CParameterList =======================================================

void CParameterList::Read(const char *s)
{
	parCount = 0;
	retCount = 0;
	retValues = false;

	unsigned int i = 0;
	while (*s)
	{
		CDataType t;
		if (t.Read(s, i)) par.push_back(t);
		parCount += t.GetParBytes();
		retCount += t.GetRetBytes();
		if (t.HasRetValue()) retValues = true;
		i++;
	}
	if (par.size() == 0) throw "empty parameter list";
	if (parCount > 255) throw "too many parameters";
	if (retCount > 255) throw "too many eturn parameter";
}


void CParameterList::WriteCDeclaration(FILE *f, const char *fname)
{
	unsigned int i;
	parIterator p;
	for (p = begin(), i = 0; p != end(); p++, i++)
	{
		if (i == 0)
			fprintf(f, "%s CTestboard::%s(", p->GetCTypeName(), fname);
		else
		{
			if (i > 1) fputs(", ", f);
			if (p->IsSimpleType())
				fprintf(f, "%s rpc_par%u", p->GetCTypeName(), i);
			else
				fprintf(f, "%s &rpc_par%u", p->GetCTypeName(), i);
		}
	}
	fputs(")\n", f);
}


void CParameterList::WriteDtbFunctCall(FILE *f, const char *fname)
{
	unsigned int i;
	parIterator p;
	for (p = begin(), i = 0; p != end(); p++, i++)
	{
		if (i == 0)
		{
			if (p->type == CDataType::VOID)
				fprintf(f, "\ttb.%s(", fname);
			else
				fprintf(f, "\t%s rpc_par0 = tb.%s(", p->GetCTypeName(), fname);
		}
		else
		{
			if (i > 1) fputs(",", f);
			fprintf(f, "rpc_par%u", i);
		}
	}
	fputs(");\n", f);	
}


void CParameterList::WriteAllSendPar(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteSendPar(f);
}


void CParameterList::WriteAllSendDat(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteSendDat(f);
}


void CParameterList::WriteAllRecvPar(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteRecvPar(f);
}


void CParameterList::WriteAllRecvDat(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteRecvDat(f);
}


void CParameterList::WriteAllDtbSendPar(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteDtbSendPar(f);
}


void CParameterList::WriteAllDtbSendDat(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteDtbSendDat(f);
}


void CParameterList::WriteAllDtbRecvPar(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteDtbRecvPar(f);
}


void CParameterList::WriteAllDtbRecvDat(FILE *f)
{
	for (parIterator p = begin(); p != end(); p++) p->WriteDtbRecvDat(f);
}

