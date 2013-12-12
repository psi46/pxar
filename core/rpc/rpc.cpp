// rpc.cpp

#include "rpc.h"

CRpcIoNull RpcIoNull;

void rpcMessage::Create(uint16_t cmd)
{
	m_type = RPC_TYPE_DTB;
	m_cmd = cmd;
	m_size = 0;
	m_pos = 0;
}


void rpcMessage::Send(CRpcIo &rpc_io)
{
	rpc_io.Write(&m_type, 1);
	rpc_io.Write(&m_cmd,  2);
	rpc_io.Write(&m_size, 1);
	if (m_size) rpc_io.Write(m_par, m_size);
}


void rpcMessage::Receive(CRpcIo &rpc_io)
{
	m_pos = 0;
	rpc_io.Read(&m_type, 1);
	if ((m_type & 0xfe) != RPC_TYPE_DTB) throw CRpcError(CRpcError::WRONG_MSG_TYPE);
	if (m_type & 0x01)
	{ // remove unexpected data message from queue
		uint8_t  chn;
		uint16_t size;
		rpc_io.Read(&chn, 1);
		rpc_io.Read(&size, 2);
		rpc_DataSink(rpc_io, size);
		throw CRpcError(CRpcError::NO_CMD_MSG);
	}
	rpc_io.Read(&m_cmd, 2);
	rpc_io.Read(&m_size, 1);
	if (m_size) rpc_io.Read(m_par, m_size);
}


// === data =================================================================

void CDataHeader::RecvHeader(CRpcIo &rpc_io)
{
	rpc_io.Read(&m_type, 1);
	if ((m_type & 0xfe) != RPC_TYPE_DTB) throw CRpcError(CRpcError::WRONG_MSG_TYPE);
	if ((m_type & 0x01) == 0)
	{ // remove unexpected command message from queue
		uint16_t cmd;
		uint8_t size;
		rpc_io.Read(&cmd, 2);
		rpc_io.Read(&size, 1);
		rpc_DataSink(rpc_io, size);
		throw CRpcError(CRpcError::NO_DATA_MSG);
	}
	rpc_io.Read(&m_chn, 1);
	rpc_io.Read(&m_size, 2);
}



void rpc_SendRaw(CRpcIo &rpc_io, uint8_t channel, const void *x, uint16_t size)
{
	uint8_t value = RPC_TYPE_DTB_DATA;
	rpc_io.Write(&value, 1);
	rpc_io.Write(&channel, 1);
	rpc_io.Write(&size, 2);
	if (size) rpc_io.Write(x, size);
//	printf("Send Data [%i]\n", int(size));
}


void rpc_DataSink(CRpcIo &rpc_io, uint16_t size)
{
	if (size == 0) return;
	CBuffer buffer(size);
	rpc_io.Read(&buffer, size);
}


void rpc_Receive(CRpcIo &rpc_io, string &x)
{
	CDataHeader msg;
	msg.RecvHeader(rpc_io);
	x.clear();
	x.reserve(msg.m_size);
	char ch;
	for (uint16_t i=0; i<msg.m_size; i++)
	{
		rpc_io.Read(&ch, 1);
		x.push_back(ch);
	}
}


// === tools ================================================================

void rpc_TranslateCallName(const string &in, string &out)
{
	out.clear();

	unsigned int size = in.rfind('$');
	unsigned int pos = size;
	try
	{
		if (pos == string::npos) throw int(1);
		pos++;
		int i = 0;
		while (pos < in.size())
		{
			int comp = -1;
			if (in[pos] >= '0' && in[pos] <= '4')
			{
				comp = in[pos] - '0';
				pos++; if (pos >= in.size()) throw int(2);
			}

			const char *type;
			switch (in[pos])
			{
				case 'v': type = "void";     break;
				case 'b': type = "bool";     break;
				case 'c': type = "int8_t";   break;
				case 'C': type = "uint8_t";  break;
				case 's': type = "int16_t";  break;
				case 'S': type = "uint16_t"; break;
				case 'i': type = "int32_t";  break;
				case 'I': type = "uint32_t"; break;
				case 'l': type = "int64_t";  break;
				case 'L': type = "uint64_t"; break;
				default: type = "?";
			}

			if (i > 1) out += ", ";

			switch (comp)
			{
				case  0: out += type + '&'; break;
				case  1: out += "vector<"; out += type; out += "> &"; break;
				case  2: out += "vectorR<"; out += type; out += "> &"; break;
				case  3: out += "string &"; break;
				case  4: out += "stringR &"; break;
				default: out += type;
			}

			if (i == 0)
			{
				out += ' ';
				out += in.substr(0, size);
				out += '(';
			}

			pos++; i++;
		}
		out += ");";
	}
	catch (int) { out = in; }
}
