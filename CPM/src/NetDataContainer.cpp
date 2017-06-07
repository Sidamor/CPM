/************************************************************************************************/
/*                NetDataContainer.cpp              2011.07.15                                  */
/************************************************************************************************/
#include <sys/time.h>
#include <stdio.h>
#include "Define.h"
#include "NetDataContainer.h"
#include "SqlCtl.h"
#include "LabelIF.h"

CNetDataContainer::CNetDataContainer()
{
	m_IsInitialized = false;
}

CNetDataContainer::~CNetDataContainer()
{
}

bool CNetDataContainer::Initialize()
{
    m_pNetDataHashTable = new HashedNetDataInfo();
    if (!m_pNetDataHashTable)
	{
		return false;
	}

	m_HashTableActResp = new HashedDeviceInfoACTTERMINALRESP();
	if (NULL == m_HashTableActResp)
	{
		return false;
	}

	m_IsInitialized = true;
	InitDeviceInfo();
	//数据处理线程
	char buf[256] = {0};
	sprintf(buf, "%s %s", "CNetDataContainer", "ActCtrlThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CNetDataContainer::ActCtrlThrd, true,this))
	{
		DBG(("CNetDataContainer ActCtrlThrd Create failed!\n"))
			return false;
	}


	DBG(("CNetDataContainer::Initialize success!\n "));
    return true;
}

//指令执行守护线程
void * CNetDataContainer::ActCtrlThrd(void* arg)
{
	CNetDataContainer *_this = (CNetDataContainer *)arg;
	_this->ActCtrlThrdProc();
	return NULL;
}

void CNetDataContainer::ActCtrlThrdProc()
{
	time_t tm_NowTime;
	struct tm *st_tmNowTime;
	struct timeval tmval_NowTime;
	printf("CNetDataContainer::ActCtrlThrdProc Start .... Pid[%d]\n", getpid());
	sleep(1);
	while(true)
	{
		pthread_testcancel();

		//动作回应超时处理
		int cnt = 0;
		char seqBuf[4096][21] = {{0}};
		HashedDeviceInfoACTTERMINALRESP::iterator it;
		ACTION_MSG_WAIT_TERMINAL_RESP_HASH hash_node;		
		memset((unsigned char*)&hash_node, 0, sizeof(ACTION_MSG_WAIT_RESP_HASH));

		CLockActionResp.lock();
		for (it = m_HashTableActResp->begin(); it != m_HashTableActResp->end(); it++)
		{
			time_t tm_NowTime;
			time(&tm_NowTime);
			if( tm_NowTime - it->second.act_time >= 30 )
			{
				strncpy(seqBuf[cnt], it->second.Id, 20);
				cnt++;
				DBG(("终端网络动作超时[%s]\n", it->second.Id));
			}
			if(cnt >= 4096)
				break;
		}
		CLockActionResp.unlock();

		for( int i=0; i < cnt; i++ )
		{
			memset((unsigned char*)&hash_node, 0, sizeof(hash_node));
			strncpy(hash_node.Id, seqBuf[i], 20);

			if( !GetAndDeleteHashData(hash_node))
			{
				continue;
			}
			CDevInfoContainer::SendToActionSource(hash_node.stActionMsg, SYS_STATUS_SUBMIT_SUCCESS);
		}

		//网络型传感器轮询
		do
		{
			//获取当前时间
			time(&tm_NowTime);    		
			gettimeofday(&tmval_NowTime, NULL);
			st_tmNowTime = localtime(&tm_NowTime);

			DEVICE_INFO_POLL deviceNode;
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetNextPollAttrNode(50, m_szCurDeviceAttrId, deviceNode))//获取轮巡结点
			{
				//DBG(("NetDataContainer::Thrd GetNextPollAttrNode失败\n"));
				memset(m_szCurDeviceAttrId, 0, sizeof(m_szCurDeviceAttrId));
				break;
			}
			memcpy(m_szCurDeviceAttrId, deviceNode.ChannelNextId, DEVICE_ATTR_ID_LEN);
			m_szCurDeviceAttrId[DEVICE_ATTR_ID_LEN] = '\0';

			//更新设备表中的当前模式
			DEVICE_DETAIL_INFO stDeviceDetail;
			memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
			memcpy(stDeviceDetail.Id, deviceNode.Id, DEVICE_ID_LEN);
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))
				break;
			if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
				break;
			PDEVICE_INFO_POLL pPollNode = &deviceNode;
			//是否属性无需采集
			if ('0' == pPollNode->cBeOn)//属性未启用
			{
				break;
			}
			//是否在布防时间
			if (false == CheckTime(st_tmNowTime, *pPollNode))
			{
				break;
			}
			//判断是否到了轮询时间
			if ((0 == pPollNode->Frequence 
				|| (unsigned long)((tmval_NowTime.tv_sec - pPollNode->stLastValueInfo.LastTime.tv_sec)*1000
				+ (tmval_NowTime.tv_usec - pPollNode->stLastValueInfo.LastTime.tv_usec)/1000) < pPollNode->Frequence)
				)//开关量持续采集
			{
				break;
			}

			//是虚拟属性
			if (DEVICE_ATTR_ID_LEN == strlen(pPollNode->V_Id))
			{
				bool bSendMsg = false;
				bool bDeviceStatus = false;
				QUERY_INFO_CPM stQueryResult;
				LAST_VALUE_INFO stLastValueInfo;
				memset((unsigned char*)&stLastValueInfo, 0, sizeof(LAST_VALUE_INFO));
				stQueryResult.Cmd = 0x00;
				strncpy(stQueryResult.DevId, pPollNode->Id, DEVICE_ATTR_ID_LEN);
				Time2Chars(&tm_NowTime, stQueryResult.Time);
				DBG(("是虚拟属性[%s] 对应属性[%s]\n", pPollNode->Id, pPollNode->V_Id));
				int iRet = g_DevInfoContainer.GetAttrValue(pPollNode->V_Id, stLastValueInfo);

				if (SYS_STATUS_SUCCESS == iRet &&  0 != strcmp(stLastValueInfo.Value, "NULL"))
				{
					memcpy(stQueryResult.Type, stLastValueInfo.Type, 4);
					stQueryResult.VType = stLastValueInfo.VType;
					memcpy(stQueryResult.Value, stLastValueInfo.Value, sizeof(stQueryResult.Value));
					bDeviceStatus = true;//设备正常

					stQueryResult.DataStatus = LabelIF_StandarCheck(stQueryResult.VType, stQueryResult.Value, pPollNode->szStandard)?0:1;

					//校验并解析成功,置最新数据
					memcpy(pPollNode->stLastValueInfo.Type, stQueryResult.Type, 4);
					pPollNode->stLastValueInfo.VType = stQueryResult.VType;
					memcpy(pPollNode->stLastValueInfo.Value, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));
					memcpy(pPollNode->stLastValueInfo.szValueUseable, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));

					pPollNode->stLastValueInfo.DataStatus = stQueryResult.DataStatus;
					bSendMsg = true;

				}
				else
				{
					pPollNode->stLastValueInfo.DataStatus = 1;

					pPollNode->stLastValueInfo.VType = CHARS;
					memset(pPollNode->stLastValueInfo.Value, 0, sizeof(pPollNode->stLastValueInfo.Value));
					memcpy(pPollNode->stLastValueInfo.Value, "NULL", 4);
				}

				//设备状态更新检测
				g_DevInfoContainer.SetDeviceLineStatus(pPollNode->Id, bDeviceStatus);

				if (bSendMsg)
				{
					if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryResult, sizeof(QUERY_INFO_CPM)))
					{
						DBG(("DeviceIfBase DecodeAndSendToAnalyser 1 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
					}
					else
					{
						DBG(("E----->A   devid[%s] time[%s] value[%s] vtype[%d]\n", stQueryResult.DevId, stQueryResult.Time, stQueryResult.Value, stQueryResult.VType));
					}
				}
				memcpy((unsigned char*)(&(deviceNode.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				g_DevInfoContainer.UpdateDeviceRoll(deviceNode);
				break;
			}
			//DBG(("NetDataContainer::Thrd SetDeviceLineStatus[%ld]>[%ld]\n", (unsigned long)((tmval_NowTime.tv_sec - pPollNode->stLastValueInfo.LastTime.tv_sec)*1000 + (tmval_NowTime.tv_usec - pPollNode->stLastValueInfo.LastTime.tv_usec)/1000), deviceNode.Frequence*(deviceNode.offLineTotle - deviceNode.offLineCnt + 1)));
			/*** 1. 间隔时间 < 轮询间隔*离线次数(offTotal-offLine)	***
			 *** 2. offLineCnt --									***
			 *** 3. 主动采集										***/
			if((unsigned long)((tmval_NowTime.tv_sec - pPollNode->stLastValueInfo.LastTime.tv_sec)*1000
				+ (tmval_NowTime.tv_usec - pPollNode->stLastValueInfo.LastTime.tv_usec)/1000) 
				> deviceNode.Frequence*(deviceNode.offLineTotle - deviceNode.offLineCnt + 1))
			{
				g_DevInfoContainer.SetDeviceLineStatus(deviceNode.Id, false);
				DBG(("[%s]NET主动轮询 \n", deviceNode.Id));
				ACTION_MSG actionMsg;
				memset(&actionMsg, 0, sizeof(ACTION_MSG));
				this->PollProc(&deviceNode, actionMsg);
			}

			//记录当前编号
			char curAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
			strcpy(curAttrId, pPollNode->Id);
			while(true) 
			{
				//更新结点信息 -- 属性离线次数,轮巡时间
				memcpy((unsigned char*)(&(deviceNode.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				g_DevInfoContainer.UpdateDeviceRoll(deviceNode);

				if(0 == strlen(pPollNode->NextId) || 0 == strncmp(curAttrId, pPollNode->NextId, DEVICE_ID_LEN + DEVICE_SN_LEN))//获取相同rd配置的属性
					break;

				if (SYS_STATUS_SUCCESS != g_DevInfoContainer.GetNextAttrNode(curAttrId, pPollNode->NextId, deviceNode))
				{
					DBG(("GetNextAttrNode[%s][%s]failed\n", pPollNode->Id, pPollNode->NextId))
						break;
				}

				pPollNode = &deviceNode;
			}
		} while (false);
		sleep(5);
	}
}
bool CNetDataContainer::GetAndDeleteHashData(ACTION_MSG_WAIT_TERMINAL_RESP_HASH& hash_node)
{
	bool ret = false;
	if(!m_IsInitialized)
		return false;
	HashedDeviceInfoACTTERMINALRESP::iterator it;
	CLockActionResp.lock();

	it = m_HashTableActResp->find(hash_node.Id);
	if(m_HashTableActResp->end() != it)
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id, sizeof(ACTION_MSG_WAIT_TERMINAL_RESP_HASH));
		m_HashTableActResp->erase(it);
		ret = true;
	}
	CLockActionResp.unlock();
	return ret;
}

bool CNetDataContainer::CheckTime(struct tm *st_tmNowTime, DEVICE_INFO_POLL device_node)
{
	bool ret = false;
	time_t tm_now = 0;
	tm_now = mktime(st_tmNowTime);

	if( (device_node.startTime.tm_hour < device_node.endTime.tm_hour)         //开始、结束时间在同一天
		|| ( (device_node.startTime.tm_hour==device_node.endTime.tm_hour) && (device_node.startTime.tm_min<device_node.endTime.tm_min) ) )
	{
		if( st_tmNowTime->tm_hour > device_node.startTime.tm_hour )            //
		{
			if( st_tmNowTime->tm_hour < device_node.endTime.tm_hour )
			{
				ret = true;
			}
			else if( (st_tmNowTime->tm_hour == device_node.endTime.tm_hour) && (st_tmNowTime->tm_min <= device_node.endTime.tm_hour))
			{
				ret =true;
			}
		}
		else if( st_tmNowTime->tm_hour == device_node.startTime.tm_hour )
		{
			if( st_tmNowTime->tm_min >= device_node.startTime.tm_min )
			{
				if( st_tmNowTime->tm_hour < device_node.endTime.tm_hour )
				{
					ret = true;
				}
				else if( (st_tmNowTime->tm_hour == device_node.endTime.tm_hour) && (st_tmNowTime->tm_min <= device_node.endTime.tm_min) )
				{
					ret = true;
				}
			}
		}
	}
	else                      //开始、结束时间隔天
	{
		if( (st_tmNowTime->tm_hour > device_node.startTime.tm_hour) || (st_tmNowTime->tm_hour < device_node.endTime.tm_hour) )
		{
			ret = true;
		}
		else if( ( st_tmNowTime->tm_hour == device_node.startTime.tm_hour) && ( st_tmNowTime->tm_min >= device_node.startTime.tm_min) )
		{
			ret = true;
		}
		else if( ( st_tmNowTime->tm_hour == device_node.endTime.tm_hour) && ( st_tmNowTime->tm_min <= device_node.endTime.tm_min) )
		{
			ret = true;
		}
	}

	return ret;
}

/******************************************************************************/
/* Function Name  : InitDeviceInfo                                            */
/* Description    : 设备表初始化                                              */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CNetDataContainer::InitDeviceInfo()
{
	bool ret = false;	
	g_DevInfoContainer.InitDeviceIndex(CNetDataContainer::InitDeviceIndexCallBack, (void*)this);
	return ret;
}

int CNetDataContainer::InitDeviceIndexCallBack(void* data, PDEVICE_INFO_POLL pNode)
{
	CNetDataContainer* _this = (CNetDataContainer*)data;
	_this->InitDeviceIndexCallBackProc(pNode);
	return 1;
}

int CNetDataContainer::InitDeviceIndexCallBackProc(PDEVICE_INFO_POLL pNode)   //轮询表初始化
{
	int ret = -1;
	DBG(("InitDeviceIndexCallBackProc id[%s] atoi(pNode->Upper_Channel)[%d] INTERFACE_NET_INPUT[%d]\n", pNode->Id, atoi(pNode->Upper_Channel), INTERFACE_NET_INPUT));
	if (atoi(pNode->Upper_Channel) >= INTERFACE_NET_INPUT)
	{
		//hash表中唯一号由设备号组成
	
		char szDeviceId[11] = {0};
		memcpy(szDeviceId, pNode->Id, DEVICE_ID_LEN);
		DBG(("InitDeviceIndexCallBackProc:deviceId[%s] \n", szDeviceId));
		addNode(szDeviceId);
		ret = 1;
	}
	return ret;
}

//接收数据处理
bool CNetDataContainer::updateNetData(char *inMsg, int msgLen, char* outBuf, int& outLen)//lhy 需要添加socketId
{
	bool ret = false;
	if(!m_IsInitialized)
		return false;
	int retVal = SYS_STATUS_FAILED;
	PMsgNetDataSubmit ptrPkg = (PMsgNetDataSubmit)inMsg;
	HashedNetDataInfo::iterator it;
	MSG_NETDATA_HASH hash_node;
	memset( (unsigned char*)&hash_node, 0, sizeof(hash_node));

	memcpy(hash_node.DeviceId, ptrPkg->szTid, 10);

	MSG_NETDATA_HASH* ptfNode = NULL;
	DBG(("updateNetData selfId[%s] \n", hash_node.DeviceId));
	do 
	{
		//验证数据合法性，查找设备编号
		CTblLock.lock();
		it = m_pNetDataHashTable->find(hash_node.DeviceId);
		if(m_pNetDataHashTable->end() != it)
		{
			ptfNode = (MSG_NETDATA_HASH*)(&it->second.DeviceId);
			memcpy((unsigned char*)&hash_node, (unsigned char*)ptfNode, sizeof(MSG_NETDATA_HASH));
			ptfNode = &hash_node;
			//DBG(("updateNetData 找到对应的设备[%s]\n", hash_node.DeviceId));
			ret = true;
		}
		CTblLock.unlock();
		if (!ret)
		{
			DBG(("updateNetData 没有找到对应的设备[%s] \n", hash_node.DeviceId));
			break;
		}
		ret = false;	
		bool singlePara = true;
		char szPollId[DEVICE_ATTR_ID_LEN] = {0};
		strncpy(szPollId, ptfNode->DeviceId, DEVICE_ID_LEN);
		if (0 == strncmp(ptrPkg->szAttrId, "0000", DEVICE_SN_LEN))
		{
			strcat(szPollId, "0001");
			singlePara = false;
		}
		else
			strncat(szPollId, ptrPkg->szAttrId, DEVICE_SN_LEN);
		DBG(("updateNetData pollId[%s] msgLen[%d] inMsg[%s]\n", szPollId, msgLen, inMsg));

		AddMsg((unsigned char*)inMsg, msgLen);

		//更新结点
		if (msgLen > 0)
		{
			DEVICE_INFO_POLL deviceNode;
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetAttrNode(szPollId, deviceNode))
			{
				DBG(("updateNetData GetAttrNode 未找到对应设备采集属性 ID[%s] \n", szPollId));
				break;
			}
			PDEVICE_INFO_POLL pPollNode = &deviceNode;
			DBG(("updateNetData Attr ID[%s] Name[%s] Rck_Format[%s]\n", pPollNode->Id, pPollNode->szAttrName, pPollNode->Rck_Format));
			char curAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
			strcpy(curAttrId, pPollNode->Id);

			time_t tm_NowTime;
			time(&tm_NowTime);
			while(true)
			{
				bool bSendMsg = false;
				QUERY_INFO_CPM stQueryResult;
				QUERY_INFO_CPM_EX stQueryResultEx;
				do 
				{//解析轮巡回应数据

					DEVICE_DETAIL_INFO stDeviceDetail;
					memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
					memcpy(stDeviceDetail.Id, deviceNode.Id, DEVICE_ID_LEN);
					if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))
						break;			
					if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
						break;

					if ('0' == deviceNode.cBeOn)
					{
						break;
					}
					bool bDeviceStatus = false;
					memset((unsigned char*)&stQueryResult, 0, sizeof(QUERY_INFO_CPM));
					stQueryResult.Cmd = 0x00;//轮巡到的数据
					Time2Chars(&tm_NowTime, stQueryResult.Time);
					memcpy(stQueryResult.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

					memset((unsigned char*)&stQueryResultEx, 0, sizeof(QUERY_INFO_CPM_EX));
					stQueryResultEx.Cmd = 0x00;//轮巡到的数据
					Time2Chars(&tm_NowTime, stQueryResultEx.Time);
					memcpy(stQueryResultEx.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

					//数据格式校验
					if( LabelIF_CheckRecvFormat(pPollNode->Rck_Format, (unsigned char*)inMsg, msgLen))
					{						
						int iDecRslt = LabelIF_DecodeByFormat(pPollNode, (unsigned char*)inMsg, msgLen, &stQueryResultEx);
						if (SYS_STATUS_DATA_DECODE_NORMAL == iDecRslt)
						{	
							bDeviceStatus = true;//设备正常
							bSendMsg = true;
							memcpy((unsigned char*)&stQueryResult, (unsigned char*)&stQueryResultEx, sizeof(QUERY_INFO_CPM));
							memcpy(stQueryResult.Value, stQueryResultEx.Value, sizeof(stQueryResult.Value));
							stQueryResult.DataStatus = stQueryResultEx.DataStatus;
							DBG(("updateNetData  stQueryResult.Value[%s]\n", stQueryResult.Value));
							struct timeval tmval_NowTime;
							gettimeofday(&tmval_NowTime, NULL);
							memcpy((unsigned char*)(&(pPollNode->stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));

							//校验并解析成功,置最新数据
							memcpy(pPollNode->stLastValueInfo.Type, stQueryResult.Type, 4);
							pPollNode->stLastValueInfo.VType = stQueryResult.VType;
							memcpy(pPollNode->stLastValueInfo.Value, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));
							pPollNode->stLastValueInfo.DataStatus = stQueryResult.DataStatus;
						}
						else
						{
							if (SYS_STATUS_DATA_DECODE_UNNOMAL_AGENT == iDecRslt)
							{
								bSendMsg = true;
							}
						}
					}
					char devAttrId[15] = {0};
					memcpy(devAttrId, ptrPkg->szTid, 10);
					memcpy(devAttrId + 10, ptrPkg->szAttrId, 4);
					g_DevInfoContainer.SetDeviceLineStatus(devAttrId, bDeviceStatus);
					if (bSendMsg)
					{
						
						if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryResult, sizeof(QUERY_INFO_CPM)))
						{
							DBG(("DeviceIfBase DecodeAndSendToAnalyser  SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
						}
						else
						{
							DBG(("E----->A   devid[%s] time[%s] value[%s] vtype[%d]\n", stQueryResult.DevId, stQueryResult.Time, stQueryResult.Value, stQueryResult.VType));
						}
					}
				} while (false);
				//更新结点信息,包括属性离线次数,轮巡时间
				//memcpy((unsigned char*)(&(deviceNode.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				g_DevInfoContainer.UpdateDeviceRoll(deviceNode);
				if(singlePara || 0 == strlen(pPollNode->NextId) || 0 == strncmp(curAttrId, pPollNode->NextId, DEVICE_ID_LEN + DEVICE_SN_LEN))//获取相同rd配置的属性
					break;

				//printf(" curAttrId[%s] pPollNode->NextId[%s] pPollNode->PreId[%s]\n", curAttrId, pPollNode->NextId, pPollNode->PreId);
				if (SYS_STATUS_SUCCESS != g_DevInfoContainer.GetNextAttrNode(curAttrId, pPollNode->NextId, deviceNode))
				{
					//DBG(("GetNextAttrNode[%s][%s]failed\n", pPollNode->Id, pPollNode->NextId))
						break;
				}
				retVal = SYS_STATUS_SUCCESS;
				pPollNode = &deviceNode;
			}
			//设置回应包 
			outLen = 20 + 4 + 4 + 10 + 32;
			memcpy(outBuf, inMsg, outLen);
		}
		ret = true;		
	} while (false);
    return ret;
}


