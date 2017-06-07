#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <unistd.h>
#include <locale.h>
#include <stdio.h>
#include "Init.h"
#include "SqlCtl.h"
#include "GSMCtrl.h"

#include "HardIF.h"

#define SMS_CONTENT_MAXLEN   (1024)
#define ONEDAY_SECONDS       (24*60*60)

#ifdef  DEBUG
#define DEBUG_GSMCTRL
#endif

#ifdef DEBUG_GSMCTRL
#define DBG_GSMCTRL(a)		printf a;
#else
#define DBG_GSMCTRL(a)	
#endif


CGSMCtrl::CGSMCtrl()
{
	m_SMSSendThrd = 0xFFFF;
	m_SeqCPM = 0;
}

CGSMCtrl::~CGSMCtrl(void)
{				
	if(0xFFFF != m_SMSSendThrd)
	{
		ThreadPool::Instance()->SetThreadMode(m_SMSSendThrd, false);
	}
}

/******************************************************************************/
// Function Name  : Initialize                                               
// Description    : 初始化Siemens GSM模块                                          
// Input          : none
// Output         : none                                                     
// Return         : true:成功;false:失败                                                     
/******************************************************************************/
bool CGSMCtrl::Initialize(TKU32 unHandleSend)
{   
    m_ptrActInfoTable = new HashedActInfo();

	//启动GSM模块状态检测线程
	char buf[256] = {0};
	sprintf(buf, "%s %s", "CGSMCtrl", "SMSCtrlThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CGSMCtrl::SMSCtrlThrd, true,this))
	{
		DBG_GSMCTRL(("CGSMCtrlThrd Create failed!\n"));
		return false;
	}
	m_unHandleGSM  = unHandleSend;
	return true;
}

/******************************************************************************/
//Function Name  : SMSSendThrd                                               
//Description    :                                         
//Input          : void* this
//Output         : none                                                     
//Return         : none                                                   
/******************************************************************************/
void * CGSMCtrl::SMSCtrlThrd(void *arg)
{
	CGSMCtrl *_this = (CGSMCtrl *)arg;
	_this->SMSCtrlThrdProc();
	return NULL;
}

/******************************************************************************/
//Function Name  : CheckThrdProc                                               
//Description    :                                        
//Input          : none
//Output         : none                                                     
//Return         : none                                                   
/******************************************************************************/
void CGSMCtrl::SMSCtrlThrdProc(void)
{ 
	GSM_ACTION_HASH hash_node;

	m_SMSSendThrd = pthread_self();

	printf("CGSMCtrl::GSMCtrlThrdProc Start ... Pid[%d]\n", getpid());
	QUEUEELEMENT pMsg = NULL;
	while(1)
	{
		pthread_testcancel();
        time_t tm_NowTime;
		

		//------动作待执行超时检查
		TKU32 seqBuf[256] = {0};
		int cnt = 0;
		HashedActInfo::iterator it;
		CTblLockGsmAction.lock();
		for (it = m_ptrActInfoTable->begin(); it != m_ptrActInfoTable->end(); it++)
		{
			time(&tm_NowTime);
			if( tm_NowTime - it->second.act_time >= 120000 )
			{
				seqBuf[cnt] = it->second.Seq;
				cnt++;
			}
		}
		CTblLockGsmAction.unlock();

		for( int i=0; i < cnt; i++ )
		{
			memset(&hash_node, 0, sizeof(hash_node));
			hash_node.Seq = seqBuf[i];

			//超时，只做删除
			if( !GetAndDeleteHashData(hash_node))
			{
				continue;
			}
		}

		//------队列信息处理
		do 
		{
			//先从GSM模块中取出待处理数据（发送结果 & 接收短信）
			
			GSM_MSG stGsmMsg;
			if(SYS_STATUS_SUCCESS == HardIF_GetGSMMsg(stGsmMsg))
			{
				//如果有，丢入处理队列
				if(!SendMsg_Ex(m_unHandleGSM, MUCB_DEV_GSM, (char*)(&stGsmMsg), sizeof(GSM_MSG)))
				{
					DBG_GSMCTRL(("DecodeRecvSMS SendMsg_Ex failed, handle[%d]\n", m_unHandleGSM));
				}

			}

			//GSM处理队列
			if( MMR_OK != g_MsgMgr.GetFirstMsg(m_unHandleGSM, pMsg, 100))
			{
                //未取到待发短信
                HardIF_PLALightOFF();
				break;
			}

			if (NULL == pMsg)
			{
				break;
			}

            HardIF_PLALightON();
			
			PGSM_MSG pGsmMsg = (PGSM_MSG)pMsg;
			switch(pGsmMsg->iMsgType)
			{
			case 0://从GSM接收
				{
					DBG(("GSM 接收到短信\n"));
					GsmRequestProc(&pGsmMsg->stSmsInfo);
				}
				break;
			case 1://发送到GSM,需要回应
				{
					DBG_GSMCTRL(("CPM 发送到GSM模块，需要回复执行状态 SrcId[%s] SrcSn[%s]\n", pGsmMsg->stAction.SrcId, pGsmMsg->stAction.AttrSn));		
					int actionRet = SYS_STATUS_FAILED;
					if( SYS_STATUS_SUCCESS != (actionRet = HardIF_DoGSMAction(pGsmMsg)))
					{
						ACTION_MSG stAction;
						memcpy((unsigned char*)&stAction, (unsigned char*)&pGsmMsg->stAction, sizeof(ACTION_MSG));
						CDevInfoContainer::SendToActionSource(stAction, actionRet);
					}
				}
				break;
			case 2://发送到GSM,不需要回应				
				HardIF_DoGSMAction(pGsmMsg);	//HardIF函数
				break;
			case 3://动作执行结果回复
				{
					DBG_GSMCTRL(("GSM接收到动作执行结果\n"));
					ActionResponseProc(&pGsmMsg->stActionRespMsg);
				}
				break;
			case 4:	//GSMBase发送结果（不处理）
				{
					ACTION_MSG stAction;
					memcpy((unsigned char*)&stAction, (unsigned char*)&pGsmMsg->stRespSMS, sizeof(ACTION_MSG));
					CDevInfoContainer::SendToActionSource(stAction, pGsmMsg->stRespSMS.Status);
				}
				break;
			default:
				DBG_GSMCTRL(("SMSCtrlThrdProc 未知数据\n"));
				break;
			}
		} while (false);
		if (NULL != pMsg)
		{
            MM_FREE(pMsg);
			pMsg = NULL;
		}		
		
		usleep( 1000 * 1000 );
	}

}

//数据插入短信发送队列
int CGSMCtrl::SendToSmsList(char* Tel, char* content)
{
	char newValue[1024] = {0};
	sprintf(newValue, "%s//%s", Tel, content);

	GSM_MSG stGsmMsg;
	memset((unsigned char*)&stGsmMsg, 0, sizeof(GSM_MSG));
	stGsmMsg.iMsgType = 2;//不需回应
	stGsmMsg.stAction.ActionSource = ACTION_SOURCE_GSM;
	strncpy(stGsmMsg.stAction.DstId, GSM_SMS_DEV, DEVICE_TYPE_LEN);
	memset(stGsmMsg.stAction.ActionValue, 0, sizeof(stGsmMsg.stAction.ActionValue));
	strcpy(stGsmMsg.stAction.ActionValue, newValue);
	HardIF_DoGSMAction(&stGsmMsg);
	return 0;
}	

TKU32 CGSMCtrl::GetSeq()
{
	if(0xffffffff == m_SeqCPM)
		m_SeqCPM = 0;
	return m_SeqCPM++;
}


/******************************************************************************/
//Function Name  : GsmRequestProc                                               
//Description    : GSM动作处理                                          
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
/*
		数据查询定义“1#设备编号”
		动作格式定义“2#设备编号#动作编号#动作值”，根据设备种类，动作值可为空
		快捷定义:
			“0”，列出权限范围内的设备，返回数据格式“设备编号,设备名称:动作属性编号,属性名称;...”
			“1”，全体权限范围内的设备布防。
			“2”，全体权限范围内的设备撤防。
*/
int CGSMCtrl::GsmRequestProc(SMS_INFO* pSms)
{   
	int ret = 3006;
	char* split_result = NULL;
	char* savePtr = NULL;

	do 
	{
		DBG_GSMCTRL(("Tel[%s] Content[%s] Index[%d]\n", pSms->tel, pSms->content, pSms->index));
		HardIF_DeleteSms(pSms->index);

		char tel[20] = {0};
		if ( '+' == pSms->tel[0])
		{
			memcpy(tel, &pSms->tel[3], strlen(pSms->tel) - 3);
		}
		else if('8' == pSms->tel[0] && '6' == pSms->tel[1])
		{
			memcpy(tel, &pSms->tel[2], strlen(pSms->tel) - 2);
		}
		else
			strcpy(tel, pSms->tel);

		//号码认证lhy
		int checkRet = CheckTel(tel);
		if (0 != checkRet)
		{
			ret = 9996;
			break;
		}

		//格式认证		
		char content[256] = {0};
		strcpy(content, pSms->content);
		if (8 <= strlen(content))//格式指令
		{
			char cmd[3] = {0};
			savePtr = NULL;
			split_result = strtok_r(content, "#", &savePtr);
			if (split_result != NULL)
			{
				strncpy(cmd, split_result, 2);
				switch(atoi(cmd))
				{
				case 1://配置产品到期lsd
					{
						char strPwd[7] = {0};
						split_result = strtok_r( NULL, "#", &savePtr);
						if (NULL == split_result)
							break;
						strncpy(strPwd, split_result, 6);
						//验证设备编号权限合法性
						if( 0 != CheckSmsPwd(strPwd))
						{
							DBG_GSMCTRL(("CheckSmsPwd Failed\n"));
							break;
						}

						char proDate[9] = {0};
						split_result = strtok_r( NULL, "#", &savePtr);
						if (NULL == split_result)
							break;
						strncpy(proDate, split_result, 8);
						//设置产品到期
						if( 0 != SetPro_Date(proDate))
						{
							DBG_GSMCTRL(("SetPro_Date Failed\n"));
							break;
						}
						//发回产品信息
						if( 0 != SendProInfoToSms(tel))
						{
							DBG_GSMCTRL(("SendProInfoToSms Failed\n"));
							break;
						}
					}
					break;
				case 2://配置产品控制lsd
					{
						char strPwd[7] = {0};
						split_result = strtok_r( NULL, "#", &savePtr);
						if (NULL == split_result)
							break;
						strncpy(strPwd, split_result, 6);
						//验证设备编号权限合法性
						if( 0 != CheckSmsPwd(strPwd))
						{
							DBG_GSMCTRL(("CheckSmsPwd Failed\n"));
							break;
						}

						char proTel[12] = {0};
						split_result = strtok_r( NULL, "#", &savePtr);
						if (NULL == split_result)
							break;
						strncpy(proTel, split_result, 11);
						//设置产品控制
						if( 0 != SetPro_Tel(proTel))
						{
							DBG_GSMCTRL(("SetPro_Tel Failed\n"));
							break;
						}
						//发回产品信息
						if( 0 != SendProInfoToSms(tel))
						{
							DBG_GSMCTRL(("SendProInfoToSms Failed\n"));
							break;
						}
					}
					break;
				default:
					//SendToSmsList(tel, "0:设备表,1:全布防,2:全撤防 [查询:1#设备号，动作:2#设备号#动作号#值]");
					return 9996;
				}
			}
		}
	} while (false);
	return ret;
}

/******************************************************************************/
//Function Name  : CheckSmsPwd                                               
//Description    : 验证指令密码                                          
//Input          : pwd
//Output         : None                                                     
//Return         : 0，-1                                                     
/******************************************************************************/
int CGSMCtrl::CheckSmsPwd(char* pwd)
{   
	int ret = -1;
	char sqlResult[7] = {0};
	char condition[32] = {0};
	char* split_result = NULL;
	char* savePtr = NULL;

	sprintf(condition, " where id = 'system'");
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"user_info", (char*)"pwd", condition, sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	ret = -1;
	DBG_GSMCTRL(("check[%s]\n", split_result));
	if( split_result != NULL )
	{
		if (0 == strcmp(split_result, pwd))
		{
			ret = 0;
		}
	}

	return ret;
}
/******************************************************************************/
//Function Name  : SetPro_Date                                               
//Description    : 设置产品到期                                         
//Input          : Date
//Output         : None                                                     
//Return         : 0，-1                                                     
/******************************************************************************/
int CGSMCtrl::SetPro_Date(char* proDate)
{   
	int ret = -1;
	char sqlData[128] = {0};
	char newDate[11] = {0};

	int year = 2013;
	int month = 1;
	int day = 1;
	if(0 == sscanf(proDate, "%04d%02d%02d", &year, &month, &day))
	{
		DBG(("SetPro_Date FormatErr!"));
		return ret;
	}
	sprintf(newDate, "%04d-%02d-%02d", year, month, day);
	//更新数据库设备状态
	strcat(sqlData, "PRO_DATE='");
	strcat(sqlData, newDate);
	strcat(sqlData, "', PRO_SIM='");
	strcat(sqlData, HardIF_GetGSMIMSI());
	strcat(sqlData, "'");
	if (g_SqlCtl.update_into_table("PRO_CTRL", sqlData, true))
	{
		ret = 0;
	}
	return ret;
}
int CGSMCtrl::SetPro_Tel(char* proTel)
{   
	int ret = -1;
	char sqlData[128] = {0};

	//更新数据库设备状态
	strcat(sqlData, "PRO_TEL='");
	strcat(sqlData, proTel);
	strcat(sqlData, "', PRO_SIM='");
	strcat(sqlData, HardIF_GetGSMIMSI());
	strcat(sqlData, "'");
	if (g_SqlCtl.update_into_table("PRO_CTRL", sqlData, true))
	{
		ret = 0;
	}
	return ret;
}

int CGSMCtrl::GsmCtrlProc( char* Tel, char* deviceId, char* actionId, char* actionValue)
{
	int ret = SYS_STATUS_FAILED;
	GSM_ACTION_HASH gsmAction;
	memset(&gsmAction, 0, sizeof(GSM_ACTION_HASH));
	time(&gsmAction.act_time);
	gsmAction.Seq = GetSeq();
	strcpy(gsmAction.Tel, Tel);
	strcpy(gsmAction.Value, actionValue);
	char actionSn[DEVICE_ACTION_SN_LEN_TEMP] = {0};
	memcpy(actionSn, actionId, DEVICE_ACTION_SN_LEN);
	if(addNode(gsmAction))//插入到待执行表
	{
		ACTION_MSG act_msg;
		memset((unsigned char*)&act_msg, 0, sizeof(ACTION_MSG));
		act_msg.ActionSource = ACTION_SOURCE_GSM;//lhy
		strcpy(act_msg.DstId, deviceId);
		strcpy(act_msg.ActionSn, actionSn);

		strcpy(act_msg.Operator, Tel);
		strcpy(act_msg.ActionValue, actionValue);
		act_msg.Seq = gsmAction.Seq;

		ret = CDevInfoContainer::DisPatchAction(act_msg);
		if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != ret &&  SYS_STATUS_SUBMIT_SUCCESS_IN_LIST != ret)
		{
			//从回应表中删除
			GetAndDeleteHashData(gsmAction);
		}
	}
	return ret;
}	
/******************************************************************************/
//Function Name  : SendDeviceValuesToSms                                               
//Description    : 验证设备编号合法性                                          
//Input          : Tel
//Output         : None                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::SendProInfoToSms(char*  Tel)
{   
	char szContent[1024] = {0};
	int ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"PRO_CTRL", (char*)"PRO_SEQ, PRO_DATE, PRO_SIM, PRO_TEL", (char*)"", szContent);
	if(0 != ret)
		return -1;
	SendToSmsList(Tel, szContent);
	return SYS_STATUS_SUCCESS;
}
/******************************************************************************/
//Function Name  : SendDeviceValuesToSms                                               
//Description    : 验证设备编号合法性                                          
//Input          : Tel
//Output         : None                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::SendProInfoToSmsMonthly()
{   
	int ret = -1;
	char sqlResult[32] = {0};
	char strTel[32] = {0};
	char* split_result = NULL;
	char* savePtr = NULL;

	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"PRO_CTRL", (char*)"IS_GSM, PRO_TEL", (char*)"", sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	if( split_result != NULL )
	{
		if (0 == atoi(split_result))
		{
			return 1;
		}
	}
	split_result = strtok_r(NULL, "#", &savePtr);
	if( split_result != NULL )
	{
		strcpy(strTel, split_result);
		printf("strTel[%s]\n",strTel);
	}
	//发回产品信息
	if( 0 != SendProInfoToSms(strTel))
	{
		DBG_GSMCTRL(("SendProInfoToSms Failed\n"));
		return -2;
	}
	return 0;
}

