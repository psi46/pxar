// rpc.h

#pragma once

#include <string>
#include <vector>
#include <stdint.h>

#include <unistd.h>

#include "rpc_io.h"
#include "rpc_error.h"

#ifndef RPC_PROFILING
#define RPC_PROFILING
#endif

#ifdef RPC_MULTITHREADING
#include <boost/thread.hpp>
#define RPC_THREAD boost::mutex m_sync;
#define RPC_THREAD_LOCK boost::lock_guard<boost::mutex> lock(m_sync);
#define RPC_THREAD_UNLOCK
#else
#define RPC_THREAD
#define RPC_THREAD_LOCK
#define RPC_THREAD_UNLOCK
#endif

using namespace std;

#define RPC_TYPE_ATB      0x8F
#define RPC_TYPE_DTB      0xC0
#define RPC_TYPE_DTB_DATA 0xC1

extern const char rpc_timestamp[];

#define RPC_DEFS \
	CRpcIo *rpc_io; \
	static const char rpc_timestamp[]; \
	static const unsigned int rpc_cmdListSize; \
	static const char *rpc_cmdName[]; \
	int *rpc_cmdId; \
	void rpc_Clear() { rpc_cmdId[0] = 0; rpc_cmdId[1] = 1; for ( unsigned int i=2; i<rpc_cmdListSize; i++) rpc_cmdId[i] = -1; } \
	void rpc_Connect(CRpcIo &port) { rpc_io = &port; rpc_Clear(); } \
	uint16_t rpc_GetCallId(uint16_t x) \
	{ \
		int id = rpc_cmdId[x]; \
		if (id >= 0) return id; \
		string name(rpc_cmdName[x]); \
		rpc_cmdId[x] = id = GetRpcCallId(name); \
		if (id >= 0) return id; \
		throw CRpcError(CRpcError::UNKNOWN_CMD); \
	} \
	friend class CRpcError;

#define RPC_INIT rpc_io = &RpcIoNull; rpc_cmdId = new int[rpc_cmdListSize]; rpc_Clear();

#define RPC_EXIT delete[] rpc_cmdId;

#define RPC_EXPORT

extern CRpcIoNull RpcIoNull;



class CBuffer
{
	uint8_t *p;
public:
	CBuffer(uint16_t size) { p = new uint8_t[size]; }
	~CBuffer() { delete[] p; }
	uint8_t* operator&() { return p; }
};


// === message ==============================================================

class rpcMessage
{
	uint8_t m_pos;

	uint8_t  m_type;
	uint16_t m_cmd;
	uint8_t  m_size;
	uint8_t  m_par[256];
public:
	uint16_t GetCmd() { return m_cmd; }

	uint16_t GetCheckedCmd(uint16_t cmdCnt)
	{ if (m_cmd < cmdCnt) return m_cmd; throw CRpcError(CRpcError::UNKNOWN_CMD); }

	void Create(uint16_t cmd);
	void Put_INT8(int8_t x) { m_par[m_pos++] = int8_t(x); m_size++; }
	void Put_UINT8(uint8_t x) { m_par[m_pos++] = x; m_size++; }
	void Put_BOOL(bool x) { Put_UINT8(x ? 1 : 0); }
	void Put_INT16(int16_t x) { Put_UINT8(uint8_t(x)); Put_UINT8(uint8_t(x>>8)); }
	void Put_UINT16(uint16_t x) { Put_UINT8(uint8_t(x)); Put_UINT8(uint8_t(x>>8)); }
	void Put_INT32(int32_t x) { Put_UINT16(uint16_t(x)); Put_UINT16(uint16_t(x>>16)); }
	void Put_UINT32(int32_t x) { Put_UINT16(uint16_t(x)); Put_UINT16(uint16_t(x>>16)); }
	void Put_INT64(int64_t x) { Put_UINT32(uint32_t(x)); Put_UINT32(uint32_t(x>>32)); }
	void Put_UINT64(uint64_t x) { Put_UINT32(uint32_t(x)); Put_UINT32(uint32_t(x>>32)); }

	void Send(CRpcIo &rpc_io);
	void Receive(CRpcIo &rpc_io);
	void Check(uint16_t cmd, uint8_t /*size*/)
	{
		if (m_cmd != cmd) throw CRpcError(CRpcError::UNKNOWN_CMD);
		if (m_size != m_size) throw CRpcError(CRpcError::CMD_PAR_SIZE);
		return;
	}
	void CheckSize(uint8_t size) { if (m_size != size) throw CRpcError(CRpcError::CMD_PAR_SIZE); }

	int8_t Get_INT8() { return int8_t(m_par[m_pos++]); }
	uint8_t Get_UINT8() { return uint8_t(m_par[m_pos++]); }
	bool Get_BOOL() { return Get_UINT8() != 0; }
	int16_t Get_INT16() { int16_t x = Get_UINT8(); x += (uint16_t)Get_UINT8() << 8; return x; }
	uint16_t Get_UINT16() { uint16_t x = Get_UINT8(); x += (uint16_t)Get_UINT8() << 8; return x; }
	int32_t Get_INT32() { int32_t x = Get_UINT16(); x += (uint32_t)Get_UINT16() << 16; return x; }
	uint32_t Get_UINT32() { uint32_t x = Get_UINT16(); x += (uint32_t)Get_UINT16() << 16; return x; }
 	int64_t Get_INT64() { int64_t x = Get_UINT32(); x += (uint64_t)Get_UINT32() << 32; return x; }
	uint64_t Get_UINT64() { uint64_t x = Get_UINT32(); x = (uint64_t)Get_UINT32() << 32; return x; }
};


// === data =================================================================

#define vectorR vector
#define stringR string


class CDataHeader
{
public:
	uint8_t m_type;
	uint8_t m_chn;
	uint16_t m_size;

	void RecvHeader(CRpcIo &rpc_io);
	void RecvRaw(CRpcIo &rpc_io, void *x)
	{ if (m_size) rpc_io.Read(x, m_size); }
};

void rpc_SendRaw(CRpcIo &rpc_io, uint8_t channel, const void *x, uint16_t size);

void rpc_DataSink(CRpcIo &rpc_io, uint16_t size);


template <class T>
inline void rpc_Send(CRpcIo &rpc_io, const vector<T> &x)
{
	rpc_SendRaw(rpc_io, 0, &(x[0]), sizeof(T)*x.size());
}


template <class T>
void rpc_Receive(CRpcIo &rpc_io, vector<T> &x)
{
	CDataHeader msg;
	msg.RecvHeader(rpc_io);
	if ((msg.m_size % sizeof(T)) != 0)
	{
		rpc_DataSink(rpc_io, msg.m_size);
		throw CRpcError(CRpcError::WRONG_DATA_SIZE);
	}
	x.assign(msg.m_size/sizeof(T), 0);
	if (x.size() != 0) rpc_io.Read(&(x[0]), msg.m_size);
}


inline void rpc_Send(CRpcIo &rpc_io, const string &x)
{
	rpc_SendRaw(rpc_io, 0, x.c_str(), x.length());
}


void rpc_Receive(CRpcIo &rpc_io, string &x);


// === tools ================================================================

void rpc_TranslateCallName(const string &in, string &out);
