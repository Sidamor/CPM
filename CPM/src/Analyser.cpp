#include <string>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include "Shared.h"
#include "Init.h"
#include "Define.h"
#include "SqlCtl.h"
#include "Analyser.h"
#include "HardIF.h"

#ifdef  DEBUG
#define DEBUG_ANA
#endif
#ifdef DEBUG_ANA
#define DBG_ANA(a)		printf a;
#else
#define DBG_ANA(a)	
#endif


CAnalyser::CAnalyser()
{
}

CAnalyser::~CAnalyser()
{
}

bool CAnalyser::Initialize(TKU32 unHandleAnalyser)
{
    m_unHandleAnalyser = unHandleAnalyser;

    //-------------------------申请哈希表---------------------------
    m_pActHashTable = new HashedInfo();
    if (!m_pActHashTable)
	{
		return false;
	}
	
    m_pAgentHashTable = new HashedAgentInfo();
	if (!m_pAgentHashTable)
	{
		return false;
	}	

    if (!InitAnalyseHash())           //动作哈希表初始化
	{
		return false;
	}

    //数据处理线程
    char buf[256] = {0};
	sprintf(buf, "%s %s", "CAnalyser", "AnalyserThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CAnalyser::pAnalyserThrd, true,this))
	{
		DBG(("pAnalyserThrd Create failed!\n"));
		return false;
	}

	DBG_ANA(("CAnalyser::Initialize success[%d]!\n ", unHandleAnalyser));
	return true; 
}

/******************************************************************************/
/* Function Name  : InitAnalyseHash                                           */
/* Description    : 哈希表数据初始化                                          */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CAnalyser::InitAnalyseHash()
{
	DBG(("Now try to init actHash--------------->\n"));

	const char* format = "select device_roll.id,"
					" device_roll.sn,"
					" device_detail.status,"
					" device_roll.standard,"
					" device_attr.s_define "
						" from device_roll, "
						" device_detail,"
						" device_attr"
						" where device_roll.id = device_detail.id "
						" and device_roll.sn = device_attr.sn"
						" and substr(device_detail.id,1,%d) = device_attr.id "
						" group by device_roll.id, device_roll.sn;";
	char sqlData[1024] = {0};
	sprintf(sqlData, format, DEVICE_TYPE_LEN);
	if (!InitActHashData(sqlData))
	{
		DBG(("InitActHashData InitActHashData Failed ....\n"));
		return false;
	}
	//lhy 初始化联动内容
	format = "select SEQ,"
					" ID,"
					" SN,"
					" Condition,"
					" Valibtime,"
					" Interval,"
					" Times,"
					" D_ID,"
					" D_CMD,"
					" OBJECT,"
					" CTYPE"
					" from device_agent;";
	if (!InitAgentInfo(format))
	{
		DBG(("InitAgentInfo InitActHashData Failed ....\n"));
		return false;
	}
	return true;

}

/******************************************************************************/
/* Function Name  : InitActHashInfo                                           */
/* Description    : 哈希表数据初始化                                          */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
bool CAnalyser::InitActHashData(char* sqlData)
{
	if( 0 != g_SqlCtl.select_from_tableEx(sqlData, InitActHashCallback, (void *)this))
		return false;
	return true;
}

int CAnalyser::InitActHashCallback(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CAnalyser* _this = (CAnalyser*)data;
	_this->InitActHashCallbackProc(n_column, column_value, column_name);	
	return 0;
}

int CAnalyser::InitActHashCallbackProc(int n_column, char **column_value, char **column_name)
{
	int ret = -1;
	MSG_ANALYSE_HASH hash_node;

	memset(&hash_node, 0, sizeof(MSG_ANALYSE_HASH));

	memcpy(hash_node.key, column_value[0], strlen(column_value[0]));  
	memcpy(hash_node.key+DEVICE_ID_LEN, column_value[1], strlen(column_value[1]));  //读指令sn号

	//	DBG_ANA(("c0[%s] c1[%s] c2[%s] c3[%s] c4[%s] c5[%s] c6[%s] c7[%s]\n", column_value[0], column_value[1], column_value[2], column_value[3], column_value[4], column_value[5], column_value[6], column_value[7]));
	//DBG_ANA(("nodeId:[%s]  ", hash_node.key));
	hash_node.deviceStatus = atoi(column_value[2]);                           //获取设备状态

	memcpy(hash_node.standard, column_value[3], strlen(column_value[3]));     //获取标准值范围

	strcpy(hash_node.statusDef, trim(column_value[4], strlen(column_value[4])));
	//  DBG_ANA(("statusDef:[%s]\n", hash_node.statusDef));
	hash_node.actCnt = 0;
	memset((unsigned char*)&hash_node.agentSeq[0], 0, sizeof(int) * MAX_CNT_ACTFORMAT);
	memset((unsigned char*)(&hash_node.LastTime), 0, sizeof(struct timeval));

	hash_node.dataStatus = 1;
	CTblLock.lock();
 	m_pActHashTable->insert(HashedInfo::value_type(hash_node.key,hash_node));
	CTblLock.unlock();

	// DBG_ANA(("InitActHashCallbackProc end\n"));
	return ret;
}

bool CAnalyser::InitAgentInfo(const char* sqlData)
{
	if( 0 != g_SqlCtl.select_from_tableEx((char*)sqlData, InitAgentInfoCallback, (void *)this))
		return false;
	return true;
}
int CAnalyser::InitAgentInfoCallback(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	CAnalyser* _this = (CAnalyser*)data;
	_this->InitAgentInfoCallbackProc(n_column, column_value, column_name);	
	return 0;
}

int CAnalyser::InitAgentInfoCallbackProc(int n_column, char **column_value, char **column_name)
{
	int ret = -1;
	AgentInfo stAgentInfo;
	
	memset((unsigned char*)&stAgentInfo, 0, sizeof(AgentInfo));
	
	stAgentInfo.iSeq = atoi(column_value[0]);//联动编号
	memcpy(stAgentInfo.szDevId, column_value[1], DEVICE_ID_LEN);//设备编号
	memcpy(stAgentInfo.szAttrId, column_value[2], DEVICE_SN_LEN);//属性编号
	strcpy(stAgentInfo.szCondition, column_value[3]);
	strcpy(stAgentInfo.szValibtime, column_value[4]);
	stAgentInfo.iInterval = atoi(column_value[5]);
	stAgentInfo.iTimes = atoi(column_value[6]);
	strcpy(stAgentInfo.szActDevId, column_value[7]);
	strcpy(stAgentInfo.szActActionId, column_value[8]);
	strcpy(stAgentInfo.szActValue, column_value[9]);
	stAgentInfo.cType = column_value[10][0];

	AnalyseHashAgentAdd(stAgentInfo);
	return ret;
}
void CAnalyser::getAgentMsg(int* pAgentId, AgentInfo stAgent)
{
	*pAgentId = stAgent.iSeq;

	ACT_FORMAT act_format;
	ACT_FORMAT* pact_format = &act_format;
	memset((unsigned char*)pact_format, 0, sizeof(ACT_FORMAT));

	pact_format->iSeq = stAgent.iSeq;
	memcpy(pact_format->szCondition, stAgent.szCondition, sizeof(stAgent.szCondition));  //条件
	memcpy(pact_format->szValibtime, stAgent.szValibtime, sizeof(stAgent.szValibtime));  //有效时间
	pact_format->iInterval = stAgent.iInterval*60;										//联动间隔
	pact_format->iTimes = stAgent.iTimes;												//联动次数
	memcpy(pact_format->szActDevId, stAgent.szActDevId, sizeof(pact_format->szActDevId));  
	memcpy(pact_format->szActActionId, stAgent.szActActionId, sizeof(pact_format->szActActionId));  
	memcpy(pact_format->szActValue, stAgent.szActValue, sizeof(pact_format->szActValue));  
	pact_format->time_lastTime = 0;              //上次联动时间
	pact_format->times_lastTime = 1;             //上次联动次数

	HashedAgentInfo::iterator it;
    it = m_pAgentHashTable->find((int)act_format.iSeq);	
	if (it != m_pAgentHashTable->end())
	{
		m_pAgentHashTable->erase(it);
	}
	m_pAgentHashTable->insert(HashedAgentInfo::value_type((int)act_format.iSeq,act_format));
}

//数据处理线程
void * CAnalyser::pAnalyserThrd(void* arg)
{
    CAnalyser *_this = (CAnalyser *)arg;
    _this->MsgAnalyzer();
    return NULL;
}

/******************************************************************************/
/* Function Name  : MsgAnalyzer                                               */
/* Description    : 轮询回复数据解析处理                                      */
/* Input          : none                                                      */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
void CAnalyser::MsgAnalyzer()
{
    MSG_ANALYSE_HASH hash_node;
    QUEUEELEMENT pMsg = NULL;
    char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};
    char szAttrSn[DEVICE_SN_LEN_TEMP] = {0};

    time_t tm_NowTime = 0;
	time_t tm_UpdateTime = 0;

	struct tm *tmNow = NULL;

	bool bDoAction = false;
	struct timeval tmval_NowTime;

	time(&tm_UpdateTime); 
	tmNow = localtime(&tm_UpdateTime);
	tmNow->tm_hour = 0;
	tmNow->tm_min = 0;
	tmNow->tm_sec = 0;

	tm_UpdateTime = mktime(tmNow);

	printf("CAnalyser::MsgAnalyzer .... Pid[%d]\n", getpid());

	int lightCnt = 1;
    while(true)
    {
        pthread_testcancel();
        time(&tm_NowTime); 
		tmNow = localtime(&tm_NowTime);
		lightCnt--;
		if (0 == lightCnt)
		{
			HardIF_DBGLightON();                //系统灯
		}
		//隔天并且是凌晨0点进行数据库备份
        if( (tm_NowTime - tm_UpdateTime) > ONE_DAY_SECONDS && 0 == tmNow->tm_hour)
        {
			tm_UpdateTime += ONE_DAY_SECONDS;
			//每月发一条消息给控制手机
			if(1 == tmNow->tm_mday)
			{
				g_GSMCtrl.SendProInfoToSmsMonthly();
			}

			g_SqlCtl.UpdateHistoryDB();
		}

		//从待分析队列中取数据
		if(MMR_OK != g_MsgMgr.GetFirstMsg(m_unHandleAnalyser, pMsg, 100))
		{
			pMsg = NULL;
			usleep(10*1000);
			if (0 == lightCnt)
			{
				lightCnt = 10;
				HardIF_DBGLightOFF();                //系统灯
			}
			continue;
		}
		do 
		{			
			QUERY_INFO_CPM* stMsgFromPoll = (QUERY_INFO_CPM*)pMsg;
			
			DBG_ANA(("A-------   devid:[%s]  TYPE:[%s]  TIME:[%s] VTYPE:[%d]  VALUE:[%s] Cmd[%d]\n", stMsgFromPoll->DevId, stMsgFromPoll->Type, stMsgFromPoll->Time, stMsgFromPoll->VType, stMsgFromPoll->Value, stMsgFromPoll->Cmd));
			memset(szDeviceId, 0, sizeof(szDeviceId));
			memcpy(szDeviceId, stMsgFromPoll->DevId, DEVICE_ID_LEN);

			memset(szAttrSn, 0, sizeof(szAttrSn));
			memcpy(szAttrSn, stMsgFromPoll->DevId+DEVICE_ID_LEN, DEVICE_SN_LEN);

			bDoAction = false;

			gettimeofday(&tmval_NowTime, NULL);
			
			//查询hash表，获取数据源配置
			switch(stMsgFromPoll->Cmd)
			{
			case 0://采集到的数据
				{
					memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
					memcpy(hash_node.key, stMsgFromPoll->DevId, DEVICE_ATTR_ID_LEN);
					if( !GetHashData(hash_node))
					{
						DBG_ANA(("Can not find device id[%s]\n", hash_node.key));
						break;
					}

					if(NULL == stMsgFromPoll->Value || 0 == strlen(stMsgFromPoll->Value))
						break;
					char temp[MAX_PACKAGE_LEN_TERMINAL] = {0};
					//printf("A------1.3DataStatus[%d]\n", stMsgFromPoll->DataStatus);
					if (0 == strcmp(stMsgFromPoll->Value, "NULL"))
					{
						strcpy(temp, "NULL");
					}
					else if ( 0 < strlen(hash_node.statusDef))//是状态类数据
					{
						sprintf(temp, "%d", atoi(stMsgFromPoll->Value));
					}
					else
					{
						memcpy(temp, stMsgFromPoll->Value, sizeof(stMsgFromPoll->Value));
					}		
					
					if (0 == stMsgFromPoll->DataStatus)
					{
						//如果当前数据状态为正常，则需要将告警表中的数据改成自动处理
						if(1 == hash_node.dataStatus)
						{
							CSqlCtl::UpdateAlertInfoTable(szDeviceId, szAttrSn,  '2', stMsgFromPoll->Time);
							CSqlCtl::DeleteFromAlertNowTable(szDeviceId, szAttrSn, 0);
							//更新最新数据状态
							hash_node.dataStatus = stMsgFromPoll->DataStatus;
							updateNodePara(hash_node, 4);

							DEVICE_INFO_POLL deviceNode;
							if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(stMsgFromPoll->DevId, deviceNode))
							{
								memset(deviceNode.stLastValueInfo.szLevel, 0, 4);
								memset(deviceNode.stLastValueInfo.szDescribe, 0, sizeof(deviceNode.stLastValueInfo.szDescribe));
								g_DevInfoContainer.UpdateDeviceAlarmValue(deviceNode);
							}
						}					
					}
//debug 3	pass	
					
					//数据发生变化,或者长时间未有数据插入,则将数据插入到数据表
					unsigned long time_elipse = (tmval_NowTime.tv_sec - hash_node.LastTime.tv_sec)*1000 + (tmval_NowTime.tv_usec - hash_node.LastTime.tv_usec)/1000;
					
					if(f_valueChangedCheck(stMsgFromPoll->VType, stMsgFromPoll->Value, hash_node.unionLastValue) //数据发生变化
						|| time_elipse >= 3600000																 //已有1小时未插入数据
						|| stMsgFromPoll->VType == CHARS
						|| stMsgFromPoll->VType == BYTES
						|| stMsgFromPoll->VType == BCD						
						)																 
					{
						//debug 4	pass
						
						if (0 != strcmp(temp, "NULL"))
						{
							DEVICE_INFO_POLL deviceNode;
							if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(stMsgFromPoll->DevId, deviceNode))
							{
								if(1 == g_ModeCfg.DB_Insert)
									InsertDataTable(hash_node, "data", stMsgFromPoll->Time, temp, stMsgFromPoll->DataStatus, true);//插入到数据表
								if('2' == deviceNode.cLedger)//需要考勤统计
								{//将值转换为用户姓名
									char szUserId[21] = {0};
									char szUserName[21] = {0};
									if(SYS_STATUS_SUCCESS != g_SqlCtl.GetUserInfoByCardId(temp, szUserId, szUserName))
										break;
									//插入到ATS表
									InsertATSTable(hash_node, (char*)"ATS", stMsgFromPoll->Time, temp, szUserId);
								}
							}
						}
						//debug 5	
						
						//更新最新值和最新更新时间lsd
						memcpy((unsigned char*)&hash_node.unionLastValue, stMsgFromPoll->Value, sizeof(UnionLastValue));
						memcpy((unsigned char*)(&hash_node.LastTime), (unsigned char*)(&tmval_NowTime), sizeof(struct timeval));
						updateNodePara(hash_node, 1);
						updateNodePara(hash_node, 2);
						//数据上传到平台
						if( g_upFlag == 1 )
						{
							DEVICE_DETAIL_INFO deviceInfo;
							memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
							memcpy(deviceInfo.Id, hash_node.key, DEVICE_ID_LEN);
							char szUnit[11] = {0};
							DEVICE_INFO_POLL stDevicePoll;
							if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo)
								&& SYS_STATUS_SUCCESS == g_DevInfoContainer.GetParaUnit(atoi(stMsgFromPoll->Type), szUnit)
								&& SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(stMsgFromPoll->DevId, stDevicePoll))
							{
								if (1 == stDevicePoll.isUpload)
								{
									char updateData[MAXMSGSIZE] = {0};
									memset(updateData, 0, sizeof(updateData));
									char* ptrBuf = updateData;
									memcpy(ptrBuf, hash_node.key, DEVICE_ID_LEN);//deviceId
									ptrBuf += DEVICE_ID_LEN;

									sprintf(ptrBuf, "%-30s", deviceInfo.szCName);//deviceName
									ptrBuf += 30;

									strncpy(ptrBuf, hash_node.key+DEVICE_ID_LEN, DEVICE_SN_LEN);//attrId
									ptrBuf += DEVICE_SN_LEN;

									sprintf(ptrBuf, "%-20s", stDevicePoll.szAttrName);//deviceName
									ptrBuf += 20;

									sprintf(ptrBuf, "%-20s", stMsgFromPoll->Time);
									ptrBuf += 20;

									sprintf(ptrBuf, "%-128s", stMsgFromPoll->Value);
									ptrBuf += 128;

									sprintf(ptrBuf, "%-10s", szUnit);
									ptrBuf += 10;

									Net_Up_Msg(CMD_ENVIRON_DATA_UPDATE, updateData, ptrBuf - updateData);
								}
							}
						}
					}
					else
					{
						DBG_ANA(("不满足条件\n"));
					}
					//需要执行联动策略
					bDoAction = true;
				}
				break;
			case 1://系统告警或通知
				//DBG_ANA(("在线、离线数据\n"));
				{
					UpdateDeviceStatus(szDeviceId, stMsgFromPoll->Value, stMsgFromPoll->Time);
					g_DevInfoContainer.HashDeviceStatusUpdate(szDeviceId, atoi(stMsgFromPoll->Value));//更新设备表的设备状态

					memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
					memcpy(hash_node.key, stMsgFromPoll->DevId, DEVICE_ATTR_ID_LEN);
					DBG_ANA(("系统通知 id[%s]\n", hash_node.key));
					if( GetHashData(hash_node))
					{
						DoOffLineAct(szDeviceId, szAttrSn, &hash_node.agentSeq[0], hash_node.actCnt, false);
						//updateNodePara(hash_node, 3);
					}
				}
				break;
			case 2://设备撤防布防变更
				DBG_ANA(("布防撤防数据\n"));
				UpdateDeviceStatus(szDeviceId, stMsgFromPoll->Value, stMsgFromPoll->Time);
				break;
			case 3://系统告警重复通知
				{
					memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
					memcpy(hash_node.key, stMsgFromPoll->DevId, DEVICE_ATTR_ID_LEN);
					DBG_ANA(("系统重复通知 id[%s]\n", hash_node.key));
					if( GetHashData(hash_node))
					{
						DoOffLineAct(szDeviceId, szAttrSn, &hash_node.agentSeq[0], hash_node.actCnt, true);
						//updateNodePara(hash_node, 3);
					}
				}
				break;
			case 4://情景模式触发
				{
					memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
					memcpy(hash_node.key, stMsgFromPoll->DevId, DEVICE_ATTR_ID_LEN);
					DBG_ANA(("情景模式通知 id[%s]\n", hash_node.key));
					if( GetHashData(hash_node))
					{
						bDoAction = true;
					}
				}
				break;
			}
			//-------------------数据解析操作-------------------
			if (bDoAction)
			{
				DecodeAct(szDeviceId, szAttrSn, stMsgFromPoll->VType, stMsgFromPoll->Value,  &hash_node.agentSeq[0], hash_node.actCnt, stMsgFromPoll->Time);    //解析返回数据
				
			}
		} while (false);
		if (NULL != pMsg)
		{
			MM_FREE(pMsg);
			pMsg = NULL;
		}
		if (0 == lightCnt)
		{
			lightCnt = 10;
			HardIF_DBGLightOFF();                //系统灯
		}
    }
}
/******************************************************************************/
/* Function Name  : UpdateDeviceStatus                                            */
/* Description    : 设备状态变更                                            */
/* Input          : int   vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* standard       正常值范围                           */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
void CAnalyser::UpdateDeviceStatus(char* deviceId, char* status, char* strTime)
{
	char sqlData[1024] = {0};

	//更新hash表状态
	deviceStatusUpdate( deviceId, atoi(status));

	//更新数据库设备状态
	strcat(sqlData, "status='");
	strcat(sqlData, status);
	strcat(sqlData, "' where id = '");
	strcat(sqlData, deviceId);
	strcat(sqlData, "'");
	g_SqlCtl.update_into_table("device_detail", sqlData, true);

	DBG_ANA(("UpdateDeviceStatus status[%d]\n", atoi(status)));
	//如果是异常状态，则插入到告警表
	
	switch(atoi(status))
	{
	case DEVICE_NOT_USE://撤防
		{
		}
		break;
	case DEVICE_OFFLINE://离线
		{
			CSqlCtl::InsertAlertInfoTable(deviceId, (char*)"", strTime, ALARM_SERIOUS, (char*)"设备离线",'0',(char*)"");
			CSqlCtl::DeleteFromAlertNowTable(deviceId, (char*)"", 1);

			DEVICE_DETAIL_INFO deviceInfo;
			memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
			memcpy(deviceInfo.Id, deviceId, DEVICE_ID_LEN);
			if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo))
			{
				char updateData[MAXMSGSIZE] = {0};
				memset(updateData, 0, sizeof(updateData));
				char* ptrBuf = updateData;
				memcpy(ptrBuf, deviceId, DEVICE_ID_LEN);
				ptrBuf += DEVICE_ID_LEN;

				sprintf(ptrBuf, "%-30s", deviceInfo.szCName);
				ptrBuf += 30;

				sprintf(ptrBuf, "%-4s", "");
				ptrBuf += 4;

				sprintf(ptrBuf, "%-20s", "");
				ptrBuf += 20;

				sprintf(ptrBuf, "%d", ALARM_SERIOUS);
				ptrBuf += 1;

				sprintf(ptrBuf, "%-20s", strTime);
				ptrBuf += 20;

				sprintf(ptrBuf, "%-128s", "设备离线");
				ptrBuf += 128;
				Net_Up_Msg(CMD_ALARM_UPLOAD, updateData, ptrBuf - updateData);
			}
		}
		break;
	case DEVICE_ONLINE://在线
		//CSqlCtl::InsertAlertInfoTable(deviceId, "", strTime, ALARM_ATTENTION, "设备恢复在线", '3', "");
		CSqlCtl::UpdateAlertInfoTable(deviceId, (char*)"", '2', strTime);
		CSqlCtl::DeleteFromAlertNowTable(deviceId, (char*)"", 0);

		DEVICE_DETAIL_INFO deviceInfo;
		memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(deviceInfo.Id, deviceId, DEVICE_ID_LEN);
		if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo))
		{
			char updateData[MAXMSGSIZE] = {0};
			memset(updateData, 0, sizeof(updateData));
			char* ptrBuf = updateData;
			memcpy(ptrBuf, deviceId, DEVICE_ID_LEN);
			ptrBuf += DEVICE_ID_LEN;

			sprintf(ptrBuf, "%-30s", deviceInfo.szCName);
			ptrBuf += 30;

			sprintf(ptrBuf, "%-4s", "");
			ptrBuf += 4;

			sprintf(ptrBuf, "%-20s", "");
			ptrBuf += 20;

			sprintf(ptrBuf, "%d", ALARM_ATTENTION);
			ptrBuf += 1;

			sprintf(ptrBuf, "%-20s", strTime);
			ptrBuf += 20;

			sprintf(ptrBuf, "%-128s", "设备恢复在线");
			ptrBuf += 128;
			Net_Up_Msg(CMD_ALARM_UPLOAD, updateData, ptrBuf - updateData);
		}
		break;
		
	default:
		DBG_ANA(("未知状态 [%d]\n", atoi(status)));
	}
	
	//设备状态变更上传至平台

	{
		char updateData[2048] = {0};
		memset(updateData, 0, sizeof(updateData));

		DEVICE_DETAIL_INFO deviceInfo;
		memset((unsigned char*)&deviceInfo, 0, sizeof(DEVICE_DETAIL_INFO));
		memcpy(deviceInfo.Id, deviceId, DEVICE_ID_LEN);

		if (SYS_STATUS_SUCCESS == g_DevInfoContainer.GetDeviceDetailNode(deviceInfo))
		{
			char* ptrBuf = updateData;
			memcpy(ptrBuf, deviceId, DEVICE_ID_LEN);
			ptrBuf += DEVICE_ID_LEN;

			sprintf(ptrBuf, "%-30s", deviceInfo.szCName);
			ptrBuf += 30;

			ptrBuf[0] = status[0];
			ptrBuf += 1;

			DBG_ANA(("设备状态变更[%s]\n", updateData));
			Net_Up_Msg(CMD_DEVICE_STATUS_UPDATE, updateData, ptrBuf - updateData);
		}
	}
}

