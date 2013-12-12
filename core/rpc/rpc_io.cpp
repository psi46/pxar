// rpc_io.cpp

#include "rpc_io.h"
#include <stdio.h>

void CRpcIo::Dump(const char *msg, const void *buffer, uint32_t size)
{
	printf("%s(", msg);
	for (uint32_t i=0; i<size; i++)
		printf("%02X ", int(((unsigned char *)buffer)[i]));
	printf(")\n");
}
