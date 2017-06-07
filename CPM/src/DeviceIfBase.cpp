/******************************************************************************/
/*    File    : switch_config.c                                               */
/*    Author  :                                                           */
/*    Creat   :                                                       */
/*    Function:                                                         */
/******************************************************************************/

/******************************************************************************/
/*                       头文件                                               */
/******************************************************************************/
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include "Shared.h"
#include "Init.h"
#include "SqlCtl.h"
#include "LabelIF.h"
#include "SOIF.h"
#include "DeviceIfBase.h"



#ifdef  DEBUG
#define DEBUG_IF
#endif

#ifdef DEBUG_IF
#define DBG_IF(a)		printf a;
#else
#define DBG_IF(a)	
#endif

/******************************************************************************/
/*                       局部宏定义                                           */
/******************************************************************************/

/******************************************************************************/
/*                       局部函数                                             */
/******************************************************************************/

CDeviceIfBase::CDeviceIfBase()
{
	memset(m_szCurDeviceAttrId, 0, sizeof(m_szCurDeviceAttrId));
}

CDeviceIfBase::~CDeviceIfBase(void)
{		
	Terminate();
}

bool CDeviceIfBase::Terminate()
{
	TRACE(CDeviceIfBase::Terminate());

	if(0xFFFF != m_MainThrdHandle)
	{
		ThreadPool::Instance()->SetThreadMode(m_MainThrdHandle, false);
		if (pthread_cancel(m_MainThrdHandle)!=0)
			;//DBGLINE;
	}
	return true;
}