/******************************************************************************/
/* Function Name  : InsertDataTable                                            */
/* Description    : 设备状态变更                                            */
/* Input          : int   vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* standard       正常值范围                           */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
void CAnalyser::InsertDataTable(MSG_ANALYSE_HASH hash_node, const char* szTableName, char* time, char* szValue , int status, bool bRedo)
{
	char sqlData[512] = {0};
	char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};
	char szAttrSn[DEVICE_SN_LEN_TEMP] = {0};
	char szStatus[4] = {0};

	memset(szDeviceId, 0, sizeof(szDeviceId));
	memset(szAttrSn, 0, sizeof(szAttrSn));

	memcpy(szDeviceId, hash_node.key, DEVICE_ID_LEN);
	memcpy(szAttrSn,   hash_node.key + DEVICE_ID_LEN, DEVICE_SN_LEN);

	sprintf(szStatus, "%d", status);

	strcat(sqlData, "null, ");
	strcat(sqlData, "'");
	strcat(sqlData, szDeviceId);        //设备编号
	strcat(sqlData, "','");
	strcat(sqlData, szAttrSn);
	strcat(sqlData, "', datetime('");
	strcat(sqlData, time);         //采集时间
	strcat(sqlData, "'), '");
	strcat(sqlData, szValue);        //采集数据
	strcat(sqlData, "', '");
	strcat(sqlData, szStatus);		//数据状态
	strcat(sqlData, "', '0', '', ''");         //发送状态
	g_SqlCtl.insert_into_table( szTableName, sqlData, bRedo);               //读取的数据插入数据库

}


