// rpc_error.cpp

#include "rpc_error.h"
#include "rpc_calls.h"


const char *CRpcError::GetMsg()
{
	switch (error)
	{
		case OK:              return "OK";
		case TIMEOUT:         return "TIMEOUT";
		case WRITE_ERROR:     return "WRITE_ERROR";
		case READ_ERROR:      return "READ_ERROR";
		case READ_TIMEOUT:    return "READ_TIMEOUT";
		case MSG_UNKNOWN:     return "MSG_UNKNOWN";
		case WRONG_MSG_TYPE:  return "WRONG_MSG_TYPE";
		case WRONG_DATA_SIZE: return "WRONG_DATA_SIZE";
		case ATB_MSG:         return "ATB_MSG";
		case WRONG_CGRP:      return "WRONG_CGRP";
		case WRONG_CMD:       return "WRONG_CMD";
		case CMD_PAR_SIZE:    return "CMD_PAR_SIZE";
		case NO_DATA_MSG:     return "NO_DATA_MSG";
		case NO_CMD_MSG:      return "NO_CMD_MSG";
		case UNKNOWN_CMD:     return "UNKNOWN_CMD";
		case UNDEF:           return "UNDEF";
	}
	return "?";
}


void CRpcError::What()
{
	if (functionId >= 0)
	{
		std::string fname(CTestboard::rpc_cmdName[functionId]);
		std::string fname_pretty;
		rpc_TranslateCallName(fname, fname_pretty);
		printf("\n%s\nERROR: %s\n", fname_pretty.c_str(), GetMsg());
	}
	else
		printf("ERROR: %s\n", GetMsg());
}