int CNetDataContainer::DoAction(ACTION_MSG actionMsg)
{
	//查找对应动作设备
	if(!m_IsInitialized)
		return SYS_STATUS_FAILED;
	char szActionId[DEVICE_ACT_ID_LEN_TEMP] = {0};
	DEVICE_INFO_ACT devNode;
	memset((unsigned char*)&devNode, 0, sizeof(DEVICE_INFO_ACT));
	strcpy(szActionId, actionMsg.DstId);
	memcpy(szActionId + DEVICE_ID_LEN, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
	int actionRet = SYS_STATUS_FAILED;

	DBG(("DoAction .DeviceId[%s].\n", szActionId));
	
	if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetActionNode(szActionId, devNode))
	{
		if ('0' != devNode.cBeOn)
		{
			actionRet = ActionProc(&devNode, actionMsg);
		}
		else
		{
			DBG(("该设备动作已撤防[%s]\n", szActionId));
		}
	}
	else
	{
		DBG(("获取设备动作属性失败[%s]\n", szActionId));
	}
	//CDevInfoContainer::SendToActionSource(actionMsg, actionRet);
	return actionRet;
}


//采集参数执行函数
int CNetDataContainer::PollProc(PDEVICE_INFO_POLL device_node, ACTION_MSG& actionMsg)
{
	if(!m_IsInitialized)
		return SYS_STATUS_FAILED;
	int ret = SYS_STATUS_FAILED;//提交成功
	int nLen = 0;

	unsigned char reqBuf[512] = {0};
	char szData[512] = {0};
	memcpy(szData, actionMsg.ActionValue, sizeof(actionMsg.ActionValue));
	memcpy(reqBuf, actionMsg.ActionValue, sizeof(actionMsg.ActionValue));

	//亮灯
	do 
	{
		if (0 < strlen(device_node->R_Format))
		{
			nLen = LabelIF_EncodeFormat(device_node->R_Format,device_node->Self_Id, NULL, szData, reqBuf, sizeof(reqBuf));
			if (0 > nLen)
			{
				ret = SYS_STATUS_ACTION_ENCODE_ERROR;
				break;
			}
		}

		//向相应网络端口发送数据			
		char updateData[1024] = {0};
		memset(updateData, 0, sizeof(updateData));

		//组报文头
		MsgHdr* pMsgHdr = (MsgHdr*)updateData;
		pMsgHdr->unMsgLen = MSGHDRLEN + 20 + 4 + 4 + 10 + 32 + 10 + 4 + 4 + nLen;
		pMsgHdr->unMsgCode = COMM_DELIVER;
		pMsgHdr->unMsgSeq = g_MsgCtlSvrTmn.GetSeq();

		DEVICE_DETAIL_INFO detailNode;
		memset(&detailNode, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(detailNode.Id, device_node->Id, 10);
		g_DevInfoContainer.GetDeviceDetailNode(detailNode);

		DEVICE_DETAIL_INFO upperNode;
		memset(&upperNode, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(upperNode.Id, detailNode.Upper, 10);
		g_DevInfoContainer.GetDeviceDetailNode(upperNode);	

		DBG(("NetPollProc DevId[%s] UpperId[%s] \n", detailNode.Id, upperNode.Id));

		if(g_MsgCtlSvrTmn.GetSocketId(detailNode.Upper, pMsgHdr->unRspTnlId))
		{
			//向相应网络端口发送数据
			char* msgUp = (char*)(updateData + MSGHDRLEN);

			char actSeq[21] = {0};
			GetActionRespTableKey(actSeq);
			memcpy(msgUp, actSeq, 20);						//Reserve
			msgUp += 20;

			sprintf(msgUp, "%04d", 2002);						//Deal
			msgUp += 4;

			sprintf(msgUp, "%04d", 0);							//Status
			msgUp += 4;

			memcpy(msgUp, upperNode.Id, 10);					//Pid, 网络终端编号
			msgUp += 10;

			unsigned char md5_input[80]={0};
			unsigned char md5_output[16]={0};
			int len=0, i=0;	

			/* 生成md5验证码 */
			len = 20;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//Reserve 
			i += 20;

			len = 4;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);				//Deal 
			i += 4;

			len = 4;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//D_Status 
			i += 4;

			len = 10;		// 不包括结束符'\0'
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//szApmId
			i += 10;

			len	= 6;		// 不包括结束符'\0'
			memcpy(md5_input+i, upperNode.szPwd, len);							//szPwd
			i += 6;

			MD5(md5_output, md5_input, i);
			char md5_hex[33] = {0};
			char* ptrMd5Hex = md5_hex;
			for(int i=0; i<16; i++)
			{
				sprintf(ptrMd5Hex, "%02X", md5_output[i]);
				ptrMd5Hex += 2;
			}
			memcpy(msgUp, md5_hex, 32);
			msgUp += 32;

			memcpy(msgUp, detailNode.Id, 10);					//Tid, 传感终端编号
			msgUp += 10;

			memcpy(msgUp, device_node->Id + DEVICE_ID_LEN, 4);	//AttrId, 属性编号
			msgUp += 4;

			*(int*)msgUp = nLen;
			msgUp += 4;

			if(nLen > 0)
			{
				memcpy(msgUp, reqBuf, nLen);
				msgUp += nLen;
			}

			if(!g_NetContainer.ActionRespTableInsert(actionMsg, actSeq))
			{
				return SYS_STATUS_FAILED;
			}


			if (!SendMsg_Ex(g_ulHandleNetTmnSvrSend, MUCB_IF_NET_SVR_SEND_TMN, (char*)pMsgHdr, pMsgHdr->unMsgLen))
			{
				DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetTmnSvrSend));
			}
			else
			{
				ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
			}
		}
		else
		{
			DBG(("CNetDataContainer::ActionProc GetSocketId Failed %s  %d\n", device_node->Upper_Channel, pMsgHdr->unRspTnlId));
		}
	} while (false);
	return ret;
}

