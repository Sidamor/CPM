#ifndef _MSG_CTL_
#define _MSG_CTL_

#include <tr1/unordered_map>
#include "Shared.h"
#include "Define.h"
#include "Init.h"
#include "SeqRscMgr.h"
#include "MsgCtlBase.h"
#include "Md5.h"
#include "Apm.h"
#include "NetApp.h"
using namespace std;
#pragma  pack(1)
typedef struct _MSG_ACTION_HASH
{
	TKU32   Id;				 //编号
	TKU32   unMsgLen;        //长度
	TKU32   unMsgCode;       //消息码
	TKU32   unStatus;        //消息执行状态
	TKU32   unMsgSeq;        //消息序列号	
	TKU32   unRspTnlId;      //返回消息使用的通道号

	char	szReserve[21];   //保留字
	char	szStatus[5];	 //命令发送状态
	char	szDeal[5];		 //处理指令

	time_t	act_time;
	int		source;		     //来源,参见枚举ACTION_SOURCE
}MSG_ACTION_HASH, *PMSG_ACTION_HASH;

typedef  tr1::unordered_map<TKU32, MSG_ACTION_HASH> HashedActInfo;

typedef struct _APM_INFO 
{
	int iApmId;
	char szApmPwd[17];
}APM_INFO, *PAPM_INFO;

#pragma pack()
//与BOA网页通讯的TCPSVR类
class CBOAMsgCtlSvr : CMsgCtlBase
{
protected:
	static bool SVR_Submit_Func (CBOAMsgCtlSvr *_this, char *msg);
	static bool SVR_Terminate_Func (CBOAMsgCtlSvr *_this, char *msg);

public:
	virtual bool Initialize(TKU32 unInputMask, const char *szCtlName);
    bool doSVR_Submit_Func(char *msg);
    bool doSVR_Terminate_Func (char *msg);
	bool doSVR_Err_Func (char *msg);
	bool Dispatch(CBOAMsgCtlSvr *_this, char *pMsg, char *pBuf, AppInfo *stAppInfo, BYTE Mod);
	void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len);

    bool addNode(MSG_ACTION_HASH hash_node);
//    bool GetHashData(MSG_ACTION_HASH& hash_node);
	bool GetAndDeleteHashData(MSG_ACTION_HASH& hash_node);

//	bool GetSocketId (char* channelId, TKU32& unClientId);
	TKU32 GetSeq()
	{
		if (0xffffffff == m_Seq++)
		{
			m_Seq = 0;
		}
		return m_Seq;
	}

public:
	//网络型传感器lsd
	//bool InitApmHashData(char* sqlData);
	//static int InitApmHashCallback(void *data, int n_column, char **column_value, char **column_name);
	//int InitApmHashCallbackProc(int n_column, char **column_value, char **column_name);

private://hashtable 相关

	HashedActInfo*  m_ptrActInfoTable;
    class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;
private:
    static void *pActCtrlThrd(void* arg);
    void MsgActCtrl();
    //HashedInfo* m_pSocketHashTable;           //socket连接客户端哈希表
	TKU32 ulHandleTcpSvr;
    TKU32 ulHandleActCtrl;
    int m_id;
	TKU32 m_Seq;
	int m_MaxApmCount;
};
//与网络型传感器TERMINAL 通讯的TCPSVR类
class CTMNMsgCtlSvr : CMsgCtlBase
{
protected:
	static bool SVR_Submit_Func (CTMNMsgCtlSvr *_this, char *msg);
	static bool SVR_Terminal_Sub_Func (CTMNMsgCtlSvr *_this, char *msg);
	static bool SVR_Connect_Func (CTMNMsgCtlSvr *_this, char *msg);
	static bool SVR_Terminate_Func (CTMNMsgCtlSvr *_this, char *msg);
	static bool SVR_Err_Func (CTMNMsgCtlSvr *_this, char *msg);

public:
	virtual bool Initialize(TKU32 unInputMask, const char *szCtlName);
	bool doSVR_Submit_Func(char *msg);
	bool doSVR_Terminal_Sub_Func(char *msg);
	bool doSVR_Connect_Func (char *msg);
	bool doSVR_Terminate_Func (char *msg);
	bool doSVR_Err_Func (char *msg);
	bool ApmValid(char* Reserve, char* Deal, char* D_Status, char *szApmId, char *szPwd, BYTE *btAuth);
	bool Dispatch(CTMNMsgCtlSvr *_this, char *pMsg, char *pBuf, AppInfo *stAppInfo, BYTE Mod);
	void MD5(unsigned char *szBuf, unsigned char *src, unsigned int len);

//	bool addNode(MSG_ACTION_HASH hash_node);
//	bool GetHashData(MSG_ACTION_HASH& hash_node);
//	bool GetAndDeleteHashData(MSG_ACTION_HASH& hash_node);

	bool GetSocketId (char* channelId, TKU32& unClientId);
	TKU32 GetSeq()
	{
		if (0xffffffff == m_Seq++)
		{
			m_Seq = 0;
		}
		return m_Seq;
	}

//public:
	//网络型传感器lsd
	//bool InitApmHashData(char* sqlData);
	//static int InitApmHashCallback(void *data, int n_column, char **column_value, char **column_name);
	//int InitApmHashCallbackProc(int n_column, char **column_value, char **column_name);

/*private://hashtable 相关
	typedef  tr1::unordered_map<TKU32, MSG_ACTION_HASH> HashedActInfo;
	HashedActInfo*  m_ptrActInfoTable;
	class Lock
	{
	private:
		pthread_mutex_t	 m_lock;
	public:
		Lock() { pthread_mutex_init(&m_lock, NULL); }
		bool lock() { return pthread_mutex_lock(&m_lock)==0; }
		bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
	}CTblLock;*/
private:
	static void *pActCtrlThrd(void* arg);
	void MsgActCtrl();
	//HashedInfo* m_pSocketHashTable;           //socket连接客户端哈希表
	TKU32 ulHandleTcpSvr;
	TKU32 ulHandleActCtrl;
	int m_id;
	TKU32 m_Seq;
	int m_MaxApmCount;
};
//与上级平台通讯的TCP CLIENT类
class CMsgCtlClient : public CMsgCtlBase
{
protected:
	static bool NetFuncSubmit(CMsgCtlClient *_this, char *msg);
	static bool NetFuncDeliver(CMsgCtlClient *_this, char *msg);
	static bool NetErr(CMsgCtlClient *_this, char *msg);
    

public:
	~CMsgCtlClient(void);
	virtual bool Initialize(TKU32 unInputMask,const char *szCtlName);


private:
	TKU32 ulHandleHubNet;
};

#endif
