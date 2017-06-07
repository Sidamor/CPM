#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <unistd.h>
#include <locale.h>

#include "Init.h"
#include "SqlCtl.h"
#include "OnTimeCtrl.h"

#define ONEDAY_SECONDS       (24*60*60)
#define ONEWEEK_SECONDS      (7*24*60*60)


#ifdef  DEBUG
#define DEBUG_ON_TIME_CTRL
#endif

#ifdef DEBUG_ON_TIME_CTRL
#define DBG_ONTIME_CTRL(a)		printf a;
#else
#define DBG_ONTIME_CTRL(a)	
#endif


COnTimeCtrl::COnTimeCtrl()
{
}

COnTimeCtrl::~COnTimeCtrl(void)
{				
}

/******************************************************************************/
// Function Name  : Initialize                                               
// Description    : 初始化定时任务模块                                          
// Input          : none
// Output         : none                                                     
// Return         : true:成功;false:失败                                                     
/******************************************************************************/
bool COnTimeCtrl::Initialize()
{   
    m_ptrOnTimeActInfoTable = new HashedOnTimeActInfo();
	if (NULL == m_ptrOnTimeActInfoTable)
	{
		return false;
	}
    //初始化定时任务表

    initOntimeInfo();
	return true;
}

bool COnTimeCtrl::initOntimeInfo()
{
    bool ret = false;
    char column[128] = "select seq,id,sn,ctime,cdata from task_info;";
    if( 0 == g_SqlCtl.select_from_tableEx(column, initOnTimeInfoCallBack, (void *)this) )
    {
        ret = true;
    }
    return ret;
}

int COnTimeCtrl::initOnTimeInfoCallBack(void *data, int n_column, char **column_value, char **column_name)
{
    COnTimeCtrl* _this = (COnTimeCtrl*)data;
	_this->initOnTimeInfoCallBackProc(n_column, column_value, column_name);	
	return 0;
}

int COnTimeCtrl::initOnTimeInfoCallBackProc(int n_column, char **column_value, char **column_name)
{
    int ret = -1;
    ONTIME_ACT_HASH hash_node;
    memset(&hash_node, 0, sizeof(ONTIME_ACT_HASH));
    hash_node.Seq = atoi(column_value[0]);                  //唯一键
    memcpy(hash_node.id, column_value[1], DEVICE_ID_LEN);               //设备编号
    memcpy(hash_node.id+DEVICE_ID_LEN, column_value[2], DEVICE_ACTION_SN_LEN);             //设备sn
    hash_node.actTime = getNextActTime(column_value[3]);    //计算下次动作时间
	memcpy(hash_node.szTime, column_value[3], 12);
    strcpy(hash_node.actValue, column_value[4]);            //动作内容    
	DBG(("seq:[%d] id:[%s] time:[%ld] value:[%s]\n\n", hash_node.Seq, hash_node.id, hash_node.actTime, hash_node.actValue));
 //   CTblLock.lock();	
	if(0 < hash_node.actTime)
	{//数据库状态更改
		
		addOnTimeNode(hash_node);
	}
//	CTblLock.unlock();
	
	return ret;
}

