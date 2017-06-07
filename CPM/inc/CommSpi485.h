#ifndef _CENTER_CTL_H_
#define _CENTER_CTL_H_

#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <hashtable.h>
#include "Define.h"
using namespace std;


#pragma pack(1)



//动作数据格式
typedef struct _ACT_MSG                 
{
	char cmd;					//0,联动指令；1,嵌入式Web指令；2,平台指令
    char r_id[4];               //需要返回的下发动作编号
	char sid[9];
    int  sSn;                  //读取指令sn
	char did[9];
	char seprator[11];
	char sn[10];
	char actValue[1024];
}ACT_MSG, *PACT_MSG;


typedef struct _POLL_INDEX_NODE
{
	char Id[15];                        //设备编号(6位) + 轮询SN(4位)
} POLL_INDEX_NODE, *PPOLL_INDEX_NODE;

#pragma pack()

/****************************轮询哈希表定义*********************************/
typedef  size_t (*HASHER_FUNC_POLL_INDEX)(POLL_INDEX_NODE key);
typedef  POLL_INDEX_NODE (* EXTRACTOR_FUNC_POLL_INDEX)(POLL_INDEX_NODE val); 
typedef  bool (*EQUAL_FUNC_POLL_INDEX)(POLL_INDEX_NODE key1, POLL_INDEX_NODE key2);
typedef  hashtable<POLL_INDEX_NODE, POLL_INDEX_NODE, HASHER_FUNC_POLL_INDEX, EXTRACTOR_FUNC_POLL_INDEX, EQUAL_FUNC_POLL_INDEX> HashedDevicePollIndex;

/****************************动作执行回应表定义*********************************/
typedef  size_t (*HASHER_FUNC_ACTION_RESP)(ACTION_MSG_WAIT_RESP_HASH key);
typedef  ACTION_MSG_WAIT_RESP_HASH (* EXTRACTOR_FUNC_ACTION_RESP)(ACTION_MSG_WAIT_RESP_HASH val); 
typedef  bool (*EQUAL_FUNC_ACTION_RESP)(ACTION_MSG_WAIT_RESP_HASH key1, ACTION_MSG_WAIT_RESP_HASH key2);
typedef  hashtable<ACTION_MSG_WAIT_RESP_HASH, ACTION_MSG_WAIT_RESP_HASH, HASHER_FUNC_ACTION_RESP, EXTRACTOR_FUNC_ACTION_RESP, EQUAL_FUNC_ACTION_RESP> HashedDeviceInfoActionResp;


class CCenterCtl
{
public:
	CCenterCtl(ModInfo& info);
	virtual ~CCenterCtl(void);

	virtual bool Initialize(TKU32 unQueryHandle, TKU32 unHandleCtrl);	

private:	
	pthread_t m_MainThrdHandle;

	bool InitDeviceInfo();

	static void *MainThrd(void* arg);			//主线程
	void MainThrdProc(void);					
	bool Terminate();

	int SendToGsm(char* srcDev, char* srcSn,  char *deviceId, char* dstActionSn, char *smsValue, int type, TKU32 Seq);
	void InsterIntoAlarmInfo(ACTION_MSG actionMsg, int actionRet);

private://hashtable 相关
	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;


	HashedDevicePollIndex* m_HashTable;         //轮询哈希表
	static int InitDeviceInfoCallBack(void *data, int n_column, char **column_value, char **column_name);    //查询时的回调函数
	int InitDeviceInfoCallBackProc(int n_column, char **column_value, char **column_name);


public:
	void CheckOnLineStatus(PDEVICE_INFO_POLL p_node, int nLen);
 
	bool CheckTime(struct tm *st_tmNowTime, DEVICE_INFO_POLL device_node);


	int PollProc(PDEVICE_INFO_POLL device_node, unsigned char* respBuf, int& respLen);
	void DoAction(ACTION_MSG actionMsg);
	int ActionProc(PDEVICE_INFO_ACT act_node, ACTION_MSG& actionMsg);

	int CommOpenPoll(PDEVICE_INFO_POLL devInfo);
    int CommOpenAct(PDEVICE_INFO_ACT devInfo);
	//int CommSet(unsigned int BaudRate);
	int CommMsgSend(BYTE* msgSendBuf, BYTE sendLen, int baudrate);
	int CommMsgRecive(BYTE* msgRecivBuf, int baudrate);
	int CommClose();
	int GetCommId(char *up_channel, char* CommId);

public:
	bool InitDeviceQueryInfo();
	static int InitDeviceIndexCallBack(void* data, PDEVICE_INFO_POLL pNode);
	int InitDeviceIndexCallBackProc(PDEVICE_INFO_POLL pNode);   //轮询表初始化

public:
	bool delNode(char* pDeviceAttrId);
	bool delNodeByDeviceId(char* pDeviceId);
	bool addNode(char* pDeviceAttrId);

public:
	//动作执行回应表相关函数
	bool ActionRespTableUpdate(TKU32 Seq, int actionRet);
	bool ActionRespTableInsert(ACTION_MSG actionMsg, TKU32& Key);
	bool ActionRespTableGetAndDelete(ACTION_MSG_WAIT_RESP_HASH& hash_node);
	void SendToActionSource(ACTION_MSG actionMsg, int actionRet);

private:

	class LockActionResp
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		LockActionResp() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CLockActionResp;

	HashedDeviceInfoActionResp* m_HashTableActResp;      //动作回应哈希表
	TKU32 m_ActRespKey;
	TKU32 GetActionRespTableKey()
	{
		if (m_ActRespKey < 0xffffffff)
			m_ActRespKey++;
		else
			m_ActRespKey = 0;
		return m_ActRespKey;
	}

private:

	termios termios_new;
	int m_commFd;                     //串口状态
	ModInfo m_modeInfo;              //串口模块
	TKU32 m_unHandleQuery;              //任务队列句柄
	TKU32 m_unHandleCtrl;              //任务队列句柄 

	float m_EnumADSW[MAX_INTERFACE_COUNT];

	bool m_bIndexStatusChange;
};

#endif
