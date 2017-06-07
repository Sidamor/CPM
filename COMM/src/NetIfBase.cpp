#include <sys/time.h>
#include <sys/timeb.h>
#include <errno.h>
#include "NetIfBase.h"
#include "ConnMgr.h"  
//#include <curses.h>
 
extern  CRouteMgr g_RouteMgr;
// UdpPenderHandle  insertMsg remarked
// UdpRecevier    remarked

int FreeIfMsg(QUEUEELEMENT pMsg)
{
	MM_IF_FREE(pMsg);
	return 0;
}

  
CNetIfBase::CNetIfBase(ModIfInfo& info, 
					   LinkDownEvent hdl): m_nIfId(info.nTcpSocket), CallbackLinkDown(hdl)
{
	TRACE(CNetIfBase::CNetIfBase);
	nrecv = nsend = 0;
	nrecverror=nsenderror=0;
	nresend = nrefail = 0;
	nrecvvalid=nrecvinvalid=0;
	recvMsgSeq = 0;
	m_info = info;
//	DBG(("M_info=%s\n",m_info.szIniFileName));
	memset(m_szLocalAddr, 0, sizeof(m_szLocalAddr));
	m_unLocalPort = 0;
	memset(m_szRemoteAddr, 0, sizeof(m_szRemoteAddr));
	m_unRemotePort = 0;
	m_nTimeout = 30;
	m_unMsgSeq = 1;
	m_iTestSta = 0;
	m_downTime = 0;
	m_Status = IF_STA_DOWN;
	m_peer = NULL;
	m_nPeerSize = sizeof(sockaddr);
	m_unToSelf = 0;
	m_unToCtrl = 0;
	RcvData = NULL;
	SndData = NULL;
	m_pSock =   NULL;
	pthread_mutex_init(&m_Lock, NULL);
	pthread_mutex_init(&m_recvLock, NULL);
	pSvrConnMgr = NULL;
	m_RcvrTID =	m_SndrTID = m_PendTID = 0xFFFF;
	m_bIsInitialized = false;
}   

//destructor
CNetIfBase::~CNetIfBase(void)
{
	TRACE(CNetIfBase::~CNetIfBase);
	//DBG(("begin ~CNetIfBase\n"));
	Terminate();
	//DBG(("end ~CNetIfBase\n"));
}
bool CNetIfBase::Terminate()
{
	TRACE(CNetIfBase::Terminate());
	//DBG(("begin ~CNetIfBase\n"));
	if(0xFFFF != m_RcvrTID)
	{
		ThreadPool::Instance()->SetThreadMode(m_RcvrTID, false);
		if (pthread_cancel(m_RcvrTID)!=0)
			;//DBGLINE;
	}
	//DBG(("m_RcvrTID\n"));
	if(0xFFFF != m_SndrTID)
	{
		ThreadPool::Instance()->SetThreadMode(m_SndrTID, false);
		if (pthread_cancel(m_SndrTID)!=0)
			;//DBGLINE;
	}
	//DBG(("m_SndrTID\n"));
	if(0xFFFF != m_PendTID)
	{
		ThreadPool::Instance()->SetThreadMode(m_PendTID, false);
		if (pthread_cancel(m_PendTID)!=0)
			;//DBGLINE;
	}
	//DBG(("m_PendTID\n"));
	if (m_info.npType!=NP_TCPSVR)
	{
		if (m_pSock)
			delete m_pSock;
		if (m_info.bTcpIsShareMode)
			g_MsgMgr.FreeTunnel((int)m_unToSelf);
	}
	if(NP_TCP == m_info.npType)
	{
		if(m_info.bTcpIsSvrConn)
		{
			g_RouteMgr.DelRouteI2A(m_nIfId);
		}
		else
		{
			ClsTCPClientSocket* pSock = (ClsTCPClientSocket*)m_pSock;
			sockaddr_in* peer = (sockaddr_in*) pSock->GetPeer();
			g_RouteMgr.DelRouteA2I(peer->sin_addr, peer->sin_port);
		}
	}
	//DBG(("end ~CNetIfBase\n"));	
	return true;
}

bool CNetIfBase::OnReadUdpIfCfg(void)
{
	TRACE(CNetIfBase::OnReadUdpIfCfg);
	if (-1 == INI_READ_STRING(m_info, "LocalAddr", m_szLocalAddr)) return false;
	if (-1 == INI_READ_INT(m_info, "LocalPort", m_unLocalPort))	return false;
	if (-1 == INI_READ_STRING(m_info, "RemoteAddr", m_szRemoteAddr)) return false;
	if (-1 == INI_READ_INT(m_info, "RemotePort", m_unRemotePort)) return	false;
	if (-1 == INI_READ_INT(m_info, "TimeOut", m_nTimeout)) return false;

//	DBG(("name = %s, local = %s :	%d,	remote = %s	: %d, timeout =	%d\n", m_info.szModName, m_szLocalAddr,
//		   m_unLocalPort, m_szRemoteAddr, m_unRemotePort, m_nTimeout));
	return true;
}

bool CNetIfBase::OnReadTcpIfCfg(void)
{
	TRACE(CNetIfBase::OnReadTcpIfCfg);
	if (-1 == INI_READ_STRING(m_info, "RemoteAddr", m_szRemoteAddr)) return false;
	if (-1 == INI_READ_INT(m_info, "RemotePort", m_unRemotePort)) return	false;
	if (-1 == INI_READ_INT(m_info, "TimeOut", m_nTimeout)) return false;

//	DBG(("name = %s, remote =	%s : %d, timeout = %d\n", m_info.szModName, 
//		   m_szRemoteAddr, m_unRemotePort, m_nTimeout));
	return true;
}

bool CNetIfBase::InitUdp(void)
{
	TRACE(CNetIfBase::InitUdp);

	assert(m_bIsInitialized == false );
	
	if (false == OnReadUdpIfCfg()) return false;

	ClsUDPSocket* pSock = NULL;
	if (m_pSock) delete m_pSock;
	m_pSock = pSock = new ClsUDPSocket;
	if (!m_pSock) return false;

//	DBG(("000SetServerSocket\n"));
	if (false == pSock->SetServerSocket(m_szLocalAddr, m_unLocalPort)) return false;
//DBG(("aaaSetServerSocket\n"));
	pSock->SetPeer(m_szRemoteAddr, m_unRemotePort);
	pSock->SetCompletion();

	m_peer = (sockaddr*) pSock->GetPeer();
	m_nPeerSize = sizeof(sockaddr);
//DBG(("m_info.self=%d,m_info.ctrl=%d\n",m_info.self, m_info.ctrl));
	TKU32 unMask = COMBINE_MOD(m_info.ctrl, m_info.self);
	if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToSelf))
		return false;

	unMask  = COMBINE_MOD(m_info.self, m_info.ctrl);
	if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToCtrl))
		return false;

	RcvData = (SRHandler)UdpReceive;
	SndData = (SRHandler)UdpSend;

	m_Status = IF_STA_UP;

	return true;    
}
bool CNetIfBase::InitTcpSvr(void)
{
	TRACE(CNetIfBase::InitTcp);

	assert(m_bIsInitialized == false);
	INI_READ_INT(m_info, "TimeOut", m_nTimeout);
	if (m_nTimeout==0)
		m_nTimeout = 30;

	TKU32 unMask = 0;
printf("m_info.self=%d,m_info.ctrl=%d\n",m_info.self, m_info.ctrl);
	unMask = COMBINE_MOD(m_info.self, m_info.ctrl);
	if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToCtrl))
		return false;

	unMask = COMBINE_MOD(m_info.ctrl, m_info.self);
	if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToSelf))
		return false;

	RcvData = (SRHandler)TcpReceive;
	SndData = (SRHandler)TcpSend;
	m_Status = IF_STA_UP;

	return true;
}