/******************************************************************************/
//Function Name  : SendDeviceValuesToSms                                               
//Description    : 验证设备编号合法性                                          
//Input          : Tel
//Output         : None                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::SendDeviceValuesToSms(char* Tel, char* deviceId)
{   
	DEVICE_DETAIL stDeviceInfo;
	memset((unsigned char*)&stDeviceInfo, 0, sizeof(DEVICE_DETAIL));
	if(SYS_STATUS_SUCCESS != g_DevInfoContainer.GetDeviceInfoNode(deviceId, stDeviceInfo))
		return SYS_STATUS_FAILED;

	char szContent[1512] = {0};
	char szShow[128] = {0};
	strcat(szContent, stDeviceInfo.szCName);
	strcat(szContent, " [");
	memcpy(szShow, stDeviceInfo.Show, sizeof(stDeviceInfo.Show));
	strcat(szContent, trim(szShow, strlen(szShow)));
	strcat(szContent, "] ");
	strcat(szContent, stDeviceInfo.ParaValues);
	SendToSmsList(Tel, szContent);
	return SYS_STATUS_SUCCESS;

	/*
	int ret = -1;
	char sqlResult[128] = {0};
	char condition[128] = {0};

	char* split_result = NULL;
	char* savePtr = NULL;

	char deviceName[20] = {0};
	char status[5] = {0};
	//获取设备名
	char* hjFormat = " where a.id = '%s'";
	sprintf(condition, hjFormat, deviceId);
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, "device_detail a", "a.cname, a.status", condition, sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	
	strcpy(deviceName, split_result);
	split_result = strtok_r(NULL, "#", &savePtr);
	if (NULL == split_result)
	{
		return -1;
	}
	switch(atoi(split_result))
	{
	case 0:
		strcpy(status, "撤防,");
		break;
	case 1:
		strcpy(status, "离线,");
		break;
	case 2:
		strcpy(status, "正常,");
		break;
	case 3:
		strcpy(status, "异常,");
		break;
	default:
		return -1;
	}
	
	SMS smsInfo;
	memset(&smsInfo, 0, sizeof(SMS));
	smsInfo.sn = 0;
	strcat(smsInfo.content, deviceName);
	strcat(smsInfo.content, status);
	strcat(smsInfo.tel, Tel);

	char* devValueFormat = "select c.attr_name, a.ctime, a.value, b.unit from data_now a, data_type b, device_attr c "
        "where a.id = '%s' and substr(a.id,1,4) = c.id and a.ctype = c.sn and b.id = c.attr_id "
        "group by a.id, a.ctype order by a.ctime desc;";
        //"select data_now.ctime, data_now.value, data_type.unit  from data_now, data_type where data_now.id = '%s' and data_now.ctype = data_type.id order by data_now.ctime;";
	char devValueSql[256] = {0};
	sprintf(devValueSql, devValueFormat, deviceId);
    g_SqlCtl.select_from_tableEx(devValueSql, CGSMCtrl::SelectCallBack_DevDataInfo, (void *)&smsInfo);

	if (strlen(smsInfo.content) > 0)
	{
		SendToSmsList(Tel, smsInfo.content);
	}

	return ret;
	*/
}


