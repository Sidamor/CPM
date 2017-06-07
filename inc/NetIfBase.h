#ifndef __NET_IF_BASE__
#define __NET_IF_BASE__
#include <sys/socket.h>
#include <netinet/in.h>
#include <tr1/unordered_map>

#include <string>

#include "Shared.h"
#include "ClsUdp.h"
#include "ClsTcp.h"
#include "MsgMgrBase.h"
#include "ThreadPool.h"
#include "MemMgmt.h"
#include "RouteMgr.h"
#include "IniFile.h"

using namespace std;

#define DEF_QUEUE_LEN		1024	//Tcp类IF动态生成的消息通道长度
#define MAX_PEND_BUF 		1024
#define MAX_RSP_TIMEOUT	1
#define IF_BASE_SEND		0
#define IF_BASE_RCV			1

#define MM_IF_ALLOC()   MM_ALLOC(m_info.unMemHdl)
#define MM_IF_FREE(p)   MM_FREE(p)

//接口的状态
typedef enum{
	IF_STA_UP = 0,
	IF_STA_DOWN,
	IF_STA_RECONN,
	IF_STA_LOGON_FAIL,
	IF_STA_MAX
}IFStatus;

class CNetIfBase;
class ClsSvrConnMgr;
typedef void (*LinkDownEvent)(int nIfId);

typedef int (*SRHandler)(CNetIfBase*, char *, int);

//接口的基类 
class CNetIfBase {
public:
	int nrecv,nsend, nresend, nrefail;
	int nrecvvalid, nrecvinvalid;
	int nrecverror, nsenderror;
	ClsSvrConnMgr* pSvrConnMgr;
	//constructor 
	//info参数是确定NetIfBase模块的运行方式，具体说明参见shared.h
	//hdl是连接断开
	CNetIfBase(ModIfInfo& info, LinkDownEvent hdl = NULL);
	//destructor
	virtual ~CNetIfBase(void);
	//使用前必须调用的方法，判断是否成功
	bool Initialize(void);
	bool Terminate();
	//判断是否已经初始化
	bool IsInitialized(void) {return m_bIsInitialized;}
	//得到当前使用的socketId
	int GetSockId(void);
	//获得接口号
	int GetIfId(void) {return m_nIfId;}
	// for OMS purpose
	//设定响应时间
	bool SetRspTimeout(const  TKU32 unSec = MAX_RSP_TIMEOUT);
	//设定未响应的消息缓存大小
	bool SetPendBuf(const TKU32 unSize = MAX_PEND_BUF);
	// 增加一个客户连接
	bool SetClient(int socketid);

	void GetTcpClientInfo(char* szdst )
	{
		char *pdst = szdst;
		//char szBuf[1024] = {0};
		printf("Client Info:\n");
		for (int i = 0; i < (int)COMM_TCPSRV_MAXClIENT; ++i)
		{
			if (m_clients[i].socketid > 0)
			{
				//printf("=================\n");
				//ClsTCPClientSocket *m_Sock = (ClsTCPClientSocket*)m_clients[i].socketid;
				//m_Sock->GetPeer(szBuf, sizeof(szBuf));
				//printf("=================%s\n",szBuf);
				sprintf(pdst,"%d,%d,%d;", m_clients[i].socketid, m_clients[i].blFirst, m_clients[i].blValid);
				pdst += strlen(pdst);
				printf("SocketID [%d]\tblFirst [%s]\tblValid [%s]\n", m_clients[i].socketid, m_clients[i].blFirst?"true":"false", m_clients[i].blValid?"true":"false");
			}
		}
	}
	
	bool SetTcpClientInfoStatus(int ClientId, bool Status, char* pEdId)
	{
		bool flag = true;
		bool rslt = false;
		for (int i = 0; i < (int)COMM_TCPSRV_MAXClIENT; ++i)
	  	{
	   		if (m_clients[i].socketid == ClientId)
	   		{	   		
	   		    m_clients[i].blValid = Status;
	   		    memset(m_clients[i].szEdId, 0, sizeof(m_clients[i].szEdId));
						strncpy(m_clients[i].szEdId, pEdId, sizeof(m_clients[i].szEdId));
	   		    flag = false;
	   		    rslt = true;
	   		    //printf("Status of client [%d] has been changed [%s]!\n", ClientId, Status?"true":"false");
	   		    break;
	   		}
	  	}
	  	if(flag)
	  	{
	  		printf("ClientId [%d] doesn't exist!\n", ClientId);
	  	}
	  	return rslt;
	 }

protected:
	//连接断开事件处理方法
	virtual bool OnLinkDown(void);
	virtual bool InitUdp(void);
	virtual bool InitTcp(void);
	virtual bool InitTcpSvr(void);
	virtual bool OnReadUdpIfCfg(void);
	virtual bool OnReadTcpIfCfg(void);
	
