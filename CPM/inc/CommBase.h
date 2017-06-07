#ifndef _COMM_BASE_H_
#define _COMM_BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "Define.h"


class CDeviceIfBase
{
public:
	CDeviceIfBase();
	virtual ~CDeviceIfBase(void);

	virtual bool Initialize(int iChannelId, TKU32 unHandleCtrl);	

public:
	virtual int OpenComm(int iChannelId, int iBaudRate, int iDataBytes, char cCheckMode, int iStopBit, int iReadTimeOut) = 0;
	virtual int SendCommMsg(int nHandle, BYTE* msgSendBuf, BYTE sendLen) = 0;
	virtual int RecvCommMsg(int nHandle, BYTE* msgRecvBuf, BYTE iNeedLen) = 0;	
	virtual int CloseComm(int iChannelId) = 0;

private:	
	pthread_t m_MainThrdHandle;

	bool InitDeviceInfo();

	static void *MainThrd(void* arg);			//主线程
	void MainThrdProc(void);					
	bool Terminate();

	void InsterIntoAlarmInfo(ACTION_MSG actionMsg, int actionRet);


private:
	int CheckOnLineStatus(PDEVICE_INFO_POLL p_node, bool bStatus);

	bool CheckTime(struct tm *st_tmNowTime, DEVICE_INFO_POLL device_node);

	bool PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen);
	void DoAction(ACTION_MSG actionMsg);
	int ActionProc(PDEVICE_INFO_ACT act_node, ACTION_MSG& actionMsg);

public:
	//动作执行回应表相关函数
	void SendToActionSource(ACTION_MSG actionMsg, int actionRet);

private:
	int			m_iChannelId;					//通道号
	char		m_szCurDeviceAttrId[15];

	TKU32 m_unHandleCtrl;              //任务队列句柄 
};

#endif