int CGSMCtrl::SelectCallBack_DevDataInfo(void *data, int n_column, char **column_value, char **column_name)    //查询时的回调函数
{
	SMS* smsBuf = (SMS*)data;

	char tempData[128] = {0};

	sprintf(tempData, "%s: %s,%s%s|",column_value[0],column_value[1],column_value[2],column_value[3]);

        DBG_GSMCTRL(("tempData[%s] content[%s] contentLen[%zu]\n", tempData, smsBuf->content, strlen(smsBuf->content)));
	if (strlen(smsBuf->content) + strlen(tempData) > SMS_CONTENT_MAXLEN)
	{
        DBG_GSMCTRL(("Tel[%s] content[%s]\n", smsBuf->tel, smsBuf->content));
		SendToSmsList(smsBuf->tel, smsBuf->content);
		memset(smsBuf->content, 0, sizeof(smsBuf->content));
		strcpy(smsBuf->content, tempData);
	}
	else
	{
		strcat(smsBuf->content, tempData);
	}

	return 0;
}

/******************************************************************************/
//Function Name  : GetRoleArea                                               
//Description    : 获取用户防区                                          
//Input          : Tel
//Output         : hj_role                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::GetRoleArea(const char*  Tel, char* hj_role)//lhy 查询所有
{   
	int ret = -1;
	char condition[256] = {0};

	//环境防区
	const char* hjFormat 
		= "where length(b.id) = 8 and substr(b.id, 1, 4) = a.ykt_role and a.tel = '%s' group by b.id";
	sprintf(condition, hjFormat, Tel);
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"user_info a, role b", (char*)"b.id", condition, hj_role);
	return ret;
}