bool CNetIfBase::InitTcp(void)
{
	TRACE(CNetIfBase::InitTcp);

	assert(m_bIsInitialized == false);
	
	ClsTCPClientSocket* pSock = NULL;

	if (m_info.bTcpIsSvrConn)
	{
		if (m_pSock) delete m_pSock;
		m_pSock = pSock = new ClsTCPClientSocket(m_info.nTcpSocket );
		if (!m_pSock)
		{
//DBGLINE
			return false;
		}

	} 
	else
	{
		if (false == OnReadTcpIfCfg())
		{
//DBGLINE
			 return false; 
		}
		if (m_pSock) delete m_pSock;
		sleep(2);
		m_pSock = pSock = new ClsTCPClientSocket();
		if (!m_pSock)
		{
//DBGLINE
			return false;
		}
		pSock->Connect(m_szRemoteAddr, m_unRemotePort);
	}

	if (!pSock->IsConnected())
	{
//DBGLINE	
		return false;
	}

	pSock->SetCompletion();

	TKU32 unMask = 0;

	if (m_info.bTcpIsShareMode)		 //共享模式
	{
		sockaddr_in* peer = (sockaddr_in*) pSock->GetPeer();
		if (m_info.nTcpDynTnlSize < 0)
			m_info.nTcpDynTnlSize = DEF_QUEUE_LEN;

		//IF 到CTRL的通道由外部静态生成
		unMask = COMBINE_MOD(m_info.self, m_info.ctrl); 
		if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToCtrl))
		{
//DBGLINE
			return false;
		}

		if (MMR_OK != g_MsgMgr.AllocTunnel((TKU32)0, (TKU32)m_info.nTcpDynTnlSize, (size_t&)m_unToSelf , (FreeMemEvent)FreeIfMsg))
		{
//DBGLINE
			return false;
		}
		if (m_info.bTcpIsSvrConn)
		{
			if (!g_RouteMgr.AddRouteI2A((int)m_unToSelf, peer->sin_addr, peer->sin_port))
			{
//DBGLINE	
			return false;
			}
		}
		else
		{
			if (!g_RouteMgr.AddRouteA2I(peer->sin_addr, peer->sin_port, (int)m_unToSelf))
			{
//DBGLINE	
			return false;
			}		
		}
	} 
	else //点对点模式
	{
		unMask = COMBINE_MOD(m_info.self, m_info.ctrl);
		if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToCtrl))
			return false;

		unMask = COMBINE_MOD(m_info.ctrl, m_info.self);
		if (MMR_OK != g_MsgMgr.Attach(unMask, m_unToSelf))
			return false;
		else
			{
				if(m_info.CallbackLogin)
				{
					if(false == m_info.CallbackLogin(m_pSock))
					{
						return false;
					}
				}
			}
	}

	RcvData = (SRHandler)TcpReceive;
	SndData = (SRHandler)TcpSend;
//	DBG(("m_Status = IF_STA_UP\n"));
	m_Status = IF_STA_UP;
	DBG(("CNetIfBase::Init success\n"));
	return true;
}

bool CNetIfBase::Initialize(void)
{
	TRACE(CNetIfBase::Initialize);
	assert(m_bIsInitialized == false);
	bool bRet = true;

	switch (m_info.npType)
	{
		case NP_UDP:
			bRet = InitUdp();

			break;
	
		case NP_TCP:
			bRet = InitTcp();
			
			if(!m_info.bTcpIsSvrConn && !bRet)
			{
				DBG(("%s 连接失败\n",m_info.szModName));
				m_Status = IF_STA_DOWN;
				bRet = true;
			}
			break;
		case NP_TCPSVR:
			bRet = InitTcpSvr();
			break;
		default:
			bRet = false;
			break;
	}

	if ( bRet)
		return Run();
	else
	{
		return false;
	}
}

bool CNetIfBase::Run(void)
{
	TRACE(CNetIfBase::Run);
	assert(m_bIsInitialized == false);
//	DBG(("toSelf Handle = %d,\t toCtrl Handle = %d\n", (int)m_unToSelf, (int)m_unToCtrl));

	
	char buf[256] = {0};
	
	sprintf(buf, "%s %s", m_info.szModName, "Sender");
//	DBG(("CNetIfBase::Run -- CreateThread\n"));
	if (!ThreadPool::Instance()->CreateThread(buf, &CNetIfBase::pThrdTSender, true,this))
	{
//DBGLINE
		return false;
	}

	sprintf(buf, "%s %s", m_info.szModName, "Receiver");
	if (!ThreadPool::Instance()->CreateThread(buf, &CNetIfBase::pThrdReceiver, true,this))
	{
//DBGLINE
		return false;
	}
 
	DBG(("CNetIfBase::pThrdReceiver creat success\n"));
	sprintf(buf, "%s %s", m_info.szModName, "PendReqHander");
	if (!ThreadPool::Instance()->CreateThread(buf, &CNetIfBase::pThrdPendReqHandler, true,this))
	{
//DBGLINE		
		return false;
	}
	
	m_bIsInitialized = true;


	return true;
}


// for OAM purpose
bool CNetIfBase::SetRspTimeout(const  TKU32 unSec)
{
	TRACE(CNetIfBase::SetRspTimeout);
	m_nTimeout = unSec;
	return true;
}

bool CNetIfBase::SetNetIfHash(char * pMsg)
{
	TRACE(CNetIfBase::SetItemVal);
	_NET_IF_TIMEOUT item;
	
	MsgHdr *pMsgHdr = (MsgHdr*)pMsg;
//lhy	Msg *pTMsg = (Msg *)(pMsg + MSGHDRLEN);
	
	if(	pMsgHdr -> unMsgCode & COMM_RSP)
		return true;
	pMsgHdr -> unMsgSeq = m_unMsgSeq;
//	item.btType = pTMsg -> btType;
//lhy	item.unTsn = pTMsg -> unTsn;
	item.lIndex = pMsgHdr->unMsgSeq;
	item.btMsgCode = pMsgHdr->unMsgCode;
	item.TimeStamp = time(NULL);//pMsgHdr->tTimeStamp;
	item.iStatus = pMsgHdr->unStatus;
	item.unRspTnlId = pMsgHdr->unRspTnlId;
	
	if(m_unMsgSeq < COMM_MAXSEQ)
		m_unMsgSeq ++;
	else
		m_unMsgSeq = 2;
	pthread_mutex_lock(&m_Lock);		
	htNetIfHash.insert(_NET_IF_HASH::value_type(item.lIndex, item));
	pthread_mutex_unlock(&m_Lock);
	return true;
}
bool CNetIfBase::SetNetIfHashTcpSvr(char * pMsg)
{
	MsgHdr *pMsgHdr = (MsgHdr*)pMsg;
//lhy	Msg *pTMsg = (Msg *)(pMsg + MSGHDRLEN);
	if(	pMsgHdr -> unMsgCode & COMM_RSP)
		return true;
	int	socketid = pMsgHdr->unRspTnlId;
	unsigned int i;
	for (i=0; i<COMM_TCPSRV_MAXClIENT; ++i)
		if (m_clients[i].socketid==socketid)
			break;
	if (i>=COMM_TCPSRV_MAXClIENT)
		return false;
	CLIENTINFO& theClient = m_clients[i];
	
	_NET_IF_TIMEOUT item;
	pMsgHdr -> unMsgSeq = theClient.nMsgSeq;
//	item.btType = pTMsg -> btType;
//lhy	item.unTsn = pTMsg -> unTsn;
	item.lIndex = pMsgHdr->unMsgSeq;
	item.btMsgCode = pMsgHdr->unMsgCode;
	item.TimeStamp = time(NULL);//pMsgHdr->tTimeStamp;
	item.iStatus = pMsgHdr->unStatus;
	item.unRspTnlId = pMsgHdr->unRspTnlId;
	
	if(theClient.nMsgSeq < COMM_MAXSEQ)
		theClient.nMsgSeq ++;
	else
		theClient.nMsgSeq = 2;
	if (pMsgHdr->unMsgCode!=COMM_CONNECT && pMsgHdr->unMsgCode!=COMM_TERMINATE)
	{
		pthread_mutex_lock(&theClient.m_Lock);	
		theClient.htNetIfHash.insert(_NET_IF_HASH::value_type(item.lIndex, item));
		pthread_mutex_unlock(&theClient.m_Lock);
	}
	return true;

}