/******************************************************************************/
void CAnalyser::f_DelData()
{
	char sqlData[512] = {0};
    //strcat(sqlData, "where julianday(strftime(\'%Y-%m-%d\',\'now\'))-julianday(strftime(\'%Y-%m-%d\', ctime))>60");
    strcat(sqlData, "where sn not in (select sn from data desc limit 0,1000000)" ); 
	g_SqlCtl.delete_record("data", sqlData, false);
}


/******************************************************************************/
/* Function Name  : f_valueChangedCheck                                       */
/* Description    : 数据变化判断                                            */
/* Input          : int   vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* lastValue      最新数据值                           */
/* Output         : none                                                      */
/* Return         : true       数据发生变化                               */
/*                  false      数据未发生变化                               */
/******************************************************************************/
bool CAnalyser::f_valueChangedCheck(int vType, char* value, UnionLastValue unionLastValue)
{
	bool ret = false;
	int cmpLen = 0;
	switch(vType)
	{
	case SHORT:
		cmpLen = sizeof(short);
		break;
	case INTEGER:
		cmpLen = sizeof(int);
		break;
	case FLOAT:
		cmpLen = sizeof(float);
		break;
	case DOUBLE:
		cmpLen = sizeof(double);
		break;
	case CHAR:
		cmpLen = sizeof(char);
		break;
	case CHARS:
	case BYTES:
	case BCD:
		cmpLen = 128;
		break;
	default:
		break;
	}
	if (cmpLen > 0)
	{
		if(0 != memcmp(value, &unionLastValue, cmpLen))//值有变化
		{
			ret = true;
		}
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : DecodeAct                                                 */
/* Description    : 数据联动分析                                              */
/* Input          : char* deviceId       数据来源设备编号                     */
/*                  char* ctype          数据种类                             */
/*                  char* vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* a_format       联动协议                             */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
void CAnalyser::DecodeAct(char* deviceId, const char* sn,int vType, char* value, int* pSeq, int actCnt, char* strTime)
{
    char act_msg[MAXMSGSIZE] = {0};
    char inCondition[129] = {0};                         //a_format条件
    char timeFormat[30] = {0};                          //有效时间范围
    time_t tm_NowTime;

    memset(act_msg, 0, sizeof(act_msg));
    ACTION_MSG *pSendMsg = (ACTION_MSG *)act_msg;
	ACT_FORMAT a_format;
    //---------------------------------遍历所有联动配置-------------------------------
    for( int j=0; j<actCnt; j++ )
    {
        memset(inCondition, 0, sizeof(inCondition));
        memset(timeFormat, 0, sizeof(timeFormat));
		memset((unsigned char*)&a_format, 0, sizeof(ACT_FORMAT));
		//lhy根据联动编号到联动配置表获取联动配置lhylhy
		HashedAgentInfo::iterator it;
		CTblLock.lock();
		a_format.iSeq = pSeq[j];
		it = m_pAgentHashTable->find((int)a_format.iSeq);	
		if (it == m_pAgentHashTable->end())
		{
			CTblLock.unlock();
			continue;
		}

		memcpy((unsigned char*)&a_format, (unsigned char*)&it->second.iSeq, sizeof(ACT_FORMAT));
		CTblLock.unlock();

        memcpy(inCondition, a_format.szCondition, sizeof(a_format.szCondition));
        DBG_ANA(("condition:[%s]\n", inCondition));
		char attrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
		strncpy(attrId, deviceId, DEVICE_ID_LEN);
		strncpy(attrId + DEVICE_ID_LEN, sn, DEVICE_SN_LEN);
		if(!ConditionCheck(attrId, vType, value, inCondition))//条件不满足，跳过此条配置
		{
			//该条件恢复正常，重新计数 lsd 2016.1.4
			if(1 != a_format.times_lastTime)
			{
				a_format.times_lastTime = 1;
				CTblLock.lock();
				it = m_pAgentHashTable->find((int)a_format.iSeq);	
				if (it == m_pAgentHashTable->end())
				{
					CTblLock.unlock();
					continue;
				}
				((PACT_FORMAT)&it->second.iSeq)->times_lastTime = a_format.times_lastTime;	//lsd 2016.1.4
				CTblLock.unlock();
			}
			continue;
		}

		memcpy(timeFormat, a_format.szValibtime, sizeof(a_format.szValibtime));
		time(&tm_NowTime);                 //获取当前时间
		if( tm_NowTime - a_format.time_lastTime < a_format.iInterval )
		{
			DBG_ANA(("尚在联动间隔内，跳过配置 NowTime[%ld] oldTime[%ld] 联动间隔[%d]\n", tm_NowTime, a_format.time_lastTime, a_format.iInterval));
			continue;
		}  
		DBG_ANA(("联动当前[%d] 一共[%d]\n", a_format.times_lastTime, a_format.iTimes));
		if( a_format.times_lastTime > a_format.iTimes && 0 != a_format.iTimes)	//iTimes=0表示不设置联动次数
		{
			DBG_ANA(("超出联动次数，跳过联动 联动当前[%d] 一共[%d]\n", a_format.times_lastTime, a_format.iTimes));
			continue;
		}
		if( f_timeCheck( tm_NowTime, timeFormat ) == false )
		{
			DBG_ANA(("不在布防时间内，跳过配置[%s]\n", timeFormat));
			continue;
		}   //时间检测不通过，跳过配置

		DBG_ANA(("DecodeAct Begin--deviceId[%s]- sn[%s]---actCnt:[%d] szCondition[%s] szActValue[%s]\n",deviceId, sn,  actCnt, a_format.szCondition, a_format.szActValue));
	    memcpy(pSendMsg->DstId, a_format.szActDevId, sizeof(a_format.szActDevId));    //目标设备ID
		pSendMsg->DstId[sizeof(a_format.szActDevId)] = '\0';
        

		memcpy(pSendMsg->ActionSn, a_format.szActActionId, sizeof(a_format.szActActionId));		
		memcpy(pSendMsg->ActionValue, a_format.szActValue, sizeof(a_format.szActValue));
		pSendMsg->ActionValue[sizeof(a_format.szActValue)] = '\0';
		char szData[257] = {0};
		memcpy(szData, pSendMsg->ActionValue, sizeof(pSendMsg->ActionValue));
		char* t_posi = strstr(szData, "[V]");
		if( t_posi != NULL )
		{
			char temp[257] = {0};
			memcpy(temp, szData, sizeof(szData));
			memset(szData, 0, sizeof(szData));
			t_posi = strstr(temp, "[V]");
			memcpy(szData, temp, t_posi - temp);
			strcat(szData, value);
			strcat(szData, t_posi+3);
			szData[256] = '\0';
			memset(pSendMsg->ActionValue, 0, sizeof(pSendMsg->ActionValue));
			strcpy(pSendMsg->ActionValue, szData);
		}

        a_format.time_lastTime = tm_NowTime;      //联动时间更新
		a_format.times_lastTime ++;				  //联动次数更新

        memcpy(pSendMsg->SrcId, deviceId, DEVICE_ID_LEN);                       //源设备ID
        pSendMsg->SrcId[DEVICE_ID_LEN] = '\0';

	    strncpy(pSendMsg->AttrSn, sn, DEVICE_SN_LEN);		//触发属性
		strncpy(pSendMsg->SrcValue, value, 128);//触发值

        memcpy(pSendMsg->Operator, "AUTO", 4);  //发起者
        pSendMsg->Operator[4] = '\0';
        pSendMsg->ActionSource = ACTION_SOURCE_SYSTEM;

	//	Time2Chars(&tm_NowTime, pSendMsg->szActionTimeStamp);
		strcpy(pSendMsg->szActionTimeStamp, strTime);
        DBG_ANA(("pSendMsg    ID:[%s]   Value:[%s]\n", pSendMsg->DstId, pSendMsg->ActionValue));
        

		//告警数据处理,lhy
		if (0 == strncmp(pSendMsg->DstId, CPM_GATEWAY_ID, DEVICE_ID_LEN))
		{
			char level[5] = {0};
			char szDescribe[257] = {0};
			char* valuePos = strstr(pSendMsg->ActionValue, ":");
			if (NULL == valuePos || 4 < (valuePos - pSendMsg->ActionValue) || valuePos == pSendMsg->ActionValue)
			{
				strcpy(szDescribe, pSendMsg->ActionValue);
			}
			else//生成级别
			{
				strncpy(level, pSendMsg->ActionValue, valuePos - pSendMsg->ActionValue);
				strcpy(szDescribe, valuePos + 1);
			}

			CSqlCtl::InsertAlertInfoTable(pSendMsg->SrcId, pSendMsg->AttrSn, pSendMsg->szActionTimeStamp,ALARM_GENERAL, szDescribe,'0',level);
			CSqlCtl::DeleteFromAlertNowTable(pSendMsg->SrcId, pSendMsg->AttrSn, 1);

			char deviceAttrId[DEVICE_ATTR_ID_LEN_TEMP] = {0};
			memcpy(deviceAttrId, deviceId, DEVICE_ID_LEN);
			memcpy(deviceAttrId + DEVICE_ID_LEN, sn, DEVICE_SN_LEN);
			DEVICE_INFO_POLL deviceNode;
			if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrNode(deviceAttrId, deviceNode))
			{
				memcpy(deviceNode.stLastValueInfo.szLevel, level, 4);
				memcpy(deviceNode.stLastValueInfo.szDescribe, szDescribe, 128);
				g_DevInfoContainer.UpdateDeviceAlarmValue(deviceNode);
			}
			//更新数据表记录
			const char* sqlFormat = "update %s "
				"set level = '%s', desc = '%s' "
				"where id = '%s' and ctype = '%s' and ctime = '%s';";
			char sql[512] = {0};
			sprintf(sql, sqlFormat, 
				"data",
				level, 
				szDescribe, 
				pSendMsg->SrcId, 
				pSendMsg->AttrSn, 
				pSendMsg->szActionTimeStamp);
			g_SqlCtl.ExecSql(sql, NULL, NULL, true, 0);

			//更新当前结点最新数据状态
			MSG_ANALYSE_HASH hash_node;
			memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
			memcpy(hash_node.key, deviceId, DEVICE_ID_LEN);
			memcpy(hash_node.key + DEVICE_ID_LEN, sn, DEVICE_SN_LEN);
			if( GetHashData(hash_node))
			{
				hash_node.dataStatus = 1;
				updateNodePara(hash_node, 4);
			}
		}
		
		CTblLock.lock();
		it = m_pAgentHashTable->find((int)a_format.iSeq);	
		if (it == m_pAgentHashTable->end())
		{
			CTblLock.unlock();
			continue;
		}
		((PACT_FORMAT)&it->second.iSeq)->time_lastTime = a_format.time_lastTime;
		((PACT_FORMAT)&it->second.iSeq)->times_lastTime = a_format.times_lastTime;	//lsd 2016.1.4
		CTblLock.unlock();

		int patchRet = CDevInfoContainer::DisPatchAction(*pSendMsg);
		if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != patchRet && SYS_STATUS_SUBMIT_SUCCESS_IN_LIST != patchRet)
		{
			CDevInfoContainer::SendToActionSource(*pSendMsg, patchRet);
		}
    }

	//DBG_ANA(("DecodeAct End--deviceId[%s]- sn[%s]---actCnt:[%d] szCondition[%s] szActValue[%s]\n",deviceId, sn,  actCnt, a_format->szCondition, a_format->szActValue));
}