/******************************************************************************/
// Function Name  : getNextActTime                                               
// Description    : 根据定时信息获得下一次动作的时间                                         
// Input          : char* shortTime   定时信息(12字节，yyyymmddhhss)
//					  yyyy:	0000-每年，2013、2014.。。
//					    mm:	00-每月
//							90-每周
//							01-1月
//							02-2月
//							...
//					    dd:	00-每天
//							91-周日
//							...
//							96-周六
//							01-1号
//							02-2号
//							...
// Output         : none                                                     
// Return         : time_t 下次动作时间                                                     
/******************************************************************************/
time_t COnTimeCtrl::getNextActTime(char* shortTime)
{
	int shortYear = 0;
	int shortMonth = 0;
	int shortDay = 0;

	int shortHour = 0;
	int shortMin = 0;
	char temp4[5] = {0};
	char temp2[3] = {0};

	memcpy(temp4, shortTime, 4);
	shortYear = atoi(temp4);
	memcpy(temp2, shortTime+4, 2);
	shortMonth = atoi(temp2);
	memcpy(temp2, shortTime+6, 2);
	shortDay = atoi(temp2);

	memcpy(temp2, shortTime+8, 2);
	shortHour = atoi(temp2);
	memcpy(temp2, shortTime+10, 2);
	shortMin = atoi(temp2);


	//当前时间
	time_t tm_NowTime;
	struct tm *st_tmNowTime;
	time(&tm_NowTime);								//取得当前时间
	st_tmNowTime = localtime(&tm_NowTime);			//转换成结构体模式
	time_t tm_ZeroTime = tm_NowTime - tm_NowTime;
	DBG_ONTIME_CTRL(("shortTime[%s] now_yday[%d] now_wday[%d] now_month[%d] now_mday[%d] now_hour[%d] now_min[%d] \n",  shortTime, st_tmNowTime->tm_yday, st_tmNowTime->tm_wday, st_tmNowTime->tm_mon, st_tmNowTime->tm_mday, st_tmNowTime->tm_hour, st_tmNowTime->tm_min));
	//下次执行时间
	time_t tm_ActTime;
	struct tm stNewTime = {0};
	if(0 == shortYear)
	{//0000 XX XX XX XX 每年
		memcpy((unsigned char*)&stNewTime, (unsigned char*)st_tmNowTime, sizeof(struct tm));
		if (90 == shortMonth)
		{//0000 90 9X XX XX 每周			
			stNewTime.tm_hour = shortHour;
			stNewTime.tm_min = shortMin;
			stNewTime.tm_sec = 0;
			tm_ActTime = mktime(&stNewTime);
			int diffWDay = shortDay - 91 - st_tmNowTime->tm_wday;
			if (0 == diffWDay)
			{//今天
				if(tm_NowTime >= tm_ActTime)
				{//今天已过，下周
					tm_ActTime += ONEWEEK_SECONDS;
				}
			}
			else if(diffWDay > 0)
			{//这周
				tm_ActTime += diffWDay*ONEDAY_SECONDS;
			}
			else
			{//下周
				diffWDay =  diffWDay + 7;
				tm_ActTime += diffWDay*ONEDAY_SECONDS;
			}
			printf("diffWDay[%d]\n", diffWDay);
		}
		else if(0 == shortMonth)
		{//0000 00 每月
			if (0 == shortDay)
			{//0000 00 00 HH MM 每天
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				stNewTime.tm_sec = 0;
				tm_ActTime = mktime(&stNewTime);
				if(tm_NowTime >= tm_ActTime)
				{
					tm_ActTime += ONEDAY_SECONDS;
				}	
			}
			else
			{//0000 00 DD HH MM 每年每月某天
				stNewTime.tm_mday = shortDay;	//日
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				stNewTime.tm_sec = 0;
				tm_ActTime = mktime(&stNewTime);
				if (tm_NowTime >= tm_ActTime)
				{//这个月已过，下个月
					if (12 == st_tmNowTime->tm_mon)
					{
						tm_ActTime = mktime(&stNewTime);
						tm_ActTime += 31*ONEDAY_SECONDS;
					}
					else
					{
						struct tm* stTmpTime = localtime(&tm_ActTime);
						stTmpTime->tm_mon += 1;
						tm_ActTime = mktime(stTmpTime);
					}
				}
			}
		}
		else
		{//0000 MM XX XX XX 每年某月
			stNewTime.tm_mon = shortMonth - 1;	//月
			if (0 == shortDay)
			{//0000 MM 00 XX XX 每年某月每天
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				stNewTime.tm_sec = 0;
				if (st_tmNowTime->tm_mon < shortMonth - 1)
				{//没到该月
					stNewTime.tm_mday = 1;
					tm_ActTime = mktime(&stNewTime);
				}
				else if (st_tmNowTime->tm_mon == shortMonth - 1)
				{//在这个月中
					stNewTime.tm_mday = st_tmNowTime->tm_mday;
					tm_ActTime = mktime(&stNewTime);
					if(tm_NowTime >= tm_ActTime)
					{//今天已过，明天
						tm_ActTime += ONEDAY_SECONDS;
						struct tm* stTmpTime = localtime(&tm_ActTime);
						if(stTmpTime->tm_mon != shortMonth - 1)
						{//不是某月了，明年
							stTmpTime->tm_year += 1;
							stTmpTime->tm_mday = 1;
							stTmpTime->tm_mon = shortMonth - 1;
							tm_ActTime = mktime(stTmpTime);
						}
					}
				}
				else
				{//已经过了这个月，明年
					stNewTime.tm_year += 1;
					stNewTime.tm_mday = 1;
					tm_ActTime = mktime(&stNewTime);
				}
			}
			else
			{//0000 MM DD XX XX 每年某月某天	
				stNewTime.tm_mday = shortDay;	//日
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				stNewTime.tm_sec = 0;
				tm_ActTime = mktime(&stNewTime);
				if (tm_NowTime >= tm_ActTime)
				{//今年已过
					stNewTime.tm_year += 1;
					tm_ActTime = mktime(&stNewTime);
				}
			}
		}
	}
	else
	{//YYYY XX XX XX XX 某年
		stNewTime.tm_year = shortYear - 1900;
		if(st_tmNowTime->tm_year > shortYear -1900)
		{//某年已过
			tm_ActTime = tm_ZeroTime;
		}
		else if(st_tmNowTime->tm_year == shortYear - 1900)
		{//今年符合条件
			memcpy((unsigned char*)&stNewTime, (unsigned char*)st_tmNowTime, sizeof(struct tm));
			if (90 == shortMonth)
			{//YYYY 90 9X XX XX 某年每周				
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				stNewTime.tm_sec = 0;
				tm_ActTime = mktime(&stNewTime);
				int diffWDay = shortDay - 91 - st_tmNowTime->tm_wday;
				if (0 == diffWDay)
				{//今天
					if(tm_NowTime >= tm_ActTime)
					{//今天已过，下周	
						tm_ActTime += ONEWEEK_SECONDS;
						struct tm* stTmpTime = localtime(&tm_ActTime);
						if(stTmpTime->tm_year != shortYear - 1900)
						{//不是某年了
							tm_ActTime = tm_ZeroTime;
						}
					}
				}
				else if(diffWDay > 0)
				{//这周
					diffWDay =  diffWDay;
					tm_ActTime += diffWDay*ONEDAY_SECONDS;
					struct tm* stTmpTime = localtime(&tm_ActTime);
					if(stTmpTime->tm_year != shortYear - 1900)
					{//不是某年了
						tm_ActTime = tm_ZeroTime;
					}	
				}
				else
				{//下周
					diffWDay =  diffWDay + 7;
					tm_ActTime += diffWDay*ONEDAY_SECONDS;
					struct tm* stTmpTime = localtime(&tm_ActTime);
					if(stTmpTime->tm_year != shortYear - 1900)
					{//不是某年了
						tm_ActTime = tm_ZeroTime;
					}
				}
				printf("diffWDay[%d]\n", diffWDay);
			}
			else if(0 == shortMonth)
			{//YYYY 00 XX XX XX 某年每月
				if (0 == shortDay)
				{//YYYY 00 00 XX XX 某年每月每天
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					stNewTime.tm_sec = 0;
					tm_ActTime = mktime(&stNewTime);
					if(tm_NowTime >= tm_ActTime)
					{//今天已过，明天
						tm_ActTime += ONEDAY_SECONDS;
						struct tm* stTmpTime = localtime(&tm_ActTime);
						if(stTmpTime->tm_year != shortYear - 1900)
						{//不是某年了
							tm_ActTime = tm_ZeroTime;
						}
					}
				}
				else
				{//YYYY 00 DD XX XX 某年每月某天
					stNewTime.tm_mday = shortDay;	//日
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					stNewTime.tm_sec = 0;
					tm_ActTime = mktime(&stNewTime);
					if (tm_NowTime >= tm_ActTime)
					{//当月某天已过，下个月
						struct tm* stTmpTime = localtime(&tm_ActTime);
						if (12 == stTmpTime->tm_mon)
						{
							tm_ActTime = tm_ZeroTime;
						}
						else
						{							
							stTmpTime->tm_mon += 1;
							tm_ActTime = mktime(stTmpTime);
						}
					}
				}
			}
			else
			{//YYYY MM XX XX XX 某年某月
				stNewTime.tm_mon = shortMonth - 1;	//月
				if (0 == shortDay)
				{//YYYY MM 00 XX XX 某年某月每天
					if(st_tmNowTime->tm_mon > shortMonth - 1)
					{//某年某月已过
						tm_ActTime = tm_ZeroTime;
					}
					else if(st_tmNowTime->tm_mon == shortMonth - 1)
					{//某年某月
						stNewTime.tm_mday = st_tmNowTime->tm_mday;	//当前日
						stNewTime.tm_hour = shortHour;
						stNewTime.tm_min = shortMin;
						stNewTime.tm_sec = 0;
						tm_ActTime = mktime(&stNewTime);
						if(tm_NowTime >= tm_ActTime)
						{//今天已过，明天
							tm_ActTime += ONEDAY_SECONDS;
							struct tm* stTmpTime = localtime(&tm_ActTime);
							if(stTmpTime->tm_mon != shortMonth - 1)
							{//不是某年某月了
								tm_ActTime = tm_ZeroTime;
							}
						}
					}
					else
					{//某年未到某月
						stNewTime.tm_mday = 1;
						stNewTime.tm_hour = shortHour;
						stNewTime.tm_min = shortMin;
						stNewTime.tm_sec = 0;
						tm_ActTime = mktime(&stNewTime);
					}
				}
				else
				{//YYYY MM DD XX XX 某年某月某天
					stNewTime.tm_mday = shortDay;	//日
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					stNewTime.tm_sec = 0;
					tm_ActTime = mktime(&stNewTime);
					if (tm_NowTime >= tm_ActTime)
					{
						tm_ActTime = tm_ZeroTime;
					}
				}
			}
		}
		else
		{//以后某年
			stNewTime.tm_year = shortYear - 1900;			//年
			if (90 == shortMonth)
			{//YYYY 90 9X XX XX 某年每周
				stNewTime.tm_yday = 0;
				stNewTime.tm_hour = shortHour;
				stNewTime.tm_min = shortMin;
				tm_ActTime = mktime(&stNewTime);
				struct tm* stTmpTime = localtime(&tm_ActTime);
				int diffWDay = shortDay - 90 - stTmpTime->tm_wday;
				if (0 == diffWDay)
				{//某年第一天
					tm_ActTime = mktime(&stNewTime);
				}
				else if(diffWDay > 0)
				{//某年第一周
					tm_ActTime += diffWDay*ONEDAY_SECONDS;
				}
				else
				{//某年第二周
					diffWDay = diffWDay + 7;
					tm_ActTime += diffWDay*ONEDAY_SECONDS;
				}
				printf("diffWDay[%d]\n", diffWDay);
			}
			else if(0 == shortMonth)
			{//YYYY 00 XX XX XX 某年每月
				stNewTime.tm_mon = 1;	//1月
				if (0 == shortDay)
				{//YYYY 00 00 XX XX 某年每月每天
					stNewTime.tm_mday = 1;
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					tm_ActTime = mktime(&stNewTime);
				}
				else
				{//YYYY 00 DD XX XX 某年每月某天
					stNewTime.tm_mday = shortDay;
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					tm_ActTime = mktime(&stNewTime);
				}
			}
			else
			{//YYYY MM XX XX XX 某年某月
				stNewTime.tm_mon = shortMonth - 1;	//月
				if (0 == shortDay)
				{//YYYY MM 00 XX XX 某年某月每天
					stNewTime.tm_mday = 1;
					stNewTime.tm_hour = shortHour;
					stNewTime.tm_min = shortMin;
					tm_ActTime = mktime(&stNewTime);
				}
				else
				{//YYYY MM DD XX XX 某年某月某天
					stNewTime.tm_mday = shortDay;	//日
					tm_ActTime = mktime(&stNewTime);
				}
			}			
		}
	}

	struct tm* stActTime = localtime(&tm_ActTime);	
	DBG_ONTIME_CTRL(("当前时间[%ld] 执行时间[%ld] year[%d] month[%d] day[%d] hour[%d] min[%d]\n", tm_NowTime, tm_ActTime, stActTime->tm_year+1900, stActTime->tm_mon+1, stActTime->tm_mday, stActTime->tm_hour, stActTime->tm_min));
	return tm_ActTime;
}