int CNetIfBase::OnEncodeIf(void* pMsg, char* pBuf)
{
	MsgHdr *pMsgHdr = (MsgHdr *)pMsg;
	memcpy(pBuf, (char *)pMsg, pMsgHdr -> unMsgLen);
	//AddMsg((unsigned char*)pBuf, pMsgHdr -> unMsgLen);
	return pMsgHdr -> unMsgLen;	
}
bool CNetIfBase::SenderSvr(void)
{
	TRACE(CNetIfBase::Sender);
	int nMsgLeft = 0;
	int nMsgLen = 0;
	int nMaxResend = 0;
	char MsgBuf[COMM_RECV_MAXBUF];

	QUEUEELEMENT pMsg = NULL;
	MsgHdr* pMsgHdr  = NULL;
//	Msg * pTMsg = NULL;
	int nBytes = 0;
	int socketid = 0;

	m_SndrTID = pthread_self();
	
	while (1)
	{
		pthread_testcancel();
		if(IF_STA_UP != m_Status)
		{	
			usleep(1000*500);
			continue;
		}

		if (MMR_OK == g_MsgMgr.GetFirstMsg(m_unToSelf, pMsg, 20))
		{
			if (pMsg == NULL) continue;
			
			pMsgHdr = (MsgHdr*)pMsg;
			socketid = pMsgHdr->unRspTnlId;
			if (socketid<0) continue;
		//	pTMsg = (Msg *)((char *)pMsg + MSGHDRLEN);
			if (pMsgHdr->unMsgCode==COMM_TERMINATE) // 主动断开
			{				
				int i;
				for (i=0; i<(int)COMM_TCPSRV_MAXClIENT; ++i)
					if (m_clients[i].socketid==socketid)
					{
						m_clients[i].Reset();				
						OnLinkDown();
						break;
					}

			}
			else if(SetNetIfHashTcpSvr((char *)pMsg))
			{				
				if(m_info.CallbackOnEncode)
					nMsgLeft = nMsgLen = m_info.CallbackOnEncode(pMsg, MsgBuf);
				else
					nMsgLeft = nMsgLen = OnEncodeIf(pMsg, MsgBuf);
//AddMsg((BYTE*)MsgBuf, nMsgLen);
				if(nMsgLen < (int)COMM_RECV_MAXBUF)
				{ 
					nMaxResend = COMM_SEND_MAXNUM;
					while (nMsgLeft && nMaxResend) 
					{
						nBytes = send(socketid, MsgBuf + nMsgLen - nMsgLeft, nMsgLeft, 0);
						if (nBytes < 0)
						{
							nMaxResend = -1;
							break;
						}
						char Temp[256] = {0};
						sprintf(Temp, "Send[TCPServer] To Socket=%d,DataLen=%d\n",socketid,nBytes);
						DataLogSock(FUNC_HEXINFO,Temp,(char *)(MsgBuf + nMsgLen - nMsgLeft),nBytes);
						nMsgLeft = nMsgLeft - nBytes;
						nMaxResend--;
					}
					switch(nMaxResend) {
						case 0:
							NetIfErr(0 /*lhy pTMsg->unTsn*/, STA_SEND_FAILED,(void*)socketid);
							break;
						case -1:
							NetIfErr(0 /*lhy pTMsg->unTsn*/, STA_SYSTEM_ERR,(void*)socketid);
							break;
					}
				}

			}
			MM_IF_FREE(pMsg);
			pMsg = NULL;
		}
	}
	return true;}

