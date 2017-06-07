#include <string.h>
#include <stdio.h>
#include "SqlCtl.h"
#include "Analyser.h"
#include "Init.h"
#include "MsgCtl.h"
#include "NetApp.h"
#include "Define.h"

#define UP_TYPE_NUM        (2)
#define UP_TYPE_DATA       (0)
#define UP_TYPE_ACT        (1)

#define USERNAMELEN        (10)

bool CBOAMsgCtlSvr::Initialize(TKU32 unInputMask,const char *szCtlName)
{
 	if(false == CMsgCtlBase::Initialize(unInputMask, szCtlName)) 
		return false;
	
	m_Seq = 0;

	AddFunc(COMM_SUBMIT,	(CMDFUNC)SVR_Submit_Func);  	// 
	AddFunc(COMM_TERMINATE, (CMDFUNC)SVR_Terminate_Func);

    m_id = 0;
    m_ptrActInfoTable = new HashedActInfo();

    //数据处理线程
    char buf[256] = {0};
	sprintf(buf, "%s %s", "CBOAMsgCtlSvr", "ActCtrlThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CBOAMsgCtlSvr::pActCtrlThrd, true,this))
	{
		DBG(("pActCtrlThrd Create failed!\n"))
		return false;
	}

	return true;   		
}

//数据处理线程
void * CBOAMsgCtlSvr::pActCtrlThrd(void* arg)
{
    CBOAMsgCtlSvr *_this = (CBOAMsgCtlSvr *)arg;
    _this->MsgActCtrl();
    return NULL;
}
void CBOAMsgCtlSvr::MsgActCtrl()
{
    QUEUEELEMENT pMsg = NULL;
	char send_msg[1024] = {0};
    MsgHdr *pSendMsgHdr = (MsgHdr *)send_msg;
	MsgRespToPl* pPlRespMsg = (MsgRespToPl*)&send_msg[MSGHDRLEN];

	printf("CBOAMsgCtlSvr::MsgActCtrl Start ....Pid[%d]\n", getpid());
    while(true)
    {
        pthread_testcancel();
		
		//上级动作执行结果分析
		time_t tm_NowTime;
		TKU32 seqBuf[256] = {0};
		memset(send_msg, 0, sizeof(send_msg));
		int cnt = 0;
		HashedActInfo::iterator it;
		CTblLock.lock();
		for (it = m_ptrActInfoTable->begin(); it != m_ptrActInfoTable->end(); it++)
		{
			time(&tm_NowTime);
			if( tm_NowTime - it->second.act_time >= 30 )
			{
				seqBuf[cnt] = it->second.Id;
				cnt++;
				DBG(("平台网络动作超时[%d]\n", it->second.Id));
			}
		}
		CTblLock.unlock();
		for( int i=0; i < cnt; i++ )
		{
			MSG_ACTION_HASH hash_node;
			memset(&hash_node, 0, sizeof(MSG_ACTION_HASH));
			hash_node.Id = seqBuf[i];

			if( !GetAndDeleteHashData(hash_node))
			{
				continue;
			}

			pSendMsgHdr->unMsgLen = MSGHDRLEN + 28;
			pSendMsgHdr->unMsgCode = hash_node.unMsgCode;
			pSendMsgHdr->unMsgSeq = hash_node.unMsgSeq;
			pSendMsgHdr->unRspTnlId = hash_node.unRspTnlId;
			pSendMsgHdr->unStatus = 0;

			memcpy(pPlRespMsg->szReserve, hash_node.szReserve, 20);

			memcpy(pPlRespMsg->szDeal, hash_node.szDeal, 4);
			char tempStatus[5] = {0};
			sprintf(tempStatus, "%04d", SYS_STATUS_TIMEOUT);
			memcpy(pPlRespMsg->szStatus, tempStatus, 4);
			DBG(("MsgActCtrl 超时[%s]\n", (char*)pPlRespMsg));
			if (ACTION_SOURCE_NET_CPM == hash_node.source)//来自嵌入式
			{
				if(!SendMsg_Ex(g_ulHandleNetBoaSvrSend, MUCB_IF_NET_SVR_SEND_BOA, send_msg, pSendMsgHdr->unMsgLen))
				{
					DBG(("BOAdoSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetBoaSvrSend));
				}
			}
			else if (ACTION_SOURCE_NET_PLA == hash_node.source)//来自平台
			{	
				if(!SendMsg_Ex(g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND, send_msg, pSendMsgHdr->unMsgLen))
				{
					DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetCliSend));
				}
				else
				{
					DBG(("MsgCtrl.cpp 117 [%d] \n", g_ulHandleNetCliSend));
				}
			}
		}

        if(MMR_OK != g_MsgMgr.GetFirstMsg(g_ulHandleBoaMsgCtrl, pMsg, 10))      //返回给平台（待处理）
        {
            usleep(200*1000);
            continue;
        }

		do 
		{
			PACTION_RESP_MSG pRespMg = (PACTION_RESP_MSG)pMsg;
			MSG_ACTION_HASH hash_node;
			memset(&hash_node, 0, sizeof(MSG_ACTION_HASH));
			hash_node.Id = pRespMg->Seq;
			if( !g_MsgCtlSvrBoa.GetAndDeleteHashData(hash_node))
			{
				break;
			}

			pSendMsgHdr->unMsgLen = MSGHDRLEN + 28;
			pSendMsgHdr->unMsgCode = hash_node.unMsgCode;
			pSendMsgHdr->unMsgSeq = hash_node.unMsgSeq;
			pSendMsgHdr->unRspTnlId = hash_node.unRspTnlId;
			pSendMsgHdr->unStatus = 0;
			memcpy(pPlRespMsg->szReserve, hash_node.szReserve, 20);
			char tempStatus[5] = {0};
			sprintf(tempStatus, "%04d", pRespMg->Status);
			memcpy(pPlRespMsg->szStatus, tempStatus, 4);
			memcpy(pPlRespMsg->szDeal, hash_node.szDeal, 4);

			//lhy
			DBG(("Action Result[%s] Seq[%d] DealCmd[%s]  actionSource[%d] TsnId[%d] MsgLen[%d] send_msg[%s]\n",
				pPlRespMsg->szStatus,  hash_node.Id, hash_node.szDeal,  hash_node.source, hash_node.unRspTnlId, pSendMsgHdr->unMsgLen, send_msg));
			if (ACTION_SOURCE_NET_CPM == hash_node.source)//来自嵌入式
			{
				if(!SendMsg_Ex(g_ulHandleNetBoaSvrSend, MUCB_IF_NET_SVR_SEND_BOA, send_msg, pSendMsgHdr->unMsgLen))
				{
					DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetBoaSvrSend));
				}
			}
			else if (ACTION_SOURCE_NET_PLA == hash_node.source)//来自平台
			{
				//AddMsg((unsigned char*)send_msg, pSendMsgHdr->unMsgLen);
				if (!SendMsg(g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND, (char*)pSendMsgHdr))
				{
					DBG(("SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND));
				}
				else
				{
					DBG(("MsgCtl.cpp 169 [%d] \n", g_ulHandleNetCliSend));
				}
			}
			else
			{
				DBG(("MsgActCtrl  SendMsg_Ex Failed\n"));
			}
		} while (false);
		if (NULL != pMsg)
		{
			MM_FREE(pMsg);
			pMsg = NULL;
		}
    }
}

/******************************************************************************/
/* Function Name  : SVR_Submit_Func                                        */
/* Description    : 指令任务处理                                              */
/* Input          : char* msg       指令及信息                                */
/* Output         : none                                                      */
/* Return         : true                                                      */
/******************************************************************************/
bool CBOAMsgCtlSvr::SVR_Submit_Func(CBOAMsgCtlSvr *_this, char *msg)
{
    _this->doSVR_Submit_Func(msg);
    return true;
}

bool CBOAMsgCtlSvr::doSVR_Submit_Func(char *msg)
{
	time_t tm_TimeStart = 0;
	time(&tm_TimeStart); 
    DBG(("CPM Web ---------------------StartTime[%ld]----------------> CPM\n", tm_TimeStart));
	
	//通信包指针
	MsgHdr *pRecMsgHdr = (MsgHdr *)msg;
	DBG(("doSVR_Submit_Func: sorckeid[%d] Msg[%s]\n", pRecMsgHdr->unRspTnlId, msg + MSGHDRLEN));
	char srcIp[20] = {0};
	getpeeraddr(pRecMsgHdr->unRspTnlId,srcIp);
	if (0 != strcmp(g_szLocalIp, srcIp))
	{
		DBG(("LocalIp:%s SrcIp:%s\n", g_szLocalIp, srcIp));
		return false;
	}

    //DBG(("Len:[%d]  Status:[%d]  Seq:[%d]  TnlId:[%d]\n", pRecMsgHdr->unMsgLen, pRecMsgHdr->unStatus, pRecMsgHdr->unMsgSeq, pRecMsgHdr->unRspTnlId));
	//业务包指针
	PMsgRecvFromPl pRcvMsg = (PMsgRecvFromPl)(msg + MSGHDRLEN);
	//回应缓存
	char resp_msg[MAXMSGSIZE] = {0};
	MsgHdr *pRespMsgHdr = (MsgHdr *)resp_msg;
	char* pRespMsg = resp_msg + MSGHDRLEN;
	memcpy(resp_msg, msg, MSGHDRLEN);

	int resp_len = 0;
	int resp_ret = SYS_STATUS_FAILED;

	char deal[5] = {0};
	memcpy( deal, pRcvMsg->szDeal, 4);

	switch(atoi(deal))
	{
	case CMD_DEVICE_STATUS_QUERY:
		resp_ret = Net_Dev_Real_Status_Query((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICES_STATUS_QUERY:
		resp_ret = Net_Dev_Status_Query((void*)msg);
		break;
	case CMD_DEVICES_PARAS_QUERY:
		resp_ret = Net_Dev_Paras_Query((void*)msg);
		break;
	case CMD_DEVICES_MODES_QUERY:
		resp_ret = Net_Dev_Modes_Query((void*)msg);		
		break;
	case CMD_DEVICE_QUERY://设备状态查询
		resp_ret = Net_Dev_Query((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ADD:							//设备添加
		resp_ret = Net_Dev_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_UPDATE:						//设备修改
		resp_ret = Net_Dev_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_DEL:							//设备删除
		resp_ret = Net_Dev_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_PRIATTR_UPDATE:				//设备采集属性修改
		resp_ret = Net_Dev_Attr_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ACTION_UPDATE:				//设备动作属性修改
		resp_ret = Net_Dev_Action_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
    case CMD_ONTIME_TASK_ADD:                       //定时任务添加
        resp_ret = Net_Time_Task_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
        break;
    case CMD_ONTIME_TASK_UPDATE:                    //定时任务修改
        resp_ret = Net_Time_Task_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
        break;
    case CMD_ONTIME_TASK_DEL:                       //定时任务删除
        resp_ret = Net_Time_Task_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
        break;
	case CMD_DEVICE_AGENT_ADD:
		resp_ret = Net_Dev_Agent_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_AGENT_UPDATE:
		resp_ret = Net_Dev_Agent_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_AGENT_DEL:
		resp_ret = Net_Dev_Agent_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
//	case CMD_SYS_CONFIG_SCH:
//		resp_ret = Net_SCH_SYS_CFG((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
//		break;
	case CMD_GSM_READ_IMSI:
		resp_ret = Net_GSM_READ_IMSI((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_ACT_SEND:							//动作下发
		{
			MSG_ACTION_HASH hash_node;
			memset((unsigned char*)&hash_node, 0, sizeof(MSG_ACTION_HASH));
			hash_node.unMsgCode = pRecMsgHdr->unMsgCode;
			hash_node.unMsgSeq = pRecMsgHdr->unMsgSeq;
			hash_node.unStatus = pRecMsgHdr->unStatus;
			hash_node.unRspTnlId = pRecMsgHdr->unRspTnlId;

			memcpy(hash_node.szReserve, (unsigned char*)pRecMsgHdr + MSGHDRLEN, 20);
			memcpy(hash_node.szStatus, (unsigned char*)pRecMsgHdr + MSGHDRLEN + 20, 4);
			memcpy(hash_node.szDeal, (unsigned char*)pRecMsgHdr + MSGHDRLEN + 24, 4);

			hash_node.Id = GetSeq();
			DBG(("Action Seq [%d] tnlId[%d]\n",hash_node.Id, hash_node.unRspTnlId));
			hash_node.source = ACTION_SOURCE_NET_CPM;
			time(&hash_node.act_time);
			if (g_MsgCtlSvrBoa.addNode(hash_node))//插入到待回应列表，lhy
			{
				resp_ret = Net_Dev_Action((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, hash_node.Id, ACTION_SOURCE_NET_CPM);
			}
			else
			{
				resp_ret = SYS_STATUS_FAILED;
			}
			if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != resp_ret)
			{
				memcpy(pRespMsg, pRcvMsg, 28);
				resp_len = 28;
				char szRet[5] = {0};
				sprintf(szRet, "%04d", resp_ret);
				memcpy(pRespMsg + 20, szRet, 4);
				
				//从回应表中删除
				GetAndDeleteHashData(hash_node);
			}			
			DBG(("action end.tm_TimeStart[%ld] ret[%d]\n", tm_TimeStart, resp_ret));
		}
		break;
	default:
		resp_ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
		break;
	}
	//g_DevInfoContainer.PrintPollHash();
	if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != resp_ret && SYS_STATUS_FAILED_NO_RESP != resp_ret)
	{
		pRespMsgHdr->unMsgLen = MSGHDRLEN + resp_len;

		pRespMsgHdr->unStatus = pRecMsgHdr->unStatus;
		pRespMsgHdr->unMsgSeq = pRecMsgHdr->unMsgSeq;
		pRespMsgHdr->unRspTnlId = pRecMsgHdr->unRspTnlId;

		time_t tm_TimeEnd = 0;
		time(&tm_TimeEnd); 
		DBG(("CPM ------TimeProc[%ld] - [%ld] = [%ld]----------[%d]---------------------> CPM Plat \n", tm_TimeEnd, tm_TimeStart, tm_TimeEnd - tm_TimeStart, g_ulHandleNetBoaSvrSend));
      //  DBG(("Len:[%d]  Status:[%d]  Seq:[%d]  TnlId:[%d]\n", pRespMsgHdr->unMsgLen, pRespMsgHdr->unStatus, pRespMsgHdr->unMsgSeq, pRespMsgHdr->unRspTnlId));
		DBG(("%s\n", pRespMsg));

		if (!SendMsg(g_ulHandleNetBoaSvrSend, MUCB_IF_NET_SVR_SEND_BOA, resp_msg))
        {
		    DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetBoaSvrSend));
        }
	}

	return true;
}

bool CBOAMsgCtlSvr::addNode(MSG_ACTION_HASH hash_node)
{
    bool ret = false;
    MSG_ACTION_HASH msg_node;
    HashedActInfo::iterator it;

    memset((BYTE*)&msg_node, 0, sizeof(MSG_ACTION_HASH));
    memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(MSG_ACTION_HASH));

	CTblLock.lock();
	it = m_ptrActInfoTable->find(msg_node.Id);
	if (it == m_ptrActInfoTable->end())
	{
		m_ptrActInfoTable->insert(HashedActInfo::value_type((TKU32)msg_node.Id, msg_node));
		ret = true;
	}
	CTblLock.unlock();

    return ret;
}

bool CBOAMsgCtlSvr::GetAndDeleteHashData(MSG_ACTION_HASH& hash_node)
{
	bool ret = false;
	HashedActInfo::iterator it;
	time_t tTime;
	time(&tTime);

	CTblLock.lock();
	it = m_ptrActInfoTable->find((TKU32)hash_node.Id);
	if (it != m_ptrActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id, sizeof(MSG_ACTION_HASH));
		m_ptrActInfoTable->erase(it);
		ret = true;
		DBG(("-----------------GetAndDeleteHashData time elipse[%ld] - [%ld] = [%ld]\n", tTime, hash_node.act_time, tTime - hash_node.act_time));
	}
	CTblLock.unlock();
	return ret;
}

bool CBOAMsgCtlSvr::SVR_Terminate_Func (CBOAMsgCtlSvr *_this, char *msg)
{
    _this->doSVR_Terminate_Func(msg);
    return true;
}
bool CBOAMsgCtlSvr::doSVR_Terminate_Func (char *msg)
{
    DBG(("Connect terminate...\n"));
	return true;
}

void CBOAMsgCtlSvr::MD5(unsigned char *szBuf, unsigned char *src, unsigned int len)
{
	MD5_CTX context;

	MD5Init(&context);
	MD5Update(&context, src, len);
	MD5Final(szBuf, &context);
}

bool CBOAMsgCtlSvr::Dispatch(CBOAMsgCtlSvr *_this, char *pMsg, char *pBuf, AppInfo *stAppInfo, BYTE Mod)
{		
  return 	true;	
}
/****************************************终端SVR***************************************************/
typedef  tr1::unordered_map<string, ApmInfo> HashedApmInfo;
HashedApmInfo  ApmInfoTable;

class LockApm
{
private:
	pthread_mutex_t	 m_lock;
public:
	LockApm() { pthread_mutex_init(&m_lock, NULL); }
	bool lock() { return pthread_mutex_lock(&m_lock)==0; }
	bool unlock() { return pthread_mutex_unlock(&m_lock)==0; }
} CTblLockApm;

bool CTMNMsgCtlSvr::Initialize(TKU32 unInputMask,const char *szCtlName)
{
	if(false == CMsgCtlBase::Initialize(unInputMask, szCtlName)) 
		return false;

	m_Seq = 0;

	AddFunc(COMM_SUBMIT,	(CMDFUNC)SVR_Submit_Func);  	// 
	AddFunc(COMM_DELIVER,	(CMDFUNC)SVR_Submit_Func);  	// 
	AddFunc(COMM_ERR,		(CMDFUNC)SVR_Err_Func);
	AddFunc(COMM_TERMINATE, (CMDFUNC)SVR_Terminate_Func);

	m_id = 0;

//	m_ptrActInfoTable = new HashedActInfo();

	//数据处理线程
	char buf[256] = {0};
	sprintf(buf, "%s %s", "CTMNMsgCtlSvr", "ActCtrlThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CTMNMsgCtlSvr::pActCtrlThrd, true,this))
	{
		DBG(("pActCtrlThrd Create failed!\n"))
			return false;
	}

	return true;   		
}

//数据处理线程
void * CTMNMsgCtlSvr::pActCtrlThrd(void* arg)
{
	CTMNMsgCtlSvr *_this = (CTMNMsgCtlSvr *)arg;
	_this->MsgActCtrl();
	return NULL;
}
void CTMNMsgCtlSvr::MsgActCtrl()
{
	QUEUEELEMENT pMsg = NULL;
//	MSG_ACTION_HASH hash_node;
//	char send_msg[1024] = {0};
//	MsgHdr *pSendMsgHdr = (MsgHdr *)send_msg;
//	MsgRespToPl* pPlRespMsg = (MsgRespToPl*)&send_msg[MSGHDRLEN];

	printf("CTMNMsgCtlSvr::MsgActCtrl Start ....Pid[%d]\n", getpid());
	while(true)
	{
		pthread_testcancel();

		if(MMR_OK != g_MsgMgr.GetFirstMsg(g_ulHandleTmnMsgCtrl, pMsg, 10))      //从分析队列中取数据
		{
			usleep(200*1000);
			continue;
		}

		do 
		{
			
		} while (false);
		if (NULL != pMsg)
		{
			MM_FREE(pMsg);
			pMsg = NULL;
		}
	}
}

/******************************************************************************/
/* Function Name  : SVR_Submit_Func                                        */
/* Description    : 指令任务处理                                              */
/* Input          : char* msg       指令及信息                                */
/* Output         : none                                                      */
/* Return         : true                                                      */
/******************************************************************************/
bool CTMNMsgCtlSvr::SVR_Submit_Func(CTMNMsgCtlSvr *_this, char *msg)
{
	_this->doSVR_Submit_Func(msg);
	return true;
}

bool CTMNMsgCtlSvr::doSVR_Submit_Func(char *msg)
{
	DBG(("CPM Terminal -------------------------------------> CPM[%s]\n", msg));
	MsgHdr *pMsgHdr = (MsgHdr *)msg;
	PMsgNetDataDeliver pTMsgHdr = (PMsgNetDataDeliver)(msg + MSGHDRLEN);	
	bool bLogAuth = false;

	ApmInfo stApmInfo ;
	HashedApmInfo::iterator itApm;

	char loginId[11] = {0};
	strncpy(loginId, (char*)pTMsgHdr->szPid, 10);
	memset(&stApmInfo, 0, sizeof(ApmInfo));
	strncpy(stApmInfo.szApmId, loginId, 10);

	CTblLockApm.lock();
	//找到先删除
	itApm = ApmInfoTable.find(stApmInfo.szApmId);
	if (itApm != ApmInfoTable.end())
	{
		ApmInfoTable.erase(itApm);
	}

	//登陆验证
	DEVICE_DETAIL_INFO stDevDetailInfo;
	memset((unsigned char*)&stDevDetailInfo, 0, sizeof(DEVICE_DETAIL_INFO));
	memcpy(stDevDetailInfo.Id, loginId, 10);	
	if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(stDevDetailInfo))
	{
		DBG(("stDevDetailInfo.Id[%s] Name[%s] szPwd[%s]\n", stDevDetailInfo.Id, stDevDetailInfo.szCName, stDevDetailInfo.szPwd));
		if(ApmValid(pTMsgHdr->szReserve, pTMsgHdr->szDeal, pTMsgHdr->szStatus, pTMsgHdr->szPid, stDevDetailInfo.szPwd, pTMsgHdr->szMd5))//lsd 密码存放方式
		{
			stApmInfo.nClientId = pMsgHdr->unRspTnlId;	
			bLogAuth = true;
			ApmInfoTable.insert(HashedApmInfo::value_type(stApmInfo.szApmId,stApmInfo));
			DBG(("HUB/MsgCtl/CMsgCtlSvr:::ApmId = [%s], nClientId=[%d] Connected!\n",stApmInfo.szApmId,stApmInfo.nClientId));
		}
		else
		{
			DBG(("HUB/MsgCtl/CMsgCtlSvr::ApmValid Failed:ApmId = [%s], nClientId=[%d] Valid Failed!\n",stApmInfo.szApmId,stApmInfo.nClientId));
			pMsgHdr-> unMsgCode = COMM_TERMINATE;
			pMsgHdr -> unMsgLen = MSGHDRLEN;
			SendMsg(g_ulHandleNetTmnSvrSend, MUCB_IF_NET_SVR_SEND_TMN, msg);
		}
	}
	CTblLockApm.unlock();

	if (bLogAuth)		//登陆验证通过
	{
		char szOutBuf[MAXMSGSIZE] = {0};
		int iOutLen = 0;
		char szCmd[5] = {0};
		memcpy(szCmd, pTMsgHdr->szDeal, 4);
		int iCmd = atoi(szCmd);

		AddMsg((unsigned char*)msg, pMsgHdr->unMsgLen);

		switch(iCmd)
		{
		case NET_TERMINAL_CMD_DATA_DELEIVE://采集回应
			g_NetContainer.actionRespProc(msg + MSGHDRLEN, pMsgHdr->unMsgLen - MSGHDRLEN);
		case NET_TERMINAL_CMD_DATA_SUBMIT://数据上传
			if(g_NetContainer.updateNetData(msg + MSGHDRLEN, pMsgHdr->unMsgLen - MSGHDRLEN, szOutBuf, iOutLen))
			{
				if (0 < iOutLen)
				{
					//向相应网络端口发送数据			
					char updateData[2048] = {0};
					memset(updateData, 0, sizeof(updateData));

					//组报文头
					pMsgHdr->unMsgCode = COMM_DELIVER;
					memcpy(updateData, pMsgHdr, sizeof(MsgHdr));
					memcpy(updateData + sizeof(MsgHdr), szOutBuf, iOutLen);

					if (!SendMsg_Ex(g_ulHandleNetTmnSvrSend, MUCB_IF_NET_SVR_SEND_TMN, (char*)updateData, sizeof(MsgHdr) + iOutLen))
					{
						DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetBoaSvrSend));
					}
				}
			}
			break;
		case NET_TERMINAL_CMD_ACTION_DELEIVE://执行回应
			g_NetContainer.actionRespProc(msg+MSGHDRLEN, pMsgHdr->unMsgLen - MSGHDRLEN);
			break;
		case NET_TERMINAL_CMD_ACTIVE_TEST://执行回应
			DBG(("链路测试\n"));
			break;
		default:
			DBG(("未知数据指令\n"));
			break;
		}
	}
	else
	{
		DBG(("doSVR_Terminal_Sub_Func  [%s] 登陆验证失败\n", stApmInfo.szApmId));
	}

	return true;
}