/******************************************************************************/
//Function Name  : CheckTel                                               
//Description    : 验证手机合法性                                          
//Input          : Tel
//Output         : None                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::CheckTel(const char*  Tel)
{   
	int ret = -1;
	char sqlResult[56] = {0};
	char condition[128] = {0};
	char* split_result = NULL;
	char* savePtr = NULL;
	sprintf(condition, "where pro_tel = '%s'", Tel);

	//if (0 == strcmp(Tel, "18657190588")
	//	|| 0 == strcmp(Tel, "18668208766")
	//	|| 0 == strcmp(Tel, "15658881077"))
	//{
	//	return 0;
	//}
	DBG_GSMCTRL(("CheckTel.....\n"));
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"PRO_CTRL", (char*)"count(*)", condition, sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	ret = -1;
	DBG_GSMCTRL(("checke[%s]\n", split_result));
	if( split_result != NULL )
	{
		if (1 == atoi(split_result))
		{
			ret = 0;
		}
	}

	return ret;
}


/******************************************************************************/
//Function Name  : CheckDeviceAllow                                               
//Description    : 验证设备编号合法性                                          
//Input          : Tel
//Output         : None                                                     
//Return         : true，false                                                     
/******************************************************************************/
int CGSMCtrl::CheckDeviceAllow(const char*  Tel, char* deviceId)
{   
	int ret = -1;
	char sqlResult[56] = {0};
	char condition[512] = {0};
	char* split_result = NULL;
	char* savePtr = NULL;

	//select count(*) from user_info a, role b where length(b.id) = 6 and substr(b.id, 1, 4) = a.hj_role and a.tel = '13588200884' and b.point like '%030101%';
	//环境防区

	const char* hjFormat 
		= " where length(b.id) = 8 and substr(b.id, 1, 4) = a.ykt_role and a.tel = '%s' and b.point like '%%%s%%'";
	sprintf(condition, hjFormat, Tel, deviceId);
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"user_info a, role b", (char*)"count(*)", condition, sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	ret = -1;
	DBG_GSMCTRL(("check[%s]\n", split_result));
	if( split_result != NULL )
	{
		if (1 <= atoi(split_result))
		{
			ret = 0;
		}
	}

	return ret;
}