/******************************************************************************/
/* Function Name  : DoOffLineAct                                                 */
/* Description    : 数据联动分析                                              */
/* Input          : char* deviceId       数据来源设备编号                     */
/*                  char* ctype          数据种类                             */
/*                  char* vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* a_format       联动协议                             */
/* Output         : none                                                      */
/* Return         : none                                                      */
/******************************************************************************/
void CAnalyser::DoOffLineAct(char* deviceId, const char* sn, int* pSeq, int actCnt, bool bIsReNotice)
{
	char act_msg[MAXMSGSIZE] = {0};

	char timeFormat[30] = {0};                          //有效时间范围
	time_t tm_NowTime;

	ACTION_MSG *pSendMsg = (ACTION_MSG *)act_msg;


	ACT_FORMAT a_format;
	//---------------------------------遍历所有联动配置-------------------------------
	for( int j=0; j<actCnt; j++ )
	{
		memset(act_msg, 0, sizeof(act_msg));
		memset(timeFormat, 0, sizeof(timeFormat));		
		memset((unsigned char*)&a_format, 0, sizeof(ACT_FORMAT));

		HashedAgentInfo::iterator it;
		CTblLock.lock();
		a_format.iSeq = pSeq[j];
		it = m_pAgentHashTable->find((int)a_format.iSeq);	
		if (it == m_pAgentHashTable->end())
		{
			CTblLock.unlock();
			continue;
		}
		
		memcpy((unsigned char*)&a_format, (unsigned char*)&it->second.iSeq, sizeof(ACT_FORMAT));
		CTblLock.unlock();

		memcpy(timeFormat, a_format.szValibtime, sizeof(a_format.szValibtime));
		time(&tm_NowTime);                 //获取当前时间
		
		if (bIsReNotice)//如果是重复通知
		{
			if (0 == a_format.iInterval//如果没有联动间隔
				|| ( tm_NowTime - a_format.time_lastTime < a_format.iInterval ))//尚在联动间隔内，跳过配置
			{
				DBG_ANA(("尚在联动间隔内或只需要一次联动，跳过配置 NowTime[%ld] oldTime[%ld] 联动间隔[%d]\n", tm_NowTime, a_format.time_lastTime, a_format.iInterval));
				continue;
			}
			DBG_ANA(("重复通知 NowTime[%ld] oldTime[%ld] 联动间隔[%d]\n", tm_NowTime, a_format.time_lastTime, a_format.iInterval));
		}

		if( false == f_timeCheck( tm_NowTime, timeFormat ))
		{
			DBG_ANA(("不在布防时间内，跳过配置[%s]\n", timeFormat));
			continue;
		}   //时间检测不通过，跳过配置

		memcpy(pSendMsg->DstId, a_format.szActDevId, sizeof(a_format.szActDevId));    //目标设备ID
		pSendMsg->DstId[sizeof(a_format.szActDevId)] = '\0';

		memcpy(pSendMsg->ActionSn, a_format.szActActionId, sizeof(a_format.szActActionId));

		memcpy(pSendMsg->ActionValue, a_format.szActValue, sizeof(a_format.szActValue));
		pSendMsg->ActionValue[sizeof(a_format.szActValue)] = '\0';

		a_format.time_lastTime = tm_NowTime;      //联动时间更新

		memcpy(pSendMsg->SrcId, deviceId, DEVICE_ID_LEN);                       //源设备ID
		pSendMsg->SrcId[DEVICE_ID_LEN] = '\0';

		strncpy(pSendMsg->AttrSn, sn, DEVICE_SN_LEN);		//触发属性

		memcpy(pSendMsg->Operator, "AUTO", 4);  //发起者
		pSendMsg->Operator[4] = '\0';
		pSendMsg->ActionSource = ACTION_SOURCE_SYSTEM;
		DBG_ANA(("DoOffLineAct pSendMsg    ID:[%s]   Value:[%s]\n", pSendMsg->DstId, pSendMsg->ActionValue));


		CTblLock.lock();
		it = m_pAgentHashTable->find((int)a_format.iSeq);	
		if (it == m_pAgentHashTable->end())
		{
			CTblLock.unlock();
			continue;
		}
		((PACT_FORMAT)&it->second.iSeq)->time_lastTime = a_format.time_lastTime;
	
		CTblLock.unlock();

		int patchRet = CDevInfoContainer::DisPatchAction(*pSendMsg);

		if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != patchRet && SYS_STATUS_SUBMIT_SUCCESS_IN_LIST != patchRet)
		{
			CDevInfoContainer::SendToActionSource(*pSendMsg, patchRet);
		}
	}
}