bool CTMNMsgCtlSvr::ApmValid(char* Reserve, char* Deal, char* D_Status, char *szApmId, char *szPwd, BYTE *btAuth)
{
	unsigned char md5_input[80]={0};
	unsigned char md5_output[16]={0};
	int len=0, i=0;	

	/* 生成md5验证码 */
	len = 20;
	memcpy(md5_input+i, Reserve, len);			//Reserve 
	i += 20;

	len = 4;
	memcpy(md5_input+i, Deal, len);				//Deal 
	i += 4;

	len = 4;
	memcpy(md5_input+i, D_Status, len);			//D_Status 
	i += 4;

	len = 10;		// 不包括结束符'\0'
	memcpy(md5_input+i, szApmId, len);			//szApmId 
	i += 10;

	len	= 6;		// 不包括结束符'\0'
	memcpy(md5_input+i, szPwd, len);			//szPwd
	i += 6;

	MD5(md5_output, md5_input, i);
	char outString[33] = {0};
	char* ptrMd5Hex = outString;
	for(int i=0; i<16; i++)
	{
		sprintf(ptrMd5Hex, "%02X", md5_output[i]);
		ptrMd5Hex += 2;
	}	
	//DBG(("Reserve[%s] Deal[%s] D_Status[%s] ApmId[%s] Pwd[%s] FromMD5[%s] MyMD5[%s]\n", Reserve, Deal, D_Status, szApmId, szPwd, (char*)btAuth, outString));
	if (0 == strncmp(outString, (char*)btAuth, 32))
	{
		return true;
	}
	/*
	if(memcmp(md5_output,btAuth,16)== 0)
	{
	return true;
	}
	*/
	else
	{
		return false;
	}
}
/*
bool CTMNMsgCtlSvr::addNode(MSG_ACTION_HASH hash_node)
{
	bool ret = false;
	MSG_ACTION_HASH msg_node;
	HashedActInfo::iterator it;

	memset((BYTE*)&msg_node, 0, sizeof(msg_node));
	memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(MSG_ACTION_HASH));

	CTblLock.lock();
    it = m_ptrActInfoTable->find(msg_node.Id);
    if (it == m_ptrActInfoTable->end())
    {
		m_ptrActInfoTable->insert(HashedActInfo::value_type(msg_node.Id,msg_node));
        ret = true;
	}
	CTblLock.unlock();
	return ret;
}

bool CTMNMsgCtlSvr::GetAndDeleteHashData(MSG_ACTION_HASH& hash_node)
{
	bool ret = false;
	HashedActInfo::iterator it;
	time_t tTime;
	time(&tTime);

	CTblLock.lock();

	it = m_ptrActInfoTable->find(hash_node.Id);
	if (it != m_ptrActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id, sizeof(MSG_ACTION_HASH));
		m_ptrActInfoTable->erase(it);
		ret = true;
		DBG(("-----------------GetAndDeleteHashData time elipse[%ld] - [%ld] = [%ld]\n", tTime, hash_node.act_time, tTime - hash_node.act_time));
	}
	CTblLock.unlock();
	return ret;
}

bool CTMNMsgCtlSvr::GetHashData(MSG_ACTION_HASH& hash_node)
{
	bool ret = false;
	HashedActInfo::iterator it;
	CTblLock.lock();
    
    it = m_ptrActInfoTable->find(hash_node.Id);
    if (it != m_ptrActInfoTable->end())
	{
        memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id, sizeof(MSG_ACTION_HASH));
        ret = true;
    }
	CTblLock.unlock();
	return ret;
}
*/
bool CTMNMsgCtlSvr::GetSocketId (char* channelId, TKU32& unClientId)
{
	bool ret = false;
	ApmInfo stApmInfo ;
	HashedApmInfo::iterator itApm;
	CTblLockApm.lock();
	memset(&stApmInfo, 0, sizeof(ApmInfo));
	//strcpy(stApmInfo.szApmId, rtrim(channelId, strlen(channelId)));
	sprintf(stApmInfo.szApmId, "%s", channelId);
	itApm = ApmInfoTable.find(stApmInfo.szApmId);
	if (itApm != ApmInfoTable.end())
	{		
		unClientId = itApm->second.nClientId;
		ret = true;
	}
	DBG(("CMsgCtlSvr::GetSocketId apmId[%s] socketId[%d]\n", stApmInfo.szApmId, unClientId));
	CTblLockApm.unlock();
	return ret;
}

