// rpc_io.h

#pragma once

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdint.h>

#include "rpc_error.h"


class CRpcIo
{
public:
	virtual ~CRpcIo() {}
	virtual void Write(const void *buffer, uint32_t size) = 0;
	virtual void Flush() = 0;
	virtual void Clear() = 0;
	virtual void Read(void *buffer, uint32_t size) = 0;
	virtual const char* Name() = 0;
	// Error processing
	virtual int32_t GetLastError() = 0;
	virtual const char* GetErrorMsg(int error) = 0;
	// Connection
	virtual bool Open(char name[]) = 0;
	virtual void Close() = 0;
	virtual bool EnumFirst(uint32_t &nDevices) = 0;
	virtual bool EnumNext(char name[]) = 0;
	virtual bool Enum(char name[], uint32_t pos) = 0;
	virtual bool Connected() = 0;
	virtual void SetTimeout(unsigned int timeout) = 0;
};


class CRpcIoNull : public CRpcIo
{
public:
        void Write(const void * /*buffer*/, uint32_t /*size*/) { throw CRpcError(CRpcError::WRITE_ERROR); }
	void Flush() {}
	void Clear() {}
	void Read(void * /*buffer*/, uint32_t /*size*/) { throw CRpcError(CRpcError::READ_ERROR); }
	const char* Name() { return "CRpcIoNull";};
	// Error processing
	int32_t GetLastError() { return 0; };
	const char* GetErrorMsg(int /*error*/) { return NULL; };
	// Connection
	bool Open(char /*name*/[]) { return false; }
	void Close() {}
	bool EnumFirst(uint32_t &/*nDevices*/) { return false; };
	bool EnumNext(char /*name*/[]) { return false; };
	bool Enum(char /*name*/[], uint32_t /*pos*/) { return false; };
	bool Connected() { return false; };
	void SetTimeout(unsigned int /*timeout*/) {};
};