/******************************************************************************/
bool CAnalyser::f_timeCheck(time_t tm_NowTime, char *timeFormat)
{
    bool ret = false;
	char timeTemp[2][15] = {{0}};
    char *split_result = NULL;
    struct tm st_startTime;
    struct tm st_endTime;
    struct tm *st_nowTime;
    char timeTrans[3] = {0};
	char* savePtr = NULL;
    if( 0>= strlen(timeFormat))
    {
        return true;
    }
    split_result = strtok_r(timeFormat, ",", &savePtr);
    if( split_result != NULL )
    {
        strcat(timeTemp[0], split_result);
        split_result = strtok_r(NULL, ",", &savePtr);
        if( split_result != NULL )
        {
            strcat(timeTemp[1], split_result);
        }
        else
        {
            return ret;
        }
    }
    else
    {
        return ret;
    }

    if( timeTemp[0][0] == '*' )            //日循环
    {
        st_nowTime = localtime(&tm_NowTime);
        memset(timeTrans, 0, sizeof(timeTrans));
        timeTrans[0] = timeTemp[0][1];
        timeTrans[1] = timeTemp[0][2];
        st_startTime.tm_hour = atoi(timeTrans);
        memset(timeTrans, 0, sizeof(timeTrans));
        timeTrans[0] = timeTemp[0][3];
        timeTrans[1] = timeTemp[0][4];
        st_startTime.tm_min = atoi(timeTrans);
        memset(timeTrans, 0, sizeof(timeTrans));
        timeTrans[0] = timeTemp[1][1];
        timeTrans[1] = timeTemp[1][2];
        st_endTime.tm_hour = atoi(timeTrans);
        memset(timeTrans, 0, sizeof(timeTrans));
        timeTrans[0] = timeTemp[1][3];
        timeTrans[1] = timeTemp[1][4];
        st_endTime.tm_min = atoi(timeTrans);

        //DBG_ANA(("start time:[%d:%d]\n", st_startTime.tm_hour, st_startTime.tm_min));
        //DBG_ANA(("end time:[%d:%d]\n", st_endTime.tm_hour, st_endTime.tm_min));
        //DBG_ANA(("now time:[%d:%d]\n", st_nowTime->tm_hour, st_nowTime->tm_min));
        if( (st_startTime.tm_hour < st_endTime.tm_hour)         //开始、结束时间在同一天
        || ( (st_startTime.tm_hour==st_endTime.tm_hour) && (st_startTime.tm_min<st_endTime.tm_min) ) )
        {
            if( st_nowTime->tm_hour > st_startTime.tm_hour )            //
            {
                if( st_nowTime->tm_hour < st_endTime.tm_hour )
                {
                    ret = true;
                }
                else if( (st_nowTime->tm_hour == st_endTime.tm_hour) && (st_nowTime->tm_min <= st_endTime.tm_hour))
                {
                    ret =true;
                }
            }
            else if( st_nowTime->tm_hour == st_startTime.tm_hour )
            {
                if( st_nowTime->tm_min >= st_startTime.tm_min )
                {
                    if( st_nowTime->tm_hour < st_endTime.tm_hour )
                    {
                        ret = true;
                    }
                    else if( (st_nowTime->tm_hour == st_endTime.tm_hour) && (st_nowTime->tm_min <= st_endTime.tm_min) )
                    {
                        ret = true;
                    }
                }
            }
        }
        else                      //开始、结束时间隔天
        {
            if( (st_nowTime->tm_hour > st_startTime.tm_hour) || (st_nowTime->tm_hour < st_endTime.tm_hour) )
            {
                ret = true;
            }
            else if( ( st_nowTime->tm_hour == st_startTime.tm_hour) && ( st_nowTime->tm_min >= st_startTime.tm_min) )
            {
                ret = true;
            }
            else if( ( st_nowTime->tm_hour == st_endTime.tm_hour) && ( st_nowTime->tm_min <= st_endTime.tm_min) )
            {
                ret = true;
            }
        }
    }
    else                                   //时间段
    {
    }

    return ret;
}

/******************************************************************************/
/* Function Name  : conditionCheck                                            */
/* Description    : 条件匹配                                                  */
/* Input          : char* value               数据值                          */
/*                  char* split_result_pos    条件                            */
/* Output         : none                                                      */
/* Return         : true   条件匹配成功                                       */
/*                  false  条件匹配失败                                       */
/******************************************************************************/
bool CAnalyser::ConditionCheck(char* attrId, int vType, const char* curValue, char* strConditions)//只支持“与”
{
	bool ret = true; 
	char* savePtr = NULL;
	char* split_result = NULL;
	split_result = strtok_r(strConditions, "@", &savePtr);
	DBG_ANA(("ConditionCheck   attrId[%s] vType[%d] curValue[%s] condition[%s]\n", attrId, vType, curValue, strConditions));
	while(split_result != NULL)
	{
		if(!IsTrue(attrId, vType, curValue, split_result))
		{
			ret = false;
			break;
		}
		split_result = strtok_r(NULL, "@", &savePtr);
	}
	return ret;
}

/******************************************************************************/
bool CAnalyser::IsTrue(char* pAttrId, int vType, const char* curValue, char* expression)
{
	bool ret = false;
	char compareFlag = 0;
	char* pPperator = NULL;
	char szPre[128] = {0};
	char szNext[128] = {0};

	DBG_ANA(("IsTrue   attrId[%s] vType[%d] curValue[%s] expression[%s]\n", pAttrId, vType, curValue, expression));
	if(NULL != (pPperator = strstr(expression, "<")))
	{
		memcpy(szPre, expression, pPperator - expression);
		strcpy(szNext,pPperator + 1);
		compareFlag = 1;
	}
	else if (NULL != (pPperator = strstr(expression, ">")))
	{
		memcpy(szPre, expression, pPperator - expression);
		strcpy(szNext,pPperator + 1);
		compareFlag = 2;
	}
	else if (NULL != (pPperator = strstr(expression, "=")))
	{
		memcpy(szPre, expression, pPperator - expression);
		strcpy(szNext,pPperator + 1);
		compareFlag = 3;
	}
	else if (NULL != (pPperator = strstr(expression, "!=")))
	{
		memcpy(szPre, expression, pPperator - expression);
		strcpy(szNext,pPperator + 2);
		compareFlag = 4;
	}
	else
	{
		return ret;
	}

	//计算条件表达式比较符左边数值
	DBG_ANA(("IsTrue   szPre[%s] szNext[%s] compareFlag[%d]\n", szPre, szNext,compareFlag));
	char tempCalFunc[256] = {0};
	float fRetPre = 0.0;
	transPolish1(szPre, tempCalFunc);//转换为逆波兰式
	DBG_ANA(("IsTrue after transPolish1 11 tempCalFunc[%s] curValue[%s]\n", tempCalFunc, curValue));
	int cacuRetPre = compvalue1(pAttrId, vType, curValue, tempCalFunc, fRetPre);
	DBG_ANA(("IsTrue after compvalue1 Pre cacuRetPre[%d] fRetPre[%f]\n", cacuRetPre, fRetPre));

	//计算条件表达式比较符右边数值
	float fRetNext = 0.0;
	memset(tempCalFunc, 0, sizeof(tempCalFunc));
	transPolish1(szNext, tempCalFunc);
	DBG_ANA(("IsTrue after transPolish1 22 tempCalFunc[%s] \n", tempCalFunc));
	int cacuRetNext = compvalue1(pAttrId, vType, curValue, tempCalFunc, fRetNext);
	DBG_ANA(("IsTrue after compvalue1 Next cacuRetNext[%d] fRetNext[%f]\n", cacuRetNext, fRetNext));

	if (SYS_STATUS_SUCCESS == cacuRetPre && SYS_STATUS_SUCCESS == cacuRetNext)//都是数值计算
	{
		DBG_ANA(("IsTrue after 都是数值计算 pre[%f] next[%f]\n", fRetPre, fRetNext));
		switch(compareFlag)
		{
		case 1:
			if (fRetPre < fRetNext)
			{
				ret = true;
			}
			break;
		case 2:
			if (fRetPre > fRetNext)
			{
				ret = true;
			}
			break;
		case 3:
			if (fRetPre == fRetNext)
			{
				ret = true;
			}
			break;
		case 4:
			if (fRetPre != fRetNext)
			{
				ret = true;
			}
			break;
		}
	}
	else if(SYS_STATUS_CACULATE_MODE_ERROR == cacuRetPre || SYS_STATUS_CACULATE_MODE_ERROR == cacuRetNext)//字符串计算
	{
		char szPreValue[512] = {0};
		char szNextValue[512] = {0};

		DBG_ANA(("IsTrue after 不是数值计算，默认使用字符串比较 pre[%s] next[%s]\n", szPre, szNext));
		if(IsAttrId(szPre))
		{
			char* ptrAttrId = szPre + 1;
			if (0 == strncmp(ptrAttrId, pAttrId, DEVICE_ATTR_ID_LEN))//触发属性
			{
				if (CHARS == vType)
				{
					memset(szPreValue, 0, sizeof(szPreValue));
					strcpy(szPreValue, curValue);
				}
			}
			else
			{
				LAST_VALUE_INFO stLastValue;
				if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrValue(ptrAttrId, stLastValue))
				{
					memset(szPreValue, 0, sizeof(szPreValue));
					strcpy(szPreValue, stLastValue.Value);
				}
				else
				{
					DBG_ANA(("IsTrue Pre无法找到[%s]对应的结点\n",ptrAttrId));
				}
			}
			DBG_ANA(("IsTrue Pre是属性编号，最终值为[%s]\n",szPreValue));
			if (0 == strncmp(szPreValue, "NULL", 4))
			{
				compareFlag = 3;
			}
		}
		else
		{
			strcpy(szPreValue, szPre);
		}

		if(IsAttrId(szNext))
		{
			char* ptrAttrId = szNext + 1;
			if (0 == strncmp(ptrAttrId, pAttrId, DEVICE_ATTR_ID_LEN))//触发属性
			{
				if (CHARS == vType)
				{
					memset(szNextValue, 0, sizeof(szNextValue));
					strcpy(szNextValue, curValue);
				}
			}
			else
			{
				LAST_VALUE_INFO stLastValue;
				if(SYS_STATUS_SUCCESS == g_DevInfoContainer.GetAttrValue(ptrAttrId, stLastValue))
				{
					memset(szNextValue, 0, sizeof(szNextValue));
					strcpy(szNextValue, stLastValue.Value);
				}
				else
				{
					DBG_ANA(("IsTrue Next无法找到[%s]对应的结点\n",ptrAttrId));
				}
			}
			DBG_ANA(("IsTrue Next是属性编号，最终值为[%s]\n",szNextValue));

			if (0 == strncmp(szPreValue, "NULL", 4))
			{
				compareFlag = 3;
			}
		}
		else
		{
			strcpy(szNextValue, szNext);
		}

		DBG_ANA(("IsTrue 转换后LastPre[%s] LastNext[%s]\n",szPreValue, szNextValue));
		switch(compareFlag)
		{
		case 1:
			if (0 > strcmp(szPreValue, szNextValue))
			{
				ret = true;
			}
			break;
		case 2:
			if (0 < strcmp(szPreValue, szNextValue))
			{
				ret = true;
			}
			break;
		case 3:
			if (0 == strcmp(szPreValue, szNextValue))
			{
				ret = true;
			}
			break;
		case 4:
			if (0 != strcmp(szPreValue, szNextValue))
			{
				ret = true;
			}
			break;
		}
	}
	return ret;
}