/******************************************************************************/
/* Function Name  : Initialize                                            */
/* Description    : 接口初始化                                              */
/* Input          : int iChannelId         通道号                               */
/*                  TKU32 unHandleCtrl     动作队列内存句柄                             */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
int CDeviceIfBase::Initialize(int iChannelId, TKU32 unHandleCtrl)
{
	//通道号
	m_iChannelId = iChannelId;

	//动作队列内存句柄
	m_unHandleCtrl	= unHandleCtrl;
	
	//设备轮询线程
	char buf[256] = {0};	
	sprintf(buf, "%s %d %s", "CDeviceIfBase", iChannelId,  "CDeviceIfBaseThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CDeviceIfBase::MainThrd, true,this))
	{
		DBG(("pCommThrdCtrl Create failed!\n"));
		return SYS_STATUS_CREATE_THREAD_FAILED;
	}
	return SYS_STATUS_SUCCESS; 
}

//串口设备轮询线程
void * CDeviceIfBase::MainThrd(void *arg)
{
	CDeviceIfBase *_this = (CDeviceIfBase *)arg;
	_this->MainThrdProc();
	return NULL;
}

/******************************************************************************/
/* Function Name  : CommMsgCtrlFunc                                           */
/* Description    : 串口设备轮询处理                                          */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
void CDeviceIfBase::MainThrdProc(void)
{
	time_t tm_NowTime;
    struct tm *st_tmNowTime;

	struct timeval tmval_NowTime;
    QUEUEELEMENT pMsg = NULL;

	usleep(500*1000);
	m_MainThrdHandle = pthread_self();


	char buf[256] = {0};	
	sprintf(buf, "%s %d start....Pid[%d]\n", "CDeviceIfBase", m_iChannelId, getpid());
	DBG((buf));

	while(1)
	{
		pthread_testcancel();
		CLockQuery.lock();
		do 
		{
			//查看是否有动作任务需要执行
			if( MMR_OK == g_MsgMgr.GetFirstMsg(m_unHandleCtrl, pMsg, 100))
			{
				ACTION_MSG actionMsg;
				PACTION_MSG pstActMsg = (PACTION_MSG)pMsg;
				memset(&actionMsg, 0, sizeof(ACTION_MSG));
				memcpy(&actionMsg,  pstActMsg, sizeof(ACTION_MSG));
				MM_FREE(pMsg);
				pMsg = NULL;
				DoAction(actionMsg);
				usleep( 10 * 1000 );
				break;			
			}		
			//获取当前时间
			time(&tm_NowTime);    		
			gettimeofday(&tmval_NowTime, NULL);
			st_tmNowTime = localtime(&tm_NowTime);

			DEVICE_INFO_POLL deviceNode;
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetNextPollAttrNode(m_iChannelId, m_szCurDeviceAttrId, deviceNode))//获取轮巡结点
			{
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

			unsigned char respBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
			int respLen = 0;

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
				CheckOnLineStatus(pPollNode, bDeviceStatus, stQueryResult);

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
			/*********************************************************************************************/
			//是实体属性
			int iPollRet = 0;
			if (1 == pPollNode->isSOIF)
			{
				iPollRet = SOIF_PollProc(this->m_iChannelId, pPollNode, respBuf, respLen);
			}
			else
			{
				iPollRet = this->PollProc(pPollNode, respBuf, respLen);				
			}
			if(SYS_STATUS_DEVICE_COMMUNICATION_ERROR == iPollRet && 3 > pPollNode->retryCount)
			{
				pPollNode->retryCount++;
				g_DevInfoContainer.UpdateDeviceRollRetryCount(deviceNode);
				break;
			}
			/*********************************************************************************************/
			//记录当前编号
			char curAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
			strcpy(curAttrId, pPollNode->Id);
			while(true) 
			{
				bool bSendMsg = false;
				QUERY_INFO_CPM_EX stQueryResultEx;
				QUERY_INFO_CPM stQueryResult;

				do 
				{//解析轮巡回应数据
					bool bDeviceStatus = false;

					memset((unsigned char*)&stQueryResult, 0, sizeof(QUERY_INFO_CPM));
					Time2Chars(&tm_NowTime, stQueryResult.Time);
					stQueryResult.Cmd = 0x00;//轮巡到的数据
					memcpy(stQueryResult.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

					memset((unsigned char*)&stQueryResultEx, 0, sizeof(QUERY_INFO_CPM_EX));
					Time2Chars(&tm_NowTime, stQueryResultEx.Time);
					stQueryResultEx.Cmd = 0x00;//轮巡到的数据
					memcpy(stQueryResultEx.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

					//数据格式校验
					if( SYS_STATUS_SUCCESS == iPollRet && LabelIF_CheckRecvFormat(pPollNode->Rck_Format, respBuf, respLen))
					{
						int iDecRslt = LabelIF_DecodeByFormat(pPollNode, respBuf, respLen, &stQueryResultEx);
						if (SYS_STATUS_DATA_DECODE_NORMAL == iDecRslt)
						{	
							
							bSendMsg = true;
							bDeviceStatus = true;//设备正常
							//校验并解析成功,置最新数据
							memcpy((unsigned char*)&stQueryResult, (unsigned char*)&stQueryResultEx, sizeof(QUERY_INFO_CPM));
							memcpy(stQueryResult.Value, stQueryResultEx.Value, sizeof(stQueryResult.Value));
							stQueryResult.DataStatus = stQueryResultEx.DataStatus;

							memcpy(pPollNode->stLastValueInfo.Type, stQueryResult.Type, 4);
							pPollNode->stLastValueInfo.VType = stQueryResult.VType;
							if('1' == pPollNode->cIsNeedIdChange)//需要将值转换为用户姓名
							{
								char szUserId[21] = {0};
								char szUserName[21] = {0};
								if(SYS_STATUS_SUCCESS != g_SqlCtl.GetUserInfoByCardId(stQueryResult.Value, szUserId, szUserName))
									break;
								memset(pPollNode->stLastValueInfo.Value, 0, sizeof(pPollNode->stLastValueInfo.Value));
								strcpy(pPollNode->stLastValueInfo.Value, szUserName);
							}
							else
							{
								memcpy(pPollNode->stLastValueInfo.Value, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));
								memcpy(pPollNode->stLastValueInfo.szValueUseable, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));
							}

							stQueryResult.DataStatus = LabelIF_StandarCheck(stQueryResult.VType, stQueryResult.Value, pPollNode->szStandard)?0:1;
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
					else
					{
						pPollNode->stLastValueInfo.DataStatus = 1;

						pPollNode->stLastValueInfo.VType = CHARS;
						memset(pPollNode->stLastValueInfo.Value, 0, sizeof(pPollNode->stLastValueInfo.Value));
						memcpy(pPollNode->stLastValueInfo.Value, "NULL", 4);
					}

					//设备状态更新检测
					if (INTERFACE_DIN_A0 > atoi(pPollNode->Upper_Channel) || INTERFACE_DIN_B3 < atoi(pPollNode->Upper_Channel))//开关量特殊处理
					{
						CheckOnLineStatus(pPollNode, bDeviceStatus, stQueryResult);
					}
					if (bSendMsg)
					{
						//透明传输(加上中海油设备编号判断)
						if((1 == pPollNode->isTransparent)
							|| (0 == strncmp(stQueryResult.DevId, DEV_TYPE_CNOOC_DEAL, 3)
							&& 0 == strncmp(stQueryResult.DevId + DEVICE_ID_LEN, DEV_TYPE_CNOOC_DEAL_ATTID, DEVICE_SN_LEN)))
						{
							SendMsg_Ex(g_ulHandleTransparent, MUCB_DEV_TRANSPARENT, (char*)&stQueryResultEx, sizeof(QUERY_INFO_CPM_EX));
						}
						else if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryResult, sizeof(QUERY_INFO_CPM)))
						{
							DBG(("DeviceIfBase DecodeAndSendToAnalyser 2 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
						}
						else
						{
							DBG(("E----->A   devid[%s] time[%s] value[%s] vtype[%d]\n", stQueryResult.DevId, stQueryResult.Time, stQueryResult.Value, stQueryResult.VType));
						}
					}
				} while (false);

				//更新结点信息,包括属性离线次数,轮巡时间
				memcpy((unsigned char*)(&(deviceNode.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
				g_DevInfoContainer.UpdateDeviceRoll(deviceNode);

				if(0 == strlen(pPollNode->NextId) || 0 == strncmp(curAttrId, pPollNode->NextId, DEVICE_ID_LEN + DEVICE_SN_LEN))//获取相同rd配置的属性
					break;

				DBG(("pPollNode->NextId[%s] deviceNode.id[%s]\n", pPollNode->NextId, deviceNode.Id));
				if (SYS_STATUS_SUCCESS != g_DevInfoContainer.GetNextAttrNode(curAttrId, pPollNode->NextId, deviceNode))
				{
					DBG(("GetNextAttrNode[%s][%s]failed\n", pPollNode->Id, pPollNode->NextId))
						break;
				}
				pPollNode = &deviceNode;
			}
		} while (false);
		CLockQuery.unlock();
		usleep( 10 * 1000 );
	}
}

/******************************************************************************/
/* Function Name  : QueryAttrProc                                           */
/* Description    : 设备实时查询                                          */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
void CDeviceIfBase::QueryAttrProc(DEVICE_INFO_POLL& deviceNode)
{
	QUERY_INFO_CPM stQueryResult;
	
	QUERY_INFO_CPM_EX stQueryResultEx;
	PDEVICE_INFO_POLL pPollNode = &deviceNode;
	bool bDeviceStatus = false;
	bool bSendMsg = false;

	memset((unsigned char*)&stQueryResult, 0, sizeof(QUERY_INFO_CPM));
	stQueryResult.Cmd = 0x00;
	memcpy(stQueryResult.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

	memset((unsigned char*)&stQueryResultEx, 0, sizeof(QUERY_INFO_CPM_EX));
	stQueryResultEx.Cmd = 0x00;
	memcpy(stQueryResultEx.DevId, pPollNode->Id,  DEVICE_ATTR_ID_LEN);

	//获取当前时间  	
	time_t tm_NowTime;
	time(&tm_NowTime);  
	Time2Chars(&tm_NowTime, stQueryResult.Time);
	Time2Chars(&tm_NowTime, stQueryResultEx.Time);

	unsigned char respBuf[MAX_PACKAGE_LEN_TERMINAL] = {0};
	int respLen = 0;

	struct timeval tmval_NowTime;	
	gettimeofday(&tmval_NowTime, NULL);

	CLockQuery.lock();

	DBG(("QueryAttrProc ------------Start--------------- atrId[%s]\n", deviceNode.Id));
	//是虚拟属性
	if (DEVICE_ATTR_ID_LEN == strlen(pPollNode->V_Id))
	{
		LAST_VALUE_INFO stLastValue;
		memset((unsigned char*)&stLastValue, 0, sizeof(LAST_VALUE_INFO));

		int iRet = g_DevInfoContainer.GetAttrValue(pPollNode->V_Id, stLastValue);

		if (SYS_STATUS_SUCCESS == iRet &&  0 != strcmp(stLastValue.Value, "NULL"))
		{
			strncpy(stQueryResult.Type, stLastValue.Type, 4);
			stQueryResult.VType = stLastValue.VType;
			memcpy(stQueryResult.Value, stLastValue.Value, sizeof(stQueryResult.Value));
			bDeviceStatus = true;//设备正常

			stQueryResult.DataStatus = LabelIF_StandarCheck(stQueryResult.VType, stQueryResult.Value, pPollNode->szStandard)?0:1;

			//校验并解析成功,置最新数据
			memcpy(pPollNode->stLastValueInfo.Type, stQueryResult.Type, 4);
			pPollNode->stLastValueInfo.VType = stQueryResult.VType;
			memcpy(pPollNode->stLastValueInfo.Value, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));
			pPollNode->stLastValueInfo.DataStatus = stQueryResult.DataStatus;		
		}
		else
		{
			pPollNode->stLastValueInfo.DataStatus = 1;
			
			pPollNode->stLastValueInfo.VType = CHARS;
			memset(pPollNode->stLastValueInfo.Value, 0, sizeof(pPollNode->stLastValueInfo.Value));
			memcpy(pPollNode->stLastValueInfo.Value, "NULL", 4);
		}

		//设备状态更新检测
		CheckOnLineStatus(pPollNode, bDeviceStatus, stQueryResult);
	}
	else//是实体属性
	{		
		respLen = sizeof(respBuf);
		this->PollProc(pPollNode, respBuf, respLen);

		//数据格式校验
		if( LabelIF_CheckRecvFormat(pPollNode->Rck_Format, respBuf, respLen))
		{
			int iDecRslt = LabelIF_DecodeByFormat(pPollNode, respBuf, respLen, &stQueryResultEx);
			if (SYS_STATUS_DATA_DECODE_NORMAL == iDecRslt)
			{	
				bSendMsg = true;
				bDeviceStatus = true;//设备正常
				//校验并解析成功,置最新数据
				memcpy((unsigned char*)&stQueryResult, (unsigned char*)&stQueryResultEx, sizeof(QUERY_INFO_CPM));
				memcpy(stQueryResult.Value, stQueryResultEx.Value, sizeof(stQueryResult.Value));
				stQueryResult.DataStatus = stQueryResultEx.DataStatus;

				//校验并解析成功,置最新数据
				memcpy(pPollNode->stLastValueInfo.Type, stQueryResult.Type, 4);
				pPollNode->stLastValueInfo.VType = stQueryResult.VType;
				memcpy(pPollNode->stLastValueInfo.Value, stQueryResult.Value, sizeof(pPollNode->stLastValueInfo.Value));

				stQueryResult.DataStatus = LabelIF_StandarCheck(stQueryResult.VType, stQueryResult.Value, pPollNode->szStandard)?0:1;
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
		else
		{
			pPollNode->stLastValueInfo.DataStatus = 1;			
			pPollNode->stLastValueInfo.VType = CHARS;
			memset(pPollNode->stLastValueInfo.Value, 0, sizeof(pPollNode->stLastValueInfo.Value));
			memcpy(pPollNode->stLastValueInfo.Value, "NULL", 4);
		}

		//设备状态更新检测
		if (INTERFACE_AD_A0 >= atoi(pPollNode->Upper_Channel) || INTERFACE_DIN_B3 <= atoi(pPollNode->Upper_Channel))//开关量特殊处理
		{
			CheckOnLineStatus(pPollNode, bDeviceStatus, stQueryResult);
		}
	}

	if (bSendMsg)
	{
		if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stQueryResult, sizeof(QUERY_INFO_CPM)))
		{
			DBG(("DeviceIfBase DecodeAndSendToAnalyser 3 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
		}
		else
		{
			DBG(("E----->A   devid[%s] time[%s] value[%s] vtype[%d]\n", stQueryResult.DevId, stQueryResult.Time, stQueryResult.Value, stQueryResult.VType));
		}
	}
	memcpy((unsigned char*)(&(deviceNode.stLastValueInfo.LastTime)), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
	g_DevInfoContainer.UpdateDeviceRoll(deviceNode);
	
	DBG(("QueryAttrProc ------------End--------------- atrId[%s]\n", deviceNode.Id));
	CLockQuery.unlock();
}

bool CDeviceIfBase::CheckTime(struct tm *st_tmNowTime, DEVICE_INFO_POLL device_node)
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


void CDeviceIfBase::DoAction(ACTION_MSG actionMsg)
{
	//查找对应动作设备
	char szActionId[DEVICE_ACT_ID_LEN_TEMP] = {0};
	DEVICE_INFO_ACT devNode;
	memset((unsigned char*)&devNode, 0, sizeof(DEVICE_INFO_ACT));
	memcpy(szActionId, actionMsg.DstId, DEVICE_ID_LEN);
	
	memcpy(szActionId + DEVICE_ID_LEN, actionMsg.ActionSn, DEVICE_ACTION_SN_LEN);
	int actionRet = SYS_STATUS_FAILED;

	DBG((" CDeviceIfBase DoAction .DeviceId[%s].\n", szActionId));
	do
	{
		if (0 == strcmp(ACTION_SET_DEFENCE, actionMsg.ActionSn) || 0 == strcmp(ACTION_REMOVE_DEFENCE, actionMsg.ActionSn))//布防、撤防
		{
			int status = 0;
			if(0 == strcmp(ACTION_SET_DEFENCE, actionMsg.ActionSn))
			{
				status = SYS_ACTION_SET_DEFENCE;
			}
			else
			{
				status = SYS_ACTION_REMOVE_DEFENCE;
			}
			if(g_DevInfoContainer.HashDeviceDefenceSet(actionMsg.DstId, status))
				actionRet = SYS_STATUS_SUCCESS;
		}
		else
		{
			DEVICE_DETAIL_INFO stDeviceDetail;
			memset((unsigned char*)&stDeviceDetail, 0, sizeof(DEVICE_DETAIL_INFO));
			memcpy(stDeviceDetail.Id, actionMsg.DstId, DEVICE_ID_LEN);
			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceDetailNode(stDeviceDetail))//获取设备
			{
				actionRet = SYS_STATUS_DEVICE_NOT_EXIST;
				break;
			}
			if(DEVICE_NOT_USE == stDeviceDetail.cStatus)//设备已撤防
			{
				actionRet = SYS_STATUS_DEVICE_REMOVE_DEFENCE;
				break;
			}

			if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetActionNode(szActionId, devNode))//获取设备动作
			{
				DBG(("获取设备动作属性失败[%s]\n", szActionId));
				actionRet = SYS_STATUS_DEVICE_ACTION_NOT_EXIST;
				break;
			}
			if ('0' == devNode.cBeOn)//动作未启用
			{
				DBG(("该设备动作未启用[%s]\n", szActionId));
				actionRet = SYS_STATUS_DEVICE_ACTION_NOT_USE;
				break;
			}
			int actionCount = 3;
			actionRet = SYS_STATUS_DEVICE_COMMUNICATION_ERROR;
			while(SYS_STATUS_DEVICE_COMMUNICATION_ERROR == actionRet && actionCount-- > 0)
				actionRet = ActionProc(&devNode, actionMsg);
		}
	}while(false);

	CDevInfoContainer::SendToActionSource(actionMsg, actionRet);
}


/******************************************************************************/
/* Function Name  : CheckOnLineStatus                                            */
/* Description    : 在线离线判断		                                      */
/* Input          : DEVICE_INFO_POLL *p_node	        轮巡结点      */
/*		          : int nLen							数据长度      */
/* Output         : none                   */
/* Return         : int ret          0:无变化，1:属性重新能采集， 2:属性采集不成功                        */
/******************************************************************************/
int CDeviceIfBase::CheckOnLineStatus(DEVICE_INFO_POLL *p_node, bool bStatus, QUERY_INFO_CPM stQueryResult)
{
	int ret = SYS_STATUS_SUCCESS;
	QUERY_INFO_CPM stOnlinNotice;

	memcpy((unsigned char*)&stOnlinNotice, (unsigned char*)&stQueryResult, sizeof(QUERY_INFO_CPM));

	bool isAlreadyOffLine = false;
    if( bStatus)        //接收到了数据
    {
        if(0 == p_node->offLineCnt)        //
        {
           ret = SYS_STATUS_DEVICE_ATTR_ONLINE;
		}
		p_node->offLineCnt = p_node->offLineTotle;
    }
    else
    {
		if (0 == p_node->offLineCnt)//设备属性已离线
		{
			isAlreadyOffLine = true;
			ret = SYS_STATUS_DEVICE_ATTR_OFFLINE;
			stOnlinNotice.Cmd = 0;
			memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
			strcpy(stOnlinNotice.Value, "NULL");
			stOnlinNotice.VType = CHARS;
			stOnlinNotice.DataStatus = 1;
			DBG(("[%s] 属性离线1\n", stOnlinNotice.DevId));
			if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
			{
				DBG(("DeviceIfBase DecodeAndSendToAnalyser 4 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
			}
		}
        else if(0 < p_node->offLineCnt && 0 == --p_node->offLineCnt)    //设备属性离线
        {
            ret = SYS_STATUS_DEVICE_ATTR_OFFLINE;
			stOnlinNotice.Cmd = 0;
			memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
			strcpy(stOnlinNotice.Value, "NULL");
			stOnlinNotice.VType = CHARS;
			stOnlinNotice.DataStatus = 1;
			DBG(("[%s] 属性离线2\n", stOnlinNotice.DevId));
			if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
			{
				DBG(("DeviceIfBase DecodeAndSendToAnalyser 5 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
			}
        }
		DBG(("[%s] 属性无数据 p_node->offLineCnt[%d]\n", stOnlinNotice.DevId, p_node->offLineCnt));
    }

	bool bSendMsg = false;
	switch(ret)
	{
		case SYS_STATUS_DEVICE_ATTR_OFFLINE:
			{
				//判断是否设备离线lhy
				if (!g_DevInfoContainer.IsDeviceOnline(p_node->Id))
				{
					sprintf(stOnlinNotice.Value, "%d", DEVICE_OFFLINE);

					if (isAlreadyOffLine)
						stOnlinNotice.Cmd = 3;
					else
						stOnlinNotice.Cmd = 1;
					stOnlinNotice.DevId[DEVICE_ID_LEN] = '\0';
					strncpy(stOnlinNotice.DevId + DEVICE_ID_LEN, "0000", DEVICE_SN_LEN);
					memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
					sprintf(stOnlinNotice.Value, "%d", DEVICE_OFFLINE);
					bSendMsg = true;
				}
			}
			break;
		case SYS_STATUS_DEVICE_ATTR_ONLINE:
			//判断是否重置设备在线lhy
			{
				if (!g_DevInfoContainer.IsDeviceOnline(p_node->Id))
				{
					stOnlinNotice.Cmd = 1;
					stOnlinNotice.DevId[DEVICE_ID_LEN] = '\0';
					
					memset(stOnlinNotice.Value, 0, sizeof(stOnlinNotice.Value));
					sprintf(stOnlinNotice.Value, "%d", DEVICE_ONLINE);
					bSendMsg = true;
				}
			}
			break;
	}

	if(bSendMsg)
	{
		if(!SendMsg_Ex(g_ulHandleAnalyser, MUCB_DEV_ANALYSE, (char*)&stOnlinNotice, sizeof(QUERY_INFO_CPM)))
		{
			DBG(("DeviceIfBase DecodeAndSendToAnalyser 6 SendMsg_Ex failed  handle[%d]\n", g_ulHandleAnalyser));
		}
		else
		{
			DBG(("E----->A Cmd[%d]  devid[%s] time[%s] value[%s] vtype[%d]\n", stOnlinNotice.Cmd, stOnlinNotice.DevId, stOnlinNotice.Time, stOnlinNotice.Value, stOnlinNotice.VType));
		}
	}
	return ret;
}

/*************************************  END OF FILE *****************************************/