bool CNetIfBase::Sender(void)
{

	TRACE(CNetIfBase::Sender);
	int nMsgLeft = 0;
	int nMsgLen = 0;
	int nMaxResend = 0;
	char MsgBuf[COMM_RECV_MAXBUF];

	QUEUEELEMENT pMsg = NULL;
	MsgHdr* pMsgHdr  = NULL;
//	Msg * pTMsg = NULL;
	int nBytes = 0;

	m_SndrTID = pthread_self();
	
	while (1)
	{
		pthread_testcancel();
		if(IF_STA_UP != m_Status)
		{	
			usleep(1000*900);
			continue;
		}

        //DBG(("m_unToSelf:[%d]\n", m_unToSelf));
		if (MMR_OK == g_MsgMgr.GetFirstMsg(m_unToSelf, pMsg, 20))
		{
			if (pMsg == NULL) continue;
			
			pMsgHdr = (MsgHdr*)pMsg;

//			pTMsg = (Msg *)((char *)pMsg + MSGHDRLEN);
			if(SetNetIfHash((char *)pMsg))//if hash is not sucess, insert back to queue,continue
			{	
				if(m_info.CallbackOnEncode)
					nMsgLeft = nMsgLen = m_info.CallbackOnEncode(pMsg, MsgBuf);
				else
					nMsgLeft = nMsgLen = OnEncodeIf(pMsg, MsgBuf);
AddMsg((BYTE*)MsgBuf, nMsgLen);
				if(nMsgLen < (int)COMM_RECV_MAXBUF)
				{ 
					nMaxResend = COMM_SEND_MAXNUM;
					while (nMsgLeft && nMaxResend) 
					{
						nBytes = SndData(this, MsgBuf + nMsgLen - nMsgLeft, nMsgLeft);
						if (nBytes < 0)
						{
							nMaxResend = -1;
							break;
						}
						nMsgLeft = nMsgLeft - nBytes;
						nMaxResend--;
					}
					switch(nMaxResend) {
						case 0:
							NetIfErr( 0/*lhy pTMsg->unTsn*/, STA_SEND_FAILED);
							break;
						case -1:
							NetIfErr(0/*lhy pTMsg->unTsn*/, STA_SYSTEM_ERR);
							break;
					}
				}

			}
			MM_IF_FREE(pMsg);
			pMsg = NULL;
		}
	}
	return true;
}
//NP_TCP Receiver
bool CNetIfBase::Receiver(void)
{
	TRACE(CNetIfBase::Receiver);
	int nRcvLen = 0;
	char cBuff[COMM_RECV_MAXBUF] = {0};
	char MsgBuf[COMM_RECV_MAXBUF]= {0};
	char RespBuf[MAXMSGSIZE] = {0};
	int RespLen = 0;
	TKU32 nRcvPos = 0;
	int nCursor = 0;
	bool bContParse = true;
	CodecType ctRslt;
	MsgHdr *pMsgHdr = NULL;

	m_RcvrTID = pthread_self();	
	while (1)
	{
		pthread_testcancel();

		if(IF_STA_UP != m_Status)
		{	
			usleep(1000*900);
			continue;
		}
		if (m_pSock->IsPending(COMM_PENDING_TIME))
		{
			assert(nRcvPos < COMM_RECV_MAXBUF);

			nRcvLen = RcvData(this, &cBuff[nRcvPos], COMM_RECV_MAXBUF - nRcvPos - 1 );
				
			if (nRcvLen <= 0)
			{
				DBG(("Tcp Recv Len <= 0\n"));
				m_Status = IF_STA_DOWN;
				if(true == OnLinkDown())
				{
					m_pSock -> EndSocket();
					m_Status = IF_STA_DOWN;
					continue;
				}
			}
			else
			{
				if(m_info.bIsZeroSeq)
					m_iTestSta = 0;
				DBG(("Tcp Recv RecvLen[%d] nRcvPos[%d]", nRcvLen, nRcvPos));
				AddMsg((unsigned char*)(cBuff + nRcvPos), nRcvLen);
				nRcvPos += nRcvLen;			
				nRcvLen = 0;
				nCursor = 0;
				RespLen = 0;
				int nLen = 0;	
				bContParse = true;	
				while (bContParse)
				{
					
					nLen = nRcvPos - nCursor;
					if(0 >= nLen) break;
					
					memset(MsgBuf, 0, sizeof(MsgBuf));
					memset(RespBuf, 0, sizeof(RespBuf));
					if(m_info.CallbackOnDecode)	//需要解码的外部通信包
						ctRslt = m_info.CallbackOnDecode(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);
					else
						ctRslt = OnDecodeIf(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);

					switch(ctRslt)
					{
						case CODEC_CMD:
							DBG(("CODEC_CMD!\n"));
							if(RespLen > 0)
							{
								if(false == SendMsg(m_unToSelf, m_info.unMemHdl, RespBuf))
								{
									NetIfErr(0, STA_MEM_ALLOC_FAILED);
									DBG(("NetIfErr CODEC_CMD RespLen > 0\n"));
								}
							}
							pMsgHdr = (MsgHdr *)MsgBuf;
							pMsgHdr->unRspTnlId = m_unToSelf;
							if(false == SendMsg(m_unToCtrl, m_info.unMemHdl, (char *)MsgBuf))
							{
								NetIfErr(0, STA_MEM_ALLOC_FAILED);
								DBG(("NetIfErr CODEC_CMD\n"));
							}				
							nCursor += nLen;
							break;
						case CODEC_RESP:
							DBG(("CODEC_RESP!\n"));
							pMsgHdr = (MsgHdr *)MsgBuf;
							if (pMsgHdr -> unMsgCode==(COMM_ACTIVE_TEST|COMM_RSP))
								m_iTestSta = 0;
							pthread_mutex_lock(&m_Lock);
							htNetIfHash.erase(pMsgHdr -> unMsgSeq);
							 pthread_mutex_unlock(&m_Lock);
							nCursor += nLen;
							break;
						case CODEC_TRANS:
							DBG(("CODEC_TRANS!\n"));
							pMsgHdr = (MsgHdr *)MsgBuf;
							pthread_mutex_lock(&m_Lock);
							it = htNetIfHash.find(pMsgHdr -> unMsgSeq);							
							if (it!=htNetIfHash.end())
							{
//lhy								Msg* pTMsg = (Msg*) (pMsgHdr+1);
								//lhy pTMsg->unTsn = it->unTsn;
								htNetIfHash.erase(it);
								
							}
							pthread_mutex_unlock(&m_Lock);
							if(m_info.CallbackOnDecode)
							{
								if(false == SendMsg(m_unToCtrl, m_info.unMemHdl, (char *)MsgBuf))
								{
									NetIfErr(0, STA_MEM_ALLOC_FAILED);
									DBG(("NetIfErr CODEC_TRANS\n"));
								}				
							}
							nCursor += nLen;
							break;
						case CODEC_NEED_DATA:
							DBG(("CODEC_NEED_DATA!\n"));
							bContParse = false;
							break;
						case CODEC_CONN:
							DBG(("CODEC_CONN!\n"));
							m_Status = IF_STA_DOWN;
							break;
						case CODEC_ERR:			
							DBG(("CODEC_ERR!\n"));
							nRcvPos = 0;							
							bContParse = false;
							break;
						default:
							DBG(("default!\n"));
							break;
					}
				}
				assert(nRcvPos >= 0);
				if(0 != nRcvPos)
				{
					memcpy(cBuff, &cBuff[nCursor], nRcvPos - nCursor);
					nRcvPos -= nCursor;
				}
			}
		}
	}
	return true;
}

CodecType CNetIfBase::OnDecodeIf(void* pMsg, void* pBuf, void* pRespBuf, int &RespLen, int& nUsed)
{
	char *szMsg = (char *)pMsg;
	char *pOut = (char *)pBuf;	
	MsgHdr *pMsgHdr = (MsgHdr *)pMsg;

	if((TKU32)nUsed < MSGHDRLEN )
	{
		nUsed = 0;
		return CODEC_NEED_DATA;
	}

	TKU32 unMsgLen  = pMsgHdr->unMsgLen;
	TKU32 unMsgCode = pMsgHdr->unMsgCode;
	TKU32 unStatus  = pMsgHdr->unStatus;
	//TKU32 unMsgSeq  = pMsgHdr->unMsgSeq;
	
    DBG(("unmsgLen:[%d]\n", unMsgLen));
	if (unMsgLen < MSGHDRLEN || unMsgLen > COMM_RECV_MAXBUF)
	{
		nUsed = 0;
		return CODEC_ERR;
	}
	if(nUsed < (int)unMsgLen)
	{
		nUsed = 0;
		return CODEC_NEED_DATA;
	}
	nUsed = unMsgLen;	
	memcpy(pOut, szMsg, unMsgLen);
	if(unMsgCode & COMM_RSP)
	{
		if(unStatus == 0x00)
		{
			return CODEC_RESP;
		}
		else
		{
			return CODEC_ERR;
		}	
	}	

//回应
	pMsgHdr -> unMsgLen = MSGHDRLEN; 
	pMsgHdr -> unMsgCode = unMsgCode + COMM_RSP ;
	memcpy(pRespBuf, pMsgHdr , MSGHDRLEN);
	RespLen = MSGHDRLEN;
	
	return CODEC_CMD;
}
	

bool CNetIfBase::OnLinkDown(void)
{
	TRACE(CNetIfBase::OnLinkDown);
	if (NP_TCP == m_info.npType)
	{
		if(m_info.bTcpIsSvrConn)
		{
			if (NULL != CallbackLinkDown)
			{
				try
				{
					CallbackLinkDown(m_nIfId);
				} catch (...)
				{
				}
				return true;
			}
		}
		return true;
	}
	else if (NP_TCPSVR == m_info.npType)
	{
		assert(pSvrConnMgr);
		((ClsTCPSvrSocket*)pSvrConnMgr)->ReduceConn();
		return true;	
	}
	else if (NP_UDP== m_info.npType)
	{
		// 将重发计数据表复原
		recvMsgSeq = 0;
		pthread_mutex_lock(&m_recvLock);
		htRecvNetIfHash.clear();
		pthread_mutex_unlock(&m_recvLock);
		
		return true;	
	}
	
	return false;
}

