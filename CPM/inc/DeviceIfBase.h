#ifndef _DEVICE_IF_BASE_H_
#define _DEVICE_IF_BASE_H_

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

	virtual int Initialize(int iChannelId, TKU32 unHandleCtrl);	
	virtual bool Terminate();

public:				
	virtual int PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& nRespLen) = 0;
	virtual int ActionProc(PDEVICE_INFO_ACT act_node, ACTION_MSG& actionMsg) = 0;
	void QueryAttrProc(DEVICE_INFO_POLL& deviceNode);

private:
	static void *MainThrd(void* arg);			//主线程
	void MainThrdProc(void);	

	class LockQuery
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockQuery() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CLockQuery;

private:
	bool CheckTime(struct tm *st_tmNowTime, DEVICE_INFO_POLL device_node);
	int CheckOnLineStatus(PDEVICE_INFO_POLL p_node, bool bStatus, QUERY_INFO_CPM stQueryResult);

private:
	void DoAction(ACTION_MSG actionMsg);

protected:
	int			m_iChannelId;					//通道号
	char		m_szCurDeviceAttrId[DEVICE_ATTR_ID_LEN_TEMP];
	TKU32		m_unHandleCtrl;              //任务队列句柄 

private:
	pthread_t	m_MainThrdHandle;
};

#endif
