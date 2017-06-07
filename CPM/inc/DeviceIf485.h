#ifndef _DEVICE_IF_485_H_
#define _DEVICE_IF_485_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "Define.h"

class CDeviceIf485 : public CDeviceIfBase
{
public:
	CDeviceIf485();
	virtual ~CDeviceIf485(void);

	virtual int Initialize(int iChannelId, TKU32 unHandleCtrl);	
	virtual bool Terminate();

public:
	virtual int PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen);
	virtual int ActionProc(PDEVICE_INFO_ACT act_node, ACTION_MSG& actionMsg);
};

#endif