/******************************************************************************/
//Function Name  : OnTimeCtrlProc                                               
//Description    :                                        
//Input          : none
//Output         : none                                                     
//Return         : none                                                   
/******************************************************************************/
void COnTimeCtrl::OnTimeCtrlProc(void)
{ 
    char dstDev[DEVICE_ID_LEN_TEMP] = {0};
    char actId[DEVICE_ACTION_SN_LEN_TEMP] = {0};
    char szOprator[10] = "TIMING";

    time_t tm_NowTime;
	
    //------定时动作执行
    CTblLock.lock();
    HashedOnTimeActInfo::iterator it_onTime;
	PONTIME_ACT_HASH ontime_node;
    for (it_onTime = m_ptrOnTimeActInfoTable->begin(); it_onTime != m_ptrOnTimeActInfoTable->end(); it_onTime++)
    {
        time(&tm_NowTime);
		ontime_node = (PONTIME_ACT_HASH)(&it_onTime->second);
        if( tm_NowTime >= ontime_node->actTime && 0 < ontime_node->actTime )
		{//到时间
			DBG_ONTIME_CTRL(("定时任务 当前时间[%ld] 执行时间[%ld]\n", tm_NowTime, ontime_node->actTime));
            memset(dstDev, 0, sizeof(dstDev));
            memset(actId, 0, sizeof(actId));
            memcpy(dstDev, ontime_node->id, DEVICE_ID_LEN);    //目标设备ID
            memcpy(actId, ontime_node->id+DEVICE_ID_LEN, DEVICE_ACTION_SN_LEN); //动作sn
            DBG_ONTIME_CTRL(("DoCmd:[%s %s]\n", dstDev, actId));

			ACTION_MSG act_msg;
			memset((unsigned char*)&act_msg, 0, sizeof(ACTION_MSG));
			act_msg.ActionSource = ACTION_SOURCE_ONTIME;//lhy
			strcpy(act_msg.DstId, dstDev);
			strcpy(act_msg.ActionSn, actId);

			strcpy(act_msg.Operator, szOprator);
			strcpy(act_msg.ActionValue, ontime_node->actValue);
	
			int patchRet = CDevInfoContainer::DisPatchAction(act_msg);
			if (SYS_STATUS_SUBMIT_SUCCESS_NEED_RESP != patchRet)
			{
				CDevInfoContainer::SendToActionSource(act_msg, patchRet);
			}
//lsd增加

			ontime_node->actTime = getNextActTime(ontime_node->szTime);
			if(tm_NowTime >= ontime_node->actTime)
			{
				DBG_ONTIME_CTRL(("删除任务 \n"));
				m_ptrOnTimeActInfoTable->erase(it_onTime);
			}
        }
    }
	CTblLock.unlock();
}

