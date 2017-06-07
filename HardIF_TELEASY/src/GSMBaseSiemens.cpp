#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <unistd.h>
#include <locale.h>

#include "SqlCtl.h"
#include "CodeUtil.h"
#include "Util.h"
#include "EmApi.h"
#include "LightCtrl.h"
#include "GSMBaseSiemens.h"
#include "MemMgmt.h"
#include "MsgMgrBase.h"

using namespace std;

#ifdef  DEBUG
//#define DEBUG_GSM
#endif

//#define DEBUG_GSM
#ifdef DEBUG_GSM
#define DBG_GSM(a)		printf a;
#else
#define DBG_GSM(a)	
#endif

#define  SMS_CONTENT_MAXLEN      (256)
#define  SMS_CONTENT_SINGLE_LEN      (60)
#define  SMS_CONTENT_PDU_MAXLEN  (134)

INTERNET_SERVICE g_szNetService[10] =
{
	{GPRS_TCP_CLI,		"",		0},
	{GPRS_TCP_CLI,		"",		0},
	{GPRS_TCP_CLI,		"",		0},
	{GPRS_UDP_CLI,		"",		0},
	{GPRS_UDP_CLI,		"",		0},
	{GPRS_UDP_CLI,		"",		0},
	{GPRS_FTP_CLI,		"",		0},
	{GPRS_SMTP_CLI,		"",		0},
	{GPRS_POP3_CLI,		"",		0},
	{GPRS_HTTP_CLI,		"",		0}
};

//********字符串奇偶转换
void  TelParityChange(char* pSrc, char* pDst)
{
	char* tTemp = NULL;
	if (NULL != (tTemp = strstr(pSrc, "+")))
		tTemp = tTemp + 1;
	else
		tTemp = pSrc;

	strcpy(pDst, tTemp);
	if (0 != strlen(pDst)%2)
	{
		strcat(pDst, "F");
	}
	for (unsigned int i=0; i<strlen(pDst); i+=2)
	{
		char c = pDst[i+1];
		pDst[i+1] = pDst[i];
		pDst[i] = c;
	}
}

void  getTimeStamp(char* pDst)
{
    char strTime[30] = {0};
    time_t tm_NowTime;
	time(&tm_NowTime);
    Time2Chars(&tm_NowTime, strTime);

    pDst[0] = strTime[3];
    pDst[1] = strTime[2];
    pDst[2] = strTime[6];
    pDst[3] = strTime[5];
    pDst[4] = strTime[9];
    pDst[5] = strTime[8];

    pDst[6] = strTime[12];
    pDst[7] = strTime[11];
    pDst[8] = strTime[15];
    pDst[9] = strTime[14];
    pDst[10] = strTime[18];
    pDst[11] = strTime[17];

    pDst[12] = '2';
    pDst[13] = '3';
}


CGSMBaseSiemens::CGSMBaseSiemens()
{
	m_hFd = -1;
	
	m_enGSMStatus  = STA_CLOSED_SIEMENS;		//GSM模块状态
	m_enGPRS = STA_CLOSED_SIEMENS;
	m_enPhone = STA_LIVING_SIEMENS;

	m_hCheckThrdHandle =0xFFFF;
	m_ucSmsId = 0;
	m_nSmsCnt = 0;

	m_PidPlayAudio = -1;

	m_iWatchCnt = 0;
	memset(m_szGsmCenter, 0, sizeof(m_szGsmCenter));
	memset(m_strIMSI, 0, sizeof(m_strIMSI));
}

CGSMBaseSiemens::~CGSMBaseSiemens(void)
{			
	m_enGSMStatus  = STA_HALTED_SIEMENS;	
	
	if(0xFFFF != m_hCheckThrdHandle)
	{
		ThreadPool::Instance()->SetThreadMode(m_hCheckThrdHandle, false);
	}

	//关闭GSM模块串口
	if(0 <= m_hFd)
	{
		close(m_hFd);
		m_hFd = -1;
	}
}

/******************************************************************************/
// Function Name  : Initialize                                               
// Description    : 初始化Siemens GSM模块                                          
// Input          : none
// Output         : none                                                     
// Return         : true:成功;false:失败                                                     
/******************************************************************************/
bool CGSMBaseSiemens::Initialize(char* comId, int baudrate, char* szSmsCenter, int ulHandleGSMSend, int ulHandleGSMRecv)
{   
	m_ulHandleGSMSend = ulHandleGSMSend;
	m_ulHandleGSMRecv = ulHandleGSMRecv;
	memset(m_strComId, 0 , sizeof(m_strComId));
	strcpy(m_strComId, comId);
	m_iBaudRate = baudrate;
	strcpy(m_szGsmCenter, szSmsCenter);

	//声音处理模块
	if (false == m_AudioPlay.Initialize((char*)"/dev/audio"))return false;

	if (!OpenCom())
	{
		DBG_GSM(("Open Siemens dev[%s] failed\n", comId));
		return false;
	}
//	m_enGSM  = STA_CLOSED_SIEMENS;	
	//启动GSM模块状态检测线程
	char buf[256] = {0};	
	sprintf(buf, "%s %s", "CGSMBaseSiemens", "GSMCheckThrd");
	if (!ThreadPool::Instance()->CreateThread(buf, &CGSMBaseSiemens::CheckThrd, true,this))
	{
		DBG_GSM(("pCommThrdCtrl Create failed!\n"));
		return false;
	}

	DBG_GSM(("GSM Initialize success\n"));
	return true;
}

/******************************************************************************/
//Function Name  : CheckThrd                                               
//Description    : GSM模块状态检测线程                                          
//Input          : void* this
//Output         : none                                                     
//Return         : none                                                   
/******************************************************************************/
void * CGSMBaseSiemens::CheckThrd(void *arg)
{
	CGSMBaseSiemens *_this = (CGSMBaseSiemens *)arg;
	_this->CheckThrdProc();
	return NULL;
}