int CGSMCtrl::GetDevList(char*  roleid, char* deviceList)
{   
	int ret = -1;
	char condition[256] = {0};
	//
	const char* hjFormat = "where id like '%s%%' and length(id) = 8";
	sprintf(condition, hjFormat, roleid);
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"role", (char*)"point", condition, deviceList);
	return ret;
}


int CGSMCtrl::CheckActionId(char* tel, char* deviceId, char* actionSn)
{
	int ret = -1;
	char sqlResult[56] = {0};
	char condition[512] = {0};
	//char roleId[24] = {0};

	char* split_result = NULL;
	char* savePtr = NULL;

	char devType[DEVICE_TYPE_LEN_TEMP] = {0};

	memcpy(devType, deviceId, DEVICE_TYPE_LEN);

	//获取防区编号
	const char* roleFormat = "where tel = '%s'";
	sprintf(condition, roleFormat, tel);
	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"user_info", (char*)"ctrl_role", condition, sqlResult);
	if (0 != ret)
	{
		return -1;
	}
	split_result = strtok_r(sqlResult, "#", &savePtr);
	if (NULL == split_result)
	{
		return -1;
	}
	

	//环境防区

	memset(sqlResult, 0, sizeof(sqlResult));
	memset(condition, 0, sizeof(condition));
	const char* hjFormat = " where b.id like '%%%s%%' and b.point like '%%%s%%'";
	sprintf(condition, hjFormat, split_result, deviceId);

	ret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"user_info a, role b", (char*)"count(*)", condition, sqlResult);
	if(0 != ret)
		return -1;

	split_result = strtok_r(sqlResult, "#", &savePtr);
	ret = -1;
	if( split_result != NULL )
	{
		if (1 <= atoi(split_result))
		{
			ret = 0;
		}
	}

	return ret;
}
int CGSMCtrl::ActionResponseProc( PACTION_RESP_MSG pActionRespMsg)
{
    DBG_GSMCTRL(("ActionResponseProc--> seq:[%d] \n", pActionRespMsg->Seq));
	GSM_ACTION_HASH hashnode;
	memset(&hashnode, 0, sizeof(GSM_ACTION_HASH));
	hashnode.Seq = pActionRespMsg->Seq;

	if(!GetAndDeleteHashData(hashnode))
    {
        DBG_GSMCTRL(("Delete HashData Failed!\n"));
		return -1;
    }

	//将结果发回到用户手机
	char status[56] = {0};
	switch(pActionRespMsg->Status)
	{
	case SYS_STATUS_SUCCESS:
			strcpy(status, "操作成功");
			break;
		case SYS_STATUS_ILLEGAL_CHANNELID:	
			strcpy(status, "通道不合法");
			break;
		case SYS_STATUS_DEVICE_TYPE_NOT_SURPORT:
			strcpy(status, "该类设备不支持");
			break;
		case SYS_STATUS_DEVICE_NOT_EXIST:
			strcpy(status, "设备不存在");
			break;

		case SYS_STATUS_SUBMIT_SUCCESS:
			strcpy(status, "提交成功");
			break;
		case SYS_STATUS_FAILED:
			strcpy(status, "操作失败");
			break;		
		case SYS_STATUS_SEQUENCE_ERROR:
			strcpy(status, "流水号异常");
			break;
		case SYS_STATUS_FORMAT_ERROR:
			strcpy(status, "格式错误");
			break;
		case SYS_STATUS_ILLEGAL_REQUEST:
			strcpy(status, "非法请求");
			break;
		case SYS_STATUS_TIMEOUT:
			strcpy(status, "处理超时");
			break;
		case SYS_STATUS_SYS_BUSY:
			strcpy(status, "系统繁忙，请稍后重试");
			break;
		default:
			strcpy(status, "操作异常");
			break;
	}
	SendToSmsList(hashnode.Tel, status);
	return 0;
}	