bool CTMNMsgCtlSvr::SVR_Terminate_Func (CTMNMsgCtlSvr *_this, char *msg)
{
	_this->doSVR_Terminate_Func(msg);
	return true;
}
bool CTMNMsgCtlSvr::doSVR_Terminate_Func (char *msg)
{
	DBG(("Connect terminate...\n"));
	return true;
}

bool CTMNMsgCtlSvr::SVR_Err_Func (CTMNMsgCtlSvr *_this, char *msg)
{
	_this->doSVR_Err_Func(msg);
	return true;
}
bool CTMNMsgCtlSvr::doSVR_Err_Func (char *msg)
{

	MsgHdr *pMsgHdr = (MsgHdr *)msg;
	Msg *pTMsg = (Msg *)(msg + MSGHDRLEN);
	char OmsMsg[1024] = {0};
	//if(pTMsg->stAlarm.btClass == ERR_CRITICAL )
	{
		switch(pTMsg->stAlarm.btErrNo )
		{
		case STA_TIMEOUT:
			DBG(("STA_TIMEOUT  Error ..socketId[%d]\n", (int)pMsgHdr->unRspTnlId));
			break;
		case STA_IF_DOWN:
			{
				sprintf(OmsMsg,"[%s],nClientId=[%d] 断开!\n",pTMsg->stAlarm.szModName,(int)pMsgHdr->unRspTnlId);
				DBG(("%s", OmsMsg));

				ApmInfo* pstApmInfo = NULL;
				HashedApmInfo::iterator it;

				char clientId[21] = {0};

				CTblLockApm.lock();
				for (it = ApmInfoTable.begin(); it != ApmInfoTable.end(); it++)
				{
					pstApmInfo = (ApmInfo*)(&it->second);
					//printf("Client Id[%d] pMsgHdr->unRspTnlId[%d] pstApmInfo->szApmId[%s]\n", pstApmInfo->nClientId, pMsgHdr->unRspTnlId, pstApmInfo->szApmId);
					if (pstApmInfo->nClientId == (int)pMsgHdr->unRspTnlId)
					{
						strcpy(clientId, pstApmInfo->szApmId);
						break;
					}
				}
				printf("Client [%s] 断开\n", clientId);
				if(0 < strlen(clientId))
				{
					ApmInfo hash_node;
					memset((unsigned char*)&hash_node, 0, sizeof(ApmInfo));
					strcpy(hash_node.szApmId, clientId);

					it = ApmInfoTable.find(hash_node.szApmId);	
					if (it != ApmInfoTable.end())
					{
						ApmInfoTable.erase(it);
					}
				}

				CTblLockApm.unlock();
				//lhy 发起设备离线				
				//g_DevInfoContainer.SetDeviceLineStatus(clientId, false);
			}
			break;
		default:
			break;	
		}	
	}
	return true;		
}