bool COnTimeCtrl::addOnTimeNode(ONTIME_ACT_HASH hash_node)
{
    bool ret = false;
	ONTIME_ACT_HASH msg_node;
	HashedOnTimeActInfo::iterator it;

	memset((BYTE*)&msg_node, 0, sizeof(msg_node));
	memcpy((BYTE*)&msg_node, (BYTE*)&hash_node, sizeof(ONTIME_ACT_HASH));

	CTblLock.lock();
	it = m_ptrOnTimeActInfoTable->find(msg_node.Seq);
	if (it != m_ptrOnTimeActInfoTable->end())
	{
		m_ptrOnTimeActInfoTable->erase(it);
	}
	m_ptrOnTimeActInfoTable->insert(HashedOnTimeActInfo::value_type(msg_node.Seq,msg_node));
	CTblLock.unlock();
	return ret;
}

//取得定时任务节点并删除
bool COnTimeCtrl::GetAndDeleteOnTimeNode(ONTIME_ACT_HASH& hash_node)
{
    bool ret = false;
	HashedOnTimeActInfo::iterator it;
	CTblLock.lock();

	it = m_ptrOnTimeActInfoTable->find(hash_node.Seq);
	if (it != m_ptrOnTimeActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second, sizeof(ONTIME_ACT_HASH));
		m_ptrOnTimeActInfoTable->erase(it);
		ret = true;
	}
	CTblLock.unlock();
	return ret;
}

bool COnTimeCtrl::GetOnTimeHashData(ONTIME_ACT_HASH& hash_node)
{
    bool ret = false;
	HashedOnTimeActInfo::iterator it;
	CTblLock.lock();

	it = m_ptrOnTimeActInfoTable->find(hash_node.Seq);
	if (it != m_ptrOnTimeActInfoTable->end())
	{
		memcpy((BYTE*)&hash_node, (BYTE*)&it->second, sizeof(ONTIME_ACT_HASH));
		ret = true;
	}
	CTblLock.unlock();
	return ret;
}

/*******************************End of file*********************************************/
