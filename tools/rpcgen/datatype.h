// datatype.h

#pragma once

#include <string>
using namespace std;

#define NUMBER_OF_CGROUPS 32


class CDataType
{
	static const char* typeCNames[12];
	static const char* typeNames[12];
	static const unsigned int typeByteSize[12];
	enum { SIMPLE, REFERENCE,  VECTOR, VECTORR, STRING, STRINGR} comp;
	enum { VOID, BOOL, CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, DWORD, UDWORD } type;
	string cTypeName;
	bool retValue;
	unsigned int id; // 0 = return value
	unsigned int parByteCount;
	unsigned int retByteCount;
public:
	bool IsReturn() { return id == 0; }
	bool IsSimpleType() { return comp == SIMPLE; }
	const char* GetCTypeName() { return cTypeName.c_str(); }
	const char* GetTypeName()  { return typeNames[type]; }
	unsigned int GetParBytes() { return parByteCount; }
	unsigned int GetRetBytes() { return retByteCount; }
	bool HasRetValue() { return retValue; }

	bool Read(const char* &s, int parId);
	
	void WriteSendPar(FILE *f);
	void WriteSendDat(FILE *f);
	void WriteRecvPar(FILE *f);
	void WriteRecvDat(FILE *f);

	void WriteDtbSendPar(FILE *f);
	void WriteDtbSendDat(FILE *f);
	void WriteDtbRecvPar(FILE *f);
	void WriteDtbRecvDat(FILE *f);


	void WriteTypeDTB(FILE *f);
	void WriteCallPar(FILE *f);
	void WriteCallParDTB(FILE *f);
	void WriteArrayCreationDTB(FILE *f);
	void WriteArrayDeleteDTB(FILE *f);
	void WriteResponsePar(FILE *f);
	void WriteResponseParDTB(FILE *f);
	void WriteSendArray(FILE *f);
	void WriteReceiveArrayDTB(FILE *f);
	void WriteReceiveArray(FILE *f);
	void WriteSendArrayDTB(FILE *f);
	friend class CParameterList;
};

typedef list<CDataType>::iterator parIterator;


class CParameterList
{
	unsigned int parCount;
	unsigned int retCount;
	bool retValues;
	list<CDataType> par;
public:
	unsigned short GetTotalParBytes() { return parCount; }
	unsigned short GetTotalRetBytes() { return retCount; }
	bool HasRetValues() { return retValues; }
	parIterator begin() { return par.begin(); }
	parIterator end()   { return par.end(); }

	void Read(const char *s);

	void WriteCDeclaration(FILE *f, const char *fname);
	void WriteDtbFunctCall(FILE *f, const char *fname);

	void WriteAllSendPar(FILE *f);
	void WriteAllSendDat(FILE *f);
	void WriteAllRecvPar(FILE *f);
	void WriteAllRecvDat(FILE *f);

	void WriteAllDtbSendPar(FILE *f);
	void WriteAllDtbSendDat(FILE *f);
	void WriteAllDtbRecvPar(FILE *f);
	void WriteAllDtbRecvDat(FILE *f);
};