int CNetIfBase::GetSockId(void)
{
	TRACE(CNetIfBase::GetSockId);
	if (m_pSock)
		return m_pSock->GetSockId();
	else
		return -1;
}
bool CNetIfBase::SetClient(int socketid)
{
	int i;
	for (i=0; i<(int)COMM_TCPSRV_MAXClIENT; ++i)
		if (m_clients[i].socketid<0)
		{
			m_clients[i].socketid = socketid;
			return true;
		}
	return false;
}
//TCP Server
bool CNetIfBase::PendReqHandlerSvr(void)
{
	TRACE(CNetIfBase::PendReqHandler);
	unsigned int i = 0;
	time_t tNow = 0;
	time_t tTimeTest = 0;
	time(&tTimeTest);
	char stMsgHdr[MSGHDRLEN] = {0};
	m_PendTID = pthread_self();
	double dTime = 0;
	while (1)
	{
		usleep(1000*500);
		bool hasclient = false;
		for (i=0; i<COMM_TCPSRV_MAXClIENT; ++i)
		{
			pthread_testcancel();
			if (m_clients[i].socketid<0)
				continue;
			hasclient = true;		
			_NET_IF_HASH& htcNetIfHash = m_clients[i].htNetIfHash;
			pthread_mutex_t& mtxHashLock = m_clients[i].m_Lock;
			//检查是否有超时的请求
			pthread_mutex_lock(&mtxHashLock);
			_NET_IF_HASH::iterator it, ittmp;
			it = htcNetIfHash.begin();
			time(&tNow);
			while (it != htcNetIfHash.end())
			{
				ittmp = it;
				++ittmp;
				
				dTime = difftime(tNow, it->second.TimeStamp);
				if((TKU32)dTime >= m_nTimeout)
				{
					NetIfErr(0/*lhy it->unTsn*/, STA_TIMEOUT,(void*)it->second.unRspTnlId);	
					htcNetIfHash.erase(it);
				}
				it = ittmp;
			}
			pthread_mutex_unlock(&mtxHashLock);
		}
		
		if (hasclient==false)
			usleep(1000*50);
			
		//网络测试
		if(m_info.bIsTest)
		{
			
			time(&tNow);
			double dTime = difftime(tNow, tTimeTest);
			if((TKU32)dTime >= m_nTimeout)
			{
				for (i=0; i<COMM_TCPSRV_MAXClIENT; ++i)
				{
					if (m_clients[i].socketid<0)
						continue;
					unsigned int& nTestSta =  m_clients[i].nTestSta;
					nTestSta++;
					if(nTestSta >= COMM_ACTIVE_TEST_END)
					{
						OnLinkDown();
						NetIfErr(0, STA_IF_DOWN,(void *)m_clients[i].socketid);
						m_clients[i].Reset();
					}
					else if(nTestSta >= COMM_ACTIVE_TEST_START)
					{
						MsgHdr *pMsgHdr = (MsgHdr *)stMsgHdr;
						pMsgHdr->unMsgLen = MSGHDRLEN;
						pMsgHdr->unMsgCode = COMM_ACTIVE_TEST;
						pMsgHdr->unRspTnlId = m_clients[i].socketid;
						SendMsg(m_unToSelf, m_info.unMemHdl, stMsgHdr);						
					}	
				}
				tTimeTest = tNow;
			}
		}//if		
	}
	return true;
}
//TCP Client
bool CNetIfBase::PendReqHandler(void)
{
	TRACE(CNetIfBase::PendReqHandler);
	time_t tNow = 0;
	time_t tTimeTest = 0;
	char stMsgHdr[MSGHDRLEN] = {0};
	time(&tTimeTest);
	m_PendTID = pthread_self();

	while (1)
	{
		usleep(1000*900);
		
		pthread_testcancel();
		//检查网络是否正常
		if((IF_STA_DOWN == m_Status) && (NP_TCP == m_info.npType) && (!m_info.bTcpIsSvrConn) )
		{			
			m_bIsInitialized = false;
			InitTcp();
		}
		//检查是否有超时的请求
//		pthread_mutex_lock(&m_mtxHashLock);
		pthread_mutex_lock(&m_Lock);
		_NET_IF_HASH::iterator it, it_temp;
		it = htNetIfHash.begin();
		while (it != htNetIfHash.end())
		{
			it_temp = it;
			++it_temp;

			time(&tNow);
			double dTime = difftime(tNow, it->second.TimeStamp);
			if((TKU32)dTime >= m_nTimeout)
			{
				NetIfErr(0/*lhy it->unTsn*/, STA_TIMEOUT,(void*)it->second.unRspTnlId);	
				htNetIfHash.erase(it);
			}
			it = it_temp;
		}
		pthread_mutex_unlock(&m_Lock);
		//网络测试包
		if(m_info.bIsTest)
		{		
			time(&tNow);
			double dTime = difftime(tNow, tTimeTest);
			if((TKU32)dTime >= m_nTimeout)
			{
				m_iTestSta++;				
				if(m_iTestSta >= COMM_ACTIVE_TEST_END)
				{				
					if(true == OnLinkDown())
					{
						m_iTestSta = 0;
						m_pSock -> EndSocket();
						m_Status = IF_STA_DOWN;
					}	
					NetIfErr(0, STA_IF_DOWN);
				}
				else
				{
					if(m_iTestSta >= COMM_ACTIVE_TEST_START)
					{
						MsgHdr *pMsgHdr = (MsgHdr *)stMsgHdr;
						pMsgHdr->unMsgLen = MSGHDRLEN;
						pMsgHdr->unMsgCode = COMM_ACTIVE_TEST;
						pMsgHdr->unRspTnlId = m_unToSelf;
						SendMsg(m_unToSelf, m_info.unMemHdl, stMsgHdr);
					}
				}
				tTimeTest = tNow;
			}		
		}
	}
	return true;
}

void * CNetIfBase::pThrdTSender(void *arg)
{
	TRACE(CNetIfBase::pThrdTSender);
	CNetIfBase* _this = (CNetIfBase *)arg;
	if (_this->m_info.npType==NP_TCP)
		_this->Sender();
	else if(_this->m_info.npType==NP_UDP)
		_this->udpSender();
	else if (_this->m_info.npType==NP_TCPSVR)
		_this->SenderSvr();
	return NULL;
}

void * CNetIfBase::pThrdReceiver(void *arg)
{
	TRACE(CNetIfBase::pThrdReceiver);
	CNetIfBase* _this = (CNetIfBase *)arg;
	if (_this->m_info.npType==NP_TCP)
		_this->Receiver();
	else if (_this->m_info.npType==NP_UDP)
		_this->udpReceiver();
	else if (_this->m_info.npType==NP_TCPSVR)
		_this->ReceiverSvr();

	return NULL;
}