/******************************************************************************/
/* Function Name  : transPolish                                               */
/* Description    : 计算式转换成逆波兰式                                      */
/* Input          : szRdCalFunc     计算式                                    */
/* Output         : tempCalFunc     逆波兰式                                  */
/* Return         :                                                           */
/******************************************************************************/
void CAnalyser::transPolish1(char* szRdCalFunc, char* tempCalFunc)
{
	char stack[128] = {0};
	char ch;
	int i = 0,t = 0,top = 0;

	ch = szRdCalFunc[i];
	i++;

	while( ch != 0)          //计算式结束
	{
		switch( ch )
		{
		case '+':
		case '-': 
			while(top != 0 && stack[top] != '(')
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top++;
			stack[top] = ch;
			break;
		case '*':
		case '/':
			while(stack[top] == '*'|| stack[top] == '/')
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top++;
			stack[top] = ch;
			break;
		case '(':
			top++;
			stack[top] = ch;
			break;
		case ')':
			while( stack[top]!= '(' )
			{
				tempCalFunc[t] = stack[top];
				top--;
				t++;
			}
			top--;
			break;
		case ' ':break;
		default:

			while( isdigit(ch) || ch == '.' || IsAlp(ch))
			{
				tempCalFunc[t] = ch;
				t++;
				ch = szRdCalFunc[i];
				i++;
			}
			i--;
			tempCalFunc[t] = ' ';	//数字之后要加空格，以避免和后面的数字连接在一起无法正确识别真正的位数
			t++;
		}
		ch = szRdCalFunc[i];
		i++;

		DBG_ANA(("tempCalFunc[%s]\n", tempCalFunc));
	}
	while(top!= 0)
	{
		tempCalFunc[t] = stack[top];
		t++;
		top--;
	}
	tempCalFunc[t] = ' ';
}
bool CAnalyser::IsAlp(char c)
{
	return ((c <= 'z' && c >= 'a') || (c >= 'A' && c <= 'Z'));
}

bool CAnalyser::IsDigit(char* ch)
{
	bool ret = true;
	for(int i=0; i<(int)strlen(ch); i++)
	{
		if(!isdigit(ch[i]) && ch[i] != '.')
		{
			ret =false;
			break;
		}
	}
	return ret;
}
bool CAnalyser::IsAttrId(char* ch)
{
	return 'P' == ch[0] && (DEVICE_ATTR_ID_LEN + 1) == strlen(ch) && IsDigit(ch + 1);
}


/******************************************************************************/
/* Function Name  : compvalue                                                 */
/* Description    : 逆波兰式计算结果                                          */
/* Input          : tempCalFunc     逆波兰式                                  */
/*                  midResult       计算用数值                                */
/* Output         :                                                           */
/* Return         : 计算结果                                                  */
/******************************************************************************/
int CAnalyser::compvalue1(char* pAttrId, int vType, const char* curValue, char* tempCalFunc, float& fRsult)
{
	float stack[20];
	char ch = 0;
	char str[100] = {0};
	int i = 0, t = 0,top = 0;
	ch = tempCalFunc[t];
	t++;

	while(ch!= ' ')
	{
		switch(ch)
		{
		case '+':
			stack[top-1] = stack[top-1] + stack[top];
			top--;
			break;
		case '-':
			stack[top-1] = stack[top-1] - stack[top];
			top--;
			break;
		case '*':
			stack[top-1] = stack[top-1] * stack[top];
			top--;
			break; 
		case '/':
			if(stack[top]!= 0)
				stack[top-1] = stack[top-1]/stack[top];
			else
			{
				DBG_ANA(("除零错误！\n"));
				return SYS_STATUS_FORMAT_ERROR;
			}
			top--;
			break;
		default:
			i = 0;
			while( isdigit(ch) || ch == '.' || IsAlp(ch))
			{
				str[i] = ch;
				i++;
				ch = tempCalFunc[t];
				t++;
			}
			str[i] = '\0';
			if (IsAttrId(str))//找到属性结点
			{
				DBG_ANA(("compvalue1 是属性结点[%s] 实际属性AttrId[%s]\n", str, pAttrId));
				if (0 == strcmp(str+1, pAttrId))//触发属性
				{
					if (CHARS == vType)
					{
						return SYS_STATUS_CACULATE_MODE_ERROR;
					}
					top++;
					stack[top] = atof(curValue);
				}
				else//其他属性
				{
					DEVICE_INFO_POLL devicePoll;
					if (SYS_STATUS_SUCCESS != g_DevInfoContainer.GetAttrNode(str+1, devicePoll))
					{
						return SYS_STATUS_DEVICE_NOT_EXIST;
					}
					if (IsDigit(devicePoll.stLastValueInfo.Value) && FLOAT == devicePoll.stLastValueInfo.VType)//不是离线值，并且类型为浮点型
					{
						top++;
						stack[top] = atof(devicePoll.stLastValueInfo.Value);
					}
					else
					{
						return SYS_STATUS_CACULATE_MODE_ERROR;
					}
				}
			}
			else if (IsDigit(str))//是实际数值
			{
				DBG_ANA(("compvalue1 是常量[%s]\n", str));
				top++;
				stack[top] = atof(str);
			}
			else
			{
				DBG_ANA(("compvalue1 解析失败[%s]\n", str));
				return SYS_STATUS_CACULATE_MODE_ERROR;
			}

		}
		ch = tempCalFunc[t];
		t++;
	}
	fRsult = stack[top];
	return SYS_STATUS_SUCCESS;
}

/******************************************************************************/
/* Function Name  : conditionCheck                                            */
/* Description    : 条件匹配                                                  */
/* Input          : char* value               数据值                          */
/*                  char* split_result_pos    条件                            */
/* Output         : none                                                      */
/* Return         : true   条件匹配成功                                       */
/*                  false  条件匹配失败                                       */
/******************************************************************************/
bool CAnalyser::conditionCheck(int vType, const char* value, char* split_result_pos)
{
    bool ret = true;
    const char* split = ",";
    char* split_result = NULL;
    int recValue_interg = 0;           //轮询取得的值
    double recValue_doub = 0;
    int conValue_interg = 0;           //基准值
    double conValue_doub = 0;

    if( vType == INTEGER )
    {
        recValue_interg = atoi(value);
        //DBG_ANA(("RecValue:[%d]", recValue_interg));
    }
    else if( vType == FLOAT )
    {
        recValue_doub = atof(value);
        //DBG_ANA(("RecValue:[%f]", recValue_doub));
    }

	char* savePtr = NULL;
    split_result = strtok_r(split_result_pos, ",", &savePtr);
    while(split_result != NULL)
    {
        //DBG_ANA(("Condition:[%s]\n", split_result));
        if( *split_result == '*' )        //首字节为 *,表示任何值均使条件成立
        {
            //DBG_ANA(("No Condition\n"));
            ret = true;
            return ret;
        }
        else if( *split_result == '<' )
        {
            if( vType == INTEGER )
            {
                conValue_interg = atoi(split_result+1);
                //DBG_ANA(("convalue:[%d]\n", conValue_interg));
                if( recValue_interg < conValue_interg ){    ret = true;  }
                else{    return false;  }    //任何条件不满足，均为条件不成立
            }
            else if( vType == FLOAT )
            {
                conValue_doub = atof(split_result+1);
                //DBG_ANA(("convalue <:[%f]\n", conValue_doub));
                if( recValue_doub < conValue_doub ){    ret = true;  }
                else{    return false;  }
            }
        }
        else if( *split_result == '>' )
        {
            if( vType == INTEGER )
            {
                conValue_interg = atoi(split_result+1);
                DBG_ANA(("convalue:[%d]\n", conValue_interg));
                if( recValue_interg > conValue_interg ){    ret = true;  }
                else{    return false;  }
            }
            else if( vType == FLOAT )
            {
                conValue_doub = atof(split_result+1);
                DBG_ANA(("convalue >:[%f]\n", conValue_doub));
                if( recValue_doub > conValue_doub ){    ret = true;  }
                else{    return false;  }
            }
        }
        else if( *split_result == '=' )
        {
            if( vType == INTEGER )
            {
                conValue_interg = atoi(split_result+1);
                //DBG_ANA(("convalue:[%d]\n", conValue_interg));
                if( recValue_interg == conValue_interg ){    ret = true;  }
                else{    return false;  }
            }
            else if( vType == FLOAT )
            {
                conValue_doub = atof(split_result+1);
                //DBG_ANA(("convalue =:[%f]\n", conValue_doub));
                if( recValue_doub == conValue_doub ){    ret = true;  }
                else{    return false;  }
            }
        }

        split_result = strtok_r(NULL, split, &savePtr);
    }
    return ret;
}


