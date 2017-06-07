#ifndef _DEVICE_IF_AD_H_
#define _DEVICE_IF_AD_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "DeviceIfBase.h"
#include "Define.h"

class CDeviceIfAD : public CDeviceIfBase
{
public:
	CDeviceIfAD();
	virtual ~CDeviceIfAD(void);

	virtual int Initialize(int iChannelId, TKU32 unHandleCtrl);	
	virtual bool Terminate();

private:
	virtual int PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen);
	virtual int ActionProc(PDEVICE_INFO_ACT act_node, ACTION_MSG& actionMsg);

};

#endif