//动作执行函数
int CNetDataContainer::ActionProc(PDEVICE_INFO_ACT device_node, ACTION_MSG& actionMsg)
{
	if(!m_IsInitialized)
		return SYS_STATUS_FAILED;
	int ret = SYS_STATUS_FAILED;//提交成功
	int nLen = 0;

	unsigned char reqBuf[512] = {0};
	char szData[512] = {0};
	memcpy(szData, actionMsg.ActionValue, sizeof(actionMsg.ActionValue));
	memcpy(reqBuf, actionMsg.ActionValue, sizeof(actionMsg.ActionValue));

	//亮灯
	do 
	{
		if (0 < strlen(device_node->W_Format))
		{
			nLen = LabelIF_EncodeFormat(device_node->W_Format,device_node->Self_Id, NULL, szData, reqBuf, sizeof(reqBuf));
			if (0 > nLen)
			{
				ret = SYS_STATUS_ACTION_ENCODE_ERROR;
				break;
			}
		}
		else
		{
			nLen = strlen(szData);
		}

		//向相应网络端口发送数据			
		char updateData[1024] = {0};
		memset(updateData, 0, sizeof(updateData));

		//组报文头
		MsgHdr* pMsgHdr = (MsgHdr*)updateData;
		pMsgHdr->unMsgLen = MSGHDRLEN + 20 + 4 + 4 + 10 + 32 + 10 + 8 + 4 + nLen;
		pMsgHdr->unMsgCode = COMM_DELIVER;
		pMsgHdr->unMsgSeq = g_MsgCtlSvrTmn.GetSeq();

		DEVICE_DETAIL_INFO detailNode;
		memset(&detailNode, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(detailNode.Id, device_node->Id, 10);
		g_DevInfoContainer.GetDeviceDetailNode(detailNode);

		DEVICE_DETAIL_INFO upperNode;
		memset(&upperNode, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(upperNode.Id, detailNode.Upper, 10);
		g_DevInfoContainer.GetDeviceDetailNode(upperNode);	

		DBG(("NetActionProc DevId[%s] UpperId[%s] \n", detailNode.Id, upperNode.Id));

		if(g_MsgCtlSvrTmn.GetSocketId(detailNode.Upper, pMsgHdr->unRspTnlId))
		{
			//向相应网络端口发送数据
			char* msgUp = (char*)(updateData + MSGHDRLEN);

			char actSeq[21] = {0};
			GetActionRespTableKey(actSeq);
			memcpy(msgUp, actSeq, 20);						//Reserve
			msgUp += 20;

			sprintf(msgUp, "%04d", 2001);						//Deal
			msgUp += 4;

			sprintf(msgUp, "%04d", 0);							//Status
			msgUp += 4;

			memcpy(msgUp, upperNode.Id, 10);					//Pid, 网络终端编号
			msgUp += 10;

			unsigned char md5_input[80]={0};
			unsigned char md5_output[16]={0};
			int len=0, i=0;	

			/* 生成md5验证码 */
			len = 20;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//Reserve 
			i += 20;

			len = 4;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);				//Deal 
			i += 4;

			len = 4;
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//D_Status 
			i += 4;

			len = 10;		// 不包括结束符'\0'
			memcpy(md5_input+i, (char*)(updateData + MSGHDRLEN + i), len);			//szApmId
			i += 10;

			len	= 6;		// 不包括结束符'\0'
			memcpy(md5_input+i, upperNode.szPwd, len);							//szPwd
			i += 6;

			MD5(md5_output, md5_input, i);
			char md5_hex[33] = {0};
			char* ptrMd5Hex = md5_hex;
			for(int i=0; i<16; i++)
			{
				sprintf(ptrMd5Hex, "%02X", md5_output[i]);
				ptrMd5Hex += 2;
			}
			memcpy(msgUp, md5_hex, 32);
			msgUp += 32;

			memcpy(msgUp, detailNode.Id, 10);					//Tid, 传感终端编号
			msgUp += 10;

			memcpy(msgUp, device_node->Id + DEVICE_ID_LEN, 8);//ActionId, 动作编号
			msgUp += 8;

			*(int*)msgUp = nLen;
			msgUp += 4;

			if(nLen > 0)
			{
				memcpy(msgUp, reqBuf, nLen);
				msgUp += nLen;
			}

			if(!g_NetContainer.ActionRespTableInsert(actionMsg, actSeq))
			{
				return SYS_STATUS_FAILED;
			}


			if (!SendMsg_Ex(g_ulHandleNetTmnSvrSend, MUCB_IF_NET_SVR_SEND_TMN, (char*)pMsgHdr, pMsgHdr->unMsgLen))
			{
				DBG(("doSVR_Submit_Func SendMsg_Ex Failed; handle[%d]\n", g_ulHandleNetTmnSvrSend));
			}
			else
			{
				ret = SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP;
			}
		}
		else
		{
			DBG(("CNetDataContainer::ActionProc GetSocketId Failed %s  %d\n", device_node->Upper_Channel, pMsgHdr->unRspTnlId));
		}
	} while (false);
	return ret;
}


//接收数据处理
bool CNetDataContainer::actionRespProc(char *inMsg, int msgLen)
{
	if(!m_IsInitialized)
		return false;
	bool ret = false;
	
	PMsgNetDataActionResp ptrPkg = (PMsgNetDataActionResp)inMsg;
	HashedDeviceInfoACTTERMINALRESP::iterator it;
	ACTION_MSG_WAIT_TERMINAL_RESP_HASH hash_node_resp;
	memset( (unsigned char*)&hash_node_resp, 0, sizeof(hash_node_resp));
	strncpy(hash_node_resp.Id, ptrPkg->szReserve, 20);

	//验证数据合法性，查找设备编号
	if (ActionRespTableGetAndDelete(hash_node_resp))
	{
		char szStatus[5] = {0};
		memcpy(szStatus, ptrPkg->szStatus, 4);

		CDevInfoContainer::SendToActionSource(hash_node_resp.stActionMsg, atoi(szStatus));
		ret = true;
	}
	return ret;
}
//发送数据处理
bool CNetDataContainer::ActionRespTableInsert(ACTION_MSG& stMsgAction, char* actSeq)
{
	if(!m_IsInitialized)
		return false;
	bool ret = false;
	HashedDeviceInfoACTTERMINALRESP::iterator it;
	ACTION_MSG_WAIT_TERMINAL_RESP_HASH hash_node;
	ACTION_MSG_WAIT_TERMINAL_RESP_HASH hash_nodeTemp;
	bool tempExist = false;

	memset((unsigned char*)&hash_node, 0, sizeof(ACTION_MSG_WAIT_RESP_HASH));
	strncpy(hash_node.Id, actSeq, 20);
	time(&hash_node.act_time);
	memcpy(&hash_node.stActionMsg, &stMsgAction, sizeof(ACTION_MSG));

	memset((unsigned char*)&hash_nodeTemp, 0, sizeof(ACTION_MSG_WAIT_RESP_HASH));
	strncpy(hash_nodeTemp.Id, hash_node.Id, 20);

	CLockActionResp.lock();
	it = m_HashTableActResp->find(hash_nodeTemp.Id);
	
	if(it != m_HashTableActResp->end())
	{
		PACTION_MSG_WAIT_RESP_HASH curr_node = (PACTION_MSG_WAIT_RESP_HASH)(&it->second.Id);
		memcpy((unsigned char*)&hash_nodeTemp, (unsigned char*)curr_node, sizeof(ACTION_MSG_WAIT_RESP_HASH));
		m_HashTableActResp->erase(it);
		tempExist = true;
	}
	m_HashTableActResp->insert(HashedDeviceInfoACTTERMINALRESP::value_type(hash_node.Id,hash_node));
	DBG(("m_HashTableActResp->insert[%s]\n", hash_node.Id));
	CLockActionResp.unlock();
	ret = true;
	if(tempExist)//发送结果到发起源
	{
		//g_CenterCtl.ActionRespTableUpdate(hash_nodeTemp.iSourceSeq, 3000); lhy返回动作结果值
	}
	return ret;
}
bool CNetDataContainer::ActionRespTableGetAndDelete(ACTION_MSG_WAIT_TERMINAL_RESP_HASH& hash_node)
{
	if(!m_IsInitialized)
		return false;
	bool ret = false;
	HashedDeviceInfoACTTERMINALRESP::iterator it;

	CLockActionResp.lock();
	it = m_HashTableActResp->find(hash_node.Id);
	if(it != m_HashTableActResp->end())
	{
		printf("CNetDataContainer::ActionRespTableGetAndDelete[%s]\n",hash_node.Id);
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Id, sizeof(ACTION_MSG_WAIT_TERMINAL_RESP_HASH));
		m_HashTableActResp->erase(it);
		ret = true;
	}
	CLockActionResp.unlock();
	return ret;
}
bool CNetDataContainer::SendRespMsg(char* dstId, int seq, int status, unsigned char* sendBuf, int sendlen, TKU32 soketId)//lhy
{
	return true;
}