	SRHandler SndData;
	SRHandler RcvData;
	
	static int UdpSend(CNetIfBase*, char *buf, int len) ;
	static int UdpReceive(CNetIfBase*, char *buf, int len);
	static int TcpSend(CNetIfBase*, char *buf, int len);
	static int TcpReceive(CNetIfBase*, char *buf, int len);

	
	bool Run(void);
	static void *pThrdTSender(void *);
	static void *pThrdReceiver(void *);
   	static void *pThrdPendReqHandler(void *);
	virtual bool PendReqHandler(void);
	virtual bool Sender(void);
	virtual bool Receiver(void);
	virtual bool PendReqHandlerSvr(void);
	virtual bool SenderSvr(void);
	virtual bool ReceiverSvr(void);
    bool SetNetIfHash(char * msg);
	bool SetNetIfHashTcpSvr(char * pMsg);

	TKU32 recvMsgSeq;
	virtual bool udpPendReqHandler(void);
	virtual bool udpSender(void);
	virtual bool udpReceiver(void);
    bool udpSetNetIfHash(char * msg);
	
	int OnEncodeIf(void* pMsg, char* pBuf);
	virtual CodecType OnDecodeIf(void* pMsg, void* pBuf, void* pRespBuf, int &RespLen, int& nUsed);
	virtual bool NetIfErr(TKU32 unTsn, int Errno, void* param=NULL);
	int CanRead(int& socketid, int ntimout);
	
	
public:	
	ModIfInfo m_info ;

	TKU32 m_unMsgSeq;
	char m_szLocalAddr[20];
	tpport_t m_unLocalPort;
	char m_szRemoteAddr[20];
	tpport_t m_unRemotePort;
	TKU32 m_nTimeout;

private:
	time_t m_downTime;	//上次网络故障时间
	
	pthread_mutex_t m_mtxHashLock;
	IFStatus m_Status;
	
	sockaddr *m_peer;
	int m_nPeerSize;

	TKU32 m_unToSelf;
	TKU32 m_unToCtrl;

	int m_nIfId;
	LinkDownEvent CallbackLinkDown; 
//Hash表结构
    typedef struct {
    	BYTE btType;
    	TKU32 unTsn;
    	int lIndex;
    	TKU32 btMsgCode;
    	time_t TimeStamp;
    	unsigned short TimeStampms;
    	int iStatus;
		TKU32 unRspTnlId;
		bool isResp;
		void* pMsg;  // 发出消息
		int nReSend; // 当重发时的次数
    }_NET_IF_TIMEOUT;
    

	struct HASHER
	{
		size_t operator()(const int &s) const
		{
			return s;
		}
	};

	struct EQUALFUNC
	{
		bool operator()(const int &a, const int &b) const
		{
			return a==b;
		}
	};
    typedef tr1::unordered_map<int, _NET_IF_TIMEOUT, HASHER, EQUALFUNC> _NET_IF_HASH;

    _NET_IF_TIMEOUT val;

    _NET_IF_HASH htNetIfHash;
    _NET_IF_HASH htRecvNetIfHash;

    _NET_IF_HASH::iterator it;


	ClsSocket* m_pSock;
	unsigned int m_iTestSta;
	struct CLIENTINFO
	{
		int socketid;
		bool blValid;
  		bool blFirst;
  		char szEdId[20];
		unsigned int nTestSta;
		TKU32 nRcvPos;
		char cBuff[COMM_RECV_MAXBUF];
		_NET_IF_HASH htNetIfHash;
		TKU32 nMsgSeq;
		pthread_mutex_t m_Lock;
		CLIENTINFO()
		{
			socketid = -1;
			blValid = false;
  			blFirst = true;
			pthread_mutex_init(&m_Lock, NULL);
			Reset();
		}

		void Reset()
		{
			if (socketid>0)
			{
				close(socketid);
			}
			nMsgSeq = 1;
			socketid = -1;
			blValid = false;
  		blFirst = true;
			nTestSta = 0;
			nRcvPos = 0;
			memset(cBuff, 0, sizeof(cBuff));
			htNetIfHash.clear();
		}
	} m_clients[COMM_TCPSRV_MAXClIENT];
	

	pthread_t m_RcvrTID;
	pthread_t m_SndrTID;
	pthread_t m_PendTID;
	
	pthread_mutex_t m_Lock;
	pthread_mutex_t m_recvLock;
	
	bool m_bIsInitialized;
};

#endif
