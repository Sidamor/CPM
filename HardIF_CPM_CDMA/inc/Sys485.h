#ifndef _SYS_485_H_
#define _SYS_485_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "Define.h"

int OpenComm(int iChannelId, int iBaudRate, int iDataBytes, char cCheckMode, int iStopBit, int iReadTimeOut, int& nHandle);
int SendCommMsg(int nHandle, BYTE* msgSendBuf, int sendLen);
int RecvCommMsg(int nHandle, BYTE* msgRecvBuf, int iNeedLen, int iTimeOut);	
void CloseComm(int iChannelId);


#endif