void * CNetIfBase::pThrdPendReqHandler(void *arg)
{
	TRACE(CNetIfBase::pThrdPendReqHandler);
	CNetIfBase* _this = (CNetIfBase *)arg;
	if (_this->m_info.npType==NP_TCP)
		_this->PendReqHandler();
	else if (_this->m_info.npType==NP_UDP)
		_this->udpPendReqHandler();
	else if (_this->m_info.npType==NP_TCPSVR)
		_this->PendReqHandlerSvr();

	return NULL;
}
bool CNetIfBase::NetIfErr(TKU32 unTsn, int ErrNo, void* param)
{

	char szMsg[MAXMSGSIZE] = {0};
	MsgHdr *pMsgHdr = (MsgHdr *)szMsg;
	Msg *pTMsg = (Msg *)(szMsg + MSGHDRLEN);
	
	pMsgHdr -> unMsgCode = COMM_ERR;
	if (param!=NULL)
		pMsgHdr->unRspTnlId = (TKU32)(size_t)param;		//lsd
//	pTMsg -> unTsn = unTsn;
	memcpy(pTMsg -> stAlarm.szModName, m_info.szModName, strlen(m_info.szModName));
	pTMsg -> stAlarm.btErrNo = ErrNo;
	pMsgHdr -> unMsgLen = MSGHDRLEN + MSGPRELEN + 32 + strlen(pTMsg -> stAlarm.szErrInfo);
	return SendMsg(m_unToCtrl, m_info.unMemHdl, (char *)szMsg);
}
int CNetIfBase::CanRead(int& socketid, int ntimout)
{
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(socketid, &fdset);
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = ntimout;
	int ret = select(socketid+1,&fdset,(fd_set *)NULL,(fd_set *)NULL, &timeout);
	return ret;
}
bool CNetIfBase::SetPendBuf(const TKU32 unSize)
{
	TRACE(CNetIfBase::SetPendBuf);
	return true;
}
int CNetIfBase::UdpSend(CNetIfBase* _this, char *buf, int len) 
{
	TRACE(CNetIfBase::UdpSend);
	return((ClsUDPSocket*)(_this->m_pSock))->Send(buf, len, (struct sockaddr *)(_this->m_peer), _this->m_nPeerSize);
}
int CNetIfBase::UdpReceive(CNetIfBase*_this,char *buf, int len) 
{
	TRACE(CNetIfBase::UdpReceive);
	return((ClsUDPSocket*)(_this->m_pSock))->RecvFrom(buf, len,(struct sockaddr *)(_this->m_peer),&(_this->m_nPeerSize));
}
int CNetIfBase::TcpSend(CNetIfBase* _this,char *buf, int len) 
{
	TRACE(CNetIfBase::TcpSend);
	return((ClsTCPClientSocket*)(_this->m_pSock))->Send(buf, len);
}
int CNetIfBase::TcpReceive(CNetIfBase*  _this,char *buf, int len) 
{
	TRACE(CNetIfBase::TcpReceive);
	return((ClsTCPClientSocket*)(_this->m_pSock))->Recv(buf, len);
}