//hashtables

bool CGSMCtrl::addNode(GSM_ACTION_HASH hash_node)
{
	bool ret = false;
	GSM_ACTION_HASH msg_node;
	HashedActInfo::iterator it;

	memset((BYTE*)&msg_node, 0, sizeof(msg_node));
	memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(GSM_ACTION_HASH));

	CTblLockGsmAction.lock();
	it = m_ptrActInfoTable->find(msg_node.Seq);
	if (it == m_ptrActInfoTable->end())
	{
		m_ptrActInfoTable->insert(HashedActInfo::value_type(hash_node.Seq,hash_node));
		ret = true;
	}
	CTblLockGsmAction.unlock();
	return ret;
}

bool CGSMCtrl::GetAndDeleteHashData(GSM_ACTION_HASH& hash_node)
{
	bool ret = false;
	HashedActInfo::iterator it;
	CTblLockGsmAction.lock();

	it = m_ptrActInfoTable->find(hash_node.Seq);
	if (it != m_ptrActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Seq, sizeof(GSM_ACTION_HASH));
		m_ptrActInfoTable->erase(it);
		ret = true;
	}
	CTblLockGsmAction.unlock();
	return ret;
}

bool CGSMCtrl::GetHashData(GSM_ACTION_HASH& hash_node)
{
	bool ret = false;
	HashedActInfo::iterator it;
	CTblLockGsmAction.lock();

	it = m_ptrActInfoTable->find(hash_node.Seq);
	if (it != m_ptrActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second.Seq, sizeof(GSM_ACTION_HASH));
		ret = true;
	}
	CTblLockGsmAction.unlock();
	return ret;
}

/*******************************End of file*********************************************/
