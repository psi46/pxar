// rpc_io.h

#pragma once

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdint.h>

#include "rpc_error.h"


class CRpcIo
{
protected:
	void Dump(const char *msg, const void *buffer, uint32_t size);
public:
	virtual ~CRpcIo() {}
	virtual void Write(const void *buffer, uint32_t size) = 0;
	virtual void Flush() = 0;
	virtual void Clear() = 0;
	virtual void Read(void *buffer, uint32_t size) = 0;
	virtual void Close() = 0;
};


class CRpcIoNull : public CRpcIo
{
public:
        void Write(const void * /*buffer*/, uint32_t /*size*/) { throw CRpcError(CRpcError::WRITE_ERROR); }
	void Flush() {}
	void Clear() {}
	void Read(void * /*buffer*/, uint32_t /*size*/) { throw CRpcError(CRpcError::READ_ERROR); }
	void Close() {}

};

