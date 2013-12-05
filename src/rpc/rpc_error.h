// rpc_error.h

#pragma once

#include <stdio.h>
#include <string>


class CRpcError
{
public:
	enum errorId
	{
		OK,
		TIMEOUT,
		WRITE_ERROR,
		READ_ERROR,
		READ_TIMEOUT,
		MSG_UNKNOWN,
		WRONG_MSG_TYPE,
		WRONG_DATA_SIZE,
		ATB_MSG,
		WRONG_CGRP,
		WRONG_CMD,
		CMD_PAR_SIZE,
		NO_DATA_MSG,
		NO_CMD_MSG,
		UNKNOWN_CMD,
		UNDEF
	} error;
	int functionId;
	CRpcError() : error(CRpcError::OK), functionId(-1) {}
	CRpcError(errorId e) : error(e) {}
	void SetFunction(unsigned int cmdId) { functionId = cmdId; }
	const char *GetMsg();
	void What();
};