bool CNetDataContainer::addNode(char* deviceId)
{
	if(!m_IsInitialized)
		return false;
	bool ret = false;
	HashedNetDataInfo::iterator it;

	MSG_NETDATA_HASH hashNode;
	memset(&hashNode, 0, sizeof(hashNode));
	sprintf(hashNode.DeviceId, "%s", deviceId);
	
	DBG(("添加设备DeviceId[%s]\n", hashNode.DeviceId));
	CTblLock.lock();
	it = m_pNetDataHashTable->find(hashNode.DeviceId);	
	
	if(m_pNetDataHashTable->end() == it)
	{
		strcpy(hashNode.DeviceId, deviceId);		
		m_pNetDataHashTable->insert(HashedNetDataInfo::value_type(hashNode.DeviceId,hashNode));
		ret = true;
	}
	CTblLock.unlock();

	return ret;
}



bool CNetDataContainer::deleteNode(char* deviceId)
{
	if(!m_IsInitialized)
		return false;
	bool ret = false;
	HashedNetDataInfo::iterator it;
	MSG_NETDATA_HASH hashNode;
	memset(&hashNode, 0, sizeof(hashNode));

	sprintf(hashNode.DeviceId, "%s", deviceId);
	CTblLock.lock();
	it = m_pNetDataHashTable->find(hashNode.DeviceId);	
	if(m_pNetDataHashTable->end() != it)
	{
		m_pNetDataHashTable->erase(it);
		ret = true;
	}
	CTblLock.unlock();
	return ret;
}
int CNetDataContainer::deleteNodeByDevice(char* deviceId)
{
	if(!m_IsInitialized)
		return false;
	int ret = SYS_STATUS_FAILED;

	int cnt = 0;
	char seqBuf[256][26] = {{0}};
	HashedNetDataInfo::iterator it;
	CTblLock.lock();
	for (it = m_pNetDataHashTable->begin(); it != m_pNetDataHashTable->end(); it++)
	{
		MSG_NETDATA_HASH* ptfNode = (MSG_NETDATA_HASH*)(&it->second.DeviceId);
		if (0 == strcmp(ptfNode->DeviceId, deviceId))
		{
			strcpy(seqBuf[cnt++], ptfNode->DeviceId);
		}
	}

	MSG_NETDATA_HASH hash_node;		
	for( int i=0; i < cnt; i++ )
	{
		memset((unsigned char*)&hash_node, 0, sizeof(hash_node));
		strcpy(hash_node.DeviceId, seqBuf[i]);
		it = m_pNetDataHashTable->find(hash_node.DeviceId);		
		if(m_pNetDataHashTable->end() != it)
		{
			m_pNetDataHashTable->erase(it);
		}
	}
	ret = SYS_STATUS_SUCCESS;
	CTblLock.unlock();

	return ret;
}

//-------------------------------end of file------------------------------------------