/******************************************************************************/
/* Function Name  : addNode                                                   */
/* Description    : 向设备动作表加入节点                                      */
/* Input          : MSG_ACT_HASH hash_node      设备节点                      */
/* Output         : none                                                      */
/* Return         : true   节点已插入                                         */
/*                  false  节点已更新                                         */
/******************************************************************************/
bool CAnalyser::addNode(MSG_ANALYSE_HASH hash_node)
{
    int ret = false;
    MSG_ANALYSE_HASH msg_node;
    HashedInfo::iterator it;

    memset((BYTE*)&msg_node, 0, sizeof(msg_node));
    memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(MSG_ANALYSE_HASH));

    CTblLock.lock();
    it = m_pActHashTable->find(msg_node.key);
    if (it == m_pActHashTable->end())
    {		
  		m_pActHashTable->insert(HashedInfo::value_type(msg_node.key,msg_node));
		ret = true;
	}
    CTblLock.unlock();
    return ret;
}


/******************************************************************************/
/* Function Name  : delNode                                                   */
/* Description    : 从设备动作表里删除节点                                    */
/* Input          : MSG_ACT_HASH hash_node      设备节点                      */
/* Output         : none                                                      */
/* Return         : true   节点删除成功                                       */
/*                  false  节点删除失败                                       */
/******************************************************************************/
bool CAnalyser::delNode(MSG_ANALYSE_HASH hash_node)
{
    HashedInfo::iterator it;
	CTblLock.lock();
    it = m_pActHashTable->find(hash_node.key);	
	if (it != m_pActHashTable->end())
	{
		m_pActHashTable->erase(it);
	}
	CTblLock.unlock();
	return true;
}


/******************************************************************************/
/* Function Name  : updateNodePara                                            */
/* Description    : 更新设备动作表节点                                        */
/* Input          : MSG_ACT_HASH hash_node      设备节点                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CAnalyser::updateNodePara(MSG_ANALYSE_HASH hash_node, int paraId)
{
	bool ret = false;
	HashedInfo::iterator it;
	CTblLock.lock();
	it = m_pActHashTable->find(hash_node.key);
	if (it != m_pActHashTable->end())
	{
		PMSG_ANALYSE_HASH pNode = (PMSG_ANALYSE_HASH)(&it->second.key);
		switch (paraId)
		{
		case 1://UnionLastValue
			memset((unsigned char*)&pNode->unionLastValue, 0, sizeof(pNode->unionLastValue));
			memcpy((unsigned char*)&pNode->unionLastValue, (unsigned char*)&hash_node.unionLastValue, sizeof( hash_node.unionLastValue));
			break;
		case 2://LastTime
			memset((unsigned char*)&pNode->LastTime, 0, sizeof(pNode->LastTime));
			memcpy((unsigned char*)&pNode->LastTime, (unsigned char*)&hash_node.LastTime, sizeof( hash_node.LastTime));
			break;
		case 3://ActFormat,联动时间更新
			ACT_FORMAT act_format;
			for(int i=0; i<MAX_CNT_ACTFORMAT; i++)
			{				
				memset((unsigned char*)&act_format, 0, sizeof(ACT_FORMAT));
				act_format.iSeq = pNode->agentSeq[i];
				HashedAgentInfo::iterator agentIt = m_pAgentHashTable->find(act_format.iSeq);
				if (agentIt != m_pAgentHashTable->end())
				{
					PACT_FORMAT pNode = (PACT_FORMAT)(&agentIt->second.iSeq);
					pNode->time_lastTime = pNode->time_lastTime;
				}
			}
			break;
		case 4://最终数据状态
			pNode->dataStatus = hash_node.dataStatus;
			break;
		}
		ret = true;
	}
	CTblLock.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : searchNode                                                */