void CTMNMsgCtlSvr::MD5(unsigned char *szBuf, unsigned char *src, unsigned int len)
{
	MD5_CTX context;

	MD5Init(&context);
	MD5Update(&context, src, len);
	MD5Final(szBuf, &context);
}

bool CTMNMsgCtlSvr::Dispatch(CTMNMsgCtlSvr *_this, char *pMsg, char *pBuf, AppInfo *stAppInfo, BYTE Mod)
{
	return 	true;	
}


/*******************************************************************************************************/

CMsgCtlClient::~CMsgCtlClient(void)
{

}

bool CMsgCtlClient::Initialize(unsigned int ulInputMask,const char *szCtlName)
{   
	if(false == CMsgCtlBase::Initialize(ulInputMask,szCtlName))
		return false;

    AddFunc(COMM_SUBMIT,	(CMDFUNC)NetFuncSubmit);  	// 0004上行
	AddFunc(COMM_DELIVER,	(CMDFUNC)NetFuncDeliver);  	// 0005转发
	AddFunc(COMM_ERR,		(CMDFUNC)NetErr);           // 0009错误报告

	return true;
}

bool CMsgCtlClient::NetFuncSubmit(CMsgCtlClient *_this, char *msg)
{		
	return true;
}

bool CMsgCtlClient::NetFuncDeliver(CMsgCtlClient *_this, char *msg)
{
	DBG(("Platform -------------------------------------> CPM\n"));

	time_t tm_TimeStart = 0;
	time(&tm_TimeStart); 

	//通信包指针
	MsgHdr *pRecMsgHdr = (MsgHdr *)msg;
	DBG(("%s\n", msg + MSGHDRLEN));
	//业务包指针
	PMsgRecvFromPl pRcvMsg = (PMsgRecvFromPl)(msg + MSGHDRLEN);

	//回应缓存
	char resp_msg[MAXMSGSIZE] = {0};
	MsgHdr *pRespMsgHdr = (MsgHdr *)resp_msg;
	char* pRespMsg = resp_msg + MSGHDRLEN;
	memcpy(resp_msg, msg, MSGHDRLEN);

	int resp_len = 0;
	int resp_ret = SYS_STATUS_FAILED;

	char deal[5] = {0};
	memcpy( deal, pRcvMsg->szDeal, 4);

	switch(atoi(deal))
	{
	case CMD_DEVICE_STATUS_QUERY:
		resp_ret = Net_Dev_Real_Status_Query((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;	
	case CMD_DEVICES_STATUS_QUERY:
		resp_ret = Net_Dev_Status_Query((void*)msg);
		break;
	case CMD_DEVICES_PARAS_QUERY:
		resp_ret = Net_Dev_Paras_Query((void*)msg);
		break;
	case CMD_DEVICES_MODES_QUERY:
		resp_ret = Net_Dev_Modes_Query((void*)msg);		
		break;
	case CMD_DEVICE_QUERY://设备状态查询
		resp_ret = Net_Dev_Query((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ADD:							//设备添加
		resp_ret = Net_Dev_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_UPDATE:						//设备修改
		resp_ret = Net_Dev_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_DEL:							//设备删除
		resp_ret = Net_Dev_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_PRIATTR_UPDATE:				//设备采集属性修改
		resp_ret = Net_Dev_Attr_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ACTION_UPDATE:
		resp_ret = Net_Dev_Action_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_ONTIME_TASK_ADD:                       //定时任务添加
		resp_ret = Net_Time_Task_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_ONTIME_TASK_UPDATE:                    //定时任务修改
		resp_ret = Net_Time_Task_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_ONTIME_TASK_DEL:                       //定时任务删除
		resp_ret = Net_Time_Task_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_AGENT_ADD:
		resp_ret = Net_Dev_Agent_Add((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_AGENT_UPDATE:
		resp_ret = Net_Dev_Agent_Update((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_AGENT_DEL:
		resp_ret = Net_Dev_Agent_Del((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_SYN:
		resp_ret = Net_Dev_Syn((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ATTR_SYN:
		resp_ret = Net_Dev_Attr_Syn((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;
	case CMD_DEVICE_ACT_SYN:
		resp_ret = Net_Dev_Act_Syn((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
		break;		
//	case CMD_SYS_CONFIG_SCH:
//		resp_ret = Net_SCH_SYS_CFG((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, pRespMsg, resp_len);
//		break;
	case CMD_ACT_SEND:							//动作下发
		{
			MSG_ACTION_HASH hash_node;
			memset((unsigned char*)&hash_node, 0, sizeof(MSG_ACTION_HASH));
			hash_node.unMsgCode = pRecMsgHdr->unMsgCode;
			hash_node.unMsgSeq = pRecMsgHdr->unMsgSeq;
			hash_node.unStatus = pRecMsgHdr->unStatus;
			hash_node.unRspTnlId = pRecMsgHdr->unRspTnlId;

			memcpy(hash_node.szReserve, (unsigned char*)pRecMsgHdr + MSGHDRLEN, 20);
			memcpy(hash_node.szStatus, (unsigned char*)pRecMsgHdr + MSGHDRLEN + 20, 4);
			memcpy(hash_node.szDeal, (unsigned char*)pRecMsgHdr + MSGHDRLEN + 24, 4);

			hash_node.Id = g_MsgCtlSvrBoa.GetSeq();
			hash_node.source = ACTION_SOURCE_NET_PLA;
			time(&hash_node.act_time);
			DBG(("data[%s] Seq[%d], dealCmd[%s] tnlId[%d]\n", (unsigned char*)pRecMsgHdr + MSGHDRLEN, hash_node.Id, hash_node.szDeal, hash_node.unRspTnlId));
			if (g_MsgCtlSvrBoa.addNode(hash_node))//插入到待回应列表
			{
				resp_ret = Net_Dev_Action((char*)pRcvMsg, pRecMsgHdr->unMsgLen - MSGHDRLEN, hash_node.Id, ACTION_SOURCE_NET_PLA);
			}
			else
			{
				resp_ret = SYS_STATUS_FAILED;
			}
			if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != resp_ret)
			{
				memcpy(pRespMsg, pRcvMsg, 28);
				resp_len = 28;
				char szRet[5] = {0};
				sprintf(szRet, "%04d", resp_ret);
				memcpy(pRespMsg + 20, szRet, 4);

				g_MsgCtlSvrBoa.GetAndDeleteHashData(hash_node);
			}
		}
		break;
	case CMD_TRANSPARENT_DATA_UP://数据上传回应
		resp_ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
		g_CTransparent.Net_Resp_Deal(pRcvMsg);
		break;
	default:
		resp_ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
		break;
	}	
	if ( SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != resp_ret)//不需等待回应
	{
		pRespMsgHdr->unMsgLen = MSGHDRLEN + resp_len;

		pRespMsgHdr->unStatus = pRecMsgHdr->unStatus;
		pRespMsgHdr->unMsgSeq = pRecMsgHdr->unMsgSeq;
		pRespMsgHdr->unRspTnlId = pRecMsgHdr->unRspTnlId;
		time_t tm_TimeEnd = 0;
		time(&tm_TimeEnd); 
		DBG(("CPM ------Time[%ld]-------------------------------> Platform\n", tm_TimeEnd - tm_TimeStart));
		DBG(("%s\n", pRespMsg));
		if (!SendMsg(g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND, resp_msg))
		{
			DBG(("SendMsg_Ex Failed: handle[%d] mucb[%d]\n", g_ulHandleNetCliSend, MUCB_IF_NET_CLI_SEND));
		}
		else
		{
			DBG(("MsgCtl.cpp 1008 [%d] \n", g_ulHandleNetCliSend));
		}
	}
	return true;
}


bool CMsgCtlClient::NetErr(CMsgCtlClient *_this, char *msg)
{	
	return true;		
}