//UDP
bool CNetIfBase::udpPendReqHandler(void)
{
	TRACE(CNetIfBase::PendReqHandler);
	time_t tNow = 0;
	time_t tTimeTest = 0;
	struct timeb tbNow;
	char stMsgHdr[MSGHDRLEN] = {0};
	time(&tTimeTest);
	m_PendTID = pthread_self();

	while (1)
	{
		usleep(1000*900); // ms轮询
		
		pthread_testcancel();
		//检查网络是否正常
		if((IF_STA_DOWN == m_Status) && (NP_TCP == m_info.npType) && (!m_info.bTcpIsSvrConn) )
		{			
			m_bIsInitialized = false;
			InitTcp();
		}
		//检查是否有超时的请求
		_NET_IF_HASH::iterator it, ittmp;
		//pthread_mutex_lock(&m_mtxHashLock);
		pthread_mutex_lock(&m_Lock);
		it = htNetIfHash.begin();		
		while (it != htNetIfHash.end())
		{
			if (!it->second.pMsg)
			{
				++it;
				continue;
			}
			
			ittmp = it;
			++ittmp;	
				
			memset(&tbNow, 0, sizeof(tbNow));
			ftime(&tbNow);
			//double dTime = difftime(tNow, it->TimeStamp);
			int dTimems = (tbNow.time-it->second.TimeStamp)*1000+tbNow.millitm-it->second.TimeStampms;
			if(dTimems >= it->second.nReSend*900) //if((TKU32)dTime >= (double)m_nTimeout/COMM_SEND_MAXNUM*it->nReSend)
			{
				if(it->second.nReSend>3)
				{
					//pthread_mutex_lock(&m_Lock);

					NetIfErr(0/*lhy it->unTsn*/, STA_TIMEOUT,(void*)it->second.unRspTnlId);
//printf("%s Pender timeout free Msg%p, MsgTotal[%d]\n", m_info.szModName, it->pMsg, htNetIfHash.size()-1);
					if (it->second.pMsg)
						MM_FREE(it->second.pMsg);
					htNetIfHash.erase(it);
					++nrefail;
					//pthread_mutex_unlock(&m_Lock);
				}
				else
				{
//printf("%s Pender resend no[%d] Msg%p, MsgTotal[%d] dtime[%d]=sms[%d]-dms[%d] \n",m_info.szModName,  it->nReSend, it->pMsg,  htNetIfHash.size(), dTimems, it->TimeStampms, tbNow.millitm);
					//char *pHashMsg = (char *)MM_ALLOC(m_info.unMemHdl);//	memcpy(pHashMsg, it->pMsg, ((MsgHdr*)it->pMsg)->unMsgLen);
/*在正式版本中要
					if(it->pMsg)					
						g_MsgMgr.InsertMsg(m_unToSelf,(QUEUEELEMENT)it->pMsg, 50); //g_MsgMgr.SendMsg(m_unToSelf,(QUEUEELEMENT)it->pMsg, 50); //MMR_OK
*/
					++(it->second.nReSend);
					++nresend;
				}			
				
			}
			it = ittmp;
		}
		//pthread_mutex_lock(&m_mtxHashLock);
		pthread_mutex_unlock(&m_Lock);
		
		//网络测试包
		time(&tNow);
		double dTime = difftime(tNow, tTimeTest);
		if((TKU32)dTime >= m_nTimeout)
		{
			m_iTestSta++;
			
			if(m_iTestSta >= COMM_ACTIVE_TEST_END)
			{				
				if(true == OnLinkDown())
				{
					m_iTestSta = 0;
					//m_pSock -> EndSocket();
					//m_Status = IF_STA_DOWN;
				}

				NetIfErr(0, STA_IF_DOWN);

			}
			else
			{
				if(m_iTestSta >= COMM_ACTIVE_TEST_START)
				{
					MsgHdr *pMsgHdr = (MsgHdr *)stMsgHdr;
					pMsgHdr->unMsgLen = MSGHDRLEN;
					pMsgHdr->unMsgCode = COMM_ACTIVE_TEST;
					pMsgHdr->unRspTnlId = m_unToSelf;
					SendMsg(m_unToSelf, m_info.unMemHdl, stMsgHdr);

				}
			}
			tTimeTest = tNow;
		}		
	}
	return true;	
}
bool CNetIfBase::udpSender(void)
{
	TRACE(CNetIfBase::Sender);
	int nMsgLeft = 0;
	int nMsgLen = 0;
	int nMaxResend = 0;
	char MsgBuf[COMM_RECV_MAXBUF];

	QUEUEELEMENT pMsg = NULL;
	MsgHdr* pMsgHdr  = NULL;
	Msg * pTMsg = NULL;
	int nBytes = 0;

	m_SndrTID = pthread_self();
	
	while (1)
	{
		pthread_testcancel();
		if(IF_STA_UP != m_Status)
		{	
			sleep(1);
			continue;
		}

		if (MMR_OK == g_MsgMgr.GetFirstMsg(m_unToSelf, pMsg, 20))
		{
			if (pMsg == NULL)
				continue;
			
			pMsgHdr = (MsgHdr*)pMsg;
			if (pMsgHdr->unMsgLen<1)
				continue;

			pTMsg = (Msg *)((char *)pMsg + MSGHDRLEN);
			if(udpSetNetIfHash((char *)pMsg))//if hash is not sucess, insert back to queue,continue
			{				
				if(m_info.CallbackOnEncode)
					nMsgLeft = nMsgLen = m_info.CallbackOnEncode(pMsg, MsgBuf);
				else
					nMsgLeft = nMsgLen = OnEncodeIf(pMsg, MsgBuf);

				if(nMsgLen < (int)COMM_RECV_MAXBUF)
				{ 
					nMaxResend = COMM_SEND_MAXNUM;
					while (nMsgLeft && nMaxResend) 
					{
						nBytes = SndData(this, MsgBuf + nMsgLen - nMsgLeft, nMsgLeft);
						if (nBytes < 0)
						{
							nMaxResend = -1;
							break;
						}
						nMsgLeft = nMsgLeft - nBytes;
						nMaxResend--;
					}
					if (nMaxResend>0)
						++nsend;
					else
					{
						++nsenderror;
					}
					
					switch(nMaxResend) {
						case 0:

							NetIfErr(0/*lhy pTMsg->unTsn*/, STA_SEND_FAILED);
							break;
						case -1:

							NetIfErr(0/*lhy pTMsg->unTsn*/, STA_SYSTEM_ERR);
							break;
					}
				}

			}
			if (pMsgHdr->unMsgCode!=COMM_SUBMIT && pMsgHdr->unMsgCode!=COMM_DELIVER)
				MM_IF_FREE(pMsg);
			pMsg = NULL;
		}
	}
	return true;	
}
bool CNetIfBase::udpReceiver(void)
{
	TRACE(CNetIfBase::Receiver);
	int nRcvLen = 0;
	char cBuff[COMM_RECV_MAXBUF] = {0};
	char MsgBuf[COMM_RECV_MAXBUF]= {0};
	char RespBuf[MAXMSGSIZE] = {0};
	int RespLen = 0;
	TKU32 nRcvPos = 0;
	int nCursor = 0;
	bool bContParse = true;
	CodecType ctRslt;
	MsgHdr *pMsgHdr = NULL;
	_NET_IF_HASH::iterator it;
//	_NET_IF_TIMEOUT  msgseq;

	m_RcvrTID = pthread_self();	
	while (1)
	{
		pthread_testcancel();

		if(IF_STA_UP != m_Status)
		{	
			sleep(1); 
			continue;
		}
		if (m_pSock->IsPending(COMM_PENDING_TIME))
		{
			assert(nRcvPos < COMM_RECV_MAXBUF);

			nRcvLen = RcvData(this, &cBuff[nRcvPos], COMM_RECV_MAXBUF - nRcvPos - 1 );
				
			if (nRcvLen <= 0)
			{
				m_Status = IF_STA_DOWN;
				if(true == OnLinkDown())
				{
					m_pSock -> EndSocket();
					m_Status = IF_STA_DOWN;
					continue;
				}
			}
			else
			{
				if(m_info.bIsZeroSeq)
					m_iTestSta = 0;
				nRcvPos += nRcvLen;
				nRcvLen = 0;
				nCursor = 0;
				RespLen = 0;
				int nLen = 0;	
				bContParse = true;				
				bool isValidMsg = true;
				
				while (bContParse)
				{
					nLen = nRcvPos - nCursor;
					if(0 >= nLen) break;
					
					memset(MsgBuf, 0, sizeof(MsgBuf));
					memset(RespBuf, 0, sizeof(RespBuf));
					if(m_info.CallbackOnDecode)	//需要解码的外部通信包
						ctRslt = m_info.CallbackOnDecode(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);
					else
						ctRslt = OnDecodeIf(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);
					switch(ctRslt)
					{
						case CODEC_CMD:
						 	++nrecv;
						 	
							if(RespLen > 0)
							{
								if(false == SendMsg(m_unToSelf, m_info.unMemHdl, RespBuf))
								{
									++nrecverror;

									NetIfErr(0, STA_MEM_ALLOC_FAILED);
								}
							}
							isValidMsg = true;
							pMsgHdr = (MsgHdr *)MsgBuf;
/*在正式版本中要
							if ( (pMsgHdr->unMsgCode==COMM_SUBMIT || pMsgHdr->unMsgCode==COMM_DELIVER) && recvMsgSeq!=0 )
							{								
								pthread_mutex_lock(&m_recvLock);
								if (pMsgHdr->unMsgSeq<=recvMsgSeq)
								{
									// 查找是否为未收到的包
									it =  htRecvNetIfHash.find(pMsgHdr->unMsgSeq);
									if (it!=htRecvNetIfHash.end())
									{
										htRecvNetIfHash.erase(it);
										++nrecvvalid;
									}
									else
									{
										isValidMsg = false;
										++nrecvinvalid;
									}							
									
								}
								else if (recvMsgSeq+1!=pMsgHdr->unMsgSeq)
								{
									memset(&msgseq, 0, sizeof(msgseq));
									// 此中间的包有丢,记录
									time_t tmsg = time(NULL);
									for (TKU32 k=recvMsgSeq+1; k<pMsgHdr->unMsgSeq; ++k)
									{
										msgseq.TimeStamp = tmsg;
										msgseq.lIndex = k;
										htRecvNetIfHash.insert_unique(msgseq);
									}
								}
								pthread_mutex_unlock(&m_recvLock);
								
							}*/
							if (recvMsgSeq<pMsgHdr->unMsgSeq)
								recvMsgSeq = pMsgHdr->unMsgSeq;
						
							if (isValidMsg)
								if(false == SendMsg(m_unToCtrl, m_info.unMemHdl, (char *)MsgBuf))
								{
									++nrecverror;
									NetIfErr(0, STA_MEM_ALLOC_FAILED);
								}				
							nCursor += nLen;
							break;
						case CODEC_RESP:
							//DBG(("CODEC_RESP!\n"));
							pMsgHdr = (MsgHdr *)MsgBuf;
//printf("%s Recv Resp Msg SEQ[%u], ",m_info.szModName, pMsgHdr->unMsgSeq);
							pthread_mutex_lock(&m_Lock);
							it = htNetIfHash.find(pMsgHdr -> unMsgSeq);
							if (it!=htNetIfHash.end())
							{
//printf("to Free=%p\n", it->pMsg);
								//it->isResp = true;
								if (it->second.pMsg)
									MM_FREE(it->second.pMsg);
								htNetIfHash.erase(it);
							}
							pthread_mutex_unlock(&m_Lock);
							nCursor += nLen;
							break;
						case CODEC_NEED_DATA:
							//DBG(("CODEC_NEED_DATA!\n"));
							bContParse = false;
							break;
						case CODEC_ERR:													
							//DBG(("CODEC_ERR UdpReceiver!\n"));
							//DBG(("M_info=%s\n",m_info.szModName));
							//AddMsg((unsigned char *)&cBuff[nCursor],200);
							nRcvPos = 0;							
							bContParse = false;
							break;
						default:
							//DBG(("CODEC_DEFAULT!\n"));
							break;
					}
				}
				assert(nRcvPos >= 0);
				if(0 != nRcvPos)
				{
					memcpy(cBuff, &cBuff[nCursor], nRcvPos - nCursor);
					nRcvPos -= nCursor;
				}
			}
		}
	}
	return true;	
}
bool CNetIfBase::udpSetNetIfHash(char * pMsg)
{
	TRACE(CNetIfBase::SetItemVal);
	_NET_IF_TIMEOUT item;
	
	MsgHdr *pMsgHdr = (MsgHdr*)pMsg;
//lhy	Msg *pTMsg = (Msg *)(pMsg + MSGHDRLEN);
	
	if(	pMsgHdr -> unMsgCode & COMM_RSP)
	{
//printf("%s  Send RespMsg SEQ[%u]\n",m_info.szModName, pMsgHdr->unMsgSeq);
		return true;
	}

	_NET_IF_HASH::iterator it;
	pthread_mutex_lock(&m_Lock);		
	it = htNetIfHash.find(pMsgHdr -> unMsgSeq);
	if (it==htNetIfHash.end())
	{
		struct timeb tmtmp;
		memset(&tmtmp, 0, sizeof(tmtmp));
		ftime(&tmtmp);
		pMsgHdr -> unMsgSeq = m_unMsgSeq;
//		item.btType = pTMsg -> btType;
//lhy		item.unTsn = pTMsg -> unTsn;
		item.lIndex = pMsgHdr->unMsgSeq;
		item.btMsgCode = pMsgHdr->unMsgCode;
		item.TimeStamp = tmtmp.time; //time(NULL);
		item.TimeStampms = tmtmp.millitm;
		item.iStatus = pMsgHdr->unStatus;
		item.unRspTnlId = pMsgHdr->unRspTnlId;
		item.nReSend = 1; // 当重发时次数
		item.isResp = false;
		//char *pHashMsg = (char *)MM_ALLOC(m_info.unMemHdl);
		//if(pHashMsg)
		//{
		//	memcpy(pHashMsg, pMsg, pMsgHdr->unMsgLen);
			item.pMsg = pMsg;
		//}
		//else
		//	item.pMsg = NULL;

		if (pMsgHdr -> unMsgCode==COMM_SUBMIT || pMsgHdr -> unMsgCode==COMM_DELIVER)
		{
			
			htNetIfHash.insert(_NET_IF_HASH::value_type(item.lIndex, item));
//printf("%s  Send Msg=%p TSN[%u] SEQ[%u]\n",m_info.szModName, pMsg, pTMsg -> unTsn, pMsgHdr->unMsgSeq);
		}
		
		if(m_unMsgSeq < COMM_MAXSEQ)
			m_unMsgSeq ++;
		else
			m_unMsgSeq = 2;
	}
	pthread_mutex_unlock(&m_Lock);
	return true;	
}