/******************************************************************************/
//Function Name  : CheckThrdProc                                               
//Description    : GSM模块状态检测线程执行体                                          
//Input          : none
//Output         : none                                                     
//Return         : none                                                   
/******************************************************************************/
void CGSMBaseSiemens::CheckThrdProc(void)
{
	m_hCheckThrdHandle = pthread_self();
	time(&m_tmActivTimer);
	time(&m_tmRecvSms);
	time_t tmNow;

	printf("CGSMBaseSiemens::CheckThrdProc Start .... Pid[%d]\n", getpid());
	while(1)
	{
//		DBG_GSM(("GSM check thrd\n"));
		pthread_testcancel();
		if (m_iWatchCnt > 20)
		{
			CTblLockGSMBase.lock();
			//模块断电，再上电

			CLightCtrl::GSMLightOFF();
			sleep(5);
			CLightCtrl::GSMLightON();
			sleep(10);

			m_iWatchCnt = 0;
			m_enGSMStatus = STA_CLOSED_SIEMENS;
			m_enPhone = STA_LIVING_SIEMENS;
			CTblLockGSMBase.unlock();
		}
		
		do //处理待发送
		{
			QUEUEELEMENT pMsg = NULL;
			time(&tmNow);
			//待发送
			if( MMR_OK != g_MsgMgr.GetFirstMsg(m_ulHandleGSMSend, pMsg, 100))
			{
				//未取到待发短信
				CLightCtrl::PLALightOFF();
				break;
			}
			
			if (NULL == pMsg)
			{
				break;
			}
			CLightCtrl::PLALightON();
			
			int actionRet = SYS_STATUS_FAILED;
			SMS_INFO smsInfo;
			memset((unsigned char*)&smsInfo, 0, sizeof(SMS_INFO));
			PGSM_MSG pGsmMsg = (PGSM_MSG)pMsg;
			if (SYS_STATUS_SUCCESS == (actionRet = GetSmsInfoByActionMsg(pGsmMsg->stAction, smsInfo))
				&& SYS_STATUS_GSM_BUSY == (actionRet = GsmActionProc(&smsInfo)))
			{
				//插回到队列
				GSM_MSG gsmMsg;
				memcpy(&gsmMsg, pGsmMsg, sizeof(GSM_MSG));
				if(!SendMsg_Ex(m_ulHandleGSMSend, MUCB_DEV_SMS_SEND, (char*)&gsmMsg, sizeof(GSM_MSG)))
				{
					DBG(("SmsInfo SendMsg_Ex failed, handle[%d]\n", m_ulHandleGSMSend));
				}
				else
				{
					DBG(("插回到对列\n"));
				}
			}
			else
			{
				if(1 == pGsmMsg->iMsgType)//需要回复
				{
					GSM_MSG gsmMsg;
					memcpy(&gsmMsg, pGsmMsg, sizeof(GSM_MSG));
					gsmMsg.stRespSMS.Status = actionRet;
					gsmMsg.iMsgType = 4;
					if(!SendMsg_Ex(m_ulHandleGSMRecv, MUCB_DEV_SMS_RECV, (char*)&gsmMsg, sizeof(GSM_MSG)))
					{
						DBG(("SmsInfo SendMsg_Ex failed, handle[%d]\n", m_ulHandleGSMRecv));
					}
					else
					{
						DBG(("插回到队列\n"));
					}
				}
			}
			if (NULL != pMsg)
			{
				MM_FREE(pMsg);
				pMsg = NULL;
			}	
		} while (false);

		switch(m_enGSMStatus)
		{
		case STA_LIVING_SIEMENS:
			{
				time(&tmNow);
				//接收短信
				if (SMS_RECV_INTERVAL < tmNow - m_tmRecvSms)
				{
					RecvSms(m_nSmsType);
					m_tmRecvSms = tmNow;
					m_nSmsType = 0;
				}

				CTblLockGSMBase.lock();
				unsigned char buf[128] = {0};
				int nRcvLen = 0;
				if (0 < (nRcvLen = ComRecv(buf, 128, 1, 500, true)))
				{
					if (NULL != strstr((char*)buf, "RING"))//有电话进来
					{
						char sendbuf[56] = {0};
						strcpy(sendbuf, "ATH\r");//直接挂断
						ComSend((unsigned char*)sendbuf, strlen(sendbuf));
					}
					else if(NULL != strstr((char*)buf, "NOT READY"))//卡不在
					{
						m_iWatchCnt = 51;
					}
				}
				CTblLockGSMBase.unlock();
			}
			break;
		case STA_CLOSED_SIEMENS:
			CTblLockGSMBase.lock();
			if (InitGSM())
			{
				m_enGSMStatus = STA_LIVING_SIEMENS;
				m_nSmsType = 4;
			}
			else
			{
				DBG_GSM(("InitGSM Failed![%s] \n", m_strIMSI));
			}
			CTblLockGSMBase.unlock();
			break;
			/*
		case STA_CLOSED_SIEMENS:
			CTblLockGSMBase.lock();
			DBG_GSM(("CheckThrdProc opencomm\n"));
			if(OpenCom())
				m_enGSM = STA_LIVING_SIEMENS;
			m_nSmsType = 4;
			CTblLockGSMBase.unlock();
			break;
			*/
		case STA_HALTED_SIEMENS:
			break;
		}		
		usleep( 1000 * 500 );
	}
}

int CGSMBaseSiemens::GetSmsInfoByActionMsg(ACTION_MSG stAction, SMS_INFO& stSms)
{

	char telNumber[256] = {0};
	char sendValue[1025] = {0};
	char tempValue[1025] = {0};

	char* t_posi = strstr(stAction.ActionValue, "[V]");

	if( t_posi != NULL )
	{
		char temp[1024] = {0};
		strcpy(temp, stAction.ActionValue);
		t_posi = strstr(temp, "[V]");
		memcpy(tempValue, temp, t_posi - temp);
		strcat(tempValue, stAction.SrcValue);
		strcat(tempValue, t_posi+3);
	}
	else
	{
		strcpy(tempValue, stAction.ActionValue);
	}
	printf("tempValue[%s] \n", tempValue);
	char* pContent = strstr(tempValue, "//");
	if (NULL == pContent)
	{
		DBG(("GetSmsInfoByActionMsg SYS_STATUS_FORMAT_ERROR\n"));
		return SYS_STATUS_FORMAT_ERROR;//格式错误
	}

	strcpy(sendValue, pContent + 2);
	pContent[0] = '\0';
	strncpy(telNumber, tempValue, pContent - tempValue);

	PSMS_INFO pSms_info = &stSms;

	DBG(("Tel[%s] Content[%s] DstId[%s]\n",telNumber, sendValue, stAction.DstId));
	pSms_info->sn = 0;
	if(strncmp(stAction.DstId, GSM_SMS_DEV, DEVICE_TYPE_LEN) == 0)//lhy 
	{
		pSms_info->ctype = 1;
		strcpy( pSms_info->content, sendValue );
	}
	else if (strncmp(stAction.DstId, GSM_CALL_DEV, DEVICE_TYPE_LEN) == 0)//lhy
	{
		pSms_info->ctype = 2;
		//电话
		char sqlCondition[100] = {0};
		char sqlResult[100] = {0};
		char* split_result = NULL;
		char* savePtr = NULL;
		memset(sqlCondition, 0, sizeof(sqlCondition));
		memset(sqlResult, 0, sizeof(sqlResult));
		split_result = NULL;
		savePtr = NULL;
		strcat(sqlCondition, "where sn = '");
		strcat(sqlCondition, sendValue);
		strcat(sqlCondition, "'");

		int dbret = g_SqlCtl.select_from_table(DB_TODAY_PATH, (char*)"call", (char*)"route", sqlCondition, sqlResult);
		if(0 == dbret)
		{
			split_result = strtok_r(sqlResult, "#", &savePtr);
			
			if( split_result != NULL )
			{
				strcpy(pSms_info->content, split_result);
			}
		}
	}
	strncpy( pSms_info->srcDev, stAction.SrcId, DEVICE_ID_LEN);
	strncpy( pSms_info->srcAttr, stAction.AttrSn, DEVICE_SN_LEN);
	strncpy( pSms_info->dstDev, stAction.DstId, DEVICE_ID_LEN);
	strncpy( pSms_info->dstAction, stAction.ActionSn, DEVICE_ACTION_SN_LEN);
	strcpy( pSms_info->seprator, stAction.Operator );
	strcpy( pSms_info->tel, telNumber );
	/*
	time_t nowTime = 0;
	time(&nowTime);
	Time2Chars(&nowTime, pSms_info->szTimeStamp);
	*/
	memcpy( pSms_info->szTimeStamp, stAction.szActionTimeStamp, 20 );//lhy

	DBG(("GetSmsInfoByActionMsg SYS_STATUS_SUCCESS\n"));
	return SYS_STATUS_SUCCESS;
}

/******************************************************************************/
//Function Name  : SetGsmSend                                               
//Description    : 插入发送队列                                          
//Input          : none
//Output         : none                                                     
//Return         : none                                             
/******************************************************************************/
int CGSMBaseSiemens::SetGsmSend(PGSM_MSG pGsmMsg)
{
	if(!SendMsg_Ex(m_ulHandleGSMSend, MUCB_DEV_SMS_SEND, (char*)(pGsmMsg), sizeof(GSM_MSG)))
	{
		DBG_GSM(("SetGsmSend SendMsg_Ex failed, handle[%d]\n", m_ulHandleGSMSend));
		return SYS_STATUS_GSM_SENDLIST_EXCEPTION;
	}
	DBG_GSM(("SetGsmSend SendMsg_Ex Success, handle[%d]\n", m_ulHandleGSMSend));
	return SYS_STATUS_SUCCESS;
}
/******************************************************************************/
//Function Name  : GetGsmRecv                                               
//Description    : 插入发送队列                                          
//Input          : none
//Output         : none                                                     
//Return         : GSM_MSG
/******************************************************************************/
int CGSMBaseSiemens::GetGsmRecv(GSM_MSG& outMsg)
{
	int ret = SYS_STATUS_FAILED;
	QUEUEELEMENT pMsg = NULL;
	if( MMR_OK != g_MsgMgr.GetFirstMsg(m_ulHandleGSMRecv, pMsg, 100))
	{
		//CLightCtrl::GSMLightOFF();
	}
	else
	{
		memcpy((unsigned char*)&outMsg, (unsigned char*)pMsg, sizeof(GSM_MSG));
		ret = SYS_STATUS_SUCCESS;
		MM_FREE(pMsg);
		pMsg = NULL;
	}
	return ret;
}
/******************************************************************************/
//Function Name  : GsmActionProc                                               
//Description    : GSM动作处理                                          
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
int CGSMBaseSiemens::GsmActionProc(SMS_INFO* pSms)
{   
	int ret = SYS_STATUS_FAILED;
	time_t addTime = 0;
	time_t nowTime = 0;
	
	switch(pSms->ctype)
	{
	case 2://语音
		ret = PhoneCall(pSms->tel, pSms->content);
		break;
	case 1://短信
		ret = SendSms(pSms->tel, pSms->content, pSms->sn, pSms->index);
		break;
	default:
		return ret;
	}
	printf("GsmActionProc -------------ret[%d]-----: tel[%s] content[%s] sn[%d] type[%d]\n", ret, pSms->tel, pSms->content, pSms->sn, pSms->ctype);
	
	switch(ret)
	{
	case SYS_STATUS_GSM_BUSY:
		Str2time( pSms->szTimeStamp, &addTime );
		time(&nowTime);
		DBG(("GsmActionProc nowTime[%ld] strTime[%s] addTime[%ld] TimeElipse[%ld]\n", nowTime , pSms->szTimeStamp, addTime, nowTime - addTime));
		if( nowTime - addTime <= 300 )    //五分钟之内
			ret = SYS_STATUS_GSM_BUSY;
		else
			ret = SYS_STATUS_TIMEOUT;
		break;
	}
	return ret;
}