/* Description    : 在哈希表里查找节点                                        */
/* Input          : MSG_ACT_HASH hash_node      设备节点                      */
/* Output         : none                                                      */
/* Return         : true   查找到节点                                         */
/*                  false  未查到节点                                         */
/******************************************************************************/
bool CAnalyser::searchNode(MSG_ANALYSE_HASH hash_node)
{
    bool ret = false;
    HashedInfo::iterator it;
	CTblLock.lock();
    //DBG_ANA(("hashnode.key:[%s]\n", hash_node.key));
    it = m_pActHashTable->find(hash_node.key);	
	if (it != m_pActHashTable->end())
	{
		ret = true;
	}
	CTblLock.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : GetHashData                                               */
/* Description    : 从哈希表中寻找节点并取出内容                              */
/* Input          : char* deviceId       设备编号                             */
/*                  char* ctype          数据种类                             */
/*                  char* value          数据值                               */
/*                  char* a_format       联动协议                             */
/* Output         : none                                                      */
/* Return         : true                                                      */
/*                  false                                                     */
/******************************************************************************/
bool CAnalyser::GetHashData(MSG_ANALYSE_HASH& hash_node)
{
    bool ret = false;
    HashedInfo::iterator it;
	CTblLock.lock();
    it = m_pActHashTable->find(hash_node.key);
    if (it != m_pActHashTable->end())
	{
        memcpy((BYTE*)&hash_node, (BYTE*)&it->second, sizeof(MSG_ANALYSE_HASH));
        ret = true;
    }
	CTblLock.unlock();
    return ret;
}


//设备状态发生变化时，所有子sn均需变化
bool CAnalyser::deviceStatusUpdate(char* devId, int status)
{
	bool ret = false;
    CTblLock.lock();
    HashedInfo::iterator it;
    for (it = m_pActHashTable->begin(); it != m_pActHashTable->end(); it++)
	{
        if( 0 == memcmp(it->second.key, devId, DEVICE_ID_LEN))
        {
            ((PMSG_ANALYSE_HASH)(&it->second))->deviceStatus = status;
			ret = true;
        }
    }
    CTblLock.unlock();
    return ret;
}

/******************************************************************************/
/* Function Name  : AnalyseHashAgentCheck                                   */
/* Description    : 更新设备动作表联动配置                                    */
/* Input          : device_attr_id 设备编号					                      */
/*		          : actions 联动配置					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CAnalyser::AnalyseHashAgentCheck( AgentInfo stAgentInfo)
{
	int ret = SYS_STATUS_FAILED;
	HashedInfo::iterator it;
	MSG_ANALYSE_HASH hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
	CTblLock.lock();
	
	DBG_ANA(("检查设备联动结点[%s] Seq[%d]\n", stAgentInfo.szDevId,stAgentInfo.iSeq));
	strcpy(hash_node.key, stAgentInfo.szDevId);
	if ('1' == stAgentInfo.cType)//离线联动
	{		
		ret = SYS_STATUS_SUCCESS;
	}
	else//		
	{
		strncpy(hash_node.key + DEVICE_ID_LEN, stAgentInfo.szAttrId, DEVICE_SN_LEN);
		it = m_pActHashTable->find(hash_node.key);
		if (it != m_pActHashTable->end())
		{
			PMSG_ANALYSE_HASH curr_node = (PMSG_ANALYSE_HASH)(&it->second.key);
			if (curr_node->actCnt < MAX_CNT_ACTFORMAT)
			{
				ret = SYS_STATUS_SUCCESS;
				DBG_ANA(("添加联动成功 当前[%s]联动条数[%d]\n", curr_node->key, curr_node->actCnt));
			}
			else
			{
				ret = SYS_STATUS_DEVICE_ANALYSE_ACTION_FULL;
				DBG_ANA(("AnalyseHashAgentAdd  联动条数已满 当前[%s]联动条数[%d]\n", curr_node->key, curr_node->actCnt));
			}
		}	
	}
	CTblLock.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : AnalyseHashAgentAdd                                    */
/* Description    : 更新设备动作表联动配置                                        */
/* Input          : device_attr_id 设备编号					                      */
/*		          : actions 联动配置					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CAnalyser::AnalyseHashAgentAdd( AgentInfo stAgentInfo)
{
	int ret = SYS_STATUS_FAILED;
	HashedInfo::iterator it;
	MSG_ANALYSE_HASH hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
	CTblLock.lock();
	DBG_ANA(("添加设备联动结点[%s] Seq[%d]\n", stAgentInfo.szDevId,stAgentInfo.iSeq));
	strcpy(hash_node.key, stAgentInfo.szDevId);
	if ('1' == stAgentInfo.cType)//离线联动
	{
		strncpy(hash_node.key + DEVICE_ID_LEN, "0000", DEVICE_SN_LEN);
		it = m_pActHashTable->find(hash_node.key);
		if (it == m_pActHashTable->end())
		{
			memset((unsigned char*)&hash_node.agentSeq[0], 0, sizeof(int) * MAX_CNT_ACTFORMAT);
			hash_node.actCnt = 0;
			m_pActHashTable->insert(HashedInfo::value_type(hash_node.key,hash_node));
			DBG_ANA(("添加设备离线联动结点[%s]\n", hash_node.key));
		}
	}
	else//
		strncpy(hash_node.key + DEVICE_ID_LEN, stAgentInfo.szAttrId, DEVICE_SN_LEN);

	
	it = m_pActHashTable->find(hash_node.key);
	if (it != m_pActHashTable->end())
	{
		PMSG_ANALYSE_HASH curr_node = (PMSG_ANALYSE_HASH)(&it->second.key);
		if (curr_node->actCnt < MAX_CNT_ACTFORMAT)
		{
			getAgentMsg(&curr_node->agentSeq[curr_node->actCnt], stAgentInfo);
			curr_node->actCnt++;
			ret = SYS_STATUS_SUCCESS;
			DBG_ANA(("添加联动成功 当前[%s]SEQ[%d]联动条数[%d]\n", curr_node->key, curr_node->agentSeq[curr_node->actCnt-1],curr_node->actCnt));
		}
		else
		{
			ret = SYS_STATUS_DEVICE_ANALYSE_ACTION_FULL;
			DBG_ANA(("AnalyseHashAgentAdd  联动条数已满 当前[%s]联动条数[%d]\n", curr_node->key, curr_node->actCnt));
		}
	}	
	else
	{
		DBG_ANA(("未找到对应的设备结点[%s]\n", hash_node.key));
		ret = SYS_STATUS_DEVICE_ANALYSE_NOT_EXIST;
	}
	CTblLock.unlock();
	return ret;
}


/******************************************************************************/
/* Function Name  : AnalyseHashAgentUpdate                                    */
/* Description    : 更新设备动作表联动配置                                        */
/* Input          : device_attr_id 设备编号					                      */
/*		          : actions 联动配置					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
int CAnalyser::AnalyseHashAgentUpdate( AgentInfo stAgentInfo)
{
	int ret = SYS_STATUS_FAILED;
	HashedInfo::iterator it;
	MSG_ANALYSE_HASH hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
	strncpy(hash_node.key, stAgentInfo.szDevId, DEVICE_ID_LEN);

	strncpy(hash_node.key + DEVICE_ID_LEN, "0000", DEVICE_SN_LEN);
	if (0 < strlen(stAgentInfo.szAttrId) && 0 != strncmp(stAgentInfo.szAttrId, "    ", DEVICE_SN_LEN))
	{
		strncpy(hash_node.key + DEVICE_ID_LEN, stAgentInfo.szAttrId, DEVICE_SN_LEN);
	}

	CTblLock.lock();
	it = m_pActHashTable->find(hash_node.key);
	if (it != m_pActHashTable->end())
	{
		PMSG_ANALYSE_HASH curr_node = (PMSG_ANALYSE_HASH)(&it->second.key);
		for(int i=0; i<curr_node->actCnt; i++)
		{
			if (stAgentInfo.iSeq == curr_node->agentSeq[i])
			{
				getAgentMsg(&curr_node->agentSeq[i], stAgentInfo);
				ret = SYS_STATUS_SUCCESS;
			}
		}
	}	
	CTblLock.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : AnalyseHashAgentDelete                                    */
/* Description    : 更新设备动作表联动配置                                     */
/* Input          : device_attr_id 设备编号					                      */
/*		          : actions 联动配置					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CAnalyser::AnalyseHashAgentDelete( AgentInfo stAgentInfo)
{
	bool ret = false;
	HashedInfo::iterator it;
	MSG_ANALYSE_HASH hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
	strncpy(hash_node.key, stAgentInfo.szDevId, DEVICE_ID_LEN);
	strncpy(hash_node.key + DEVICE_ID_LEN, "0000", DEVICE_SN_LEN);
	if (0 < strlen(stAgentInfo.szAttrId) && 0 != strncmp(stAgentInfo.szAttrId, "    ", DEVICE_SN_LEN))
	{
		strncpy(hash_node.key + DEVICE_ID_LEN, stAgentInfo.szAttrId, DEVICE_SN_LEN);
	}

	DBG_ANA(("AnalyseHashAgentDelete 从联动表中删除[%s]\n", hash_node.key));
	CTblLock.lock();
	
	DBG_ANA(("AnalyseHashAgentDelete ...........111111\n"));
	it = m_pActHashTable->find(hash_node.key);
	if (it != m_pActHashTable->end())
	{
		int index = -1;
		PMSG_ANALYSE_HASH curr_node = (PMSG_ANALYSE_HASH)(&it->second.key);
		DBG_ANA(("AnalyseHashAgentDelete ...........2222222\n"));
		for(int i=0; i<curr_node->actCnt; i++)
		{
			DBG_ANA(("AnalyseHashAgentDelete iSeq[%d] oldSeq[%d]\n", stAgentInfo.iSeq, curr_node->agentSeq[i]));
			if (stAgentInfo.iSeq == curr_node->agentSeq[i])
			{
				index = i;
				break;
			}
		}

		DBG_ANA(("AnalyseHashAgentDelete ...........333333\n"));
		if (0 <= index)//lhy 检查
		{
			curr_node->agentSeq[index] = 0;
			memcpy((unsigned char*)&curr_node->agentSeq[index], (unsigned char*)&curr_node->agentSeq[index + 1], (curr_node->actCnt - index) * sizeof(int));
			curr_node->actCnt--;
			DBG_ANA(("删除联动成功 当前[%s]联动条数[%d]\n", curr_node->key, curr_node->actCnt));
			
			//删除agentInfo表中记录lhy增加
			HashedAgentInfo::iterator agentIt;
			ACT_FORMAT act_format;
			act_format.iSeq = curr_node->agentSeq[index];
			agentIt = m_pAgentHashTable->find((int)act_format.iSeq);	
			if (agentIt != m_pAgentHashTable->end())
			{
				m_pAgentHashTable->erase(agentIt);
			}
			ret = true;
		}
	}	
	else
	{
		DBG_ANA(("删除联动结点[%s]未找到\n", hash_node.key));
	}
	
	DBG_ANA(("AnalyseHashAgentDelete ...........444444\n"));
	CTblLock.unlock();
	return ret;
}

/******************************************************************************/
/* Function Name  : AnalyseHashDeleteByDeviceAttr                                    */
/* Description    : 更新设备动作表节点                                        */
/* Input          : device_attr_id 设备编号					                      */
/*		          : standard 标准范围					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CAnalyser::AnalyseHashDeleteByDeviceAttr( char* device_attr_id)
{
	HashedInfo::iterator it;
	MSG_ANALYSE_HASH hash_node;
	memset((unsigned char*)&hash_node, 0, sizeof(MSG_ANALYSE_HASH));
	strncpy(hash_node.key, device_attr_id, DEVICE_ATTR_ID_LEN);
	CTblLock.lock();
	it = m_pActHashTable->find(hash_node.key);	
	if (it != m_pActHashTable->end())
	{
		m_pActHashTable->erase(it);
	}
	CTblLock.unlock();
	return true;
}


/******************************************************************************/
/* Function Name  : AnalyseHashDeleteByNet                                    */
/* Description    : 删除设备动作表节点                                        */
/* Input          : device_id 设备编号					                      */
/*		          : apptype 应用类型					                      */
/* Output         : none                                                      */
/* Return         : true   节点更新                                           */
/*                  false  节点增加                                           */
/******************************************************************************/
bool CAnalyser::AnalyseHashDeleteByNet( char* device_id)
{

	bool ret = false;
	HashedInfo::iterator it;
	PMSG_ANALYSE_HASH device_node;	
	char keys[256][16];
	memset(&keys[0][0], 0, 256*16);
	int i = 0;
	CTblLock.lock();

	for (it = m_pActHashTable->begin(); it != m_pActHashTable->end(); it++)
	{
		device_node = (PMSG_ANALYSE_HASH)(&it->second.key);

		if (0 != strncmp(device_node->key, device_id, DEVICE_ID_LEN))
		{
			continue;
		}
		strcpy(&(keys[i][0]), device_node->key);
		i++;
	}
	MSG_ANALYSE_HASH hashnode;
	for(int j=0; j<=i; j++)
	{
		memset((unsigned char*)&hashnode, 0, sizeof(DEVICE_INFO_POLL));
		strcpy(hashnode.key, &(keys[i][0]));
		it = m_pActHashTable->find(hashnode.key);	
		if (it != m_pActHashTable->end())
		{
			m_pActHashTable->erase(it);
		}
	}
	CTblLock.unlock();
	ret = true;
	return ret;
}

/******************************************************************************/
/* Function Name  : InsertATSTable                                            */
/* Description    : 插入到考勤记录表                                          */
/* Input          : int   vType          数值类型                             */
/*                  char* value          数据值                               */
/*                  char* standard       正常值范围                           */
/* Output         : none                                                      */
/* Return         : true       数据在正常范围内                               */
/*                  false      数据在正常范围外                               */
/******************************************************************************/
void CAnalyser::InsertATSTable(MSG_ANALYSE_HASH hash_node, char* szTableName, char* time, char* szValue , char* pUserId)
{
	char sqlData[512] = {0};
	char szDeviceId[DEVICE_ID_LEN_TEMP] = {0};
	char szAttrSn[DEVICE_SN_LEN_TEMP] = {0};

	memset(szDeviceId, 0, sizeof(szDeviceId));
	memset(szAttrSn, 0, sizeof(szAttrSn));

	memcpy(szDeviceId, hash_node.key, DEVICE_ID_LEN);
	memcpy(szAttrSn,   hash_node.key + DEVICE_ID_LEN, DEVICE_SN_LEN);

	strcat(sqlData, "null, ");
	strcat(sqlData, "'");
	strcat(sqlData, szDeviceId);        //设备编号
	strcat(sqlData, "','");
	strcat(sqlData, szAttrSn);
	strcat(sqlData, "', datetime('");
	strcat(sqlData, time);         //采集时间
	strcat(sqlData, "'), '");
	strcat(sqlData, szValue);        //采集数据
	strcat(sqlData, "', '");
	strcat(sqlData, pUserId);		//数据状态
	strcat(sqlData, "'");         //发送状态
	g_SqlCtl.insert_into_table( szTableName, sqlData, false);               //读取的数据插入数据库

}
/*******************************END OF FILE************************************/