bool CNetIfBase::ReceiverSvr(void)
{
	TRACE(CNetIfBase::Receiver);
	int nRcvLen = 0;
	int ret;
	char MsgBuf[COMM_RECV_MAXBUF]= {0};
	char RespBuf[MAXMSGSIZE] = {0};
	int RespLen = 0;
	int nCursor = 0;
	bool bContParse = true;
	CodecType ctRslt;
	MsgHdr *pMsgHdr = NULL;
	m_RcvrTID = pthread_self();	
	int i=0;
	for (;;)
	{
		bool hasClient = false;
		if(IF_STA_UP != m_Status)
		{	
			usleep(1000*500);
			continue;
		}
		for (i=0; i<(int)COMM_TCPSRV_MAXClIENT; ++i)
		{
			pthread_testcancel();
			if (m_clients[i].socketid<0)
				continue;
			if(!m_clients[i].blFirst && !m_clients[i].blValid)
    		continue;
			
			hasClient = true;
			TKU32& nRcvPos = m_clients[i].nRcvPos;
		  char* const &  cBuff = m_clients[i].cBuff;
			int& socketid = m_clients[i].socketid;
			nRcvLen = 0;
			RespLen = 0;
			nCursor = 0;
			bContParse = true;
			pMsgHdr = NULL;
			_NET_IF_HASH& htcNetIfHash = m_clients[i].htNetIfHash;
			pthread_mutex_t& mtxHashLock = m_clients[i].m_Lock;
			ret = CanRead(socketid, 100);
			if (ret<0)
			{
				NetIfErr(0, STA_IF_DOWN, (void*)socketid);
				OnLinkDown();
				m_clients[i].Reset();
				continue;
			}
			else if (ret==0)
				continue;
			
			if(m_clients[i].blFirst)
			{
				m_clients[i].blFirst = false;
				m_clients[i].blValid = true;
			}
			
			assert(nRcvPos < COMM_RECV_MAXBUF);
			nRcvLen = recv(socketid, &cBuff[nRcvPos], COMM_RECV_MAXBUF - nRcvPos - 1, 0);
            DBG(("RecLen:[%d]\n", nRcvLen));
			if (nRcvLen <= 0)
			{
				NetIfErr(0, STA_IF_DOWN, (void*)socketid);
				OnLinkDown();
				m_clients[i].Reset();				
				continue;
			}
			else
			{
				char Temp[255] ={0};
				sprintf(Temp, "Recv[TCPServer] From Socket=%d,DataLen=%d\n",socketid,nRcvLen);
				DataLogSock(FUNC_HEXINFO,Temp,(char *)&cBuff[nRcvPos],nRcvLen);
                DBG(("TcpSvr recived msg from socket----------------->\n")); 
				//AddMsg((BYTE *)&cBuff[nRcvPos],nRcvLen);

				m_clients[i].nTestSta = 0;
				nRcvPos += nRcvLen;
				nRcvLen = 0;
				nCursor = 0;
				RespLen = 0;
				int nLen = 0;	
				bContParse = true;	
				
				while (bContParse)
				{
					nLen = nRcvPos - nCursor;
					if(0 >= nLen) break;
					
					memset(MsgBuf, 0, sizeof(MsgBuf));
					memset(RespBuf, 0, sizeof(RespBuf));
					if(m_info.CallbackOnDecode)	//需要解码的外部通信包
						ctRslt = m_info.CallbackOnDecode(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);
					else
						ctRslt = OnDecodeIf(&cBuff[nCursor] , MsgBuf, RespBuf, RespLen, nLen);
                    DBG(("ctRslt:[%d]\n", ctRslt));
					switch(ctRslt)
					{
						case CODEC_CMD:
							/*pMsgHdr = (MsgHdr *)MsgBuf;
							if(RespLen > 0 && pMsgHdr->unMsgCode!=COMM_CONNECT && pMsgHdr->unMsgCode!=COMM_TERMINATE)
							{
								pMsgHdr = (MsgHdr *)RespBuf;
								pMsgHdr->unRspTnlId = (TKU32)socketid;
								if(false == SendMsg(m_unToSelf, m_info.unMemHdl, RespBuf))
								{
									NetIfErr(0, STA_MEM_ALLOC_FAILED);
								}
							}*/
							pMsgHdr = (MsgHdr *)MsgBuf;
							if(RespLen > 0 && pMsgHdr->unMsgCode!=COMM_CONNECT && pMsgHdr->unMsgCode!=COMM_TERMINATE)
							{
								pMsgHdr = (MsgHdr *)RespBuf;
								pMsgHdr->unRspTnlId = (TKU32)socketid;
								if(false == SendMsg(m_unToSelf, m_info.unMemHdl, RespBuf))
								{
									NetIfErr(0, STA_MEM_ALLOC_FAILED);
								}
							}
							pMsgHdr = (MsgHdr *)MsgBuf;
							pMsgHdr->unRspTnlId = (TKU32)socketid;
							if(false == SendMsg(m_unToCtrl, m_info.unMemHdl, (char *)MsgBuf))
							{
								NetIfErr(0, STA_MEM_ALLOC_FAILED);
							}				
							nCursor += nLen;
							break;
						case CODEC_RESP:
							//DBG(("CODEC_RESP!\n"));
							pMsgHdr = (MsgHdr *)MsgBuf;
							pthread_mutex_lock(&mtxHashLock);
							htcNetIfHash.erase(pMsgHdr -> unMsgSeq);
							pthread_mutex_unlock(&mtxHashLock);
							nCursor += nLen;
							break;
						case CODEC_NEED_DATA:
							//DBG(("CODEC_NEED_DATA!\n"));
							bContParse = false;
							break;
						case CODEC_ERR:													
							//DBG(("CODEC_ERR ReceiverSvr!\n"));
							//AddMsg((unsigned char *)&cBuff[nCursor],200);
							nRcvPos = 0;							
							bContParse = false;
							break;
						default:
							//DBG(("CODEC_DEFAULT!\n"));
							break;
					}
				}
				assert(nRcvPos >= 0);
				if(0 != nRcvPos)
				{
					memcpy(cBuff, &cBuff[nCursor], nRcvPos - nCursor);
					nRcvPos -= nCursor;
				}
			}

		}
		
		if (hasClient==false)
			usleep(1000*500);
	}
	
	return true;	
	
}