/******************************************************************************/
//Function Name  : InitGSM                                               
//Description    : 初始化GSM模块                                          
//Input          : none
//Output         : none                                                     
//Return         :             
/******************************************************************************/
bool CGSMBaseSiemens::InitGSM()
{   
	bool ret = false;
	int nLen = 0;

	memset(m_strIMSI, 0, sizeof(m_strIMSI));

	//断电lhy
	//通电

	/*
	SMS初始化 
	1.查询字符编码，“UCS2”
	2.查询短信中心号，如果没有则进行设置
	3.设置CNMI，配置接收短信通知，送达报告等 
	4.设置SMGO，存储满报告
	5.设置CMGF，设置PDU的字符模式为0
	6.
	*/

	//查询SMS支持格式
	if (!(ret = DoCmd("AT+CMGF=?\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}
	//查询SMS当前CNMI
	if (!(ret = DoCmd("AT+CNMI?\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}
	//回显设置，需要回显
	if (!(ret = DoCmd("ATE0\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}
	
	//短消息发送格式，1-TEXT，0-PDU
	if (!(ret = DoCmd("AT+CMGF=0\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//获取短信中心码
	char szTempBuf[128] = {0};
	strcpy(szTempBuf, m_szGsmCenter);
	/*
	if(!(ret = QueryAddr( szTempBuf)))
	{
		//设置默认短信中心码
		return ret;
	}
	*/
	char szTempOut[128] = {0};

	TelParityChange(szTempBuf, szTempOut);//奇偶字节转换

	m_strCenterAddr = "91";//添加国际化码“91”
	m_strCenterAddr += szTempOut;
	
	//生成长度
	char szLen[5] = {0};
	nLen = m_strCenterAddr.size()/2;
	sprintf(szLen, "%02X", nLen);
	m_strCenterAddr = szLen + m_strCenterAddr;
	
	//通知方式:[<mode>[,<mt>[,<bm>[,<ds>[,<bfr>]]]]] 
	if (!(ret = DoCmd("AT+CNMI=2,1,0,0,1\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//CREG
	if (!(ret = DoCmd("AT+CREG=1\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//COPS
	if (!(ret = DoCmd("AT+COPS?\r", (char*)"+COPS", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	char szIMSI[16] = {0};
//记录IMSI
	if (!(ret = DoCmd("AT+CIMI\r", NULL, szIMSI, nLen, 0, 500)))
	{
		printf("AT+CIMI Fail\n");
		return ret;
	}
	sscanf(szIMSI, "%*[^0-9]%[0-9]%*s", m_strIMSI);
	printf("InitGSM Success![%s] \n", m_strIMSI);
/*
	//初始化GPRS
	if(!InitGPRS())
		return ret;
*/	
	ret = true;
	return (ret);
}

/************************************************************************/
/* GPRS                                                                 */
/************************************************************************/
/************************************************************************/
//Function Name  : InitGPRS                                               
//Description    : 初始化GPRS通信                                          
//Input          : none
//Output         : none                                                     
//Return         : true/false            
/************************************************************************/
bool CGSMBaseSiemens::InitGPRS()
{   
	bool ret = false;
	int nLen = 0;

	//初始化连接平台

	//网络注册状态
	if (!(ret = DoCmd("at+cgreg?\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//GPRS attach/detach
	if (!(ret = DoCmd("at+cgatt?\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}
	//指定GPRS连接方式
	if (!(ret = DoCmd("at^sics=0,conType,none\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//指定GPRS连接方式
	if (!(ret = DoCmd("at^sics=0,conType,GPRS0\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//设置用户名
	if (!(ret = DoCmd("at^sics=0,user,gprs\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//设置密码
	if (!(ret = DoCmd("at^sics=0,user,gprs\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	//设置APN
	if (!(ret = DoCmd("at^sics=0,apn,CMNET\r", (char*)"OK", NULL, nLen, 0, 500)))
	{
		return ret;
	}

	m_enGPRS = STA_LIVING_SIEMENS;
	ret = true;
	DBG_GSM(("GPRS init success...\n"));
	return (ret);
}

int CGSMBaseSiemens::OpenSocket(GPRS_INTERNET_SERVICE_TYPE connectType, char* url)
{
	int ret = -1;
	int serviceId = -1;

	for (unsigned int i=0; i < sizeof(g_szNetService); i++)
	{
		if (0 == g_szNetService[i].status && connectType == g_szNetService[i].ServiceType)
		{
			serviceId = i;
			memset(g_szNetService[i].url, 0, sizeof(g_szNetService[i].url));
			strcpy(g_szNetService[i].url, url);
			break;
		}
	}

	DBG_GSM(("Open Socket 333\n"));
	if (-1 == serviceId)
	{
		return serviceId;
	}
	DBG_GSM(("Open Socket 444\n"));
	//打开服务
	CTblLockGSMBase.lock();
	if (STA_LIVING_SIEMENS != m_enGPRS || STA_LIVING_SIEMENS != m_enGSMStatus)
	{
		CTblLockGSMBase.unlock();
		return ret;
	}
	if(true == (ret = OpenNetService(g_szNetService[serviceId], serviceId)))
		g_szNetService[serviceId].status = 1;
	CTblLockGSMBase.unlock();
	return serviceId;
}

bool CGSMBaseSiemens::OpenNetService(INTERNET_SERVICE netService, int svrId)
{
	int nLen = 0;
	char tempBuf[128] = {0};
	CloseNetService(svrId + 1);
	switch(netService.ServiceType)
	{
	case GPRS_TCP_CLI:
	case GPRS_UDP_CLI:
		sprintf(tempBuf, "at^siss=%d,srvType,socket\r", svrId + 1);
		break;
	default:
		return false;
	}
	if (!DoCmd(tempBuf, (char*)"OK", NULL, nLen, 0, 500))
	{
		return false;
	}

	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "at^siss=%d,conID,0\r",svrId + 1);
	if (!DoCmd(tempBuf, (char*)"OK", NULL, nLen, 0, 500))
	{
		return false;
	}

	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "at^siss=%d,address,\"%s\"\r",svrId + 1, netService.url);
	if (!DoCmd(tempBuf, (char*)"OK", NULL, nLen, 0, 500))
	{
		return false;
	}

	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "at^siso=%d\r",svrId + 1);
	for (int i=0; i<5; i++)
	{
		if (DoCmd(tempBuf, (char*)"OK", NULL, nLen, 3, 0))
		{
			return true;
		}
	}
	
	return false;
}

void CGSMBaseSiemens::CloseSocket(int handle)
{
	CTblLockGSMBase.lock();
	if (STA_LIVING_SIEMENS != m_enGPRS || STA_LIVING_SIEMENS != m_enGSMStatus)
	{
		CTblLockGSMBase.unlock();
		return;
	}
	CloseNetService(handle);
	CTblLockGSMBase.unlock();
}

bool CGSMBaseSiemens::CloseNetService(int svrId)
{
	int nLen = 0;
	char tempBuf[128] = {0};
	sprintf(tempBuf, "at^sisc=%d\r",svrId);

	g_szNetService[svrId].status = 0;
	if (!DoCmd(tempBuf, (char*)"OK", NULL, nLen, 0, 500))
	{
		return false;
	}
	return true;
}

int CGSMBaseSiemens::SendGPRS(int handle, unsigned char* buf, int len)
{
	int ret = -1;	
	if (handle >= 0 && handle < (int)sizeof(g_szNetService) && 1 == g_szNetService[handle].status)
	{
		ret = 0;
		CTblLockGSMBase.lock();
		if (STA_LIVING_SIEMENS != m_enGPRS || STA_LIVING_SIEMENS != m_enGSMStatus)
		{
			CTblLockGSMBase.unlock();
			return ret;
		}
		ret = DoSendGPRS( handle, buf, len);
		CTblLockGSMBase.unlock();
	}
	return ret;
}

int CGSMBaseSiemens::DoSendGPRS(int handle, unsigned char* buf, int len)
{
	char tempBuf[128] = {0};
	char sendBuf[MAX_MSG_LEN_GPRS] = {0};
	int nLen = len;
	if (MAX_MSG_LEN_GPRS < len)
	{
		nLen = MAX_MSG_LEN_GPRS - 2;
	}

	sprintf(tempBuf, "AT^SISW=%d,%d\r",handle, nLen);
	if (!DoCmd(tempBuf, (char*)"^SISW:", NULL, nLen, 0, 500))
	{
		return -1;
	}	

	memcpy(sendBuf, buf, nLen);
	sendBuf[nLen] = 0x0d;
	sendBuf[nLen + 1] = 0x0a;
	bool sendRet = ComSend(buf, nLen + 2);
	if (!sendRet)
	{
		return -1;
	}
	char respBuf[128] = {0};
	ComRecv((unsigned char*)respBuf, 128, 0, 500, false);
	if (NULL == strstr(respBuf, "OK"))
	{
		return -1;
	}
	return nLen;	
}
int CGSMBaseSiemens::RecvGPRS(int handle, unsigned char* buf, int len)
{
	int ret = -1;
	
	if (handle >= 0 && handle < (int)sizeof(g_szNetService) && 1 == g_szNetService[handle].status)
	{
		ret = 0;
		CTblLockGSMBase.lock();
		if (STA_LIVING_SIEMENS != m_enGPRS || STA_LIVING_SIEMENS != m_enGSMStatus)
		{
			CTblLockGSMBase.unlock();
			return ret;
		}
		ret = DoSendGPRS( handle, buf, len);
		CTblLockGSMBase.unlock();
	}
	return ret;
}

int CGSMBaseSiemens::DoRecvGPRS(int handle, unsigned char* buf, int len)
{	
	char tempBuf[128] = {0};
	char respBuf[MAX_MSG_LEN_GPRS] = {0};
	int nLen = len;
	if (MAX_MSG_LEN_GPRS < len)
	{
		nLen = MAX_MSG_LEN_GPRS - 2;
	}

	sprintf(tempBuf, "AT^SISR=%d,%d\r",handle, nLen);
	if (!DoCmd(tempBuf, (char*)"^SISR:", respBuf, nLen, 0, 500))
	{
		return -1;
	}	

	char* ptrSplite = strstr(respBuf, ",");
    char* ptrBuf = strstr(respBuf, "\r\n");	
	if (NULL == ptrSplite || NULL == ptrBuf)
	{
		return -1;
	}
	nLen = ptrBuf - ptrSplite - 1;
	memset(tempBuf, 0, sizeof(tempBuf));
	memcpy(tempBuf, ptrBuf + 1, nLen);
	len = atoi(tempBuf);
	memcpy(buf, ptrBuf + 2, len);
	return len;
}
/************************************************************************/
/* SMS                                                                  */
/************************************************************************/
/******************************************************************************/
//Function Name  : SendSms                                               
//Description    : 发送短信                                     
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
int CGSMBaseSiemens::SendSms(char* tel, char* content, int sn, int& seq)
{
	int ret = SYS_STATUS_GSM_BUSY;                 //设备忙
    char smsTel[20] = {0};
	//bool ret = false;
	
	if ('+' != tel[0])
	{
		strcat(smsTel, "+86");
	}
	
    strcat(smsTel, tel);
	CTblLockGSMBase.lock();
	if ( STA_LIVING_SIEMENS != m_enGSMStatus  || STA_LIVING_SIEMENS != m_enPhone)
	{
		CTblLockGSMBase.unlock();
		return ret;
	}
	ret = SendSmsPDU2(smsTel, content, seq);
	/*
	int len = 0;
	while(len < strlen(content))
	{
		if ((strlen(content) - len) > SMS_CONTENT_SINGLE_LEN)
		{
			memset(temp, 0,sizeof(temp));
			strncpy(temp, content + len, SMS_CONTENT_SINGLE_LEN);
			ret = SendSmsPDU(smsTel, temp, seq);
			len += SMS_CONTENT_SINGLE_LEN;
		}
		else
		{
			ret = SendSmsPDU(smsTel, content + len, seq);
			len = strlen(content);
			break;
		}
	}
	*/
	CTblLockGSMBase.unlock();
	return ret;
}

/******************************************************************************/
//Function Name  : SendSmsPDU                                               
//Description    : 以PDU方式发送短信                                         
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
int CGSMBaseSiemens::SendSmsPDU2(char* tel, char* content, int& seq)
{ 
	char szATMsg[1500] = {0};
	char szATBuf[1500] = {0};
	char cZ = (char)0x1a;
	string strTel;
	int nLen = 0;
	string strContent = "";
	char szTel[20] = {0};
    char szTime[14] = {0};
	
	DBG_GSM(("\n------------------------------GSM:发送短信--------------------------------\n"));
	TelParityChange(tel, szTel);
    getTimeStamp(szTime);

	//编码转换
	nLen = GB2UniCode(content, (unsigned char*)szATBuf);
	
	for(int i=0;i<nLen;i++ ) 
	{
		char tempBuf[3] = {0};
		sprintf(tempBuf, "%02X",(unsigned char)szATBuf[i]);
		strContent += tempBuf;
	}

	char szLen[5] = {0};
	nLen = strContent.size()/2;

    if( nLen <= 70)//非长短信，直接发送
    {
	    sprintf(szLen, "%02x", nLen);
	    strContent = szLen + strContent;

	    strTel = "11000D91";
	    strTel += szTel;
	    strTel += "000800";
	    strTel += strContent;

	    nLen = strTel.size()/2;
	    memset(szATMsg, 0, sizeof(szATMsg));
	    sprintf(szATMsg, "AT+CMGS=%d\r", nLen);//lhy 短信收发格式确认
	    if (!DoCmd(szATMsg, (char*)">", NULL, nLen, 0, 500))
        {
		    return SYS_STATUS_FAILED;
        }

	    strTel = m_strCenterAddr + strTel;
	    strTel += cZ;
	    char respBuf[1500] = {0};
	    nLen = sizeof(respBuf);
	    if (!DoCmd(strTel.c_str(), (char*)"OK", respBuf, nLen, 10, 500))
		{
			DBG(("Send Msg error 111!\n"));
		    return SYS_STATUS_FAILED;
        }
    }
    else                                 //长短信，需分拆
    {
		int iSmsLenth = nLen/66;
		if (0 != nLen%66)
		{
			iSmsLenth++;
		}
		DBG_GSM(("SMS strlen[%d] smsCount[%d]\n" ,nLen, iSmsLenth));
		
		unsigned char smsId = GetSmsId();
		for (int i = 1; i <= iSmsLenth; i++) 
		{ 
			string len; 
			char msg[512] = {0};
			string strSendMsg = "";
			char szTemp[3] = {0};
			int msgcount = 132;
			if (i == iSmsLenth)
			{
				strcpy(msg, strContent.c_str() + (i-1)*msgcount);
			}
			else
			{
				memcpy(msg, strContent.c_str() + (i-1)*msgcount, msgcount);
			}
			
			strSendMsg += m_strCenterAddr;
			strSendMsg += "51000D91";
			strSendMsg += szTel;
			strSendMsg += "000800";

			sprintf(szTemp, "%02x", (TKU32)strlen(msg)/2 + 6);
			strSendMsg += szTemp;

			
			strSendMsg += "050003";

			sprintf(szTemp, "%02x", smsId);
			strSendMsg += szTemp;

			sprintf(szTemp, "%02x", iSmsLenth);
			strSendMsg += szTemp;

			sprintf(szTemp, "%02x", i);
			strSendMsg += szTemp;

			strSendMsg += msg;

			nLen = strSendMsg.size()/2 - m_strCenterAddr.size()/2;

			memset(szATMsg, 0, sizeof(szATMsg));
			sprintf(szATMsg, "AT+CMGS=%d\r", nLen);
			if (!DoCmd(szATMsg, (char*)">", NULL, nLen, 10, 500))
			{
				return SYS_STATUS_FAILED;
			}	
			strSendMsg += cZ;
			strSendMsg += '\r';
			//DBG_GSM(("strSendMsg[%s]\n", strSendMsg.c_str()));
			char respBuf[1500] = {0};
			nLen = sizeof(respBuf);
			if (!DoCmd(strSendMsg.c_str(), (char*)"OK", respBuf, nLen, 10, 500))
			{
				DBG(("Send Msg error 222!\n"));
				return SYS_STATUS_FAILED;
			}
		}
		
    }
	//lhy 截取序号

	return SYS_STATUS_SUCCESS;
}

char* CGSMBaseSiemens::GetIMSI()
{
	return m_strIMSI;
}

unsigned char CGSMBaseSiemens::GetSmsId()
{
	if (0xff == m_ucSmsId++)
	{
		m_ucSmsId = 0x01;
	}
	return m_ucSmsId;
}
/******************************************************************************/
//Function Name  : SendSmsPDU                                               
//Description    : 以PDU方式发送短信                                         
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
int CGSMBaseSiemens::SendSmsPDU(char* tel, char* content, int& seq)
{ 
	char szATMsg[1500] = {0};
	char szATBuf[1500] = {0};
	char cZ = (char)0x1a;
	string strTel;
	int nLen = 0;
    int leftLen = 0;
	string strContent = "";
	char szTel[20] = {0};
    char szTime[14] = {0};

	DBG_GSM(("\n------------------------------GSM:发送短信--------------------------------\n"));
	TelParityChange(tel, szTel);
    getTimeStamp(szTime);
//	AddMsg((unsigned char*)content, strlen(content));
	//编码转换
	nLen = GB2UniCode(content, (unsigned char*)szATBuf);
	
	for(int i=0;i<nLen;i++ ) 
	{
		char tempBuf[3] = {0};
		sprintf(tempBuf, "%02X",szATBuf[i]);
		strContent += tempBuf;
	}

	char szLen[5] = {0};
	nLen = strContent.size()/2;

    if( nLen < SMS_CONTENT_PDU_MAXLEN )      //非长短信，直接发送
    {
	    sprintf(szLen, "%02x", nLen);
	    strContent = szLen + strContent;

	    strTel = "11000D91";
	    strTel += szTel;
	    strTel += "000800";
	    strTel += strContent;

	    nLen = strTel.size()/2;
	    memset(szATMsg, 0, sizeof(szATMsg));
	    sprintf(szATMsg, "AT+CMGS=%d\r", nLen);//lhy 短信收发格式确认
	    if (!DoCmd(szATMsg, (char*)">", NULL, nLen, 0, 500))
        {
		    return SYS_STATUS_FAILED;
        }	

	    strTel = m_strCenterAddr + strTel;
	    strTel += cZ;
	    char respBuf[1500] = {0};
	    nLen = sizeof(respBuf);
	    if (!DoCmd(strTel.c_str(), (char*)"OK", respBuf, nLen, 10, 500))
		{
			DBG(("Send Msg error 333!\n"));
		    return SYS_STATUS_FAILED;
        }
    }
    else                                 //长短信，需分拆
    {
        int cntMax = 0;
        int cnt = 1;
        string subContent = "";
        cntMax = nLen / SMS_CONTENT_PDU_MAXLEN;
        if( nLen % SMS_CONTENT_PDU_MAXLEN != 0 )
        {
            cntMax++;
        }
        char temp[4] = {0};
        for( cnt=1; cnt <= cntMax; cnt++ )
        {
            //memset( subContent, 0, sizeof(subContent));
            strTel =  "51";
            strTel += "0D91683137085495F8";//"0D91683165170028F2";        //回复地址
            strTel += "0008";                      //TP-PID ， TP-DCS
            strTel += szTime;                      //时间戳
            strTel += "8C05000302";                //用户信息
            memset( temp, 0, sizeof(temp));
            sprintf( temp, "%02d", cntMax );
            strTel += temp;                        //分拆总条数
            memset( temp, 0, sizeof(temp));
            sprintf( temp, "%02d", cnt); 
            strTel += temp;                        //当前短信编号

            if( cnt != cntMax )
            {
                subContent = strContent.substr( (cnt-1)*SMS_CONTENT_PDU_MAXLEN*2, (cnt-1)*SMS_CONTENT_PDU_MAXLEN*2+SMS_CONTENT_PDU_MAXLEN*2 );
            }
            else
            {
                leftLen = strTel.size() - (cnt-1)*SMS_CONTENT_PDU_MAXLEN*2;
                subContent = strContent.substr( (cnt-1)*SMS_CONTENT_PDU_MAXLEN*2, (cnt-1)*SMS_CONTENT_PDU_MAXLEN*2+leftLen );
            }
            strTel += subContent;

            nLen = strTel.size()/2;
	        memset(szATMsg, 0, sizeof(szATMsg));
	        sprintf(szATMsg, "AT+CMGS=%d\r", nLen);//lhy 短信收发格式确认
            if( cnt == 1 )
            {
	        if (!DoCmd(szATMsg, (char*)">", NULL, nLen, 0, 500))
            {
		        return SYS_STATUS_FAILED;
            }
            }

            strTel = m_strCenterAddr + strTel;
            strTel += cZ;
            char respBuf[1500] = {0};
	        nLen = sizeof(respBuf);
	        if (!DoCmd(strTel.c_str(), (char*)"OK", respBuf, nLen, 10, 500))
            {
                DBG(("Send Msg error!\n"));
		        return SYS_STATUS_FAILED;
            }
            DBG_GSM(("cnt:[%d]  cntMax:[%d]\n", cnt, cntMax));
        }
    }
	//lhy 截取序号

	return SYS_STATUS_SUCCESS;
}
/******************************************************************************/
//Function Name  : DeleteSms                                               
//Description    : 接收短信                                    
//Input          : none
//Output         : none                                                     
//Return         : true/false                                                   
/*****************************************************************************/
bool CGSMBaseSiemens::DeleteSms(int index)
{ 
	bool ret = false;	
	CTblLockGSMBase.lock();
	if ( STA_LIVING_SIEMENS != m_enGSMStatus || STA_LIVING_SIEMENS != m_enPhone)
	{
		CTblLockGSMBase.unlock();
		return ret;
	}
	ret = DoDeleteSms(index);
	CTblLockGSMBase.unlock();
	return ret;
}

/******************************************************************************/
//Function Name  : DeleteSms                                               
//Description    : 接收短信                                    
//Input          : none
//Output         : none                                                     
//Return         : true/false                                                   
/*****************************************************************************/
bool CGSMBaseSiemens::DoDeleteSms(int index)
{ 
	bool ret = false;
	int nLen = 0;
	char respBuf[1400] = {0};
	char reqBuf[128] = {0};
	nLen = sizeof(reqBuf);
	sprintf(reqBuf, "AT+CMGD=%d\r",  index);
	ret = DoCmd(reqBuf, (char*)"OK", respBuf, nLen, 2, 500);
	DBG(("CGSMBaseSiemens::DoDeleteSms([%d] Ret[%d])", index, ret));
	return ret;
}

/******************************************************************************/
//Function Name  : RecvSms                                               
//Description    : 接收短信                                    
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
bool CGSMBaseSiemens::RecvSms(int smsType)
{ 
	bool ret = false;
	if(STA_LIVING_SIEMENS != m_enGSMStatus)
		return ret;
	CTblLockGSMBase.lock();
	if ( STA_LIVING_SIEMENS != m_enGSMStatus)
	{
		CTblLockGSMBase.unlock();
		return ret;
	}
	ret = DoRecvSms(smsType);
	CTblLockGSMBase.unlock();
	return ret;
}
/******************************************************************************/
//Function Name  : RecvSms                                               
//Description    : 接收短信                                    
//Input          : none
//Output         : none                                                     
//Return         : 串口句柄， -1为失败                                                     
/******************************************************************************/
bool CGSMBaseSiemens::DoRecvSms(int smsType)
{ 
	bool ret = false;

	char reqBuf[50] = {0};
	sprintf(reqBuf, "AT+CMGL=%d\r", smsType);

	int nRcvLen = 0;
	char cBuff[MAX_RECV_BUF_SIEMENS] = {0};
	int RespLen = 0;
	TKU32 nRcvPos = 0;
	int nCursor = 0;
	bool bContParse = true;
	CodecTypeGSM ctRslt;

	if (!ComSend((unsigned char*)reqBuf, strlen(reqBuf)))
		return false;
	do 
	{
		memset((unsigned char*)&cBuff[nRcvPos], 0, MAX_RECV_BUF_SIEMENS - nRcvPos - 1);
		if (0 >= (nRcvLen = ComRecv((unsigned char*)&cBuff[nRcvPos], MAX_RECV_BUF_SIEMENS - nRcvPos - 1, 0, 500, false)))
		{
			break;
		}

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

			ctRslt = DecodeRecvSMS(&cBuff[nCursor], nLen);

			switch(ctRslt)
			{
			case CODEC_SUCCESS_GSM:
				nCursor += nLen;
				break;
			case CODEC_NEED_DATA_GSM:
				bContParse = false;
				break;
			case CODEC_PDU_TYPE_ERROR:
			case CODEC_CODE_TYPE_ERROR:
			case CODEC_ERR_GSM:													
				//nRcvPos = 0;	
				nCursor += nLen;
				//bContParse = false;
				break;
			default:
				break;
			}
		}

		assert(nRcvPos >= 0);
		if(0 != nRcvPos)
		{
			memcpy(cBuff, &cBuff[nCursor], nRcvPos - nCursor);
			nRcvPos -= nCursor;
		}

	} while (true);

	return ret;
}

/******************************************************************************/
//Function Name  : DecodeRecvSMS                                               
//Description    : 数据解码                                    
//Input          : none
//Output         : none                                                     
//Return         : CodecTypeGSM                                                    
/******************************************************************************/
CodecTypeGSM CGSMBaseSiemens::DecodeRecvSMS(char* pPDUMsg, int& nUsed)
{ 
	char* ptrMsgTemp = NULL;
	char sms[1024] = {0};

	//解析
	if (NULL == (ptrMsgTemp = strstr(pPDUMsg, "+CMGL:")))
	{
		return CODEC_ERR_GSM;
	}

	char* tempCMGL = strstr(ptrMsgTemp + strlen("+CMGL:"), "+CMGL:");
	if (NULL != tempCMGL)
	{
		memcpy(sms, ptrMsgTemp, tempCMGL - ptrMsgTemp);
		nUsed = tempCMGL - pPDUMsg;
	}
	else
	{
		strcpy(sms, ptrMsgTemp);
		nUsed = strlen(pPDUMsg);
	}

	GSM_MSG gsmMsg;
	memset((unsigned char*)&gsmMsg, 0, sizeof(GSM_MSG));
	gsmMsg.iMsgType = 0;
	CodecTypeGSM parseRet = ParseRecvPDU(sms, gsmMsg.stSmsInfo);
	switch(parseRet)
	{
	case CODEC_NEED_DATA_GSM:
		DBG_GSM(("Need Data\n"));
		break;
	case CODEC_SUCCESS_GSM:
		DBG(("SMS: MsgType[%d], tel[%s] index[%d] Data[%s] Time[%s]\n", gsmMsg.iMsgType, gsmMsg.stSmsInfo.tel, gsmMsg.stSmsInfo.index, gsmMsg.stSmsInfo.content, gsmMsg.stSmsInfo.szTimeStamp));
		if(!SendMsg_Ex(m_ulHandleGSMRecv, MUCB_DEV_SMS_RECV, (char*)(&gsmMsg), sizeof(GSM_MSG)))
		{
			DBG_GSM(("GsmActionProc SendMsg_Ex failed, handle[%d]\n", m_ulHandleGSMRecv));
		}
		break;
	case CODEC_PDU_TYPE_ERROR:
	case CODEC_CODE_TYPE_ERROR:
		DBG_GSM(("PDU_TYPE_ERR_GSM CODEC_CODE_TYPE_ERROR\n"));
		DoDeleteSms(gsmMsg.stSmsInfo.index);
		break;
	case CODEC_ERR_GSM:
		DBG_GSM(("CODEC_ERR_GSM\n"));
		break;
	}
	return parseRet;
}


/******************************************************************************/
//Function Name  : ParseRecvPDU                                               
//Description    : PDU数据解析                                    
//Input          : 字符串数据
//Output         : smsOut，短信内容                                                     
//Return         : CodecTypeGSM                                                   
/******************************************************************************/
CodecTypeGSM CGSMBaseSiemens::ParseRecvPDU(char* pPDUMsg, SMS_INFO& smsOut)
{ 
//	bool ret = false;
	char szIndex[10] = {0};
	char szStat[10] = {0};
	char szAlpha[10] = {0};
	char szLength[10] = {0};
	//int pduLen = 0;
	char szPDU[512] = {0};
	//int nScanf = 0;
	int nLen = 0;
	char* ptrBuf = NULL;
//	CodecFormatGSM dataFormat = UN_KONW;

	if (CMGL_PRE_LEN >= strlen(pPDUMsg))
	{
		return CODEC_NEED_DATA_GSM;
	}
	memset((char*)&smsOut, 0, sizeof(SMS_INFO));

//	DBG_GSM(("-------------0000000000-------------- \n%s\n",pPDUMsg));

//	AddMsg((unsigned char*)pPDUMsg, strlen(pPDUMsg));
	char preBuf[56] = {0};
	ptrBuf = strstr(pPDUMsg, "\r\n");
	if (NULL == ptrBuf)
	{
		return CODEC_ERR_GSM;
	}
	
	memcpy(preBuf, pPDUMsg, ptrBuf - pPDUMsg);

	//DBG_GSM(("PreBuf[%s]\n", preBuf));

	char* tempT1 = strstr(preBuf, ",");
	char* tempT2 = preBuf + strlen("+CMGL:");

	//szIndex
	if (NULL == tempT1)
		return CODEC_ERR_GSM;
	if (tempT1 != tempT2)
	{
		memcpy(szIndex,tempT2, tempT1 - tempT2);
	}
	smsOut.index = atoi(szIndex);

	//szStat
	if (NULL == (tempT2 = strstr(tempT1 + 1, ",")))
	{
		return CODEC_ERR_GSM;
	}
	if (tempT1 != tempT2)
	{
		memcpy(szStat,tempT1 + 1, tempT2 - tempT1 - 1);
	}

	//szAlpha
	if (NULL == (tempT1 = strstr(tempT2 + 1, ",")))
	{
		return CODEC_ERR_GSM;
	}
	if (tempT1 != tempT2)
	{
		memcpy(szAlpha,tempT2+1, tempT1 - tempT2 - 1);
	}

	//szLength
	strcpy(szLength, tempT1 + 1);

	strcpy(szPDU, ptrBuf + 2);

	DBG_GSM(("-------------1111111111-------------- \n1:[%s] 2:[%s]  3:[%s]  4:[%s] \n 5:[%s]", szIndex, szStat, szAlpha, szLength, szPDU));
	if (atoi(szLength)*2 > (long)strlen(szPDU))
	{
		return CODEC_NEED_DATA_GSM;
	}

	//0891683108501705F011FF0881018076000000470131
	//0891683108501705F0
	//11FF0881018076000000470131
	//0891683108508741F4
	//2405A10180F6000811218241841523625C0A656C76845BA26237FF0C60A84ECA59295DF28F93516595198BEF670D52A15BC6780100316B21FF0C8FD853EF4EE58F93516500326B21FF0C670D52A15BC6780151737CFB60A84E2A4EBA4FE1606F5B895168FF0C8BF76CE8610F4FDD62A40021
	
	//SCA PDUType OA PID DCS SCTS UDL UD 
	//0891683108501705F2 24(PDU Type) 0C(源号码长度) A1(号码类型) 015657852033(源号码) 00(PID) 08(DataCoding Type, 00， 7bit, F6 8bit, 08 UCS2) 11101221340323(时间戳) 80(数据长度) data
	//0891683108501705F0 04 0D 91 683185883478F7000811501151938123064E2D56FD4EBA
	char* pPos = szPDU;

	//SCA
	char szLen[3] = {0};
	memcpy(szLen, pPos, 2);
	pPos += strtol(szLen, NULL, 16)*2 + 2;//跳过短信中心地址	
	//PDUType lhy不同类型处理，目前只支持24
	char szPDUType[3] = {0};
	memcpy(szPDUType, pPos, 2);
	if (24 != atoi(szPDUType) && 4 != atoi(szPDUType) && 64 != atoi(szPDUType))
	{
		return CODEC_PDU_TYPE_ERROR;
	}
	pPos += 2;
	//OA,源地址

	char sourceTel[20] = {0};
	memcpy(szLen, pPos, 2);//源号码长度
	pPos += 2;
	pPos += 2;//跳过源号码类型
	nLen = (0 != strtol(szLen, NULL, 16)%2)?strtol(szLen, NULL, 16) + 1:strtol(szLen, NULL, 16);
	memcpy(sourceTel, pPos, nLen);	
	TelParityChange(sourceTel, sourceTel);
	if (NULL != (ptrBuf = strstr(sourceTel, "F")))
	{
		*ptrBuf = '\0';
	}

	strcpy(smsOut.tel, sourceTel);

	pPos += nLen;//跳过源地址
	//PID
	pPos += 2;
	//DataCoding Type
	char codetype[3] = {0};
	memcpy(codetype, pPos, 2);
	pPos += 2;	

	//TimeStamp
	char timestamp[15] = {0};
	memcpy(timestamp, pPos, 14);
	pPos += 14;
	
	//DataLen
	memcpy(szLen, pPos, 2);
	nLen = strtol(szLen, NULL, 16);
	pPos += 2;

	strcpy(smsOut.szTimeStamp, "20");
	gsmSerializeNumbers(timestamp, smsOut.szTimeStamp + 2, 12);
	
	//Data
	char smsData[500] = {0};
	char smsDataU[500] = {0};
	memcpy(smsData, pPos, nLen*2);
//	Hex2String((char*)smsDataU, (char*)smsData, nLen*2);
	String2Hex((unsigned char*)smsDataU, (unsigned char*)smsData, nLen*2);
	switch (strtol(codetype, NULL, 16))
	{
	case 0:
		gsmDecode7bit((unsigned char*)smsDataU, smsOut.content, nLen);
		break;
	case 0xF6:
		memcpy(smsOut.content, smsDataU, nLen);
		break;
	case 0x08:		
		Unicode2GB(smsDataU, smsOut.content, nLen);
		break;
	default:
		return CODEC_CODE_TYPE_ERROR;
	}

	DBG(("DataCode 2 Type[%ld] Content[%s][%s][%d]\n", strtol(codetype, NULL, 16), smsData, smsOut.content, nLen));
	return CODEC_SUCCESS_GSM;
}

/************************************************************************/
/* outgoing calls                                                       */
/************************************************************************/
/***********************************************************************
//Function Name  : PhoneCall                                               
//Description    : 拨打指定电话并放音                                          
//Input          : char* tel			目的电话                                
//		         : char* soundPos		录音索引
//Output         : none
//Return         : -1=Unkonw error；0=OK;1=无载波，NO CARRIER;2=对方正在通话中，BUSY;3=无拨号音，NO DIAL TONE；4=呼出受阻，Call Barred
***********************************************************************/
int CGSMBaseSiemens::PhoneCall(char* Tel, char* soundPos)
{
	int ret = SYS_STATUS_GSM_BUSY;
	if(STA_LIVING_SIEMENS != m_enGSMStatus)
		return ret;
	CTblLockGSMBase.lock();
	if ( STA_LIVING_SIEMENS != m_enGSMStatus || STA_LIVING_SIEMENS != m_enPhone )
	{
		CTblLockGSMBase.unlock();
		return SYS_STATUS_GSM_BUSY;
	}
	ret = DoPhoneCall(Tel, soundPos);
	CTblLockGSMBase.unlock();
	return ret;
}
int CGSMBaseSiemens::DoPhoneCall(char* Tel, char* soundPos)
{
	int ret = SYS_STATUS_FAILED;
	char szBuf[128] = {0};
	char szOutBuf[56] = {0};
	int nLen = sizeof(szOutBuf);

	DBG_GSM(("PhoneCall [%s] Play %s \n", Tel, soundPos));
	//拨号
	sprintf(szBuf, "ATD%s;\r", Tel);
	do 
	{
		if (!DoCmd(szBuf, (char*)"OK", szOutBuf, nLen, 60, 0))
		{
			if (NULL != strstr(szOutBuf, "NO CARRIER"))
			{
				ret = SYS_STATUS_GSM_CALL_NO_CARRIER;
			}
			else if (NULL != strstr(szOutBuf, "BUSY"))
			{
				ret = SYS_STATUS_GSM_CALL_BUSY;
			}
			else if (NULL != strstr(szOutBuf, "NO DIALTONE"))
			{
				ret = SYS_STATUS_GSM_CALL_NO_DIALTONE;
			}
			else if (NULL != strstr(szOutBuf, "Call barred"))
			{
				ret = SYS_STATUS_GSM_CALL_BARRED;
			}

			break;
		}

		//放音
		if (!m_AudioPlay.StartPlay(soundPos))
		{
			m_enPhone = STA_LIVING_SIEMENS;
		}
		else
		{
			ret = SYS_STATUS_SUCCESS;
			m_enPhone = STA_CLOSED_SIEMENS;
		}
	} while (false);
	if (SYS_STATUS_SUCCESS != ret)
	{
		memset(szBuf, 0, sizeof(szBuf));
		memset(szOutBuf, 0, sizeof(szOutBuf));
		strcpy(szBuf, "ATH\r");
		nLen = sizeof(szOutBuf);
		DoCmd(szBuf, (char*)"OK", szOutBuf, nLen, 0, 500);
		m_enPhone = STA_LIVING_SIEMENS;
	}
	return ret;
}
/***********************************************************************
//Function Name  : DoCmd                                               
//Description    : 模块控制指令函数                                          
//Input          : char* ptrResq     控制指令                                
//		         : char* checkValue     回应校验码，如为空则不校验
//Output         : char* outBuf			回应的数据， 如为空则不返回
//               : char& outLen			回应的数据长度
//Return         : 指令执行结果，成功-true，失败-false     
***********************************************************************/
bool CGSMBaseSiemens::DoCmd(const char* ptrResq, char* checkValue, char* outBuf, int& outLen, int timeoutSec, int timeoutMill)
{
	bool ret = false;
	unsigned char buf[1400] = {0};
	unsigned char* ptrBuf = (unsigned char*)outBuf;
	int needLen = outLen;
	if (NULL == ptrBuf)
	{
		ptrBuf = buf;
		needLen = sizeof(buf);
	}
	outLen = 0;
	if (0 >= strlen(ptrResq))
	{
		return false;
	}
	if (ComSend((unsigned char*)ptrResq, strlen(ptrResq)))
	{
        DBG_GSM(("Sms comm recv----->\n"));
		if (0 < (outLen = ComRecv(ptrBuf,needLen, timeoutSec, timeoutMill, false)))
		{
			if (NULL == checkValue || NULL != strstr((char*)ptrBuf, checkValue))
			{
				ret = true;
			}
		}
        DBG_GSM(("ret:[%d]  \n", ret));
	}	
	return  ret;					//返回错误
}

bool CGSMBaseSiemens::DoCmdEx(const char* ptrResq, char* checkValue, char* outBuf, int& outLen, int timeoutSec, int timeoutMill)
{
	bool ret = false;
	unsigned char buf[1400] = {0};
	unsigned char* ptrBuf = (unsigned char*)outBuf;
	int needLen = outLen;
	if (NULL == ptrBuf)
	{
		ptrBuf = buf;
		needLen = sizeof(buf);
	}
	outLen = 0;
	if (0 >= strlen(ptrResq))
	{
		return false;
	}
	if (ComSend((unsigned char*)ptrResq, strlen(ptrResq)))
	{
        DBG_GSM(("Sms comm recv----->\n"));
		
				ret = true;

        DBG_GSM(("ret:[%d]  \n", ret));
	}	
	return  ret;					//返回错误
}

/***********************************************************************
//Function Name  : QueryAddr                                               
//Description    : 查询短信中心地址码                                        
//Input          : none     
//Output         : char* outBuf			回应的数据  
//               : char& outLen			回应的数据长度                                                   
//Return         : 指令执行结果，成功-true，失败-false     
***********************************************************************/
bool CGSMBaseSiemens::QueryAddr(char* outBuf)
{
	bool ret = false;
	unsigned char buf[1400] = {0};
	int recvLen = 0;

	if (ComSend((unsigned char*)"AT+CSCA?\r", strlen("AT+CSCA?\r")))
	{
		if (0 < (recvLen = ComRecv(buf, sizeof(buf), 1, 0, false)))
		{
			if (NULL != strstr((char*)buf, "+CSCA"))
			{
				sscanf((char*)buf, "%*[^\"]\"%[^\"]", outBuf);
				if(0 < strlen(outBuf))
					ret = true;
			}
		}
	}	
	return  ret;					//返回错误
}
/******************************************************************************/
//Function Name  : OpenCom                                               
//Description    : 打开GSM串口                                          
//Input          : none
//Output         : none                                                     
//Return         : true/false                                                    
/******************************************************************************/
bool CGSMBaseSiemens::OpenCom()
{    
	//------------------------------串口初始化----------------------------------
	if(-1 != m_hFd)
	{
		close(m_hFd);
		m_hFd = -1;
	}

	if (-1 >= (m_hFd = open(m_strComId, O_RDWR | O_NOCTTY | O_NDELAY))) 
	{
		fprintf (stderr, "cannot open port %s\n", m_strComId);
		return false;
	}
	
	fcntl(m_hFd, F_SETFL, 0);

	termios termios_new;
	/* 0 on success, -1 on failure */
	bzero (&termios_new, sizeof (termios_new));
	cfmakeraw (&termios_new);

	//termios_new.c_cflag = BAUDRATE (baudrate);
	int baudrate = BAUDRATE (m_iBaudRate);
	if(0 == baudrate)
		return false;

	termios_new.c_cflag = baudrate;
	termios_new.c_cflag |= CLOCAL | CREAD;      /* | CRTSCTS */
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8;

	//	termios_new.c_cflag &= ~CNEW_RTSCTS;

	termios_new.c_cflag &= ~PARENB;				// No Patriy
	termios_new.c_cflag &= ~CSTOPB;				/* 1 stop bit */
	termios_new.c_oflag = 0;
	termios_new.c_lflag |= 0;
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_cc[VTIME] = 100;				/* unit: 1/10 second. */
	termios_new.c_cc[VMIN] = 1;					/* minimal characters for reading */
	tcflush (m_hFd, TCIFLUSH);
	//		tcflush (m_hFd, TCOFLUSH);

	int retval = tcsetattr (m_hFd, TCSANOW, &termios_new);
	if (0 != retval)
	{
		return false;
	}

	DBG(("Open Comm handle[%d] comname[%s]\n", m_hFd, m_strComId));
//	m_enGSM = STA_INIT_SIEMENS;
	return true;
}

/******************************************************************************/
//Function Name  : ComSend                                               
//Description    : 向串口发送数据                                            
//Input          : BYTE* inBuf			发送到串口的数据                    
//                 BYTE sendLen         发送数据长度                       
//Output         : none                                                      
//Return         : 成功发送的数据长度                                                    
/******************************************************************************/
bool CGSMBaseSiemens::ComSend(BYTE* inBuf, BYTE sendLen)
{
	bool ret = false;
	int nMsgLeft = 0;
	int nMsgLen = 0;
	int nMaxResend = 0;

	int nBytes = 0;
	nMsgLeft = nMsgLen = sendLen;

	if (m_hFd < 0)
	{
		DBG_GSM(("ComSend opencomm Old hld[%d]\n", m_hFd));
		OpenCom();
	}
	DBG_GSM(("SEND Comm:  %s\n", inBuf));
	if(nMsgLeft > 0)
	{ 		
		nMaxResend = 3;
		while (nMsgLeft && nMaxResend) 
		{
			nBytes = write(m_hFd, inBuf + nMsgLen - nMsgLeft, nMsgLeft);
			if (nBytes < 0)
			{
				m_iWatchCnt++;
				DBG_GSM(("SendMsg write < 0 nByte[%d] hdl[%d]\n", nBytes, m_hFd));
				nMaxResend = -1;
				DBG_GSM(("ComSend -1 opencomm Old hld[%d]\n", m_hFd));
				OpenCom();
				break;
			}
			//AddMsg(inBuf + nMsgLen - nMsgLeft, nBytes);
			nMsgLeft = nMsgLeft - nBytes;
			nMaxResend--;
		}
		switch(nMaxResend) 
		{
		case 0:
			break;
		case -1:
			break;
		default:
			ret = true;
			break;
		}
	}
    DBG_GSM(("SEND Comm End\n"));
	return ret;
}

/******************************************************************************/
//Function Name  : ComRecv                                             
//Description    : 从串口接收数据                                            
//Input          : none                     
//Output         : BYTE* oubBuf			从串口接收数据                    
//                 BYTE needLen			希望接收的长度                                                  
//Return         : 实际接收的长度，-1表示接收失败                                                     
/******************************************************************************/
int CGSMBaseSiemens::ComRecv(BYTE* oubBuf, int needLen, int timeoutSec, int timeoutmill, bool isPassive)
{
	int ret = -1;

	if (m_hFd < 0)
	{
		DBG_GSM(("serial port have not been opened \n"));

		DBG_GSM(("ComRecv opencomm Old hld[%d]\n", m_hFd));
		OpenCom();
		return ret;
	}

	fd_set ReadSetFD;
	FD_ZERO(&ReadSetFD);
	FD_SET(m_hFd, &ReadSetFD);

	struct timeval sttimeval;
	memset(&sttimeval, 0, sizeof(timeval));

	sttimeval.tv_sec = timeoutSec;
	sttimeval.tv_usec = timeoutmill * 1000;

//	DBG_GSM(("Recv Comm handle[%d]\n", m_hFd));
	while (ret < needLen)
	{
		int n = 0;
		if (0 >= (n = select(m_hFd + 1, &ReadSetFD, NULL, NULL, &sttimeval)))
		{
			//DBG_GSM((strerror(errno)));
			break;
		}
		else if (0 < n)
		{
			if (FD_ISSET(m_hFd, &ReadSetFD)) 
			{
				if (0 > ret)
				{
					ret = 0;
				}
				int readlen = read(m_hFd, oubBuf + ret, needLen - ret);
				ret += readlen;
			}
		}
		sttimeval.tv_sec = 0;
		sttimeval.tv_usec = 50 * 1000;
	}
	if ((0 < ret && NULL != strstr((char*)oubBuf, "ERROR")) || (0 >= ret && !isPassive))
	{
		m_iWatchCnt++;
		DBG_GSM(("watchcnt[%d] ret[%d]\n", m_iWatchCnt, ret));
		//AddMsg(oubBuf, ret);
	}
	else if (NULL != strstr((char*)oubBuf, "NO CARRIER") || NULL != strstr((char*)oubBuf, "NO ANSWER"))//
	{
		//关闭放音
		m_AudioPlay.StopPlay();
		m_enPhone = STA_LIVING_SIEMENS;
	}
	else if(ret > 0)
	{
		m_iWatchCnt=0;
	}
	
	if(ret > 0)
		DBG_GSM(("RECV:  %s\n", oubBuf));
		
	return ret;
}
