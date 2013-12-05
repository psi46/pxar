// cpp_parser.h

#pragma once

#include <string>
using namespace std;


class CppParser
{
	bool typeUnsigned; // f = signed, t = unsigned
	char typeChar;   // 'b' = bool, 'c' = char, 's' = short, 'i' = int, 'l' = long, 'd' = long long
	char ctypeChar;  // 0 = simple, '0' = ref, '1' = Array, '2' = ArrayR, '3' = String, '4' = StringR
	string fname;
	string fparam;

	CppScanner f;

	void GetIntegerType();
	void GetType();
	void GetComplexType();
	void GetParameter();
	void GetParList();
	void GetModifier();
	void GetFunction();
public:
	CppParser() {}
	void Open(const char *fileName) { f.Open(fileName); }
	void Close() { f.Close(); }

	void GetFunctionDecl();
	unsigned char GetRpcExport();

	const string& GetFname() { return fname; }
	const string& GetFparam() { return fparam; }
};
